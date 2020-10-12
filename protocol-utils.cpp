#include <netinet/in.h>

#include "protocol-utils.hpp"


std::pair<std::string, std::string> parseShoutcastHeader(const std::string& line) {
  std::string headerName = line.substr(0, line.find(':'));
  std::string valueString = line.substr(line.find(':') + 1, line.find('\r'));
  size_t properBeginPosition = 0;
        
  while (valueString[properBeginPosition] == ' ') {
    properBeginPosition++;
  }

  if (properBeginPosition != 0) {
    valueString = valueString.substr(properBeginPosition);
  }

  return std::make_pair(headerName, valueString);
}


MessageHeaderData parseProxyCommunicationHeader(MessageHeader data) {
  MessageHeaderData result;
  result.type = ntohs(data.typeNetworkOrder);
  result.length = ntohs(data.lengthNetworkOrder);

  return result;
}


MessageHeader encryptProxyCommunicationHeader(MessageHeaderData data) {
  MessageHeader result;
  result.typeNetworkOrder = htons(data.type);
  result.lengthNetworkOrder = htons(data.length);

  return result;
}


MessageHeader constructMessageHeader(uint16_t type, uint16_t length) {
  MessageHeaderData data;
  data.type = type;
  data.length = length;

  MessageHeader result = encryptProxyCommunicationHeader(data);
  return result;
}

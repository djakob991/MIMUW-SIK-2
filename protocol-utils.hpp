/*
 * Zestaw struktur i funkcji pomagających w obsłudze protokołów (Shoutcast 
 * i protokołu pomiędzy radio-proxy a klientem)
 */


#ifndef PROTOCOL_UTILS
#define PROTOCOL_UTILS

#include <iostream>


struct MessageHeader {
  uint16_t typeNetworkOrder;
  uint16_t lengthNetworkOrder;
};

struct MessageHeaderData {
  uint16_t type;
  uint16_t length;
};


std::pair<std::string, std::string> parseShoutcastHeader(const std::string& line);
MessageHeaderData parseProxyCommunicationHeader(MessageHeader data);
MessageHeader encryptProxyCommunicationHeader(MessageHeaderData data);
MessageHeader constructMessageHeader(uint16_t type, uint16_t length);

#endif
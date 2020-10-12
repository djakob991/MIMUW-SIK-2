#include <iostream>
#include <set>
#include <memory>

#include "args-parser.hpp"


bool isNumber(std::string str);

RadioProxyArgsParser::RadioProxyArgsParser(int argc, char *argv[]): 
metadata(false), timeout(5), proxy(false), proxyMulticast(false), proxyTimeout(5) {
  std::set<std::string> present;
  std::set<std::string> toGoObligatory = {"-h", "-r", "-p"};
  
  int flagIndex = 1;
  int valueIndex;

  while (flagIndex < argc) {
    valueIndex = flagIndex + 1;

    if (valueIndex >= argc) {
      throw RadioProxyArgsParserException();
    }
    
    std::string flag = argv[flagIndex];
    std::string value = argv[valueIndex];

    if (present.find(flag) != present.end()) {
      throw RadioProxyArgsParserException(flag + " flag duplicated.");
    }

    auto obligatoryIterator = toGoObligatory.find(flag);
    bool isObligatory = obligatoryIterator != toGoObligatory.end();

    if (flag == "-h") {
      host = validateHost(value);
    
    } else if (flag == "-r") {
      resource = validateResource(value);
    
    } else if (flag == "-p") {
      port = validatePort(value);
    
    } else if (flag == "-m") {
      metadata = validateMetadata(value);
    
    } else if (flag == "-t") {
      timeout = validateTimeout(value);

    } else if (flag == "-P") {
      proxyPort = validateNumber(value);
      proxy = true;

    } else if (flag == "-B") {
      proxyMulticastAddress = validateProxyMulticastAddress(value);
      proxyMulticast = true;

    } else if (flag == "-T") {
      proxyTimeout = validateTimeout(value);

    } else {
      // unknown flag
      throw RadioProxyArgsParserException();
    }

    if (isObligatory) {
      toGoObligatory.erase(obligatoryIterator);
    }

    present.insert(flag);

    flagIndex += 2;
  }

  if (!toGoObligatory.empty()) {
    throw RadioProxyArgsParserException("not all obligatory parameters were given");
  }

  bool multicastPresent = present.find("-B") != present.end();
  bool timeoutPresent = present.find("-T") != present.end();

  if (!proxy and (multicastPresent or timeoutPresent)) {
    throw RadioProxyArgsParserException("missing -P flag");
  }
}


std::string RadioProxyArgsParser::validateHost(std::string str) {
  // Currently no specific host validation
  return str;
}


std::string RadioProxyArgsParser::validateResource(std::string str) {
  // Currently no specific resource validation
  return str;
}


std::string RadioProxyArgsParser::validatePort(std::string str) {
  if (!isNumber(str)) {
    throw RadioProxyArgsParserException(str + "  is not a valid port");
  }

  return str;
}


bool RadioProxyArgsParser::validateMetadata(std::string str) {
  if (str == "yes") {
    return true;
  
  } else if (str == "no") {
    return false;
  }

  throw RadioProxyArgsParserException("-m value must be yes|no");
}


int RadioProxyArgsParser::validateNumber(std::string str) {
  if (!isNumber(str)) {
    throw RadioProxyArgsParserException(str + " is not a valid number");
  }

  try {
    return std::stoi(str);
  } catch (...) {
    throw RadioProxyArgsParserException("error while parsing " + str + " to number");
  }
}


int RadioProxyArgsParser::validateTimeout(std::string str) {
  int result = validateNumber(str);

  if (result == 0) {
    throw RadioProxyArgsParserException("0 is not a valid timeout");
  }

  return result;
}


std::string RadioProxyArgsParser::validateProxyMulticastAddress(std::string str) {
  // currently no validation
  return str;
}


std::string RadioProxyArgsParser::getHost() {
  return host;
}


std::string RadioProxyArgsParser::getResource() {
  return resource;
}


std::string RadioProxyArgsParser::getPort() {
  return port;
}


bool RadioProxyArgsParser::getMetadata() {
  return metadata;
}


int RadioProxyArgsParser::getTimeout() {
  return timeout;
}


bool RadioProxyArgsParser::getProxy() {
  return proxy;
}


int RadioProxyArgsParser::getProxyPort() {
  if (!proxy) throw RadioProxyArgsParserException("Not a proxy");
  return proxyPort;
}


bool RadioProxyArgsParser::getProxyMulticast() {
  if (!proxy) throw RadioProxyArgsParserException("Not a proxy");
  return proxyMulticast;
}


std::string RadioProxyArgsParser::getProxyMulticastAddress() {
  if (!proxy) throw RadioProxyArgsParserException("Not a proxy");
  if (!proxyMulticast) throw RadioProxyArgsParserException("No multicast");
  return proxyMulticastAddress;
}


int RadioProxyArgsParser::getProxyTimeout() {
  if (!proxy) throw RadioProxyArgsParserException("Not a proxy");
  return proxyTimeout;
}


RadioProxyArgsParserException::RadioProxyArgsParserException(std::string message) {
  message_ = message_ + '\n' + message;
}

RadioProxyArgsParserException::RadioProxyArgsParserException() {}


const char * RadioProxyArgsParserException::what () const throw () {
  return message_.c_str();
}


bool isNumber(std::string str) {
  for (char& digit : str) {
    if (digit < '0' or digit > '9') {
      return false;
    }
  }

  if (str[0] == '0' and str.size() > 1) return false;
  return true;
}

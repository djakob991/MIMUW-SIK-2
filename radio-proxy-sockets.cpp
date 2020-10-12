
#include <string.h>
#include <arpa/inet.h>

#include "radio-proxy-sockets.hpp"


int createTCPClientSocket(const std::string& address, const std::string& port, int userTimeout) {
  int sock;
  struct addrinfo addr_hints;
  struct addrinfo *addr_result;

  memset(&addr_hints, 0, sizeof(struct addrinfo));
  addr_hints.ai_family = AF_INET; // IPv4
  addr_hints.ai_socktype = SOCK_STREAM;
  addr_hints.ai_protocol = IPPROTO_TCP;

  int32_t err = getaddrinfo(address.c_str(), port.c_str(), &addr_hints, &addr_result);

  if (err != 0) {
    throw SocketException("getaddrinfo failed");
  }

  sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);

  if (sock < 0) {
    throw SocketException("socket creation failed");
  }

  struct timeval timeout;
  timeout.tv_sec = userTimeout;
  timeout.tv_usec = 0;

  if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
    throw SocketException("setsockopt failed");
  }

  if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0) {
    throw SocketException("connection failed");
  }

  freeaddrinfo(addr_result);

  return sock;
}


int createUDPServerSocket(in_port_t port, const char *multicastAddress) {
  int sock;
  
  struct sockaddr_in local_address;
  struct ip_mreq ip_mreq;

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    throw SocketException("socket creation failed");
  }

  if (multicastAddress != nullptr) {
    ip_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (inet_aton(multicastAddress, &ip_mreq.imr_multiaddr) == 0) {	  
      throw SocketException("invalid multicast address");
    }
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&ip_mreq, sizeof ip_mreq) < 0) {
      throw SocketException("setsockopt failed");
    }
  }

  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
    throw SocketException("setsockopt failed");
  }

  local_address.sin_family = AF_INET;
  local_address.sin_addr.s_addr = htonl(INADDR_ANY);
  local_address.sin_port = htons(port);
  
  if (bind(sock, (struct sockaddr *)&local_address, sizeof local_address) < 0) {
    throw SocketException("bind failed");
  }

  return sock;
}



SocketException::SocketException(std::string message):
message_(message) {}

const char * SocketException::what () const throw () {
  return message_.c_str();
}
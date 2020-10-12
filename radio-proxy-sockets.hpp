#ifndef RADIO_PROXY_SOCKETS
#define RADIO_PROXY_SOCKETS

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <exception>
#include <string>


int createTCPClientSocket(const std::string& address, const std::string& port, int userTimeout);
int createUDPServerSocket(in_port_t port, const char *multicastAddress);


class SocketException : public std::exception {
public:
  SocketException(std::string message);
  const char * what () const throw ();

private:
  std::string message_;
};


#endif
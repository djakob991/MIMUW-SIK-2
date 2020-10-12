#include <unistd.h>

#include "socket-raii.hpp"

SocketRAII::~SocketRAII() {
  close(socket_);
}


int SocketRAII::getSocket() {
  return socket_;
}
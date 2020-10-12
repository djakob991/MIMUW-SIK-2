#ifndef SOCKET_RAII_HPP
#define SOCKET_RAII_HPP


/*
 * Wrapper na socket. W destruktorze wykonuje close(). 
 */
class SocketRAII {
public:
  SocketRAII(int socket): socket_(socket) {}
  ~SocketRAII();
  int getSocket();

private:
  int socket_;
};

#endif
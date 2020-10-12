#ifndef ARGS_PARSER_HPP
#define ARGS_PARSER_HPP


#include <exception>
#include <string>

/*
 * Ta klasa w konstruktorze przyjmuje argumenty z linii poleceń programu radio-proxy,
 * parsuje je i waliduje, a następnie udostępnia odpowiednie wartości poprzez gettery.
 * Wszelkie nieprawidłowości sygnalizowane są przez rzucenie wyjątku. 
 */
class RadioProxyArgsParser {
public:  
  RadioProxyArgsParser(int argc, char *argv[]);
  std::string getHost();
  std::string getResource();
  std::string getPort();
  bool getMetadata();
  int getTimeout();
  
  bool getProxy();
  int getProxyPort();
  bool getProxyMulticast();
  std::string getProxyMulticastAddress();
  int getProxyTimeout();

private:
  std::string host;
  std::string resource;
  std::string port;
  bool metadata;
  int timeout;
  
  bool proxy;
  int proxyPort;
  bool proxyMulticast;
  std::string proxyMulticastAddress;
  int proxyTimeout;

  std::string validateHost(std::string str);
  std::string validateResource(std::string str);
  std::string validatePort(std::string str);
  bool validateMetadata(std::string str);
  std::string validateProxyMulticastAddress(std::string str);
  int validateNumber(std::string str);
  int validateTimeout(std::string str);
};


class RadioProxyArgsParserException : public std::exception {
public:
  RadioProxyArgsParserException(std::string message);
  RadioProxyArgsParserException();
  const char * what () const throw ();

private:
  std::string message_ = 
    "Usage: ./radio-proxy -h host -r resource -p port [-m yes|no] [-t timeout] [-P port [-B multi] [-T timeout]]";
};


#endif
#include <iostream>
#include <map>
#include <memory>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <csignal>
#include <atomic>
#include <poll.h>

#include <thread>
#include <chrono>
#include <mutex>

#include "args-parser.hpp"
#include "radio-proxy-sockets.hpp"
#include "socket-raii.hpp"
#include "thread-raii.hpp"
#include "protocol-utils.hpp"

#define DISCOVER 1
#define IAM 2
#define KEEPALIVE 3
#define AUDIO 4
#define METADATA 5

#define SEND_BLOCK_SIZE 65000

using addr_data = std::pair<in_addr_t, in_port_t>;
using addr_map = std::map<addr_data, uint64_t>;


std::atomic<bool> interrupted(false);

void handlerSIGINT(int) {
  interrupted.store(true);
}

struct sockaddr_in restoreAddress(in_addr_t address, in_port_t port);
uint64_t currentTime();

/*
 * Główna funkcja drugiego wątku. Obsługuje komunikaty UDP od klientów i 
 * aktualizuje mapę adresów.
 */
void handleUDPMessages(
  std::shared_ptr<SocketRAII> udp, 
  addr_map& map, 
  std::mutex& mutex,
  RadioProxyArgsParser& parser
); 

/* Wysyła wiadomość z buffera do klientów zapamiętanych w mapie adresów. */
void sendToClients(
  int socket,
  char *buffer,
  size_t length,
  RadioProxyArgsParser& parser,
  addr_map& addressesMap,
  std::mutex& mutex
);


int main(int argc, char *argv[]) {
  std::signal(SIGINT, handlerSIGINT);

  try {
    RadioProxyArgsParser parser(argc, argv);
    SocketRAII tcp(createTCPClientSocket(
      parser.getHost(), parser.getPort(), parser.getTimeout()
    ));
    
    /* Gniazdo UDP i drugi wątek będą stworzone tylko, gdy parser.getProxy() == true */
    addr_map addressesMap;
    std::mutex mutex;
    std::shared_ptr<SocketRAII> udp;
    std::unique_ptr<ThreadRAII> udpReceiverThread;
    
    if (parser.getProxy()) {
      const char *multicastAddress = nullptr;
      
      if (parser.getProxyMulticast()) {
        multicastAddress = parser.getProxyMulticastAddress().c_str();
      }

      udp = std::make_shared<SocketRAII>(createUDPServerSocket(
        (in_port_t)parser.getProxyPort(),
        multicastAddress
      ));

      udpReceiverThread = std::make_unique<ThreadRAII>(
        std::thread(
          handleUDPMessages, 
          udp, 
          std::ref(addressesMap), 
          std::ref(mutex),
          std::ref(parser)
        ), 
        ThreadRAII::DtorAction::join
      );
    }

    /* Tworzenie i wysyłanie requestu http */
    std::string request = "";
    request += "GET " + parser.getResource() + " HTTP/1.1\r\n";
    request += "Host: " + parser.getHost() + "\r\n";
    if (parser.getMetadata()) request += "Icy-MetaData: 1\r\n";
    request += "\r\n";

    if (write(tcp.getSocket(), request.c_str(), request.size()) < 0) {
      std::cerr << "write failed\n";
      return 1;
    }


    /* Otwarcie gniazda jak pliku: będzie można używać fread */
    FILE *stream = fdopen(tcp.getSocket(), "r");

    /* Wczytywanie nagłówka i headerów response */
    char *lineptr = nullptr;
    size_t bytes = 0;
    size_t blockSize = 8000;
    bool metadata = false;
    bool status = true;

    while (true) {
      ssize_t r = getline(&lineptr, &bytes, stream);

      if (r < 0) {
        std::cerr << "getline failed\n";
        fclose(stream);
        free(lineptr);
        return 1;
      }

      std::string line(lineptr);

      if (status) {
        status = false;

        if (!(
          line == "ICY 200 OK\r\n" 
          or line == "HTTP/1.0 200 OK\r\n" 
          or line == "HTTP/1.1 200 OK\r\n"
        )) {
          std::cerr << line;
          fclose(stream);
          free(lineptr);
          return 1;
        }
      }

      if (line == "\r\n") {
        break;
      }

      auto parsed = parseShoutcastHeader(line);

      if (parsed.first == "icy-metaint") {
        blockSize = std::stoi(parsed.second);
        metadata = true;
      }
    }

    free(lineptr);

    if (!parser.getMetadata() and metadata) {
      std::cerr << "metadata present but not requested\n";
      fclose(stream);
      return 1;
    }


    /*
     * Główna pęla tego wątku w każdym obrocie wczytuje blok danych audio lub
     * metadanych, a następnie wypisuje dane na standardowe wyjście lub
     * rozsyła klientom przez UDP. Pętla działa dopóki nie zajdzie błąd
     * lub zostanie przerwana przez SIGINT.
     */ 

    char *buffer = new char[blockSize]; // bufor na dane audio
    char metadataBuffer[5000]; // bufor na metadane
    char sendBuffer[SEND_BLOCK_SIZE + 4]; // bufor na dane do wysłania

    while (!interrupted.load()) {
      /* Wczytanie danych audio */
      size_t r = fread(buffer, 1, blockSize, stream);
      
      if (r < blockSize) {
        break;
      }
      
      if (!parser.getProxy()) {
        for (size_t i=0; i<r; i++) {
          std::cout << buffer[i];
        }
      
      } else {
        /* Dane przed wysłaniem dzielone są na chunki nie większe niż SEND_BLOCK_SIZE */

        size_t bytesLeft = blockSize;
        size_t index = 0;

        while (bytesLeft > 0) {
          size_t chunk = std::min((int)bytesLeft, SEND_BLOCK_SIZE);
          MessageHeader messageHeader = constructMessageHeader(AUDIO, chunk);

          memcpy(sendBuffer, &messageHeader, 4);
          memcpy(sendBuffer + 4, buffer + index, chunk);

          sendToClients(
            udp->getSocket(),
            sendBuffer,
            chunk + 4,
            parser,
            addressesMap,
            mutex
          );          

          bytesLeft -= chunk;
          index += chunk;
        }
      }

      if (metadata) {
        /* Wczytanie bajtu oznaczającego długość metadanych */
        char metadataLengthByte;
        size_t r = fread(&metadataLengthByte, 1, 1, stream);
        if (r == 0) break;

        size_t metadataLength = (size_t)metadataLengthByte * 16;
        if (metadataLength == 0) continue;

        /* Wczytanie metadanych */
        r = fread(metadataBuffer, 1, metadataLength, stream);
        if (r < metadataLength) break;

        if (!parser.getProxy()) {
          for (size_t i=0; i<r; i++) {
            std::cerr << metadataBuffer[i];
          }
        
        } else {
          /* Długość metadanych nie przekroczy SEND_BLOCK_SIZE, nie trzeba dzielić na chunki */

          MessageHeader messageHeader = constructMessageHeader(
            METADATA, metadataLength
          );

          memcpy(sendBuffer, &messageHeader, 4);
          memcpy(sendBuffer + 4, metadataBuffer, metadataLength);

          sendToClients(
            udp->getSocket(),
            sendBuffer,
            metadataLength + 4,
            parser,
            addressesMap,
            mutex
          );   
        }
      }
    }

    /* Pętla się skończyła - na skutek błędu lub SIGINTa */
    bool interruptedBySignal = interrupted.load();
    /* Upewnienie, że interrupted = true, aby na pewno przerwana została pętla w 2 wątku */
    interrupted.store(true); 

    fclose(stream);
    delete[] buffer;
    
    if (interruptedBySignal) {
      return 0;
    } else {
      return 1;
    }
  
  } catch (std::exception& e) {
    std::cout << e.what() << '\n';
    return 1;
  }
}


void handleUDPMessages(
  std::shared_ptr<SocketRAII> udp, 
  addr_map& map, 
  std::mutex& mutex,
  RadioProxyArgsParser& parser
) {
  MessageHeader request;
  struct sockaddr_in client_address;
  socklen_t client_address_length = sizeof(client_address);
  char sendBuffer[SEND_BLOCK_SIZE + 4];

  while (!interrupted.load()) {
    ssize_t r = recvfrom(
      udp->getSocket(),
      &request,
      4,
      0,
      (struct sockaddr *) &client_address,
      &client_address_length
    );

    /* Odebrana wiadomość jest poprawna tylko jeśli składa się z 2 2-bajtowych liczb ... */
    if (r != 4) {
      continue;
    }

    MessageHeaderData requestData = parseProxyCommunicationHeader(request);

    /* ... z których druga musi być zerem */
    if (requestData.length != 0) {
      continue;
    }

    /* Jeśli odebrano wiadomość DISCOVER, odeślij wiadomość IAM */
    if (requestData.type == DISCOVER) {
      std::string body = "";
      body += "host=" + parser.getHost() + ';';
      
      MessageHeader responseHeader = constructMessageHeader(IAM, body.size());

      memcpy(sendBuffer, &responseHeader, 4);
      memcpy(sendBuffer + 4, (char *)body.c_str(), body.size());

      sendto(
        udp->getSocket(),
        sendBuffer,
        body.size() + 4,
        0,
        (struct sockaddr *) &client_address,
        client_address_length
      );
    }

    /* Para stworzona z tych 2 wartości będzie kluczem w mapie, i wystarczy do oddtworzenia adresu */
    addr_data addressData = std::make_pair(
      client_address.sin_addr.s_addr,
      client_address.sin_port
    );
    
    /* Aktualizacja mapy adresów (pod zamkniętym mutexem) */
    mutex.lock();   
    bool addressPresent = map.find(addressData) != map.end();
    uint64_t time = currentTime();

    if (requestData.type == DISCOVER) {
      map[addressData] = time;
    }

    if (requestData.type == KEEPALIVE and addressPresent) {
      uint64_t diff = time - map[addressData];

      if (diff <= (uint64_t)parser.getProxyTimeout() * 1000) {
        map[addressData] = time;
      }
    }

    mutex.unlock();
  }
}


void sendToClients(
  int socket,
  char *buffer,
  size_t length,
  RadioProxyArgsParser& parser,
  addr_map& addressesMap,
  std::mutex& mutex
) {
  uint64_t time = currentTime();
  mutex.lock();

  /* Iteracja po mapie adresów */
  for (auto it = addressesMap.begin(); it != addressesMap.end();) {
    uint64_t diff = time - it->second;

    if (diff > (uint64_t)parser.getProxyTimeout() * 1000) {
      /* Jeśli od ostatniego zameldowania danego adresu minęło za dużo czasu, usuń go z mapy */
      it = addressesMap.erase(it);
      continue;
    }
    
    /* W przeciwnym przypadku wyślij do niego wiadomość */
    struct sockaddr_in client_address = restoreAddress(
      it->first.first, it->first.second
    );
    socklen_t client_address_length = sizeof(client_address);

    sendto(
      socket,
      buffer,
      length,
      0,
      (const struct sockaddr*)&client_address,
      client_address_length
    );

    it++;
  }

  mutex.unlock();
}


struct sockaddr_in restoreAddress(in_addr_t address, in_port_t port) {
  struct sockaddr_in result;

  result.sin_family = AF_INET;
  result.sin_addr.s_addr = address;
  result.sin_port = port;

  return result;
}


uint64_t currentTime() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()
  ).count();
}
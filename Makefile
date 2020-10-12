CFLAGS = -Wall -Wextra -O2
LDFLAGS = -pthread

.PHONY: all clean

all: radio-proxy

radio-proxy: radio-proxy.o args-parser.o radio-proxy-sockets.o thread-raii.o protocol-utils.o socket-raii.o
	g++ $(LDFLAGS) -o $@ $^

radio-proxy.o: radio-proxy.cpp args-parser.hpp radio-proxy-sockets.hpp thread-raii.hpp protocol-utils.hpp socket-raii.hpp
	g++ $(CFLAGS) -c $<

args-parser.o: args-parser.cpp
	g++ $(CFLAGS) -c $<

radio-proxy-sockets.o: radio-proxy-sockets.cpp
	g++ $(CFLAGS) -c $<

thread-raii.o: thread-raii.cpp
	g++ $(CFLAGS) -c $<

socket-raii.o: socket-raii.cpp
	g++ $(CFLAGS) -c $<

protocol-utils.o: protocol-utils.cpp
	g++ $(CFLAGS) -c $<

clean:
	rm -rf *.o radio-proxy
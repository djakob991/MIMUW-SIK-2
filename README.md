## Usage

```
./radio-proxy <parameters>

Parameters:
-h <host>
-r <resource>
-p <port>
-m yes|no ('yes', if program should demand metadata from server. 'no' by default.)
-t <timeout> (time in seconds until the program considers server to not work. 5 by default)
-P <port> (Port on which program listens for UDP messages from client and from which it sends data to clients. Optional)
-B <multi> (Multicast address on which the program will listen)
-T <timeout> (Time in seconds until the program decides no to send data to the client anymore, if the client doesn't send any messages)
```

The repository contains the test client that can communicate with radio-proxy by UDP.

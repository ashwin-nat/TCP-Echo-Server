# TCP Echo server

This is yet another implementation of a simple TCP echo server. This echo server is capable of supporting multiple concurrent connections (configurable by modifying a macro constant in the source code).

But unlike most other implementations out there (at least on the first page of a google search), this implementation handles all these simultaneous connections without using [fork()](https://man7.org/linux/man-pages/man2/fork.2.html) or any multithreading. 

This server uses the [poll()](https://man7.org/linux/man-pages/man2/poll.2.html) system call to multiplex all connections on the single main thread.

## Building

Use the provided make file to build the code.

```bash
make
```
To build a debuggable executable, use the below command

```bash
make debug
```

## Usage

```bash
./echo_server <port_num>
```

Use any telnet client as a TCP connection to send out messages and receive the (echo) responses.


```bash
telnet <server_ip_addr> <server_port>
```

## Credit
This readme file was formatted using [www.makeareadme.com](www.makeareadme.com)
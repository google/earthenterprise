/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */



#ifndef __TCPINETSOCKET_H
#define __TCPINETSOCKET_H

#include "InetSocket.h"
#include <string>
#include <errno.h>


/******************************************************************************
 ***  TCPInetSocket
 ***
 ***  Low level C++ abstraction of BSD sockets (using TCP)
 ***
 ***  As this is intended to be a low-level abstraction, the error return
 ***  semantics have remained unchanged. In particular, NO exceptions are
 ***  thrown. And error values are still placed in errno.
 ******************************************************************************/
class TCPInetSocket : public InetSocket {
 public:
  explicit TCPInetSocket(InetAddr::Family family) throw() {
    (void) open(family, TCP);
  }
  explicit TCPInetSocket(const TCPInetSocket &listenSock) throw() {
    (void) acceptOpen(listenSock);
  }

  int maxPayload(void) const throw() { return maxTCPPayload(); }

  bool acceptOpen(const TCPInetSocket &listenSock) throw() {
    if (isOpen()) (void)close();
    do {
      sock = ::accept(listenSock.sock, 0, 0);
    } while ( (sock == -1) && (errno == EINTR) );
    return isOpen();
  }

  bool listen(int backlog = SOMAXCONN) throw() {
    return (::listen(sock, backlog) == 0);
  }    
};


/******************************************************************************
 ***  TCPListener
 ***
 ***  Higher level abstraction. This one can throw exceptions!
 ******************************************************************************/
class TCPListener : public TCPInetSocket {
 public:
  explicit TCPListener(const SockAddr &addr);
};


/******************************************************************************
 ***  TCPConnection
 ***
 ***  Higher level abstraction. This one can throw exceptions!
 ******************************************************************************/
class TCPConnection : public TCPInetSocket {
 public:
  // Used by a TCP server to accept a connection from a listening socket
  explicit TCPConnection(const TCPListener &listener);

  // Used by a TCP client to connect to a server
  explicit TCPConnection(const SockAddr &hostAddr);
  TCPConnection(const std::string &hostname,
                in_port_t port, InetAddr::Family = InetAddr::IPv4);

  // these will send/recv all or throw an exception
  // timeout <= 0 means don't timeout, but wait forever
  ssize_t sendall(const void *buf, size_t count, int flags, int timeout);
  ssize_t recvall(void *buf, size_t count, int flags, int timeout);
};

#endif /* __TCPINETSOCKET_H */

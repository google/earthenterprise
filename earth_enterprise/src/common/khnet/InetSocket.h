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


#ifndef __INETSOCKET_H
#define __INETSOCKET_H

#include "InetAddr.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <cerrno>

/******************************************************************************
 ***  InetSocket
 ***
 ***  Low level C++ abstraction of BSD sockets
 ***
 ***  InetSocket should never be instantiated directly (note the protected
 ***  constructor & destructor). Instead you should use the one of the
 ***  derived classes TCPListener, TCPConnection, UDPReceiver, etc.
 ***
 ***  The purpose of this class is twofold:
 ***  1) Implement self-closing semantics to simplify exception handling
 ***  2) Provide a "slightly" simplified API:
 ***    a) 'InetAddr' instead of 'struct sockaddr*'
 ***    b) 'bool' return values
 ***
 ***  As a side-effect of the self-closing semantics, InetSockets must never
 ***  be copied or assigned. This is enforced in the following ways:
 ***  1) both the copy constructor and the assignment operator are private
 ***     and unimplemented.
 ***  2) the global 'socket' function has been implemented as InetSocket::open()
 ***  3) the global 'accept' function has been implemented as
 ***     InetSocket::acceptOpen()
 ***
 ***  As this is intended to be a low-level abstraction, the error return
 ***  semantics have remained unchanged. In particular, NO exceptions are
 ***  thrown. And error values are still placed in errno.
 ******************************************************************************/
class InetSocket {
  friend class SocketTimeoutGuard_;

  // prohibit compiler generated versions of these.
  // we don't want any copying of InetSocket.
  InetSocket(const InetSocket&);
  InetSocket& operator=(const InetSocket&);

 protected:
  int sock;

  InetSocket(void) throw() : sock(-1) { }
  ~InetSocket(void) throw() { if (isOpen()) close(); }

 public:
  bool isOpen(void) const throw()   { return (sock != -1);}
  operator bool(void) const throw() { return isOpen();}
  inline bool operator==(const InetSocket &o) const {
    return sock == o.sock;
  }
  inline bool operator!=(const InetSocket &o) const {
    return !operator==(o);
  }

  enum SockType {TCP = SOCK_STREAM, UDP = SOCK_DGRAM};

  bool open(InetAddr::Family family, SockType sockType) throw() {
    if (isOpen()) (void)close();
    switch (family) {
      case InetAddr::Invalid:
      default:
        sock = -1;
        errno = EPFNOSUPPORT;
        break;
      case InetAddr::IPv4:
        sock = ::socket(AF_INET, sockType, 0);
        break;
      case InetAddr::IPv6:
        sock = ::socket(AF_INET6, sockType, 0);
        break;
    }
    return isOpen();
  }

  bool setReuseAddr(void) throw() {
    const int on = 1;
    return (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))==0);
  }

  bool setNonBlocking(bool wantNonBlocking = true) throw() {
    int flags = ::fcntl(sock, F_GETFL, 0);
    if (flags == -1) return false;
    if (wantNonBlocking)
      flags |= O_NONBLOCK;
    else
      flags &= ~O_NONBLOCK;
    return (::fcntl(sock, F_SETFL, flags) == 0);
  }

  bool setCloseOnExec(bool wantCloseOnExec = true) throw() {
    int flags = ::fcntl(sock, F_GETFD, 0);
    if (flags == -1) return false;
    if (wantCloseOnExec)
      flags |= FD_CLOEXEC;
    else
      flags &= ~FD_CLOEXEC;
    return (::fcntl(sock, F_SETFD, flags) == 0);
  }

  bool bind(const SockAddr &addr) throw() {
    return (::bind(sock, addr.rawSockaddr(), sizeof(addr)) == 0);
  }

  bool connect(const SockAddr &addr) throw() {
    return (::connect(sock, addr.rawSockaddr(), sizeof(addr)) == 0);
  }

  bool close(void) throw() {
    int s = sock;
    sock = -1;
    return (::close(s) == 0);
  }

  enum Flags { Peek     = MSG_PEEK,
               DontWait = MSG_DONTWAIT,
               WaitAll  = MSG_WAITALL };

  ssize_t send(const void *buf, size_t count, int flags = 0) throw() {
    // MSG_NOSIGNAL - return EPIPE rather than emit SIGPIPE
    return ::send(sock, buf, count, flags | MSG_NOSIGNAL);
  }
  
  ssize_t send(iovec *iov, size_t num, int flags = 0) throw() {
    struct msghdr msg = {
      NULL, 0,                  // dest address
      iov, num,
      NULL, 0,                  // control buffer
      0,                        // return flags (for recvmsg only)
    };

    // MSG_NOSIGNAL - return EPIPE rather than emit SIGPIPE
    return ::sendmsg(sock, &msg, flags | MSG_NOSIGNAL);
  }
  
  ssize_t recv(void *buf, size_t count, int flags = 0) throw() {
    return ::recv(sock, buf, count, flags);
  }

  ssize_t recv(iovec *iov, size_t num, int flags = 0) throw() {
    struct msghdr msg = {
      NULL, 0,                  // dest address
      iov, num,
      NULL, 0,                  // control buffer
      0,                        // return flags (for recvmsg only)
    };

    return ::recvmsg(sock, &msg, flags);
  }

  int mtu(void) const throw();
  int maxIPPayload(void) const throw();
  int maxUDPPayload(void) const throw() { return (maxIPPayload() - 8); }
  int maxTCPPayload(void) const throw() { return (maxIPPayload() - 20); }

  struct PollFD {
    const InetSocket &sock;
    short events;
    short revents;
  };

  static int poll(PollFD *fds, unsigned int num, int timeout) throw();
  bool getSockAddr(SockAddr &sockAddr) const throw();
  bool getPeerAddr(SockAddr &sockAddr) const throw();

  // timeout is in milliseconds, negative value means wait forever
  bool waitUntilReadable(int timeout = -1) throw();

  std::string getSockName(void) const throw() {
    SockAddr addr;
    if (getSockAddr(addr)) {
      return addr.toString();
    } else {
      return std::string();
    }
  }

  std::string getPeerName(void) const throw() {
    SockAddr addr;
    if (getPeerAddr(addr)) {
      return addr.toString();
    } else {
      return std::string();
    }
  }
};



#endif /* __INETSOCKET_H */

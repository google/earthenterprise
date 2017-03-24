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



#ifndef __UDPINETSOCKET_H
#define __UDPINETSOCKET_H

#include "InetSocket.h"

class MulticastJoinSpec {
  friend class UDPInetSocket;

  InetAddr::Family fam;
  union {
    ip_mreq   ipv4;
    ipv6_mreq ipv6;
  };
 public:
  InetAddr::Family family(void) const throw() { return fam;}
  bool operator==(const MulticastJoinSpec &o) const throw();

  const ip_mreq& getIPv4MReq(void) const throw() {
    return ipv4;
  }
  const ipv6_mreq& getIPv6MReq(void) const throw() {
    return ipv6;
  }

  MulticastJoinSpec(const InetAddr &mcast, const InetInterface &inter) throw();
};


/******************************************************************************
 ***  UDPInetSocket
 ***
 ***  Low level C++ abstraction of BSD sockets (using UDP)
 ***
 ***  As this is intended to be a low-level abstraction, the error return
 ***  semantics have remained unchanged. In particular, NO exceptions are
 ***  thrown. And error values are still placed in errno.
 ***  
 ***  To keep this abstraction lightweight, all member functions are inline!
 ******************************************************************************/
class UDPInetSocket : public InetSocket {
 public:
  explicit UDPInetSocket(InetAddr::Family family) throw() {
    open(family, UDP);
  }

  int maxPayload(void) const throw() { return maxUDPPayload(); }

  bool multicastJoin(const MulticastJoinSpec &join);
  bool multicastLeave(const MulticastJoinSpec &join);

  ssize_t sendto(const SockAddr &to, const void *buf, size_t count,
                 int flags = 0) throw() {
    // MSG_NOSIGNAL - return EPIPE rather than emit SIGPIPE
    return ::sendto(sock, buf, count, flags | MSG_NOSIGNAL,
                    to.rawSockaddr(), sizeof(to));
  }
  
  ssize_t sendto(const SockAddr &to, iovec *iov, size_t num,
                 int flags = 0) throw() {
    struct msghdr msg = {
      // the struct doesn't declare it const, but that's how it's used
      const_cast<sockaddr*>(static_cast<const sockaddr*>(to.rawSockaddr())),
      sizeof(to), // dest address
      iov, num,
      NULL, 0,                  // control buffer
      0,                        // return flags (for recvmsg only)
    };

    // MSG_NOSIGNAL - return EPIPE rather than emit SIGPIPE
    return ::sendmsg(sock, &msg, flags | MSG_NOSIGNAL);
  }
  
  ssize_t recvfrom(SockAddr &from, void *buf, size_t count,
                   int flags = 0) throw();
  ssize_t recvfrom(SockAddr &from, iovec *iov, size_t num,
                   int flags = 0) throw();
};


/******************************************************************************
 ***  UDPInetReceiver
 ***
 ***  Higher level abstraction. This one can throw exceptions!
 ***
 ******************************************************************************/
class UDPInetReceiver : public UDPInetSocket {
 public:
  explicit UDPInetReceiver(const SockAddr &addr);
};



#endif /* __UDPSOCKET_H */

// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include "InetSocket.h"
#include <iostream>

int
InetSocket::mtu(void) const throw()
{
  // TODO: Use ioctl(SIOCGIFMTU) to query MTU, but this is per device, not
  // per socket so it would be good to just query those interfaces when we
  // start up, then be able to lookup the interface from the address or
  // index
  //
  // ... see commoncpp/src/network.cpp
  return 1500;
}

int
InetSocket::maxIPPayload(void) const throw()
{
  SockAddr addr;
  if (getSockAddr(addr)) {
    switch (addr.family()) {
      case InetAddr::Invalid:
      default:
        break;
      case InetAddr::IPv4:
        return (mtu() - 20);
      case InetAddr::IPv6:
        return (mtu() - 40);
    }
  }
  return 0;
}

int
InetSocket::poll(PollFD *fds, unsigned int num, int timeout) throw() {
  if (num > 0) {
    pollfd rfds[num];
    for (unsigned int i = 0; i < num; ++i) {
      rfds[i].fd = fds[i].sock.sock;
      rfds[i].events = fds[i].events;
    }
    int ret = ::poll(rfds, num, timeout);
    if (ret > 0) {
      for (unsigned int i = 0; i < num; ++i) {
        fds[i].revents = rfds[i].revents;
      }
    }
    return ret;
  } else {
    return 0;
  }
}

bool
InetSocket::waitUntilReadable(int timeout) throw() {
  PollFD me = {
    *this,
    POLLIN,
    0
  };
  int ret = poll(&me, 1, timeout);
  return (ret == 1);
}

bool
InetSocket::getSockAddr(SockAddr &sockAddr) const throw() {
  union {
    sockaddr     base;
    sockaddr_in  ipv4;
    sockaddr_in6 ipv6;
  } u;
  socklen_t addrLen = sizeof(u);
  if (::getsockname(sock, &u.base, &addrLen) == 0) {
    switch (u.base.sa_family) {
      case AF_INET:
        sockAddr.setAddr(u.ipv4);
        return true;
      case AF_INET6:
        sockAddr.setAddr(u.ipv6);
        return true;
    }
  }
  return false;
}

bool
InetSocket::getPeerAddr(SockAddr &sockAddr) const throw() {
  union {
    sockaddr     base;
    sockaddr_in  ipv4;
    sockaddr_in6 ipv6;
  } u;
  socklen_t addrLen = sizeof(u);
  if (::getpeername(sock, &u.base, &addrLen) == 0) {
    switch (u.base.sa_family) {
      case AF_INET:
        sockAddr.setAddr(u.ipv4);
        return true;
      case AF_INET6:
        sockAddr.setAddr(u.ipv6);
        return true;
    }
  }
  return false;
}

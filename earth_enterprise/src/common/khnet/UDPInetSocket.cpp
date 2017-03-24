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

#include "UDPInetSocket.h"
#include "SocketException.h"


/******************************************************************************
 ***  MulticastJoinSpec
 ******************************************************************************/
bool
MulticastJoinSpec::operator==(const MulticastJoinSpec &o) const throw() {
  if (fam == o.fam) {
    switch (fam) {
      case InetAddr::Invalid:
      default:
        return false;
      case InetAddr::IPv4:
        return (memcmp(&ipv4, &o.ipv4, sizeof(ipv4)) == 0);
      case InetAddr::IPv6:
        return (memcmp(&ipv6, &o.ipv6, sizeof(ipv6)) == 0);
    }
  } else {
    return false;
  }
}

MulticastJoinSpec::MulticastJoinSpec(const InetAddr &mcast,
                                     const InetInterface &inter) throw()
    : fam(mcast.family())
{
  if (fam != inter.family()) {
    fam = InetAddr::Invalid;
  } else {
    switch (fam) {
      case InetAddr::Invalid:
      default:
        // fam is already invalid - just leave it that way
        break;
      case InetAddr::IPv4:
        ipv4.imr_multiaddr = mcast;
        ipv4.imr_interface = inter.getIPv4Interface();
        break;
      case InetAddr::IPv6:
        ipv6.ipv6mr_multiaddr = mcast;
        ipv6.ipv6mr_interface = inter.getIPv6Interface();
        break;
    }
  }
}


/******************************************************************************
 ***  UDPInetSocket
 ******************************************************************************/
bool
UDPInetSocket::multicastJoin(const MulticastJoinSpec &join)
{
  switch (join.family()) {
    case InetAddr::Invalid:
    default:
      return false;
    case InetAddr::IPv4: {
      return (::setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                           &join.getIPv4MReq(), sizeof(ip_mreq)) == 0);
    }
    case InetAddr::IPv6: {
      return (::setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
                           &join.getIPv6MReq(), sizeof(ipv6_mreq)) == 0);
    }
  }
}

bool
UDPInetSocket::multicastLeave(const MulticastJoinSpec &join)
{
  switch (join.family()) {
    case InetAddr::Invalid:
    default:
      return false;
    case InetAddr::IPv4: {
      return (::setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                           &join.getIPv4MReq(), sizeof(ip_mreq)) == 0);
    }
    case InetAddr::IPv6: {
      return (::setsockopt(sock, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP,
                           &join.getIPv6MReq(), sizeof(ipv6_mreq)) == 0);
    }
  }
}

ssize_t
UDPInetSocket::recvfrom(SockAddr &from, void *buf, size_t count,
                        int flags) throw()
{
  union {
    sockaddr     base;
    sockaddr_in  ipv4;
    sockaddr_in6 ipv6;
  } u;
  socklen_t addrLen = sizeof(u);
  ssize_t read = ::recvfrom(sock, buf, count, flags, &u.base, &addrLen);
  if (read != -1) {
    switch (u.base.sa_family) {
      case AF_INET:
        from.setAddr(u.ipv4);
        break;
      case AF_INET6:
        from.setAddr(u.ipv6);
        break;
      default:
        read = -1;
        errno = ENOMSG;
        break;
    }
  }
  return read;
}

ssize_t
UDPInetSocket::recvfrom(SockAddr &from, iovec *iov, size_t num,
                        int flags) throw()
{
  union {
    sockaddr     base;
    sockaddr_in  ipv4;
    sockaddr_in6 ipv6;
  } u;
  struct msghdr msg = {
    &u.base, sizeof(u),       // dest address
    iov, num,
    NULL, 0,                  // control buffer
    0,                        // return flags (for recvmsg only)
  };

  ssize_t read = ::recvmsg(sock, &msg, flags);
  if (read != -1) {
    switch (u.base.sa_family) {
      case AF_INET:
        from.setAddr(u.ipv4);
        break;
      case AF_INET6:
        from.setAddr(u.ipv6);
        break;
      default:
        read = -1;
        errno = ENOMSG;
        break;
    }
  }
  return read;
}



/******************************************************************************
 ***  UDPInetReceiver
 ******************************************************************************/
UDPInetReceiver::UDPInetReceiver(const SockAddr &addr)
    : UDPInetSocket(addr.family())
{
  if (!isOpen() || !bind(addr))
    throw SocketException(kh::no_tr("open"));
}

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



#include "InetAddr.h"
#include <netdb.h>
#include <arpa/inet.h>
#include <endian.h>
#include <stdio.h>

/******************************************************************************
 ***  InetAddr
 ******************************************************************************/

// endian symetric (all 0's and all 1's)
static const in_addr  csIPv4Any = { INADDR_ANY };
static const in_addr  csIPv4Broadcast = { INADDR_BROADCAST };

#if __BYTE_ORDER == __LITTLE_ENDIAN
static const in_addr  csIPv4Loopback = { (in_addr_t)0x0100007f } ;
static const in_addr  csIPv4MulticastAll = { (in_addr_t)0x010000e0 };
#else
static const in_addr  csIPv4Loopback = { (in_addr_t)0x7f000001 } ;
static const in_addr  csIPv4MulticastAll = { (in_addr_t)0xe0000001 };
#endif

static const in6_addr csIPv6Any = IN6ADDR_ANY_INIT;
static const in6_addr csIPv6Loopback = IN6ADDR_LOOPBACK_INIT;
static const in6_addr csIPv6MulticastAll =
{ { { 0xff,0x02,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } } };

const InetAddr InetAddr::InvalidInetAddr;
const InetAddr InetAddr::IPv4Any(csIPv4Any);
const InetAddr InetAddr::IPv4Loopback(csIPv4Loopback);
const InetAddr InetAddr::IPv4Broadcast(csIPv4Broadcast);
const InetAddr InetAddr::IPv4MulticastAll(csIPv4MulticastAll);
const InetAddr InetAddr::IPv6Any(csIPv6Any);
const InetAddr InetAddr::IPv6Loopback(csIPv6Loopback);
const InetAddr InetAddr::IPv6MulticastAll(csIPv6MulticastAll);


InetAddr::InetAddr(const std::string &nameornode) throw()
    : fam(Invalid)
{
  memset(&u, 0, sizeof(u));
  if (!setAddr(nameornode)) {
    (void)setAddrByName(nameornode);
  }
}


bool
InetAddr::setAddr(Family family, const std::string &node) throw() {
  if (node.empty())
    return false;

  switch (family) {
    case Invalid:
      return false;
      break;
    case IPv4:
      if (::inet_pton(AF_INET, node.c_str(), &u.ipv4_addr) <= 0)
        return false;
      break;
    case IPv6:
      if (::inet_pton(AF_INET6, node.c_str(), &u.ipv6_addr) <= 0)
        return false;
      break;
  }
  fam = family;
  return true;
}



bool
InetAddr::setAddrByName(const std::string &hostname) throw() {
  if (hostname.empty())
    return false;


#if defined(__GLIBC__)
  char   hbuf[8192];
  struct hostent hb;
  struct hostent *result = 0;
  int    error = 0;
  int    ret = 0;

  memset(&hbuf, 0, sizeof(hbuf));
  memset(&hb, 0, sizeof(hb));
  ret = gethostbyname_r(hostname.c_str(), &hb, hbuf, sizeof(hbuf),
                        &result, &error);
  if ((ret == 0) && (result != 0)) {
    if (result->h_addr_list && *result->h_addr_list) {
      switch (result->h_addrtype) {
        case AF_INET:
          if (result->h_length == sizeof(u.ipv4_addr)) {
            fam = IPv4;
            memcpy(&u.ipv4_addr, *result->h_addr_list, sizeof(u.ipv4_addr));
            return true;
          }
          break;
        case AF_INET6:
          if (result->h_length == sizeof(u.ipv6_addr)) {
            fam = IPv6;
            memcpy(&u.ipv6_addr, *result->h_addr_list, sizeof(u.ipv6_addr));
            return true;
          }
          break;
        default:
          break;
      }
    }
  }
#else
#error "Missing code for invoking gethostbyname_r"
#endif
  return false;
}



void
InetAddr::generateMulticastGroup(const InetAddr &pattern) throw() {
  *this = pattern;
  switch (fam) {
    case Invalid:
      // just leave it invalid
      break;
    case IPv4: {
      // a.b.c.d -> 239.255.c.d
      in_addr_t tmp = ntohl(u.ipv4_addr.s_addr);
      tmp = 0xefff0000 | (tmp & 0x0000ffff);    
      u.ipv4_addr.s_addr = htonl(tmp);
      break;
    }
    case IPv6:
      u.ipv6_addr.s6_addr[0] = 0xff;        // multicast
      u.ipv6_addr.s6_addr[1] = 0x00 | 0x5;  // isWellknown | site-local
      break;
  }
}


std::string
InetAddr::toString(void) const throw() {
  // convert the sockaddr to a string
  char buf[100];              // more than enough, by far
  switch (fam) {
    case Invalid:
    default:                      // 'default' avoids bogus 'no-return' warnings
      return std::string();
    case IPv4:
      if (::inet_ntop(AF_INET, &u.ipv4_addr, buf, sizeof(buf)) <= 0)
        return std::string();
      return std::string(buf);
    case IPv6:
      if (::inet_ntop(AF_INET6, &u.ipv6_addr, buf, sizeof(buf)) <= 0)
        return std::string();
      return std::string(buf);
  }

}

std::string
SockAddr::toString(void) const throw() {
  // convert the sockaddr to a string
  char buf[100];              // more than enough, by far
  switch (u.base.sa_family) {
    case AF_INET: {
      if (::inet_ntop(AF_INET, &u.ipv4.sin_addr, buf, sizeof(buf)) <= 0)
        return std::string();
      size_t len = strlen(buf);
      snprintf(&buf[len], sizeof(buf)-len, ":%d", ntohs(u.ipv4.sin_port));
      return std::string(buf);
    }
    case AF_INET6: {
      if (::inet_ntop(AF_INET6, &u.ipv6.sin6_addr, buf, sizeof(buf)) <= 0)
        return std::string();
      size_t len = strlen(buf);
      snprintf(&buf[len], sizeof(buf)-len, ".%d", ntohs(u.ipv6.sin6_port));
      return std::string(buf);
    }
    default:
      return std::string();
  }
}

/******************************************************************************
 ***  InetInterface
 ******************************************************************************/
bool
InetInterface::operator==(const InetInterface &o) const throw() {
  if (fam == o.fam) {
    switch (fam) {
      case InetAddr::Invalid:
      default:
        return false;
      case InetAddr::IPv4:
        return memcmp(&ipv4, &o.ipv4, sizeof(ipv4));
      case InetAddr::IPv6:
        return ipv6 == o.ipv6;
    }
  } else {
    return false;
  }
}

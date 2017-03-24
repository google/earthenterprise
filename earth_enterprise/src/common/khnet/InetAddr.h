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


#ifndef __INETADDR_H
#define __INETADDR_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <string>



class InetAddr {
 public:
  enum Family {Invalid = 0, IPv4, IPv6};

 protected:
  Family fam;
  union {
    in_addr  ipv4_addr;
    in6_addr ipv6_addr;
  } u;
  
 public:
  // completely uninitialized, must call setAddr or the like before using
  // setting fam to Invalid is just to help ensure no usage of this structure
  InetAddr(void) throw() : fam(Invalid) {
    memset(&u, 0, sizeof(u));
  }

  // conversions from C structures
  // (implicit contruction allowed)
  InetAddr(const in_addr &addr)  throw() : fam(IPv4) {
    u.ipv4_addr = addr;
  }
  InetAddr(const in6_addr &addr) throw() : fam(IPv6) {
    u.ipv6_addr = addr;
  }

  // this will try to interpret the string as an IP address if that fails
  // it will try it as a hostname, if that fails, the InetAddr is
  // left as Invalid
  InetAddr(const std::string &nameornode) throw();

  // address as "a.b.c.d" or "a:b::d"
  bool setAddr(const std::string &node) throw() {
    return (setAddr(IPv4, node) || setAddr(IPv6, node));
  }
  bool setAddr(Family family, const std::string &node) throw();

  // address as "foo.bar.com", "a.b.c.d" or "a:b::d"
  // system policies define whether "A" or "AAAA" records are searched
  bool setAddrByName(const std::string &hostname) throw();
  bool setAddrByName(Family family, const std::string &hostname) throw() {
    return (setAddrByName(hostname) && (fam == family));
  }

  // accessors
  Family            family(void) const throw() { return fam; }
  operator const  in_addr&(void) const throw() { return u.ipv4_addr; }
  operator const in6_addr&(void) const throw() { return u.ipv6_addr; }
  std::string     toString(void) const throw();

  operator bool() const throw() { return fam != Invalid; }

  bool operator==(const InetAddr &o) const throw() {
    if (fam != o.fam) return false;
    switch (fam) {
      case Invalid:
      default:
        return false;             // invalid's don't even match each other
      case IPv4:
        return (memcmp(&u.ipv4_addr, &o.u.ipv4_addr, sizeof(u.ipv4_addr)) == 0);
      case IPv6:
        return (memcmp(&u.ipv6_addr, &o.u.ipv6_addr, sizeof(u.ipv6_addr)) == 0);
    }
  }
  bool isAny(void) const throw() {
    return ((*this == IPv4Any) || (*this == IPv6Any));
  }

  static const InetAddr& GetAny(Family family) {
    switch (family) {
      case Invalid:
      default:
        return InvalidInetAddr;
      case IPv4:
        return IPv4Any;
      case IPv6:
        return IPv6Any;
    }
  }
  static const InetAddr& GetMulticastAll(Family family) {
    switch (family) {
      case Invalid:
      default:
        return InvalidInetAddr;
      case IPv4:
        return IPv4MulticastAll;
      case IPv6:
        return IPv6MulticastAll;
    }
  }

  // create a semi-unique multicast group address (seed with pattern)
  void generateMulticastGroup(const InetAddr &pattern) throw();

  // convenience addresses
  static const InetAddr InvalidInetAddr;
  static const InetAddr IPv4Any;
  static const InetAddr IPv4Loopback;
  static const InetAddr IPv4Broadcast;
  static const InetAddr IPv4MulticastAll;
  static const InetAddr IPv6Any;
  static const InetAddr IPv6Loopback;
  static const InetAddr IPv6MulticastAll;
};


class SockAddr {
 protected:
  union {
    sockaddr     base;
    sockaddr_in  ipv4;
    sockaddr_in6 ipv6;
  } u;

 public:
  // completely uninitialized, must initialize before using
  // filling with 0's helps us ensure no usage of this structure
  SockAddr(void) throw() {
    memset(&u, 0, sizeof(u));
  }
  void setAddr(const sockaddr_in &addr) throw() {
    u.ipv4 = addr;
  }
  void setAddr(const sockaddr_in6 &addr) throw() {
    u.ipv6 = addr;
  }


  operator bool() const throw() { return u.base.sa_family != AF_UNSPEC; }


  // node address from hostname/ipaddress & port
  explicit SockAddr(const std::string &nameornode, in_port_t port = 0) throw(){
    setAddr(InetAddr(nameornode), port);
  }


  // node address from InetAddr
  explicit SockAddr(const InetAddr &node, in_port_t port = 0) throw() {
    setAddr(node, port);
  }
  void setAddr(const InetAddr &node, in_port_t port = 0) throw() {
    memset(&u, 0, sizeof(u));
    switch (node.family()) {
      case InetAddr::Invalid:
        // memset above will leave sa_family at AF_UNSPEC
        break;
      case InetAddr::IPv4:
        u.base.sa_family = AF_INET;
        u.ipv4.sin_addr  = node;
        u.ipv4.sin_port  = htons(port);
        break;
      case InetAddr::IPv6:
        u.base.sa_family  = AF_INET6;
        u.ipv6.sin6_addr  = node;
        u.ipv6.sin6_port  = htons(port);
        break;
    }
  }


  // node address from another SockAddr
  SockAddr(const SockAddr &o) throw() {
    memcpy(&u, &o.u, sizeof(u));
  }

  SockAddr(const SockAddr &o, in_port_t port) throw() {
    memset(&u, 0, sizeof(u));
    switch (o.u.base.sa_family) {
      case AF_INET:
        setAddr(o.u.ipv4.sin_addr, port);
        break;
      case AF_INET6:
        setAddr(o.u.ipv6.sin6_addr, port);
        break;
      default:
        // memset above will leave sa_family at AF_UNSPEC
        break;
    }
  }

  // accessors
  InetAddr::Family family(void) const throw() {
    switch (u.base.sa_family) {
      case AF_INET:
        return InetAddr::IPv4;
      case AF_INET6:
        return InetAddr::IPv6;
      default:
        return InetAddr::Invalid;
    }
  }
  InetAddr addr(void) const throw() {
    switch (u.base.sa_family) {
      case AF_INET:
        return u.ipv4.sin_addr;
      case AF_INET6:
        return u.ipv6.sin6_addr;
      default:
        return InetAddr();
    }
  }
  in_port_t port(void) const throw() {
    switch (u.base.sa_family) {
      case AF_INET:
        return ntohs(u.ipv4.sin_port);
        break;
      case AF_INET6:
        return ntohs(u.ipv6.sin6_port);
        break; 
      default:
        return 0;
    }
  }
  std::string toString(void) const throw();

  const sockaddr* rawSockaddr(void) const throw() { return &u.base;}
};


class InetInterface {
  InetAddr::Family fam;
  union {
    in_addr      ipv4;
    unsigned int ipv6;
  };
 public:
  InetAddr::Family         family(void) const throw() { return fam; }
  const in_addr& getIPv4Interface(void) const throw() { return ipv4; }
  unsigned int   getIPv6Interface(void) const throw() { return ipv6; }

  bool operator==(const InetInterface &o) const throw();

  InetInterface(const InetAddr &interface) throw() : fam(InetAddr::IPv4) {
    ipv4 = interface;
  }
  InetInterface(unsigned int interface) throw() : fam(InetAddr::IPv6) {
    ipv6 = interface;
  }
};

#endif /* __INETADDR_H */

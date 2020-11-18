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



#include "TCPInetSocket.h"
#include "SocketException.h"
#include "SocketTimeout.h"
#include <errno.h>

/******************************************************************************
 ***  TCPListener
 ******************************************************************************/
TCPListener::TCPListener(const SockAddr &addr)
    : TCPInetSocket(addr.family())
{
  if (!isOpen())
    throw SocketException(kh::no_tr("socket open"));

  (void)setReuseAddr();

  if (!bind(addr))
    throw SocketException(kh::no_tr("socket bind"));

  if (!listen())
    throw SocketException(kh::no_tr("socket listen"));
}


/******************************************************************************
 ***  TCPConnection
 ******************************************************************************/
TCPConnection::TCPConnection(const TCPListener &listener)
    : TCPInetSocket(listener)
{
  if (!isOpen())
    throw SocketException(kh::no_tr("socket accept"));
}

TCPConnection::TCPConnection(const SockAddr &hostAddr)
    : TCPInetSocket(hostAddr.family())
{
  if (!hostAddr)
    throw khException(kh::no_tr("socket open: ") +
                      kh::tr("invalid host addr"));
    
  if (!isOpen())
    throw SocketException(kh::no_tr("socket open"));

  if (!connect(hostAddr)) {
    throw SocketException(kh::no_tr("socket connect '%1'").
                          arg(hostAddr.toString().c_str()));
  }
}

TCPConnection::TCPConnection(const std::string &hostname,
                             in_port_t port,
                             InetAddr::Family family)
    : TCPInetSocket(family)
{
  if (!isOpen())
    throw SocketException(kh::no_tr("socket open"));

  InetAddr addr;
  if (!addr.setAddrByName(family, hostname)) {
    throw khException(kh::tr("Host %1 not found")
                      .arg(hostname.c_str()));
  }

  SockAddr sockAddr(addr, port);

  if (!connect(sockAddr)) {
    throw SocketException(kh::no_tr("socket connect '%1'").
                          arg(sockAddr.toString().c_str()));
  }
};

ssize_t
TCPConnection::sendall(const void *buf, size_t count,
                       int flags, int timeout) {
  ssize_t saveCount = count;

  SocketSendTimeoutGuard guard(this, timeout);
  while (count) {
    ssize_t sent;

    do {
      // MSG_NOSIGNAL - return EPIPE rather than emit SIGPIPE
      sent = ::send(sock, buf, count, flags | MSG_NOSIGNAL);
    } while ((sent == -1) && (errno == EINTR));

    if (sent == -1)
      throw SocketException(kh::no_tr("socket sendall"));

    count -= sent;
    buf = (const char*)buf + sent;
  }
  return saveCount;
}
  
ssize_t
TCPConnection::recvall(void *buf, size_t count, int flags, int timeout) {
  ssize_t saveCount = count;

  SocketRecvTimeoutGuard guard(this, timeout);
  while (count) {
    ssize_t numread;
    do {
      numread = ::recv(sock, buf, count, flags | WaitAll);
    } while ((numread == -1) && (errno == EINTR));

    if (numread == -1)
      throw SocketException(kh::no_tr("socket recvall"));
    else if (numread == 0) {
      errno = EPIPE;
      throw SocketException(kh::no_tr("socket recvall"));
    } else {
      count -= numread;
      buf = (char*)buf + numread;
    }
  }
  return saveCount;
}

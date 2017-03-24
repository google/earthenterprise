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


#ifndef __SocketTimeout_h
#define __SocketTimeout_h

#include <sys/types.h>
#include <sys/socket.h>
#include "InetSocket.h"
#include "SocketException.h"

// ****************************************************************************
// ***  Guards to Set/Restore socket timeout values
// ***
// ***  Don't use SocketTimeoutGuard_ directly. Use SocketSendTimeoutGuard and
// ***  SocketRecvTimeoutGuard instead.
// ****************************************************************************
class SocketTimeoutGuard_
{
 private:
  int sock;
  int op;
  struct timeval savetv;

 public:
  SocketTimeoutGuard_(InetSocket *sock_, int op_, int timeout) : op(op_)
  {
    if (timeout <= 0) {
      // don't want to use timeouts
      sock = -1;
    } else {
      sock = sock_->sock;
      struct timeval tv;
      tv.tv_sec = timeout;
      tv.tv_usec = 0;

      socklen_t savetvlen = sizeof(savetv);
      if (::getsockopt(sock, SOL_SOCKET, op, &savetv, &savetvlen)
          != 0) {
        throw SocketException(kh::no_tr("SocketTimeoutGuard: getsockopt %1").arg(op));
      }

      if (::setsockopt(sock, SOL_SOCKET, op, &tv, sizeof(tv)) != 0) {
        throw SocketException(kh::no_tr("SocketTimeoutGuard: setsockopt %1").arg(op));
      }
    }
  }

  ~SocketTimeoutGuard_(void)
  {
    if (sock != -1) {
      if (::setsockopt(sock, SOL_SOCKET, op,&savetv,sizeof(savetv))!=0){
        notify(NFY_WARN, "Unable to restore socket timeout");
      }
    }
  }
};


class SocketSendTimeoutGuard : public SocketTimeoutGuard_
{
 public:
  SocketSendTimeoutGuard(InetSocket *sock, int timeout)
      : SocketTimeoutGuard_(sock, SO_SNDTIMEO, timeout) { }
};

class SocketRecvTimeoutGuard : public SocketTimeoutGuard_
{
 public:
  SocketRecvTimeoutGuard(InetSocket *sock, int timeout)
      : SocketTimeoutGuard_(sock, SO_RCVTIMEO, timeout) { }
};



#endif /* __SocketTimeout_h */

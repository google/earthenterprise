/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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

/******************************************************************************
File:        FusionConnection.h
Description: Specialized TCPConnection for communicating via the Fusion
             Wire Protocol

-------------------------------------------------------------------------------
******************************************************************************/
#ifndef __FusionConnection_h
#define __FusionConnection_h

/******************************************************************************
 ***  See .cpp file for details of protocol
 ******************************************************************************/


/******************************************************************************
 ***  Low-level Usage
 ***
 ***  SendRegister()
 ***     Once a Register is sent, no more sending may EVER be done on the
 ***  connection. You must call ReceiveMsg once to ensure that you get a
 ***  ReplyMsg (empty payload) and not an ExceptionMsg. Then you must run a
 ***  dedicated thread to repeatedly call ReceiveMsg() in order to guarantee
 ***  that the server won't block trying to send notify messages to the
 ***  client.
 ***      If you wish to change your Regsiter arguments you must do so on
 ***  another connection. When you no longer with to receive notifications,
 ***  you simple destroy the object.
 ***
 ***
 ***  SendRequest()
 ***  Once a Request is sent you must guarantee that the server won't block
 ***  trying to send the response. This is typically done by immediately
 ***  calling ReceiveMsg in the same thread. All current CommandName's are
 ***  implmented sychronously in the server process. This means that only one
 ***  request at a time is handled per connection. There is nothing in the
 ***  protocol to require this. In fact the message serial number is
 ***  explicitly added to support multiple outstanding requests. In that case
 ***  both the client and server socket loops will need to deal with multiple
 ***  outstanding requests w/ replies potentially comming out of
 ***  order. Although the protocol handles this, the SystemManager daemon
 ***  currently does not.
 ***
 ***
 ***  ReceiveMsg()
 ***  After calling Request the client must call ReceiveMsg to get the
 ***  response.  The code can determine if the request finished or had an
 ***  exception by checking the RecvPacket's msgType field.
 ***  msgType == ExceptionMsg
 ***     utf8 encoded error string is in the payload
 ***  msgType == ReplyMsg
 ***     payload contains the cmdname specific reply data
 ***
 ***
 ***  High-level Usage (encapsulates encoding, decoding & exception handling)
 ***
 ***  Notify()
 ***  Request()
 ***  NoRetRequest()
 ***  Register()
 ***  NoArgRegister()
 ******************************************************************************/

#include <cstdint>
#include <khnet/TCPInetSocket.h>
#include <khRefCounter.h>
#include <notify.h>
#include <khException.h>
#include <khstrconv.h>

class FusionConnection : public TCPConnection
{
 private:
  std::uint32_t serial;


  // private to force use of handles, use AcceptClient or ConnectTo*
  explicit FusionConnection(const TCPListener &listener) :
      TCPConnection(listener), serial(0) { }
  explicit FusionConnection(const SockAddr &hostAddr) :
      TCPConnection(hostAddr), serial(0) { }


  static std::string SystemManagerHostname(void);
  static SockAddr khAssetManagerAddr(void);
  static SockAddr khResourceManagerAddr(void);

 public:
  typedef khSharedHandle<FusionConnection> Handle;

  static const in_port_t khAssetManagerPort;
  static const in_port_t khResourceManagerPort;
  static const in_port_t khResourceProviderPort;


  // ensures that a handle is always used
  static Handle AcceptClient(const TCPListener &);
  static Handle ConnectTokhAssetManager(void);
  static Handle ConnectTokhResourceManager(void);

  // also encapsulates exception handling
  static Handle TryAcceptClient(const TCPListener &, QString &error);
  static Handle TryConnectTokhAssetManager(QString &error);
  static Handle TryConnectTokhResourceManager(QString &error);

  enum MsgType {InvalidMsg = 0, RegisterMsg = 1, NotifyMsg = 2,
                RequestMsg = 3, ReplyMsg = 4, ExceptionMsg = 5};

  class RecvPacket {
   public:
    std::uint32_t  serial;
    MsgType msgType;
    char    rawcmdname[32];
    std::uint8_t   reserved1;
    std::uint8_t   reserved2;
    std::string payload;

    std::string cmdname(void) const {
      // trim trailing padding of '\0'
      std::string cmd(rawcmdname, 32);
      std::string::size_type len = 32;
      while (len && (cmd[len-1] == '\0')) --len;
      cmd.resize(len);
      return cmd;
    }

    RecvPacket(void) : msgType(InvalidMsg) { }

    RecvPacket(std::uint32_t serial_, MsgType type_,
               const std::string &cmdname_,
               const std::string &payload_)
        : serial(serial_), msgType(type_),
          reserved1(0), reserved2(0),
          payload(payload_)
    {
      if (cmdname_.size() >= sizeof(rawcmdname)) {
        memcpy(rawcmdname, cmdname_.data(), sizeof(rawcmdname));
      } else {
        memcpy(rawcmdname, cmdname_.data(), cmdname_.size());
        memset(rawcmdname + cmdname_.size(), 0, sizeof(rawcmdname) - cmdname_.size());
      }

    }
    RecvPacket(const RecvPacket &req, MsgType type_,
               const std::string &payload_)
        : serial(req.serial), msgType(type_),
          reserved1(req.reserved1), reserved2(req.reserved2),
          payload(payload_)
    {
      memcpy(rawcmdname, req.rawcmdname, sizeof(rawcmdname));
    }

    void Dump(const std::string &header) const;
  };


  // **********************************************
  // *** Used by both client and server to recv ***
  // **********************************************
  void ReceiveMsg(RecvPacket &msg, int timeout = 0);


  // ****************************************
  // *** Used by server to send to client ***
  // ****************************************
  void SendNotify(const std::string &cmdname, const std::string &payload,
                  int timeout = 0)
  {
    Send(RecvPacket(serial++, NotifyMsg, cmdname, payload), timeout);
  }
  void SendReply(const RecvPacket &req, const std::string &payload,
                 int timeout = 0)
  {
    Send(RecvPacket(req, ReplyMsg, payload), timeout);
  }
  void SendException(const RecvPacket &req, const QString &errorMsg,
                     int timeout = 0)
  {
    Send(RecvPacket(req, ExceptionMsg, (const char *)(errorMsg.utf8())),
         timeout);
  }


  // ****************************************
  // *** Used by client to send to server ***
  // ****************************************
  void SendRegister(const std::string &cmdname, const std::string &payload,
                    int timeout = 0)
  {
    Send(RecvPacket(serial++, RegisterMsg, cmdname, payload), timeout);
  }
  std::uint32_t SendRequest(const std::string &cmdname, const std::string &payload,
                     int timeout = 0)
  {
    Send(RecvPacket(serial++, RequestMsg, cmdname, payload), timeout);
    return serial - 1;
  }


  // ********************************************
  // *** Higher level routines (non throwing) ***
  // ********************************************
  template <class Arg>
  bool TryNotify(const std::string &cmdname, const Arg &arg,
                 QString &error, int timeout = 0) throw();
  template <class Ret>
  bool TryReceiveNotify(Ret &ret, QString &error, int timeout = 0) throw();
  template <class Arg, class Ret>
  bool TryRequest(const std::string &cmdname, const Arg &arg,
                  Ret &ret, QString &error, int timeout = 0) throw();
  template <class Arg>
  bool TryNoRetRequest(const std::string &cmdname, const Arg &arg,
                       QString &error, int timeout = 0) throw();
  template <class Arg>
  bool TryRegister(const std::string &cmdname, const Arg &arg,
                   QString &error, int timeout = 0) throw();
  bool TryNoArgRegister(const std::string &cmdname,
                        QString &error, int timeout = 0) throw();

  // ****************************************
  // *** Higher level routines (throwing) ***
  // ****************************************
  template <class Arg>
  void Notify(const std::string &cmdname, const Arg &arg, int timeout = 0);
  template <class Ret>
  void ReceiveNotify(Ret &ret, int timeout = 0);
  template <class Arg, class Ret>
  void Request(const std::string &cmdname, const Arg &arg, Ret &ret,
               int timeout = 0);
  template <class Arg>
  void NoRetRequest(const std::string &cmdname, const Arg &arg,
                    int timeout = 0);
  template <class Arg>
  void Register(const std::string &cmdname, const Arg &arg, int timeout = 0);
  void NoArgRegister(const std::string &cmdname, int timeout = 0);

 private:
  void Send(const RecvPacket &recvPack, int timeout);
};



template <class T>
bool
ToPayload(const T &t, std::string &payload) {
  return t.SaveToString(payload, "");
}

template <class T>
bool
FromPayload(const std::string &payload, T &t) {
  return t.LoadFromString(payload, "");
}

inline bool
ToPayload(bool t, std::string &payload) {
  payload = ToString(t);
  return true;
}
inline bool
FromPayload(const std::string &payload, bool &result) {
  FromString(payload, result);
  return true;
}

inline bool
ToPayload(const std::string &str, std::string &payload) {
  payload = str;
  return true;
}
inline bool
FromPayload(const std::string &payload, std::string &str) {
  str = payload;
  return true;
}


template <class Arg>
void
FusionConnection::Notify(const std::string &cmdname, const Arg &arg,
                         int timeout)
{
  std::string payload;
  if (!ToPayload(arg, payload)) {
    throw khException(kh::tr("Unable to encode payload"));
  } else {
    SendNotify(cmdname, payload, timeout);
  }
}



template <class Ret>
void
FusionConnection::ReceiveNotify(Ret &ret, int timeout)
{
  RecvPacket reply;
  ReceiveMsg(reply, timeout);
  if (reply.msgType == NotifyMsg) {
    if (!FromPayload(reply.payload, ret)) {
      throw khException(kh::tr("Unable to decode notify"));
    }
  } else {
    throw khException(kh::tr("Unexpected response"));
  }
}


template <class Arg, class Ret>
void
FusionConnection::Request(const std::string &cmdname, const Arg &arg,
                          Ret &ret, int timeout)
{
  std::string payload;
  if (!ToPayload(arg, payload)) {
    throw khException(kh::tr("Unable to encode payload"));
  } else {
    SendRequest(cmdname, payload, timeout);
    RecvPacket reply;
    ReceiveMsg(reply, timeout);
    if (reply.msgType == ReplyMsg) {
      if (!FromPayload(reply.payload, ret)) {
        throw khException(kh::tr("Unable to decode reply"));
      }
    } else if (reply.msgType == ExceptionMsg) {
      throw khException(QString::fromUtf8(reply.payload.c_str()));
    } else {
      throw khException(kh::tr("Unexpected response"));
    }
  }
}

template <class Arg>
void
FusionConnection::NoRetRequest(const std::string &cmdname,
                               const Arg &arg, int timeout)
{
  std::string payload;
  if (!ToPayload(arg, payload)) {
    throw khException(kh::tr("Unable to encode payload"));
  } else {
    SendRequest(cmdname, payload, timeout);
    RecvPacket reply;
    ReceiveMsg(reply, timeout);
    if (reply.msgType == ReplyMsg) {
      // noop, there is no return
    } else if (reply.msgType == ExceptionMsg) {
      throw khException(QString::fromUtf8(reply.payload.c_str()));
    } else {
      throw khException(kh::tr("Unexpected response"));
    }
  }
}

template <class Arg>
void
FusionConnection::Register(const std::string &cmdname, const Arg &arg,
                           int timeout)
{
  std::string payload;
  if (!ToPayload(arg, payload)) {
    throw khException(kh::tr("Unable to encode payload"));
  } else {
    SendRegister(cmdname, payload, timeout);
    RecvPacket reply;
    ReceiveMsg(reply, timeout);
    if (reply.msgType == ReplyMsg) {
      // no-op, there is no return value
      // we just drop off the end and return true
    } else if (reply.msgType == ExceptionMsg) {
      throw khException(QString::fromUtf8(reply.payload.c_str()));
    } else {
      throw khException(kh::tr("Unexpected response"));
    }
  }
}


template <class Arg>
bool
FusionConnection::TryNotify(const std::string &cmdname, const Arg &arg,
                            QString &error, int timeout) throw()
{
  try {
    Notify(cmdname, arg, timeout);
  } catch (const std::exception &e) {
    error = QString(cmdname.c_str()) + ": " + QString::fromUtf8(e.what());
    return false;
  } catch (...) {
    error = QString(cmdname.c_str()) + ": " + kh::tr("Unknown error");
    return false;
  }
  return true;
}


template <class Ret>
bool
FusionConnection::TryReceiveNotify(Ret &ret, QString &error,
                                   int timeout) throw()
{
  try {
    ReceiveNotify(ret, timeout);
  } catch (const std::exception &e) {
    error = QString::fromUtf8(e.what());
    return false;
  } catch (...) {
    error = kh::tr("Unknown error");
    return false;
  }
  return true;
}


template <class Arg, class Ret>
bool
FusionConnection::TryRequest(const std::string &cmdname, const Arg &arg,
                             Ret &ret, QString &error,
                             int timeout) throw()
{
  try {
    Request(cmdname, arg, ret, timeout);
  } catch (const std::exception &e) {
    error = QString(cmdname.c_str()) + ": " + QString::fromUtf8(e.what());
    return false;
  } catch (...) {
    error = QString(cmdname.c_str()) + ": " + kh::tr("Unknown error");
    return false;
  }
  return true;
}


template <class Arg>
bool
FusionConnection::TryNoRetRequest(const std::string &cmdname, const Arg &arg,
                                  QString &error, int timeout) throw()
{
  try {
    NoRetRequest(cmdname, arg, timeout);
  } catch (const std::exception &e) {
    error = QString(cmdname.c_str()) + ": " + QString::fromUtf8(e.what());
    return false;
  } catch (...) {
    error = QString(cmdname.c_str()) + ": " + kh::tr("Unknown error");
    return false;
  }
  return true;
}

template <class Arg>
bool
FusionConnection::TryRegister(const std::string &cmdname, const Arg &arg,
                              QString &error, int timeout) throw()
{
  try {
    Register(cmdname, arg, timeout);
  } catch (const std::exception &e) {
    error = QString(cmdname.c_str()) + ": " + QString::fromUtf8(e.what());
    return false;
  } catch (...) {
    error = QString(cmdname.c_str()) + ": " + kh::tr("Unknown error");
    return false;
  }
  return true;
}



#endif /* __FusionConnection_h */

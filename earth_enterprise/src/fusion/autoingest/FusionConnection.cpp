// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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


/******************************************************************************
File:        FusionConnection.cpp
******************************************************************************/
#include "FusionConnection.h"
#include <notify.h>
#include <khEndian.h>
#include <khnet/SocketException.h>
#include <algorithm>
#include "khVolumeManager.h"
#include <fusionversion.h>
#include <autoingest/.idl/storage/AssetDefs.h>


/******************************************************************************
 ***  Wire Protocol
 ***
 ***  Header is fixed size (72 bytes) and in network byte order
 ***  This fixed size includes room to grow for future versions of the protocol
 ***      many more MsgTypes available
 ***      two reserved bytes
 ***      32 byte CommandName whose purpose can be overloaded by type
 ***
 ***
 ***  +====================================+
 ***  | "Keyhole Fusion Wire Protocol"     | (28 bytes ASCII - no 0 terminator)
 ***  +------------------------------------+
 ***  | Protocol Version                   | (unsigned 8 bit) set to 1
 ***  +------------------------------------+
 ***  | MsgSerial#                         | (unsigned 32 bit)
 ***  +------------------------------------+
 ***  | Payload length in bytes            | (unsigned 32 bit)
 ***  +------------------------------------+
 ***  | {Register|Notify|Request|Reply|Exception}| (unsigned 8 bit) Enum MsgType
 ***  +------------------------------------+
 ***  | CommandName                        | (32 bytes ASCII - end padded w/ 0)
 ***  +------------------------------------+
 ***  |            - reserved -            | (unsigned 8 bit) set to 0
 ***  +------------------------------------+
 ***  |            - reserved -            | (unsigned 8 bit) set to 0
 ***  +====================================+
 ***  |                                    |
 ***  | Payload ...                        | (format depends on MsgType and
 ***  |                                    |  CommandName)
 ***  +====================================+
 ***
 ***
 ***  MsgType:
 ***     Register - One way registration
 ***              CommandName will indicate type of notification (e.g.
 ***                  "AssetChanged", "VersionChanged", "TaskProgress", etc)
 ***              MsgSerial# will be set to number of msgs sent so far (not
 ***                  particularly useful here)
 ***              Payload will contain data specific to the CommandName (filter
 ***                  perhaps)
 ***     Notify - One way notification
 ***              CommandName will indicate type of notification (e.g.
 ***                  "AssetChanged", "VersionChanged", "TaskProgress", etc)
 ***              MsgSerial# will be set to number of msgs sent so far (not
 ***                  particularly useful here)
 ***              Payload will contain data specific to the CommandName
 ***     Request - Request for action/information
 ***               CommandName will indicate which command to run (e.g.
 ***                  "RasterProductImportRequest", "Update", "Cancel", etc.)
 ***               MsgSerial# will be set to number of msgs sent so far (this
 ***                  # will be returned in the Response or Exception)
 ***               Payload will contain data specific to the CommandName
 ***     Reply - Reply to a previous request
 ***             CommandName will be copied from request
 ***             MsgSerial# will be copied from request
 ***             Payload will contain return data specific to the CommandName
 ***     Exception - Exception to a previous request
 ***               CommandName will be copied from request
 ***               MsgSerial# will be copied from request
 ***               Payload will contain an ASCII string describing the
 ***                   nature of the exception. (no 0 terminator)
 ***
 ******************************************************************************/



// NOTE: 1303x numbers are in the range assigned by google production
// They are also in an IANA unassigned range, so they should not cause any
// problems for customers.
const in_port_t
FusionConnection::khAssetManagerPort = 13031 /* 2083 */;

const in_port_t
FusionConnection::khResourceManagerPort = 13032 /* 2084 */;

const in_port_t
FusionConnection::khResourceProviderPort = 13033 /* 2085 */;

static const char * const ProtocolMagic = "Keyhole Fusion Wire Protocol";
static const unsigned int MagicLen = 28;


// Do not change.
// TODO: - add a COMPILE_TILE_ASSERT (see examples in ffio/ffioIndex.cpp)
static const unsigned int HeaderSize = 72;


std::string
FusionConnection::SystemManagerHostname(void)
{
  static std::string hostname;
  static bool first = true;
  if (first) {
    first = false;

    hostname = AssetDefs::MasterHostName();
  }
  return hostname;
}

SockAddr
FusionConnection::khAssetManagerAddr(void)
{
  std::string node = SystemManagerHostname();
  if (node.empty()) {
    return SockAddr(InetAddr::IPv4Loopback, khAssetManagerPort);
  } else {
    SockAddr addr(node, khAssetManagerPort);
    if (!addr) {
      throw khException(kh::tr("Host %1 not found")
                        .arg(node.c_str()));
    }
    return addr;
  }
}

SockAddr
FusionConnection::khResourceManagerAddr(void)
{
  std::string node = SystemManagerHostname();
  if (node.empty()) {
    return SockAddr(InetAddr::IPv4Loopback, khResourceManagerPort);
  } else {
    SockAddr addr(node, khResourceManagerPort);
    if (!addr) {
      throw khException(kh::tr("Host %1 not found")
                        .arg(node.c_str()));
    }
    return addr;
  }
}

FusionConnection::Handle
FusionConnection::AcceptClient(const TCPListener &listener)
{
  Handle handle(TransferOwnership(new FusionConnection(listener)));
  return handle;
}


FusionConnection::Handle
FusionConnection::TryAcceptClient(const TCPListener &listener, QString &error)
{
  try {
    return AcceptClient(listener);
  } catch (const std::exception &e) {
    error = QString::fromUtf8(e.what());
  } catch (...) {
    error = "Unknown error";
  }
  return Handle();
}


FusionConnection::Handle
FusionConnection::ConnectTokhAssetManager(void)
{
  Handle handle(TransferOwnership(new FusionConnection
                                  (khAssetManagerAddr())));

  handle->NoRetRequest("ValidateProtocolVersion",
                       std::string(GEE_VERSION));

  return handle;
}

FusionConnection::Handle
FusionConnection::ConnectTokhResourceManager(void)
{
  Handle handle(TransferOwnership(new FusionConnection
                                  (khResourceManagerAddr())));
  return handle;
}

FusionConnection::Handle
FusionConnection::TryConnectTokhAssetManager(QString &error)
{
  try {
    return ConnectTokhAssetManager();
  } catch (const std::exception &e) {
    error = QString::fromUtf8(e.what());
  } catch (...) {
    error = QObject::tr("Unknown error");
  }
  return Handle();
}

FusionConnection::Handle
FusionConnection::TryConnectTokhResourceManager(QString &error)
{
  try {
    return ConnectTokhResourceManager();
  } catch (const std::exception &e) {
    error = QString::fromUtf8(e.what());
  } catch (...) {
    error = QObject::tr("Unknown error");
  }
  return Handle();
}


void
FusionConnection::ReceiveMsg(RecvPacket &recvPacket, int timeout)
{
  char buf[4096];

  // will throw exception if unable to recv it all
  recvall(buf, HeaderSize, 0 /* flags */, timeout);
  if (memcmp(buf, ProtocolMagic, MagicLen)!=0) {
    notify(NFY_WARN, "Invalid protocol magic.");
    errno = EPROTO;
    throw SocketException(kh::no_tr("ReceiveMsg"));
  }
  BigEndianReadBuffer bebuf(buf, HeaderSize);
  bebuf.Skip(MagicLen);
  std::uint8_t protover;
  bebuf >> protover;
  if (protover != 1) {
    notify(NFY_WARN, "Unsupported protocol version (%d).", protover);
    errno = EPROTONOSUPPORT;
    throw SocketException(kh::no_tr("ReceiveMsg"));
  }
  std::uint32_t payloadLen;
  std::uint8_t  msgType;
  bebuf >> recvPacket.serial
        >> payloadLen
        >> msgType
        >> FixedLengthString(recvPacket.rawcmdname,
                             sizeof(recvPacket.rawcmdname))
        >> recvPacket.reserved1
        >> recvPacket.reserved2;
  recvPacket.msgType   = (MsgType)msgType;
  while (payloadLen > 0) {
    std::uint32_t toread = std::min((size_t)payloadLen, sizeof(buf));
    // will throw exception if unable to recv it all
    recvall(buf, toread, 0 /* flags */, timeout);
    recvPacket.payload.append(buf, toread);
    payloadLen -= toread;
  }

  if (getNotifyLevel() >= NFY_DEBUG)
    recvPacket.Dump("FusionConnection::ReceiveMsg");
}


void
FusionConnection::Send(const RecvPacket &recvPack, int timeout)
{
  if (getNotifyLevel() >= NFY_DEBUG)
    recvPack.Dump("FusionConnection::Send");

  BigEndianWriteBuffer buf;

  buf.rawwrite(ProtocolMagic, MagicLen);
  buf.push(std::uint8_t(1));  // version
  buf.push(recvPack.serial);
  buf.push(std::uint32_t(recvPack.payload.size()));
  buf.push(std::uint8_t(recvPack.msgType));
  buf.rawwrite(recvPack.rawcmdname, sizeof(recvPack.rawcmdname));
  buf.push(recvPack.reserved1);
  buf.push(recvPack.reserved2);
  buf.rawwrite(recvPack.payload.data(), recvPack.payload.size());

  if (getNotifyLevel() >= NFY_DEBUG) {
    fprintf(stderr, "========== message dump ===========\n");
    HexDump(stderr, buf.data(), buf.size());
    fprintf(stderr, "==================================\n");
  }

  // will throw exceptions if unable to send it all
  sendall(buf.data(), buf.size(), 0 /* flags */, timeout);
}


void
FusionConnection::NoArgRegister(const std::string &cmdname, int timeout)
{
  SendRegister(cmdname, std::string(), timeout);
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

bool
FusionConnection::TryNoArgRegister(const std::string &cmdname,
                                   QString &error, int timeout) throw()
{
  try {
    NoArgRegister(cmdname, timeout);
  } catch (const std::exception &e) {
    error = QString::fromUtf8(e.what());
    return false;
  } catch (...) {
    error = kh::tr("Unknown error");
    return false;
  }
  return true;
}


// ****************************************************************************
// ***  FusionConnection::RecvPacket
// ****************************************************************************
void
FusionConnection::RecvPacket::Dump(const std::string &header) const
{
  //      std::uint32_t  serial;
  //      MsgType msgType;
  //      char    rawcmdname[32];
  //      std::uint8_t   reserved1;
  //      std::uint8_t   reserved2;
  //      std::string payload;

  fprintf(stderr, "========== %s ==========\n", header.c_str());
  fprintf(stderr, "serial = %d\n", serial);
  fprintf(stderr, "msgType = %d\n", msgType);
  fprintf(stderr, "rawcmdname = ");
  for (unsigned int i = 0; i < sizeof(rawcmdname); ++i) {
    if (rawcmdname[i] == 0) {
      fprintf(stderr, "\\0");
    } else {
      fprintf(stderr, "%c", rawcmdname[i]);
    }
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "reserved1 = %d\n", reserved1);
  fprintf(stderr, "reserved2 = %d\n", reserved2);
  fprintf(stderr, "payload = |%s|\n", payload.c_str());
  fprintf(stderr, "========== %s ==========\n", header.c_str());
}

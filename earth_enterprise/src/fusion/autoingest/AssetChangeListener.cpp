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


#include "AssetChangeListener.h"
#include "FusionConnection.h"
#include <khnet/SocketException.h>
#include <notify.h>

void
AssetChangeListener::thread_main(void)
{
  while (1) {
    FusionConnection::Handle conn;
    const unsigned int maxBlockSecs = 5;
    int retryConnDelay      = 15;
    int delayRemaining      = 0;
    QString error;

    // retry until we can connect
    bool first = true;
    while (!conn) {
      if (wantAbort())
        return;
      if (delayRemaining > 0) {
        sleep(maxBlockSecs);
        delayRemaining -= maxBlockSecs;
      } else {
        conn = FusionConnection::TryConnectTokhAssetManager(error);
        if (conn) {
          // successful, reset timers for the next time we
          // try to connect
          delayRemaining = 0;
        } else {
          // failure
          delayRemaining = retryConnDelay;
          if (first) {
            notify(NFY_WARN,
                   "Unable to connect to System Manager: %s",
                   error.latin1());
            notify(NFY_WARN, "Will try again every %d seconds",
                   retryConnDelay);
          }
        }
        first = false;
      }
    }

    // tell the system manager that I want to receive asset change
    // notifications on this socket
    if (!conn->TryNoArgRegister("AssetChanges", error)) {
      notify(NFY_WARN, "Unable to register for asset changes: %s",
             error.latin1());
      sleep(maxBlockSecs);
      // fall out to disconnect and reconnect and try again
    } else {
      while (1) {
        if (wantAbort())
          break;
        try {
          // wake up every 5 seconds to see if I need to go away
          if (conn->waitUntilReadable(5000)) {
            FusionConnection::RecvPacket msg;
            conn->ReceiveMsg(msg);
            if ((msg.msgType == FusionConnection::NotifyMsg) &&
                (msg.cmdname() == "AssetChanges")) {
              AssetChanges changes;
              if (changes.LoadFromString(msg.payload, "Asset Change Message")) {
                try {
                  handleAssetChanges(changes);
                } catch (const std::exception &e) {
                  notify(NFY_WARN, "Error processing asset changes: %s\n",
                         e.what());
                } catch (...) {
                  notify(NFY_WARN, "Unknown error processing asset changes");
                }
              } else {
                notify(NFY_WARN, "Unable to decode asset changes");
              }
            } else {
              notify(NFY_WARN, "Unexpected message from system manager");
            }
          } else {
            // notify(NFY_DEBUG, "AssetChangeListener: NOT READABLE");
          }
        } catch (const SocketException &e) {
          notify(NFY_WARN, "Socket error listening for asset changes: %s\n",
                 e.what());
          // drop out and try to reconnect
          break;
        } catch (const std::exception &e) {
          notify(NFY_WARN, "Error listening for asset changes: %s\n",
                 e.what());
        } catch (...) {
          notify(NFY_WARN, "Unknown error listening for asset changes\n");
        }
      }
    }
  }
}

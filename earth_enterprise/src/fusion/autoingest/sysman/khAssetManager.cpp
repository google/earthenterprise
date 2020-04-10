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
File:        khAssetManager.cpp
Description:

-------------------------------------------------------------------------------
******************************************************************************/
#include "khAssetManager.h"

#include "common/khnet/SocketException.h"
#include "fusion/autoingest/sysman/AssetOperation.h"
#include "fusion/autoingest/sysman/khSystemManager.h"
#include "fusion/autoingest/sysman/khResourceManager.h"
#include "fusion/autoingest/sysman/plugins/RasterProductAssetD.h"
#include "fusion/autoingest/sysman/plugins/MercatorRasterProductAssetD.h"
#include "fusion/autoingest/sysman/plugins/RasterProjectAssetD.h"
#include "fusion/autoingest/sysman/plugins/MercatorRasterProjectAssetD.h"
#include "fusion/autoingest/sysman/plugins/VectorProductAssetD.h"
#include "fusion/autoingest/sysman/plugins/VectorLayerXAssetD.h"
#include "fusion/autoingest/sysman/plugins/VectorProjectAssetD.h"
#include "fusion/autoingest/sysman/plugins/DatabaseAssetD.h"
#include "fusion/autoingest/sysman/plugins/MapProjectAssetD.h"
#include "fusion/autoingest/sysman/plugins/MapLayerAssetD.h"
#include "fusion/autoingest/sysman/plugins/MapDatabaseAssetD.h"
#include "fusion/autoingest/sysman/plugins/MercatorMapDatabaseAssetD.h"
#include "fusion/autoingest/sysman/plugins/KMLProjectAssetD.h"
#include "fusion/autoingest/sysman/.idl/NextTaskId.h"
#include "fusion/autoingest/sysman/.idl/FusionUniqueId.h"
#include "fusion/autoingest/plugins/SourceAsset.h"
#include "common/khSpawn.h"
#include "common/geConsoleProgress.h"
#include "common/geConsoleAuth.h"
#include "fusion/autoingest/.idl/ServerCombination.h"
#include "fusion/gepublish/PublisherClient.h"
#include "fusion/fusionversion.h"
#include "common/khProgressMeter.h"

// ****************************************************************************
// ***  khAssetManager
// ****************************************************************************
khAssetManager theAssetManager;

khAssetManager::khAssetManager(void)
    : readerThreads(khThreadPool::Create(1, 3))
{
}

khAssetManager::~khAssetManager(void)
{
  // Don't delete readerThreads - it's not allowed

  // really nothing to do, main is long gone and it's too late for us to do
  // anything useful
}

void
khAssetManager::Init(void)
{
  khLockGuard lock(mutex);

  NextTaskId::Load();
  FusionUniqueId::Init(AssetDefs::AssetRoot());
}

void
khAssetManager::AssertPendingEmpty(void) {
  assert(pendingStateChanges.size() == 0);
  assert(pendingProgress.size() == 0);
  assert(pendingTaskCmds.size() == 0);
  assert(pendingFileDeletes.size() == 0);
  assert(alwaysTaskCmds.size() == 0);
  assert(Asset::DirtySize() == 0);
  assert(AssetVersion::DirtySize() == 0);
}

// ****************************************************************************
// ***  khAssetManager::ApplyPending
// ****************************************************************************

// #define SKIP_SAVE


void
khAssetManager::ApplyPending(void)
{
  khFilesTransaction filetrans(".new");

  // The actual list saved may be smaller than what's
  // in the dirty set. Some things can be in the dirty set
  // even if it really didn't change
  std::vector<SharedString> savedAssets;


  QTime timer;
  int elapsed = 0;

  notify(NFY_INFO, "Asset cache size = %d", Asset::CacheSize());
  notify(NFY_INFO, "Version cache size = %d", AssetVersion::CacheSize());
  notify(NFY_INFO, "Approx. memory used by asset cache = %lu B", Asset::CacheMemoryUse());
  notify(NFY_INFO, "Approx. memory used by version cache = %lu B", AssetVersion::CacheMemoryUse());

#ifndef SKIP_SAVE

  // ***** prep the pending changes *****
  // save the asset & version records to ".new"
  timer.start();
  if (!MutableAssetD::SaveDirtyToDotNew(filetrans, &savedAssets)) {
    throw khException(kh::tr("Unable to save modified assets"));
  }
  elapsed = timer.elapsed();
  notify(NFY_INFO, "Elapsed writing time: %s",
           khProgressMeter::msToString(elapsed).latin1());
  timer.start();
  if (!MutableAssetVersionD::SaveDirtyToDotNew(filetrans, 0)) {
    throw khException(kh::tr("Unable to save modified versions"));
  }
  elapsed = timer.elapsed();
  notify(NFY_INFO, "Elapsed writing time: %s",
         khProgressMeter::msToString(elapsed).latin1());



  // processing pending deletes
  if (pendingFileDeletes.size()) {
    notify(NFY_INFO, "Deleting %lu files",
           static_cast<long unsigned>(pendingFileDeletes.size()));
    timer.start();
    for (std::vector<std::string>::const_iterator d =
           pendingFileDeletes.begin();
         d != pendingFileDeletes.end(); ++d) {
      filetrans.DeletePath(*d);
    }
    int elapsed = timer.elapsed();
    notify(NFY_INFO, "Elapsed delete time: %s",
           khProgressMeter::msToString(elapsed).latin1());
  }


  // build a list of AssetChanges
  AssetChanges changes;
  for (const auto & ref : savedAssets) {
    changes.items.push_back(AssetChanges::Item(ref, "Modified"));
  }
  for (std::map<SharedString, AssetDefs::State>::const_iterator i
         = pendingStateChanges.begin();
       i != pendingStateChanges.end(); ++i) {
    changes.items.push_back(AssetChanges::Item(i->first.toString(),
                                               ToString(i->second)));
  }
  for (std::map<SharedString, double>::const_iterator i
         = pendingProgress.begin();
       i != pendingProgress.end(); ++i) {
    changes.items.push_back(AssetChanges::Item(i->first.toString(),
                                               "Progress( " +
                                               ToString(i->second) +
                                               ")"));
  }

  // ***** Commit all the pending changes *****
  // file changes
  notify(NFY_INFO, "Commiting file transaction");
  timer.start();
  if (!filetrans.Commit()) {
    throw khException(kh::tr("Unable to commit asset changes"));
  }
  elapsed = timer.elapsed();
  notify(NFY_INFO, "Elapsed commit time: %s",
         khProgressMeter::msToString(elapsed).latin1());

  // AssetChanges
  notify(NFY_INFO, "Submitting change notifications");
  timer.start();
  if (changes.items.size()) {
    changes.changeTime = time(0);
    changesQueue.push(changes);
  }
  elapsed = timer.elapsed();
  notify(NFY_INFO, "Elapsed submit time: %s",
         khProgressMeter::msToString(elapsed).latin1());

  // task cmds
  notify(NFY_INFO, "Sending %lu task commands",
         static_cast<long unsigned>(pendingTaskCmds.size() +
                                    alwaysTaskCmds.size()));
  timer.start();
  if (pendingTaskCmds.size()) {
    theResourceManager.taskCmdQueue.push(pendingTaskCmds.begin(),
                                         pendingTaskCmds.end());
  }
  SendAlwaysTaskCmds();
  elapsed = timer.elapsed();
  notify(NFY_INFO, "Elapsed task submit time: %s",
         khProgressMeter::msToString(elapsed).latin1());

#endif

  // ***** just before we leave, clear out all our pending lists *****
  notify(NFY_INFO, "Cleaning up");
  pendingStateChanges.clear();
  pendingProgress.clear();
  pendingTaskCmds.clear();
  pendingFileDeletes.clear();
  elapsed = timer.elapsed();
  notify(NFY_INFO, "Elapsed cleanup time: %s",
         khProgressMeter::msToString(elapsed).latin1());
}


void
khAssetManager::SendAlwaysTaskCmds(void) throw()
{
  if (alwaysTaskCmds.size()) {
    theResourceManager.taskCmdQueue.push(alwaysTaskCmds.begin(),
                                         alwaysTaskCmds.end());
    alwaysTaskCmds.clear();
  }
}


// ****************************************************************************
// ***  khAssetManager::AbortPending
// ****************************************************************************
void
khAssetManager::AbortPending(void) throw()
{
  // none of these had better throw because this routine
  // is called during stack unwinding
  pendingStateChanges.clear();
  pendingProgress.clear();
  pendingTaskCmds.clear();
  pendingFileDeletes.clear();
  MutableAssetD::AbortDirty();
  MutableAssetVersionD::AbortDirty();

  SendAlwaysTaskCmds();
}


// ****************************************************************************
// ***  khAssetManager::PendingAssetGuard
// ****************************************************************************
khAssetManager::PendingAssetGuard::PendingAssetGuard(khAssetManager &assetman_)
    : assetman(assetman_)
{
  notify(NFY_INFO, "---------- PendingAssetGuard ----------");

  // assert that we're already locked
  assert(!assetman.mutex.trylock());

  assetman.AssertPendingEmpty();

  timer.start();
}

khAssetManager::PendingAssetGuard::~PendingAssetGuard(void)
{
  if (std::uncaught_exception()) {
    // we're begin destroyed as the result of an exception
    assetman.AbortPending();
    notify(NFY_INFO, "---------- ~PendingAssetGuard (Aborted) ----------");
  } else {
    // we're being destroyed under normal circumstances
    try {
      int elapsed = timer.elapsed();
      notify(NFY_INFO, "Elapsed load and think time: %s",
             khProgressMeter::msToString(elapsed).latin1());
      assetman.ApplyPending();
      notify(NFY_INFO,
             "---------- ~PendingAssetGuard (Successfull) ----------");
    } catch (...) {
      assetman.AbortPending();
      notify(NFY_INFO, "---------- ~PendingAssetGuard (Aborted) ----------");

      // XXX - THIS IS SAFE ONLY IF CALLER CAN DEAL WITH THIS A DESTRUCTOR
      // THROWING AND EXCEPTION. STACK VARIABLE CAN.
      throw;
    }
  }
}


// ****************************************************************************
// ***  AssetCmdLoop thread
// ****************************************************************************
void
khAssetManager::ProcessAssetCmd(const AssetCmd &cmd) throw()
{
  try {
    cmd(this);
  } catch (const std::exception &e) {
    notify(NFY_WARN, "Caught exception processing asset command: %s",
           e.what());
  } catch (...) {
    notify(NFY_WARN, "Caught unknown exception processing asset command");
  }
}

void
khAssetManager::AssetCmdLoop(void) throw()
{
  while (1) {
    try {
      AssetCmd cmd = assetCmdQueue.pop();
      {
        khLockGuard lock(mutex);
        PendingAssetGuard guard(*this);
        ProcessAssetCmd(cmd);
      }
      if (theSystemManager.WantExit()) {
        break;
      }
    } catch (const std::exception &e) {
      notify(NFY_WARN, "Caught exception popping assset command queue: %s",
             e.what());
    } catch (...) {
      notify(NFY_WARN, "Caught unknown exception popping assset command queue");
    }
  }
}




// ****************************************************************************
// ***  AssetNotifierLoop
// ****************************************************************************
void
khAssetManager::SendAssetChanges(const AssetChanges &changes)
{
  SendList tosend(listeners.keys());
  for (SendList::const_iterator s = tosend.begin();
       s != tosend.end(); ++s) {
    try {
      s->SendChanges(changes);
    } catch (const SocketException &e) {
      // I can't talk to this guy.
      // Drop him from my listeners.
      // If he really cares, he'll get his act together and
      // reconnect.
      listeners.erase(*s);
    } catch (const std::exception &e) {
      notify(NFY_WARN, "Caught exception while sending asset changes: %s",
             e.what());
    } catch (...) {
      notify(NFY_WARN, "Caught unknown exception while sending asset changes");
    }
  }
}

void
khAssetManager::AssetNotifierLoop(void) throw()
{
  while (1) {
    try {
      if (theSystemManager.WantExit()) {
        break;
      }
      AssetChanges changes = changesQueue.pop();
      if (theSystemManager.WantExit()) {
        break;
      }
      time_t timediff = time(0) - changes.changeTime;
      if (timediff < 0) {
        // our time_t has wrapped :-(
        // just send it right now
      } else if (timediff < 2) {
        // help reduce severity of "delayed NFS visibility" bug
        sleep(2 - timediff);
      }
      SendAssetChanges(changes);
    } catch (const std::exception &e) {
      notify(NFY_WARN, "Caught exception popping asset changes queue: %s",
             e.what());
    } catch (...) {
      notify(NFY_WARN, "Caught unknown popping asset changes queue");
    }
  }
}



// ****************************************************************************
// ***  HandleClientLoop
// ****************************************************************************
void
khAssetManager::HandleClientLoop(FusionConnection::Handle client) throw()
{
  try {
    // the first thing that must be handled is the ValidateProtocolVersion
    // request
    {
      FusionConnection::RecvPacket msg;
      client->ReceiveMsg(msg);
      if (theSystemManager.WantExit()) {
        return;
      }
      if ((msg.msgType != FusionConnection::RequestMsg) ||
          (msg.cmdname() != "ValidateProtocolVersion")) {
        client->SendException
          (msg, "Invalid protocol version (client too old)");
        return;
      }
      if (msg.payload != std::string(GEE_VERSION)) {
        client->SendException
          (msg, "Invalid protocol version (client/server mismatch)");
        return;
      }
      client->SendReply(msg, std::string());
    }

    // no loop through handling all other requests
    while (1) {
      FusionConnection::RecvPacket msg;
      client->ReceiveMsg(msg);
      if (theSystemManager.WantExit()) {
        break;
      }
      std::string cmdname = msg.cmdname();
      if (msg.msgType == FusionConnection::RequestMsg) {
        try {
          notify(NFY_DEBUG, "Handling request %s: %s",
                 cmdname.c_str(), msg.payload.c_str());
          std::string replyPayload;
          if (cmdname == ASSET_STATUS) {
            // TODO: Deserialize parameters from protobuf here.
            replyPayload = GetAssetStatus(msg.payload);
          } else if (cmdname == PUSH_DATABASE) {
            // TODO: Deserialize parameters from protobuf here.
            replyPayload = PushDatabase(msg.payload);
          } else if (cmdname == PUBLISH_DATABASE) {
            // TODO: Deserialize parameters from protobuf here.
            replyPayload = PublishDatabase(msg.payload);
          } else if (cmdname == GET_TASKS) {
            replyPayload = RetrieveTasking(msg);
          } else {
            DispatchRequest(msg, replyPayload);
          }
          if (replyPayload.substr(0, 6) == "ERROR:") {
            client->SendException(msg, replyPayload.c_str());
          } else {
            client->SendReply(msg, replyPayload);
          }
        } catch (const SocketException &e) {
          // Socket errors should bail us out of our loop
          throw;
        } catch (const std::exception &e) {
          notify(NFY_WARN, "%s: Error: %s", cmdname.c_str(),
                 e.what());
          client->SendException(msg, e.what());
        } catch (...) {
          notify(NFY_WARN, "%s: Unknown error", cmdname.c_str());
          client->SendException(msg, "Unknown error");
        }
      } else if (msg.msgType == FusionConnection::NotifyMsg) {
        notify(NFY_DEBUG, "Handling notify %s", cmdname.c_str());
        try {
          DispatchNotify(msg);
        } catch (const std::exception &e) {
          notify(NFY_WARN, "%s: Error: %s", cmdname.c_str(),
                 e.what());
        } catch (...) {
          notify(NFY_WARN, "%s: Unknown error", cmdname.c_str());
        }
      } else if (msg.msgType == FusionConnection::RegisterMsg) {
        notify(NFY_INFO, "Handling register %s", cmdname.c_str());
        if (cmdname == "AssetChanges") {
          // add the SockHandle to the appropriate MT-safe set of
          // listeners
          listeners.insert(AssetListenerD(client));
          // confirm to client that he's now listening
          client->SendReply(msg, std::string());
        } else {
          notify(NFY_WARN, "Unrecognized register request: %s",
                 msg.payload.c_str());
          client->SendException(msg, kh::tr("Unrecognized register request"));
        }

        // And now exit this thread. My copy of the SockHandle will
        // be destroyed, but the listener framework now has its own
        // copy which will keep the socket open
        break;
      } else {
        notify(NFY_WARN, "Unsupported message type: %d",
               msg.msgType);
        // we can't recover from this since the client may be
        // waiting for a reply of some sort
        throw khException(kh::tr("Unrecognized message"));
      }
    }
  } catch (const SocketException &e) {
    if (e.errnum() == EPIPE) {
      notify(NFY_DEBUG, "Client closed connection");
      // the client has closed its side of the connection
      // No big deal, just fall off the bottom of this handler thread
    } else {
      notify(NFY_WARN, "Client socket error: %s", e.what());
    }
  } catch (const std::exception &e) {
    notify(NFY_WARN, "Error handling client messages: %s", e.what());
  } catch (...) {
    notify(NFY_WARN, "Unknown error handling client messages");
  }
}


// ****************************************************************************
// ***  ClientListenerLoop
// ****************************************************************************
void
khAssetManager::ClientListenerLoop(void) throw() {
  try {
    TCPListener serverSock(SockAddr(InetAddr::IPv4Any,
                                    FusionConnection::khAssetManagerPort));
    while (1) {
      try {
        FusionConnection::Handle client
          (FusionConnection::AcceptClient(serverSock));
        if (theSystemManager.WantExit()) {
          break;
        }
        readerThreads->run
          (khFunctor<void>(std::mem_fun(&khAssetManager::HandleClientLoop),
                           this,
                           client));
      } catch (const std::exception &e) {
        notify(NFY_WARN,
               "Caught exception in asset manager server loop: %s", e.what());
      } catch (...) {
        notify(NFY_WARN, "Caught exception in asset manager server loop");
      }
    }
  } catch (const std::exception &e) {
    notify(NFY_WARN, "Unable to open server socket: %s", e.what());
  } catch (...) {
    notify(NFY_WARN, "Unable to open server socket");
  }
  theSystemManager.SetWantExit();
}



// ****************************************************************************
// ***  khAssetManager - routines that gather changes while AssetGuard is held
// ****************************************************************************
void
khAssetManager::NotifyVersionStateChange(const SharedString &ref,
                                         AssetDefs::State state)
{
  // assert that we're already locked
  assert(!mutex.TryLock());
  
  if (ToString(pendingStateChanges[ref]) == "") {
    notify(NFY_VERBOSE, "Set pendingStateChanges[%s]: <empty> to state: %s",
         ref.toString().c_str(),
         ToString(state).c_str());
  }
  else {
    notify(NFY_VERBOSE, "Set pendingStateChanges[%s]: %s to state: %s",
         ref.toString().c_str(),
         ToString(pendingStateChanges[ref]).c_str(),
         ToString(state).c_str());
  }
  pendingStateChanges[ref] = state;
}

void
khAssetManager::NotifyVersionProgress(const SharedString &ref, double progress)
{
  // assert that we're already locked
  assert(!mutex.TryLock());

  pendingProgress[ref] = progress;
}

void
khAssetManager::SubmitTask(const SharedString &verref, const TaskDef &taskdef,
                           int priority)
{
  // assert that we're already locked
  assert(!mutex.TryLock());

  std::uint32_t taskid = NextTaskId::Get();
  MutableAssetVersionD(verref)->taskid = taskid;

  SubmitTaskMsg submitMsg(verref,
                          taskid,
                          priority,
                          taskdef);
  pendingTaskCmds.push_back
    (TaskCmd(std::mem_fun(&khResourceManager::SubmitTask), submitMsg));
}

void
khAssetManager::DeleteTask(const std::string &verref)
{
  // assert that we're already locked
  assert(!mutex.TryLock());

  pendingTaskCmds.push_back
    (TaskCmd(std::mem_fun(&khResourceManager::DeleteTask), verref));
}

// ****************************************************************************
// ***  Command Routines - invoked from AssetCmdLoop and HandleClientLoop
// ****************************************************************************
void
khAssetManager::TaskLost(const TaskLostMsg &msg)
{
  assert(!mutex.TryLock());
  notify(NFY_INFO, "TaskLost %s", msg.verref.c_str());

  // No matter what else happens (even exceptions), we need to tell the
  // task manager that we've finished processing the TaskLost. This is
  // because the task processor won't activate anything else until after
  // we've finished. That way we can submit other (perhaps heigher priority)
  // tasks as the result of this TaskLost and have them considered before
  // the next task is launched.
  // The alwaysTaskCmds are sent even if there is an exception during
  // processing. They are sent AFTER the pendingTaskCmds
  alwaysTaskCmds.push_back
    (TaskCmd(std::mem_fun(&khResourceManager::BumpDownBlockers)));

  AssetVersionD ver(msg.verref);
  if (ver->taskid == msg.taskid) {
    MutableAssetVersionD(msg.verref)->HandleTaskLost(msg);
  }
}

void
khAssetManager::TaskProgress(const TaskProgressMsg &msg)
{
  assert(!mutex.TryLock());
  notify(NFY_INFO, "TaskProgress %s", msg.verref.c_str());
  ::HandleTaskProgress(msg);
}

void
khAssetManager::TaskDone(const TaskDoneMsg &msg)
{
  assert(!mutex.TryLock());
  notify(NFY_INFO, "TaskDone %s", msg.verref.c_str());

  // No matter what else happens (even exceptions), we need to tell the
  // task manager that we've finished processing the TaskDone. This is
  // because the task processor won't activate anything else until after
  // we've finished. That way we can submit other (perhaps heigher priority)
  // tasks as the result of this TaskDone and have them considered before
  // the next task is launched.
  // The alwaysTaskCmds are sent even if there is an exception during
  // processing. They are sent AFTER the pendingTaskCmds
  alwaysTaskCmds.push_back
    (TaskCmd(std::mem_fun(&khResourceManager::BumpDownBlockers)));

  ::HandleTaskDone(msg);
}

void
khAssetManager::BuildAsset(const std::string &assetref, bool &needed)
{
  assert(!mutex.TryLock());
  notify(NFY_INFO, "BuildAsset %s", assetref.c_str());
  AssetD asset(assetref);
  needed = false;
  asset->Update(needed);
}

void
khAssetManager::ProductReImport(const std::string &assetref, bool &updated)
{
  assert(!mutex.TryLock());

  updated = false;

  Asset asset(assetref);
  AssetDefs::Type type = asset->type;
  const std::string& subtype = asset->subtype;
  assert(subtype == kProductSubtype && (type == AssetDefs::Vector ||
         type == AssetDefs::Imagery || type == AssetDefs::Terrain) ||
         subtype == kMercatorProductSubtype && type == AssetDefs::Imagery);

  // Get the source file from configuration.
  SourceAssetVersion asset_source(asset->inputs[0]);
  const SourceConfig& old_config = asset_source->config;
  assert(old_config.sources.size());

  // Refresh the size and modification date and compare with current.
  SourceConfig new_config;
  bool modified = false;
  for (size_t i = 0 ; i < old_config.sources.size() ; i++) {
    const SourceConfig::FileInfo& original = old_config.sources[i];
    khFusionURI uri(original.uri);
    std::string src_file = uri.Valid() ? uri.NetworkPath() : original.uri;
    new_config.AddFile(src_file);
    const SourceConfig::FileInfo& refreshed = new_config.sources.back();
    if (original.size != refreshed.size ||
        original.mtime != refreshed.mtime) {
      modified = true;
    }
  }

  // Submit import request if files have changed.
  if (modified) {
    if (type == AssetDefs::Vector) {
      VectorProductAsset prod(assetref);
      VectorProductImport(VectorProductImportRequest(assetref, prod->config,
                                                     prod->meta, new_config));
    } else {
      if (subtype == kMercatorProductSubtype) {
        MercatorRasterProductAsset prod(assetref);
        MercatorRasterProductImport(
            RasterProductImportRequest(prod->type, assetref, prod->config,
                                       prod->meta, new_config));
      } else {
        RasterProductAsset prod(assetref);
        RasterProductImport(
            RasterProductImportRequest(prod->type, assetref, prod->config,
                                       prod->meta, new_config));
      }
    }
    updated = true;
  }
}

void
khAssetManager::CancelVersion(const std::string &verref)
{
  assert(!mutex.TryLock());
  notify(NFY_INFO, "CancelVersion %s", verref.c_str());
  ::CancelVersion(verref);
}

void
khAssetManager::RebuildVersion(const std::string &verref)
{
  assert(!mutex.TryLock());
  notify(NFY_INFO, "RebuildVersion %s", verref.c_str());
  ::RebuildVersion(verref);
}

void
khAssetManager::SetBadVersion(const std::string &verref)
{
  assert(!mutex.TryLock());
  notify(NFY_INFO, "SetBadVersion %s", verref.c_str());
  MutableAssetVersionD(verref)->SetBad();
}

void
khAssetManager::ClearBadVersion(const std::string &verref)
{
  assert(!mutex.TryLock());
  notify(NFY_INFO, "ClearBadVersion %s", verref.c_str());
  MutableAssetVersionD(verref)->ClearBad();
}

void
khAssetManager::CleanVersion(const std::string &verref)
{
  assert(!mutex.TryLock());
  notify(NFY_INFO, "CleanVersion %s", verref.c_str());
  MutableAssetVersionD(verref)->Clean();
}

void
khAssetManager::RasterProductImport(const RasterProductImportRequest &req)
{
  assert(!mutex.TryLock());
  notify(NFY_INFO, "RasterProductImport %s", req.assetname.c_str());
  RasterProductAssetImplD::HandleImportRequest(req);
}

void
khAssetManager::MercatorRasterProductImport(
    const RasterProductImportRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "MercatorRasterProductImport %s", req.assetname.c_str());
  MercatorRasterProductAssetImplD::HandleImportRequest(req);
}

void
khAssetManager::RasterProjectEdit(const RasterProjectEditRequest &req)
{
  assert(!mutex.TryLock());
  notify(NFY_INFO, "RasterProjectEdit %s", req.assetname.c_str());
  RasterProjectAssetImplD::HandleEditRequest(req);
}

void
khAssetManager::RasterProjectNew(const RasterProjectModifyRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "RasterProjectNew %s", req.assetname.c_str());
  RasterProjectAssetImplD::HandleNewRequest(req);
}

void
khAssetManager::RasterProjectModify(const RasterProjectModifyRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "RasterProjectModify %s", req.assetname.c_str());
  RasterProjectAssetImplD::HandleModifyRequest(req);
}

void
khAssetManager::RasterProjectAddTo(const RasterProjectModifyRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "RasterProjectAddTo %s", req.assetname.c_str());
  RasterProjectAssetImplD::HandleAddToRequest(req);
}

void
khAssetManager::RasterProjectDropFrom(
    const RasterProjectDropFromRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "RasterProjectDropFrom %s", req.assetname.c_str());
  RasterProjectAssetImplD::HandleDropFromRequest(req);
}

void
khAssetManager::MercatorRasterProjectEdit(
    const RasterProjectEditRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "MercatorRasterProjectEdit %s", req.assetname.c_str());
  MercatorRasterProjectAssetImplD::HandleEditRequest(req);
}

void
khAssetManager::MercatorRasterProjectNew(
    const RasterProjectModifyRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "MercatorRasterProjectNew %s", req.assetname.c_str());
  MercatorRasterProjectAssetImplD::HandleNewRequest(req);
}

void
khAssetManager::MercatorRasterProjectModify(
    const RasterProjectModifyRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "MercatorRasterProjectModify %s", req.assetname.c_str());
  MercatorRasterProjectAssetImplD::HandleModifyRequest(req);
}

void
khAssetManager::MercatorRasterProjectAddTo(
    const RasterProjectModifyRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "MercatorRasterProjectAddTo %s", req.assetname.c_str());
  MercatorRasterProjectAssetImplD::HandleAddToRequest(req);
}

void
khAssetManager::MercatorRasterProjectDropFrom(
    const RasterProjectDropFromRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "MercatorRasterProjectDropFrom %s", req.assetname.c_str());
  MercatorRasterProjectAssetImplD::HandleDropFromRequest(req);
}

void
khAssetManager::VectorProductImport(const VectorProductImportRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "VectorProductImport %s", req.assetname.c_str());
  VectorProductAssetImplD::HandleImportRequest(req);
}

void
khAssetManager::VectorLayerXEdit(const VectorLayerXEditRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "VectorLayerXEdit %s", req.assetname.c_str());
  VectorLayerXAssetImplD::HandleEditRequest(req);
}

void
khAssetManager::VectorProjectEdit(const VectorProjectEditRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "VectorProjectEdit %s", req.assetname.c_str());
  VectorProjectAssetImplD::HandleEditRequest(req);
}

void
khAssetManager::VectorProjectNew(const VectorProjectNewRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "VectorProjectNew %s", req.assetname.c_str());
  VectorProjectAssetImplD::HandleNewRequest(req);
}

void
khAssetManager::VectorProjectModify(const VectorProjectModifyRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "VectorProjectModify %s", req.assetname.c_str());
  VectorProjectAssetImplD::HandleModifyRequest(req);
}

void
khAssetManager::VectorProjectAddTo(const VectorProjectModifyRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "VectorProjectAddTo %s", req.assetname.c_str());
  VectorProjectAssetImplD::HandleAddToRequest(req);
}

void
khAssetManager::VectorProjectDropFrom(
    const VectorProjectDropFromRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "VectorProjectDropFrom %s", req.assetname.c_str());
  VectorProjectAssetImplD::HandleDropFromRequest(req);
}

void
khAssetManager::DatabaseEdit(const DatabaseEditRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "DatabaseEdit %s", req.assetname.c_str());
  DatabaseAssetImplD::HandleEditRequest(req);
}

void
khAssetManager::DatabaseNew(const DatabaseEditRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "DatabaseNew %s", req.assetname.c_str());
  DatabaseAssetImplD::HandleNewRequest(req);
}

void
khAssetManager::DatabaseModify(const DatabaseEditRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "DatabaseModify %s", req.assetname.c_str());
  DatabaseAssetImplD::HandleModifyRequest(req);
}

void
khAssetManager::MapDatabaseEdit(const MapDatabaseEditRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "MapDatabaseEdit %s", req.assetname.c_str());
  MapDatabaseAssetImplD::HandleEditRequest(req);
}

void
khAssetManager::MapDatabaseNew(const MapDatabaseEditRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "MapDatabaseNew %s", req.assetname.c_str());
  MapDatabaseAssetImplD::HandleNewRequest(req);
}

void
khAssetManager::MapDatabaseModify(const MapDatabaseEditRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "MapDatabaseModify %s", req.assetname.c_str());
  MapDatabaseAssetImplD::HandleModifyRequest(req);
}

void
khAssetManager::MercatorMapDatabaseEdit(const MapDatabaseEditRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "MercatorMapDatabaseEdit %s", req.assetname.c_str());
  MercatorMapDatabaseAssetImplD::HandleEditRequest(req);
}

void
khAssetManager::MercatorMapDatabaseNew(const MapDatabaseEditRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "MercatorMapDatabaseNew %s", req.assetname.c_str());
  MercatorMapDatabaseAssetImplD::HandleNewRequest(req);
}

void
khAssetManager::MercatorMapDatabaseModify(const MapDatabaseEditRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "MercatorMapDatabaseModify %s", req.assetname.c_str());
  MercatorMapDatabaseAssetImplD::HandleModifyRequest(req);
}

void
khAssetManager::GetCurrTasks(const std::string &dummy, TaskLists &ret) {
  unsigned int mutexWaitTime = MiscConfig::Instance().MutexTimedWaitSec;

  // Passing in a timeout will throw khTimedMutexException if mutexWaitTime is
  // exceeded trying to acquire the lock. 
  notify(NFY_INFO, "GetCurrTasks");
  khLockGuard timedLock(theResourceManager.mutex, mutexWaitTime);
  theResourceManager.GetCurrTasks(ret);
}

void
khAssetManager::ReloadConfig(const std::string &dummy) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "ReloadConfig");
  alwaysTaskCmds.push_back
    (TaskCmd(std::mem_fun(&khResourceManager::ReloadConfig)));
}

void
khAssetManager::MapProjectEdit(const MapProjectEditRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "MapProjectEdit %s", req.assetname.c_str());
  MapProjectAssetImplD::HandleEditRequest(req);
}

void
khAssetManager::MapLayerEdit(const MapLayerEditRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "MapLayerEdit %s", req.assetname.c_str());
  MapLayerAssetImplD::HandleEditRequest(req);
}

void
khAssetManager::KMLProjectEdit(const KMLProjectEditRequest &req) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "KMLProjectEdit %s", req.assetname.c_str());
  KMLProjectAssetImplD::HandleEditRequest(req);
}

void
khAssetManager::MakeAssetDir(const std::string &assetdir) {
  assert(!mutex.TryLock());
  notify(NFY_INFO, "MakeAssetDir %s", assetdir.c_str());
  if (assetdir.empty()) {
    return;
  }
  if (assetdir[0] == '/') {
    throw khException
      (kh::tr
       ("INTERNAL ERROR: Attempt to make a non-relative asset directory:\n%1")
       .arg(assetdir.c_str()));
  }
  std::string dir = AssetDefs::AssetPathToFilename(assetdir);
  khMakeDirOrThrow(dir, 0755);
}

// ****************************************************************************
// Protobuf-based requests.
// ****************************************************************************
const std::string khAssetManager::ASSET_STATUS = "AssetStatus";
const std::string khAssetManager::PUSH_DATABASE = "PushDatabase";
const std::string khAssetManager::PUBLISH_DATABASE = "PublishDatabase";
const std::string khAssetManager::GET_TASKS = "GetCurrTasks";

/**
 * Get status of all versions of an asset.
 * TODO: return results as serialized protobuf.
 */
std::string khAssetManager::GetAssetStatus(const std::string asset_name) {
  if (asset_name.empty()) {
    return std::string("NO ASSET NAME");
  }

  AssetVersionRef version_ref(
      AssetDefs::GuessAssetVersionRef(asset_name, "").Bind());
  Asset asset(version_ref.AssetRef());


  if (asset->versions.size()) {
    std::string result;
    // Iterate through all of the versions of the asset.
    // TODO: Fill protobuf here.
    for (AssetStorage::VersionList::const_iterator version_iter =
           asset->versions.begin();
         version_iter != asset->versions.end(); ++version_iter) {
      AssetVersion version(*version_iter);
      result.append(*version_iter);
      result.append(": ");
      result.append(version->PrettyState());
      result.append("|");
    }
    // TODO: Return serialized protobuf here.
    return result;

  } else {
    return std::string("NO VERSIONS");
  }
}

/**
 * Push database according to serialized arguments.
 * @param arguments_string Comma-separated hostname, dbname.
 * @return serialized return values.
 */
// TODO: Receive and return results as serialized protobuf.
std::string khAssetManager::PushDatabase(
    const std::string arguments_string) {

  // TODO: Temporary comma separated parsing until protobufs.
  int idx = arguments_string.find(",");
  if (idx < 0) {
    notify(NFY_WARN, "Push Failed: expected two arguments.");
    return std::string("ERROR: Expected two arguments, not one.");
  }
  std::string hostname = arguments_string.substr(0, idx);
  if (hostname.empty()) {
    hostname = khHostname();
  }

  std::string dbname = arguments_string.substr(idx + 1);
  if (dbname.empty()) {
    notify(NFY_WARN, "Push Failed: expected two arguments.");
    return std::string("ERROR: Expected two arguments, not two.");
  }

  ServerConfig server_stream;
  server_stream.url = hostname;

  ServerConfig server_search;
  server_search.url = hostname;

  geConsoleProgress progress(0, "bytes");
  geConsoleAuth auth("Authentication required.");
  PublisherClient publisher_client(hostname,
                                   server_stream,
                                   server_search,
                                   &progress, &auth);
  std::string gedb_path;
  std::string db_type;
  std::string db_ref;

  if (!AssetVersionImpl::GetGedbPathAndType(
      dbname, &gedb_path, &db_type, &db_ref)) {
    notify(NFY_WARN, "Path Failed: Unable to get database path and type.");
    return "ERROR: Unable to get database path and type.";
  }

  if (!publisher_client.AddDatabase(gedb_path, db_ref)) {
    notify(NFY_WARN, "Add Database Failed: %s",
           publisher_client.ErrMsg().c_str());
    return "ERROR: " + publisher_client.ErrMsg();
  }

  if (!publisher_client.PushDatabase(gedb_path)) {
    notify(NFY_WARN, "Push Failed: %s", publisher_client.ErrMsg().c_str());
    return "ERROR: " + publisher_client.ErrMsg();
  }

  return std::string("Database Successfully Pushed.");
}

/**
 * Publish database according to serialized arguments.
 * @param arguments_string Comma-separated hostname, dbname,
 *                         stream server, and search server.
 * @return serialized return values.
 */
// TODO: Receive and return results as serialized protobuf.
std::string khAssetManager::PublishDatabase(
    const std::string arguments_string) {

  // Note: Since GEE 4.5 we switched to path-based publishing
  // scheme, so PublisherClient API is changed.
  // Since code is not used, just report Fatal error.
  notify(NFY_FATAL, "Publish is not implemented.");

  // TODO: Temporary comma separated parsing until protobufs.
  int idx = arguments_string.find(",");
  if (idx < 0) {
    notify(NFY_WARN, "Publish Failed: expected four arguments.");
    return std::string("ERROR: Expected four arguments, not one.");
  }
  std::string hostname = arguments_string.substr(0, idx);
  if (hostname.empty()) {
    hostname = khHostname();
  }

  int last_idx = idx + 1;
  idx = arguments_string.find(",", last_idx);
  if (idx < 0) {
    notify(NFY_WARN, "Publish Failed: expected four arguments.");
    return std::string("ERROR: Expected four arguments, not two.");
  }
  std::string dbname = arguments_string.substr(last_idx, idx - last_idx);

  last_idx = idx + 1;
  idx = arguments_string.find(",", last_idx);
  if (idx < 0) {
    notify(NFY_WARN, "Publish Failed: expected four arguments.");
    return std::string("ERROR: Expected four arguments, not three.");
  }

  ServerConfig server_stream;
  server_stream.url = hostname;
  server_stream.vs = arguments_string.substr(last_idx, idx - last_idx);

  last_idx = idx + 1;
  ServerConfig server_search;
  server_search.url = hostname;
  server_search.vs = arguments_string.substr(last_idx);

  geConsoleProgress progress(0, "bytes");
  geConsoleAuth auth("Authentication required.");
  PublisherClient publisher_client(hostname,
                                   server_stream,
                                   server_search,
                                   &progress, &auth);
  std::string gedb_path;
  std::string db_type;
  std::string db_ref;

  if (!AssetVersionImpl::GetGedbPathAndType(
      dbname, &gedb_path, &db_type, &db_ref)) {
    notify(NFY_WARN, "Path Failed: Unable to get database path and type.");
    return "ERROR: Unable to get database path and type.";
  }

  if (!publisher_client.PublishDatabase(gedb_path, "", "")) {
    notify(NFY_WARN, "Publish Failed: %s", publisher_client.ErrMsg().c_str());
    return "ERROR: " + publisher_client.ErrMsg();
  }

  return std::string("Database Successfully Published.");
}

std::string khAssetManager::RetrieveTasking(const FusionConnection::RecvPacket& msg) {
  std::string replyPayload;
  try {
    TaskLists ret;
    std::string dummy;
    GetCurrTasks(dummy, ret);
    if (!ToPayload(ret, replyPayload)) {
	    throw khException(kh::tr("Unable to encode %1 reply payload")
	      .arg(ToQString(GET_TASKS)));
	  }
  }
  catch (khTimedMutexException e) {
    // Replying with a string beginning "ERROR:" passes an exception message back to the caller
    // alternatively we could throw an exception but that could flood fusion logs with warnings
    replyPayload = sysManBusyMsg;
  }
  catch (...) {
    // Allow all other exceptions to continue as before
    throw;
  }

  return replyPayload;
}

/*
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

#include "AssetOperation.h"
#include "AssetVersionD.h"
#include "StateUpdater.h"

#include <memory>

std::unique_ptr<StateUpdater> stateUpdater(new StateUpdater());
StorageManagerInterface<AssetVersionImpl> * assetOpStorageManager = &AssetVersion::storageManager();

void RebuildVersion(const SharedString & ref, MiscConfig::GraphOpsType graphOps) {
  if (graphOps >= MiscConfig::FAST_GRAPH_OPS) {
    // Rebuilding an already succeeded asset is quite dangerous!
    // Those who depend on me may have already finished their work with me.
    // If I rebuild, they have the right to recognize that nothing has
    // changed (based on my version # and their config) and just reuse their
    // previous results. Those previous results may reference my outputs.
    // But if my inputs reference disk files that could change (sources
    // overwritten with new versions), I may change some of my outputs,
    // thereby invalidating the cached work from later stages.
    //
    // For this reason, requests to rebuild 'Succeeded' versions will fail.
    // Assets marked 'Bad' were once succeeded, so they too are disallowed.
    // The same logic could hold true for 'Offline' as well.
    {
      // Limit the scope to release the AssetVersion as quickly as possible.
      auto version = assetOpStorageManager->Get(ref);
      if (version && version->state & (AssetDefs::Succeeded | AssetDefs::Offline | AssetDefs::Bad)) {
        throw khException(kh::tr("%1 marked as %2. Refusing to resume.")
                          .arg(ToQString(ref), ToQString(version->state)));
      }
      else if (!version){
        notify(NFY_WARN, "Could not load %s for rebuild", ref.toString().c_str());
        return;
      }
    }

    stateUpdater->SetAndPropagateState(ref, AssetDefs::New, AssetDefs::CanRebuild);
  }
  else {
    MutableAssetVersionD version(ref);
    if (version) {
      version->Rebuild();
    }
    else {
      notify(NFY_WARN, "Could not load %s for rebuild", ref.toString().c_str());
    }
  }
}

void CancelVersion(const SharedString & ref, MiscConfig::GraphOpsType graphOps) {
  // The Cancel operation is currently slower than the legacy code, so we give
  // users the ability to selectively disable it until we can optimize it
  // further.
  if (graphOps >= MiscConfig::ALL_GRAPH_OPS) {
    {
      auto version = assetOpStorageManager->Get(ref);
      if (!version) {
        notify(NFY_WARN, "Could not load %s for cancel", ref.toString().c_str());
        return;
      }
      else if (!version->CanCancel()) {
        throw khException(kh::tr("%1 already %2. Unable to cancel.")
                          .arg(ToQString(ref), ToQString(version->state)));
      }
    }

    stateUpdater->SetAndPropagateState(ref, AssetDefs::Canceled, AssetDefs::NotFinished);
  }
  else {
    MutableAssetVersionD version(ref);
    if (version) {
      version->Cancel();
    }
    else {
      notify(NFY_WARN, "Could not load %s for cancel", ref.toString().c_str());
    }
  }
}

void HandleTaskProgress(const TaskProgressMsg & msg, MiscConfig::GraphOpsType graphOps) {
  if (graphOps >= MiscConfig::FAST_GRAPH_OPS) {
    auto version = assetOpStorageManager->GetMutable(msg.verref);
    if (version && version->taskid == msg.taskid) {
      version->beginTime = msg.beginTime;
      version->progressTime = msg.progressTime;
      version->progress = msg.progress;
      stateUpdater->SetInProgress(version);
    }
    else if (!version) {
      notify(NFY_WARN, "Could not load %s to update progress", msg.verref.c_str());
    }
  }
  else {
    AssetVersionD ver(msg.verref);
    if (ver && ver->taskid == msg.taskid) {
      MutableAssetVersionD(msg.verref)->HandleTaskProgress(msg);
    }
    else if (!ver) {
      notify(NFY_WARN, "Could not load %s to update progress", msg.verref.c_str());
    }
  }
}

void HandleTaskDone(const TaskDoneMsg & msg, MiscConfig::GraphOpsType graphOps) {
  if (graphOps >= MiscConfig::FAST_GRAPH_OPS) {
    auto version = assetOpStorageManager->GetMutable(msg.verref);
    if (version && version->taskid == msg.taskid) {
      version->beginTime = msg.beginTime;
      version->progressTime = msg.endTime;
      version->endTime = msg.endTime;
      if (msg.success) {
        version->ResetOutFiles(msg.outfiles);
        stateUpdater->SetSucceeded(version);
      }
      else {
        stateUpdater->SetFailed(version);
      }
    }
    else if (!version) {
      notify(NFY_WARN, "Could not load %s to mark done", msg.verref.c_str());
    }
  }
  else {
    AssetVersionD ver(msg.verref);
    if (ver && ver->taskid == msg.taskid) {
      MutableAssetVersionD(msg.verref)->HandleTaskDone(msg);
    }
    else if (!ver) {
      notify(NFY_WARN, "Could not load %s to mark done", msg.verref.c_str());
    }
  }
}

// This function ensures that the State Updater stays in sync with any state
// changes that happen in the legacy code.
void HandleExternalStateChange(
    const SharedString & ref,
    AssetDefs::State oldState,
    std::uint32_t numInputsWaitingFor,
    std::uint32_t numChildrenWaitingFor,
    MiscConfig::GraphOpsType graphOps) {
  if (graphOps >= MiscConfig::FAST_GRAPH_OPS) {
    auto version = assetOpStorageManager->Get(ref);
    if (version) {
      // Update the lists of waiting assets
      stateUpdater->UpdateWaitingAssets(version, oldState, {numInputsWaitingFor, numChildrenWaitingFor});
      if (version->state == AssetDefs::Succeeded) {
        // Notify this asset's parents and listeners so that they can decrement
        // their waiting count.
        stateUpdater->UpdateSucceeded(version, false);
      }
    }
    else {
      notify(NFY_WARN, "Could not load %s to update waiting assets", ref.toString().c_str());
    }
  }
}

/*
 * Copyright 2019 The Open GEE Contributors
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

#ifndef STATEUPDATER_H
#define STATEUPDATER_H

#include "AssetVersion.h"
#include "DependentStateTree.h"
#include "khAssetManager.h"
#include "StorageManager.h"
#include "WaitingAssets.h"

#include <vector>

// This class efficiently updates and propagates asset states. Internally, it
// represents the asset versions as a directed acyclic graph, and state updates
// are mostly performed as graph operations.
class StateUpdater
{
  private:
    class UnsupportedException {};
    class SetStateVisitor;

    StorageManagerInterface<AssetVersionImpl> * const storageManager;
    khAssetManagerInterface * const assetManager;
    WaitingAssets waitingListeners;
    WaitingAssets inProgressParents;

    void SetState(
        DependentStateTree & tree,
        DependentStateTreeVertexDescriptor vertex,
        AssetDefs::State newState,
        bool finalStateChange);
    void SetVersionStateAndRunHandlers(
        AssetHandle<AssetVersionImpl> & version,
        AssetDefs::State newState);
    AssetDefs::State RunStateChangeHandlers(
        AssetHandle<AssetVersionImpl> & version,
        AssetDefs::State newState,
        AssetDefs::State oldState);
    AssetDefs::State RunVersionStateChangeHandler(
        AssetHandle<AssetVersionImpl> & version,
        AssetDefs::State newState,
        AssetDefs::State oldState);
    void SendStateChangeNotification(
        const SharedString & name,
        AssetDefs::State state) const;
    void PropagateInProgress(const AssetHandle<AssetVersionImpl> & version);
    void NotifyChildOrInputInProgress(
        const WaitingAssets & waitingAssets,
        const std::vector<SharedString> & toNotify);
    void RecalcState(const SharedString & ref);
    bool IsParent(const AssetHandle<AssetVersionImpl> & version) const
        { return !version->children.empty(); }
  public:
    StateUpdater(
        StorageManagerInterface<AssetVersionImpl> * sm,
        khAssetManagerInterface * am) :
      storageManager(sm), assetManager(am),
      waitingListeners(AssetDefs::Waiting), inProgressParents(AssetDefs::InProgress) {}
    StateUpdater() : StateUpdater(&AssetVersion::storageManager(), &theAssetManager) {}
    void SetStateForRefAndDependents(
        const SharedString & ref,
        AssetDefs::State newState,
        std::function<bool(AssetDefs::State)> updateStatePredicate);
    void UpdateWaitingAssets(
        const AssetHandle<AssetVersionImpl> & version,
        AssetDefs::State oldState);
    void SetInProgress(AssetHandle<AssetVersionImpl> & version);
};

#endif

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

#include <set>
#include <vector>

#include "AssetVersion.h"
#include "DependentStateTree.h"
#include "khAssetManager.h"
#include "StorageManager.h"

// This class efficiently updates the states of lots of asset versions at the
// same time. Internally, this class represents the asset versions as a
// directed acyclic graph, and state updates are performed as graph operations.
class StateUpdater
{
  private:
    class UnsupportedException {};
    class SetStateVisitor;

    StorageManagerInterface<AssetVersionImpl> * const storageManager;
    khAssetManagerInterface * const assetManager;
    std::set<SharedString> waitingListeners;
    std::set<SharedString> inProgressParents;

    void SetState(
        DependentStateTree & tree,
        DependentStateTreeVertexDescriptor vertex,
        AssetDefs::State newState,
        bool temporary);
    void SetVersionStateAndRunHandlers(
        const SharedString & name,
        AssetHandle<AssetVersionImpl> & version,
        AssetDefs::State oldState,
        AssetDefs::State newState,
        bool temporary);
    void SendStateChangeNotification(
        const SharedString & name,
        AssetDefs::State state);
    void HandleProgress(std::set<SharedString> & waitingAssets, const SharedString & ref);
    void NotifyInProgress(const SharedString & ref);
    void RecalcState(const SharedString & ref);
  public:
    StateUpdater(StorageManagerInterface<AssetVersionImpl> * sm, khAssetManagerInterface * am) :
        storageManager(sm), assetManager(am) {}
    StateUpdater() : StateUpdater(&AssetVersion::storageManager(), &theAssetManager) {}
    void SetStateForRefAndDependents(
        const SharedString & ref,
        AssetDefs::State newState,
        std::function<bool(AssetDefs::State)> updateStatePredicate);
    void HandleStateChange(const SharedString & ref, AssetDefs::State newState, AssetDefs::State oldState);
    void SetInProgress(const SharedString & ref);
};

#endif

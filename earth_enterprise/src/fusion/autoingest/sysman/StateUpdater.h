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
#include <map>

#include "DependentStateTree.h"
#include "AssetVersion.h"
#include "StorageManager.h"

// This class efficiently updates the states of lots of asset versions at
// the same time. The idea is that you create a state updater, use it to
// perform one "macro" operation (such as a clean or resume), and then release
// it.
//
// Internally, this class represents the asset versions as a directed
// acyclic graph, and state updates are performed as graph operations.
class StateUpdater
{
  private:
    class SetStateVisitor;

    StorageManagerInterface<AssetVersionImpl> * const storageManager;
    DependentStateTree tree;

    void SetState(
        DependentStateTreeVertexDescriptor vertex,
        AssetDefs::State newState,
        bool sendNotifications);
  public:
    StateUpdater(StorageManagerInterface<AssetVersionImpl> * sm = &AssetVersion::storageManager()) : storageManager(sm) {}
    void SetStateForRefAndDependents(
        const SharedString & ref,
        AssetDefs::State newState,
        std::function<bool(AssetDefs::State)> updateStatePredicate);
};

#endif

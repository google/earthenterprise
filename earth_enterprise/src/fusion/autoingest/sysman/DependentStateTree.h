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

#ifndef DEPENDENT_STATE_TREE_H
#define DEPENDENT_STATE_TREE_H

#include <boost/graph/adjacency_list.hpp>
#include <functional>

#include "AssetVersion.h"
#include "autoingest/.idl/storage/AssetDefs.h"
#include "common/SharedString.h"
#include "StorageManager.h"

struct AssetVertex {
  SharedString name;
  AssetDefs::State state;
  bool inDepTree;
  size_t index; // Used by the dfs function
};

enum DependencyType { INPUT, CHILD, DEPENDENT, DEPENDENT_AND_CHILD };

struct AssetEdge {
  DependencyType type;
};

using DependentStateTree = boost::adjacency_list<
    boost::setS,
    boost::setS,
    boost::directedS,
    AssetVertex,
    AssetEdge>;

using DependentStateTreeVertexDescriptor = DependentStateTree::vertex_descriptor;

DependentStateTree BuildDependentStateTree(
    const SharedString & ref,
    std::function<bool(AssetDefs::State)> includePredicate,
    bool includeDependentChildren,
    bool includeAllChildrenAndInputs,
    StorageManagerInterface<AssetVersionImpl> * sm);

#endif

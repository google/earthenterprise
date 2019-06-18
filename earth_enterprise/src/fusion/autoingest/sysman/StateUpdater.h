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

#ifndef ASSETTREE_H
#define ASSETTREE_H

#include <boost/graph/adjacency_list.hpp>
#include <map>

#include "autoingest/.idl/storage/AssetDefs.h"
#include "common/SharedString.h"

class StateUpdater
{
  private:
    struct AssetVertex {
      SharedString name;
      AssetDefs::State state;
      size_t index; // Used by the dfs function
    };

    enum DependencyType { INPUT, CHILD };

    struct AssetEdge {
      DependencyType type;
    };

    typedef boost::adjacency_list<
        boost::setS,
        boost::setS,
        boost::directedS,
        AssetVertex,
        AssetEdge> TreeType;
    typedef std::map<SharedString, TreeType::vertex_descriptor> VertexMap;

    // Used by dfs function to update states of assets in the tree
    friend struct boost::property_map<TreeType, boost::vertex_index_t>;
    class UpdateStateVisitor;

    TreeType tree;

    TreeType::vertex_descriptor AddSelfAndConnectedAssets(
        const SharedString & ref,
        VertexMap & vertices,
        size_t & index);
    void AddEdge(TreeType::vertex_descriptor from,
                 TreeType::vertex_descriptor to,
                 AssetEdge data);
  public:
    StateUpdater(const SharedString & ref);
    void RecalculateStates();
};

#endif // ASSETTREE_H

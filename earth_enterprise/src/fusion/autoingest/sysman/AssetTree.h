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

class AssetTree
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
        AssetEdge> AssetTreeImpl;
    typedef std::map<SharedString, AssetTreeImpl::vertex_descriptor> VertexMap;

    // Used by dfs function to update states of assets in the tree
    friend struct boost::property_map<AssetTreeImpl, boost::vertex_index_t>;
    class UpdateStateVisitor;

    AssetTreeImpl tree;
    VertexMap vertices;
    size_t index;
    
    AssetTreeImpl::vertex_descriptor AddSelfAndConnectedAssets(const SharedString & ref);
    void AddEdge(AssetTreeImpl::vertex_descriptor from,
                 AssetTreeImpl::vertex_descriptor to,
                 AssetEdge data);
  public:
    AssetTree(const SharedString & ref);
    void RecalculateStates();
};

#endif // ASSETTREE_H

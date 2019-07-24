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
#include <functional>
#include <list>
#include <map>

#include "autoingest/.idl/storage/AssetDefs.h"
#include "common/SharedString.h"

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
    struct AssetVertex {
      AssetRefKey name;
      AssetDefs::State state;
      size_t index; // Used by the dfs function
    };

    enum DependencyType { INPUT, CHILD, DEPENDENT, DEPENDENT_AND_CHILD };

    struct AssetEdge {
      DependencyType type;
    };

    typedef boost::adjacency_list<
        boost::setS,
        boost::setS,
        boost::directedS,
        AssetVertex,
        AssetEdge> TreeType;
    typedef std::map<AssetRefKey, TreeType::vertex_descriptor> VertexMap;

    // Used by dfs function to update states of assets in the tree
    friend struct boost::property_map<TreeType, boost::vertex_index_t>;
    class UpdateStateVisitor;

    TreeType tree;

    inline bool IsDependent(DependencyType type) { return type == DEPENDENT || type == DEPENDENT_AND_CHILD; }

    TreeType::vertex_descriptor BuildTree(const AssetRefKey & ref);
    TreeType::vertex_descriptor AddEmptyVertex(
        const AssetRefKey & ref,
        VertexMap & vertices,
        size_t & index,
        std::list<TreeType::vertex_descriptor> & toFillIn);
    void FillInVertex(
        TreeType::vertex_descriptor vertex,
        VertexMap & vertices,
        size_t & index,
        std::list<TreeType::vertex_descriptor> & toFillIn);
    void AddEdge(TreeType::vertex_descriptor from,
                 TreeType::vertex_descriptor to,
                 AssetEdge data);
    void SetStateForVertexAndDependents(
        TreeType::vertex_descriptor vertex,
        AssetDefs::State newState,
        std::function<bool(AssetDefs::State)> updateStatePredicate);
    void SetState(
        TreeType::vertex_descriptor vertex,
        AssetDefs::State newState,
        bool sendNotifications);
  public:
    StateUpdater() = default;
    void SetStateForRefAndDependents(
        const AssetRefKey & ref,
        AssetDefs::State newState,
        std::function<bool(AssetDefs::State)> updateStatePredicate);
    void RecalculateAndSaveStates();
};

#endif // ASSETTREE_H

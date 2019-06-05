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

#include "AssetOperation.h"
#include "AssetVersionD.h"

#include <algorithm>
#include <boost/graph/adjacency_list.hpp>
#include <functional>
#include <map>
#include <vector>

using namespace boost;
using namespace std;

struct AssetVertex {
  SharedString name;
  AssetDefs::State state;
  size_t index; // Used by the dfs function
};

enum DependencyType { INPUT, CHILD };

struct AssetEdge {
  DependencyType type;
};

typedef adjacency_list<setS, setS, directedS, AssetVertex, AssetEdge> AssetTree;

// The depth_first_search function needs a way to map vertexes to indexes. We
// store a unique index inside each vertex; the code below provides a way for
// boost to access them. These must be defined before including
// depth_first_search.hpp.
template <class Graph>
class InNodeVertexIndexMap {
  public:
    typedef readable_property_map_tag category;
    typedef size_t value_type;
    typedef value_type reference;
    typedef typename Graph::vertex_descriptor key_type;

    InNodeVertexIndexMap(const Graph & graph) : graph(graph) {};
    const Graph & graph;
};

namespace boost {
  template<>
  struct property_map<AssetTree, vertex_index_t> {
    typedef InNodeVertexIndexMap<AssetTree> const_type;
  };

  template<class Graph>
  InNodeVertexIndexMap<Graph> get(vertex_index_t, const Graph & graph) {
    return InNodeVertexIndexMap<Graph>(graph);
  }

  template<class Graph>
  typename InNodeVertexIndexMap<Graph>::value_type get(
      const InNodeVertexIndexMap<Graph> & map,
      typename InNodeVertexIndexMap<Graph>::key_type vertex) {
    return map.graph[vertex].index;
  }
}

#include <boost/graph/depth_first_search.hpp>

void UpdateStateForSelfAndDependentChildren(
    MutableAssetVersionD version,
    AssetDefs::State newState,
    function<bool(AssetDefs::State)> updateStatePredicate) {
  if (updateStatePredicate(version->state)) {
    // Set the state. The OnStateChange handler will take care
    // of stopping any running tasks, etc
    // false -> don't send notifications about the new state because we
    // will change it soon.
    version->SetMyStateOnly(newState, false);
    vector<SharedString> dependents;
    version->DependentChildren(dependents);
    for (auto & child : dependents) {
      UpdateStateForSelfAndDependentChildren(child, newState, updateStatePredicate);
    }
  }
}

// Convenience class for building a graph representation of a version tree
// for a specified version. You call it like a function. Writing a class
// instead of a function is useful to maintain temporary state needed to
// build the graph.
class GetAssetTree {
  private:
    typedef map<SharedString, AssetTree::vertex_descriptor> VertexMap;
    SharedString version;
    AssetTree tree;
    VertexMap vertices;
    size_t index;
    void AddEdge(AssetTree::vertex_descriptor from,
                 AssetTree::vertex_descriptor to,
                 AssetEdge data,
                 AssetTree & tree) {
      auto edgeData = add_edge(from, to, tree);
      if (edgeData.second) tree[edgeData.first] = data;
    }
    AssetTree::vertex_descriptor AddSelfAndConnectedAssets(const SharedString & ref) {
      auto myVertexIter = vertices.find(ref);
      if (myVertexIter == vertices.end()) {
        // I'm not in the graph yet, so make a new vertex and add my connections
        AssetVersion version(ref);
        auto myVertex = add_vertex(tree);
        tree[myVertex] = {ref, version->state, index};
        ++index;
        vertices[version->GetRef()] = myVertex;
        for (const auto & child : version->children) {
          auto childVertex = AddSelfAndConnectedAssets(child);
          AddEdge(myVertex, childVertex, {CHILD}, tree);
        }
        for (const auto & input : version->inputs) {
          auto inputVertex = AddSelfAndConnectedAssets(input);
          AddEdge(myVertex, inputVertex, {INPUT}, tree);
        }
        for (const auto & parent : version->parents) {
          auto parentVertex = AddSelfAndConnectedAssets(parent);
          AddEdge(parentVertex, myVertex, {CHILD}, tree);
        }
        for (const auto & listener : version->listeners) {
          auto listenerVertex = AddSelfAndConnectedAssets(listener);
          AddEdge(listenerVertex, myVertex, {INPUT}, tree);
        }
        return myVertex;
      }
      else {
        // I'm already in the graph, so someone else is adding my connections.
        // Just return my vertex descriptor.
        return myVertexIter->second;
      }
    }
  public:
    GetAssetTree(const SharedString & version) : version(version), index(0) {
      AddSelfAndConnectedAssets(version);
    }
    operator AssetTree() { return tree; }
};

class UpdateStateVisitor : public default_dfs_visitor {
  private:
    class InputStates {
      private:
        uint numinputs = 0;
        uint numgood = 0;
        uint numblocking = 0;
        uint numoffline = 0;
      public:
        void Add(AssetDefs::State inputState) {
          ++numinputs;
          if (inputState == AssetDefs::Succeeded) {
            ++numgood;
          } else if (inputState == AssetDefs::Offline) {
            ++numblocking;
            ++numoffline;
          } else if (inputState & (AssetDefs::Blocked |
                                   AssetDefs::Failed |
                                   AssetDefs::Canceled |
                                   AssetDefs::Bad)) {
            ++numblocking;
          }
        }
        void GetOutputs(AssetDefs::State & stateByInputs, bool & blockersAreOffline, uint32 & numWaitingFor) {
          if (numinputs == numgood) {
            stateByInputs = AssetDefs::Queued;
          } else if (numblocking) {
            stateByInputs = AssetDefs::Blocked;
          } else {
            stateByInputs = AssetDefs::Waiting;
          }

          blockersAreOffline = (numblocking == numoffline);

          if (stateByInputs == AssetDefs::Waiting) {
            numWaitingFor = (numinputs - numgood);
          } else {
            numWaitingFor = 0;
          }
        }
    };
    class ChildStates {
      private:
        uint numkids = 0;
        uint numgood = 0;
        uint numblocking = 0;
        uint numinprog = 0;
        uint numfailed = 0;
      public:
        void Add(AssetDefs::State childState) {
          ++numkids;
          if (childState == AssetDefs::Succeeded) {
            ++numgood;
          } else if (childState == AssetDefs::Failed) {
            ++numfailed;
          } else if (childState == AssetDefs::InProgress) {
            ++numinprog;
          } else if (childState & (AssetDefs::Blocked  |
                                   AssetDefs::Canceled |
                                   AssetDefs::Offline  |
                                   AssetDefs::Bad)) {
            ++numblocking;
          }
        }
        void GetOutputs(AssetDefs::State & stateByChildren) {
          if (numkids == numgood) {
            stateByChildren = AssetDefs::Succeeded;
          } else if (numblocking || numfailed) {
            stateByChildren = AssetDefs::Blocked;
          } else if (numgood || numinprog) {
            stateByChildren = AssetDefs::InProgress;
          } else {
            stateByChildren = AssetDefs::Queued;
          }
        }
    };

    void CalcStateByInputsAndChildren(
        AssetTree::vertex_descriptor vertex,
        const AssetTree & tree,
        AssetDefs::State &stateByInputs,
        AssetDefs::State &stateByChildren,
        bool & blockersAreOffline,
        uint32 & numWaitingFor) const {
      InputStates inputStates;
      ChildStates childStates;

      auto edgeIters = out_edges(vertex, tree);
      auto edgeBegin = edgeIters.first;
      auto edgeEnd = edgeIters.second;
      for (auto i = edgeBegin; i != edgeEnd; ++i) {
        DependencyType type = tree[*i].type;
        AssetTree::vertex_descriptor dep = i->m_target;
        AssetDefs::State depState = tree[dep].state;
        switch(type) {
          case INPUT:
            inputStates.Add(depState);
            break;
          case CHILD:
            childStates.Add(depState);
            break;
        }
      }

      inputStates.GetOutputs(stateByInputs, blockersAreOffline, numWaitingFor);
      childStates.GetOutputs(stateByChildren);
    }
  public:
    // Update the state of an asset after we've updated the state of its
    // inputs and children.
    virtual void finish_vertex(AssetTree::vertex_descriptor vertex, const AssetTree & tree) const {
      // TODO: could optimize here to short circuit if no inputs or children changed state
      MutableAssetVersionD version(tree[vertex].name);
      if (!version->NeedComputeState()) return;
      AssetDefs::State stateByInputs;
      AssetDefs::State stateByChildren;
      bool blockersAreOffline;
      uint32 numWaitingFor;
      CalcStateByInputsAndChildren(vertex, tree, stateByInputs, stateByChildren, blockersAreOffline, numWaitingFor);
      AssetDefs::State newState;
      newState = version->CalcStateByInputsAndChildren(stateByInputs, stateByChildren, blockersAreOffline, numWaitingFor);
      // Set the state and send notifications but don't propagate the change
      // (we will take care of propagation during the graph traversal).
      version->SetMyStateOnly(newState);
      // Update the state in the tree because other assets may need it to compute
      // their own states. Use the state from the version because setting the
      // state can sometimes trigger additional state changes.
      const_cast<AssetTree&>(tree)[vertex].state = version->state;
    }
};

void RebuildVersion(const SharedString & ref) {
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
  AssetVersion version(ref);
  if (version->state & (AssetDefs::Succeeded | AssetDefs::Offline | AssetDefs::Bad)) {
    throw khException(kh::tr("%1 marked as %2. Refusing to resume.")
                      .arg(ToQString(ref), ToQString(version->state)));
  }

  UpdateStateForSelfAndDependentChildren(ref, AssetDefs::New, AssetDefs::CanRebuild);
  AssetTree assets = GetAssetTree(ref);
  depth_first_search(assets, visitor(UpdateStateVisitor()));
  // Possible optimization: Many assets have significant overlap in their
  // inputs. It might save time if we could calculate the overlapping state
  // only once.
}

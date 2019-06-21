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

#include "StateUpdater.h"
#include "AssetVersionD.h"

using namespace boost;
using namespace std;

// The depth_first_search function needs a way to map vertices to indexes. We
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
  struct property_map<StateUpdater::TreeType, vertex_index_t> {
    typedef InNodeVertexIndexMap<StateUpdater::TreeType> const_type;
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

StateUpdater::TreeType::vertex_descriptor
StateUpdater::BuildTree(const SharedString & ref) {
  VertexMap vertices;
  size_t index = 0;
  list<TreeType::vertex_descriptor> toFillIn, toFillInNext;
  // First create an empty vertex for the provided asset. Then fill it in,
  // which includes adding its connections to other assets. Every time we fill
  // in a node we will get new assets to add to the tree until all assets have
  // been added. This basically builds the tree using a breadth first search,
  // which allows us to keep memory usage (relatively) low by not forcing
  // assets to stay in the cache and limiting the size of the toFillIn and
  // toFillInNext lists.
  auto myVertex = AddEmptyVertex(ref, vertices, index, toFillIn);
  while (toFillIn.size() > 0) {
    for (auto vertex : toFillIn) {
      FillInVertex(vertex, vertices, index, toFillInNext);
    }
    toFillIn = std::move(toFillInNext);
    toFillInNext.clear();
  }
  return myVertex;
}

// Creates an "empty" node for this asset if it has not already been added to
// the tree. The node has a default state and doesn't include links to
// inputs/children/etc. The vertex must be "filled in" by calling FillInVertex
// before it can be used.
StateUpdater::TreeType::vertex_descriptor
StateUpdater::AddEmptyVertex(
    const SharedString & ref,
    VertexMap & vertices,
    size_t & index,
    list<TreeType::vertex_descriptor> & toFillIn) {
  auto myVertexIter = vertices.find(ref);
  if (myVertexIter == vertices.end()) {
    // I'm not in the graph yet, so make a new empty vertex and let the caller
    // know we need to load it with the correct information
    auto myVertex = add_vertex(tree);
    tree[myVertex] = {ref, AssetDefs::New, index};
    ++index;
    vertices[ref] = myVertex;
    toFillIn.push_back(myVertex);
    return myVertex;
  }
  else {
    // I'm already in the graph, so just return my vertex descriptor.
    return myVertexIter->second;
  }
}

// "Fills in" an existing vertex with the state of an asset and its connections
// to other assets. Adds any new nodes that need to be filled in to toFillIn.
void StateUpdater::FillInVertex(
    TreeType::vertex_descriptor myVertex,
    VertexMap & vertices,
    size_t & index,
    list<TreeType::vertex_descriptor> & toFillIn) {
  AssetVersionD version(tree[myVertex].name);
  tree[myVertex].state = version->state;
  vector<SharedString> dependents;
  version->DependentChildren(dependents);
  // Add the dependent children first. When we add the children next it will
  // skip any that were already added
  for (const auto & dep : dependents) {
    auto depVertex = AddEmptyVertex(dep, vertices, index, toFillIn);
    AddEdge(myVertex, depVertex, {DEPENDENT_CHILD});
  }
  for (const auto & child : version->children) {
    auto childVertex = AddEmptyVertex(child, vertices, index, toFillIn);
    AddEdge(myVertex, childVertex, {CHILD});
  }
  for (const auto & input : version->inputs) {
    auto inputVertex = AddEmptyVertex(input, vertices, index, toFillIn);
    AddEdge(myVertex, inputVertex, {INPUT});
  }
  for (const auto & parent : version->parents) {
    auto parentVertex = AddEmptyVertex(parent, vertices, index, toFillIn);
    AddEdge(parentVertex, myVertex, {CHILD});
  }
  for (const auto & listener : version->listeners) {
    auto listenerVertex = AddEmptyVertex(listener, vertices, index, toFillIn);
    AddEdge(listenerVertex, myVertex, {INPUT});
  }
}

void StateUpdater::AddEdge(
    TreeType::vertex_descriptor from,
    TreeType::vertex_descriptor to,
    AssetEdge data) {
  auto edgeData = add_edge(from, to, tree);
  // If this is a new edge, set the edge data. Also, if this is a dependent
  // child it may have been added previously as a "normal" child, so update
  // the data in that case also.
  if (edgeData.second || data.type == DEPENDENT_CHILD) {
    tree[edgeData.first] = data;
  }
}

void StateUpdater::SetStateForRefAndDependents(
    const SharedString & ref,
    AssetDefs::State newState,
    std::function<bool(AssetDefs::State)> updateStatePredicate) {
  auto refVertex = BuildTree(ref);
  SetStateForVertexAndDependents(refVertex, newState, updateStatePredicate);
}

void StateUpdater::SetStateForVertexAndDependents(
    TreeType::vertex_descriptor vertex,
    AssetDefs::State newState,
    std::function<bool(AssetDefs::State)> updateStatePredicate) {
  if (updateStatePredicate(tree[vertex].state)) {
    {
      // Limit the scope of the MutableAssetVersionD so that we release it
      // as quickly as possible.
      MutableAssetVersionD version(tree[vertex].name);
      // Set the state. The OnStateChange handler will take care
      // of stopping any running tasks, etc
      // false -> don't send notifications about the new state because we
      // will change it soon.
      version->SetMyStateOnly(newState, false);
    }
    auto edgeIters = out_edges(vertex, tree);
    auto edgeBegin = edgeIters.first;
    auto edgeEnd = edgeIters.second;
    for (auto i = edgeBegin; i != edgeEnd; ++i) {
      if (tree[*i].type == DEPENDENT_CHILD) {
        SetStateForVertexAndDependents(target(*i, tree), newState, updateStatePredicate);
      }
    }
  }
}

class StateUpdater::UpdateStateVisitor : public default_dfs_visitor {
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

    void GetStateInputs(
        StateUpdater::TreeType::vertex_descriptor vertex,
        const StateUpdater::TreeType & tree,
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
        StateUpdater::DependencyType type = tree[*i].type;
        StateUpdater::TreeType::vertex_descriptor dep = target(*i, tree);
        AssetDefs::State depState = tree[dep].state;
        switch(type) {
          case StateUpdater::INPUT:
            inputStates.Add(depState);
            break;
          case StateUpdater::CHILD:
          case StateUpdater::DEPENDENT_CHILD:
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
    virtual void finish_vertex(
        StateUpdater::TreeType::vertex_descriptor vertex,
        const StateUpdater::TreeType & tree) const {
      AssetVersionD version(tree[vertex].name);
      if (!version->NeedComputeState()) return;
      AssetDefs::State stateByInputs;
      AssetDefs::State stateByChildren;
      bool blockersAreOffline;
      uint32 numWaitingFor;
      GetStateInputs(vertex, tree, stateByInputs, stateByChildren, blockersAreOffline, numWaitingFor);
      AssetDefs::State newState = 
          version->CalcStateByInputsAndChildren(stateByInputs, stateByChildren, blockersAreOffline, numWaitingFor);
      if (newState != tree[vertex].state) {
        // Set the state and send notifications but don't propagate the change
        // (we will take care of propagation during the graph traversal).
        MutableAssetVersionD mutableVersion(tree[vertex].name);
        mutableVersion->SetMyStateOnly(newState);
        // Update the state in the tree because other assets may need it to compute
        // their own states. Use the state from the version because setting the
        // state can sometimes trigger additional state changes.
        const_cast<StateUpdater::TreeType&>(tree)[vertex].state = mutableVersion->state;
      }
    }
};

void StateUpdater::RecalculateStates() {
  // Possible optimization: Many assets have significant overlap in their
  // inputs. It might save time if we could calculate the overlapping state
  // only once.
  depth_first_search(tree, visitor(UpdateStateVisitor()));
}

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
#include "AssetVersion.h"
#include "common/notify.h"

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

struct StateUpdater::TreeBuildData {
  VertexMap vertices;
  size_t index = 0;
  function<bool(AssetDefs::State)> includePredicate;
};

// Builds a tree containing the specified asset, its depedent children, their
// dependent children, and so on, along with any other assets that are needed
// to update the state of these assets. That includes their inputs and children,
// their parents and listeners all the way up the tree, and the inputs and
// children for their parents and listeners.
void StateUpdater::BuildDependentTree(
    const SharedString & ref,
    function<bool(AssetDefs::State)> includePredicate) {
  TreeBuildData buildData;
  buildData.includePredicate = includePredicate;
  set<TreeType::vertex_descriptor> toFillIn, toFillInNext;
  // First create an empty vertex for the provided asset. Then fill it in,
  // which includes adding its connections to other assets. Every time we fill
  // in a node we will get new assets to add to the tree until all assets have
  // been added. This basically builds the tree using a breadth first search,
  // which allows us to keep memory usage (relatively) low by not forcing
  // assets to stay in the cache and limiting the size of the toFillIn and
  // toFillInNext lists.
  AddOrUpdateVertex(ref, buildData, true, true, toFillIn);
  while (toFillIn.size() > 0) {
    for (auto vertex : toFillIn) {
      FillInVertex(vertex, buildData, toFillInNext);
    }
    toFillIn = std::move(toFillInNext);
    toFillInNext.clear();
  }
}

// Creates an "empty" node for this asset if it has not already been added to
// the tree. The node has a default state and doesn't include links to
// inputs/children/etc. The vertex must be "filled in" by calling FillInVertex
// before it can be used.
StateUpdater::TreeType::vertex_descriptor
StateUpdater::AddOrUpdateVertex(
    const SharedString & ref,
    TreeBuildData & buildData,
    bool inDepTree,
    bool recalcState,
    set<TreeType::vertex_descriptor> & toFillIn) {
  auto myVertexIter = buildData.vertices.find(ref);
  if (myVertexIter == buildData.vertices.end()) {
    // I'm not in the graph yet, so make a new empty vertex and let the caller
    // know we need to load it with the correct information
    auto myVertex = add_vertex(tree);
    tree[myVertex] = {ref, AssetDefs::New, inDepTree, recalcState, false, buildData.index};
    ++buildData.index;
    buildData.vertices[ref] = myVertex;
    toFillIn.insert(myVertex);
    return myVertex;
  }
  else {
    // I'm already in the graph. If we learned new information that we didn't
    // know when I was added to the tree (e.g., I'm in the dependent tree),
    // add this to the list of vertexes to fill in so that we can add its
    // connections. Then return the existing descriptor.
    auto myVertex = myVertexIter->second;
    auto & myVertexData = tree[myVertex];
    if (inDepTree && !myVertexData.inDepTree) {
      myVertexData.inDepTree = true;
      toFillIn.insert(myVertex);
    }
    if (recalcState && !myVertexData.recalcState) {
      myVertexData.recalcState = true;
      toFillIn.insert(myVertex);
    }
    return myVertex;
  }
}

// "Fills in" an existing vertex with the state of an asset and its connections
// to other assets. Adds any new nodes that need to be filled in to toFillIn.
void StateUpdater::FillInVertex(
    TreeType::vertex_descriptor myVertex,
    TreeBuildData & buildData,
    set<TreeType::vertex_descriptor> & toFillIn) {
  SharedString name = tree[myVertex].name;
  notify(NFY_PROGRESS, "Loading '%s' for state update", name.toString().c_str());
  auto version = storageManager->Get(name);
  if (!version) {
    notify(NFY_WARN, "Could not load asset '%s' which is referenced by another asset.",
           name.toString().c_str());
    // Set it to a bad state, but use a state that can be fixed by another
    // rebuild operation.
    tree[myVertex].state = AssetDefs::Blocked;
    return;
  }
  tree[myVertex].state = version->state;
  // If this vertex is in the dependent tree but doesn't need to be included,
  // act as though it's not in the dependency tree. We'll leave it in the tree
  // to avoid messing up the index numbering and in case it is needed as an
  // input, but we won't bring in any of its dependents.
  if (tree[myVertex].inDepTree && !buildData.includePredicate(tree[myVertex].state)) {
    tree[myVertex].inDepTree = false;
    tree[myVertex].recalcState = false;
  }
  // If I'm in the dependency tree I need to add my dependents because they are
  // also in the dependency tree.
  if (tree[myVertex].inDepTree) {
    vector<SharedString> dependents;
    version->DependentChildren(dependents);
    for (const auto & dep : dependents) {
      auto depVertex = AddOrUpdateVertex(dep, buildData, true, true, toFillIn);
      AddEdge(myVertex, depVertex, {DEPENDENT});
    }
  }
  // If I need to recalculate my state, I need to have my children and inputs
  // in the tree because my state is based on them. In addition, I need my
  // parents and listeners, because they may also have to recalculate their
  // state. If I don't need to recalculate my state, I don't need to add any of
  // my connections. I'm only used to calculate someone else's state.
  if (tree[myVertex].recalcState) {
    for (const auto & child : version->children) {
      auto childVertex = AddOrUpdateVertex(child, buildData, false, false, toFillIn);
      AddEdge(myVertex, childVertex, {CHILD});
    }
    for (const auto & input : version->inputs) {
      auto inputVertex = AddOrUpdateVertex(input, buildData, false, false, toFillIn);
      AddEdge(myVertex, inputVertex, {INPUT});
    }
    for (const auto & parent : version->parents) {
      auto parentVertex = AddOrUpdateVertex(parent, buildData, false, true, toFillIn);
      AddEdge(parentVertex, myVertex, {CHILD});
    }
    for (const auto & listener : version->listeners) {
      auto listenerVertex = AddOrUpdateVertex(listener, buildData, false, true, toFillIn);
      AddEdge(listenerVertex, myVertex, {INPUT});
    }
  }
}

void StateUpdater::AddEdge(
    TreeType::vertex_descriptor from,
    TreeType::vertex_descriptor to,
    AssetEdge data) {
  auto edgeData = add_edge(from, to, tree);
  if (edgeData.second) {
    // This is a new edge
    tree[edgeData.first] = data;
  }
  else {
    // Check if this is both a dependent and a child
    DependencyType currentType = tree[edgeData.first].type;
    DependencyType newType = data.type;
    if ((currentType == DEPENDENT && newType == CHILD) ||
        (currentType == CHILD && newType == DEPENDENT)) {
      tree[edgeData.first].type = DEPENDENT_AND_CHILD;
    }
  }
}

class StateUpdater::SetStateVisitor : public default_dfs_visitor {
  private:
    StateUpdater * const updater;
    const AssetDefs::State newState;

    bool NeedComputeState(AssetDefs::State state) const {
      // these states are explicitly set and must be explicitly cleared
      return !(state & (AssetDefs::Bad |
                        AssetDefs::Offline |
                        AssetDefs::Canceled));
    }

    // Helper class for calculating state from inputs
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
    // Helper class for calculating state from children
    class ChildStates {
      private:
        uint numkids = 0;
        uint numgood = 0;
        uint numblocking = 0;
        uint numinprog = 0;
      public:
        void Add(AssetDefs::State childState) {
          ++numkids;
          if (childState == AssetDefs::Succeeded) {
            ++numgood;
          } else if (childState == AssetDefs::InProgress) {
            ++numinprog;
          } else if (childState & (AssetDefs::Failed   |
                                   AssetDefs::Blocked  |
                                   AssetDefs::Canceled |
                                   AssetDefs::Offline  |
                                   AssetDefs::Bad)) {
            ++numblocking;
          }
        }
        void GetOutputs(AssetDefs::State & stateByChildren) {
          if (numkids == numgood) {
            stateByChildren = AssetDefs::Succeeded;
          } else if (numblocking) {
            stateByChildren = AssetDefs::Blocked;
          } else if (numgood || numinprog) {
            stateByChildren = AssetDefs::InProgress;
          } else {
            stateByChildren = AssetDefs::Queued;
          }
        }
    };

    // Loops through the inputs and children of an asset and calculates
    // everything the asset verion needs to know to figure out its state. This
    // data will be passed to the asset version so it can calculate its own
    // state.
    void CalculateStateParameters(
        StateUpdater::TreeType::vertex_descriptor vertex,
        const StateUpdater::TreeType & tree,
        AssetDefs::State &stateByInputs,
        AssetDefs::State &stateByChildren,
        bool & blockersAreOffline,
        uint32 & numWaitingFor,
        bool & childOrInputStateChanged) const {
      InputStates inputStates;
      ChildStates childStates;

      childOrInputStateChanged = false;
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
            childOrInputStateChanged = childOrInputStateChanged || tree[dep].stateChanged;
            break;
          case StateUpdater::CHILD:
          case StateUpdater::DEPENDENT_AND_CHILD:
            childStates.Add(depState);
            childOrInputStateChanged = childOrInputStateChanged || tree[dep].stateChanged;
            break;
          case StateUpdater::DEPENDENT:
            // Dependents that are not also children are not considered when
            // calculating state.
            break;
        }
      }

      inputStates.GetOutputs(stateByInputs, blockersAreOffline, numWaitingFor);
      childStates.GetOutputs(stateByChildren);
    }

  public:
    SetStateVisitor(StateUpdater * updater, AssetDefs::State newState) :
        updater(updater), newState(newState) {}

    // This function is called after the DFS has completed for every vertex
    // below this one in the tree. Thus, we don't calculate the state for an
    // asset until we've calculated the state for all of its inputs and
    // children.
    virtual void finish_vertex(
        TreeType::vertex_descriptor vertex,
        const StateUpdater::TreeType & tree) const {
      if (!tree[vertex].recalcState) return;
      SharedString name = tree[vertex].name;
      notify(NFY_PROGRESS, "Calculating state for '%s'", name.toString().c_str());

      // Set the state for assets in the dependent tree. Don't send
      // notifications because we'll set the state again below.
      if (tree[vertex].inDepTree) {
        updater->SetState(vertex, newState, false);
      }

      // For all assets (including parents and listeners) update the state
      // based on the state of inputs and children.
      if (NeedComputeState(tree[vertex].state)) {
        AssetDefs::State stateByInputs;
        AssetDefs::State stateByChildren;
        bool blockersAreOffline;
        uint32 numWaitingFor;
        bool childOrInputStateChanged;
        CalculateStateParameters(
              vertex, tree, stateByInputs, stateByChildren,
              blockersAreOffline, numWaitingFor, childOrInputStateChanged);
        if (tree[vertex].stateChanged || childOrInputStateChanged) {
          AssetDefs::State calculatedState;
          // Run this in a separate block so that the asset version is released
          // before we try to update it.
          {
            auto version = updater->storageManager->Get(name);
            if (!version) {
              // This shoud never happen - we had to successfully load the asset
              // previously to get it into the tree.
              notify(NFY_WARN, "Could not load asset '%s' to recalculate state.",
                     name.toString().c_str());
              return;
            }
            calculatedState = version->CalcStateByInputsAndChildren(
                  stateByInputs, stateByChildren, blockersAreOffline, numWaitingFor);
          }
          // Set the state and send notifications.
          updater->SetState(vertex, calculatedState, true);
        }
      }
    }
};

void StateUpdater::SetStateForRefAndDependents(
    const SharedString & ref,
    AssetDefs::State newState,
    function<bool(AssetDefs::State)> updateStatePredicate) {
  SharedString verref = AssetVersionImpl::Key(ref);
  BuildDependentTree(verref, updateStatePredicate);
  depth_first_search(tree, visitor(SetStateVisitor(this, newState)));
}

void StateUpdater::SetState(
    TreeType::vertex_descriptor vertex,
    AssetDefs::State newState,
    bool sendNotifications) {
  SharedString name = tree[vertex].name;
  if (newState != tree[vertex].state) {
    auto version = storageManager->GetMutable(name);
    notify(NFY_PROGRESS, "Setting state of '%s' to '%s'",
           name.toString().c_str(), ToString(newState).c_str());
    if (version) {
      // Set the state. The OnStateChange handler will take care
      // of stopping any running tasks, etc.
      // This call does not propagate the state change to other assets. We will
      // take care of that inside the state updater.
      version->SetMyStateOnly(newState, sendNotifications);
      // Setting the state can trigger additional state changes, so get the new
      // state directly from the asset version.
      tree[vertex].state = version->state;
      tree[vertex].stateChanged = true;
    }
    else {
      // This shoud never happen - we had to successfully load the asset
      // previously to get it into the tree.
      notify(NFY_WARN, "Could not load asset '%s' to set state.",
             name.toString().c_str());
    }
  }
}

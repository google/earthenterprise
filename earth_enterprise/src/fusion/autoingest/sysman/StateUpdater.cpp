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

// Builds the asset version tree containing the specified asset version.
StateUpdater::TreeType::vertex_descriptor
StateUpdater::BuildDependentTreeForStateCalculation(const SharedString & ref) {
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
  auto myVertex = AddEmptyVertex(ref, vertices, index, true, toFillIn);
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
    bool recalcState,
    list<TreeType::vertex_descriptor> & toFillIn) {
  auto myVertexIter = vertices.find(ref);
  if (myVertexIter == vertices.end()) {
    // I'm not in the graph yet, so make a new empty vertex and let the caller
    // know we need to load it with the correct information
    auto myVertex = add_vertex(tree);
    tree[myVertex] = {ref, AssetDefs::New, recalcState, index};
    ++index;
    vertices[ref] = myVertex;
    toFillIn.push_back(myVertex);
    return myVertex;
  }
  else {
    // I'm already in the graph. Make sure recalcState is set correctly and
    // return my existing vertex descriptor.
    auto myVertex = myVertexIter->second;
    tree[myVertex].recalcState = tree[myVertex].recalcState || recalcState;
    return myVertex;
  }
}

// "Fills in" an existing vertex with the state of an asset and its connections
// to other assets. Adds any new nodes that need to be filled in to toFillIn.
void StateUpdater::FillInVertex(
    TreeType::vertex_descriptor myVertex,
    VertexMap & vertices,
    size_t & index,
    list<TreeType::vertex_descriptor> & toFillIn) {
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
  // The ref passed in may be slightly different than the ref used in the storage
  // manager, so fix that here.
  if (name != version->GetRef()) {
    name = version->GetRef();
    tree[myVertex].name = name;
    vertices[name] = myVertex;
  }
  // If I need to recalculate my state, I need to have my children and inputs
  // in the tree because my state is based on them. In addition, I need my
  // dependent children, parents, and listeners, because they may also have to
  // recalculate their state. If I don't need to recalculate my state, I don't
  // need to add any of my connections. I'm only used to calculate someone
  // else's state.
  if (tree[myVertex].recalcState) {
    vector<SharedString> dependents;
    version->DependentChildren(dependents);
    for (const auto & dep : dependents) {
      auto depVertex = AddEmptyVertex(dep, vertices, index, true, toFillIn);
      AddEdge(myVertex, depVertex, {DEPENDENT});
    }
    for (const auto & child : version->children) {
      auto childVertex = AddEmptyVertex(child, vertices, index, false, toFillIn);
      AddEdge(myVertex, childVertex, {CHILD});
    }
    for (const auto & input : version->inputs) {
      auto inputVertex = AddEmptyVertex(input, vertices, index, false, toFillIn);
      AddEdge(myVertex, inputVertex, {INPUT});
    }
    for (const auto & parent : version->parents) {
      auto parentVertex = AddEmptyVertex(parent, vertices, index, true, toFillIn);
      AddEdge(parentVertex, myVertex, {CHILD});
    }
    for (const auto & listener : version->listeners) {
      auto listenerVertex = AddEmptyVertex(listener, vertices, index, true, toFillIn);
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

void StateUpdater::SetStateForRefAndDependents(
    const SharedString & ref,
    AssetDefs::State newState,
    function<bool(AssetDefs::State)> updateStatePredicate) {
  SharedString verref = AssetVersionRef::Bind(ref);
  auto refVertex = BuildDependentTreeForStateCalculation(verref);
  SetStateForVertexAndDependents(refVertex, newState, updateStatePredicate);
}

// Sets the state for the specified ref and recursively sets the state for
// the ref's dependent children.
void StateUpdater::SetStateForVertexAndDependents(
    TreeType::vertex_descriptor vertex,
    AssetDefs::State newState,
    function<bool(AssetDefs::State)> updateStatePredicate) {
  if (updateStatePredicate(tree[vertex].state)) {
    // Set the state. The OnStateChange handler will take care
    // of stopping any running tasks, etc
    // false -> don't send notifications about the new state because we
    // will change it soon.
    SetState(vertex, newState, false);
    
    // Now update the dependent children
    auto edgeIters = out_edges(vertex, tree);
    auto edgeBegin = edgeIters.first;
    auto edgeEnd = edgeIters.second;
    for (auto i = edgeBegin; i != edgeEnd; ++i) {
      if (IsDependent(tree[*i].type)) {
        SetStateForVertexAndDependents(target(*i, tree), newState, updateStatePredicate);
      }
    }
  }
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
    }
    else {
      // This shoud never happen - we had to successfully load the asset
      // previously to get it into the tree.
      notify(NFY_WARN, "Could not load asset '%s' to set state.",
             name.toString().c_str());
    }
  }
}

// Helper class to calculate the state of asset versions based on the states
// of their inputs and children. It calculates states in depth-first order;
// we use the finish_vertex function to ensure that we calculate the state
// of an asset version after we've calculated the states of its inputs
// and children.
class StateUpdater::UpdateStateVisitor : public default_dfs_visitor {
  private:
    // Keep a pointer to the state updater
    StateUpdater * const updater;
    
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
          case StateUpdater::DEPENDENT_AND_CHILD:
            childStates.Add(depState);
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
    UpdateStateVisitor(StateUpdater * updater) : updater(updater) {};

    // Update the state of an asset after we've updated the state of its
    // inputs and children.
    virtual void finish_vertex(
        StateUpdater::TreeType::vertex_descriptor vertex,
        const StateUpdater::TreeType & tree) const {
      SharedString name = tree[vertex].name;
      if (!tree[vertex].recalcState) return;
      notify(NFY_PROGRESS, "Calculating state for '%s'", name.toString().c_str());
      auto version = updater->storageManager->Get(name);
      if (!version) {
        // This shoud never happen - we had to successfully load the asset
        // previously to get it into the tree.
        notify(NFY_WARN, "Could not load asset '%s' to recalculate state.",
               name.toString().c_str());
        return;
      }
      if (!version->NeedComputeState()) return;
      AssetDefs::State stateByInputs;
      AssetDefs::State stateByChildren;
      bool blockersAreOffline;
      uint32 numWaitingFor;
      CalculateStateParameters(vertex, tree, stateByInputs, stateByChildren, blockersAreOffline, numWaitingFor);
      AssetDefs::State newState = 
          version->CalcStateByInputsAndChildren(stateByInputs, stateByChildren, blockersAreOffline, numWaitingFor);
      // Set the state and send notifications.
      updater->SetState(vertex, newState, true);
    }
};

void StateUpdater::RecalculateAndSaveStates() {
  // Traverse the state tree, recalculate states, and update states as needed.
  // State is calculated for each vertex in the tree after state is calculated
  // for all of its child and input vertices.
  // Possible optimization: Many assets have significant overlap in their
  // inputs. It might save time if we could calculate the overlapping state
  // only once.
  depth_first_search(tree, visitor(UpdateStateVisitor(this)));
}

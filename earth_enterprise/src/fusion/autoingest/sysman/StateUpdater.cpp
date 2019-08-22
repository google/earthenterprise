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
  struct property_map<DependentStateTree, vertex_index_t> {
    typedef InNodeVertexIndexMap<DependentStateTree> const_type;
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
        bool decided = false;
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
            // If we enter this case we know what the outputs will be, so we
            // don't need to check any more states.
            decided = true;
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
        bool Decided() {
          return decided;
        }
    };
    // Helper class for calculating state from children
    class ChildStates {
      private:
        bool decided = false;
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
            // If we enter this case we know what the outputs will be, so we
            // don't need to check any more states.
            decided = true;
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
        bool Decided() {
          return decided;
        }
    };

    // Loops through the inputs and children of an asset and calculates
    // everything the asset verion needs to know to figure out its state. This
    // data will be passed to the asset version so it can calculate its own
    // state.
    void CalculateStateParameters(
        DependentStateTreeVertexDescriptor vertex,
        const DependentStateTree & tree,
        AssetDefs::State &stateByInputs,
        AssetDefs::State &stateByChildren,
        bool & blockersAreOffline,
        uint32 & numWaitingFor,
        bool & needRecalcState) const {
      InputStates inputStates;
      ChildStates childStates;

      needRecalcState = tree[vertex].stateChanged;
      auto edgeIters = out_edges(vertex, tree);
      auto edgeBegin = edgeIters.first;
      auto edgeEnd = edgeIters.second;
      for (auto i = edgeBegin; i != edgeEnd; ++i) {
        DependencyType type = tree[*i].type;
        DependentStateTreeVertexDescriptor dep = target(*i, tree);
        AssetDefs::State depState = tree[dep].state;
        switch(type) {
          case INPUT:
            inputStates.Add(depState);
            needRecalcState = needRecalcState || tree[dep].stateChanged;
            break;
          case CHILD:
          case DEPENDENT_AND_CHILD:
            childStates.Add(depState);
            needRecalcState = needRecalcState || tree[dep].stateChanged;
            break;
          case DEPENDENT:
            // Dependents that are not also children are not considered when
            // calculating state.
            break;
        }
        // If we already know what the value of all of the parameters will be
        // we can exit the loop early.
        if (inputStates.Decided() && childStates.Decided() && needRecalcState) {
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
        DependentStateTreeVertexDescriptor vertex,
        const DependentStateTree & tree) const {
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
        bool needRecalcState;
        CalculateStateParameters(
              vertex, tree, stateByInputs, stateByChildren,
              blockersAreOffline, numWaitingFor, needRecalcState);
        if (needRecalcState) {
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
  tree = BuildDependentStateTree(verref, updateStatePredicate, storageManager);
  depth_first_search(tree, visitor(SetStateVisitor(this, newState)));
}

void StateUpdater::SetState(
    DependentStateTreeVertexDescriptor vertex,
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

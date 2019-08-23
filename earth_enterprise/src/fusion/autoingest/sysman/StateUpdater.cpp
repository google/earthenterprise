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
        DependentStateTreeVertexDescriptor vertex,
        const DependentStateTree & tree,
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
        DependencyType type = tree[*i].type;
        DependentStateTreeVertexDescriptor dep = target(*i, tree);
        AssetDefs::State depState = tree[dep].state;
        switch(type) {
          case INPUT:
            inputStates.Add(depState);
            childOrInputStateChanged = childOrInputStateChanged || tree[dep].stateChanged;
            break;
          case CHILD:
          case DEPENDENT_AND_CHILD:
            childStates.Add(depState);
            childOrInputStateChanged = childOrInputStateChanged || tree[dep].stateChanged;
            break;
          case DEPENDENT:
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
        DependentStateTreeVertexDescriptor vertex,
        const DependentStateTree & tree) const {
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
  tree = BuildDependentStateTree(verref, updateStatePredicate, storageManager);
  depth_first_search(tree, visitor(SetStateVisitor(this, newState)));
}

void StateUpdater::SetState(
    DependentStateTreeVertexDescriptor vertex,
    AssetDefs::State newState,
    bool sendNotifications) {
  SharedString name = tree[vertex].name;
  AssetDefs::State oldState = tree[vertex].state;
  if (newState != oldState) {
    notify(NFY_PROGRESS, "Setting state of '%s' from '%s' to '%s'",
           name.toString().c_str(), ToString(oldState).c_str(), ToString(newState).c_str());
    auto version = storageManager->GetMutable(name);
    if (version) {
      do {
        version->state = newState;
        if (sendNotifications) {
          AssetDefs::State nextState = AssetDefs::Failed;
          try {
            // This will take care of stopping any running tasks, etc.
            nextState = version->OnStateChange(newState, oldState);
          } catch (const StateChangeException &e) {
            notify(NFY_WARN, "Exception during %s: %s : %s",
                   e.location.c_str(), name.toString().c_str(), e.what());
            AssetVersionImplD::WriteFatalLogfile(name, e.location, e.what());
          } catch (const std::exception &e) {
            notify(NFY_WARN, "Exception during OnStateChange: %s", e.what());
          } catch (...) {
            notify(NFY_WARN, "Unknown exception during OnStateChange");
          }
          oldState = newState;
          newState = nextState;
        }
      } while(version->state != newState);
      // Get the new state directly from the asset version in case it changed.
      tree[vertex].state = version->state;
      tree[vertex].stateChanged = true;
      if (sendNotifications) {
        notify(NFY_VERBOSE, "Calling theAssetManager.NotifyVersionStateChange(%s, %s)", 
               name.toString().c_str(), 
               ToString(tree[vertex].state).c_str());
        assetManager->NotifyVersionStateChange(name, tree[vertex].state);
      }
    }
    else {
      // This shoud never happen - we had to successfully load the asset
      // previously to get it into the tree.
      notify(NFY_WARN, "Could not load asset '%s' to set state.",
             name.toString().c_str());
    }
  }
}

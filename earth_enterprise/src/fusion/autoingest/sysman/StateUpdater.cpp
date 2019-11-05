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
#include "DependentStateTree.h"

#include <unordered_set>
#include <memory>

// Must be included before depth_first_search.hpp
#include "InNodeVertexIndexMap.h"

#include <boost/graph/depth_first_search.hpp>

using namespace boost;
using namespace std;

class StateUpdater::SetStateVisitor : public default_dfs_visitor {
  private:
    using RecalcSet = std::unordered_set<SharedString>;

    StateUpdater & updater;
    DependentStateTree & tree;
    const AssetDefs::State newState;
    // This is a shared_ptr because the visitor will be copied several times
    // during the course of the traversal.
    const std::shared_ptr<RecalcSet> needsRecalc;

    // Helper class for calculating state from inputs
    class InputStates {
      private:
        bool decided = false;
        uint32 numinputs = 0;
        uint32 numgood = 0;
        uint32 numblocking = 0;
        uint32 numoffline = 0;
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
        void GetOutputs(AssetDefs::State & stateByInputs, bool & blockersAreOffline, uint32 & numInputsWaitingFor) {
          if (numinputs == numgood) {
            stateByInputs = AssetDefs::Queued;
          } else if (numblocking) {
            stateByInputs = AssetDefs::Blocked;
          } else {
            stateByInputs = AssetDefs::Waiting;
          }

          blockersAreOffline = (numblocking == numoffline);

          if (stateByInputs == AssetDefs::Waiting) {
            numInputsWaitingFor = (numinputs - numgood);
          } else {
            numInputsWaitingFor = 0;
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
        uint32 numkids = 0;
        uint32 numgood = 0;
        uint32 numblocking = 0;
        uint32 numinprog = 0;
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
        void GetOutputs(AssetDefs::State & stateByChildren, uint32 & numChildrenWaitingFor) {
          if (numkids == numgood) {
            stateByChildren = AssetDefs::Succeeded;
          } else if (numblocking) {
            stateByChildren = AssetDefs::Blocked;
          } else if (numgood || numinprog) {
            stateByChildren = AssetDefs::InProgress;
          } else {
            stateByChildren = AssetDefs::Queued;
          }
          
          if (stateByChildren == AssetDefs::InProgress) {
            numChildrenWaitingFor = numkids - numgood;
          }
          else {
            numChildrenWaitingFor = 0;
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
    InputAndChildStateData CalculateStateParameters(
        DependentStateTreeVertexDescriptor vertex) const {
      InputStates inputStates;
      ChildStates childStates;

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
            break;
          case CHILD:
          case DEPENDENT_AND_CHILD:
            childStates.Add(depState);
            break;
          case DEPENDENT:
            // Dependents that are not also children are not considered when
            // calculating state.
            break;
        }
        // If we already know what the value of all of the parameters will be
        // we can exit the loop early.
        if (inputStates.Decided() && childStates.Decided()) {
          break;
        }
      }

      InputAndChildStateData data;
      inputStates.GetOutputs(data.stateByInputs, data.blockersAreOffline, data.waitingFor.inputs);
      childStates.GetOutputs(data.stateByChildren, data.waitingFor.children);
      return data;
    }

    void SetState(
        DependentStateTreeVertexDescriptor vertex,
        AssetDefs::State newState,
        const WaitingFor & waitingFor,
        bool sendNotification) const {
      SharedString name = tree[vertex].name;
      AssetDefs::State oldState = tree[vertex].state;
      if (newState != oldState) {
        notify(NFY_PROGRESS, "Setting state of '%s' from '%s' to '%s'",
              name.toString().c_str(), ToString(oldState).c_str(), ToString(newState).c_str());
        auto version = updater.storageManager->GetMutable(name);
        if (version) {
          updater.SetVersionStateAndRunHandlers(version, newState, waitingFor, sendNotification);

          // Get the new state directly from the asset version since it may be
          // different from the passed-in state
          tree[vertex].state = version->state;

          needsRecalc->insert(version->parents.begin(), version->parents.end());
          needsRecalc->insert(version->listeners.begin(), version->listeners.end());
        }
        else {
          // This shoud never happen - we had to successfully load the asset
          // previously to get it into the tree.
          notify(NFY_WARN, "Could not load asset '%s' to set state.",
                name.toString().c_str());
        }
      }
    }

    bool NeedComputeState(DependentStateTreeVertexDescriptor vertex) const {
      auto & vertexData = tree[vertex];
      bool userActionRequired = 
          (vertexData.state & (AssetDefs::Bad |
                               AssetDefs::Offline |
                               AssetDefs::Canceled));
      bool recalcRequested = (needsRecalc->find(vertexData.name) != needsRecalc->end());
      return !userActionRequired && (vertexData.inDepTree || recalcRequested);
    }

    void ComputeAndSetState(
        const SharedString & name,
        DependentStateTreeVertexDescriptor vertex) const {
      const InputAndChildStateData data = CalculateStateParameters(vertex);
      AssetDefs::State calculatedState;
      {
        // Limit scope to release the asset version as quickly as possible
        auto version = updater.storageManager->Get(name);
        if (!version) {
          // This shoud never happen - we had to successfully load the asset
          // previously to get it into the tree.
          notify(NFY_WARN, "Could not load asset '%s' to recalculate state.",
                  name.toString().c_str());
          return;
        }
        calculatedState = version->CalcStateByInputsAndChildren(data);
      }
      // Set the state. "true" means we're done changing this asset's state.
      SetState(vertex, calculatedState, data.waitingFor, true);
    }

  public:
    SetStateVisitor(StateUpdater & updater, DependentStateTree & tree, AssetDefs::State newState) :
        updater(updater), tree(tree), newState(newState),
        needsRecalc(std::make_shared<RecalcSet>()) {}

    // This function is called after the DFS has completed for every vertex
    // below this one in the tree. Thus, we don't calculate the state for an
    // asset until we've calculated the state for all of its inputs and
    // children.
    void finish_vertex(
        DependentStateTreeVertexDescriptor vertex,
        const DependentStateTree &) const {
      SharedString name = tree[vertex].name;
      notify(NFY_PROGRESS, "Calculating state for '%s'", name.toString().c_str());

      // Set the state for assets in the dependent tree. "false" means we'll
      // set the state again below.
      if (tree[vertex].inDepTree) {
        SetState(vertex, newState, {0, 0}, false);
      }

      // For all assets (including parents and listeners) update the state
      // based on the state of inputs and children.
      if (NeedComputeState(vertex)) {
        ComputeAndSetState(name, vertex);
        needsRecalc->erase(name);
      }
    }
};

void StateUpdater::SetStateForRefAndDependents(
    const SharedString & ref,
    AssetDefs::State newState,
    function<bool(AssetDefs::State)> updateStatePredicate) {
  try {
    SharedString verref = AssetVersionImpl::Key(ref);
    DependentStateTree tree = BuildDependentStateTree(verref, updateStatePredicate, storageManager);
    depth_first_search(tree, visitor(SetStateVisitor(*this, tree, newState)));
  }
  catch (UnsupportedException) {
    // This is intended as a temporary condition that will no longer be needed
    // when all operations have been converted to use the state updater for
    // propagating state changes. When this exception is thrown, the legacy
    // code will have already propagated the state change, so no further action
    // is necessary.
    notify(NFY_INFO, "Unsupported condition encountered in state updater. "
           "Reverting to legacy state propagation.");
  }
}

void StateUpdater::SetVersionStateAndRunHandlers(
    AssetHandle<AssetVersionImpl> & version,
    AssetDefs::State newState,
    const WaitingFor & waitingFor,
    bool sendNotification) {
  // RunStateChangeHandlers can return a new state that we need to transition
  // to, so we may have to change the state repeatedly.
  AssetDefs::State oldState = version->state;
  do {
    version->state = newState;
    AssetDefs::State nextState = RunStateChangeHandlers(version, newState, oldState, waitingFor);
    oldState = newState;
    newState = nextState;
  } while(version->state != newState);

  if (sendNotification) {
    SendStateChangeNotification(version->GetRef(), version->state);
  }
}

AssetDefs::State StateUpdater::RunStateChangeHandlers(
    AssetHandle<AssetVersionImpl> & version,
    AssetDefs::State newState,
    AssetDefs::State oldState,
    const WaitingFor & waitingFor) {
  UpdateWaitingAssets(version, oldState, waitingFor);
  try {
    return RunVersionStateChangeHandler(version, newState, oldState);
  } catch (const UnsupportedException &) {
    // We'll catch this exception farther up the stack
    throw;
  } catch (const StateChangeException &e) {
    notify(NFY_WARN, "Exception during %s: %s : %s",
           e.location.c_str(), version->GetRef().toString().c_str(), e.what());
    version->WriteFatalLogfile(e.location, e.what());
  } catch (const std::exception &e) {
    notify(NFY_WARN, "Exception during OnStateChange: %s", e.what());
  } catch (...) {
    notify(NFY_WARN, "Unknown exception during OnStateChange");
  }
  return AssetDefs::Failed;
}

AssetDefs::State StateUpdater::RunVersionStateChangeHandler(
    AssetHandle<AssetVersionImpl> & version,
    AssetDefs::State newState,
    AssetDefs::State oldState) {
  bool hasChildrenBefore = IsParent(version);
  // This will take care of stopping any running tasks, etc.
  AssetDefs::State nextState = version->OnStateChange(newState, oldState);
  bool hasChildrenAfter = IsParent(version);
  if (!hasChildrenBefore && hasChildrenAfter) {
    // OnStateChange can call DelayedBuildChildren, which creates new
    // children for this asset. This code is not yet able to handle that
    // case, so we let OnStateChange perform the legacy state propagation
    // and abandon this operation.
    throw UnsupportedException();
  }
  return nextState;
}

void StateUpdater::SendStateChangeNotification(
    const SharedString & name,
    AssetDefs::State state) const {
  notify(NFY_VERBOSE, "Calling theAssetManager.NotifyVersionStateChange(%s, %s)", 
         name.toString().c_str(), 
         ToString(state).c_str());
  assetManager->NotifyVersionStateChange(name, state);
}

void StateUpdater::UpdateWaitingAssets(
    AssetHandle<const AssetVersionImpl> version,
    AssetDefs::State oldState,
    const WaitingFor & waitingFor) {
  const SharedString ref = version->GetRef();
  const AssetDefs::State newState = version->state;
  waitingListeners.Update(ref, newState, oldState, waitingFor.inputs);
  if (IsParent(version)) {
    inProgressParents.Update(ref, newState, oldState, waitingFor.children);
  }
}

void StateUpdater::SetInProgress(AssetHandle<AssetVersionImpl> & version) {
  try {
    SetVersionStateAndRunHandlers(version, AssetDefs::InProgress);
    PropagateInProgress(version);
    if (!AssetDefs::Finished(version->state)) {
      assetManager->NotifyVersionProgress(version->GetRef(), version->progress);
    }
  }
  catch (UnsupportedException) {
    // This is intended as a temporary condition that will no longer be needed
    // when all operations have been converted to use the state updater for
    // propagating state changes. When this exception is thrown, the legacy
    // code will have already propagated the state change, so no further action
    // is necessary.
    notify(NFY_INFO, "Unsupported condition encountered while setting %s to "
           "InProgress in state updater. Reverting to legacy state propagation.",
           version->GetRef().toString().c_str());
  }
}

void StateUpdater::PropagateInProgress(const AssetHandle<AssetVersionImpl> & version) {
  NotifyChildOrInputInProgress(inProgressParents, version->parents);
  NotifyChildOrInputInProgress(waitingListeners, version->listeners);
}

void StateUpdater::NotifyChildOrInputInProgress(
    const WaitingAssets & waitingAssets,
    const std::vector<SharedString> & toNotify) {
  // If the asset to notify is already waiting, there's nothing to do.
  // Otherwise, we need to recalculate (and possibly propagate) the state.
  for (const SharedString & ref : toNotify) {
    if (!waitingAssets.IsWaiting(ref)) {
      RecalcState(ref);
    }
  }
}

void StateUpdater::SetSucceeded(AssetHandle<AssetVersionImpl> & version) {
  try {
    SetVersionStateAndRunHandlers(version, AssetDefs::Succeeded);
    UpdateSucceeded(version, true);
  }
  catch (UnsupportedException) {
    // This is intended as a temporary condition that will no longer be needed
    // when all operations have been converted to use the state updater for
    // propagating state changes. When this exception is thrown, the legacy
    // code will have already propagated the state change, so no further action
    // is necessary.
    notify(NFY_INFO, "Unsupported condition encountered while setting %s to "
           "Succeeded in state updater. Reverting to legacy state propagation.",
           version->GetRef().toString().c_str());
  }
}

void StateUpdater::UpdateSucceeded(AssetHandle<const AssetVersionImpl> version, bool propagate) {
  NotifyChildOrInputSucceeded(inProgressParents, version->parents, propagate);
  NotifyChildOrInputSucceeded(waitingListeners, version->listeners, propagate);
}

void StateUpdater::NotifyChildOrInputSucceeded(
    WaitingAssets & waitingAssets,
    const std::vector<SharedString> & toNotify,
    bool propagate) {
  // If the asset is no longer waiting, or if it wasn't waiting to begin with,
  // we need to recalculate and propagate the state.
  for (const SharedString & ref : toNotify) {
    if (!waitingAssets.DecrementAndCheckWaiting(ref) && propagate) {
      RecalcState(ref);
    }
  }
}

void StateUpdater::SetFailed(AssetHandle<AssetVersionImpl> & version) {
  version->SetAndPropagateState(AssetDefs::Failed);
}

void StateUpdater::RecalcState(const SharedString & ref) {
  auto version = storageManager->Get(ref);
  WaitingFor waitingFor;
  if (!version->RecalcState(waitingFor)) {
    // If the state didn't change, we may still need to add the asset to the
    // waiting list. For example, Fusion could have restarted since the asset
    // transitioned to the Waiting state.
    UpdateWaitingAssets(version, AssetDefs::New, waitingFor);
  }
}

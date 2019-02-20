// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <notify.h>
#include <khException.h>
#include <AssetThrowPolicy.h>
#include <khFileUtils.h>
#include <khstl.h>
#include <algorithm>
#include "AssetVersionD.h"
#include "AssetD.h"
#include "Asset.h"
#include "Misc.h"
#include "khAssetManager.h"
#include "fusion/autoingest/MiscConfig.h"

// ****************************************************************************
// ***  StateUpdateNotifier
// ****************************************************************************
std::shared_ptr<AssetVersionImplD::StateChangeNotifier>
AssetVersionImplD::StateChangeNotifier::GetNotifier(
    std::shared_ptr<StateChangeNotifier> callerNotifier) {
  if (callerNotifier) {
    // If the caller passed in a notifier there's no need to create a new one.
    // Just use the caller's notifier. This allows us to consolidate duplicate
    // notifications across several function calls.
    return callerNotifier;
  }
  else {
    return std::make_shared<AssetVersionImplD::StateChangeNotifier>();
  }
}

void
AssetVersionImplD::StateChangeNotifier::AddParentsToNotify(const std::vector<std::string> & parents) {
  std::copy(parents.begin(), parents.end(), std::inserter(parentsToNotify, parentsToNotify.end()));
}

void
AssetVersionImplD::StateChangeNotifier::AddListenersToNotify(const std::vector<std::string> & listeners, AssetDefs::State inputState) {
  for (const std::string & listener : listeners) {
    // This ensures that the listener is in the list of listeners to notify
    InputStates & elem = listenersToNotify[listener];
    if (inputState == AssetDefs::Succeeded) {
      ++elem.numSucceeded;
    }
    else if (!AssetDefs::Working(inputState)) {
      elem.allWorkingOrSucceeded = false;
    }
  }
}

AssetVersionImplD::StateChangeNotifier::~StateChangeNotifier() {
  if (parentsToNotify.size() > 0 || listenersToNotify.size() > 0) {
    // Notify my parents and listeners. Pass them a new notifier in case my
    // parents or listeners also need to notify their parents or listeners. I
    // can't use myself as their notifier because if there are any duplicates
    // between what I've notified and what they're going to notify, those
    // duplicates need to be notified again since a relevant state might change
    // as part of this operation.
    std::shared_ptr<StateChangeNotifier> notifier = GetNotifier(nullptr);
    NotifyParents(notifier);
    NotifyListeners(notifier);
  }
}

void
AssetVersionImplD::StateChangeNotifier::NotifyParents(
    std::shared_ptr<StateChangeNotifier> notifier) {
  notify(NFY_VERBOSE, "Iterate through parents");
  int i = 1;
  for (const std::string & ref : parentsToNotify) {
    AssetVersionD assetVersion(ref);
    notify(NFY_PROGRESS, "Iteration: %d | Total Iterations: %s | parent: %s",
           i,
           ToString(parentsToNotify.size()).c_str(),
           ref.c_str());
    if (assetVersion) {
      notify(NFY_VERBOSE, "Calling parent->HandleChildStateChange()");
      assetVersion->HandleChildStateChange(notifier);
    } else {
      notify(NFY_WARN, "'%s' has broken parent '%s'",
             assetVersion->GetRef().c_str(), ref.c_str());
    }
    i++;
  }
  parentsToNotify.clear();
}

void
AssetVersionImplD::StateChangeNotifier::NotifyListeners(
    std::shared_ptr<StateChangeNotifier> notifier) {
  notify(NFY_VERBOSE, "Iterate through listeners");
  int i = 1;
  for (const std::pair<std::string, InputStates> & elem : listenersToNotify) {
    const std::string & ref = elem.first;
    const InputStates & states = elem.second;
    AssetVersionD assetVersion(ref);
    notify(NFY_PROGRESS, "Iteration: %d | Total Iterations: %s | listener: %s",
           i,
           ToString(listenersToNotify.size()).c_str(),
           ref.c_str());
    if (assetVersion) {
      notify(NFY_VERBOSE, "Calling listener->HandleInputStateChange(). Num Succeeded: %zu, All Working or Succeeded: %s",
            states.numSucceeded,
            states.allWorkingOrSucceeded ? "true" : "false");
      assetVersion->HandleInputStateChange(states, notifier);
    } else {
      notify(NFY_WARN, "'%s' has broken listener '%s'",
             assetVersion->GetRef().c_str(), ref.c_str());
    }
    i++;
  }
  listenersToNotify.clear();
}

// ****************************************************************************
// ***  MutableAssetVersionD
// ****************************************************************************
template <>
MutableAssetVersionD::DirtyMap MutableAssetVersionD::dirtyMap = MutableAssetVersionD::DirtyMap();


// ****************************************************************************
// ***  AssetVersionImplD
// ****************************************************************************
khRefGuard<AssetVersionImplD>
AssetVersionImplD::Load(const std::string &boundref)
{
  khRefGuard<AssetVersionImplD> result;

  // make sure the base class loader actually instantiated one of me
  // this should always happen, but there are no compile time guarantees
  result.dyncastassign(AssetVersionImpl::Load(boundref));
  if (!result) {
    AssetThrowPolicy::FatalOrThrow(
        kh::tr("Internal error: AssetVersionImplD loaded wrong type for ") +
        boundref);
  }

  return result;
}

// since AssetVersionImpl is a virtual base class
// my derived classes will initialize it directly
AssetVersionImplD::AssetVersionImplD(const std::vector<std::string> &inputs)
    : AssetVersionImpl(), verholder(0)
{
  AddInputAssetRefs(inputs);
}


void
AssetVersionImplD::AddInputAssetRefs(const std::vector<std::string> &inputs_)
{
  for (std::vector<std::string>::const_iterator i = inputs_.begin();
       i != inputs_.end(); ++i) {
    // Add myself to the input's list of listeners.
    MutableAssetVersionD input(*i);

    // NOTE: we don't need to put listeners to asset versions of any type
    // which state is 'Succeeded'. A state is 'Succeeded', so the state
    // change event will never be triggered for this asset version -
    // the propagation mechanism will not be applied, listeners are not needed.

    // Skip adding of PacketLevelAsset versions as listeners to a
    // CombinedRPAsset version. It is guaranteed by the fusing pipeline that
    // CombinedRPAsset versions of all input insets of a raster project are
    // 'Succeeded' when reaching this point for a PacketLevelAsset version.
    // NOTE:
    // - A CombinedRPAsset version as an input is only applicable for
    // PacketLevelAsset versions.
    // - A CombinedRPAsset version is always 'Succeeded' when reaching this
    // point for PacketLevelAsset version.
    // Check these statements with asserts below, and do not add
    // PacketLevelAsset versions as listeners to CombinedRPAsset version.
    if (input->subtype == kCombinedRPSubtype) {
      assert(subtype == kPacketLevelSubtype);
      assert(input->state == AssetDefs::Succeeded);
      // NOTE: when above asserts are true, it is safe to clear
      // the listeners of CombinedRPAsset version. Allows to clean up
      // the CombinedRPAsset versions of the raster resources built before
      // GEE-5.1.2.
      input->listeners.clear();
    } else {
      input->listeners.push_back(GetRef());
    }

    // Now add the input to my list of inputs.
    // The MutableAssetVersion bound the versionref for me,
    // so use input->GetRef() instead of *i.
    inputs.push_back(input->GetRef());
  }
}

AssetDefs::State
AssetVersionImplD::StateByInputs(bool *blockersAreOffline,
                                 uint32 *numWaiting) const
{
  // load my input versions (only if they aren't already loaded)
  InputVersionGuard guard(this);


  // find out how my inputs are doing
  uint numinputs = inputs.size();
  uint numgood = 0;
  uint numblocking = 0;
  uint numoffline = 0;
  for (std::vector<AssetVersion>::const_iterator i =
         guard->inputvers.begin();
       i != guard->inputvers.end(); ++i) {
    const AssetVersion &input(*i);
    if (input) {
      AssetDefs::State istate = input->state;
      if (istate == AssetDefs::Succeeded) {
        ++numgood;
      } else if (istate == AssetDefs::Offline) {
        ++numblocking;
        ++numoffline;
      } else if (istate & (AssetDefs::Blocked |
                           AssetDefs::Failed |
                           AssetDefs::Canceled |
                           AssetDefs::Bad)) {
        ++numblocking;
        // At this point we already know what the values of all of the outputs
        // will be (statebyinputs will be Blocked, blockersAreOffline will
        // be false since numblockers will be different from numoffline, and
        // numWaiting will be 0 because statebyinputs is not Waiting).
        break;
      }
    } else {
      notify(NFY_WARN, "StateByInputs: %s missing input %s",
             GetRef().c_str(), input->GetRef().c_str());
      ++numblocking;
      // At this point we already know what the values of all of the outputs
      // will be (statebyinputs will be Blocked, blockersAreOffline will
      // be false since numblockers will be different from numoffline, and
      // numWaiting will be 0 because statebyinputs is not Waiting).
      break;
    }
  }


  // figure out what my state would be based only on my inputs
  AssetDefs::State statebyinputs;
  if (numinputs == numgood) {
    statebyinputs = AssetDefs::Queued;
  } else if (numblocking) {
    statebyinputs = AssetDefs::Blocked;
  } else {
    statebyinputs = AssetDefs::Waiting;
  }

  if (blockersAreOffline)
    *blockersAreOffline = (numblocking == numoffline);

  if (numWaiting) {
    if (statebyinputs == AssetDefs::Waiting) {
      *numWaiting = (numinputs - numgood);
    } else {
      *numWaiting = 0;
    }
  }
  return statebyinputs;
}

template <bool propagate>
void AssetVersionImplD::SetState(
    AssetDefs::State newstate,
    const std::shared_ptr<StateChangeNotifier> notifier)
{
  if (newstate != state) {
    notify(NFY_DEBUG, "SetState: current state: %s | newstate: %s | asset: %s",
    	   ToString(state).c_str(),
           ToString(newstate).c_str(),
           GetRef().c_str());
    AssetDefs::State oldstate = state;
    state = newstate;
    try {
      // NOTE: This can end up calling back here to switch us to
      // another state (usually Failed or Succeded)
      OnStateChange(newstate, oldstate);
    } catch (const std::exception &e) {
      notify(NFY_WARN, "Exception during OnStateChange: %s", 
             e.what());
    } catch (...) {
      notify(NFY_WARN, "Unknown exception during OnStateChange");
    }

    // only notify and propagate changes if the state is still what we
    // set it to above. OnStateChange can call SetState recursively. We
    // don't want to notify/propagate an old state.
    if (propagate && (state == newstate)) {
      notify(NFY_VERBOSE, "Calling theAssetManager.NotifyVersionStateChange(%s, %s)", 
             GetRef().c_str(), 
             ToString(newstate).c_str());
      theAssetManager.NotifyVersionStateChange(GetRef(), newstate);
      PropagateStateChange(notifier);
    }
  }
}

void
AssetVersionImplD::SetProgress(double newprogress)
{
  progress = newprogress;
  if (!AssetDefs::Finished(state)) {
    theAssetManager.NotifyVersionProgress(GetRef(), progress);
    PropagateProgress();
  }
}

void
AssetVersionImplD::SyncState(const std::shared_ptr<StateChangeNotifier> notifier) const
{
  if (CacheInputVersions()) {
    // load my input versions (only if they aren't already loaded)
    // I do this here because ComputeState and SetState
    // may both need to access them, this way they only get looked up once
    InputVersionGuard guard(this);

    AssetDefs::State newstate = ComputeState();
    if (newstate != state) {
      MutableAssetVersionD self(GetRef());
      self->SetState(newstate, notifier);
    }
  } else {
    AssetDefs::State newstate = ComputeState();
    if (newstate != state) {
      MutableAssetVersionD self(GetRef());
      self->SetState(newstate, notifier);
    }
  }
}

void
AssetVersionImplD::PropagateStateChange(const std::shared_ptr<StateChangeNotifier> callerNotifier)
{
  notify(NFY_PROGRESS, "PropagateStateChange(%s): %s",
         ToString(state).c_str(), 
         GetRef().c_str());
  std::shared_ptr<StateChangeNotifier> notifier = StateChangeNotifier::GetNotifier(callerNotifier);
  notifier->AddParentsToNotify(parents);
  notifier->AddListenersToNotify(listeners, state);
}

void
AssetVersionImplD::PropagateProgress(void)
{
  notify(NFY_VERBOSE, "PropagateProgress(%s): %s",
         ToString(progress).c_str(), GetRef().c_str());
  for (std::vector<std::string>::const_iterator p = parents.begin();
       p != parents.end(); ++p) {
    AssetVersionD parent(*p);
    if (parent) {
      parent->HandleChildProgress(GetRef());
    } else {
      notify(NFY_WARN, "'%s' has broken parent '%s'",
             GetRef().c_str(), p->c_str());
    }
  }
}

void
AssetVersionImplD::HandleTaskLost(const TaskLostMsg &)
{
  // NoOp in base since composites don't need to do anything
}

void
AssetVersionImplD::HandleTaskProgress(const TaskProgressMsg &)
{
  // NoOp in base since composites don't need to do anything
}

void
AssetVersionImplD::HandleTaskDone(const TaskDoneMsg &)
{
  // NoOp in base since composites don't need to do anything
}

void
AssetVersionImplD::HandleChildStateChange(const std::shared_ptr<StateChangeNotifier>) const
{
  // NoOp in base since leaves don't need to do anything
  notify(NFY_VERBOSE, "AssetVersionImplD::HandleChildStateChange: %s", GetRef().c_str());
}

void
AssetVersionImplD::HandleChildProgress(const std::string &) const
{
  // NoOp in base since leaves don't do anything
}

void
AssetVersionImplD::OnStateChange(AssetDefs::State newstate,
                                 AssetDefs::State oldstate)
{
  // NoOp in base class
  if (newstate == AssetDefs::Succeeded) {
    notify(NFY_VERBOSE, "newstate: %s", 
           ToString(newstate).c_str());
#ifdef TEMP_ASSETS
    notify(NFY_VERBOSE, "TEMP_ASSETS has been defined");
    if (haveTemporaryInputs) {
      notify(NFY_VERBOSE, "haveTemporaryInputs: %s", 
             haveTemporaryInputs.c_str());
      if (OfflineInputsBreakMe()) {
        throw khException(kh:tr("Internal Error: AssetVersion with temporary "
                                "inputs but marked as OfflineInputsBreakMe"));
      }
      // traverse inputs (use Inputs guard?)
      // clean those that are temps
    }
#endif
  }
}


void
AssetVersionImplD::SetBad(void)
{
  if (state == AssetDefs::Succeeded) {
    SetState(AssetDefs::Bad);
  } else if (!AssetDefs::Finished(state)) {
    throw khException(kh::tr("%1 not finished. Use 'cancel' instead of 'setbad'")
                      .arg(ToQString(GetRef())));
  } else {
    throw khException(kh::tr("%1 already %2. Unable to mark as bad")
                      .arg(ToQString(GetRef()), ToQString(state)));
  }
}

void
AssetVersionImplD::ClearBad(void)
{
  if (state == AssetDefs::Bad) {
    SetState(AssetDefs::Succeeded);
  } else {
    throw khException(kh::tr("%1 not 'bad'. Unable to clear bad.")
                      .arg(ToQString(GetRef())));
  }
}


bool
AssetVersionImplD::OkToClean(std::vector<std::string> *wouldbreak) const
{
  // --- check that it's ok to clean me ---
  // If I have a successful parent, then my cleaning would break him
  for (std::vector<std::string>::const_iterator p = parents.begin();
       p != parents.end(); ++p) {
    AssetVersionD parent(*p);
    if (parent) {
      if ((parent->state != AssetDefs::Offline) &&
          (parent->state != AssetDefs::Bad)) {
        if (wouldbreak) {
          wouldbreak->push_back(*p);
        } else {
          return false;
        }
      }
    } else {
      notify(NFY_WARN, "'%s' has broken parent '%s'",
             GetRef().c_str(), p->c_str());
    }
  }

  // If I have succesfull listeners that depend on me, then my cleaning woul
  // break them
  for (std::vector<std::string>::const_iterator l = listeners.begin();
       l != listeners.end(); ++l) {
    AssetVersionD listener(*l);
    if (listener) {
      if (((listener->state != AssetDefs::Offline) &&
           (listener->state != AssetDefs::Bad)) &&
          listener->OfflineInputsBreakMe()) {
        if (wouldbreak) {
          wouldbreak->push_back(*l);
        } else {
          return false;
        }
      }
    } else {
      notify(NFY_WARN, "'%s' has broken listener '%s'",
             GetRef().c_str(), l->c_str());
    }
  }

  return wouldbreak ? wouldbreak->empty() : true;
}

bool
AssetVersionImplD::OkToCleanAsInput(void) const
{
  // already offline
  if (state == AssetDefs::Offline) {
    return false;
  }

#ifdef TEMP_ASSETS
  if (isTemporary) {
    return true;
  }
#endif

  // when we're digging down to clean as much as we can,
  // don't clean the most recent version of user-visible assets
  AssetVersionRef verref(GetRef());
  AssetD inputAsset(verref.AssetRef());
  if (inputAsset->CurrVersionRef() == GetRef()) {
    if ((subtype == kProductSubtype) ||
        (subtype == kMercatorProductSubtype) ||
        (subtype == kLayer) ||
        (subtype == kProjectSubtype) ||
        (subtype == kMercatorProjectSubtype) ||
        (subtype == kDatabaseSubtype)) {
      return false;
    }
  }

  // see if Cleaning this one would break anybody else
  return OkToClean();
}

void
AssetVersionImplD::Clean(void)
{
  if (!AssetDefs::Finished(state) &&
      (state != AssetDefs::Blocked)) {
    throw khException(kh::tr("%1 not finished.\nUse 'cancel' first.")
                      .arg(ToQString(GetRef())));
  } else if (state == AssetDefs::Offline) {
    throw khException(kh::tr("%1 already offline.")
                      .arg(ToQString(GetRef())));
  }

  if (subtype == "Source") {
    throw khException(kh::tr("%1 is a source asset version.\nRefusing to clean.")
                      .arg(ToQString(GetRef())));
  }

  // Check to see if it's OK to clean me
  std::vector<std::string> wouldbreak;
  if (!OkToClean(&wouldbreak)) {
    throw khException
      (kh::tr("Unable to clean '%1'.\nIt would break the following:\n")
       .arg(GetRef()) +
       join<std::vector<std::string>::iterator>(wouldbreak.begin(), wouldbreak.end(), "\n"));
  }

  DoClean();
}


AssetVersionImplD::InputVersionHolder::InputVersionHolder
(const std::vector<std::string> &inputrefs)
{
  inputvers.reserve(inputrefs.size());
  for (std::vector<std::string>::const_iterator i = inputrefs.begin();
       i != inputrefs.end(); ++i) {
    inputvers.push_back(*i);
  }
}

AssetVersionImplD::InputVersionHolder::InputVersionHolder
(const std::vector<AssetVersion> &inputvers_) : inputvers(inputvers_)
{
}


AssetVersionImplD::InputVersionGuard::InputVersionGuard
(const AssetVersionD &ver) :
    khRefGuard<InputVersionHolder>(khRefGuardFromThis_(ver->verholder)),
    impl(ver.operator->())
{
  if (!operator bool()) {
    impl->verholder = new InputVersionHolder(impl->inputs);
    khRefGuard<InputVersionHolder>::operator=(khRefGuardFromNew(impl->verholder));
  }
}

AssetVersionImplD::InputVersionGuard::InputVersionGuard
(const AssetVersionImplD *impl_) :
    khRefGuard<InputVersionHolder>(khRefGuardFromThis_(impl_->verholder)),
    impl(impl_)
{
  if (!operator bool()) {
    impl->verholder = new InputVersionHolder(impl->inputs);
    khRefGuard<InputVersionHolder>::operator=(khRefGuardFromNew(impl->verholder));
  }
}

AssetVersionImplD::InputVersionGuard::InputVersionGuard
(const AssetVersionImplD *impl_,
 const std::vector<AssetVersion> &inputvers) :
    khRefGuard<InputVersionHolder>(khRefGuardFromThis_(impl_->verholder)),
    impl(impl_)

{
  if (!operator bool()) {
    if (inputvers.size()) {
      impl->verholder = new InputVersionHolder(inputvers);
    } else {
      impl->verholder = new InputVersionHolder(impl->inputs);
    }
    khRefGuard<InputVersionHolder>::operator=(khRefGuardFromNew(impl->verholder));
  }
}


AssetVersionImplD::InputVersionGuard::~InputVersionGuard(void)
{
  if (refcount() == 1) {
    impl->verholder = 0;
    // my base destructor will delete the object for me
  }
}

void
AssetVersionImplD::GetInputFilenames(std::vector<std::string> &out) const
{
  // load my input versions (only if they aren't already loaded)
  InputVersionGuard guard(this);

  for (std::vector<AssetVersion>::const_iterator iver =
         guard->inputvers.begin();
       iver != guard->inputvers.end(); ++iver) {
    (*iver)->GetOutputFilenames(out);
  }
}

void
AssetVersionImplD::WriteFatalLogfile(const AssetVersionRef &verref,
                                     const std::string &prefix,
                                     const std::string &error) throw()
{
  std::string logfilename = AssetVersionImpl::LogFilename(verref);
  khEnsureParentDir(logfilename);
  FILE *logfile = fopen(logfilename.c_str(), "w");
  if (logfile) {
    notify(NFY_WARN, "FATAL ERROR: %s, logging to file: %s",
           prefix.c_str(), logfilename.c_str());

    fprintf(logfile, "FATAL ERROR: %s: %s\n",
            prefix.c_str(), error.c_str());
    fclose(logfile);
  } else {
    notify(NFY_WARN, "Unable to log failure: %s: %s: %s",
           std::string(verref).c_str(), prefix.c_str(), error.c_str());
  }
}


// ****************************************************************************
// ***  LeafAssetVersionImplD
// ****************************************************************************
void
LeafAssetVersionImplD::HandleInputStateChange(InputStates newStates,
                                              const std::shared_ptr<StateChangeNotifier> notifier) const
{
  notify(NFY_VERBOSE, "HandleInputStateChange: %s", GetRef().c_str());
  // If I'm waiting on my inputs to complete there's no need to do a full sync.
  // I just reduce the number that I'm waiting for by the number that have
  // succeeded. However, if any of my assets have fallen out of a working state
  // without succeeding I still need to sync my state so that I can respond
  // to the error encountered by my input.
  if (state == AssetDefs::Waiting &&
      newStates.allWorkingOrSucceeded &&
      numWaitingFor > newStates.numSucceeded) {
    // In this case it is safe to ignore inputs that are working because I'm
    // already waiting and even if another child regresses to become Working,
    // I'll still stay Waiting. My numWaitingFor will be too low, but that
    // won't hurt, I'll just call SyncState when I don't have to.
    numWaitingFor -= newStates.numSucceeded;
  }
  else {
    SyncState(notifier);
  }
}

bool
LeafAssetVersionImplD::CacheInputVersions(void) const
{
  return NeedComputeState();
}

AssetDefs::State
LeafAssetVersionImplD::ComputeState(void) const
{
  if (!NeedComputeState()) {
    return state;
  }

  // will be !Ready() until all my inputs are good
  bool blockersAreOffline = false;
  AssetDefs::State statebyinputs =
    StateByInputs(&blockersAreOffline, &numWaitingFor);

  AssetDefs::State newstate = state;
  if (!AssetDefs::Ready(state)) {
    // I'm currently not ready, so take whatever my inputs say
    newstate = statebyinputs;
  } else if (statebyinputs != AssetDefs::Queued) {
    // My imputs have regressed
    // Let's see if I should regress too

    if (AssetDefs::Working(state)) {
      // I'm in the middle of building myself
      // revert my state to wait/block on my inputs
      // OnStateChange will pick up this revert and stop my running task
      newstate = statebyinputs;
    } else {
      // my task has already finished
      if ((statebyinputs == AssetDefs::Blocked) && blockersAreOffline) {
        // If the only reason my inputs have reverted is because
        // some of them have gone offline, that's usually OK and
        // I don't need to revert my state.
        // Check to see if I care about my inputs going offline
        if (OfflineInputsBreakMe()) {
          // I care, revert my state too.
          newstate = statebyinputs;
        } else {
          // I don't care, so leave my state alone.
          newstate = state;
        }
      } else {
        // My inputs have regresseed for some reason other than some
        // of them going offline.
        // revert my state
        newstate = statebyinputs;
      }
    }
  } else {
    // nothing to do
    // my current state is correct based on what my task has told me so far
  }

  return newstate;
}

void
LeafAssetVersionImplD::HandleTaskLost(const TaskLostMsg &msg)
{
  // go ahead and clear taskid ourself, we know it's gone (that's the
  // purpose of this message) By us clearing it, OnStateChange won't try
  // to send a DeleteTask message.
  taskid = 0;


  if (state == AssetDefs::Queued) {
    // We're still in the queued state (no progress messages yet).
    // Lie and say we're currently 'New' so SetSate below won't
    // short-ciruit and say "I'm already queued"

    // Just set the field rather that calling SetState(New).
    // We don't want to do any extra work since we're going to turn right
    // around and say SetState(Queued)
    state = AssetDefs::New;
  }

  SetState(AssetDefs::Queued);
}

void
LeafAssetVersionImplD::HandleTaskProgress(const TaskProgressMsg &msg)
{
  // We don't have to worry about old progress messages messing us up
  // the caller makes sure that the message is from the task that
  // we want

  beginTime = msg.beginTime;
  progressTime = msg.progressTime;
  SetState(AssetDefs::InProgress);
  SetProgress(msg.progress);
}

void
LeafAssetVersionImplD::HandleTaskDone(const TaskDoneMsg &msg)
{
  beginTime = msg.beginTime;
  progressTime = msg.endTime;
  endTime = msg.endTime;

  if (msg.success) {
    // the output files in the msg have full paths, make them relative to
    // the AssetRoot again
    ClearOutfiles();                // should already be empty
    for (std::vector<std::string>::const_iterator of
           = msg.outfiles.begin();
         of != msg.outfiles.end(); ++of) {
      outfiles.push_back(*of);
    }
    SetState(AssetDefs::Succeeded);
  } else {
    SetState(AssetDefs::Failed);
  }
}

void
LeafAssetVersionImplD::SubmitTask(void)
{
  try {
    DoSubmitTask();
  } catch (const std::exception &e) {
    notify(NFY_WARN, "Exception during SubmitTask: %s : %s",
           GetRef().c_str(), e.what());
    WriteFatalLogfile(GetRef(), "SubmitTask", e.what());
    SetState(AssetDefs::Failed);
  } catch (...) {
    notify(NFY_WARN, "Unknown exception during SubmitTask: %s",
           GetRef().c_str());
    WriteFatalLogfile(GetRef(), "SubmitTask", "Unknown error");
    SetState(AssetDefs::Failed);
  }
}

void
LeafAssetVersionImplD::ClearOutfiles(void)
{
  for (std::vector<std::string>::const_iterator o = outfiles.begin();
       o != outfiles.end(); ++o) {
    if (khIsURI(*o)) {
      // don't do anything for URI outfiles
    } else {
      // version 2.0 specified the outfiles relative to the assetroot
      // version 2.1 specifies them as volume netpath's (abs paths)
      // Calling AssetPathToFilename is needed to 2.0 and won't
      // hurt for 2.1
      std::string filename = AssetDefs::AssetPathToFilename(*o);

      // get list of all files to delete (incl. piggybacks & overflows)
      std::vector<std::string> todelete;
      todelete.push_back(filename);
      khGetOverflowFilenames(filename, todelete);

      for (std::vector<std::string>::const_iterator d = todelete.begin();
           d != todelete.end(); ++d) {
        if (khExists(*d)) {
          // add to list of files to delete
          theAssetManager.DeleteFile(*d);
        }
      }
    }
  }

  outfiles.clear();
}


void
LeafAssetVersionImplD::OnStateChange(AssetDefs::State newstate,
                                     AssetDefs::State oldstate)
{
  AssetVersionImplD::OnStateChange(newstate, oldstate);

  // task related members that need to be maintained
  // - taskid (may need to SubmitTask or DeleteTask)
  // - output files
  // - progress
  // - beginTime
  // - progressTime
  // - endTime

  switch (state) {
    case AssetDefs::New:
    case AssetDefs::Waiting:
    case AssetDefs::Blocked:
    case AssetDefs::Queued:
    case AssetDefs::Canceled:
    case AssetDefs::Offline:
      // everything should be cleared out and any running tasks deleted
      ClearOutfiles();
      progress = 0.0;
      progressTime = 0;
      beginTime = 0;
      endTime = 0;
      if (taskid) {
        theAssetManager.DeleteTask(GetRef());
        taskid = 0;
      }

      // When we first enter the Queued state, we kick off our task
      if (state == AssetDefs::Queued) {
        // NOTE: This can end up calling back here to switch us to
        // another state (usually Failed or Succeded)
        SubmitTask();
      }
      break;
    case AssetDefs::InProgress:
      // Nothing to do here ...
      break;
    case AssetDefs::Failed:
      ClearOutfiles();
      progress = 1.0;
      taskid = 0;
      // leave beginTime alone
      // leave progressTime alone
      // leave endTime alone
      break;
    case AssetDefs::Succeeded:
      progress = 1.0;
      taskid = 0;
      // leave output files alone
      // leave beginTime alone
      // leave progressTime alone
      // leave endTime alone
      break;
    case AssetDefs::Bad:
      // This had to come from Succeeded and it can go back
      // to Succeeded. So don't muck with anything.
      break;
  }
}


void
LeafAssetVersionImplD::Rebuild(const std::shared_ptr<StateChangeNotifier> callerNotifier)
{
  if (!CanRebuild()) {
    throw khException
      (kh::tr("Resume only allowed for versions that are %1 or %2.")
       .arg(ToQString(AssetDefs::Failed))
       .arg(ToQString(AssetDefs::Canceled)));
  }

#if 0
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
  if (state & (AssetDefs::Succeeded | AssetDefs::Offline | AssetDefs::Bad)) {
    throw khException(kh::tr("%1 marked as %2. Refusing to resume.")
                      .arg(ToQString(GetRef()), ToQString(state)));
  }
#endif

  std::shared_ptr<StateChangeNotifier> notifier = StateChangeNotifier::GetNotifier(callerNotifier);

  // SetState to New. The OnStateChange handler will take care
  // of stopping any running tasks, etc
  // false -> don't propagate the new state (we're going to change
  // it right away by calling SyncState)
  SetState<false>(AssetDefs::New, notifier);

  // Now get my state back to where it should be
  SyncState(notifier);
}


void
LeafAssetVersionImplD::Cancel(const std::shared_ptr<StateChangeNotifier> callerNotifier)
{
  if (!CanCancel()) {
    throw khException(kh::tr("%1 already %2. Unable to cancel.")
                      .arg(ToQString(GetRef()), ToQString(state)));
  }

  std::shared_ptr<StateChangeNotifier> notifier = StateChangeNotifier::GetNotifier(callerNotifier);
  
  // On state change will take care of cleanup (deleting task,
  // clearing fields, etc)
  SetState(AssetDefs::Canceled, notifier);
}



void
LeafAssetVersionImplD::DoClean(const std::shared_ptr<StateChangeNotifier> callerNotifier)
{
  if (state == AssetDefs::Offline)
    return;

  if (subtype == "Source")
    return;

  std::shared_ptr<StateChangeNotifier> notifier = StateChangeNotifier::GetNotifier(callerNotifier);

  // On state change will take care of cleanup (deleting task,
  // clearing fields, etc)
  SetState(AssetDefs::Offline, notifier);

  // now try to clean my inputs too
  for (std::vector<std::string>::const_iterator i = inputs.begin();
       i != inputs.end(); ++i) {
    AssetVersionD input(*i);
    if (input) {
      if (input->OkToCleanAsInput()) {
        MutableAssetVersionD(*i)->DoClean(notifier);
      }
    } else {
      notify(NFY_WARN, "'%s' has broken input '%s'",
             GetRef().c_str(), i->c_str());
    }
  }
}


// ****************************************************************************
// ***  CompositeAssetVersionImplD
// ****************************************************************************
void
CompositeAssetVersionImplD::HandleChildStateChange(const std::shared_ptr<StateChangeNotifier> notifier) const
{
  notify(NFY_VERBOSE, "CompositeAssetVersionImplD::HandleChildStateChange: %s", GetRef().c_str());
  SyncState(notifier);
}

void
CompositeAssetVersionImplD::HandleInputStateChange(InputStates, const std::shared_ptr<StateChangeNotifier> notifier) const
{
  if (children.empty()) {
    // Undecided composites need to listen to inputs
    notify(NFY_VERBOSE, "HandleInputStateChange: %s", GetRef().c_str());
    SyncState(notifier);
  }
}

void
CompositeAssetVersionImplD::HandleChildProgress(const std::string &) const
{
  // TODO: - implement me some day
}


bool
CompositeAssetVersionImplD::CacheInputVersions(void) const
{
  return (NeedComputeState() &&
          (children.empty() || CompositeStateCaresAboutInputsToo()));
}

AssetDefs::State
CompositeAssetVersionImplD::ComputeState(void) const
{
  if (!NeedComputeState()) {
    return state;
  }

  // Undecided composites take their state from their inputs
  if (children.empty()) {
    bool blockersAreOffline;
    AssetDefs::State statebyinputs = StateByInputs(&blockersAreOffline);
    return statebyinputs;
  }

  // some composite assets (namely Database) care about the state of their
  // inputs, for all others all that matters is the state of their children
  if (CompositeStateCaresAboutInputsToo()) {
    bool blockersAreOffline;
    AssetDefs::State statebyinputs = StateByInputs(&blockersAreOffline);
    if (statebyinputs != AssetDefs::Queued) {
      // something is wrong with my inputs (or they're not done yet)
      if ((statebyinputs == AssetDefs::Blocked) && blockersAreOffline) {
        if (OfflineInputsBreakMe()) {
          return statebyinputs;
        } else {
          // fall through and compute my state based on my children
        }
      } else {
        return statebyinputs;
      }
    } else {
      // fall through and compute my state based on my children
    }
  } else {
    // fall through and compute my state based on my children
  }



  // find out how my children are doing
  uint numkids = children.size();
  uint numgood = 0;
  uint numblocking = 0;
  uint numinprog = 0;
  uint numfailed = 0;
  for (std::vector<std::string>::const_iterator c = children.begin();
       c != children.end(); ++c) {
    AssetVersion child(*c);
    if (child) {
      AssetDefs::State cstate = child->state;
      if (cstate == AssetDefs::Succeeded) {
        ++numgood;
      } else if (cstate == AssetDefs::Failed) {
        ++numfailed;
      } else if (cstate == AssetDefs::InProgress) {
        ++numinprog;
      } else if (cstate & (AssetDefs::Blocked  |
                           AssetDefs::Canceled |
                           AssetDefs::Offline  |
                           AssetDefs::Bad)) {
        ++numblocking;
      }
    } else {
      ++numblocking;
    }
  }


  // determine my state based on my children
  if (numkids == numgood) {
    return AssetDefs::Succeeded;
  } else if (numblocking || numfailed) {
    return AssetDefs::Blocked;
  } else if (numgood || numinprog) {
    return AssetDefs::InProgress;
  } else {
    return AssetDefs::Queued;
  }
}

void
CompositeAssetVersionImplD::DelayedBuildChildren(void)
{
  // NoOp - OnStateChange will see that I haven't added any children
  // and will set my state to Succeeded
}

void
CompositeAssetVersionImplD::OnStateChange(AssetDefs::State newstate,
                                          AssetDefs::State oldstate)
{
  notify(NFY_VERBOSE,
         "CompositeAssetVersionImplD::OnStateChange() %s: %s -> %s",
         GetRef().c_str(),
         ToString(oldstate).c_str(),
         ToString(newstate).c_str());
  AssetVersionImplD::OnStateChange(newstate, oldstate);

  // certain plugins (e.g. Database, RasterProject) don't create their
  // children until after their inputs are done.
  // Composite::HandleInputStateChange detects this case and defer to this
  // function.  Detect this case and call DelayedBuildChildren
  if (children.empty() &&
      (!AssetDefs::Ready(oldstate) &&
       AssetDefs::Working(state))) {
    try {
      DelayedBuildChildren();
    } catch (const std::exception &e) {
      notify(NFY_WARN, "Exception during OnStateChange: %s", e.what());
      WriteFatalLogfile(GetRef(), "DelayedBuildChildren", e.what());
      SetState(AssetDefs::Failed);
      return;
    } catch (...) {
      notify(NFY_WARN, "Unknown exception during OnStateChange");
      WriteFatalLogfile(GetRef(), "DelayedBuildChildren",
                        "Unknown error");
      SetState(AssetDefs::Failed);
      return;
    }

    if (!children.empty()) {
      SyncState();
    } else {
      SetState(AssetDefs::Succeeded);
    }
  }
}

void
CompositeAssetVersionImplD::ChildrenToCancel(std::vector<AssetVersion> &out)
{
  copy(children.begin(), children.end(), back_inserter(out));
}

void
CompositeAssetVersionImplD::AddChild(MutableAssetVersionD &child)
{
  // add ourself as the parent
  child->parents.push_back(GetRef());

  // add the child to our list
  children.push_back(child->GetRef());
}

void
CompositeAssetVersionImplD::AddChildren
(std::vector<MutableAssetVersionD> &kids)
{
  for (std::vector<MutableAssetVersionD>::iterator child = kids.begin();
       child != kids.end(); ++child) {

    // add ourself as the parent
    (*child)->parents.push_back(GetRef());

    // add the child to our list
    children.push_back((*child)->GetRef());
  }
}

void
CompositeAssetVersionImplD::Rebuild(const std::shared_ptr<StateChangeNotifier> callerNotifier)
{
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
  if (state & (AssetDefs::Succeeded | AssetDefs::Offline | AssetDefs::Bad)) {
    throw khException(kh::tr("%1 marked as %2. Refusing to resume.")
                      .arg(ToQString(GetRef()), ToQString(state)));
  }

  std::shared_ptr<StateChangeNotifier> notifier = StateChangeNotifier::GetNotifier(callerNotifier);

  std::vector<AssetVersion> tocancel;
  ChildrenToCancel(tocancel);
  if (tocancel.size()) {
    for (std::vector<AssetVersion>::const_iterator i = tocancel.begin();
         i != tocancel.end(); ++i) {
      // only rebuild the child if it is necessary
      if (*i) {
        if ((*i)->state & (AssetDefs::Canceled | AssetDefs::Failed)) {
          MutableAssetVersionD child((*i)->GetRef());
          child->Rebuild(notifier);
        }
      } else {
        notify(NFY_WARN, "'%s' has broken child to resume '%s'",
               GetRef().c_str(), i->Ref().c_str());
      }
    }
  }
  state = AssetDefs::New; // low-level to avoid callbacks
  SyncState(notifier);
}


void
CompositeAssetVersionImplD::Cancel(const std::shared_ptr<StateChangeNotifier> callerNotifier)
{
  if (!CanCancel()) {
    throw khException(kh::tr("%1 already %2. Unable to cancel.")
                      .arg(ToQString(GetRef()), ToQString(state)));
  }

  std::shared_ptr<StateChangeNotifier> notifier = StateChangeNotifier::GetNotifier(callerNotifier);

  SetState(AssetDefs::Canceled, notifier);

  std::vector<AssetVersion> tocancel;
  ChildrenToCancel(tocancel);
  if (tocancel.size()) {
    for (std::vector<AssetVersion>::const_iterator i = tocancel.begin();
         i != tocancel.end(); ++i) {
      // only cancel the child if it's not already finished
      if (*i) {
        if (!AssetDefs::Finished((*i)->state)) {
          MutableAssetVersionD child((*i)->GetRef());
          child->Cancel(notifier);
        }
      } else {
        notify(NFY_WARN, "'%s' has broken child to cancel '%s'",
               GetRef().c_str(), i->Ref().c_str());
      }
    }
  }
}


void
CompositeAssetVersionImplD::DoClean(const std::shared_ptr<StateChangeNotifier> callerNotifier)
{
  if (state == AssetDefs::Offline)
    return;

  std::shared_ptr<StateChangeNotifier> notifier = StateChangeNotifier::GetNotifier(callerNotifier);

  // On state change will take care of cleanup (deleting task,
  // clearing fields, etc)
  SetState(AssetDefs::Offline, notifier);

  // now try to clean my children
  for (std::vector<std::string>::const_iterator c = children.begin();
       c != children.end(); ++c) {
    AssetVersionD child(*c);
    if (child) {
      if ((child->state != AssetDefs::Offline) && child->OkToClean()) {
        MutableAssetVersionD(*c)->DoClean(notifier);
      }
    } else {
      notify(NFY_WARN, "'%s' has broken child '%s'",
             GetRef().c_str(), c->c_str());
    }
  }

  // now try to clean my inputs too
  for (std::vector<std::string>::const_iterator i = inputs.begin();
       i != inputs.end(); ++i) {
    AssetVersionD input(*i);
    if (input) {
      if (input->OkToCleanAsInput()) {
        MutableAssetVersionD(*i)->DoClean(notifier);
      }
    } else {
      notify(NFY_WARN, "'%s' has broken input '%s'",
             GetRef().c_str(), i->c_str());
    }
  }
}

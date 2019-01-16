/*
 * Copyright 2017 Google Inc.
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

#ifndef __AssetVersionD_h
#define __AssetVersionD_h

#include <AssetVersion.h>
#include "AssetHandleD.h"
#include <autoingest/sysman/.idl/TaskStorage.h>
#include <set>
#include <map>
#include <memory>

// ****************************************************************************
// ***  AssetVersionImplD
// ****************************************************************************
class AssetVersionImplD : public virtual AssetVersionImpl
{
 private:
  friend class AssetImplD;
  friend class khAssetManager;
  friend class DerivedAssetHandle_<AssetVersion, AssetVersionImplD>;
  friend class MutableAssetHandleD_<DerivedAssetHandle_<AssetVersion,
                                                        AssetVersionImplD> >;

  // private and unimplemented -- illegal to copy an AssetVersionImpl
  AssetVersionImplD(const AssetVersionImplD&);
  AssetVersionImplD& operator=(const AssetVersionImplD&);

 protected:
  // Tracks the state of inputs to a given asset version that have changed. Each
  // entry maps a state to the number of inputs that have changed to that state.
  struct InputStates {
    size_t numSucceeded;
    bool allWorkingOrSucceeded;
    InputStates() : numSucceeded(0), allWorkingOrSucceeded(true) {}
  };

  // Helper class to efficently send updates of state changes to other asset
  // versions.
  class StateChangeNotifier {
    private:
      std::set<std::string> parentsToNotify;
      std::map<std::string, InputStates> listenersToNotify;
      void NotifyParents(std::shared_ptr<StateChangeNotifier>);
      void NotifyListeners(std::shared_ptr<StateChangeNotifier>);
    public:
      static std::shared_ptr<StateChangeNotifier> GetNotifier(std::shared_ptr<StateChangeNotifier>);
      StateChangeNotifier() = default;
      ~StateChangeNotifier();
      void AddParentsToNotify(const std::vector<std::string> &);
      void AddListenersToNotify(const std::vector<std::string> &, AssetDefs::State);
  };

  static khRefGuard<AssetVersionImplD> Load(const std::string &boundref);
  virtual bool Save(const std::string &filename) const = 0;

  bool NeedComputeState(void) const {
    if (state & (AssetDefs::Bad |
                 AssetDefs::Offline |
                 AssetDefs::Canceled)) {
      // these states are explicitly set and must be explicitly cleared
      return false;
    } else {
      return true;
    }
  }
  virtual AssetDefs::State ComputeState(void) const = 0;
  virtual bool CacheInputVersions(void) const = 0;

  // since AssetVersionImpl is a virtual base class
  // my derived classes will initialize it directly
  // therefore I don't need a contructor from storage
  AssetVersionImplD(void) : AssetVersionImpl(), verholder(0) { }

  // used when being contructed from an asset
  // these are the inputs I need to bind and attach to
  AssetVersionImplD(const std::vector<std::string> &inputs);

  void AddInputAssetRefs(const std::vector<std::string> &inputs_);
  AssetDefs::State StateByInputs(bool *blockersAreOffline = 0,
                                 uint32 *numWaiting = 0) const;
  template<bool propagate = true>
  void SetState(AssetDefs::State newstate, const std::shared_ptr<StateChangeNotifier> = nullptr);
  void SetProgress(double newprogress);
  virtual void SyncState(const std::shared_ptr<StateChangeNotifier> = nullptr) const; // const so can be called w/o mutable handle
  // will create a mutable handle itself if it
  // needs to call SetState
  void PropagateStateChange(const std::shared_ptr<StateChangeNotifier> = nullptr);
  void PropagateProgress(void);
  virtual void HandleTaskLost(const TaskLostMsg &msg);
  virtual void HandleTaskProgress(const TaskProgressMsg &msg);
  virtual void HandleTaskDone(const TaskDoneMsg &msg);
  virtual void HandleChildStateChange(const std::shared_ptr<StateChangeNotifier>) const;
  virtual void HandleInputStateChange(InputStates, const std::shared_ptr<StateChangeNotifier>) const = 0;
  virtual void HandleChildProgress(const std::string &) const;
  virtual void OnStateChange(AssetDefs::State newstate,
                             AssetDefs::State oldstate);
  virtual bool OfflineInputsBreakMe(void) const { return false; }
 public:

  bool OkToClean(std::vector<std::string> *wouldbreak = 0) const;
  bool OkToCleanAsInput(void) const;
  void SetBad(void);
  void ClearBad(void);
  void Clean(void);
  virtual void Cancel(const std::shared_ptr<StateChangeNotifier> = nullptr) = 0;
  virtual void Rebuild(const std::shared_ptr<StateChangeNotifier> = nullptr) = 0;
  virtual void DoClean(const std::shared_ptr<StateChangeNotifier> = nullptr) = 0;
  virtual bool MustForceUpdate(void) const { return false; }

  class InputVersionHolder : public khRefCounter {
   public:
    std::vector<AssetVersion> inputvers;

    InputVersionHolder(const std::vector<std::string> &inputrefs);
    InputVersionHolder(const std::vector<AssetVersion> &inputvers_);
  };

  class InputVersionGuard : public khRefGuard<InputVersionHolder> {
    const AssetVersionImplD *impl;
   public:
    InputVersionGuard(const DerivedAssetHandle_<AssetVersion, AssetVersionImplD> &ver);
    InputVersionGuard(const AssetVersionImplD *impl_);
    InputVersionGuard(const AssetVersionImplD *impl_,
                      const std::vector<AssetVersion> &inputvers);
    ~InputVersionGuard(void);
  };
  friend class InputVersionGuard;

  void GetInputFilenames(std::vector<std::string> &out) const;

  static void WriteFatalLogfile(const AssetVersionRef &verref,
                                const std::string &prefix,
                                const std::string &error) throw();
 private:
  // accessable only through InputVersionGuard
  mutable InputVersionHolder* verholder;

};


typedef DerivedAssetHandle_<AssetVersion, AssetVersionImplD> AssetVersionD;
typedef MutableAssetHandleD_<AssetVersionD> MutableAssetVersionD;


class LeafAssetVersionImplD : public virtual LeafAssetVersionImpl,
                              public AssetVersionImplD
{
 protected:
  mutable uint32 numWaitingFor;

  // since AssetVersionImpl and LeafAssetVersionImpl are virtual base
  // classes my derived classes will initialize it directly
  // therefore I don't need a contructor from storage
  LeafAssetVersionImplD(void)
      : AssetVersionImpl(), LeafAssetVersionImpl(),
        AssetVersionImplD(), numWaitingFor(0) { }
  // used when being contructed from an asset
  // these are the inputs I need to bind and attach to
  LeafAssetVersionImplD(const std::vector<std::string> &inputs)
      : AssetVersionImpl(), LeafAssetVersionImpl(),
        AssetVersionImplD(inputs), numWaitingFor(0) { }

  void SubmitTask(void);
  void ClearOutfiles(void);

  virtual AssetDefs::State ComputeState(void) const;
  virtual bool CacheInputVersions(void) const;
  virtual void HandleTaskLost(const TaskLostMsg &msg);
  virtual void HandleTaskProgress(const TaskProgressMsg &msg);
  virtual void HandleTaskDone(const TaskDoneMsg &msg);
  virtual void HandleInputStateChange(InputStates, const std::shared_ptr<StateChangeNotifier>) const;
  virtual void OnStateChange(AssetDefs::State newstate,
                             AssetDefs::State oldstate);
  virtual void DoSubmitTask(void) = 0;
  virtual bool OfflineInputsBreakMe(void) const { return false; }

 public:
  virtual void Cancel(const std::shared_ptr<StateChangeNotifier> = nullptr);
  virtual void Rebuild(const std::shared_ptr<StateChangeNotifier> = nullptr);
  virtual void DoClean(const std::shared_ptr<StateChangeNotifier> = nullptr);
};


class CompositeAssetVersionImplD : public virtual CompositeAssetVersionImpl,
                                   public AssetVersionImplD
{
 protected:
  // since AssetVersionImpl and CompositeAssetVersionImpl are virtual base
  // classes my derived classes will initialize it directly
  // therefore I don't need a contructor from storage
  CompositeAssetVersionImplD(void)
      : AssetVersionImpl(), CompositeAssetVersionImpl(),
        AssetVersionImplD() { }
  // used when being contructed from an asset
  // these are the inputs I need to bind and attach to
  CompositeAssetVersionImplD(const std::vector<std::string> &inputs)
      : AssetVersionImpl(), CompositeAssetVersionImpl(),
        AssetVersionImplD(inputs) { }

  virtual AssetDefs::State ComputeState(void) const;
  virtual bool CacheInputVersions(void) const;
  virtual void HandleChildStateChange(const std::shared_ptr<StateChangeNotifier>) const;
  virtual void HandleInputStateChange(InputStates, const std::shared_ptr<StateChangeNotifier>) const;
  virtual void HandleChildProgress(const std::string &) const;
  virtual void DelayedBuildChildren(void);
  virtual void OnStateChange(AssetDefs::State newstate,
                             AssetDefs::State oldstate);
  virtual void ChildrenToCancel(std::vector<AssetVersion> &out);
  virtual bool CompositeStateCaresAboutInputsToo(void) const { return false; }

  void AddChild(MutableAssetVersionD &child);
  void AddChildren(std::vector<MutableAssetVersionD> &children);

 public:
  virtual void Cancel(const std::shared_ptr<StateChangeNotifier> = nullptr);
  virtual void Rebuild(const std::shared_ptr<StateChangeNotifier> = nullptr);
  virtual void DoClean(const std::shared_ptr<StateChangeNotifier> = nullptr);
};

#endif /* __AssetVersionD_h */

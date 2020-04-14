/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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
#include "common/khRefCounter.h"
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

  AssetVersionImplD(const AssetVersionImplD&) = delete;
  AssetVersionImplD& operator=(const AssetVersionImplD&) = delete;

 protected:
  // Tracks the state of inputs and children to a given asset version that have
  // changed. Each entry contains enough information for the asset to determine
  // if it needs to change its state.
  struct NotifyStates {
    size_t numSucceeded;
    bool allWorkingOrSucceeded;
    NotifyStates() : numSucceeded(0), allWorkingOrSucceeded(true) {}
  };

  // Helper class to efficently send updates of state changes to other asset
  // versions.
  class StateChangeNotifier {
    private:
      using NotifyMap = std::map<SharedString, NotifyStates>;
      enum NotifyType {LISTENER, PARENT};
      NotifyMap parentsToNotify;
      NotifyMap listenersToNotify;
      void AddToNotify(const std::vector<SharedString> &, AssetDefs::State, NotifyMap &);
      void NotifyParents(std::shared_ptr<StateChangeNotifier>);
      void NotifyListeners(std::shared_ptr<StateChangeNotifier>);
      void DoNotify(
          NotifyMap &,
          std::shared_ptr<StateChangeNotifier>,
          NotifyType);
    public:
      static std::shared_ptr<StateChangeNotifier> GetNotifier(std::shared_ptr<StateChangeNotifier>);
      StateChangeNotifier() = default;
      ~StateChangeNotifier();
      void AddParentsToNotify(const std::vector<SharedString> &, AssetDefs::State);
      void AddListenersToNotify(const std::vector<SharedString> &, AssetDefs::State);
  };

  virtual AssetDefs::State ComputeState(void) const = 0;
  virtual bool CacheInputVersions(void) const = 0;

  // since AssetVersionImpl is a virtual base class
  // my derived classes will initialize it directly
  // therefore I don't need a contructor from storage
  AssetVersionImplD(void) : AssetVersionImpl(), verholder(nullptr), numInputsWaitingFor(0) { }

  // used when being contructed from an asset
  // these are the inputs I need to bind and attach to
  AssetVersionImplD(const std::vector<SharedString> &inputs);

  void AddInputAssetRefs(const std::vector<SharedString> &inputs_);
  AssetDefs::State StateByInputs(bool *blockersAreOffline = nullptr,
                                 std::uint32_t *numWaiting = nullptr) const;
  template<bool propagate = true>
  void SetState(AssetDefs::State newstate, const std::shared_ptr<StateChangeNotifier> = nullptr);
  void SetProgress(double newprogress);
  virtual bool SyncState(const std::shared_ptr<StateChangeNotifier> = nullptr) const; // const so can be called w/o mutable handle
  // will create a mutable handle itself if it
  // needs to call SetState
  void PropagateStateChange(const std::shared_ptr<StateChangeNotifier> = nullptr);
  virtual void HandleTaskLost(const TaskLostMsg &msg);
  virtual void HandleChildStateChange(NotifyStates, const std::shared_ptr<StateChangeNotifier>) const;
  virtual void HandleInputStateChange(NotifyStates, const std::shared_ptr<StateChangeNotifier>) const = 0;
  inline bool BlockedByOfflineInputs(const InputAndChildStateData & stateData) const;
  virtual bool OfflineInputsBreakMe(void) const { return false; }
  virtual std::uint32_t GetChildrenWaitingFor() const { return 0; }
 public:

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

  bool OkToClean(std::vector<std::string> *wouldbreak = nullptr) const;
  bool OkToCleanAsInput(void) const;
  void SetBad(void);
  void ClearBad(void);
  void Clean(void);
  virtual void Cancel(const std::shared_ptr<StateChangeNotifier> = nullptr) = 0;
  virtual void Rebuild(const std::shared_ptr<StateChangeNotifier> = nullptr) = 0;
  virtual void DoClean(const std::shared_ptr<StateChangeNotifier> = nullptr) = 0;
  virtual bool MustForceUpdate(void) const { return false; }
  virtual AssetDefs::State OnStateChange(AssetDefs::State newstate,
                                         AssetDefs::State oldstate) override;
  virtual void HandleTaskProgress(const TaskProgressMsg &msg);
  virtual void HandleTaskDone(const TaskDoneMsg &msg);
  virtual void SetAndPropagateState(AssetDefs::State newstate) override
    { SetState(newstate); }

  class InputVersionHolder : public khRefCounter {
   public:
    std::vector<AssetVersion> inputvers;

    InputVersionHolder(const std::vector<SharedString> &inputrefs);
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

  // This class includes static and non-static versions of this function so
  // you can call it with or without an asset version.
  virtual void WriteFatalLogfile(const std::string &prefix, const std::string &error) const throw() override;
  static void WriteFatalLogfile(const AssetVersionRef &verref,
                                const std::string &prefix,
                                const std::string &error) throw();
 private:
  // accessable only through InputVersionGuard
  mutable InputVersionHolder* verholder;

 protected:
  mutable std::uint32_t numInputsWaitingFor;
};


typedef DerivedAssetHandle_<AssetVersion, AssetVersionImplD> AssetVersionD;
typedef MutableAssetHandleD_<AssetVersionD> MutableAssetVersionD;


class LeafAssetVersionImplD : public virtual LeafAssetVersionImpl,
                              public AssetVersionImplD
{
 protected:
  // since AssetVersionImpl and LeafAssetVersionImpl are virtual base
  // classes my derived classes will initialize it directly
  // therefore I don't need a contructor from storage
  LeafAssetVersionImplD(void)
      : AssetVersionImpl(), LeafAssetVersionImpl(),
        AssetVersionImplD() { }
  // used when being contructed from an asset
  // these are the inputs I need to bind and attach to
  LeafAssetVersionImplD(const std::vector<SharedString> &inputs)
      : AssetVersionImpl(), LeafAssetVersionImpl(),
        AssetVersionImplD(inputs) { }

  void SubmitTask(void);
  void ClearOutfiles(void);

  virtual AssetDefs::State ComputeState(void) const;
  virtual bool CacheInputVersions(void) const;
  virtual void HandleTaskLost(const TaskLostMsg &msg);
  virtual void HandleInputStateChange(NotifyStates, const std::shared_ptr<StateChangeNotifier>) const;
  virtual void DoSubmitTask(void) = 0;
  virtual bool OfflineInputsBreakMe(void) const { return false; }

 public:
  virtual void HandleTaskProgress(const TaskProgressMsg &msg) override;
  virtual void HandleTaskDone(const TaskDoneMsg &msg) override;
  virtual void ResetOutFiles(const std::vector<std::string> &) override;
  virtual bool RecalcState(WaitingFor &) const override;
  virtual void Cancel(const std::shared_ptr<StateChangeNotifier> = nullptr);
  virtual void Rebuild(const std::shared_ptr<StateChangeNotifier> = nullptr);
  virtual void DoClean(const std::shared_ptr<StateChangeNotifier> = nullptr);
  virtual AssetDefs::State OnStateChange(AssetDefs::State newstate,
                                         AssetDefs::State oldstate) override;
  virtual AssetDefs::State CalcStateByInputsAndChildren(const InputAndChildStateData &) const override;
  virtual bool InputStatesAffectMyState(AssetDefs::State stateByInputs, bool blockedByOfflineInputs) const override;
};


class CompositeAssetVersionImplD : public virtual CompositeAssetVersionImpl,
                                   public AssetVersionImplD
{
 protected:
  mutable std::uint32_t numChildrenWaitingFor;
    
  // since AssetVersionImpl and CompositeAssetVersionImpl are virtual base
  // classes my derived classes will initialize it directly
  // therefore I don't need a contructor from storage
  CompositeAssetVersionImplD(void)
      : AssetVersionImpl(), CompositeAssetVersionImpl(),
        AssetVersionImplD(), numChildrenWaitingFor(0) { }
  // used when being contructed from an asset
  // these are the inputs I need to bind and attach to
  CompositeAssetVersionImplD(const std::vector<SharedString> &inputs)
      : AssetVersionImpl(), CompositeAssetVersionImpl(),
        AssetVersionImplD(inputs), numChildrenWaitingFor(0) { }

  virtual AssetDefs::State ComputeState(void) const;
  virtual bool CacheInputVersions(void) const;
  virtual void HandleChildStateChange(NotifyStates, const std::shared_ptr<StateChangeNotifier>) const;
  virtual void HandleInputStateChange(NotifyStates, const std::shared_ptr<StateChangeNotifier>) const;
  virtual void DelayedBuildChildren(void);
  virtual bool CompositeStateCaresAboutInputsToo(void) const { return false; }
  virtual std::uint32_t GetChildrenWaitingFor() const override { return numChildrenWaitingFor; }

  void AddChild(MutableAssetVersionD &child);
  void AddChildren(std::vector<MutableAssetVersionD> &children);

 public:
  virtual void Cancel(const std::shared_ptr<StateChangeNotifier> = nullptr);
  virtual void Rebuild(const std::shared_ptr<StateChangeNotifier> = nullptr);
  virtual void DoClean(const std::shared_ptr<StateChangeNotifier> = nullptr);
  virtual AssetDefs::State CalcStateByInputsAndChildren(const InputAndChildStateData &) const override;
  virtual void DependentChildren(std::vector<SharedString> &out) const override;
  virtual AssetDefs::State OnStateChange(AssetDefs::State newstate,
                                         AssetDefs::State oldstate) override;
  virtual bool RecalcState(WaitingFor &) const override;
  virtual bool InputStatesAffectMyState(AssetDefs::State stateByInputs, bool blockedByOfflineInputs) const override;
};

#endif /* __AssetVersionD_h */

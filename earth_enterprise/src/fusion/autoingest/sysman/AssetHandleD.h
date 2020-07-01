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

#ifndef __AssetHandleD_h
#define __AssetHandleD_h

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <khException.h>
#include <khFileUtils.h>
#include "common/khCppStd.h"
#include "common/SharedString.h"
#include "common/khConstants.h"


/******************************************************************************
 ***  Derived ReadOnly Handles (Daemon Version)
 ***
 ***  This is for things like MosaicAssetD & BlendAssetVersionD
 ***      It derives from the ReadOnly derived handle (e.g. MosaicAsset) as well
 ***  as the generic daemon handle (e.g. AssetD)
 ******************************************************************************/
template <class ROBase_, class BaseD_, class Impl_>
class DerivedAssetHandleD_ : public virtual BaseD_, public ROBase_
{
 public:
  typedef Impl_ Impl;
  typedef ROBase_ ROBase;
  typedef BaseD_ BaseD;
  typedef typename BaseD::Base BBase; // virtual base 'Asset' or 'Version'
  typedef typename ROBase::Base BROBase; // this is assumed to be the same type as BBase
  typedef typename BBase::HandleType HandleType;
 public:
  virtual bool Valid(const HandleType & entry) const {
    // we have to check if it maps to Impl* since somebody
    // else may have loaded it into the storage manager
    return dynamic_cast<Impl*>(&*entry);
  }

  DerivedAssetHandleD_(void) : BBase(), BaseD(), ROBase() { }
  DerivedAssetHandleD_(const SharedString &ref_) :
      // Only call the common (virtually inherited) base class with the initializtion state.
      // It's the only one that has state anyway.  Also, explicitly calling the virtual base
      // class puts a build time check to ensure BBase is a virtural base class of this class.
      BBase(ref_), BaseD(), ROBase() { }

#if GEE_HAS_MOVE
  // Because we are using virtual inheritance in this class hierarchy we _should_ explicitly implement
  // copy/assigment and not allow the less effecient default generated implementations be used.
  // But, for move operations we *must* implement a correct version as the default version 
  // generated for us is not correct.
  DerivedAssetHandleD_(DerivedAssetHandleD_<ROBase_, BaseD_, Impl_>&& rhs) noexcept :
      // Again we want to only call the non-default constructor for the one virtual base 
      // class that has state.
      BBase(std::move(rhs)), BaseD(), ROBase() { }
  DerivedAssetHandleD_<ROBase_, BaseD_, Impl_>& operator =(DerivedAssetHandleD_<ROBase_, BaseD_, Impl_>&& rhs) noexcept {
    // here we just call the one virtually inherited base class that has state.
    // If we didn't do this the move operation would be done twice which is something
    // we don't want as we would end up loosing our state...
    BBase::operator =(std::move(rhs));

    return *this;
  }
#endif // GEE_HAS_MOVE

  DerivedAssetHandleD_(const DerivedAssetHandleD_<ROBase_, BaseD_, Impl_>& rhs) :
      // Again we want to only call the non-default constructor for the one virtual base 
      // class that has state.
      BBase(rhs), BaseD(), ROBase() { }
  DerivedAssetHandleD_<ROBase_, BaseD_, Impl_>& operator =(const DerivedAssetHandleD_<ROBase_, BaseD_, Impl_>& rhs) {
    // here we just call the one virtually inherited base class that has state.
    // If we didn't do this the copy operation would be done twice which is something
    // we don't want as the state would be copied twice.  Doing the copy twice still
    // ends up with the same answer but takes twice a long.
    BBase::operator =(rhs);

    return *this;
  }

  const Impl* operator->(void) const {
    this->Bind();
    // we ensure that the base class handle always points to our derived
    // type so this dynamic cast should never fail.  but it needs to be
    // dynamic instead of static since we're casting from a virtual base
    return dynamic_cast<const Impl*>(this->handle.operator->());
  }

  ~DerivedAssetHandleD_()
  {
#ifdef GEE_HAS_STATIC_ASSERT
    // nothing to do other than assert some compile time assuptions
    // we will get a compile error from above if BBase is not a virtual
    // base class but we also want to be sure BROBase is a virtual base
    // class and more specifically the same virtual base class.  This 
    // build time assert will give us a compile error if something changes 
    // that causes that to be untrue.
    static_assert(std::is_same<BBase, BROBase>::value, "BBase and BROBase *must* be the same type!!!");
#endif // GEE_HAS_STATIC_ASSERT
  }
};


// ****************************************************************************
// ***  Used for MutableAssetD & MutableAssetVersionD
// ****************************************************************************


template <class Base_>
class MutableAssetHandleD_ : public virtual Base_ {
  friend class khAssetManager;

 public:
  typedef Base_ Base;
  // our base also has a virtual base
  typedef typename Base::Base BBase;
  typedef typename Base::Impl Impl;

  // Test whether an asset is a project asset version.
  static inline bool IsProjectAssetVersion(const std::string& assetName) {
    return assetName.find(kProjectAssetVersionNumPrefix) != std::string::npos;
  }

 protected:
  virtual bool isMutable() const { return true; }

  // must not throw since it's called during stack unwinding
  static void AbortDirty(void) throw()
  {
    Base::storageManager().Abort();
  }

  static bool SaveDirtyToDotNew(khFilesTransaction &savetrans,
                                std::vector<SharedString> *saveDirty) {
    return Base::storageManager().SaveDirtyToDotNew(savetrans, saveDirty);
  }


  // We need to be more explicit about copy/move constructor and assignment as we are using virtual
  // inheritance.  We can't rely as much on compiler generated move operation.
 public:
  MutableAssetHandleD_(void) : BBase(), Base() { }
  MutableAssetHandleD_(const std::string &ref_) :
      // Only call the common (virtually inherited) base class with the initializtion state.
      // It's the only one that has state anyway.  Also, explicitly calling the virtual base
      // class puts an explicit check to ensure BBase a virtural base class of this class.
      BBase(ref_), Base() { }
  MutableAssetHandleD_(const SharedString &ref_) :
      BBase(ref_), Base() { }

  // you should be able to create a mutable handle from the non-mutable
  // equivalent. Don't automatically bind it and add it to the dirtyMap
  // If they never use it via the MutableHandle it isn't really dirty
  MutableAssetHandleD_(const Base &o) :
      // Again we want to only call the non-default constructor for the one virtual base 
      // class that has state.
      BBase(o.Ref()), Base() { }

  // you should be able to create a mutable handle from the non-mutable,
  // non-daemon equivalent. Don't automatically bind it and add it to
  // the dirtyMap. If they never use it via the MutableHandle it isn't
  // really dirty
  MutableAssetHandleD_(const BBase &o) :
      // Again we want to only call the non-default constructor for the one virtual base 
      // class that has state.
      BBase(o.Ref()), Base() { }

  MutableAssetHandleD_(const MutableAssetHandleD_<Base_> &o) :
      // Again we want to only call the non-default constructor for the one virtual base 
      // class that has state.
     BBase(o), Base() { }
  MutableAssetHandleD_<Base_>& operator =(const MutableAssetHandleD_<Base_> &o) {
    // here we just call the one virtually inherited base class that has state.
    // Unlike with DerivedAssetHandleD_ or MutableDerivedAssetHandleD_ this
    // base class only has one path to its instance anyway so this isn't as
    // important but I feel it is good to be explicit anyway. 
    BBase::operator = (o);

    return *this;
  }

#if GEE_HAS_MOVE
  MutableAssetHandleD_(MutableAssetHandleD_<Base_> &&o) noexcept :
    // Again we want to only call the non-default constructor for the one virtual base 
    // class that has state.
    BBase(std::move(o)), Base() { }
  MutableAssetHandleD_<Base_>& operator =(MutableAssetHandleD_<Base_> &&o) noexcept {
    // Again only call the virtual base class.  Not as important here as in DerivedAssetHandleD_
    // or MutableDerivedAssetHandleD_ but still good to be explicit
    BBase::operator = (std::move(o));

    return *this;
  }
#endif // GEE_HAS_MOVE

  Impl* operator->(void) {
    return const_cast<Impl*>(Base::operator->());
  }

  ~MutableAssetHandleD_() {
    this->storageManager().UpdateCacheItemSize(this->ref);
  }
};


// ****************************************************************************
// ***  MutableDerivedAssetHandleD_
// ***
// ***  Used for MutableMosaicAssetD & MutableBlendAssetVersionD
// ****************************************************************************
template <class DerivedBase_, class MutableBase_>
class MutableDerivedAssetHandleD_ : public DerivedBase_, public MutableBase_
{
 public:
  typedef DerivedBase_ DerivedBase;
  typedef MutableBase_ MutableBase;
  typedef typename DerivedBase::Impl  Impl;
  typedef typename DerivedBase::BBase BBase;  // vbase 'Asset' or 'Version'
  typedef typename DerivedBase::BaseD BaseD;  // vbase 'AssetD' or 'VersionD'
  typedef typename BBase::HandleType HandleType;
  typedef typename MutableBase::BBase MBBase; // this is assumed to be the same type as BBase
 protected:
  using MutableBase::isMutable;

 public:
  // must overide these to resolve ambiguities
  virtual bool Valid(const HandleType & entry) const {
    return DerivedBase::Valid(entry);
  }

  //    Only this leaf-most daemon handle can be constructed from
  // the raw Impl. Rather than give all the base classes constructors
  // that could be mistakenly used, this one will just do all the work
  // itself
  //    This is public because the various {name}Factory classes must
  // invoke this constructor and there is no way to declare it a friend
  // here since we can't list the name
  explicit MutableDerivedAssetHandleD_(const std::shared_ptr<Impl>& handle_) :
      BBase(), BaseD(), DerivedBase(), MutableBase() {
    this->handle = handle_;
    if (this->handle != nullptr) {
      // we have a good handle

      // record the ref - since it comes from GetRef() we don't have to
      // call BindRef()
      this->ref = this->handle->GetRef();

      // Tell the storage manager about it
      this->storageManager().AddNew(this->ref, this->handle);
    }
  }
 
 public:
  MutableDerivedAssetHandleD_(void) :
      BBase(), BaseD(), DerivedBase(), MutableBase() { }
  MutableDerivedAssetHandleD_(const SharedString &ref_) :
      // Only call the common (virtually inherited) base class with the initializtion state.
      // It's the only one that has state anyway.  Also, explicitly calling the virtual base
      // class puts a build time check to ensure BBase is a virtural base class of this class.
      BBase(ref_), BaseD(), DerivedBase(), MutableBase() { }

  // you should be able to create a mutable handle from the non-mutable
  // equivalent. Don't automatically bind it and add it to the dirtyMap
  // If they never use it via the MutableHandle it isn't really dirty
  MutableDerivedAssetHandleD_(const DerivedBase &o) :
      BBase(o.Ref()), BaseD(), DerivedBase(), MutableBase() { }


#if GEE_HAS_MOVE
  // Because we are using virtual inheritance in this class hierarchy we _should_ explicitly implement
  // copy/assigment and not allow the less effecient default generated implementations be used.
  // But, for move operations we *must* implement a correct version as the default version 
  // generated for us is not correct.
  MutableDerivedAssetHandleD_(MutableDerivedAssetHandleD_<DerivedBase_, MutableBase_>&& rhs) noexcept :
      // Again we want to only call the non-default constructor for the one virtual base 
      // class that has state.
      BBase(std::move(rhs)), BaseD(), DerivedBase(), MutableBase() { }
  MutableDerivedAssetHandleD_<DerivedBase_, MutableBase_>& operator =(MutableDerivedAssetHandleD_<DerivedBase_, MutableBase_>&& rhs) noexcept {
    // here we just call the one virtually inherited base class that has state.
    // If we didn't do this the move operation would be done twice which is something
    // we don't want as we would end up loosing our state...
    BBase::operator =(std::move(rhs));
  
    return *this;
  }
#endif // GEE_HAS_MOVE

  MutableDerivedAssetHandleD_(const MutableDerivedAssetHandleD_<DerivedBase_, MutableBase_>& rhs) :
      // Only call the common (virtually inherited) base class with the initializtion state.
      // It's the only one that has state anyway.
      BBase(rhs), BaseD(), DerivedBase(), MutableBase() { }
  MutableDerivedAssetHandleD_<DerivedBase_, MutableBase_>& operator =(const MutableDerivedAssetHandleD_<DerivedBase_, MutableBase_>& rhs) {
    // here we just call the one virtually inherited base class that has state.
    // If we didn't do this the copy operation would be done twice which is something
    // we don't want as the state would be copied twice.  Doing the copy twice still
    // ends up with the same answer but takes twice a long.
    BBase::operator =(rhs);

    return *this;
  }

  Impl* operator->(void) {
    return const_cast<Impl*>(DerivedBase::operator->());
  }

  ~MutableDerivedAssetHandleD_()
  {
#ifdef GEE_HAS_STATIC_ASSERT
    // nothing to do other than assert some compile time assuptions
    // we will get a compile error from above if BBase is not a virtual
    // base class but we also want to be sure BROBase is a virtual base
    // class and more specifically the same virtual base class.  This 
    // build time assert will give us a compile error if something changes 
    // that causes that to be untrue.
    static_assert(std::is_same<BBase, MBBase>::value, "BBase and MBBase *must* be the same type!!!");
#endif // GEE_HAS_STATIC_ASSERT
    this->storageManager().UpdateCacheItemSize(this->ref);
  }
};


#endif /* __AssetHandleD_h */

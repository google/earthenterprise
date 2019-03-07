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
#include <string>
#include <vector>
#include <khException.h>
#include <khFileUtils.h>
#include "common/khCppStd.h"


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
 protected:
  // must overide both of these to verify that things match
  // my 'Impl' type
  virtual HandleType CacheFind(const std::string &boundref) const {
    HandleType entry;
    if (this->cache().Find(boundref, entry)) {
      // we have to check if it maps to Impl* since somebody
      // else may have put it in the cache
      if (!dynamic_cast<Impl*>(&*entry)) {
        entry = HandleType();
      }
    }
    return entry;
  }
  virtual HandleType Load(const std::string &boundref) const {
    // Impl::Load will succeed or throw.
    // The derived khRefGuard will be automatically converted
    // the the base khRefGuard
    return HandleType(Impl::Load(boundref));
  }
 public:
  DerivedAssetHandleD_(void) : BBase(), BaseD(), ROBase() { }
  DerivedAssetHandleD_(const std::string &ref_) :
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

  typedef std::map<std::string, Base> DirtyMap;
  static DirtyMap dirtyMap;

  // Test whether an asset is a project asset version.
  static inline bool IsProjectAssetVersion(const std::string& assetName) {
    return assetName.find(kProjectAssetVersionNumPrefix) != std::string::npos;
  }

 protected:
  virtual void OnBind(const std::string &boundref) const {
    Base::OnBind(boundref);
    // if already exists, old one will win, that's OK
    dirtyMap.insert(std::make_pair(boundref, *this));
  }

  // must not throw since it's called during stack unwinding
  static void AbortDirty(void) throw()
  {
    // remove all the dirty Impls from the cache
    for (typename DirtyMap::const_iterator d = dirtyMap.begin();
         d != dirtyMap.end(); ++d) {
      Base::cache().Remove(d->first, false); // false -> don't prune
    }
    Base::cache().Prune(); // prune at the end to avoid possible prune thrashing

    // now clear the dirtyMap itself
    dirtyMap.clear();
  }

  static void PrintDirty(const std::string &header) {
    if (dirtyMap.size()) {
      fprintf(stderr, "========== %d dirty %s ==========\n",
              dirtyMap.size(), header.c_str());
      for (typename DirtyMap::const_iterator d = dirtyMap.begin();
           d != dirtyMap.end(); ++d) {
        fprintf(stderr, "%s -> %p\n",
                d->first.c_str(), d->second.operator->());
      }
    }
  }


  static bool SaveDirtyToDotNew(khFilesTransaction &savetrans,
                                std::vector<std::string> *saveDirty) {
    for (typename DirtyMap::const_iterator d = dirtyMap.begin();
         d != dirtyMap.end(); ++d) {
      // TODO: - check to see if actually dirty
      if ( 1 ) {
        std::string filename = d->second->XMLFilename() + ".new";
        if (d->second->Save(filename)) {
          savetrans.AddNewPath(filename);
          if (saveDirty) {
            saveDirty->push_back(d->first);
          }
        } else {
          return false;
        }
      }
    }
    return true;
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

  using Base::operator->;
  Impl* operator->(void) {
    return const_cast<Impl*>(Base::operator->());
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
  // must overide these to resolve ambiguities
  virtual HandleType CacheFind(const std::string &boundref) const {
    return DerivedBase::CacheFind(boundref);
  }
  virtual HandleType Load(const std::string &boundref) const {
    return DerivedBase::Load(boundref);
  }
  virtual void OnBind(const std::string &boundref) const {
    return MutableBase::OnBind(boundref);
  }

 public:
  //    Only this leaf-most daemon handle can be constructed from
  // the raw Impl. Rather than give all the base classes constructors
  // that could be mistakenly used, this one will just do all the work
  // iteself
  //    This is public because the various {name}Factory classes must
  // invoke this constructor and there is no way to declare it a friend
  // here since we can't list the name
  explicit MutableDerivedAssetHandleD_(const khRefGuard<Impl> &handle_) :
      BBase(), BaseD(), DerivedBase(), MutableBase() {
    this->handle = handle_;
    if (this->handle) {
      // we have a good handle

      // record the ref - since it comes from GetRef() we don't have to
      // call BindRef()
      this->ref = this->handle->GetRef();

      // add it to the cache
      this->cache().Add(this->ref, this->handle);

      // add it to the dirty list - since it just sprang into existence
      // we need to make sure it gets saved
      MutableBase::dirtyMap.insert(std::make_pair(this->ref, *this));
    }
  }
 
 public:
  MutableDerivedAssetHandleD_(void) :
      BBase(), BaseD(), DerivedBase(), MutableBase() { }
  MutableDerivedAssetHandleD_(const std::string &ref_) :
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
    MBBase::operator =(std::move(rhs));
  
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

  using DerivedBase::operator->;
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
  }
};


#endif /* __AssetHandleD_h */

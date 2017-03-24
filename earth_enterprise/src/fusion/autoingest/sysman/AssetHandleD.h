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
      BBase(ref_), BaseD(), ROBase() { }

  // the compiler generated assignment and copy constructor are fine for us
  // we have no addition members or semantics to maintain

  const Impl* operator->(void) const {
    this->Bind();
    // we ensure that the base class handle always points to our derived
    // type so this dynamic cast should never fail.  but it needs to be
    // dynamic instead of static since we're casting from a virtual base
    return dynamic_cast<const Impl*>(this->handle.operator->());
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

  // Purge the cache if needed and keep recent "toKeep" items.
  static void PurgeCacheIfNeeded(size_t toKeep) {
    // Proceed only when the cache gets full.
    if (Base::cache().size() < (Base::cache().capacity()) ||
      // Don't proceed if there are no more than 1 mutable items to purge,
      // since immutable items will be purged automatically by LRU cache.
      dirtyMap.size() <= 1) {
      return;
    }
    try {
      // When there are more items to keep than cache's capacity,
      // nothing can be purged and cache's capacity needs to be increased
      // for better performance.
      if (Base::cache().capacity() < toKeep) {
        static bool warned = false;
        if (!warned) {
          notify(NFY_FATAL, "You may need to increase cache capacity for "
            "better performance: cache size %lu, cache capacity %lu, number "
            "of items requested to keep %lu", Base::cache().size(),
            Base::cache().capacity(), toKeep);
          warned = true;
        }
        return;
      }

      // Get old cache items that could be purged.
      std::vector<std::string> toDelete;
      Base::cache().GetOldKeys(toKeep, &toDelete);

      // Skip project asset versions, which should always stay in the cache.
      toDelete.erase(
          std::remove_if(toDelete.begin(), toDelete.end(),
              MutableAssetHandleD_::IsProjectAssetVersion),
          toDelete.end());

      // Save mutable items.
      khFilesTransaction filetrans(".new");
      for (std::vector<std::string>::iterator it = toDelete.begin();
           it != toDelete.end(); ++it) {
        if (dirtyMap.find(*it) != dirtyMap.end()) {
          std::string filename = dirtyMap[*it]->XMLFilename() + ".new";
          if (dirtyMap[*it]->Save(filename)) {
            filetrans.AddNewPath(filename);
          }
        }
      }
      if (!filetrans.Commit()) {
        throw khException("Unable to commit file saving in cache purge.");
      }

      // Discard saved mutable items from dirtyMap.
      size_t dirties = dirtyMap.size();
      for (std::vector<std::string>::iterator it = toDelete.begin();
           it != toDelete.end(); ++it) {
        if (dirtyMap.find(*it) != dirtyMap.end()) {
          dirtyMap.erase(*it);
        }
      }

      // Remove both immutable and mutable items from cache.
      size_t cached = Base::cache().size();
      for (std::vector<std::string>::iterator it = toDelete.begin();
           it != toDelete.end(); ++it) {
        Base::cache().Remove(*it, false);  // Do not prune for now.
      }

      // Prune the cache in the end.
      Base::cache().Prune();

      notify(NFY_INFO, "cache size %lu, dirty map %lu, "
        "assets to keep %lu, mutable assets purged %lu, total purged %lu",
        Base::cache().size(), dirtyMap.size(), toKeep,
        dirties - dirtyMap.size(), cached - Base::cache().size());
    }
    catch (const std::runtime_error& e) {
      notify(NFY_INFO, "Exception in cache purge: %s", e.what());
    }
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


  // the compiler generated assignment and copy constructor are fine for us
  // ref & handle have stable copy semantics and we don't have to worry about
  // adding to the dirtyMap because the src object will already have
  // done that
 public:
  MutableAssetHandleD_(void) : BBase(), Base() { }
  MutableAssetHandleD_(const std::string &ref_) : BBase(ref_), Base() { }

  // you should be able to create a mutable handle from the non-mutable
  // equivalent. Don't automatically bind it and add it to the dirtyMap
  // If they never use it via the MutableHandle it isn't really dirty
  MutableAssetHandleD_(const Base &o) :
      BBase(o.Ref()), Base() { }

  // you should be able to create a mutable handle from the non-mutable,
  // non-daemon equivalent. Don't automatically bind it and add it to
  // the dirtyMap. If they never use it via the MutableHandle it isn't
  // really dirty
  MutableAssetHandleD_(const BBase &o) :
      BBase(o.Ref()), Base() { }

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
      // we need to make sure it get's saved
      MutableBase::dirtyMap.insert(std::make_pair(this->ref, *this));
    }
  }

 public:
  MutableDerivedAssetHandleD_(void) :
      BBase(), BaseD(), DerivedBase(), MutableBase() { }
  MutableDerivedAssetHandleD_(const std::string &ref_) :
      BBase(ref_), BaseD(), DerivedBase(), MutableBase() { }

  // you should be able to create a mutable handle from the non-mutable
  // equivalent. Don't automatically bind it and add it to the dirtyMap
  // If they never use it via the MutableHandle it isn't really dirty
  MutableDerivedAssetHandleD_(const DerivedBase &o) :
      BBase(o.Ref()), BaseD(), DerivedBase(), MutableBase() { }


  // the compiler generated assignment and copy constructor are fine for us
  // we have no addition members or semantics to maintain

  using DerivedBase::operator->;
  Impl* operator->(void) {
    return const_cast<Impl*>(DerivedBase::operator->());
  }
};


#endif /* __AssetHandleD_h */

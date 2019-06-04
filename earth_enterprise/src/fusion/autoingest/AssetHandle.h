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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_ASSETHANDLE_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_ASSETHANDLE_H_

#include <string>
#include "common/khRefCounter.h"
#include "common/khTypes.h"
#include "common/SharedString.h"
#include "StorageManager.h"

class AssetVersionRef;

/******************************************************************************
 ***  AssetHandle templates (Private Implementation)
 ***
 ***  These templates implement the functionality for the handles to
 ***  AssetImpl's and AssetVersionImpl's.
 ***
 ***  The code should never use the various Impl classes directly, but should
 ***  always use these handle classes instead. As the trailing '_' in the
 ***  name suggests, these classes are not intended to be used
 ***  directly. Asset.h and AssetVersion.h declare typedefs to be used.
 ***
 ***  typedef AssetHandle_<AssetImpl> Asset;
 ***  typedef AssetHandle_<AssetVersionImpl> AssetVersion;
 ******************************************************************************/
template <class Impl_>
class AssetHandle_ : public AssetHandleInterface<Impl_> {
  friend class Impl;
 public:
  typedef Impl_ Impl;
  using HandleType = typename StorageManager<Impl>::HandleType;
  struct undefined_type; // never defined.  Just used for bool operations

 protected:
  static inline StorageManager<Impl> & storageManager();

  inline void DoBind(
      bool checkFileExistenceFirst,
      bool addToCache) const {
    handle = storageManager().Get(this, checkFileExistenceFirst, addToCache, isMutable());
  }

  virtual bool isMutable() const { return false; }

 public:
  static inline uint32 CacheSize(void) { return storageManager().CacheSize(); }
  static inline uint32 CacheCapacity(void) { return storageManager().CacheCapacity(); }
  static inline uint32 DirtySize(void) { return storageManager().DirtySize(); }

  virtual HandleType Load(const std::string &boundref) const {
    return HandleType(Impl::Load(boundref));
  }
  virtual bool Valid(const HandleType &) const { return true; }

  // These two functions are implemented separately by Assets and AssetVersions
  inline virtual std::string Filename() const;
  inline const SharedString Key() const;

  inline void NoLongerNeeded() {
    storageManager().NoLongerNeeded(Ref());
  }

  // Only used by Version variant.
  // Loads asset without keeping it in the storage manager
  inline void LoadAsTemporary() const {
    if (!handle) {
      DoBind(false /* don't check file */, false /* don't add to cache*/);
    }
  }

  // Adds the asset to the storage manager
  inline void MakePermanent() {
    assert(handle);
    storageManager().AddExisting(Ref(), handle);
  }

 protected:
  SharedString ref;
  mutable HandleType handle;

  inline void Bind(void) const {
    if (!handle) {
      DoBind(false /* don't check file */, true /* add to cache */);
    }
  }

 public:
  AssetHandle_(void) : ref(), handle() { }
  AssetHandle_(const std::string &ref_) : ref(ref_), handle() { }
  AssetHandle_(const SharedString &ref_) : ref(ref_), handle() { }

  // the compiler generated assignment and copy constructor are fine for us
  // ref & handle have stable copy semantics and we don't have to worry about
  // adding to the cache because the src object will already have done that.
  // Same goes for move constructor and assignment.

  virtual ~AssetHandle_(void) { }
  const SharedString & Ref(void) const { return ref; }
  bool Valid(void) const;
  // This is better than overloading the bool operator as it
  // more closely emulates what a pointer's boolean operations does.
  // For example you can't pass the class as a bool parameter but you
  // can still do other things like use it in an if statement.
  // NOTE: in C++11 we can use explicit to get same benefit without this trick
  // see: http://en.cppreference.com/w/cpp/language/explicit
  operator undefined_type *(void) const { return Valid()?reinterpret_cast<undefined_type *>(1):nullptr; }
  const Impl* operator->(void) const {
    Bind();
    return handle.operator->();
  }
  // needed so AssetHandle_ can be put in a set
  bool operator<(const AssetHandle_ &o) const {
    return ref < o.ref;
  }

  // if the ref's are equal then the cache semantics of this handle
  // guarantees that the two handles point to the same memory.
  bool operator==(const AssetHandle_ &o) const {
    return ref == o.ref;
  }
};


/******************************************************************************
 ***  Derived ReadOnly Handles
 ******************************************************************************/
template <class Base_, class Impl_>
class DerivedAssetHandle_ : public virtual Base_ {
 public:
  typedef Impl_ Impl;
  typedef Base_ Base;
  typedef typename Base::HandleType HandleType;

 public:
  virtual HandleType Load(const std::string &boundref) const {
    // Impl::Load will succeed or throw.
    // The derived khRefGuard will be automatically converted
    // the the base khRefGuard
    return HandleType(Impl::Load(boundref));
  }
  virtual bool Valid(const HandleType & entry) const { 
    // we have to check if it maps to Impl* since somebody
    // else may have loaded it into the storage manager
    return dynamic_cast<Impl*>(&*entry);
  }

  DerivedAssetHandle_(void) : Base() { }
  DerivedAssetHandle_(const std::string &ref_) : Base(ref_) { }
  DerivedAssetHandle_(const SharedString &ref_) : Base(ref_) { }

  // it's OK to construct a derived from a base, we just check first
  // and clear the handle if the types don't match
  DerivedAssetHandle_(const Base &o) : Base(o) {
    if (this->handle &&
        !dynamic_cast<const Impl*>(this->handle.operator->())) {
      this->handle = HandleType();
    }
  }

  // the compiler generated assignment and copy constructor are fine for us
  // we have no addition members or semantics to maintain
  // Same goes for move constructor and assignment.

  const Impl* operator->(void) const {
    this->Bind();
    // we ensure that the base class handle always points to our derived
    // type so this dynamic cast should never fail.  but it needs to be
    // dynamic instead of static since we're casting from a virtual base
    return dynamic_cast<const Impl*>(this->handle.operator->());
  }
};


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_ASSETHANDLE_H_

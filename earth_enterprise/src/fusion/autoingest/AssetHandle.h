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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_ASSETHANDLE_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_ASSETHANDLE_H_

#include <string>
//#include "common/khTypes.h"
#include <cstdint>
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
 *** 
 ***  IMPORTANT NOTE: The AssetHandle_ class is now considered legacy code.
 ***  New code should call Get or GetMutable on the storage manager directly,
 ***  which will return a new AssetHandle object. This class will eventually
 ***  go away.
 ******************************************************************************/
template <class Impl_>
class AssetHandle_ : public AssetHandleInterface<Impl_> {
  friend class Impl;
 public:
  typedef Impl_ Impl;
  using HandleType = typename StorageManager<Impl>::PointerType;

  static inline StorageManager<Impl> & storageManager();

 protected:
  inline void DoBind(
      bool checkFileExistenceFirst,
      bool addToCache) const {
    handle = storageManager().Get(this, ref, checkFileExistenceFirst, addToCache, isMutable());
  }

  virtual bool isMutable() const { return false; }

 public:
  static inline std::uint32_t CacheSize(void) { return storageManager().CacheSize(); }
  static inline std::uint32_t CacheCapacity(void) { return storageManager().CacheCapacity(); }
  static inline std::uint32_t DirtySize(void) { return storageManager().DirtySize(); }
  static std::uint64_t CacheMemoryUse(void) { return storageManager().CacheMemoryUse(); }

  virtual bool Valid(const HandleType &) const { return true; }

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
  explicit operator bool(void) const { return Valid(); }
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

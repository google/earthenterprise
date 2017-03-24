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
#include "common/khCache.h"
#include "common/khTypes.h"

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
class AssetHandle_  {
  friend class Impl;
 public:
  typedef Impl_ Impl;
  typedef khRefGuard<Impl> HandleType;

 protected:
  static inline khCache<std::string, HandleType>& cache(void);

 public:
  static uint32 CacheSize(void) { return cache().size(); }

  // Adds handle-object to cache.
  void CacheAdd() {
    assert(handle);
    cache().Add(Ref(), handle);
  }

  // Removes handle-object from cache.
  void CacheRemove() {
    cache().Remove(Ref());
  }

  // Only implemented/used by Version variant.
  // Loads asset version from file without caching.
  void BindNoCache() const;

 protected:
  static const bool check_timestamps;
  std::string ref;
  mutable HandleType handle;

  // Only implemented/used by Asset variant.
  void DoBind(const std::string &ref,
              bool checkFileExistenceFirst) const;

  // Only implemented/used by Version variant.
  template <int do_cache>
  void DoBind(const std::string &boundRef,
              const AssetVersionRef &boundVerRef,
              bool checkFileExistenceFirst,
              Int2Type<do_cache> do_cache_val) const;

  // Only implemented/used by Version variant.
  void DoBind(const std::string &boundRef,
              const AssetVersionRef &boundVerRef,
              bool checkFileExistenceFirst) const;

  // Implemented by both Asset & Version variants.
  void Bind(void) const;

  // Allows subclasses to do extra work.
  virtual void OnBind(const std::string &boundref) const { }

  virtual HandleType CacheFind(const std::string &boundref) const {
    HandleType entry;
    (void)cache().Find(boundref, entry);
    return entry;
  }
  virtual HandleType Load(const std::string &boundref) const {
    return HandleType(Impl::Load(boundref));
  }

 public:
  AssetHandle_(void) : ref(), handle() { }
  AssetHandle_(const std::string &ref_) : ref(ref_), handle() { }

  // the compiler generated assignment and copy constructor are fine for us
  // ref & handle have stable copy semantics and we don't have to worry about
  // adding to the cache because the src object will already have done that

  virtual ~AssetHandle_(void) { }
  std::string Ref(void) const { return ref; }
  bool Valid(void) const;
  operator bool(void) const { return Valid(); }
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

 protected:
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
  DerivedAssetHandle_(void) : Base() { }
  DerivedAssetHandle_(const std::string &ref_) : Base(ref_) { }

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

  const Impl* operator->(void) const {
    this->Bind();
    // we ensure that the base class handle always points to our derived
    // type so this dynamic cast should never fail.  but it needs to be
    // dynamic instead of static since we're casting from a virtual base
    return dynamic_cast<const Impl*>(this->handle.operator->());
  }
};


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_ASSETHANDLE_H_

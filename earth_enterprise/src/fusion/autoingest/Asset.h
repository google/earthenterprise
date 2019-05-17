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

#ifndef __Asset_h
#define __Asset_h

#include <autoingest/.idl/AssetStorage.h>
#include "AssetHandle.h"
#include <khFileUtils.h>
#include "MiscConfig.h"
#include "StorageManager.h"

/******************************************************************************
 ***  AssetImpl
 ***
 ***  Implementation for the Asset base class.
 ***
 ***  Code should NOT use this (or any other *Impl) class directly. Instead it
 ***  should use the Asset and derived Asset (BlendAsset, MosaicAsset)
 ***  handle classes.
 ***
 ***  Asset asset("projects/myvproj");
 ***  ... = asset->config.layers.size();
 ***
 ******************************************************************************/
class AssetImpl : public khRefCounter, public AssetStorage, public StorageManaged {
  friend class AssetHandle_<AssetImpl>;

  // Illegal to copy an AssetImpl
  AssetImpl(const AssetImpl&) = delete;
  AssetImpl& operator=(const AssetImpl&) = delete;
  AssetImpl(const AssetImpl&&) = delete;
  AssetImpl& operator=(const AssetImpl&&) = delete;

 protected:
  // used by my intermediate derived classes since their calls to
  // my constructor will never actualy be used
  AssetImpl(void) = default;
  AssetImpl(const AssetStorage &storage) :
      AssetStorage(storage) { }

 public:
  // implemented in LoadAny.cpp
  static khRefGuard<AssetImpl> Load(const std::string &boundref);

  virtual bool Save(const std::string &filename) const {
    assert(false); // Can only save from sub-classes
    return false;
  };

  std::string WorkingDir(void) const { return WorkingDir(GetRef()); }
  std::string XMLFilename() const { return XMLFilename(GetRef()); }


  virtual ~AssetImpl(void) { }
  const SharedString & GetRef(void) const { return name; }


  std::string  GetLastGoodVersionRef(void) const;
  void GetInputFilenames(std::vector<std::string> &out) const;
  virtual void AfterLoad(void) { }

  // static helpers
  static std::string WorkingDir(const std::string &ref);
  static std::string XMLFilename(const std::string &ref);
};

// ****************************************************************************
// ***  Asset & its specializations
// ****************************************************************************
typedef AssetHandle_<AssetImpl> Asset;

template <>
inline StorageManager<AssetImpl>&
Asset::storageManager(void)
{
  static StorageManager<AssetImpl> storageManager(
      MiscConfig::Instance().AssetCacheSize, "asset");
  return storageManager;
}

template <>
inline bool
Asset::Valid(void) const
{
  if (handle) {
    return handle->type != AssetDefs::Invalid;
  } else {
    // deal quickly with an empty ref
    if (ref.empty())
      return false;

    try {
      DoBind(true /* check file & maybe not throw */, true /* add to cache */);
    } catch (...) {
      return false;
    }
    return handle && (handle->type != AssetDefs::Invalid);
  }
}

template <>
inline std::string Asset::Filename() const {
  return AssetImpl::XMLFilename(ref);
}

template <> inline const SharedString Asset::Key() const { return ref; }


#endif /* __Asset_h */

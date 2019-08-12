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
#include "CacheSizeCalculations.h"

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
class AssetImpl : public AssetStorage, public StorageManaged {
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
  static std::shared_ptr<AssetImpl> Load(const std::string &boundref);

  virtual bool Save(const std::string &filename) const {
    assert(false); // Can only save from sub-classes
    return false;
  }

  std::string WorkingDir(void) const { return WorkingDir(GetRef()); }
  std::string XMLFilename() const { return XMLFilename(GetRef()); }


  virtual ~AssetImpl(void) { }
  const SharedString & GetRef(void) const { return name; }

  // determine amount of memory used by an AssetImpl
  uint64 GetSize() {
    return(GetObjectSize(name)
    + GetObjectSize(type)
    + GetObjectSize(subtype)
    + GetObjectSize(inputs)
    + meta.GetSize()
    + GetObjectSize(versions)
    + GetObjectSize(timestamp)
    + GetObjectSize(filesize));
  }

  std::string  GetLastGoodVersionRef(void) const;
  void GetInputFilenames(std::vector<std::string> &out) const;
  virtual void AfterLoad(void) { }

  // static helpers
  static std::string WorkingDir(const std::string &ref);
  static std::string XMLFilename(const std::string &ref);
  static std::string Filename(const std::string &ref) {
    return XMLFilename(ref);
  }
  static SharedString Key(const SharedString & ref) {
    return ref;
  }
  static bool ValidRef(const SharedString & ref) {
    return !ref.empty();
  }
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
      MiscConfig::Instance().AssetCacheSize, MiscConfig::Instance().LimitMemoryUtilization, MiscConfig::Instance().MaxAssetCacheMemorySize, "asset");
  return storageManager;
}

template <>
inline bool
Asset::Valid(void) const
{
  if (handle) {
    return handle->type != AssetDefs::Invalid;
  } else {

    if (!AssetImpl::ValidRef(ref))
      return false;

    try {
      DoBind(true /* check file & maybe not throw */, true /* add to cache */);
    } catch (...) {
      return false;
    }
    return handle && (handle->type != AssetDefs::Invalid);
  }
}

template<class AssetType>
AssetType Find(const std::string & ref, const AssetDefs::Type & type)
{
  try {
    AssetType asset(ref);
    notify(NFY_WARN, "Find: %s\t%s", ToString(asset->type).c_str(), asset->subtype.c_str());
    if (asset &&
        (asset->type == type) &&
        (asset->subtype == AssetType::GetSubtype())) {
        return AssetType(ref);
    }
  } catch (...) {
      // do nothing - don't even generate any warnings
  }
  return AssetType();
}

template<class VersionType>
VersionType FindVersion(const std::string & ref, const AssetDefs::Type & type)
{
    notify(NFY_WARN, "Version: %s\t%s\t%s", ref.c_str(), ToString(type).c_str(), VersionType::GetSubtype().c_str());
    try {
        VersionType version(ref);
        notify(NFY_WARN, "Version: %s\t%s", ToString(version->type).c_str(), version->subtype.c_str());
        if (version &&
            (version->type == type) &&
            (version->subtype == VersionType::GetSubtype())) {
            return VersionType(ref);
        }
    } catch (...) {
        // do nothing - don't even generate any warnings
    }
    return VersionType();
}

template<class VersionType>
void ValidateRefForInput(const std::string & ref, const AssetDefs::Type & type)
{
    using AssetType = typename VersionType::Impl::AssetType;
    notify(NFY_WARN, "Validate: %s\t%s\t%s", ref.c_str(), ToString(type).c_str(), VersionType::GetSubtype().c_str());
    if (AssetVersionRef(ref).Version() == "current") {
        AssetType asset = Find<AssetType>(ref, type);
        if (!asset) {
            throw std::invalid_argument(
                "No such " + ToString(type) + " " + VersionType::GetSubtype() + " asset: " + ref);
        }
    } else {
        VersionType version = FindVersion<VersionType>(ref, type);
        if (!version) {
            throw std::invalid_argument(
                "No such " + ToString(type) + " " + VersionType::GetSubtype() + " asset version: " +
                ref);
        }
    }
}

#endif /* __Asset_h */

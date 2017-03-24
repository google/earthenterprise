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
class AssetImpl : public khRefCounter, public AssetStorage {
  friend class AssetHandle_<AssetImpl>;

  // private and unimplemented -- illegal to copy an AssetImpl
  AssetImpl(const AssetImpl&);
  AssetImpl& operator=(const AssetImpl&);

 protected:
  // implemented in LoadAny.cpp
  static khRefGuard<AssetImpl> Load(const std::string &boundref);

  // used by my intermediate derived classes since their calls to
  // my constructor will never actualy be used
  AssetImpl(void) : timestamp(0), filesize(0) { }
  AssetImpl(const AssetStorage &storage) :
      AssetStorage(storage), timestamp(0), filesize(0) { }

 public:
  // use by cache - maintained outside of constructors
  time_t timestamp;
  uint64 filesize;

  std::string WorkingDir(void) const { return WorkingDir(GetRef()); }
  std::string XMLFilename() const { return XMLFilename(GetRef()); }


  virtual ~AssetImpl(void) { }
  std::string GetRef(void) const { return name; }


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
inline khCache<std::string, Asset::HandleType>&
Asset::cache(void)
{
  static khCache<std::string, Asset::HandleType>
    instance(MiscConfig::Instance().AssetCacheSize);
  return instance;
}

template <>
inline void
Asset::DoBind(const std::string &ref, bool checkFileExistenceFirst) const
{
  // Check in cache
  HandleType entry = CacheFind(ref);
  bool addToCache = false;

  // Try to load from XML
  if (!entry) {
    if (checkFileExistenceFirst) {
      std::string filename = Impl::XMLFilename(ref);
      if (!khExists(Impl::XMLFilename(ref))) {
        // in this case DoBind is allowed not to throw even if
        // we configured to normally throw
        return;
      }
    }

    // will succeed, generate stub, or throw exception
    entry = Load(ref);
    addToCache = true;
  } else if (check_timestamps) {
    std::string filename = Impl::XMLFilename(ref);
    uint64 filesize = 0;
    time_t timestamp = 0;
    if (khGetFileInfo(filename, filesize, timestamp) &&
        ((timestamp != entry->timestamp) ||
         (filesize != entry->filesize))) {
      // the file has changed on disk

      // drop the current entry from the cache
      cache().Remove(ref, false); // don't prune, the Add will

      // will succeed, generate stub, or throw exception
      entry = Load(ref);
      addToCache = true;
    }
  }

  // set my handle
  handle = entry;

  // add it to the cache
  if (addToCache)
    cache().Add(ref, entry);

  // used by derived class to mark dirty
  OnBind(ref);
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
      DoBind(ref, true /* check file & maybe not throw */);
    } catch (...) {
      return false;
    }
    return handle && (handle->type != AssetDefs::Invalid);
  }
}


template <>
inline void
Asset::Bind(void) const
{
  if (!handle) {
    DoBind(ref, false);
  }
}

#endif /* __Asset_h */

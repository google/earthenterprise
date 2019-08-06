/*
 * Copyright 2019 The Open GEE Contributors
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

#ifndef STORAGEMANAGER_H
#define STORAGEMANAGER_H

#include <map>
#include <string>
#include <time.h>
#include <vector>

#include "common/khCache.h"
#include "common/khFileUtils.h"
#include "common/notify.h"
#include "common/SharedString.h"
#include "StorageManagerAssetHandle.h"

using AssetKey = SharedString;

// Items stored in the storage manager must inherit from the StorageManaged class
class StorageManaged {
  public:
    time_t timestamp;
    uint64 filesize;
    StorageManaged() : timestamp(0), filesize(0) {}
};

// Handles to items stored in the storage manager must implement the asset
// handle interface. This is only used for the legacy Get function.
template<class AssetType>
class AssetHandleInterface {
  public:
    virtual AssetPointerType<AssetType> Load(const std::string &) const = 0;
    virtual bool Valid(const AssetPointerType<AssetType> &) const = 0;
};


template<class AssetType>
class StorageManagerInterface {
  public:
    using PointerType = AssetPointerType<AssetType>;
    virtual AssetHandle<const AssetType> Get(const AssetKey &) = 0;
    virtual AssetHandle<AssetType> GetMutable(const AssetKey &) = 0;
    virtual ~StorageManagerInterface() {}
};

template<class AssetType>
class StorageManager : public StorageManagerInterface<AssetType> {
  public:
    using Base = StorageManagerInterface<AssetType>;
    using PointerType = typename Base::PointerType;

    StorageManager(uint cacheSize, bool limitByMemory, uint64 maxMemory, const std::string & type) :
        cache(cacheSize),
        assetType(type) { SetCacheMemoryLimit(limitByMemory, maxMemory); }
    ~StorageManager() = default;

    inline uint32 CacheSize() const { return cache.size(); }
    inline uint32 CacheCapacity() const { return cache.capacity(); }
    inline uint32 DirtySize() const { return dirtyMap.size(); }
    inline uint64 CacheMemoryUse() const { return cache.getMemoryUse(); }
    inline void SetCacheMemoryLimit(bool limitByMemory, uint64 maxMemory) { cache.setCacheMemoryLimit(limitByMemory, maxMemory); }
    inline void UpdateCacheItemSize(const AssetKey & key) { cache.updateCacheItemSize(key); }
    inline uint64 GetCacheItemSize(const AssetKey & key) { return cache.getCacheItemSize(key); }
    inline void AddNew(const AssetKey &, const PointerType &);
    inline void AddExisting(const AssetKey &, const PointerType &);
    inline void NoLongerNeeded(const AssetKey &, bool = true);
    void Abort();
    bool SaveDirtyToDotNew(khFilesTransaction &, std::vector<AssetKey> *);
    PointerType Get(const AssetHandleInterface<AssetType> *, const AssetKey &, bool, bool, bool);
    
    // Pass a handle to a const to prevent callers from modifying it.
    AssetHandle<const AssetType> Get(const AssetKey &);
    
    // Pass a handle to a non-const so callers can modify it.
    AssetHandle<AssetType> GetMutable(const AssetKey &);

    //template<class Asset>
    //static Asset Find(const std::string &, const AssetDefs::Type &, const std::string &);
  private:
    using CacheType = khCache<AssetKey, PointerType>;

    static const bool check_timestamps;

    CacheType cache;
    std::map<AssetKey, PointerType> dirtyMap;
    std::string assetType;

    StorageManager(const StorageManager &) = delete;
    StorageManager& operator=(const StorageManager &) = delete;
    
    PointerType GetEntryFromCacheOrDisk(const AssetKey &);
};

template<class AssetType>
inline void
StorageManager<AssetType>::AddNew(const AssetKey & key, const PointerType & value) {
  cache.Add(key, value);
  // New assets are automatically dirty
  dirtyMap.emplace(key, value);
}

template<class AssetType>
inline void
StorageManager<AssetType>::AddExisting(const AssetKey & key, const PointerType & value) {
  cache.Add(key, value);
}

template<class AssetType>
inline void
StorageManager<AssetType>::NoLongerNeeded(const AssetKey & key, bool prune) {
  cache.Remove(key, prune);
}

// This is the "legacy" Get function used by the AssetHandle_ class (see
// AssetHandle.h). New code should use the other Get function or the
// GetMutable function as appropriate, which return AssetHandle objects,
// which are defined in StorageManagerAssetHandle.h. Evetually this function
// should go away.
template<class AssetType>
typename StorageManager<AssetType>::PointerType
StorageManager<AssetType>::Get(
    const AssetHandleInterface<AssetType> * handle,
    const AssetKey & ref,
    bool checkFileExistenceFirst,
    bool addToCache,
    bool makeMutable) {
  const AssetKey key = AssetType::Key(ref);
  const std::string filename = AssetType::Filename(key);

  // Check in cache.
  PointerType entry;
  cache.Find(key, entry);
  if (entry && !handle->Valid(entry)) entry = PointerType();
  bool updated = false;

  // Try to load from XML.
  if (!entry) {
    if (checkFileExistenceFirst) {
      if (!khExists(filename)) {
        // In this case DoBind is allowed not to throw even if
        // we configured to normally throw.
        return PointerType();
      }
    }

    // Will succeed, generate stub, or throw exception.
    entry = handle->Load(key);
    updated = true;
  } else if (check_timestamps) {
    uint64 filesize = 0;
    time_t timestamp = 0;
    if (khGetFileInfo(filename, filesize, timestamp) &&
        ((timestamp != entry->timestamp) ||
         (filesize != entry->filesize))) {
      // The file has changed on disk.

      // Drop the current entry from the cache.
      cache.Remove(key, false);  // Don't prune, the Add() will.

      // Will succeed, generate stub, or throw exception.
      entry = handle->Load(key);
      updated = true;
    }
  }

  if (entry) {
    // Add it to the cache.
    if (addToCache && updated)
      cache.Add(key, entry);

    // Add it to the dirty map. If it's already in the dirty map the existing
    // one will win; that's OK.
    if (makeMutable)
      dirtyMap.emplace(key, entry);
  }

  return entry;
}

template<class AssetType>
typename StorageManager<AssetType>::PointerType
StorageManager<AssetType>::GetEntryFromCacheOrDisk(const AssetKey & ref) {
  AssetKey key = AssetType::Key(ref);

  // Deal quickly with an invalid key
  if (!AssetType::ValidRef(key)) return PointerType();

  const std::string filename = AssetType::Filename(key);

  // Check in cache.
  PointerType entry;
  cache.Find(key, entry);
  bool updated = false;

  // Try to load from XML.
  if (!entry) {
    // Avoid throwing exceptions when the file doesn't exist
    if (!khExists(filename)) return PointerType();
    // Will succeed, generate stub, or throw exception.
    entry = AssetType::Load(key);
    updated = true;
  } else if (check_timestamps) {
    uint64 filesize = 0;
    time_t timestamp = 0;
    if (khGetFileInfo(filename, filesize, timestamp) &&
        ((timestamp != entry->timestamp) ||
         (filesize != entry->filesize))) {
      // The file has changed on disk.

      // Drop the current entry from the cache.
      cache.Remove(key, false);  // Don't prune, the Add() will.

      // Will succeed, generate stub, or throw exception.
      entry = AssetType::Load(key);
      updated = true;
    }
  }

  if (entry && updated) {
    // Add it to the cache.
    cache.Add(key, entry);
  }

  return entry;
}

template<class AssetType>
AssetHandle<const AssetType> StorageManager<AssetType>::Get(const AssetKey & ref) {
  PointerType entry = GetEntryFromCacheOrDisk(ref);
  return AssetHandle<const AssetType>(std::shared_ptr<const AssetType>(entry), nullptr);
}

template<class AssetType>
AssetHandle<AssetType> StorageManager<AssetType>::GetMutable(const AssetKey & ref) {
  PointerType entry = GetEntryFromCacheOrDisk(ref);
  // Add it to the dirty map. If it's already in the dirty map the existing
  // one will win; that's OK.
  dirtyMap.emplace(ref, entry);
  return AssetHandle<AssetType>(entry, [=]() {
    UpdateCacheItemSize(AssetType::Key(ref));
  });
}

template<class AssetType>
void StorageManager<AssetType>::Abort() {
  // remove all the dirty Impls from the cache
  for (const std::pair<AssetKey, PointerType> & entry : dirtyMap) {
    cache.Remove(entry.first, false); // false -> don't prune
  }
  cache.Prune();  // prune at the end to avoid possible prune thrashing

  // now clear the dirtyMap itself
  dirtyMap.clear();
}

template<class AssetType>
bool StorageManager<AssetType>::SaveDirtyToDotNew(
    khFilesTransaction &savetrans,
    std::vector<AssetKey> *saved) {
  notify(NFY_INFO, "Writing %lu %s records", dirtyMap.size(), assetType.c_str());
  typename std::map<AssetKey, PointerType>::iterator entry = dirtyMap.begin();
  while (entry != dirtyMap.end()) {
    std::string filename = entry->second->XMLFilename() + ".new";
    if (entry->second->Save(filename)) {
      savetrans.AddNewPath(filename);
      if (saved) {
        saved->push_back(entry->first);
      }
      // Remove the element and advance the pointer
      entry = dirtyMap.erase(entry);
    } else {
      return false;
    }
  }
  return true;
}

template<class Asset>
Asset Find(const std::string & ref, const AssetDefs::Type & type, const std::string & subtype)
{
  notify(NFY_WARN, "Find: %s\t%s\t%s", ref.c_str(), ToString(type).c_str(), subtype.c_str());
  try {
    Asset asset(ref);
    notify(NFY_WARN, "Find: %s\t%s", ToString(asset->type).c_str(), asset->subtype.c_str());
    if (asset &&
        (asset->type == type) &&
        (asset->subtype == subtype)) {
        return Asset(SharedString(ref));
    }
  } catch (...) {
      // do nothing - don't even generate any warnings
  }
  return Asset();
}

template<class Version>
Version FindVersion(const std::string & ref, const AssetDefs::Type & type, const std::string & subtype)
{
    notify(NFY_WARN, "Version: %s\t%s\t%s", ref.c_str(), ToString(type).c_str(), subtype.c_str());
    try {
        Version version(ref);
        notify(NFY_WARN, "Version: %s\t%s", ToString(version->type).c_str(), version->subtype.c_str());
        if (version &&
            (version->type == type) &&
            (version->subtype == subtype)) {
            return Version(SharedString(ref));
        }
    } catch (...) {
        // do nothing - don't even generate any warnings
    }
    return Version();
}

template<class Asset, class Version>
void ValidateRefForInput(const std::string & ref, const AssetDefs::Type & type, const std::string & subtype)
{
    notify(NFY_WARN, "Validate: %s\t%s\t%s", ref.c_str(), ToString(type).c_str(), subtype.c_str());
    if (AssetVersionRef(ref).Version() == "current") {
        Asset asset = Find<Asset>(ref, type, subtype);
        if (!asset) {
            throw std::invalid_argument(
                "No such " + ToString(type) + " " + subtype + " asset: " + ref);
        }
    } else {
        Version version = FindVersion<Version>(ref, type, subtype);
        if (!version) {
            throw std::invalid_argument(
                "No such " + ToString(type) + " " + subtype + " asset version: " +
                ref);
        }
    }
}

#endif // STORAGEMANAGER_H


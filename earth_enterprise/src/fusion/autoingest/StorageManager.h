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
#include "common/khRefCounter.h"
#include "common/notify.h"
#include "common/SharedString.h"

// Items stored in the storage manager must inherit from the StorageManaged class
class StorageManaged {
  public:
    time_t timestamp;
    uint64 filesize;
    StorageManaged() : timestamp(0), filesize(0) {}
};

template<class AssetType> class AssetHandleInterface;

template<class AssetType>
class StorageManager
{
  public:
    using HandleType = khRefGuard<AssetType>;
    using AssetKey = SharedString;

    StorageManager(uint cacheSize, const std::string & type) :
        cache(cacheSize),
        assetType(type) {}
    ~StorageManager() = default;

    inline uint32 CacheSize() const { return cache.size(); }
    inline uint32 CacheCapacity() const { return cache.capacity(); }
    inline uint32 DirtySize() const { return dirtyMap.size(); }
    inline void AddNew(const AssetKey &, const HandleType &);
    inline void AddExisting(const AssetKey &, const HandleType &);
    inline void NoLongerNeeded(const AssetKey &, bool = true);
    void Abort();
    bool SaveDirtyToDotNew(khFilesTransaction &, std::vector<std::string> *);
    HandleType Get(const AssetHandleInterface<AssetType> *, bool, bool, bool);
  private:
    using CacheType = khCache<std::string, HandleType>;

    static const bool check_timestamps;

    CacheType cache;
    std::map<AssetKey, HandleType> dirtyMap;
    std::string assetType;

    StorageManager(const StorageManager &) = delete;
    StorageManager& operator=(const StorageManager &) = delete;
};

// Handles to items stored in the storage manager must implement the asset handle interface
template<class AssetType>
class AssetHandleInterface {
  public:
    virtual const typename StorageManager<AssetType>::AssetKey Key() const = 0;
    virtual std::string Filename() const = 0;
    virtual typename StorageManager<AssetType>::HandleType Load(const std::string &) const = 0;
    virtual bool Valid(const typename StorageManager<AssetType>::HandleType &) const = 0;
};

template<class AssetType>
inline void
StorageManager<AssetType>::AddNew(const AssetKey & key, const HandleType & value) {
  cache.Add(key, value);
  // New assets are automatically dirty
  dirtyMap.emplace(key, value);
}

template<class AssetType>
inline void
StorageManager<AssetType>::AddExisting(const AssetKey & key, const HandleType & value) {
  cache.Add(key, value);
}

template<class AssetType>
inline void
StorageManager<AssetType>::NoLongerNeeded(const AssetKey & key, bool prune) {
  cache.Remove(key, prune);
}

template<class AssetType>
typename StorageManager<AssetType>::HandleType
StorageManager<AssetType>::Get(
    const AssetHandleInterface<AssetType> * handle,
    bool checkFileExistenceFirst,
    bool addToCache,
    bool makeMutable) {
  const AssetKey key = handle->Key();
  const std::string filename = handle->Filename();

  // Check in cache.
  HandleType entry;
  cache.Find(key, entry);
  if (entry && !handle->Valid(entry)) entry = HandleType();
  bool updated = false;

  // Try to load from XML.
  if (!entry) {
    if (checkFileExistenceFirst) {
      if (!khExists(filename)) {
        // In this case DoBind is allowed not to throw even if
        // we configured to normally throw.
        return HandleType();
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
void StorageManager<AssetType>::Abort() {
  // remove all the dirty Impls from the cache
  for (const std::pair<AssetKey, HandleType> & entry : dirtyMap) {
    cache.Remove(entry.first, false); // false -> don't prune
  }
  cache.Prune();  // prune at the end to avoid possible prune thrashing

  // now clear the dirtyMap itself
  dirtyMap.clear();
}

template<class AssetType>
bool StorageManager<AssetType>::SaveDirtyToDotNew(
    khFilesTransaction &savetrans,
    std::vector<std::string> *saved) {
  notify(NFY_INFO, "Writing %lu %s records", dirtyMap.size(), assetType.c_str());
  typename std::map<AssetKey, HandleType>::iterator entry = dirtyMap.begin();
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

#endif // STORAGEMANAGER_H


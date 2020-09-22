// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include "server/mod_fdb/unpackermanager.h"
#include <map>
#include <string>
#include <utility>
#include "common/geFilePool.h"
//#include "common/khTypes.h"
#include <cstdint>
#include "fusion/portableglobe/servers/fileunpacker/shared/glc_reader.h"
#include "fusion/portableglobe/servers/fileunpacker/shared/glc_unpacker.h"
#include "server/mod_fdb/apache_fdb_reader.h"

#ifdef APLOG_USE_MODULE
APLOG_USE_MODULE(fdb);
#endif

UnpackerManager::UnpackerManager() : file_pool_(-50) {
}

UnpackerManager::~UnpackerManager() {
  Reset();
}

// Gets glx to targets map of all portables with an associated unpacker.
// Used for checking the status of the UnpackerManager.
void UnpackerManager::GetPortableToTargetsMap(
    std::map<std::string, std::vector<std::string> >* portable_to_targets)
    const {
  portable_to_targets->clear();

  khLockGuard lock(mutex);
  TargetToPortableMap::const_iterator it = target_to_portable_map_.begin();
  for (; it != target_to_portable_map_.end(); ++it) {
    (*portable_to_targets)[it->second].push_back(it->first);
  }
}

// Unregisters portable for serving on specified target path.
// Deletes Reader/Unpacker and clears glx from set of registered portables
// for specific target path.
bool UnpackerManager::UnregisterPortable(const std::string& target_path) {
  khLockGuard lock(mutex);
  TargetToPortableMap::iterator it_portable =
      target_to_portable_map_.find(target_path);
  if (it_portable != target_to_portable_map_.end()) {
    // Unregister database.
    target_to_portable_map_.erase(it_portable);

    // Delete Reader/Unpacker.
    TargetToUnpackerMap::iterator it_unpacker =
        target_to_unpacker_map_.find(target_path);
    if (it_unpacker != target_to_unpacker_map_.end()) {
      if (it_unpacker->second.first) {
        delete it_unpacker->second.first;
      }
      if (it_unpacker->second.second) {
        delete it_unpacker->second.second;
      }
      target_to_unpacker_map_.erase(it_unpacker);
    }
    return true;
  }
  return false;
}

// Registers glx for serving.
void UnpackerManager::RegisterPortable(
    const std::string& target_path,
    const std::string& globe_path,
    request_rec* r) {
  if (UnregisterPortable(target_path)) {
    // Warning because normal use of API requires unpublishing before
    // publishing to the same target path.
    ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                  "Existing glx for target path %s have been unregistered.",
                  target_path.c_str());
  }

  khLockGuard lock(mutex);
  // Register glx for serving.
  target_to_portable_map_[target_path] = globe_path;
}

// Gets Reader/Unpacker for the given target if glx is registered
// for serving.
// Note: Creates reader/unpacker if they don't exist.
UnpackerPair UnpackerManager::GetUnpacker(const std::string& target_path) {
  khLockGuard lock(mutex);
  // Check if unpacker already exists and return it.
  TargetToUnpackerMap::iterator it_unpacker =
      target_to_unpacker_map_.find(target_path);

  if (it_unpacker != target_to_unpacker_map_.end())
    return it_unpacker->second;

  // Create unpacker if target path is registered for serving.
  TargetToPortableMap::const_iterator it_data =
      target_to_portable_map_.find(target_path);
  if (it_data != target_to_portable_map_.end())
    return CreateUnpacker(target_path, it_data->second);

  // Target path is not registered for serving.
  return  UnpackerPair(NULL, NULL);
}

// Resets manager: deletes all readers/unpackers and cleans internal structures.
void UnpackerManager::Reset() {
  khLockGuard lock(mutex);
  TargetToUnpackerMap::iterator it = target_to_unpacker_map_.begin();
  for (; it != target_to_unpacker_map_.end(); ++it) {
    // Delete reader and unpacker instances.
    if (it->second.first) {
      delete it->second.first;
    }
    if (it->second.second) {
      delete it->second.second;
    }
    it->second.first = 0;
    it->second.second = 0;
  }
  target_to_unpacker_map_.clear();
  target_to_portable_map_.clear();
}

// Creates a new reader/unpacker for specified target path and glx.
UnpackerPair UnpackerManager::CreateUnpacker(
    const std::string& target_path,
    const std::string& globe_path) {
  // Create unpacker pair.
  ApacheFdbReader* reader = new ApacheFdbReader(globe_path, file_pool_);
  std::string suffix = globe_path.substr(globe_path.size() - 3);
  GlcUnpacker* unpacker = new GlcUnpacker(*reader, suffix == "glc");
  UnpackerPair pair(reader, unpacker);
  std::pair<TargetToUnpackerMap::iterator, bool> ret =
      target_to_unpacker_map_.insert(TargetToUnpackerMap::value_type(
          target_path, pair));
  assert(ret.second);
  return pair;
}

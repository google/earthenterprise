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

// Class for managing a map of Fdb readers.
// We create Fdb reader for every publish target path while publishing and
// delete it while unpublishing.
#ifndef GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_FDB_READER_MANAGER_H_
#define GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_FDB_READER_MANAGER_H_

#include <map>
#include <string>
#include <vector>
#include "common/geFilePool.h"
//#include "common/khTypes.h"
#include <cstdint>

// Third party http_request.h
class request_rec;

class ServerdbReader;

// Manages the list of ServerdbReaders.
class FdbReaderManager {
  class ServerdbReaderData {
   public:
    ServerdbReaderData() {}
    ServerdbReaderData(const std::string& _db_path, int _num_cache_levels):
        db_path(_db_path),
        num_cache_levels(_num_cache_levels) {
    }
    std::string db_path;
    int num_cache_levels;
  };

  // TODO: consider to use hash_map for FdbMap, since it is used
  // while serving.
  typedef std::map<std::string, ServerdbReader*> TargetToReaderMap;
  typedef std::map<std::string, ServerdbReaderData> TargetToReaderDataMap;

 public:
  FdbReaderManager();
  ~FdbReaderManager();

  // Returns whether cutting is enabled.
  bool GetCuttingEnabled() const {
    return is_cutting_enabled_;
  }

  // Sets whether cutting is enabled and propagates to all existing Readers.
  void SetCuttingEnabled(bool enabled);

  // Registers Fusion database for serving: associates target path with database
  // path and number of cache levels.
  // The num_cache_levels is a number of cache levels for the virtual host.
  void RegisterFusionDb(const std::string& target_path,
                        const std::string& db_path,
                        int num_cache_levels,
                        request_rec*);

  // Returns reader for given target path. It is used while serving.
  ServerdbReader* GetServerdbReader(const std::string& target_path);

  // Gets database to targets associations. The fusion_db_to_targets is a map
  // in which database to targets associations are added.
  void GetFusionDbToTargetsMap(
      std::map<std::string, std::vector<std::string> >* fusion_db_to_targets)
      const;

  // Gets the path to the Fusion database associated with the given target.
  std::string GetFusionDbPath(const std::string& target_path) const {
    khLockGuard lock(mutex);
    TargetToReaderDataMap::const_iterator it_path =
        target_to_reader_data_map_.find(target_path);

    if (it_path != target_to_reader_data_map_.end())
      return it_path->second.db_path;

    return std::string();
  }

  // Unregisters Fusion database for serving on specified target path.
  // Returns whether database have been unregistered.
  bool UnregisterFusionDb(const std::string& target_path);

  // Resets FdbReaderManager: deletes all Fusion DB readers and clears internal
  // structures.
  void Reset();

 private:
  ServerdbReader* CreateServerdbReader(const std::string& target_path,
                                       const std::string& db_path,
                                       int num_cache_levels);

  bool is_cutting_enabled_;
  // Publish target path to Reader map.
  TargetToReaderMap target_to_reader_map_;
  // Publish target path to database path map.
  TargetToReaderDataMap target_to_reader_data_map_;
  geFilePool file_pool_;
  mutable khMutex mutex;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_FDB_READER_MANAGER_H_

// Copyright 2017 Google Inc.
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


#include "server/mod_fdb/fdb_reader_manager.h"

#include <http_request.h>
#include <http_log.h>
#include "common/serverdb/serverdbReader.h"

#ifdef APLOG_USE_MODULE
APLOG_USE_MODULE(fdb);
#endif

FdbReaderManager::FdbReaderManager() :
    is_cutting_enabled_(false),
    file_pool_(-50) {
}

FdbReaderManager::~FdbReaderManager() {
  Reset();
}

// Sets whether cutting is enabled to a manager object and propagates to all
// existing Readers.
void FdbReaderManager::SetCuttingEnabled(bool enabled) {
  khLockGuard lock(mutex);
  is_cutting_enabled_ = enabled;
  TargetToReaderMap::iterator it_reader = target_to_reader_map_.begin();
  for (; it_reader != target_to_reader_map_.end(); ++it_reader) {
    it_reader->second->SetCuttingEnabled(is_cutting_enabled_);
  }
}

// Registers Fusion database for serving on specified target path.
void FdbReaderManager::RegisterFusionDb(const std::string& target_path,
                                        const std::string& db_path,
                                        int num_cache_levels,
                                        request_rec* r) {
  if (UnregisterFusionDb(target_path)) {
    // Warning because normal use of API requires unpublishing before
    // publishing to the same target path.
    ap_log_rerror(
        APLOG_MARK, APLOG_WARNING, 0, r,
        "Existing database for target path %s have been unregistered.",
        target_path.c_str());
  }

  khLockGuard lock(mutex);
  // Register database for serving.
  target_to_reader_data_map_[target_path] =
      ServerdbReaderData(db_path, num_cache_levels);
}

// Gets reader for specific publish target path.
// Note: Creates reader if it doesn't exist in case of target path is registered
// for serving.
ServerdbReader* FdbReaderManager::GetServerdbReader(
    const std::string& target_path) {
  khLockGuard lock(mutex);
  // Check if reader already exists and return it.
  TargetToReaderMap::iterator it_reader =
      target_to_reader_map_.find(target_path);
  if (it_reader != target_to_reader_map_.end())
    return it_reader->second;

  // Create reader if target path is registered for serving.
  TargetToReaderDataMap::const_iterator it_data =
      target_to_reader_data_map_.find(target_path);
  if (it_data != target_to_reader_data_map_.end()) {
    return CreateServerdbReader(target_path,
                                it_data->second.db_path,
                                it_data->second.num_cache_levels);
  }

  // Target path is not registered for serving.
  return NULL;
}

// Gets Fusion database to publish target paths associations. Used for checking
// the status of the Fdb service.
void FdbReaderManager::GetFusionDbToTargetsMap(
    std::map<std::string, std::vector<std::string> >* fusion_db_to_targets)
    const {
  fusion_db_to_targets->clear();

  khLockGuard lock(mutex);
  TargetToReaderDataMap::const_iterator it = target_to_reader_data_map_.begin();
  for (; it != target_to_reader_data_map_.end(); ++it) {
    (*fusion_db_to_targets)[it->second.db_path].push_back(it->first);
  }
}

// Unregisters Fusion database for serving and deletes reader from set of
// pre-loaded readers for specific target path.
bool FdbReaderManager::UnregisterFusionDb(const std::string& target_path) {
  khLockGuard lock(mutex);
  TargetToReaderDataMap::iterator it_data =
      target_to_reader_data_map_.find(target_path);
  if (it_data != target_to_reader_data_map_.end()) {
    // Unregister database.
    target_to_reader_data_map_.erase(it_data);
    // Delete reader.
    TargetToReaderMap::iterator it_reader =
        target_to_reader_map_.find(target_path);
    if (it_reader != target_to_reader_map_.end()) {
      if (it_reader->second) {
        delete it_reader->second;
      }
      target_to_reader_map_.erase(it_reader);
    }
    return true;
  }
  return false;
}

// Resets FdbReaderManager: deletes all Fusion DB reader instances and clears
// internal structures.
void FdbReaderManager::Reset() {
  khLockGuard lock(mutex);
  TargetToReaderMap::iterator it = target_to_reader_map_.begin();
  for (; it != target_to_reader_map_.end(); ++it) {
    if (it->second) {
      delete it->second;
    }
  }
  target_to_reader_map_.clear();
  target_to_reader_data_map_.clear();
}

// Creates Reader for database and associates it with target path.
ServerdbReader* FdbReaderManager::CreateServerdbReader(
    const std::string& target_path,
    const std::string& db_path,
    int num_cache_levels) {
  ServerdbReader* reader =
      new ServerdbReader(file_pool_, db_path, num_cache_levels,
                         is_cutting_enabled_);
  std::pair<TargetToReaderMap::iterator, bool> ret =
      target_to_reader_map_.insert(TargetToReaderMap::value_type(
          target_path, reader));
  assert(ret.second);
  return reader;
}

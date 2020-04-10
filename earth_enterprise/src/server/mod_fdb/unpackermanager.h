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

// Class for managing a map of glc unpackers.
// We are assuming that there will be an unpacker for each
// globe (glc, glb, or glm) and that there will never be a reason
// to try to clear out the map. On the other hand, we don't go out
// of our way to waste space so the unpackers are only created
// when a globe is first accessed. Globes that are in the globes
// directory but are never accessed will not have an unpacker
// associated with them. The map can be cleared by restarting
// the server.
#ifndef GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_UNPACKERMANAGER_H_
#define GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_UNPACKERMANAGER_H_

#include <ap_compat.h>
#include <apr_date.h>
#include <http_request.h>
#include <http_log.h>
#include <http_protocol.h>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "common/geFilePool.h"
#include <cstdint>

// fusion/portableglobe/servers/fileunpacker/shared/glc_unpacker.h
class GlcUnpacker;

// server/mod_glc/apache_glc_reader.h
class ApacheFdbReader;

typedef std::pair<ApacheFdbReader*, GlcUnpacker*> UnpackerPair;


class UnpackerManager {
  // TODO: consider to use hash_map for UnpackerMap, since
  // it is used while serving.
  typedef std::map<std::string, UnpackerPair> TargetToUnpackerMap;
  typedef std::map<std::string, std::string> TargetToPortableMap;

 public:
  UnpackerManager();
  ~UnpackerManager();

  /**
   * Gets portable to targets associations.
   * @param globe_to_targets map in which globe to targets associations
   * are added.
   */
  void GetPortableToTargetsMap(
      std::map<std::string, std::vector<std::string> >* portable_to_targets)
      const;

  /**
   * Unregisters portable for serving on specified target path.
   * If the reader/unpacker is preloaded, it is removed and deleted.
   * @param path Path to glx.
   * @return whether portable have been unregistered.
   */
  bool UnregisterPortable(const std::string& path);

 /**
   * Registers portable for serving on specified target path.
   * @param target_path Publish target path.
   * @param globe_path Path to globe.
   * @param r  Request object from Apache.
   */
  void RegisterPortable(const std::string& target_path,
                        const std::string& globe_path,
                        request_rec* r);

  /**
   * Gets Reader/Unpacker for the given target if glx is registered
   * for serving.
   * Note: Creates reader/unpacker if they don't exist.
   * @param target_path Publish target path.
   * @return reader, unpacker pair if target path is registered for
   * serving, otherwise - (NULL, NULL)-pair.
   */
  UnpackerPair GetUnpacker(const std::string& target_path);

  // Gets the path to the Portable associated with the given target.
  std::string GetPortablePath(const std::string& target_path) const {
    khLockGuard lock(mutex);
    TargetToPortableMap::const_iterator it_path =
        target_to_portable_map_.find(target_path);

    if (it_path != target_to_portable_map_.end())
      return it_path->second;

    return std::string();
  }

  // Resets UnpackerManager: deletes all reader and unpacker instances and
  // clears internal structures.
  void Reset();

 private:
  /**
   * Creates a new reader/unpacker for specified target path and glx.
   * @param target_path Publish target path.
   * @param globe_path Path to globe.
   * @return created (reader, unpacker)- pair.
   */
  UnpackerPair CreateUnpacker(const std::string& target_path,
                              const std::string& globe_path);

  mutable khMutex mutex;
  // Object for managing file pointers.
  geFilePool file_pool_;
  TargetToUnpackerMap target_to_unpacker_map_;
  TargetToPortableMap target_to_portable_map_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_UNPACKERMANAGER_H_

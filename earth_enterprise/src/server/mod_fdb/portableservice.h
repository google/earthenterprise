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

// Class for accessing data from a glx via its associated
// glx unpacker or from a Fusion database.
#ifndef GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_PORTABLESERVICE_H_
#define GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_PORTABLESERVICE_H_

#include <string>
#include <map>
#include "common/geFilePool.h"
#include "fusion/portableglobe/cutspec.h"
#include "server/mod_fdb/tileservice.h"
#include "server/mod_fdb/unpackermanager.h"

// fusion/portableglobe/servers/fileunpacker/shared/file_package.h
class PackageFileLoc;

// fusion/portableglobe/servers/fileunpacker/shared/fdb_unpacker.h
class FdbUnpacker;

// server/mod_fdb/apache_fdb_reader.h
class ApacheFdbReader;


class PortableService {
 public:
  PortableService() { }

  /**
   * Sets the directory in which a specified fdb, glb or glm will be sought.
   * @param globe_directory Full path to the globe directory. No trailing slash
   *                        is necessary.
   */
  void SetGlobeDirectory(const std::string& globe_directory);

  void Reset() {
    unpacker_manager_.Reset();
  }

  // Registers glx for serving.
  int RegisterPortable(request_rec* r,
                      const std::string& target_path,
                      const std::string& db_path);

  // Unregisters portable for serving on specified target path.
  // If the reader/unpacker is preloaded, it is removed and deleted.
  bool UnregisterPortable(request_rec* r, const std::string& target_path);

  /**
   * Parses part of url path after target path as portable globe path.
   * @param globe_suffix The suffix of the globe.
   * @param layer_id Returns index of the layer to be accessed in a fdb.
   * @param residual Returns additional path information which might
   *                 specify a command to apply to the globe.
   * @param is_balloon_request Returns whether it is a request for balloon
   *                           content.
   */
  void ParsePortablePath(const std::string& path,
                         const std::string globe_suffix,
                         int* layer_id,
                         std::string* residual,
                         bool* is_balloon_request);

  /**
   * Processes request for Portable globe or map data.
   * @param r Request object from Apache.
   * @param source_cmd_or_path Command to process.
   * @param target_path Target path to access the Portable via a url.
   * @param suffix The suffix of the globe.
   * @param layer_id The layer to be accessed in a fdb.
   * @param is_balloon_request Flag whether it is a request for balloon
   *                           content.
   * @param cutspec Cutspec to limit extent where queries are processed. If
   *                null, then all queries are processed.
   */
  int ProcessPortableRequest(request_rec* r,
                             const std::string& source_cmd_or_path,
                             const std::string& target_path,
                             const std::string& suffix,
                             int layer_id,
                             bool is_balloon_request,
                             ArgMap& arg_map,
                             const std::string& no_value_arg,
                             fusion_portableglobe::CutSpec* cutspec);

  /**
   * Handles status request for portable globe.
   * Gives info about a particular globe as well as a list of the current
   * unpackers.
   * @param r Request object from Apache.
   * @param reader Reader for accessing globe data.
   * @return Apache return code.
   */
  int DoPortableStatus(request_rec* r,
                       ApacheFdbReader* reader,
                       const std::string& target_path);


  /**
   * Handles general status request. Gives info about list of the current
   * unpackers.
   * @param data String which status is added to.
   */
  void DoStatus(std::string* data);

  /**
   * Gets full path to a glc, glb or glm file.
   * @param globe_name Name of glc, glb or glm file in the globes directory.
   * @return Full path to the glc, glb or glm file.
   */
  std::string GetGlobePath(const std::string& globe_name) const;

 private:
  UnpackerManager unpacker_manager_;
  std::string globe_directory_;

  /**
   * Helper to pull layer index from string.
   * E.g.
   *   <prefix>/4/<suffix> - returns true and "4" in layer_id_str.
   *    <prefix>/<string>/<suffix> - returns false and undefined.
   * @param path Path being parsed.
   * @param suffix_index Index where suffix begins.
   * @param layer_id_str Get layer index string if found.
   * @return whether layer_index appears to be present.
   */
  bool ParseLayerIndex(const std::string& path,
                       unsigned int suffix_index,
                       std::string* layer_id_str);

  /**
   * Handles flatfile requests for different types of data packets.
   * @param r Request object from Apache.
   * @param unpacker_pair Reader and unpacker pointers.
   * @param layer_id Index of the layer to be accessed in a fdb.
   * @param arg_map Key/value pairs parsed from url args.
   * @param arg Argument given specifying the packet to be retrieved.
   * @return Apache return code.
   */
  int DoFlatfile(request_rec* r,
                 const UnpackerPair& unpacker_pair,
                 int layer_id,
                 ArgMap& arg_map,
                 const std::string& arg);

  /**
   * Handles query requests for different types of 2d data packets.
   * @param r Request object from Apache.
   * @param unpacker_pair Reader and unpacker pointers.
   * @param layer_id Index of the layer to be accessed in a fdb.
   * @param suffix Suffix of globe indicating type (glc, glb, or glm).
   * @param arg_map Key/value pairs parsed from url args.
   * @param arg Argument given specifying the packet to be retrieved.
   * @return Apache return code.
   */
  int DoQuery(request_rec* r,
              const UnpackerPair& unpacker_pair,
              int layer_id,
              const std::string& suffix,
              ArgMap& source_arg_map,
              const std::string& arg);

  /**
   * Gets portable to targets associations.
   * @param globe_to_targets map in which globe to targets associations
   * are added.
   */
  void GetPortableToTargetsMap(
      std::map<std::string, std::vector<std::string> >* portable_to_targets)
      const {
    unpacker_manager_.GetPortableToTargetsMap(portable_to_targets);
  }
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_PORTABLESERVICE_H_

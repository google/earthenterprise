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

// Class for accessing data from a glx via its associated
// glx unpacker or from a Fusion database.
#ifndef GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_FDBSERVICE_H_
#define GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_FDBSERVICE_H_

#include <string>
#include <map>
#include "common/geFilePool.h"
#include "fusion/portableglobe/cutspec.h"
#include "server/mod_fdb/fusiondbservice.h"
#include "server/mod_fdb/portableservice.h"

// Third party http_request.h
class request_rec;


class FdbService : public TileService {
 public:
  FdbService();
  ~FdbService();

  /**
   * Handles a request for some globe.
   */
  int ProcessRequest(request_rec* r);

  /**
   * Sets the directory in which a specified fdb, glb or glm will be sought.
   * @param globe_directory Full path to the globe directory. No trailing slash
   *                        is necessary.
   */
  void SetGlobeDirectory(const std::string& globe_directory);

 private:
  // Resets Fdb service: clears all readers/unpackers.
  int DoReset(request_rec* r);

  // Handles publish Fusion database request.
  int DoPublishFusionDb(request_rec* r,
                        const std::string& target_path,
                        const std::string& db_path);

  // Handles publish portable request.
  int DoPublishPortable(request_rec* r,
                        const std::string& target_path,
                        const std::string& db_path);

  // Handles unpublish target request.
  // If the reader/unpacker is preloaded, it is removed and deleted.
  int DoUnpublish(request_rec* r,
                  const std::string& target_path);

  /**
   * Parses url args which are in the form of globe requests.
   * @param arg_str Original data from the url.
   * @param arg_map Returns name/value pairs derived from the arg string.
   * @param no_value_arg Returns argument that has no associated value.
   *                     For globe requests, this tends to be the key
   *                     aspect of the request (e.g. f1c-03012032-i.18).
   */
  void ParseUrlArgs(const std::string& arg_str,
                    ArgMap* arg_map,
                    std::string* no_value_arg);

  /**
   * Parses path given after the virtual host specifying part of the url.
   * @param path Remaining path after virtual host specification.
   * @param target_path Returns target path which is always first item in path
   * for DB/globe requests or empty string in case of general request.
   * @param command Returns command which is always first item in path for
   * general requests or empty string in case of DB/globe request.
   * @param residual Returns part of URL path after target path - additional
   * path information which might specify a command to apply to the globe.
   */
  void ParsePath(const std::string& path,
                 std::string* target_path,
                 std::string* command,
                 std::string* residual);

  /**
   * Checks that all args are represented in the arg map.
   * @param arg_map Key/value pairs parsed from url args.
   * @param num_args Number of required keys.
   * @param args Required keys in arg_map.
   * @param msg Message to be used if check fails.
   * @return whether all args were in the arg map.
   */
  bool CheckArgs(const ArgMap& arg_map,
                 int num_args,
                 const char* args[],
                 std::string* msg) const;

  /**
   * Builds temporary qtnode file to be used for defining a cut spec.
   * @param qtnodes_file Temporary file path.
   * @param qtnodes_string Qtnodes passed in as http argument.
   */
  void BuildQtnodesFile(const std::string& qtnodes_file,
                        const std::string& qtnodes_string);

  /**
   * Adds given cut spec to map of available cut specs
   * that can be used for dynamic cutting (filtering).
   */
  int DoAddCutSpec(request_rec* r, ArgMap& arg_map);

  /**
   * Processes commands with no globe specification.
   * @param r Request object from Apache.
   * @param command Command to be processed.
   * @return Apache return code.
   */
  int ProcessGeneralCommand(request_rec* r,
                            const std::string& command);

  /**
   * Handles general status request.
   * Gives info about list of the current unpackers.
   * @param r Request object from Apache.
   */
  int DoGeneralStatus(request_rec* r);

  // Returns whether cutting is enabled.
  void DoGetCuttingEnabled() const {
    fusion_db_service_.DoGetCuttingEnabled();
  }

  // Sets whether cutting is enabled.
  void DoSetCuttingEnabled(bool enable) {
    is_cutting_enabled_ = enable;
    fusion_db_service_.DoSetCuttingEnabled(is_cutting_enabled_);
    // TODO: add SetCuttingEnabled() for portable service?
  }

  bool is_cutting_enabled_;
  std::map<std::string, fusion_portableglobe::CutSpec*> cut_specs_;
  PortableService portable_service_;
  FusionDbService fusion_db_service_;
};

extern FdbService fdb_service;

#endif  // GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_FDBSERVICE_H_

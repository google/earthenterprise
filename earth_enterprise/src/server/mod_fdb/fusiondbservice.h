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

#ifndef GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_FUSIONDBSERVICE_H_
#define GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_FUSIONDBSERVICE_H_

#include <string>
#include <map>
#include "common/geFilePool.h"
#include "fusion/portableglobe/cutspec.h"
#include "server/mod_fdb/fdb_reader_manager.h"
#include "server/mod_fdb/tileservice.h"
#include "server/mod_fdb/motf_generator.h"

class ServerdbReader;

class FusionDbService {
 public:
  FusionDbService();

  /**
   * Registers Fusion database for serving on specified target path.
   * @param r Request object from Apache.
   * @param target_path Publish target path.
   * @param db_path Path to Fusion db in the file system.
   */
  int RegisterFusionDb(request_rec* r,
                       const std::string& target_path,
                       const std::string& db_path);

  // Resets FusionDbService: clears all preloaded readers.
  void Reset() {
    fdb_reader_manager_.Reset();
  }

  // Unregisters Fusion database for serving on specified target path.
  // If the reader is preloaded, it is removed and deleted.
  bool UnregisterFusionDb(request_rec* r, const std::string& target_path);

  /**
   * Processes requests for fusion db data.
   * @param r Request object from Apache.
   * @param cmd_or_path Command to be processed.
   * @param target_path Publish target path.
   * @param source_arg_map Key/value pairs parsed from url args.
   * @return Apache return code.
   */
  int ProcessFusionDbRequest(request_rec* r,
                             const std::string& cmd_or_path,
                             const std::string& target_path,
                             ArgMap* source_arg_map,
                             const std::string& no_value_arg,
                             fusion_portableglobe::CutSpec* cutspec);

  /**
   * Handles general status request. Gives info about list of the current
   * db readers.
   * @param data String which status is added to.
   */
  void DoStatus(std::string* data);

  /**
   * Handles status request for Fusion Db.
   * Gives info about a particular globe as well as a list of the current
   * unpackers.
   * @param r Request object from Apache.
   * @return Apache return code.
   */
  int DoFusionDbStatus(request_rec* r,
                       const std::string& target_path);

  // Returns whether cutting is enabled.
  bool DoGetCuttingEnabled() const {
    return fdb_reader_manager_.GetCuttingEnabled();
  }

  // Sets whether cutting is enabled.
  void DoSetCuttingEnabled(bool enable) {
    fdb_reader_manager_.SetCuttingEnabled(enable);
  }

 private:
  /**
   * Gets fusion db to targets associations.
   * @param fusion_db_to_targets map in which fusion db to targets associations
   * will be added.
   */
  void GetFusionDbToTargetsMap(
      std::map<std::string, std::vector<std::string> >* fusion_db_to_targets)
      const {
    fdb_reader_manager_.GetFusionDbToTargetsMap(fusion_db_to_targets);
  }

  FdbReaderManager fdb_reader_manager_;

  // Mercator on the Fly processor/generator (Motf).
  geserver::MotfGenerator motf;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_FUSIONDBSERVICE_H_

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


// Class for reprojecting plate carree(pc) tiles to mercator(merc).
#ifndef GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_MOTF_GENERATOR_H_
#define GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_MOTF_GENERATOR_H_

#include "server/mod_fdb/tileservice.h"

class ServerdbReader;

namespace geserver {

// MotfGenerator reprojects tiles from a plate carree database to generate a
// mercator tile requested in quadtree x,y,z coordinate space.
class MotfGenerator {
 public:
  MotfGenerator();

  /**
   * Generates Mercator tile.
   *   ServerdbReader* reader: server database reader.
   *   ArgMap* arg_map: input argument descriptor.
   *   request_rec* r: server request structure.
   */
  int GenerateMotfTile(ServerdbReader* reader,
                       ArgMap* arg_map, request_rec* r);
};  // class MotfGenerator

};  // namespace geserver

#endif  // GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_MOTF_GENERATOR_H_

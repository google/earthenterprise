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

// Base definitions for fdb module tile services.
#ifndef GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_TILESERVICE_H_
#define GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_TILESERVICE_H_

#include <ap_compat.h>
#include <apr_date.h>
#include <http_request.h>
#include <http_log.h>
#include <http_protocol.h>
#include <string>
#include <map>

// Type for storing args that come in via the url.
typedef std::map<std::string, std::string> ArgMap;

// Hard-code Google Earth channels.
static const int IMAGERY_CHANNEL = 0;
static const int TERRAIN_CHANNEL = 1;

// Separator used in URL path to split target path from residual (command,
// layer id, ..)
const char kTargetPathSeparator[] = "/db/";


class TileService {
 public:
  // Shows information about a request that the system was unable to
  // process.
  static int ShowFailedRequest(request_rec* r,
                               const ArgMap& arg_map,
                               const std::string& cmd_or_path,
                               const std::string& target_path);

  // Reply with the given string (used for debugging).
  static int StaticReply(request_rec* r, const std::string& reply);
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_TILESERVICE_H_

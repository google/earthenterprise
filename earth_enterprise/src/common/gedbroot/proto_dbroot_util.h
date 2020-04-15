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

// Utility functions related to proto dbroots.

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_PROTO_DBROOT_UTIL_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_PROTO_DBROOT_UTIL_H_

#include <string>
//#include "common/khTypes.h"
#include <cstdint>

namespace gedbroot {
  void CreateTimemachineDbroot(const std::string& dbroot_filename,
                               const unsigned int database_version,
                               const std::string& template_filename = "");

  // Takes contents and parses it as a dbroot and gets information about
  // imagery, terrain, and proto-imagery, returns false if the parsing fails.
  bool GetDbrootInfo(const std::string& contents, bool *has_imagery,
                     bool *has_terrain, bool *is_proto_imagery);
}  // namespace gedbroot

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_PROTO_DBROOT_UTIL_H_

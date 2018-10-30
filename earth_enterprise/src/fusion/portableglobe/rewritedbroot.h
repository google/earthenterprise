// Copyright 2018 the Open GEE Contributors
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

// Functions that rewrite various aspects of the dbroot.

#include <string>
#include "common/gedbroot/proto_dbroot.h"

void SaveIcons(const std::string &path,
               const std::string &server,
               geProtoDbroot *dbroot);

void RemoveUnusedLayers(const std::string &data_type,
                        geProtoDbroot *dbroot);

void ReplaceSearchServer(const std::string &search_service,
                         geProtoDbroot *dbroot);

void DisableTimeMachine(geProtoDbroot *dbroot);

void ReplaceReferencedKml(const std::string &new_kml_base_url,
                          const std::string &kml_url_to_file_map_file,
                          const bool preserve_kml_filenames,
                          geProtoDbroot *dbroot);

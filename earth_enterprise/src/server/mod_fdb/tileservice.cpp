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



#include "server/mod_fdb/tileservice.h"

int TileService::ShowFailedRequest(request_rec* r,
                                   const ArgMap& arg_map,
                                   const std::string& cmd_or_path,
                                   const std::string& target_path) {
  char str[256];
  std::string data = "FAILURE\n";

  data.append("Path info: ");
  data.append(r->path_info);
  data.append("\n");
  data.append("Uri: ");
  data.append(r->uri);
  data.append("\n");
  if (r->args) {
    data.append("Args: ");
    data.append(r->args);
    data.append("\n");
  } else {
    data.append("Args: ");
    data.append("none");
    data.append("\n");
  }
  data.append("Globe: ");
  data.append(target_path);
  data.append("\n");
  data.append("Cmd or path: ");
  data.append(cmd_or_path);
  data.append("\n");

  data.append("Arg Map: ");
  data.append("\n");
  ArgMap::const_iterator it = arg_map.begin();
  snprintf(str, sizeof(str), "%lu", arg_map.size());
  data.append("Size: ");
  data.append(str);
  data.append("\n");
  for (; it != arg_map.end(); ++it) {
    data.append("key: ");
    data.append(it->first);
    data.append("  value: ");
    data.append(it->second);
    data.append("\n");
  }

  ap_rwrite(&data[0], data.size(), r);
  return OK;
}

  // Reply with the given string (used for debugging).
int TileService::StaticReply(request_rec* r, const std::string& reply) {
  ap_rwrite(&reply[0], reply.size(), r);
  return OK;
}

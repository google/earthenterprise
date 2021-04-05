// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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


#include "server/mod_fdb/fusiondbservice.h"
#include <string.h>
#include <http_config.h>
#include <http_request.h>
#include <http_log.h>
#include <http_protocol.h>
#include <ap_compat.h>
#include <apr_date.h>
#include <string>
#include <map>
#include "common/etencoder.h"
#include "common/geFilePool.h"
#include "common/khConstants.h"
#include "common/khFileUtils.h"
#include "common/khStringUtils.h"
#include "common/quadtreepath.h"
#include "common/serverdb/serverdbReader.h"
#include "common/serverdb/map_tile_utils.h"
#include "common/proto_streaming_imagery.h"
#include "fusion/portableglobe/quadtree/qtutils.h"
#include "fusion/portableglobe/servers/fileunpacker/shared/file_package.h"
#include "fusion/portableglobe/servers/fileunpacker/shared/glc_unpacker.h"
#include "server/mod_fdb/fdb_reader_manager.h"
#include "server/mod_fdb/unpackermanager.h"
#include "server/mod_fdb/apache_fdb_reader.h"
#include "common/geGdalUtils.h"

#ifdef APLOG_USE_MODULE
APLOG_USE_MODULE(fdb);
#endif

namespace {

const int kGlcBaseLayer = 1;

}  // namespace

FusionDbService::FusionDbService() {
  // Initialize unique VSI file creation random seed.
  geGdalVSI::InitVSIRandomSeed();
}

// Registers Fusion database for serving.
int FusionDbService::RegisterFusionDb(request_rec* r,
                                      const std::string& target_path,
                                      const std::string& db_path) {
  std::string data;
  data.append("FdbService: target:database '");
  data.append(target_path);
  data.append(":");
  data.append(db_path);
  data.append("' registered for serving.");

  // TODO: get num_cache_levels parameter from request.
  int num_cache_levels = 2;
  fdb_reader_manager_.RegisterFusionDb(
      target_path, db_path, num_cache_levels, r);
  ap_rwrite(&data[0], data.size(), r);
  return OK;
}

// Unregisters Fusion database for serving on specified target path.
// If the reader is preloaded, it is removed and deleted.
bool FusionDbService::UnregisterFusionDb(request_rec* r,
                                         const std::string& target_path) {
  if (fdb_reader_manager_.UnregisterFusionDb(target_path)) {
    std::string data;
    data.append("FdbService: target '");
    data.append(target_path);
    data.append("' unregistered for serving.");
    ap_rwrite(&data[0], data.size(), r);
    return true;
  }
  return false;
}

// Serves packets from a Fusion database.
int FusionDbService::ProcessFusionDbRequest(
    request_rec* r,
    const std::string& cmd_or_path,
    const std::string& target_path,
    ArgMap* source_arg_map,
    const std::string& no_value_arg,
    fusion_portableglobe::CutSpec* cutspec) {
  // TODO: make refactoring of working with arg_map.
  ArgMap &arg_map = *source_arg_map;
  std::string cmd = cmd_or_path;
  bool is_time_machine = false;

  if (cmd_or_path == "dbRoot.v5") {
    cmd = "query";
    arg_map["request"] = "Dbroot";
    if (arg_map.find("output") == arg_map.end()) {
      arg_map["output"] = "proto";
    }
    if (arg_map.find("hl") == arg_map.end()) {
      arg_map["hl"] = "en";
    }
    if (arg_map.find("gl") == arg_map.end()) {
      arg_map["gl"] = "us";
    }
  } else if (cmd_or_path == "flatfile") {
    std::string args = no_value_arg;
    std::string version_prefix;
    if (args.substr(0, 4) == "q2-0") {
      args = args.substr(4);
      version_prefix = "-q.";
      arg_map["request"] = "QTPacket";
    } else if (args.substr(0, 4) == "qp-0") {
      is_time_machine = true;
      args = args.substr(4);
      version_prefix = "-q.";
      arg_map["request"] = "QTPacket2";
    } else if (args.substr(0, 4) == "f1-0") {
      args = args.substr(4);
      version_prefix = "-i.";
      arg_map["request"] = "ImageryGE";
      arg_map["channel"] = Itoa(kGEImageryChannelId);
      if (arg_map["db"] == "tm") {
        is_time_machine = true;
      }
    } else if (args.substr(0, 5) == "f1c-0") {
       args = args.substr(5);
       if (args.find("-t.") != std::string::npos) {
         version_prefix = "-t.";
         arg_map["request"] = "Terrain";
         arg_map["channel"] = Itoa(kGETerrainChannelId);
       } else {
         version_prefix = "-d.";
         arg_map["request"] = "VectorGE";
       }
    } else if (args.substr(0, 5) == "lf-0-") {
      arg_map["request"] = "Icon";
      arg_map["icon_path"] = args.substr(5);
    } else {
      cmd = "unknown request";
      r->content_type = "text/plain";
      return TileService::ShowFailedRequest(r, arg_map, cmd, target_path);
    }

    // Extract the qt address.
    size_t index = args.find(version_prefix);
    arg_map["blist"] = args.substr(0, index);
    args = args.substr(index + version_prefix.size());

    // If vector, it also has the channel information.
    if (arg_map["request"] == "VectorGE") {
      index = args.find(".");
      arg_map["channel"] = args.substr(0, index);
      args = args.substr(index + 1);
    } else if ((arg_map["request"] == "ImageryGE") && is_time_machine) {
      // Time machine suffix is of form i.<version>-<channel (hex)>
      index = args.find("-");
      if (index != std::string::npos) {
        arg_map["date"] = args.substr(index + 1);
        args = args.substr(0, index);
      } else {
        ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                      "Expected date tag for tm imagery request.");
      }
    }
    arg_map["version"] = args;
    cmd = "query";

  // Check for status request.
  } else if (cmd_or_path == "statusz") {
    r->content_type = "text/html";
    return DoFusionDbStatus(r, target_path);

  // Check for search file requests (json or html).
  } else if ((cmd_or_path == "search_json") || (cmd_or_path == "search_html")) {
    // TODO: preload in ServerdbReader and reguest via fdb_manager
    // from ServerdbReader. And then maybe report warning only in
    // ServerdbReader while preloading.
    r->content_type = "text/html";
    std::string path = fdb_reader_manager_.GetFusionDbPath(target_path);
    if (path.empty()) {
      ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                    "Target path %s not registered for serving.",
                    target_path.c_str());
      return HTTP_NOT_FOUND;
    }

    if (cmd_or_path == "search_json") {
      path += "/json/search.json";
    } else {
      path += "/search.html";
    }

    std::string content;
      
    if (!khReadStringFromFile(path, content)) {
      ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                    "Database file %s not found.",
                    path.c_str());
      return HTTP_NOT_FOUND;
    }
    ap_rwrite(&content[0], content.size(), r);
    return OK;
  }

  if (cmd == "query") {
    // If query is x, y, z-based, then calculate quadtree address.
    if (arg_map.find("x") != arg_map.end()) {
      arg_map["col"] = arg_map["x"];
      arg_map["row"] = arg_map["y"];
      arg_map["level"] = arg_map["z"];
      std::uint32_t x = atoi(arg_map["x"].c_str());
      std::uint32_t y = atoi(arg_map["y"].c_str());
      std::uint32_t z = atoi(arg_map["z"].c_str());
      if (z > QuadtreePath::kMaxLevel) {
        ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                      "Zoom level exceeds maximum allowed value.");
        return HTTP_NOT_FOUND;
      }
      arg_map["blist"] =
          fusion_portableglobe::ConvertToQtNode(x, y, z).substr(1);
    }

    // If a cutspec has been defined, use it to determine if we should
    // return the requested data. We do not filter quadset requests at
    // the moment.
    // TODO: Consider using KeepNode to filter quadsets.
    // TODO: or rewriting quadsets based on ExcludeNode and KeepNode.
    if (cutspec != NULL) {
      // If node overlaps an excluded region and is above the default
      // resolution, then don't return the data.
      if (cutspec->ExcludeNode(arg_map["blist"])) {
        return HTTP_NOT_FOUND;
      }
      // If node is above the default resolution and does NOT overlap
      // the region of interest, then don't return the data.
      if (!cutspec->KeepNode(arg_map["blist"])) {
        return HTTP_NOT_FOUND;
      }
    }

    // Get DB Reader.
    ServerdbReader* reader =
        fdb_reader_manager_.GetServerdbReader(target_path);

    if (!reader) {
      ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                    "Reader for target path %s not found.",
                    target_path.c_str());
      return HTTP_NOT_FOUND;
    }
    // Reproject Plate Carree Database tile(s) to requested Mercator tile.
    if (arg_map["request"] == "ImageryMapsMercator") {
      return motf.GenerateMotfTile(reader, &arg_map, r);
    }

    ServerdbReader::ReadBuffer buf;
    bool is_cacheable = true;

    const MimeType content_type = reader->GetData(arg_map, buf, is_cacheable);

    // A couple of cases demand immediate response without need for additional
    // http headers.

    // Immediate Redirects:
    // For a redirect, the filename for the redirect will be passed back in
    // "buf".
    if (content_type.Equals(MapTileUtils::kRedirectMimeType)) {
      ap_internal_redirect(buf.c_str(), r);
      return OK;
    }

    // Not Found:
    // Some queries will want a specific "NOT_FOUND" return code.
    if (content_type.Equals(MapTileUtils::kNotFoundMimeType)) {
      // Don't log these...there may be many.
      return HTTP_NOT_FOUND;
    }

    r->content_type = content_type.GetLiteral();

    if ((r->content_type == MapTileUtils::kNotFoundMimeType.GetLiteral()) ||
        (buf.size() == 0)) {
      r->content_type = "text/plain";
    } else {
      ap_rwrite(buf.data(), buf.size(), r);
    }
    return OK;
  }

  r->content_type = "text/plain";
  return TileService::ShowFailedRequest(r, arg_map, cmd, target_path);
}

void FusionDbService::DoStatus(std::string* data) {
  // Show Fusion databases registered for serving.
  std::map<std::string, std::vector<std::string> > fusiondb_to_targets;
  GetFusionDbToTargetsMap(&fusiondb_to_targets);
  char str[256];
  snprintf(str, sizeof(str), "%lu", static_cast<unsigned long>(fusiondb_to_targets.size()));
  data->append("<h4>Fusion DB(s) serving on Target(s) (");
  data->append(str);
  data->append("):</h4>");
  std::map<std::string, std::vector<std::string> >::iterator it =
      fusiondb_to_targets.begin();
  for (; it != fusiondb_to_targets.end(); ++it) {
    data->append("<p><em>&nbsp;&nbsp;&nbsp;Fusion DB: ");
    data->append(it->first);
    data->append("</em>");
    data->append("<ul>");
    std::vector<std::string> &targets = it->second;
    for (size_t i = 0; i < targets.size(); ++i) {
      data->append("<li>");
      data->append(targets[i]);
      data->append("</li>");
    }
    data->append("</ul>");
  }
}

int FusionDbService::DoFusionDbStatus(request_rec* r,
                                      const std::string& target_path) {
  std::string data;

  data.append("<html><body>");
  data.append("<font face='arial' size='1'>");
  data.append("<h3>Status for ");
  data.append(target_path);
  data.append("</h3>");
  data.append("<b>FusionDb: <i>");
  const std::string db_path = fdb_reader_manager_.GetFusionDbPath(target_path);
  data.append(db_path.empty() ? "<not registered>" : db_path);
  data.append("</i></b>");
  data.append("</font>");
  data.append("</body></html>");
  ap_rwrite(&data[0], data.size(), r);

  return OK;
}

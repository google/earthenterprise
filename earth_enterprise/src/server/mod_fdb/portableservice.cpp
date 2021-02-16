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



#include "server/mod_fdb/portableservice.h"

#include <string.h>
#include <http_config.h>
#include <http_request.h>
#include <http_log.h>
#include <http_protocol.h>
#include <ap_compat.h>
#include <apr_date.h>
#include <string>
#include <map>
#include "common/serverdb/map_tile_utils.h"
#include "common/etencoder.h"
#include "common/geFilePool.h"
#include "common/khConstants.h"
#include "common/khFileUtils.h"
#include "common/khStringUtils.h"
#include "common/khTileAddr.h"
#include "common/khSimpleException.h"
#include "common/proto_streaming_imagery.h"
#include "fusion/portableglobe/quadtree/qtutils.h"
#include "fusion/portableglobe/servers/fileunpacker/shared/file_package.h"
#include "fusion/portableglobe/servers/fileunpacker/shared/glc_unpacker.h"
#include "server/mod_fdb/unpackermanager.h"
#include "server/mod_fdb/apache_fdb_reader.h"

#ifdef APLOG_USE_MODULE
APLOG_USE_MODULE(fdb);
#endif

// TODO: Long-term: Set publish based on new push structure.

namespace {

const int kGlbGlobe = 0;
const int kGlcBaseLayer = 1;

typedef bool (GlcUnpacker::*FindPacketFunc)(
    const char* , int, int, int, PackageFileLoc*);

// Searches for the nearest ancestor with an imagery tile for both the Data
// Packet and the Map Data Packet.
// The parameter find_packet_func specifies either FindDataPacket() or
// FindMapDataPacket() function of GlcUnpacker.
bool FindImageryDataPacket(GlcUnpacker *const unpacker,
                           FindPacketFunc find_packet_func,
                           const std::string& qtnode,
                           int channel,
                           int layer_id,
                           PackageFileLoc *const data_loc,
                           std::string *const qtnode_with_data) {
  bool found = false;
  *qtnode_with_data = qtnode;

  // TODO: support a config parameter that sets a maximum
  // distance that the ancestor can be.

  // Search for the nearest ancestor with an imagery tile.
  for (size_t len = qtnode.size(); len > 0; --len) {
    found = (unpacker->*find_packet_func)(
        qtnode_with_data->c_str(), kImagePacket, channel, layer_id, data_loc);
    if (found) {
      break;
    } else {
      *qtnode_with_data = qtnode_with_data->substr(0, len - 1);
    }
  }

  return found;
}

}  // namespace


// Registers portable for serving.
int PortableService::RegisterPortable(request_rec* r,
                                      const std::string& target_path,
                                      const std::string& db_path) {
  std::string globe_path = GetGlobePath(db_path);
  std::string data;
  data.append("PortableService: target:database '");
  data.append(target_path);
  data.append(":");
  data.append(globe_path);
  data.append("' registered for serving.");

  unpacker_manager_.RegisterPortable(target_path, globe_path, r);
  ap_rwrite(&data[0], data.size(), r);
  return OK;
}

// Unregisters portable for serving on specified target path.
// If the reader/unpacker is preloaded, it is removed and deleted.
bool PortableService::UnregisterPortable(
    request_rec* r, const std::string& target_path) {
  if (unpacker_manager_.UnregisterPortable(target_path)) {
    std::string data;
    data.append("PortableService: target '");
    data.append(target_path);
    data.append("' unregistered for serving.");
    ap_rwrite(&data[0], data.size(), r);
    return true;
  }
  return false;
}

// Helper to pull layer index from string.
// E.g.
//    <prefix>/4/<suffix> - returns true and "4" in layer_id_str.
//    <prefix>/<string>/<suffix> - returns false and undefined.
// Returns whether layer_index appears to be present.
bool PortableService::ParseLayerIndex(const std::string& path,
                                      unsigned int suffix_index,
                                      std::string* layer_id_str) {
  *layer_id_str = path.substr(0, suffix_index);
  size_t layer_id_index = layer_id_str->rfind("/");
  // C++ should use -1 if slash not found.
  *layer_id_str = layer_id_str->substr(layer_id_index + 1);
  if (((*layer_id_str)[0] >= '0') && ((*layer_id_str)[0] <= '9')) {
    return true;
  }
  return false;
}

/**
 * Parses path of url after target path as portable globe path.
 * Returns layer ID, whether it is balloon request and builds residual.
 * Examples:
 *  Glb packet request:
 *   full path: "/my_portable_globe/db/flatfile"
 *   target_path: "/my_portable_globe"
 *   path: "flatfile"
 *   globe_suffix: "glb"
 *  return:
 *   layer_id: 0
 *   residual: "flatfile"
 *
 *  Glm packet request
 *   path: "/my_portable_map/query"
 *   target_path: "/my_portable_map"
 *   globe_suffix: "glm"
 *  return:
 *   layer_id: 0
 *   residual: "query"
 *
 *  Glc base globe packet request
 *   full path: "/my_portable_globe/db/flatfile"
 *   target_path: "/my_portable_globe"
 *   path: "flatfile"
 *   globe_suffix: "glc"
 *  return:
 *   layer_id: 1
 *   residual: "flatfile"
 *
 *  3d glc layer packet request
 *   full path: "/my_globe_glc/prefix/3/kh/flatfile"
 *   target_path: "my_globe_glc"
 *   globe_suffix: "glc"
 *   path: "prefix/3/kh/flatfile"
 *  return:
 *   layer_id: 3
 *   residual: "flatfile"
 *
 *  2d glc layer tile request
 *   full path: "/my_globe_glc/prefix/3/query"
 *   target_path: "my_globe_glc"
 *   globe_suffix: "glc"
 *   path: "prefix/3/query"
 *  return:
 *   layer_id: 3
 *   residual: "query"
 */
void PortableService::ParsePortablePath(const std::string& path,
                                        const std::string globe_suffix,
                                        int* layer_id,
                                        std::string* residual,
                                        bool* is_balloon_request) {
  *is_balloon_request = false;
  if (globe_suffix == "glc") {
    *layer_id = kGlcBaseLayer;  // Fdb base globe layer id
  } else {
    *layer_id = 0;  // Default layer id for glbs.
  }

  // Check for layer id.
  size_t index = path.find("/");
  // If simple command, use default layer id.
  if (index == std::string::npos) {
    *residual = path;
    return;
  }

  // If there is a base path, it can be either a layer id
  // or one of a limited number of directory names.
  std::string base_path = path.substr(0, index);

  // If it is a directory, use the whole path.
  if ((base_path == "earth")
      || (base_path == "maps")
      || (base_path == "js")
      || (base_path == "kml")
      || (base_path == "license")
      || (base_path == "local")) {
    *residual = path;
    return;
  }

  // If there is additional path info preceding the layer_id,
  // then ignore it.
  size_t residual_index = path.rfind("/");
  std::string command = path.substr(residual_index + 1);
  bool found_layer_id = false;
  std::string layer_id_str;

  // Check for 2d map query request.
  // Expects: <prefix>/<layer_id>/query
  // E.g. "mydb/2/query"
  if (command == "query") {
    *residual = command;
    found_layer_id = ParseLayerIndex(path, residual_index, &layer_id_str);
  }

  // Expects: <prefix>/<layer_id>/<type_based_server>/<command>
  // E.g. "mydb/2/kh/dbRoot.v5"
  residual_index = path.rfind("/", residual_index - 1);
  if (!found_layer_id && (residual_index != std::string::npos)) {
    *residual = path.substr(residual_index + 1);
    found_layer_id = ParseLayerIndex(path, residual_index, &layer_id_str);
  }

  // If layer id does not look like a number, then assume it is a command
  // for the base glc.
  if (!found_layer_id) {
    residual_index = path.rfind("/");
    *residual = path.substr(residual_index + 1);
    return;
  }

  // If it is a layer_id, extract it.
  std::istringstream stream(layer_id_str);
  stream >> *layer_id;

  // Either we found a layer id and are using it, or we ignore the prefix of
  // the path and use layer 0. This is useful because we need layer
  // references (e.g. ../4/kh/dbRoot.v5 to have a bogus directory to back up
  // from. E.g. fdb/dbRoot.v5
  if (*layer_id == 0) {
    if (globe_suffix == "glc") {
      *layer_id = kGlcBaseLayer;  // Glc base globe layer id
    }

  // If we are in a specified layer, strip expected prefixes.
  } else {
    index = residual->find("/");
    if (index != std::string::npos) {
      base_path = residual->substr(0, index);
    } else {
      base_path = *residual;
    }

    // Imagery or terrain request.
    if (base_path == "kh") {
      *residual = residual->substr(index + 1);

    // Vector request.
    } else if (base_path == "kmllayer") {
      *residual = residual->substr(index + 1);
      if (*residual == "root.kml") {
        *residual = "skel.kml";
      } else if (*residual == "eb_balloon") {
        *residual = "balloon_";
        *is_balloon_request = true;
      }

      *residual = "earth/vector_layer/" + *residual;

    // Direct file request.
    } else {
      // Leave path as is.
    }
  }
}

/**
 * Processes request for data from a specific globe.
 */
int PortableService::ProcessPortableRequest(
    request_rec* r,
    const std::string& source_cmd_or_path,
    const std::string& target_path,
    const std::string& suffix,
    int layer_id,
    bool is_balloon_request,
    ArgMap& arg_map,
    const std::string& no_value_arg,
    fusion_portableglobe::CutSpec* cutspec) {
  std::string cmd_or_path = source_cmd_or_path;
  // Get unpacker.
  const UnpackerPair unpacker_pair = unpacker_manager_.GetUnpacker(target_path);
  if (!unpacker_pair.first) {
    ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                  "Unpacker for target path %s not found.",
                  target_path.c_str());
    return HTTP_NOT_FOUND;
  }

  ApacheFdbReader* reader = unpacker_pair.first;
  GlcUnpacker* unpacker = unpacker_pair.second;

  PackageFileLoc data_loc;

  // Send the data out via Apache.
  ap_set_last_modified(r);
  ap_send_http_header(r);  // noop as of Apache 2.0

  std::string data_buffer;
  if (cmd_or_path == kHttpDbRootRequest) {
    // For a glc, we return the meta dbroot.
    if (layer_id == kGlcBaseLayer) {
      unpacker->FindMetaDbRoot(&data_loc);
    // For a glb or a glc layer (also glbs), we return the glb's dbroot.
    } else {
      unpacker->FindQtpPacket("0", kDbRootPacket, 0, layer_id, &data_loc);
    }

    try {
      reader->Read(&data_buffer, data_loc.Offset(), data_loc.Size(), r);
      ap_rwrite(&data_buffer[0], data_buffer.size(), r);
      return OK;
    } catch(const std::exception &e) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "%s", e.what());
      return HTTP_NOT_FOUND;
    }
  } else if (cmd_or_path == "flatfile") {
    return DoFlatfile(r, unpacker_pair, layer_id, arg_map, no_value_arg);
  } else if (cmd_or_path == "query") {
    return DoQuery(r, unpacker_pair, layer_id, suffix, arg_map, no_value_arg);
  } else if (cmd_or_path == "statusz") {
    r->content_type = "text/html";
    return DoPortableStatus(r, unpacker_pair.first, target_path);
  // If no command, we assume it is a path to a file.
  } else {
    if (is_balloon_request) {
      std::string balloon_id = arg_map["ftid"];
      // Replace : with - in balloon id.
      size_t index = balloon_id.find(":", 0);
      if (index != std::string::npos) {
        balloon_id[index] = '-';
      }
      cmd_or_path += balloon_id + ".html";
    }
    std::string suffix = cmd_or_path.substr(cmd_or_path.size() - 3);
    if (suffix == "gif") {
      r->content_type = "image/gif";
    } else if (suffix == "png") {
      r->content_type = "image/png";
    } else if (suffix == "tml") {
      r->content_type = "text/html";
    } else if (suffix == "kml") {
      r->content_type = "application/vnd.google-earth.kml+xml";
    } else {
      r->content_type = "text/plain";
    }

    bool found;
    // NOTE: You cannot access files of the base glb.
    if (layer_id <= kGlcBaseLayer) {
      found = unpacker->FindFile(cmd_or_path.c_str(), &data_loc);
    } else {
      found = unpacker->FindLayerFile(
          cmd_or_path.c_str(), layer_id, &data_loc);
    }
    if (found) {
      try {
        reader->Read(&data_buffer, data_loc.Offset(), data_loc.Size());
        ap_rwrite(&data_buffer[0], data_buffer.size(), r);
        return OK;
      } catch(const std::exception &e) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "%s", e.what());
        return HTTP_NOT_FOUND;
      }
    } else {
      return HTTP_NOT_FOUND;
    }
  }

  r->content_type = "text/plain";
  return TileService::ShowFailedRequest(r, arg_map, cmd_or_path, target_path);
}

/**
 * Gives the size of the given globe and some basic status on the plugin
 * including how many unpackers have been created for different globes.
 * TODO: Use a template for the output.
 * TODO: Template may be JSON for new web-based server admin.
 */
int PortableService::DoPortableStatus(request_rec* r,
                                   ApacheFdbReader* reader,
                                   const std::string& target_path) {
  std::string data;
  char str[256];

  data.append("<html><body>");
  data.append("<font face='arial' size='1'>");
  data.append("<h3>Portable Status for ");
  data.append(target_path);
  data.append("</h3>");
  data.append("<b>Globe: <i>");
  data.append(unpacker_manager_.GetPortablePath(target_path));
  data.append("</i></b>");
  std::uint64_t size = reader->Size();
  const char* units = "MB";
  float sizef = size;
  if (size < 100000000) {
    sizef /= 1000000.0;
  } else {
    sizef /= 1000000000.0;
    units = "GB";
  }

  snprintf(str, sizeof(str),
           "<br>Size: %4.2f %s (%ld bytes)", sizef, units, size);
  data.append(str);

  data.append("</font>");
  data.append("</body></html>");
  ap_rwrite(&data[0], data.size(), r);

  return OK;
}

void PortableService::DoStatus(std::string* data) {
  data->append("<p><em>portable globe directory: ");
  data->append(globe_directory_);
  data->append("</em>");

  // Show glx(s) registered for serving.
  std::map<std::string, std::vector<std::string> > portable_to_targets;
  GetPortableToTargetsMap(&portable_to_targets);
  char str[256];
  snprintf(str, sizeof(str), "%lu", static_cast<unsigned long>(portable_to_targets.size()));
  data->append("<h4>Glx(s) serving on Target(s) (");
  data->append(str);
  data->append("):</h4>");
  std::map<std::string, std::vector<std::string> >::iterator it =
      portable_to_targets.begin();
  for (; it != portable_to_targets.end(); ++it) {
    data->append("<p><em>&nbsp;&nbsp;&nbsp;Glx: ");
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

std::string PortableService::GetGlobePath(const std::string& globe_name) const {
  return khNormalizeDir(globe_directory_ + globe_name, false);
}

void PortableService::SetGlobeDirectory(const std::string& globe_directory) {
  globe_directory_ = khNormalizeDir(globe_directory);
  std::cout << "SetGlobeDirectory: " << globe_directory_ << std::endl;
}

/**
 * Handles a "flatfile" request.
 */
int PortableService::DoFlatfile(request_rec* r,
                                const UnpackerPair& unpacker_pair,
                                int layer_id,
                                ArgMap& arg_map,
                                const std::string& arg) {
  ApacheFdbReader* reader = unpacker_pair.first;
  GlcUnpacker* unpacker = unpacker_pair.second;
  PackageFileLoc data_loc;

  std::string prefix = "";
  std::string data_info;
  std::string data_type;
  std::string data_version;

  std::string qtnode;
  int channel = 0;
  int packet_type = 0;

  std::vector<std::string> tokens;

  // Handle a query where arguments are passed in explicitly.
  if (arg == "query") {
    if (arg_map.empty()) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "No query args provided.");
      return HTTP_NOT_FOUND;
    }

    // If blist is a raw qt path (not a multi-path request), add preceding 0.
    qtnode = arg_map["blist"];
    if ((qtnode == "") ||
        (qtnode[qtnode.size() - 1] != kQuadTreePathSeparator)) {
      qtnode = "0" + qtnode;
    }
    if (arg_map["request"] == "QTPacket") {
      prefix = "q2";
    } else {
      channel = atoi(arg_map["channel"].c_str());
      if (arg_map["request"] == "ImageryGE") {
        channel = IMAGERY_CHANNEL;
        packet_type = kImagePacket;
      } else if (arg_map["request"] == "Terrain") {
        // It is necessary to force the channel because there is a mismatch
        // in the terrain channel used for Fusion (2) and Portable (1).
        channel = TERRAIN_CHANNEL;
        packet_type = kTerrainPacket;
      } else if (arg_map["request"] == "VectorGE") {
        packet_type = kVectorPacket;
      }
    }
    data_version = arg_map["version"];


  // Handle encoded flatfile request.
  } else {
    // Tokenize first arg: e.g. f1c-03012-i.18
    TokenizeString(arg, tokens, "-");
    if (tokens.size() < 3) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "Could not parse request.");
      return HTTP_NOT_FOUND;
    }
    prefix = tokens[0];
    qtnode = tokens[1];
    data_info = tokens[2];

    // Tokenize arg suffix: e.g. i.18
    tokens.clear();
    TokenizeString(data_info, tokens, ".");
    if (tokens.size() < 2) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "Could not parse request suffix.");
      return HTTP_NOT_FOUND;
    }
    data_type = tokens[0];
    data_version = tokens[1];
  }

  // Find data (quadset, imagery, terrain or vector) packet.
  if ((arg == "query") || (prefix == "q2") ||
      (prefix == "f1c") || (prefix == "f1")) {
    if ((arg != "query") && (prefix != "q2")) {
      // Imagery packet
      if (data_type == "i") {
        channel = IMAGERY_CHANNEL;
        packet_type = kImagePacket;
      // Terrain packet
      } else if (data_type == "t") {
        channel = TERRAIN_CHANNEL;
        packet_type = kTerrainPacket;
      // Vector (drawable) packet
      } else if (data_type == "d") {
        // TODO: Consider simple small int read.
        sscanf(data_version.c_str(), "%d", &channel);
        packet_type = kVectorPacket;
      // Error
      } else {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Unknown data type: %s", data_type.c_str());
        return HTTP_NOT_FOUND;
      }
    }

    // If it's not a multi-packet request,
    // read data loc of single packet and read and return the
    // packet at the end of this function.
    if (qtnode[qtnode.size() - 1] != kQuadTreePathSeparator) {
      // Quadset packet
      bool found_packet;
      if (prefix == "q2") {
        found_packet = unpacker->FindQtpPacket(
            qtnode.c_str(), kQtpPacket, 0, layer_id, &data_loc);

      // Imagery, vector or terrain packet.
      } else {
        found_packet = unpacker->FindDataPacket(
            qtnode.c_str(), packet_type, channel, layer_id, &data_loc);
      }
      if (!found_packet) {
        return HTTP_NOT_FOUND;
      }

    // If it is a multi-packet request,
    // build the packed string from the packets here and then return it.
    } else {
      std::string blist = qtnode;
      PackedString str;
      std::string data_buffer;
      for (size_t start = 0; start < blist.length(); ) {
        size_t last = blist.find_first_of(kQuadTreePathSeparator, start);
        data_buffer = "";
        qtnode = "0" + blist.substr(start, last - start);
        try {
          bool found_packet;
          if (prefix == "q2") {
            found_packet = unpacker->FindQtpPacket(
                qtnode.c_str(), kQtpPacket, 0, layer_id, &data_loc);
          // Imagery, vector or terrain packet.
          } else {
            found_packet = unpacker->FindDataPacket(
                qtnode.c_str(), packet_type, channel, layer_id, &data_loc);
          }
          if (found_packet) {
            // Try to read the packet data.
            try {
              reader->Read(&data_buffer, data_loc.Offset(), data_loc.Size(), r);
            } catch(const std::exception &e) {
              ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "%s", e.what());
            }
          }
          // Add packet data or empty string if it wasn't found.
          str.Append(data_buffer);
        } catch (...) {
          // catch all exceptions since we return status by "" return value
        }
        start = last + 1;  // Skip the 4
      }
      try {
        ap_rwrite(&str[0], str.size(), r);
        return OK;
      } catch(const std::exception &e) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "%s", e.what());
        return HTTP_NOT_FOUND;
      }
    }
  // Get icon.
  } else if (prefix == "lf") {
    r->content_type = "image/png";
    // NOTE: You cannot access files of the base glb.
    if (layer_id <= kGlcBaseLayer) {
      unpacker->FindFile(data_info.c_str(), &data_loc);
    } else {
      unpacker->FindLayerFile(data_info.c_str(), layer_id, &data_loc);
    }
  // Error
  } else {
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                  "Unknown prefix: %s.", prefix.c_str());
    return HTTP_NOT_FOUND;
  }

  // Try to get the found packet data.
  std::string data_buffer;
  try {
    reader->Read(&data_buffer, data_loc.Offset(), data_loc.Size(), r);
    ap_rwrite(&data_buffer[0], data_buffer.size(), r);
    return OK;
  } catch(const std::exception &e) {
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "Args: %s. %ld %ld",
                  arg.c_str(), data_loc.Offset(), data_loc.Size());
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "%s", e.what());
    return HTTP_NOT_FOUND;
  }
}


/**
 * Handles a "query" request.
 */
int PortableService::DoQuery(request_rec* r,
                             const UnpackerPair& unpacker_pair,
                             int layer_id,
                             const std::string& suffix,
                             ArgMap& arg_map,
                             const std::string& arg) {
  // ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r, "DoQuery.");
  if (arg_map.empty()) {
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "No query args provided.");
    return HTTP_NOT_FOUND;
  }

  // If query is really a flatfile request, handle it as such.
  // These requests are used by the Cutter.
  if ((arg_map["request"] == "QTPacket") ||
      (arg_map["request"] == "ImageryGE") ||
      (arg_map["request"] == "Terrain") ||
      (arg_map["request"] == "VectorGE")) {
    return DoFlatfile(r, unpacker_pair, layer_id, arg_map, "query");
  }

  ApacheFdbReader* reader = unpacker_pair.first;
  GlcUnpacker* unpacker = unpacker_pair.second;
  PackageFileLoc data_loc;

  std::string data;
  //   int x, y, z, channel;
  std::uint32_t x = 0;
  std::uint32_t y = 0;
  std::uint32_t z = 0;
  std::uint32_t channel = 0;
  std::string qtnode = "0";

  if ((arg_map["request"] == "ImageryMaps") ||
      (arg_map["request"] == "VectorMapsRaster")) {
    if(arg_map["x"].empty())
        x = atoi(arg_map["col"].c_str());
    else
        x = atoi(arg_map["x"].c_str());
    if(arg_map["y"].empty())
        y = atoi(arg_map["row"].c_str());
    else
        y = atoi(arg_map["y"].c_str());
    if(arg_map["z"].empty())
        z = atoi(arg_map["level"].c_str());
    else
        z = atoi(arg_map["z"].c_str());
    if (z > QuadtreePath::kMaxLevel) {
      ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                    "Zoom level exceeds maximum allowed value.");
      return HTTP_NOT_FOUND;
    }
    ArgMap::const_iterator arg_map_it = arg_map.find("channel");

    if (arg_map_it != arg_map.end()) {
      channel = atoi(arg_map_it->second.c_str());
    } else {
      if (arg_map["request"] == "ImageryMaps") {
        // Note: The channel is not specified for ImageryMaps request when
        // serving *.glb. So, based on that we distinguish an ImageryMaps
        // request to get imagery map tile from raw imagery vs GE packets.

        // Update request type to 'ImageryGePbMaps', and set channel to
        // IMAGERY_CHANNEL.
        arg_map["request"] = "ImageryGePbMaps";
        channel = IMAGERY_CHANNEL;
      } else {
        ap_log_rerror(
            APLOG_MARK, APLOG_WARNING, 0, r,
            "PortableService: The 'channel' is not specified for request.");
        return HTTP_NOT_FOUND;
      }
    }

    qtnode = fusion_portableglobe::ConvertToQtNode(x, y, z);
  }

  bool found = false;
  std::string qtnode_with_data = qtnode;
  const bool is_cutter = arg_map["ct"] == std::string("c");
  // Handle imagery tile request.
  if (arg_map["request"] == "ImageryMaps") {
    // Search for the nearest ancestor with an imagery tile.
    found = FindImageryDataPacket(unpacker,
                                  &GlcUnpacker::FindMapDataPacket,
                                  qtnode,
                                  channel,
                                  layer_id,
                                  &data_loc,
                                  &qtnode_with_data);

    // If the tile we got is not from the requested path and we are in
    // the Cutter, don't resample because we can do this in the Portable Server
    // and don't need to use up space in the cut maps for it.
    if (found && is_cutter && qtnode_with_data != qtnode) {
      return HTTP_NOT_FOUND;
    }
  // Handle vector tile request.
  } else if (arg_map["request"] == "VectorMapsRaster") {
    r->content_type = "image/png";
    found = unpacker->FindMapDataPacket(
        qtnode.c_str(), kVectorPacket, channel, layer_id, &data_loc);

  // 3d imagery as 2d tile request.
  } else if (arg_map["request"] == "ImageryGePbMaps") {
    found = FindImageryDataPacket(unpacker,
                                  &GlcUnpacker::FindDataPacket,
                                  qtnode,
                                  channel,
                                  layer_id,
                                  &data_loc,
                                  &qtnode_with_data);

    // If the tile we got is not from the requested path and we are in
    // the Cutter, don't resample because we can do this in the Portable Server
    // and don't need to use up space in the cut maps for it.
    if (found && is_cutter && qtnode_with_data != qtnode) {
      return HTTP_NOT_FOUND;
    }
  // Handle serverDefs request.
  } else if (arg_map["request"] == "Json") {
    r->content_type = "text/plain";
    std::string path;
    if ((unpacker->Is2d() && !unpacker->Is3d()) || (arg_map["is2d"] == "t")) {
      path = "maps/map.json";
    } else {
      path = "earth/earth.json";
    }

    if (layer_id <= kGlcBaseLayer) {
      found = unpacker->FindFile(path.c_str(), &data_loc);
    } else {
      found = unpacker->FindLayerFile(path.c_str(), layer_id, &data_loc);
    }

    if (found) {
      try {
        path = r->path_info;
        size_t index = path.find(kTargetPathSeparator);
        path = path.substr(0, index);
        std::string base_path;
        if (unpacker->Is2d()) {
          base_path = path;
        } else {
          std::string hostname = r->hostname;
          // TODO: Limited to http port 80.
          base_path = "http://" + hostname + path;
        }
        reader->Read(&data, data_loc.Offset(), data_loc.Size(), r);
        // Replace server address in serverDefs.
        data = ReplaceString(data, "http://localhost:9335", base_path);
        ap_rwrite(&data[0], data.size(), r);
        return OK;
      } catch(const std::exception &e) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "%s", e.what());
        return HTTP_NOT_FOUND;
      }
    } else {
      return HTTP_NOT_FOUND;
    }
  // Handle icon request.
  } else if (arg_map["request"] == "Icon") {
    r->content_type = "image/png";
    std::string path = arg_map["icon_path"];
    // NOTE: You cannot access files of the base glb.
    if (layer_id <= kGlcBaseLayer) {
      found = unpacker->FindFile(path.c_str(), &data_loc);
    } else {
      found = unpacker->FindLayerFile(path.c_str(), layer_id, &data_loc);
    }

  // Unknown request.
  } else {
    r->content_type = "text/plain";
    data = "Unknown query: " + arg_map["request"];
    ap_rwrite(&data[0], data.size(), r);
    return OK;
  }

  if (!found) {
    // TODO: For uniformity with portable server, need to
    // report out of cut region image for base layer ('non_base_layer'-property
    // is undefined or false)?! Analyze 'non_base_layer'-property and redirect
    // to out of cut image for base layers.
    // Meanwhile, report 'not found' for all layers.
#if 0
    if (!is_cutter && (arg_map["request"] == "ImageryMaps" ||
                       arg_map["request"] == "ImageryGePbMaps")) {
      // This should only be possible for alpha-masked imagery where
      // a particular location might have no visible imagery above it.
      const std::string& imagery_tile_format = arg_map["format"];
      ap_internal_redirect(
          MapTileUtils::GetOutOfCutTilePath(imagery_tile_format), r);
      return OK;
    }
#endif

    return HTTP_NOT_FOUND;
  }

  // Try to get the found packet data.
  std::string data_buffer;
  try {
    reader->Read(&data_buffer, data_loc.Offset(), data_loc.Size(), r);

    // For Imagery[GePb]Maps requests, re-sample if it is needed, and report
    // in requested format.
    if (arg_map["request"] == "ImageryMaps" ||
        arg_map["request"] == "ImageryGePbMaps") {
      // If 3d as 2d, then we need to decrypt it.
      if (arg_map["request"] == "ImageryGePbMaps") {
        etEncoder::EncodeWithDefaultKey(&data_buffer[0], data_buffer.size());
      }

      // Pull the imagery from a jpeg/png/protocol buffer packets, re-sample if
      // it is needed and convert to requested format.
      bool REPORT_IN_SOURCE_FORMAT = false;
      const std::string& imagery_tile_format = arg_map["format"];
      // Function throws khSimpleNotFoundException on failure.
      MimeType content = MapTileUtils::PrepareImageryMapsTile(
          qtnode_with_data, qtnode, &data_buffer,
          REPORT_IN_SOURCE_FORMAT, imagery_tile_format);
      r->content_type = content.GetLiteral();
    }

    ap_rwrite(&data_buffer[0], data_buffer.size(), r);
    return OK;
  } catch(khSimpleNotFoundException& e) {
    // We may get this exception in preparing ImageryMaps tile.
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "%s", e.what());
    return HTTP_NOT_FOUND;
  } catch(const std::exception &e) {
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "Args: %s. %ld %ld",
                  arg.c_str(), data_loc.Offset(), data_loc.Size());
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "%s", e.what());
    return HTTP_NOT_FOUND;
  }
}

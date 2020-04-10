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


// Methods for pruning away all quadtree nodes that are not at or below
// the given default level or encompassing the given set of
// quadtree nodes.

#include "fusion/portableglobe/portablemapbuilder.h"

#include <notify.h>
#include <algorithm>
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <map>
#include <sstream>  // NOLINT(readability/streams)
#include <string>
#include <vector>
#include "common/khConstants.h"
#include "common/khEndian.h"
#include "common/khFileUtils.h"
#include "common/khStringUtils.h"
//#include "common/khTypes.h"
#include <cstdint>
#include "fusion/gst/gstSimpleEarthStream.h"
#include "fusion/gst/gstSimpleStream.h"
#include "fusion/portableglobe/portableglobebuilder.h"
#include "fusion/portableglobe/quadtree/qtutils.h"
#include "fusion/portableglobe/shared/packetbundle.h"
#include "fusion/portableglobe/shared/packetbundle_reader.h"
#include "fusion/portableglobe/shared/packetbundle_writer.h"
#include "mttypes/DrainableQueue.h"
#include "mttypes/WaitBaseManager.h"

// TODO: Use Manas' technique for bundling packet requests.
namespace fusion_portableglobe {

// Names of subdirectories within the portable globe.
const std::uint16_t PortableMapBuilder::kMaxPacketDepth = 7;  // 4 levels / packet
const std::string PortableMapBuilder::kMapDataDirectory = "/mapdata";
// This limit comes from HUGE_STRING_LEN of 8192 defined in httpd.h
const size_t      PortableMapBuilder::kBundleSizeForSizeQuery = 8000;
// This limit comes from 256 * 24 < 8192, and the size of return stream,
// (concatenation of lot of QT packets) should be reasonable.
const size_t      PortableMapBuilder::kBundleSizeForPacketQuery = 256;

/**
 * Constructor.
 */
PortableMapBuilder::PortableMapBuilder(
    const std::string& source,
    std::uint16_t default_level,
    std::uint16_t max_level,
    const std::string& hires_qt_nodes_file,
    const std::string& map_directory,
    const std::string& additional_args,
    const std::string& metadata_file,
    bool ignore_imagery_depth,
    bool no_write)
    : map_directory_(map_directory),
    default_level_(default_level),
    max_level_(max_level),
    source_(source),
    additional_args_(additional_args),
    metadata_file_(metadata_file),
    ignore_imagery_depth_(ignore_imagery_depth),
    is_no_write_(no_write) {
  // Build a structure for filtering quadtree addresses.
  hires_tree_ = new HiresTree();
  if (!hires_qt_nodes_file.empty()) {
    hires_tree_->LoadHiResQtNodeFile(hires_qt_nodes_file);
  }

  server_ = new gstSimpleStream(source_);
}


/**
 * Destructor.
 */
PortableMapBuilder::~PortableMapBuilder() {
  if (server_ != NULL) {
    delete server_;
  }

  for (size_t i = 0; i < layers_.size(); ++i) {
    delete layers_[i];
  }
}

/**
 * Builds a map that is a subset of the specified source (i.e. the master
 * map) as a set of packet bundles.
 */
void PortableMapBuilder::BuildMap() {
  num_image_packets = 0;

  // Add writers for the different packet types.
  AddWriter(kMapDataDirectory);

  // Traverse the quadtree
  std::vector<char> qt_char(24);
  std::vector<std::uint32_t> level_row(24);
  std::vector<std::uint32_t> level_col(24);
  std::int32_t level = 0;
  std::int32_t max_level_traversed = 0;
  std::string qt_string = "";
  qt_char[level] = '0';
  level_row[level] = 1;
  level_col[level] = 0;

  char url_str[128];
  snprintf(url_str, sizeof(url_str),
           "%s/query?request=Json&var=geeServerDefs", source_.c_str());
  std::string url = url_str;
  std::string raw_packet;
  server_->GetRawPacket(url, &raw_packet);
  ParseServerDefs(raw_packet);

  while (true) {
    if (qt_char[level] > '3') {
      // Move up to lower resolution level.
      --level;
      if (level < 0) {
        break;
      }

      // Go to next qtnode at this level.
      qt_char[level] += 1;
    } else {
      if (level > max_level_traversed) {
        max_level_traversed = level;
      }
      qt_string = qt_string.substr(0, level) + qt_char[level];
      // Note that we stop descending once we reach the first
      // missing packet, which is important since this cue
      // indicates that we have reached the natural resolution
      // and it would be waste to look further.
      if (WriteMapPackets(qt_string, level,
                          level_col[level], level_row[level])) {
        ++num_image_packets;

        if (level < 23) {
          ++level;
          qt_char[level] = '0';
          level_row[level] = (level_row[level - 1] << 1) + 1;
          level_col[level] = level_col[level - 1] << 1;
        } else if (level == 23) {
          qt_char[level] += 1;
        } else {
          std::cout << "Bad level (" << level << ")" << std::endl;
          exit(1);
        }
      } else {
        qt_char[level] += 1;
      }
    }

    // Update the current row and col if we are continuing at this level.
    if (qt_char[level] == '1') {
      level_col[level] += 1;
    } else if (qt_char[level] == '2') {
      level_row[level] -= 1;
    } else if (qt_char[level] == '3') {
      level_col[level] -= 1;
    }
  }

  if (!metadata_file_.empty()) {
    bounds_tracker_.write_json_file(metadata_file_);
  }

  // Finish up writing all of the packet bundles.
  DeleteWriter();

  // The "+1" for max_level_traversed is because the code uses zero
  // for the first level but the cutter page and most humans use one.
  if (max_level_traversed+1 < max_level_) {
    notify(NFY_WARN,
      "Max level is %d but processing stopped at level %d.",
      max_level_, max_level_traversed+1);
  }
}

// TODO: Keep track of these as we go along rather than
// TODO: calculating each time.
void PortableMapBuilder::GetLevelRowCol(const std::string& qtpath_str,
                                          std::uint32_t* level,
                                          std::uint32_t* col,
                                          std::uint32_t* row) {
  *level = qtpath_str.size();
  std::uint32_t ndim = 1 << *level;
  *row = 0;
  *col = 0;
  for (size_t i = 0; i < *level; ++i) {
    ndim = ndim >> 1;
    if (qtpath_str[i] == '0') {
      *row += ndim;
    } else if (qtpath_str[i] == '1') {
      *row += ndim;
      *col += ndim;
    } else if (qtpath_str[i] == '2') {
      *col += ndim;
    }
  }
}


// Extracts value from string for a given key from json or pseudo json.
// String can contain keys of the form:
//     <key>"?\s*:\s*"<string value>"  E.g. name: "my name"
//  or
//     <key>"?\s*:\s*<integer value>  E.g. "id" : 23414
//
//  It is somewhat forgiving of poorly formatted js.
//      E.g. val: -1- would be interpreted as -1 for key "val".
//
// The previous version expected a very rigid key format (unquoted,
// with exactly one space before the colon), which was failing for
// cuts coming from GME.
std::string PortableMapBuilder::ExtractValue(const std::string& str,
                                             const std::string& key) {
  enum ExtractState {
    LOOKING_FOR_COLON = 1,
    LOOKING_FOR_VALUE,
    LOOKING_FOR_STRING,
    LOOKING_FOR_NUMBER,
    DONE
  };

  std::string value = "";
  notify(NFY_DEBUG, "Searching for: %s", key.c_str());
  int index = str.find(key) + key.size();
  if (index >= 0) {
    // Key may have been quoted."
    if (str[index] == '"') {
      index++;
    }
    // Use FSM approach to extract value from string.
    // Start by looking for colon after key.
    ExtractState state = LOOKING_FOR_COLON;
    // Get null terminated string.
    const char *str_ptr = &str[index];
    while (state != DONE) {
      char ch = *str_ptr++;
      // Clear white space from everything but strings.
      if (state != LOOKING_FOR_STRING) {
        while ((ch == ' ') || (ch == '\t')) {
          ch = *str_ptr++;
        }
      }
      // If we hit end of string, go with what we have.
      if (ch == 0) {
        state = DONE;
      }
      switch (state) {
        // Looking for colon after key.
        case LOOKING_FOR_COLON:
          if (ch == ':') {
            state = LOOKING_FOR_VALUE;
          } else {
            notify(NFY_WARN, "'%s' is not a key in an associative array: %s",
                   key.c_str(), str.c_str());
            state = DONE;
          }
          break;
        // Looking for string or integer value after colon.
        case LOOKING_FOR_VALUE:
          if (ch == '"') {
            state = LOOKING_FOR_STRING;
          } else if (((ch >= '0') && (ch <= '9')) || (ch == '-')) {
            state = LOOKING_FOR_NUMBER;
            value += ch;
          } else {
            notify(NFY_WARN, "Corrupt javascript: %s", str.c_str());
            state = DONE;
          }
          break;
        // Looking for rest of string value.
        case LOOKING_FOR_STRING:
          if (ch == '"') {
            state = DONE;
          } else {
            value += ch;
          }
          break;
        // Looking for rest of integer value.
        case LOOKING_FOR_NUMBER:
          if ((ch >= '0') && (ch <= '9')) {
            value += ch;
          } else {
            state = DONE;
          }
          break;
        // Done.
        case DONE:
          notify(NFY_DEBUG, "Extracted value: %s", value.c_str());
      }
    }
  }

  return value;
}

// Comparison function for layer sort.
// Note that this comparison should follow the precendence order established
// in IndexItem::operator< in packetbundle.cpp.
//
bool LayerLessThan(LayerInfo* layer1, LayerInfo* layer2) {
  if (layer1->type_id < layer2->type_id) {
    return true;
  } else if (layer1->type_id == layer2->type_id) {
    if (layer1->channel_num < layer2->channel_num) {
      return true;
    }
  }
  return false;
}

void PortableMapBuilder::ParseServerDefs(const std::string& json) {
  // Isolate the layers
  notify(NFY_DEBUG, "json: %s", json.c_str());
  int index = json.find("layers");
  if (index < 0) {
    notify(NFY_WARN, "No layers found for cut.");
    return;
  }
  int start_layers = json.find("[", index) + 1;
  int end_layers = json.find("]", start_layers);
  const std::string layers_str = json.substr(start_layers,
                                             end_layers - start_layers);

  // Process each layer
  int end_layer = -1;
  std::string type;
  std::string channel;
  std::string version;
  while (true) {
    int start_layer = layers_str.find("{", end_layer + 1) + 1;
    end_layer = layers_str.find("}", start_layer);
    if ((start_layer == 0) || (end_layer < 0)) {
      if (layers_.size() == 0) {
        notify(NFY_WARN, "No layers found for cut.");
      }
      std::sort(layers_.begin(), layers_.end(), LayerLessThan);
      return;
    }

    const std::string layer = layers_str.substr(start_layer,
                                                end_layer - start_layer);

    type = ExtractValue(layer, "requestType");
    channel = ExtractValue(layer, "id");
    version = ExtractValue(layer, "version");

    notify(NFY_DEBUG, "type: %s", type.c_str());
    notify(NFY_DEBUG, "channel: %s", channel.c_str());
    notify(NFY_DEBUG, "version: %s", version.c_str());
    // Create a struct for each layer.
    layers_.push_back(new LayerInfo(type, channel, version));
  }
  std::sort(layers_.begin(), layers_.end(), LayerLessThan);
}


bool PortableMapBuilder::WriteMapPackets(const std::string& qtpath_str,
                                           std::uint32_t level,
                                           std::uint32_t col,
                                           std::uint32_t row) {
  #ifdef WRITE_MAP_PACKETS_OUTPUT
  // The following debug output is formatted for use in a spreadsheet.
  // It captures all calls to this function and indicates if the call
  // will result in an HTTP request.
  static bool first = true;
  if (first) {
    first = false;
    std::cout << "request,qtpath,level(z),col(x),row(y),packet.size" << std::endl;
  }
  #endif
  bool keep_node = KeepNode(qtpath_str);
  #ifdef WRITE_MAP_PACKETS_OUTPUT
  std::cout << (keep_node ? "true," : "false,") << qtpath_str << "," << level << "," << col << "," << row << ",";
  #endif
  if (!keep_node) {
    #ifdef WRITE_MAP_PACKETS_OUTPUT
    std::cout << "0" << std::endl;
    #endif
    return false;
  }

  QuadtreePath qtpath(qtpath_str);

  char url_str[256];
  const char* base_url =
      "%s/query?request=%s&channel=%s&version=%s&x=%d&y=%d&z=%d%s";
  bool found_something = ignore_imagery_depth_;
  for (size_t i = 0; i < layers_.size(); ++i) {
    snprintf(url_str, sizeof(url_str), base_url,
             source_.c_str(), layers_[i]->type.c_str(),
             layers_[i]->channel.c_str(), layers_[i]->version.c_str(),
             col, row, level + 1, additional_args_.c_str());
    std::string url = url_str;
    std::string raw_packet;
    server_->GetRawPacket(url, &raw_packet);
    #ifdef WRITE_MAP_PACKETS_OUTPUT
    std::cout << raw_packet.size() << std::endl;
    #endif
    std::uint32_t packet_type = layers_[i]->type_id;
    if (raw_packet.size() > 0) {
      if (packet_type == kImagePacket) {
        // If we are using imagery to decide if we have reached the end of
        // the data, then indicate that we have not yet reached the end.
        // If there are vectors below the max resolution imagery, we will
        // ignore them. This prevents us from following empty vectors
        // (1 x 1 clear pngs) to unnecessarily deep levels.
        found_something = true;
      }

      bounds_tracker_.update(layers_[i]->channel_num,
                             static_cast<PacketType>(packet_type),
                             qtpath_str);

      std::string full_qtpath = "0" + qtpath_str;
      writer_->AppendPacket(full_qtpath, packet_type, layers_[i]->channel_num,
                            raw_packet);
    }
  }

  return found_something;
}

/**
 * Returns whether the given quadtree node should be kept (i.e. not pruned).
 * If the node is at or below (lower resolution) the default level or if
 * it is in the specified high-resolution nodes, then it should not be
 * pruned.
 */
bool PortableMapBuilder::KeepNode(const std::string& qtpath) const {
  std::uint16_t level = qtpath.size();

  // If we are at or below the default level, keep everything.
  if (level <= default_level_) {
    return true;
  }

  // If we are above the max level, keep nothing.
  if (level > max_level_) {
    return false;
  }

  // Check the quadtree path to see if it is one we are keeping.
  return hires_tree_->IsTreePath(qtpath);
}


/**
 * Create directory if needed and create packet writer.
 */
void PortableMapBuilder::AddWriter(const std::string& sub_directory) {
  std::string directory = map_directory_ + sub_directory;
  khEnsureParentDir(directory + "/index");
  writer_ = new PacketBundleWriter(directory);
}

/**
 * Delete packet writer.
 */
void PortableMapBuilder::DeleteWriter() {
  delete writer_;
}

}  // namespace fusion_portableglobe

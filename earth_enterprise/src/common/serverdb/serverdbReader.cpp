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


#include <array>
#include "common/serverdb/serverdbReader.h"
#include "keyhole/jpeg_comment_date.h"
#include "common/quadtreepath.h"
#include "common/khEndian.h"
#include "common/khStringUtils.h"
#include "common/khFileUtils.h"
#include "common/khTileAddr.h"
#include "common/khConstants.h"
#include "common/khstrconv.h"
#include "common/tile_resampler.h"
#include "common/etencoder.h"
#include "common/gedbroot/proto_dbroot.h"
#include "common/proto_streaming_imagery.h"
#include "common/geGdalUtils.h"
#include "common/generic_utils.h"

// TODO this code is from fusion/JsUtils and needs to be refactored to remove code duplication.
namespace {
  std::string JsToJson(const std::string& in)
  {
    std::ostringstream oss;

    const std::string alphaNumSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789-abcdefghijklmnopqrstuvwxyz.";
    const std::string valueSet = "0123456789.eE";  // .eE should only appear once but let something else enforce that

    for (size_t i = 0; i < in.length(); ++i) {
      if (isalpha(in[i])) {
        static const std::array<std::string, 3> jsonLiterals = {"true", "false", "null"};
        bool isLiteral = false;
        for (const std::string& jlit : jsonLiterals) {
          if (!jlit.compare(in.substr(i, jlit.size()))) {
            oss << jlit;
            i += jlit.size()-1;
            isLiteral = true;
          }
        }

        if (!isLiteral) {
          size_t endEscape = in.find_first_not_of(alphaNumSet, i);
          oss << "\"" << in.substr(i, endEscape-i) << "\"";
          i = endEscape - 1;
        }
      }
      else if (isdigit(in[i])) {
        size_t endValue = in.find_first_not_of(valueSet, i);
        oss << in.substr(i, endValue-i);
        i = endValue - 1;
      }
      else if (in[i] == '\"') {
        size_t escapedStrEnd = in.find("\"", i+1);
        std::string temp = in.substr(i, escapedStrEnd-i+1);
        oss << in.substr(i, escapedStrEnd-i+1);
        i = escapedStrEnd;
      }
      else {
        oss << in[i];
      }
    }

    return oss.str();
  }
}

int ServerdbReader::CheckLevel(const geindex::TypedEntry::ReadKey &read_key,
                               std::uint32_t level, std::uint32_t row, std::uint32_t col) {
  if (level > MaxClientLevel) return 2;
  ServerdbReader::ReadBuffer buf;
  row = InvertRow(level, row);
  QuadtreePath qpath = QuadtreePath(level, row, col);
  QuadtreePath qpath_parent;
  geindex::TypedEntry entryReturn;

  bool testentry = index_reader_->GetNearestAncestorEntry(qpath, read_key,
                   &entryReturn, buf, &qpath_parent);
  int test_z = 0;
  if (testentry) {
    test_z = 1;
    if (qpath_parent != qpath) {
      test_z = 2;
    }
  }
  return test_z;
}

ServerdbReader::ServerdbReader(geFilePool& file_pool,
                               const std::string& serverdb_path,
                               int num_cache_levels,
                               bool enable_cutting)
  : serverdb_path_(serverdb_path),
    index_reader_(NULL),
    has_timemachine_layer_(false),
    is_cutting_enabled_(enable_cutting) {
  if (!config_.Load(serverdb_path_ + "/serverdb.config"))
    throw khSimpleException("Unable to load serverdb.config");

  // Create the index reader.
  index_reader_ = new geindex::UnifiedReader(file_pool, config_.index_path,
                                             num_cache_levels);
  if (config_.db_type == ServerdbConfig::TYPE_GEDB) {
    // Read the server-wide snippet dbroot
    geProtoDbroot server_snippets;
    if (khExists(kServerSnippetFile)) {
      server_snippets = geProtoDbroot(kServerSnippetFile,
                                      geProtoDbroot::kProtoFormat);
    }

    // Read all the binary dbroots and keep them in memory. This will speed up
    // the dbroot requests.
    ServerdbConfig::Map::const_iterator dbroot
      = config_.toc_paths.begin();
    for (; dbroot != config_.toc_paths.end(); ++dbroot) {
      // config_.toc_paths is map from alias to dbroot path
      const std::string& dbroot_alias = dbroot->first;
      const std::string dbroot_path = serverdb_path_ + "/" + dbroot->second;

      if (dbroot->second.find(".v5p.") != std::string::npos) {
        // proto dbroot. Load it into a geProtoDbroot object
        geProtoDbroot dbroot(dbroot_path, geProtoDbroot::kEncodedFormat);

        // merge in the server snippets
        dbroot.MergeFrom(server_snippets);

        // encode it for the client and store it in the map
        proto_toc_[dbroot_alias] = dbroot.ToEncodedString();
      } else {
        // old ETA dbroot. Just slurp it and put it in the map
        std::string buf;
        file_pool.ReadStringFile(dbroot_path, &buf);
        toc_[dbroot_alias] = buf;
      }
    }
    // Check for empty TOC.
    if (toc_.empty() && proto_toc_.empty()) {
      throw khSimpleException("No dbroots/layerdefs in DB: ") << serverdb_path_;
    }
    // Check for missing DEFAULT layer.js.
    if ((toc_.find(kDefaultLocaleSuffix) == toc_.end()) &&
        (proto_toc_.find(kDefaultLocaleSuffix) == proto_toc_.end())) {
      throw khSimpleException("Missing default dbroot: ") << serverdb_path_;
    }
  } else {  // Map DB
    // Read all the layers definition files and keep them in memory.
    ServerdbConfig::Map::const_iterator iter = config_.toc_paths.begin();
    for (; iter != config_.toc_paths.end(); ++iter) {
      std::string buf;
      file_pool.ReadStringFile(serverdb_path_ + "/" + iter->second, &buf);
      toc_[iter->first] = buf;
    }
    // Check for empty TOC.
    if (toc_.empty()) {
      throw khSimpleException("No dbroots/layerdefs in DB: ") << serverdb_path_;
    }
    // Check for missing DEFAULT layer.js.
    iter = toc_.find(kDefaultLocaleSuffix);
    if (iter == toc_.end()) {
      throw khSimpleException("Missing default layer.js: ") <<
        kDefaultLocaleSuffix;
    }

    // TODO: preload search.json for maps API and supplemental
    // search?
    // Temporary switched off, since we don't build searchtabs.js in new
    // publisher.
    // file_pool.ReadStringFile(serverdb_path_ + "/" + config_.searchtabs_path,
    //                          &searchtabs_js_);
  }

  // Load the JSON data.
  ServerdbConfig::Map::const_iterator iter = config_.json_paths.begin();
  for (; iter != config_.json_paths.end(); ++iter) {
    std::string buf;
    file_pool.ReadStringFile(serverdb_path_ + "/" + iter->second, &buf);
    json_[iter->first] = buf;
  }

  // Load the icons from the icons dir in memory as well.
  std::vector<std::string>::const_iterator icon = config_.icons.begin();
  for (; icon != config_.icons.end(); ++icon) {
    std::string buf;
    file_pool.ReadStringFile(serverdb_path_ + "/" + *icon, &buf);
    icons_[*icon] = buf;
  }

  // Determine if this dbRoot has time machine.
  // If a db has time machine enabled, it will include "timemachine"
  // spec file which maps hex date tags to channel ids (imagery layers).
  // f3815=1001.
  // The date tags follow the timemachine protocol using JpegCommentDateTime
  // hex format.
  // The dbRoot must also have the isTimeMachine key set to true to serve
  // time machine tiles...by default we will attempt to serve the base layer
  // tiles if timemachine is not enabled.
  std::string date_channel_map_file_name = config_.index_path + "/" +
    kDatedImageryChannelsFileName;
  if (khExists(date_channel_map_file_name)) {
    std::string buf;
    file_pool.ReadStringFile(date_channel_map_file_name, &buf);
    std::vector<std::string> tokens;
    const std::string kDelimiters(" =\n");
    std::uint32_t max_tokens = 1000000;  // arbitrary limit. If this is exceeded
                                  // we will have other memory issues.
    TokenizeString(buf, tokens, kDelimiters, max_tokens);

    for (unsigned int i = 0; i < tokens.size(); i += 2) {
      if (i + 1 >= tokens.size() || tokens[i].empty() || tokens[i+1].empty())
        break;  // invalid line, ignore.

      const std::string& date_tag = tokens[i];
      std::uint32_t channel_id = std::atoi(tokens[i+1].c_str());

      std::string date_time_hex;
      std::string date_hex;
      // Timestring "date_tag" should be in valid UTC ISO 8601 string format:
      //  YYYY-MM-DDTHH:MM::SSZ or just date YYYY-MM-DD.
      // Upon parse failure, date/time is set to unknown.
      TimeToHexString(date_tag, &date_time_hex, &date_hex);

      // Latest earth client will request with timestamp as
      // "Date.Time" (if time > 0) or "Date" (if time == 0).
      date_to_channel_map_[date_time_hex] = channel_id;

      // Old earth client (7.1.7 or older) will request with timestamp as
      // "Date" only and ignore the "Time" part, so maintain compatibility
      // by linking channel_id with date only for the earliest timestamp
      // of the day.
      if (date_to_channel_map_.find(date_hex) == date_to_channel_map_.end()) {
        date_to_channel_map_[date_hex] = channel_id;
      }

      has_timemachine_layer_ = true;  // need at least one valid date.
    }
    // Add a default map for the oldest date, which really means the default.
    // Only if the db didn't generate it already.
    if (date_to_channel_map_.count(kTimeMachineOldestHexDateString) == 0) {
      date_to_channel_map_[kTimeMachineOldestHexDateString] =
        kGEImageryChannelId;
    }
  }
}

ServerdbReader::~ServerdbReader() {
  if (index_reader_)
    delete index_reader_;
}

// Tries to parse a std::uint32_t from the given string. If there is junk after the
// int, we expect this not to fail and for atoi to just return the initial int
// from the string. If there is no legal value, 0 is returned.
void ServerdbReader::GetUint32Arg(const std::string& str, std::uint32_t* arg) const {
  *arg = 0;
  // Empty string is interpreted as 0.
  if (str.empty()) {
    return;
  }

  // Skip white space.
  const char* str_ptr = str.c_str();
  while (*str_ptr == ' ') {
    ++str_ptr;
  }

  // Convert to a unsigned int if the value is a plausible integer, otherwise return 0.
  if ((*str_ptr >= '0') && (*str_ptr <= '9')) {
    *arg = atoi(str_ptr);
  }
}

// Throws an exception if any character that could be used to try to execute
// code from a var declaration is in the name string.
void ServerdbReader::CheckJavascriptName(const std::string& name) const {
  for (size_t i = 0; i < name.length(); ++i) {
    // Probably only need: semicolon and equals
    if ((name[i] == ';') ||
        (name[i] == '=') ||
        (name[i] == '(') ||
        (name[i] == ' ')) {
      throw khSimpleException("Illegal character found in JS name: ") << name;
    }
  }
}

const MimeType ServerdbReader::GetData(
    std::map<std::string, std::string>& arg_map,
    ReadBuffer& buf,
    bool& is_cacheable) {
  // TODO: fix this and remove wasteful tokenizing and string
  // copying/comparing for just a handful of very specific formats.
  // Also, try to handle multiple flatfile requests (v4.4 of GEClient handles
  // them by concatenating with "+")
  // perhaps break this up into more specific calls for each request type.
  // to avoid unnecessary processing.
  // We have 3 types of requests:
  // 1) flatfile* : 3D
  // 2) query* : maps, json
  // 3) dbRoot*
  if (!is_cutting_enabled_) {
    if (arg_map["ct"] == std::string("c")) {
      throw khSimpleException("Bad trial to cut from globe.");
    }
  }
  std::string request = arg_map["request"];
  if (request.empty()) {
    throw khSimpleException("Request was not specified.");
  }

  std::map<std::string, std::string>::iterator i = arg_map.find("size");
  const bool size_only = (i != arg_map.end()) && i->second == "1";

  is_cacheable = false;
  if (request == kQtPacketType || request == kTerrainType ||
      request == kVectorGeType || request == kVectorMapsType ||
      request == kVectorMapsRasterType || request == kQtPacketTwoType ||
      request == "ImageryGE" ||
      request == "ImageryMaps" || request == "ImageryMapsMercator") {
    is_cacheable = true;
    try {
      // Check for time machine layers, ImageryGE Only
      std::string date_tag = arg_map["date"];
      std::uint32_t channel;
      GetUint32Arg(arg_map["channel"], &channel);
      if (has_timemachine_layer_ &&
          !date_tag.empty() &&
          request == "ImageryGE") {
        // Get the channel id for this imagery layer.
        std::map<std::string, std::uint32_t>::const_iterator iter =
          date_to_channel_map_.find(date_tag);
        if (iter == date_to_channel_map_.end()) {
          channel = kGEImageryChannelId;  // Default to default imagery layer.
        } else {
          channel = iter->second;
        }
      }
      // Get format parameter (Imagery*Maps* requests).
      const std::string& format = arg_map["format"];
      std::uint32_t version;
      GetUint32Arg(arg_map["version"], &version);
      std::uint32_t level;
      GetUint32Arg(arg_map["level"], &level);
      std::uint32_t row;
      GetUint32Arg(arg_map["row"], &row);
      std::uint32_t col;
      GetUint32Arg(arg_map["col"], &col);
      std::string blist = arg_map["blist"];
      if ((blist.empty()) || (blist.back() != kQuadTreePathSeparator)) {
        return GetPacket(request, blist, level, row, col, version, channel, buf,
                         size_only, format,
                         arg_map["ct"] == std::string("c"));
      } else {
        // Note: Special processing for cutter when it requests more than one
        // quadtree packet. It is only for 3D Fusion databases.
        PackedString str;
        std::uint64_t total_size = 0;
        for (size_t start = 0; start < blist.length(); ) {
          size_t last = blist.find_first_of(kQuadTreePathSeparator, start);
          buf.SetValue("");
          std::string qt_path = blist.substr(start, last - start);
          try {
            GetPacket(request, qt_path, level, row, col, version, channel, buf,
                      size_only, format);
          } catch (...) {
            // catch all exceptions since we return status by "" return value
            // and since we need to cover all requests packed in the blist too.
          }
          if (size_only) {
            std::uint64_t size = 0;
            StringToNumber(buf, &size);
            total_size += size;
          } else {
            str.Append(buf);
          }
          start = last + 1;  // Skip the 4
        }
        if (size_only) {
          buf.SetValue(NumberToString(total_size));
        } else {
          buf.SetValue(str);
        }
        return MapTileUtils::kOctetMimeType;
      }
    } catch (khSimpleNotFoundException& e) {
      // We want to allow for custom request return behavior for "not found"
      // tiles/packets in certain circumstances.
      if (request == kVectorMapsRasterType) {
        // For empty map tiles we want to redirect to transparent.png's
        buf.assign(MapTileUtils::GetNoDataTilePath(MapTileUtils::kPngMimeType));
        return MapTileUtils::kRedirectMimeType;
      } else if (request == "ImageryMaps" || request == "ImageryMapsMercator") {
        // For empty map imagery tiles we want to return "HTTP_NOT_FOUND"
        // specifically, which is not what we normally return.
        return MapTileUtils::kNotFoundMimeType;
      } else if (request == "ImageryGE") {
        // This should never happen because client only requests existing
        // quad-tree packets.
        // But, since the index reader may raise khSimpleNotFoundException,
        // let's catch it here to properly handle this case.
        return MapTileUtils::kNotFoundMimeType;
      } else {
        throw e;
      }
    }
  } else if (request == "Icon") {
    std::string icon_path = arg_map["icon_path"];
    GetIcon(icon_path, buf, size_only);
    return MapTileUtils::kPngMimeType;
  // Please note that for the rest of the requests we don't look for size_only
  } else if (request == "SearchTabs") {
    // TODO: delete SearchTabs request handling or use it to
    // get preloaded search.json?
    // buf.SetValue(searchtabs_js_);
    buf.SetValue("DEPRECATED.");
    return MapTileUtils::kJavascriptMimeType;
  } else {
    std::string language = arg_map["hl"];
    std::string region = arg_map["gl"];
    std::string output = arg_map["output"];
    if (request == "Dbroot") {
      std::string db = arg_map["db"];
      if (db == "tm") {  // TimeMachine DbRoot request.
        language = kTimeMachineTOCSuffix;
        region = std::string();
      }
      if (GetToc(language, region, output, buf)) {
        return MapTileUtils::kOctetMimeType;
      } else {
        return MapTileUtils::kNotFoundMimeType;
      }
    } else if (request == "Json") {
      std::string json_variable_name = arg_map["var"];
      if (!arg_map["v"].empty()) {
        // CheckJavascriptName(json_variable_name);
        if (arg_map["v"] == "2") {
          GetJson(json_variable_name, language, region, buf);
        } else if (arg_map["v"] == "1") {
          // This is added for backwards compatibility reasons.
          GetJsonV1(json_variable_name, language, region, buf);
        } else {
          throw khSimpleException("Invalid argument found in v parameter: " + arg_map["v"] + ". Valid values are 1 and 2.");
        }
        return MapTileUtils::kJavascriptMimeType;
      } else {
        GetJsonV1(json_variable_name, language, region, buf);
        return MapTileUtils::kJavascriptMimeType;
      }
    } else if (request == "LayerDefs") {
      if (GetToc(language, region,
                 std::string() /* output= not supported for LayerDefs */,
                 buf)) {
        return MapTileUtils::kJavascriptMimeType;
      } else {
        return MapTileUtils::kNotFoundMimeType;
      }
    }
  }

  // Should be unreachable
  return MapTileUtils::kUnknownMimeType;
}

const MimeType ServerdbReader::GetPacket(
    const std::string& request,
    const std::string& blist,
    std::uint32_t level,
    std::uint32_t row,
    std::uint32_t col,
    std::uint32_t version,
    std::uint32_t channel,
    ReadBuffer& buf,
    const bool size_only,
    const std::string& format,
    const bool is_cutter) {
  // TODO: break into 2 or 3 separate calls that are more testable.
  // GetPacketEncrypted
  // GetPacketMaps
  // remove VectorMaps (not used!)
  // focused...request_type should be known by this point.
  QuadtreePath qpath(blist);
  bool version_matters = true;
  bool use_etcrypt = !size_only;
  MimeType content_type = MapTileUtils::kUnknownMimeType;

  geindex::TypedEntry::TypeEnum request_type;
  if (request == kQtPacketType) {
    request_type = geindex::TypedEntry::QTPacket;
    // Currently we do not care for the quadtree packet version requested till
    // we change the quadtree packet format to include versions.
    version_matters = false;
  } else if (request == kQtPacketTwoType) {
    // Proto Buf version of QuadTree packets.
    request_type = geindex::TypedEntry::QTPacket2;
    // Currently we do not care for the quadtree packet version requested till
    // we change the quadtree packet format to include versions.
    version_matters = false;
  } else if (request == "ImageryGE") {
    // Even though for the index there is just a single Imagery type we need to
    // distinguish between the two requests. We etEncode one and not the other.
    request_type = geindex::TypedEntry::Imagery;

    // Serving imagery tiles from either GE protobuf format or raw imagery
    // format (jpeg, png). A successful request will return either a jpeg/png
    // tile, or a protocol buffer.
    // The request type, format- parameter and type of a source packet define
    // which is expected.
    // ImageryMaps - returns a jpeg/png tile.
    // ImageyMapsMercator - returns a jpeg/GE protobuf tile.
  } else if (request == "ImageryMaps" ||
             request == "ImageryMapsMercator") {
    // The javascript client asks for rows in reverse order (MAPS API req.)
    row = InvertRow(level, row);
    qpath = QuadtreePath(level, row, col);
    bool report_in_source_format = (request == "ImageryMapsMercator");
    // For Maps Imagery, we want to resample from the closest parent tile in
    // the quadtree if we have a missing tile.
    // Note: No need for encoding (2d).
    return GetResampledImageryMapsTile(qpath, version, channel, &buf,
                                       report_in_source_format, format,
                                       is_cutter);
  } else if (request == kTerrainType) {
    request_type = geindex::TypedEntry::Terrain;
  } else if (request == kVectorGeType) {
    request_type = geindex::TypedEntry::VectorGE;
  } else if (request == kVectorMapsType) {
    request_type = geindex::TypedEntry::VectorMaps;
    use_etcrypt = false;  // Don't encrypt for Maps.
    row = InvertRow(level, row);
    qpath = QuadtreePath(level, row, col);
  } else if (request == kVectorMapsRasterType) {
    request_type = geindex::TypedEntry::VectorMapsRaster;
    use_etcrypt = false;  // Don't encrypt for Maps.
    content_type = MapTileUtils::kPngMimeType;
    row = InvertRow(level, row);
    qpath = QuadtreePath(level, row, col);
  } else {
    throw khSimpleException("Invalid request: " + request);
  }

  geindex::TypedEntry::ReadKey read_key(version, channel, request_type,
                                        version_matters);

  // The Reader::GedData() throws khSimpleNotFoundException on failure.
  index_reader_->GetData(qpath, read_key, buf, size_only);

  // TODO: Remove this code when we no longer support
  //                  backwards compatibility with non-pb packets.
  // If imagery is not in a protocol buffer, put it into one.
  // We could check by trying to parse the protocol buffer, but it is
  // much cheaper to just check if it is in one of the expected
  // imagery formats.
  if (request_type == geindex::TypedEntry::Imagery) {
    const char* buffer = buf.data();
    geEarthImageryPacket imagery_pb;
    bool found_non_pb_image = false;
    // Check for jpeg.
    if (gecommon::IsJpegBuffer(buffer)) {
      imagery_pb.SetImageType(::keyhole::EarthImageryPacket::JPEG);
      found_non_pb_image = true;
    // Not expecting a png, but might as well check.
    } else if (gecommon::IsPngBuffer(buffer)) {
      imagery_pb.SetImageType(::keyhole::EarthImageryPacket::PNG_RGBA);
      found_non_pb_image = true;
    }
    if (found_non_pb_image) {
      imagery_pb.SetImageData(buffer, buf.size());
      buf.resize(imagery_pb.ByteSize());
      imagery_pb.SerializeToArray(&buf[0], buf.size());
      content_type = MapTileUtils::kOctetMimeType;
    }
  }

  if (use_etcrypt) {
    content_type = MapTileUtils::kOctetMimeType;
    // etEncode this packet.
    etEncoder::EncodeWithDefaultKey(&buf[0], buf.size());
  } else if (size_only) {
    content_type = MapTileUtils::kOctetMimeType;
  }

  return content_type;
}

void ServerdbReader::GetNearestAncestorData(const QuadtreePath& qpath,
                                            std::uint32_t version,
                                            std::uint32_t channel,
                                            ReadBuffer* buf,
                                            QuadtreePath *qpath_parent) {
  bool version_matters = version != 0;
  geindex::TypedEntry::ReadKey read_key(version, channel,
                                        geindex::TypedEntry::Imagery,
                                        version_matters);

  // Get the requested buffer, but if it doesn't exist then traverse up to find
  // the nearest ancestor quade node and return that tile and the quadtree path.
  // The GetNearestAncestorData throws khSimpleNotFoundException on failure.
  index_reader_->GetNearestAncestorData(qpath, read_key, *buf, qpath_parent);
}

const MimeType ServerdbReader::GetResampledImageryMapsTile(
    const QuadtreePath& qpath,
    std::uint32_t version,
    std::uint32_t channel,
    ReadBuffer* buf,
    const bool report_in_source_format,
    const std::string& format,
    const bool is_cutter) {
  // For Maps Imagery, we want to resample if we have a missing tile.
  QuadtreePath qpath_parent;
  GetNearestAncestorData(qpath, version, channel, buf, &qpath_parent);

  // If the tile we got is not from the requested path and we are in the Cutter,
  // don't resample because we can do this in the Portable Server and don't need
  // to use up space in the cut maps for it.
  if (is_cutter && qpath_parent != qpath)  {
    buf->clear();
    return MapTileUtils::kNotFoundMimeType;
  }

  // Function throws khSimpleNotFoundException on failure.
  return MapTileUtils::PrepareImageryMapsTile(qpath_parent, qpath, buf,
                                              report_in_source_format,
                                              format);
}

void ServerdbReader::GetJson(const std::string& json_variable_name,
                             const std::string& language,
                             const std::string& region,
                             ReadBuffer& buf) {
  const std::string& json = GetLocaleKey(json_, language, region);
  if (json_variable_name.empty()) {
    // Return raw JSON
    buf.SetValue(JsToJson(json));
  } else {
    // Return a Javascript version of the JSON with the variable name declared.
    std::string javascript = "var " + json_variable_name + " = " + json + ";\n";
    buf.SetValue(javascript);
  }
}

void ServerdbReader::GetJsonV1(const std::string& json_variable_name,
                             const std::string& language,
                             const std::string& region,
                             ReadBuffer& buf) {
  const std::string& json = GetLocaleKey(json_, language, region);
  if (json_variable_name.empty()) {
    // Return raw JSON
    buf.SetValue(json);
  } else {
    // Return a Javascript version of the JSON with the variable name declared.
    std::string javascript = "var " + json_variable_name + " = " + json + ";\n";
    buf.SetValue(javascript);
  }
}

namespace {
// global const so we don't have to allocate it each time GetToc is called.
// NOT a static member of GetToc just in case our old compiler doesn't generate
// MT-safe initialization for static function variables.
const std::string kProtoOutputString("proto");
}

bool ServerdbReader::GetToc(const std::string& language,
                            const std::string& region,
                            const std::string& output,
                            ReadBuffer& buf) {
  std::string toc_value;
  if (output == kProtoOutputString) {
    toc_value = GetLocaleKey(proto_toc_, language, region);
  } else {
    toc_value = GetLocaleKey(toc_, language, region);
  }
  if (!toc_value.empty()) {
    buf.SetValue(toc_value);
    return true;
  }
  return false;
}

// TODO: this needs to be greatly simplified
// it currently is retokenizing all the TOC for every request.
// should check the standard one and return if exact match, then start doing
// something smart (but tokenization should be precomputed for each TOC).
const std::string& ServerdbReader::GetLocaleKey(
  const ServerdbConfig::Map& map,
  const std::string& language,
  const std::string& region) {
  // Different version of the client/fusion used "-" or "_" as separators.
  // We continue to support both separators here:
  const std::string kLocaleSeparators = "_-";

  // Find all the "pieces" that are part of the request:
  // Depending on GE client version language can be just one piece ("it") or
  // two pieces ("it_IT") and region can either be empty or something like "us"
  // depending on where the GE client is running.
  // We expect request of the form:
  //    language = "it", region = "IT"
  // or language = "it-IT", region = ""
  // or language = "it-IT", region = "us"
  // We would not expect:
  //    language = "it-IT", region = "IT"
  // Comparisons are perfomed case insensitive.
  std::vector<std::string> requestTokens;
  TokenizeString(UpperCaseString(language), requestTokens, kLocaleSeparators);
  requestTokens.push_back(UpperCaseString(region));

  // Find the "best" match between the requested dbroot and the ones
  // that we have available. Best means that it is either an exact match
  // (prefered) or the largest number of "pieces" match. If there are
  // multiple dbRoot that are "best", we take the first one we find:
  std::uint32_t maxNumberOfMatchingPieces = 0;
  ServerdbConfig::Map::const_iterator bestMatchingDbroot =
    map.find(kDefaultLocaleSuffix);
  std::map<std::string, std::string>::const_iterator iter;
  for (iter = map.begin(); iter != map.end(); ++iter) {
    std::vector<std::string> tokens;
    TokenizeString(UpperCaseString(iter->first), tokens, kLocaleSeparators);
    std::uint32_t numberOfMatchingPieces = 0;
    for (std::uint32_t i = 0; i < requestTokens.size(); ++i) {
      if (i >= tokens.size()) {
        break;
      }
      if (requestTokens[i] == tokens[i]) {
        numberOfMatchingPieces++;
      } else {
        break;
      }
    }
    // A precise match of all pieces is always prefered:
    if (requestTokens.size() == tokens.size() &&
        numberOfMatchingPieces == tokens.size()) {
      return iter->second;
    }
    if (numberOfMatchingPieces > maxNumberOfMatchingPieces) {
      maxNumberOfMatchingPieces = numberOfMatchingPieces;
      bestMatchingDbroot = iter;
    }
  }

  // Return the best match we found:
  // Now that the server respects output= on dbroot requests it's quite
  // possible that we don't even have a default toc for this particular
  // toc map (e.g. proto or non-proto). SO make sure we don't dereference
  // an invalid iterator
  if (bestMatchingDbroot != map.end()) {
    return bestMatchingDbroot->second;
  }
  return kEmptyString;
}


void ServerdbReader::GetIcon(const std::string& icon_path, ReadBuffer& buf,
                             const bool size_only) {
  std::map<std::string, std::string>::const_iterator icon =
      icons_.find(icon_path);
  if (icon != icons_.end()) {
    if (size_only) {
      std::ostringstream stream;
      stream << icon->second.length();
      buf.SetValue(stream.str());
    } else {
      buf.SetValue(icon->second);
    }
  } else {
    throw khSimpleException(
        "Unable to read icon:" + serverdb_path_ + "/" + icon_path);
  }
}


 std::uint32_t ServerdbReader::InvertRow(std::uint32_t level, std::uint32_t row) {
  return (1 << level) - row - 1;
}

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


#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_SERVERDB_SERVERDBREADER_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_SERVERDB_SERVERDBREADER_H_

#include <string>
#include <map>
#include "common/geindex/Reader.h"
#include "common/serverdb/serverdb.h"
#include "common/serverdb/map_tile_utils.h"

class ServerdbReader {
 public:
  typedef geindex::UnifiedReader::ReadBuffer ReadBuffer;

  // Checks tile request for level.
  // Returns:
  //   0 - no data found;
  //   1 - requested tile exists in database;
  //   2 - requested tile is not available but there is tile at lower
  //       resolution level covering requested one.
  int CheckLevel(const geindex::TypedEntry::ReadKey &read_key, std::uint32_t level,
                 std::uint32_t row, std::uint32_t col);

  // serverdb_path is a path to Fusion database.
  // num_cache_levels is number of cache levels.
  // enable_cutting is whether cutting from this Reader is enabled.
  ServerdbReader(geFilePool& file_pool,
                 const std::string& serverdb_path,
                 int num_cache_levels,
                 bool enable_cutting);
  ~ServerdbReader();

  // Tries to parse a std::uint32_t from the given string. If there is junk after the
  // int, we expect this not to fail and for atoi to just return the initial int
  // from the string.
  // Returns 0 in *arg if unsigned int cannot be parsed from the string.
  // str: The string from which the unsigned int will be parsed.
  // arg: Pointer that will receive unsigned int if it is successfully parsed.
  void GetUint32Arg(const std::string& str, std::uint32_t* arg) const;

  // Throws an exception if any character that could be used to try to execute
  // code from a Javascript var declaration is in the name string. Intended
  // to prevent cross-site scripting (XSS) attacks.
  // name: Name of variable being declared in Javascript (JSON).
  void CheckJavascriptName(const std::string& name) const;

  // Returns a buffer and content type of the http response for a http query for
  // a specific packet. In case of errors:
  // content_type == "redirect" => place the redirect url/local filename in buf
  // content_type == "not found" => a HTTP_NOT_FOUND error to be returned
  // Otherwise, this method may throw an exception indicating some other
  // error.
  const MimeType GetData(std::map<std::string, std::string>& arg_map,
                         ReadBuffer& buf,
                         bool& is_cacheable);

  std::string GetDbPath() const { return serverdb_path_; }

  // Set whether cutting can be done using this reader.
  void SetCuttingEnabled(bool flag) { is_cutting_enabled_ = flag; }

 private:
  // Returns content type and the content value (in buf).
  // request - http request.
  // blist - quadtree-path.
  // level - zoom level.
  // row - latitude representative.
  // col - longitude representative.
  // version - version of DB (Epoch in rest of GEO).
  // channel - type of data (e.g imagery/terrain/vector/other).
  // buf - Output buffer.
  // size_only - whether to return(by buf) only the size of the packet.
  // format - requested format for imagery tiles: jpeg, png. An empty string
  // means report as is.
  // is_cutter - whether request for packet came from a Cutter.
  //
  const MimeType GetPacket(const std::string& request,
                           const std::string& blist,
                           std::uint32_t level,
                           std::uint32_t row,
                           std::uint32_t col,
                           std::uint32_t version,
                           std::uint32_t channel,
                           ReadBuffer& buf,
                           const bool size_only,
                           const std::string& format = std::string(),
                           const bool is_cutter = false);

  // Get the table of contents (i.e., dbroot for GE Dbs or layerdefs.js from
  // Maps Dbs) text buffer associated with the given language and region.
  bool GetToc(const std::string& language, const std::string& region,
              const std::string& output,
              ReadBuffer& buf);
  // Get the JSON text buffer associated with the given language and region.
  // json_variable_name : an optional variable name to be prepended on the JSON
  //                      which allows it to be loaded as a JS variable.
  // buf : buffer for the ouptut JSON/Javascript text.
  void GetJson(const std::string& json_variable_name,
               const std::string& language,
               const std::string& region,
               ReadBuffer& buf);
  //
  void GetJsonV1(const std::string& json_variable_name,
                 const std::string& language,
                 const std::string& region,
                 ReadBuffer& buf);
  void GetIcon(const std::string& icon_path, ReadBuffer& buf,
               const bool size_only);

  // Gets resampled tile for ImageryMaps[Mercator] requests. The function
  // accepts JPEG, PNG or GE protobuf imagery packets in buffer.
  // Note: For Maps Imagery, we want to resample from the closest parent tile
  // in the quadtree if we have a missing tile.
  const MimeType GetResampledImageryMapsTile(const QuadtreePath& qpath,
                                             std::uint32_t version,
                                             std::uint32_t channel,
                                             ReadBuffer* const buf,
                                             const bool report_in_source_format,
                                             const std::string& format,
                                             const bool is_cutter = false);

  // Wrapper on GetNearestAncestorData of UnifiedReader (index reader).
  // version - version of DB (Epoch in rest of GEO)
  // channel - type of data (e.g imagery/terrain/vector/other)
  // buf - Output buffer
  // qpath_parent - quadtree path of tile loaded to output buffer.
  void GetNearestAncestorData(const QuadtreePath& qpath,
                              std::uint32_t version,
                              std::uint32_t channel,
                              ReadBuffer* const buf,
                              QuadtreePath *qpath_parent);

  std::uint32_t InvertRow(std::uint32_t level, std::uint32_t row);

  const std::string serverdb_path_;
  ServerdbConfig config_;
  ServerdbConfig::Map proto_toc_;
  ServerdbConfig::Map toc_;
  ServerdbConfig::Map json_;
  std::map<std::string, std::string> icons_;
  geindex::UnifiedReader* index_reader_;
  std::string searchtabs_js_;
  bool has_timemachine_layer_;
  std::map<std::string, std::uint32_t> date_to_channel_map_;  // Map of date string
                                                       // values to channel id
                                                       // for time machine
                                                       // layers.
  bool is_cutting_enabled_;

 protected:
  friend class ServerdbReader_Test;
  // static utility to make unit testing of language-region keys simpler.
  // map: a map of local keys to buffer strings
  // language: the requested language abbreviation (e.g., "en" or "fr").
  // region: the requested region two letter abbreviation (e.g., "us" or "uk").
  // returns a REFERENCE to the single copy of the dbroot that's loaded in the
  //         constructor.
  //         The dbroot chosen is the best match between the requested
  //         locale and the locale's in the map.
  //         If no map at all is found, a reference to a global empty string
  //         is returned
  // Details:
  // We expect request of the form:
  //    language = "it", region = "IT"
  // or language = "it-IT", region = ""
  // or language = "it-IT", region = "us"
  // We would not expect:
  //    language = "it-IT", region = "IT"
  // Comparisons are perfomed case insensitive.
  static const std::string& GetLocaleKey(const ServerdbConfig::Map& map,
                               const std::string& language,
                               const std::string& region);
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_SERVERDB_SERVERDBREADER_H_

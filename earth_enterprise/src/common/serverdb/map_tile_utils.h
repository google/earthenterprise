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

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_SERVERDB_MAP_TILE_UTILS_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_SERVERDB_MAP_TILE_UTILS_H_

#include <string>

#include "common/quadtreepath.h"

/*
 * Class for holding mime type literals. These need to be static so that they
 * can be passed to Apache and used at any time afterwards but never
 * deallocated.
 */
class MimeType {
 public:
  explicit MimeType(const char* content_type) : literal(content_type) {}
  const char* GetLiteral() const {return literal;}
  bool Equals(MimeType other) const {return other.GetLiteral() == literal;}
 private:
  MimeType() {}
  const char* literal;
};

// Utilities to prepare Imagery map tile.
class MapTileUtils {
 public:
  // Mime Types (literals used for Content-type in header)
  static const MimeType kUnknownMimeType;
  static const MimeType kOctetMimeType;
  static const MimeType kJpegMimeType;
  static const MimeType kPngMimeType;
  static const MimeType kJavascriptMimeType;
  static const MimeType kRedirectMimeType;
  static const MimeType kNotFoundMimeType;

  // No data jpeg and png files used for redirecting when serving ImageryMaps
  // tiles.
  static const char* const kNoDataJpg;
  static const char* const kNoDataPng;
  static const char* const kOutOfCutJpg;
  static const char* const kOutOfCutPng;

  // Returns path to no data jpeg or png file depends on specified format.
  static inline const char* GetNoDataTilePath(const std::string& format) {
    return (format == kPngMimeType.GetLiteral()) ? kNoDataPng : kNoDataJpg;
  }
  // Returns path to no data jpeg or png file depends on specified mimetype.
  static inline const char* GetNoDataTilePath(const MimeType& mimetype) {
    return mimetype.Equals(kPngMimeType) ? kNoDataPng : kNoDataJpg;
  }

  // Returns path to 'out of cut' jpeg or png file depends on specified format.
  static inline const char* GetOutOfCutTilePath(const std::string& format) {
    return (format == kPngMimeType.GetLiteral()) ? kOutOfCutPng : kOutOfCutJpg;
  }
  // Returns path to 'out of cut' jpeg or png file depends on specified
  // mimetype.
  static inline const char* GetOutOfCutTilePath(const MimeType& mimetype) {
    return mimetype.Equals(kPngMimeType) ? kOutOfCutPng : kOutOfCutJpg;
  }

  // Prepares ImageryMaps tile: resample when it is needed, convert to requested
  // output format.
  // qpath_parent: parent quad tree path if requested tile doesn't exist.
  // qpath: quad tree path of requested tile.
  // buf: the input and output buffer. Contains the jpeg/png/GEprotobuf tile
  //     on input and jpeg/png/GEprotobuf tile on output.
  // report_in_source_format: whether to report in source format {jpeg,
  //     GEprotobuf}.
  // format: requested output format {"image/png", "image/jpeg"}.
  //     If format is not specified (an empty string), write in either
  //     jpeg-format (there is no alpha-data) or png-format (there is
  //     alpha-data).
  // Returns prepared imagery tile in input buffer.
  // Note: function will throw khSimpleNotFoundException on failure.
  static const MimeType PrepareImageryMapsTile(
      const QuadtreePath& qpath_parent,
      const QuadtreePath& qpath,
      std::string* const buf,
      const bool report_in_source_format,
      const std::string& format);

  static inline const MimeType PrepareImageryMapsTile(
      const std::string& _qpath_parent,
      const std::string& _qpath,
      std::string* const buf,
      const bool report_in_source_format,
      const std::string& format) {
    QuadtreePath qpath_parent(_qpath_parent.c_str());
    QuadtreePath qpath(_qpath.c_str());
    return PrepareImageryMapsTile(qpath_parent,
                                  qpath,
                                  buf,
                                  report_in_source_format,
                                  format);
  }
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_SERVERDB_MAP_TILE_UTILS_H_

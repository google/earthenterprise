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


#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_KHCONSTANTS_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_KHCONSTANTS_H_

#include <string>

#include "common/khTypes.h"
#include <cstdint>


/******************************************************************************
Description:    This header will be the central place for all the constants
                                used in fusion/server.

******************************************************************************/

//  - Taken from USGS publication: "Map Projections -- A Working Manual", pg 12
static const double khWGS84EquatorialRadius = 6378137.0;

// NASA's mean 6371.01+/-0.02 km
static const double khEarthMeanRadius = 6371010.0;

// It really doesn't matter what this number is so long as it matches the
// client. 2.2 clients had 6371000 hard coded, so we'll pick the mean which
// is really close. Earlier and later clients look at the dbRoot for the
// planet radius. The dbRoot had better match this number.
static const double khEarthRadius = khEarthMeanRadius;

// Clients prior to 5.0 don't support negative elevations.
// Its supported in 5.0 as follows - If we want to store -X as an elevation we really store:
// X/2^kNegativeElevationExponentBias (i.e., a really small positive value).
// Older clients (pre 5.0) do not know about the dbRoot fields to transform
// these values to a negative elevation so they are practically 0.
// so for every elevation < kCompressedNegativeAltitudeThreshold, the client
// will convert El = -El_0 * 2^kNegativeElevationExponentBias
// note, there may be some Fusion 3.1 db's running around still using small
// negative numbers for negative elevation. The terrain db's should be rebuilt.
static const int kNegativeElevationExponentBias = 32;
static const double kCompressedNegativeAltitudeThreshold = 1e-12;

// U.S. Survey foot 1200/3937
static const double khUsSurveyFoot = 0.30480060960121920243;
static const double khIntlFoot = 0.3048;

// Typical compression ratio JPEG(Q75): at least 10:1, PNG: at least 1.5:1.
// We'll allow for 5x for jpeg just to make sure we have enough space.
static const std::uint32_t kJpegCompressionRatio = 5;
// PNG compression ratio for RGBA image.
static const std::uint32_t kPngCompressionRatio = 2;
// PNG compression ratio for alpha band: assume at least 10:1.
// We use 4x to be sure that we allocate large enough memory.
static const std::uint32_t kPngCompressionAlphaRatio = 4;

// GE hardcoded channel numbers
static const std::uint32_t kGEImageryChannelId = 0;
static const std::uint32_t kGETerrainChannelId = 2;

// Default Locale Suffix.
extern const std::string kDefaultLocaleSuffix;
extern const std::string kDatabaseSuffix;
// Default suffix for imagery resource asset directories.
extern const std::string kImageryAssetSuffix;
extern const std::string kImageryProjectSuffix;
extern const std::string kMapDatabaseSuffix;
extern const std::string kMapLayerSuffix;
extern const std::string kMapProjectSuffix;
extern const std::string kMercatorImageryAssetSuffix;
extern const std::string kMercatorImageryProjectSuffix;
extern const std::string kMercatorMapDatabaseSuffix;
extern const std::string kTerrainAssetSuffix;
extern const std::string kTerrainProjectSuffix;
extern const std::string kVectorAssetSuffix;
extern const std::string kVectorLayerSuffix;
extern const std::string kVectorProjectSuffix;
extern const std::string kAssetVersionNumPrefix;
extern const std::string kProjectAssetVersionNumPrefix;

extern const std::string kCombinedRPSubtype;
extern const std::string kDatabaseSubtype;
extern const std::string kDatedImageryType;
extern const std::string kImageryType;
extern const std::string kLayer;
extern const std::string kMapDatabaseSubtype;
extern const std::string kMercatorMapDatabaseSubtype;
extern const std::string kMercatorProductSubtype;
extern const std::string kMercatorProjectSubtype;
extern const std::string kPacketLevelSubtype;
extern const std::string kProductSubtype;
extern const std::string kProjectSubtype;
extern const std::string kQtPacketTwoType;
extern const std::string kQtPacketType;
extern const std::string kRasterType;
extern const std::string kResourceSubtype;
extern const std::string kTerrainType;
extern const std::string kTmeshType;
extern const std::string kUnifiedType;
extern const std::string kVectorGeType;
extern const std::string kVectorMapsRasterType;
extern const std::string kVectorMapsType;
extern const std::string kVectorType;

extern const std::string kCutAllDataFlag;
extern const std::string kCutImageryDataFlag;
extern const std::string kCutTerrainDataFlag;
extern const std::string kCutVectorDataFlag;

extern const std::string kImageryCached;
extern const std::string kHeightMapCached;
extern const std::string kAlphaCached;

// Db config and other server path constants.
extern const std::string kHttpDbRootRequest;
extern const std::string kBinaryDbrootPrefix;
extern const std::string kBinaryDbrootPrefixEta;
extern const std::string kBinaryTimeMachineDbroot;
extern const std::string kDbrootsDir;
extern const std::string kDefaultFont;
extern const std::string kDefaultFontFileName;
extern const std::string kIconsDir;
extern const std::string kPostamblePrefix;
extern const std::string kPostamblePrefixEta;
extern const std::string kServerConfigFileName;
extern const std::string kServerSnippetFile;
extern const std::string kEarthJsonPath;  // Relative to ServerConfig.
extern const std::string kMapsJsonPath;  // Relative to ServerConfig.
// Note: for output layer definitions (mapdb) Fusion uses
// 'postamble'-suffix in the filename since the file (layers.js.LOCALE) is
// updated while publishing. So, Fusion generates
// mapdb/layers.js.postamble.LOCALE while building database, then pushes it into
// published DBs directory and creates final layers.js.LOCALE while publishing.
extern const std::string kLayerDefsPostamblePrefix;
extern const std::string kLayerDefsPrefix;
extern const std::string kLayerDefsDir;  // Relative to ServerConfig.
extern const std::string kSearchTabsPath;  // Relative to ServerConfig.
// The name of the plugin search adapter for converting search requests to KML
// that the Earth Client will understand.
extern const std::string kEarthClientSearchAdapter;
// The name of the plugin search adapter for converting search requests to KML
// that the Maps and Earth Plugin JS code will understand.
extern const std::string kMapsSearchAdapter;

// Location where the unified index places the map from dates to channel id's
// for dated imagery/timemachine imagery layers.
extern const std::string kDatedImageryChannelsFileName;

// TimeMachine db is referenced from the TOC in serverdb.config.
extern const std::string kTimeMachineTOCSuffix;

// Empty date string used for dated imagery dates.
extern const std::string kUnknownDate;

// Empty date-time string used for dated imagery dates.
extern const std::string kUnknownDateTimeUTC;

// An empty string that can be returned as a const string result by any method.
extern const std::string kEmptyString;

// The oldest valid date as defined by the Google Earth 4.4 client.
extern const std::string kTimeMachineOldestDateString;
extern const std::int32_t kTimeMachineOldestDateInt;
extern const std::string kTimeMachineOldestHexDateString;

extern const std::string kDefaultIconName;

// The Google Earth Client has a hard limit of 3 search tabs...the plugin
// and Maps do not have a limit.
static const unsigned int kMaxEarthClientTabCount = 3;

// Names of our default virtual servers.
extern const std::string kDefaultMapVirtualServer;
extern const std::string kDefaultEarthVirtualServer;
extern const std::string kDefaultSearchVirtualServer;

// Names of our default virtual hosts.
// Since GEE 4.5 for path-based publishing scheme.
extern const std::string kDefaultVirtualHost;

extern const std::string kHeaderXmlFile;
// FileNames for a database need by the publisher needed for JSON generation.
extern const std::string kDbImageryLayerConfigFile;
extern const std::string kDbVectorLayerConfigFile;

// Filename to write db manifest files list to provide db manifest when
// a database version has been cleaned (delta-disconnected publishing use-case).
extern const std::string kDbManifestFilesListFile;

// The level below which we remove overlapping road segments.
extern const unsigned int kAvoidRemovingOverlappingRoadSegmentsAtOrHigherZoomLevel;

extern const std::string kGedbBase;
// The gedb_aux sub-directory to keep auxiliary files generated for GE db.
// (e.g. .filelist - db manifest files list).
extern const std::string kGedbAuxBase;
extern const std::string kMapdbBase;
// The mapdb_aux sub-directory to keep auxiliary files generated for map db.
// (e.g. .filelist - db manifest files list).
extern const std::string kMapdbAuxBase;
extern const std::string kDbKeySuffix;

extern const std::string kGedbKey;
extern const std::string kMapdbKey;
extern const std::string kUnifiedIndexKey;

// Stream and Search space sub-dirs in publishroot.
extern const std::string kStreamSpace;
extern const std::string kSearchSpace;

// Portable globe constants.

// Default search url for Portable.
extern const std::string kPortableSearchUrl;

// Template for naming kml files referenced from dbroot.
extern const std::string kDbrootKmlFileNameTemplate;
// Template for naming files referenced from other kml files.
extern const std::string kLinkedFileNameTemplate;
// Version of globe format.
extern const std::string kPortableGlobeVersion;
// Globe index offset
extern const std::uint32_t kPortableGlobeIndexOffset;

//
// This max has been around for a long time. It used to have to match the
// numbers in kbftokff and in the server. But since those tools are now
// gone, this is probably the only place that enforces the packet limit.
// The client's need to enforce these limits is gone. These are enforced in
// Fusion here, just to make the client experience interactive.
extern const std::uint32_t kMaxPacketSize;
extern const std::uint32_t kTargetPacketSize;
extern const std::uint32_t kCompressedBufferSize;

extern const char kQuadTreePathSeparator;

extern const std::string kCrcFileExtension;

extern const std::string kVirtualRasterFileExtension;

// Geo Coordinate System Plate Carree Projection (EPGS:4326).
extern const char* const kPlateCarreeGeoCoordSysName;

extern const std::string sysManBusyMsg;

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_KHCONSTANTS_H_

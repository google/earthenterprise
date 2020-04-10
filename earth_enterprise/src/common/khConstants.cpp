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


#include "common/khConstants.h"


const std::string kDefaultLocaleSuffix = "DEFAULT";
const std::string kTimeMachineTOCSuffix = "timemachine";

const std::string kDatabaseSuffix = ".kdatabase";
const std::string kImageryAssetSuffix = ".kiasset";
const std::string kImageryProjectSuffix = ".kiproject";
const std::string kMapDatabaseSuffix = ".kmdatabase";
const std::string kMapLayerSuffix = ".kmlayer";
const std::string kMapProjectSuffix = ".kmproject";
const std::string kMercatorImageryAssetSuffix = ".kimasset";
const std::string kMercatorImageryProjectSuffix = ".kimproject";
const std::string kMercatorMapDatabaseSuffix = ".kmmdatabase";
const std::string kTerrainAssetSuffix = ".ktasset";
const std::string kTerrainProjectSuffix = ".ktproject";
const std::string kVectorAssetSuffix = ".kvasset";
const std::string kVectorLayerSuffix = ".kvlayer";
const std::string kVectorProjectSuffix = ".kvproject";
const std::string kAssetVersionNumPrefix = "?version=";
const std::string kProjectAssetVersionNumPrefix = "project?version=";

const std::string kCombinedRPSubtype = "CombinedRP";
const std::string kDatabaseSubtype = "Database";
const std::string kDatedImageryType = "DatedImagery";
const std::string kImageryType = "Imagery";
const std::string kLayer = "Layer";
const std::string kMapDatabaseSubtype = "MapDatabase";
const std::string kMercatorMapDatabaseSubtype = "MercatorMapDatabase";
const std::string kMercatorProductSubtype = "MercatorProduct";
const std::string kMercatorProjectSubtype = "MercatorProject";
const std::string kPacketLevelSubtype = "PacketLevel";
const std::string kProductSubtype = "Product";
const std::string kProjectSubtype = "Project";
const std::string kQtPacketTwoType = "QTPacket2";
const std::string kQtPacketType = "QTPacket";
const std::string kRasterType = "Raster";
const std::string kResourceSubtype = "Resource";
const std::string kTerrainType = "Terrain";
const std::string kTmeshType = "TMesh";
const std::string kUnifiedType = "Unified";
const std::string kVectorGeType = "VectorGE";
const std::string kVectorMapsRasterType = "VectorMapsRaster";
const std::string kVectorMapsType = "VectorMaps";
const std::string kVectorType = "Vector";

const std::string kCutAllDataFlag = "ALL";
const std::string kCutImageryDataFlag = "IMAGERY";
const std::string kCutTerrainDataFlag = "TERRAIN";
const std::string kCutVectorDataFlag = "VECTOR";

const std::string kImageryCached = "ImageryCached";
const std::string kHeightMapCached = "HeightmapCached";
const std::string kAlphaCached = "AlphaCached";

const std::string kHttpDbRootRequest = "dbRoot.v5";
const std::string kBinaryDbrootPrefix = "dbroot.v5p";
const std::string kBinaryDbrootPrefixEta = "dbroot.v5";
const std::string kBinaryTimeMachineDbroot = "dbroot.v5p.timemachine";
const std::string kDbrootsDir = "dbroots/";
const std::string kDefaultFont = "Sans";
const std::string kDefaultFontFileName = "fonts/luxisr.ttf";
const std::string kIconsDir = "icons/";
const std::string kPostamblePrefix = "dbroot.v5p.postamble";
const std::string kPostamblePrefixEta = "dbroot.v5.postamble";
const std::string kServerConfigFileName = "serverdb.config";
const std::string kServerSnippetFile = "/opt/google/etc/server_dbroot_config";
const std::string kEarthJsonPath = "json/earth.json";
const std::string kMapsJsonPath = "json/maps.json";
const std::string kLayerDefsPostamblePrefix = "layers.js.postamble";
const std::string kLayerDefsPrefix = "layers.js";
const std::string kLayerDefsDir = "layers/";
const std::string kSearchTabsPath = "search_tabs/searchtabs.js";
const std::string kEarthClientSearchAdapter = "ECV4Adapter";
const std::string kMapsSearchAdapter = "MapsAdapter";

const std::string kUnknownDate = "0000-00-00";
const std::string kUnknownDateTimeUTC = "0000-00-00T00:00:00Z";
const std::string kDatedImageryChannelsFileName = "dated_imagery_channels";

// TimeMachine specific constants.
// The oldest date supported by GE client 4.4.
const std::string kTimeMachineOldestDateString = "0001-01-01T00:00:00Z";
// YearMonthDate integer derived from JpegDateComment.
const std::int32_t kTimeMachineOldestDateInt = 545;
// Hex string for kTimeMachineOldestDateInt.
const std::string kTimeMachineOldestHexDateString = "221";

// This is the name of default "dot" icon.
const std::string kDefaultIconName = "773";

const std::string kEmptyString = "";

const std::string kDefaultMapVirtualServer = "default_map";
const std::string kDefaultEarthVirtualServer = "default_ge";
const std::string kDefaultSearchVirtualServer = "default_search";

const std::string kDefaultVirtualHost = "public";

const std::string kHeaderXmlFile = "header.xml";
const std::string kDbImageryLayerConfigFile = "rasterlayerconfig.xml";
const std::string kDbVectorLayerConfigFile = "vectorlayerconfig.xml";

const std::string kDbManifestFilesListFile = ".filelist";

const unsigned int kAvoidRemovingOverlappingRoadSegmentsAtOrHigherZoomLevel = 13;

const std::string kGedbBase = "gedb";
const std::string kGedbAuxBase = "gedb_aux";
const std::string kMapdbBase = "mapdb";
const std::string kMapdbAuxBase = "mapdb_aux";
const std::string kDbKeySuffix = ".kda";

const std::string kGedbKey = "gedb.kda";
const std::string kMapdbKey = "mapdb.kda";
const std::string kUnifiedIndexKey = "unifiedindex.kda";

// Stream and Search space sub-dirs in publishroot.
const std::string kStreamSpace = "stream_space";
const std::string kSearchSpace = "search_space";

// Portable globe constants.
const std::string kPortableSearchUrl = "SearchServlet/ECV4Adapter";
const std::string kDbrootKmlFileNameTemplate = "dbroot_kml_%03d.kml";
const std::string kLinkedFileNameTemplate = "linked_%03d.%s";
const std::string kPortableGlobeVersion = "0001GEEG";
const std::uint32_t kPortableGlobeIndexOffset = 20;

//
// this max has been around for a long time. It used to have to match the
// numbers in kbftokff and in the server. But since those tools are now
// gone, this is probably the only place that enforces the packet limit.
// The client need to enforce these limits is gone. These are enforced in
// Fusion here, just to make the client experience interactive.
const std::uint32_t kMaxPacketSize = 1024 * 1024;
const std::uint32_t kTargetPacketSize = kMaxPacketSize * 7 / 8;
const std::uint32_t kCompressedBufferSize = kMaxPacketSize * 2;

// since only 0, 1, 2 & 3 are used in qt_path, '4' has been chosen while
// bundling set of qt_paths to a single string.
const char kQuadTreePathSeparator = '4';

// If a file-name is "foo.ext" we will expect corresponding crc side file to be
// "foo.ext.crc".
const std::string kCrcFileExtension = ".crc";
const std::string kVirtualRasterFileExtension = ".khvr";

// Geo Coordinate System Plate Carree Projection (EPGS:4326).
const char* const kPlateCarreeGeoCoordSysName = "EPSG:4326";

// Error message from SystemManager to clients querying current tasks
const std::string sysManBusyMsg = "ERROR: Timed out waiting for lock";

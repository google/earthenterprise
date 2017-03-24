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

// class ResetConfigIdsExecutor definition. It is an executor class to execute
// resetting of different unique IDs in fuse configs.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_RESETCONFIGIDSEXECUTOR_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_RESETCONFIGIDSEXECUTOR_H_

class DatabaseEditRequest;
class MapDatabaseEditRequest;
class RasterProjectEditRequest;
class RasterProductImportRequest;
class KMLProjectEditRequest;
class MapProjectEditRequest;
class VectorProductImportRequest;
class VectorLayerXEditRequest;
class VectorProjectEditRequest;
class MapLayerEditRequest;

// Class executes resetting of IDs in fuse configs.
// Note: we reset IDs (asset_uuid_, fuid_channel_, fuid_resource_) to force
// generating of new ones. It provides a unique ID for assets in assetroot when
// creating a new asset based on existing one ("SaveAs" use-case).
class ResetConfigIdsExecutor {
 public:
  static void Exec(DatabaseEditRequest *const request);

  static void Exec(MapDatabaseEditRequest *const request);

  static void Exec(RasterProjectEditRequest *const request);

  static void Exec(RasterProductImportRequest *const request);

  static void Exec(KMLProjectEditRequest *const request);

  static void Exec(MapProjectEditRequest *const request);

  static void Exec(VectorProductImportRequest *const request);

  static void Exec(VectorLayerXEditRequest *const request);

  static void Exec(VectorProjectEditRequest *const request);

  static void Exec(MapLayerEditRequest *const request);
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_RESETCONFIGIDSEXECUTOR_H_

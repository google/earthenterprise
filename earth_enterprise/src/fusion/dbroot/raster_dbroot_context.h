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

//
// Raster-specific Dbroot generation context
// Contains raster-specific config
// EmitAll() has the raster specific logic for knowing which locales need to
// be generated and to call the RasterDbrootGenerator appropriately

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_RASTER_DBROOT_CONTEXT_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_RASTER_DBROOT_CONTEXT_H_

#include "common/gedbroot/proto_dbroot.h"
#include "fusion/autoingest/.idl/storage/RasterDBRootConfig.h"
#include "fusion/dbroot/proto_dbroot_context.h"

// ****************************************************************************
// ***  RasterDbrootContext
// ***
// ***  This needs initializing only once, regardless of how many
// ***  dbroots we end up generating.
// ****************************************************************************
class RasterDbrootContext : public ProtoDbrootContext {
 private:
  void Init(const std::string &config_filename);

 public:
  RasterDbrootContext(const std::string &config_filename, bool is_imagery);
  RasterDbrootContext(const std::string &config_filename, bool is_imagery,
                      TestingFlagType testing_flag);

  void EmitAll(const std::string &out_dir,
               geProtoDbroot::FileFormat output_format);

 public:
  RasterDBRootGenConfig config_;
  bool                  is_imagery_;
};


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_RASTER_DBROOT_CONTEXT_H_

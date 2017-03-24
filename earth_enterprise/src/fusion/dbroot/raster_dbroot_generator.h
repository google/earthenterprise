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
// Raster-specific Dbroot generator
// Emit() has the raster specific logic for knowing what has to go into a
// raster dbroot.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_RASTER_DBROOT_GENERATOR_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_RASTER_DBROOT_GENERATOR_H_

#include "fusion/dbroot/proto_dbroot_generator.h"

class RasterDbrootContext;

class RasterDbrootGenerator : public ProtoDbrootGenerator {
 private:
  RasterDbrootContext        *raster_context_;

#undef SEND_RASTER_LAYERS
#ifdef SEND_RASTER_LAYERS
  // Client currently doesn't support Layer defs for Imagery/Terrain when
  // using proto dbroots. I don't like that. It makes it impossible to
  // provide multitiple imagery layers per database.
  //
  // I've left the code here in case I can get them to change their mind.
  void EmitNestedLayers(void);
#endif

 public:
  // Holds reference to context, context must outlive this class
  RasterDbrootGenerator(RasterDbrootContext *raster_context,
                        const QString &locale,
                        const std::string &outfile);

  // does the actual work to fill the dbroot protobuf message
  // and writes it to disk using the supplied format
  void Emit(geProtoDbroot::FileFormat output_format);
};


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_RASTER_DBROOT_GENERATOR_H_

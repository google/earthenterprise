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

//
// Vector-specific Dbroot generation context
// Contains vector-specific config
// Contains vector specific context that must span all locale-specific
// dbroots for a single project
// EmitAll() has the vector specific logic for knowing which locales need to
// be generated and to call the RasterDbrootGenerator appropriately

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_VECTOR_DBROOT_CONTEXT_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_VECTOR_DBROOT_CONTEXT_H_

#include "common/gedbroot/proto_dbroot.h"
#include "fusion/autoingest/.idl/storage/IconReference.h"
#include "fusion/autoingest/.idl/storage/VectorDBRootConfig.h"
#include "fusion/dbroot/proto_dbroot_context.h"

// ****************************************************************************
// ***  VectorDbrootContext
// ***
// ***  Things that need initializing only once, regardless of how many
// ***  dbroots we end up generating
// ****************************************************************************
class VectorDbrootContext : public ProtoDbrootContext {
 public:
  typedef std::map<QString,  unsigned int>  GroupMap;
  typedef std::map<QString,  unsigned int> ::const_iterator GroupMapConstIterator;
  typedef std::map<IconReference, int> IconWidthMap;

 private:
  void Init(const std::string &config_filename);

 public:
  explicit VectorDbrootContext(const std::string &config_filename);
  VectorDbrootContext(const std::string &config_filename,
                      TestingFlagType testing_flag);
  void EmitAll(const std::string &out_dir,
               geProtoDbroot::FileFormat output_format);

  int GetIconWidth(const IconReference &icon_ref) const;

 public:
  DBRootGenConfig       config_;
  GroupMap              groupMap;
  std::set< unsigned int>         used_layer_ids_;
  mutable IconWidthMap  icon_width_map_;
};


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_VECTOR_DBROOT_CONTEXT_H_

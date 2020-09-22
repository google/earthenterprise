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

// Fusion class to wrap the proto from eta_dbroot converter,
// Provides simplified Read/Write routines and a couple of other high level
// functions.

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_ETA2PROTO_DBROOT_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_ETA2PROTO_DBROOT_H_

#include "common/gedbroot/proto_dbroot.h"
#include "common/gedbroot/tools/dbroot_v2_converter.h"

// geEta2ProtoDbroot is initialized from ETA dbroot file (either ascii or
// binary) and then used as a proto file. Sample code:
//
//    try {
//     // initialize ETA2proto converter
//     geProtoDbroot protoDbroot;
//     gedbroot::geEta2ProtoDbroot eta2proto(&protoDbroot);
//
//     // Converting from ETA to proto dbroot v2.
//     eta2proto.Convert(dbroot_filename, database_version);
//
//     // Check if we get valid proto dbroot v2 after conversion.
//     if (!protoDbroot.IsValid()) {
//       // proto dbroot v2 is not valid.
//       return;
//     }
//
//     // Printing proto dbroot v2.
//     std:: cout << protoDbroot.ToTextString();
//    } catch  ... {
//    }

namespace gedbroot {

class geEta2ProtoDbroot {
 public:
  explicit geEta2ProtoDbroot(geProtoDbroot *protoDbroot);
  virtual ~geEta2ProtoDbroot();

  bool ConvertFile(const std::string &filename, const int database_version);
  bool Convert(const std::string &content, const int database_version);
  bool ConvertFromBinary(const std::string &contents);
  const std::string& EtaContent() const;

 private:
  bool Load(const std::string &filename, const int database_version);
  bool ReadBinaryContent(const std::string &filename);
  bool ReadAsciiContent(const std::string &filename,
                        const int database_version);

  std::string etaContent_;  // Eta content in ascii.
  std::uint16_t      epoch_;        // Epoch (database_version_).
  keyhole::DbRootV2Converter converter_;
};

}  // namespace gedbroot

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_ETA2PROTO_DBROOT_H_

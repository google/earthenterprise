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
// Fusion class to wrap the protobuf from dbroot_v2.proto
// Provides simplified Read/Write routines and a couple of other high level
// functions.

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_PROTO_DBROOT_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_PROTO_DBROOT_H_

#include <string>
#include <map>
#include "common/khTypes.h"
#include <cstdint>
#include "protobuf/dbroot_v2.pb.h"


class geProtoDbroot : public keyhole::dbroot::DbRootProto {
 public:
  enum FileFormat {kTextFormat, kProtoFormat, kEncodedFormat};

 private:
  typedef std::map<std::uint32_t, keyhole::dbroot::StringEntryProto*> StringTable;

 public:
  geProtoDbroot();
  geProtoDbroot(const std::string &filename, FileFormat input_format);
  ~geProtoDbroot();

  void Write(const std::string &filename, FileFormat output_format) const;
  void Combine(const std::string &filename, FileFormat input_format);
  std::string ToTextString(void) const;
  std::string ToEncodedString(void) const;
  void AddUniversalFusionConfig(std::uint32_t epoch);

  bool HasContents() const;
  bool IsValid(bool expect_epoch = true) const;

  // Throws exception if id isn't found
  keyhole::dbroot::StringEntryProto* GetTranslationEntryById(std::int32_t id);

 private:
  // populated on demand to efficiently implement GetTranslationEntryById
  StringTable cached_string_table_;
};


#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_PROTO_DBROOT_H_

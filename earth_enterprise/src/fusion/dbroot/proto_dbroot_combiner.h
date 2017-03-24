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

// Simple class to implement the combining of multiple proto dbroots
// Sample Usage (change formats to match your needs):
//   ProtoDbRootCombiner combiner;
//   combiner.AddDbroot(FILENAME1, geProtoDbroot::kProtoFormat);
//   combiner.AddDbroot(FILENAME2, geProtoDbroot::kProtoFormat);
//   combiner.AddDbroot(FILENAME3, geProtoDbroot::kProtoFormat);
//   combiner.Write(OUTPUT_FILENAME, geProtoDbroot::kEncodedFormat);

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_PROTO_DBROOT_COMBINER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_PROTO_DBROOT_COMBINER_H_

#include <string>
#include "common/gedbroot/proto_dbroot.h"

class ProtoDbrootCombiner {
 public:
  ProtoDbrootCombiner(void);
  ~ProtoDbrootCombiner(void);
  void AddDbroot(const std::string &filename,
                 geProtoDbroot::FileFormat input_format);
  void Write(const std::string &filename,
             geProtoDbroot::FileFormat output_format);
  geProtoDbroot* ProtoDbroot(void) { return &outproto_; }

 private:
  void RemoveDuplicateProviders(void);
  geProtoDbroot outproto_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_PROTO_DBROOT_COMBINER_H_

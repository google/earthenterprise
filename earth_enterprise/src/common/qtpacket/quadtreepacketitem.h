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


#ifndef COMMON_QTPACKET_QUADTREEPACKETITEM_H__
#define COMMON_QTPACKET_QUADTREEPACKETITEM_H__

#include <string>
#include "common/khTypes.h"

namespace qtpacket {

// Define quadtree packet item.  This structure is used to carry
// information through the merge which generates the quadtree packets.
// Note that copying must be allowed.

class QuadtreePacketItem {
 public:
  QuadtreePacketItem(int32 layer_id,
                     int32 version,
                     int32 copyright,
                     const std::string& date_string)
      : layer_id_(layer_id),
        version_(version),
        copyright_(copyright),
        date_string_(date_string) {
  }
  inline int32 layer_id() const { return layer_id_; }
  inline int32 version() const { return version_; }
  inline int32 copyright() const { return copyright_; }
  inline std::string date_string() const { return date_string_; }

  static const int32 kLayerImagery = 1;
  static const int32 kLayerTerrain = 2;
  static const int32 kLayerVectorMin = 3;
  static const int32 kLayerDatedImagery = 4;

 private:
  int32 layer_id_;
  int32 version_;
  int32 copyright_;
  std::string date_string_;  // only needed for imagery currently.
};

} // namespace qtpacket

#endif  // COMMON_QTPACKET_QUADTREEPACKETITEM_H__

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

// DataProvider Copyright List structure.
// Allows client to identify copyright information
// given a lat-long extent.
//
#ifndef COMMON_QTPACKET_KFFDATAPROVIDERCRL_H__
#define COMMON_QTPACKET_KFFDATAPROVIDERCRL_H__

#include <cstdint>

namespace qtpacket {

struct KffDataProviderCRL {
  std::uint8_t provider_id;
  std::uint8_t level;

  // Not used.
  std::uint8_t byte_padding[6];

  // Order - west, east, south, north
  // Normalized in lat-long space coordinates
  double bbox[4];

  // size on disk. We need 6-byte padding
  // between level and bbox fields ensure that
  // bbox is aligned on 8 bytes.
  // TODO: Reorder the structure and put bbox at the beginning,
  // we'll save 4 bytes (-6 padding, +2 to align struct on 4 bytes).
  static const int kDataProviderCRLSize = 40;
};

} // namespace qtpacket

#endif  // COMMON_QTPACKET_KFFDATAPROVIDERCRL_H__

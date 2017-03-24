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

// Implement the encoding routines used by the Google Earth Client

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_ETENCODER_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_ETENCODER_H_

#include <string>
#include "common/khTypes.h"

namespace etEncoder {

void Encode(void* data, uint32 datalen, const void* key, uint32 keylen);
void Decode(void* data, uint32 datalen, const void* key, uint32 keylen);
void EncodeWithDefaultKey(void* data, uint32 datalen);
void DecodeWithDefaultKey(void* data, uint32 datalen);

extern const std::string kDefaultKey;

};  // namespace etEncoder

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_ETENCODER_H_

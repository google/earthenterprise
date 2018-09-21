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

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_GENERIC_UTILS_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_GENERIC_UTILS_H_

namespace gecommon {

// Returns whether it is JPEG buffer.
inline bool IsJpegBuffer(const char* buffer) {
  return (buffer[6] == 'J' && buffer[7] == 'F' &&
          buffer[8] == 'I' && buffer[9] == 'F');
}

// Returns whether it is PNG buffer.
inline bool IsPngBuffer(const char* buffer) {
  return (buffer[1] == 'P' && buffer[2] == 'N' && buffer[3] == 'G');
}

}  // namespace gecommon.

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_GENERIC_UTILS_H_

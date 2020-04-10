// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// Convert strings to numbers or numbers to strings.

#ifndef GOOGLE3_STRINGS_NUMBERS_H_
#define GOOGLE3_STRINGS_NUMBERS_H_

#include <string>
#include "common/khTypes.h"
#include <cstdint>

//// START DOXYGEN NumbersFunctions grouping
///* @defgroup NumbersFunctions
// * @{ */
//

// ----------------------------------------------------------------------
// ParseLeadingInt32Value
//    A simple parser for std::int32_t values. Returns the parsed value
//    if a valid integer is found; else returns deflt. It does not
//    check if str is entirely consumed.
//    This cannot handle decimal numbers with leading 0s, since they will be
//    treated as octal.  If you know it's decimal, use ParseLeadingDec32Value.
// --------------------------------------------------------------------
std::int32_t ParseLeadingInt32Value(const char* str, std::int32_t deflt);
inline std::int32_t ParseLeadingInt32Value(const std::string& str, std::int32_t deflt) {
  return ParseLeadingInt32Value(str.c_str(), deflt);
}

// ParseLeadingUInt32Value
//    A simple parser for std::uint32_t values. Returns the parsed value
//    if a valid integer is found; else returns deflt. It does not
//    check if str is entirely consumed.
//    This cannot handle decimal numbers with leading 0s, since they will be
//    treated as octal.  If you know it's decimal, use ParseLeadingUDec32Value.
// --------------------------------------------------------------------
 std::uint32_t ParseLeadingUInt32Value(const char* str, std::uint32_t deflt);
inline std::uint32_t ParseLeadingUInt32Value(const std::string& str, std::uint32_t deflt) {
  return ParseLeadingUInt32Value(str.c_str(), deflt);
}

// ----------------------------------------------------------------------
// ParseLeadingDoubleValue
//    A simple parser for double values. Returns the parsed value
//    if a valid double is found; else returns deflt. It does not
//    check if str is entirely consumed.
// --------------------------------------------------------------------
double ParseLeadingDoubleValue(const char* str, double deflt);
inline double ParseLeadingDoubleValue(const std::string& str, double deflt) {
  return ParseLeadingDoubleValue(str.c_str(), deflt);
}

//// END DOXYGEN SplitFunctions grouping
///* @} */
//

#endif  // GOOGLE3_STRINGS_NUMBERS_H_

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

// This file contains string processing functions related to
// numeric values.


#include "common/gedbroot/strings/numbers.h"

#include <stdlib.h>
#include <google/protobuf/stubs/strutil.h>

#include <limits>
#include <cerrno>

// ----------------------------------------------------------------------
// ParseLeadingInt32Value()
// ParseLeadingUInt32Value()
//    A simple parser for [u]int32 values. Returns the parsed value
//    if a valid value is found; else returns deflt
//    This cannot handle decimal numbers with leading 0s.
// --------------------------------------------------------------------

std::int32_t ParseLeadingInt32Value(const char *str, std::int32_t deflt) {
  using std::numeric_limits;

  char *error = NULL;
  long value = strtol(str, &error, 0);
  // Limit long values to std::int32_t min/max.  Needed for lp64; no-op on 32 bits.
  if (value > numeric_limits<std::int32_t>::max()) {
    value = numeric_limits<std::int32_t>::max();
  } else if (value < numeric_limits<std::int32_t>::min()) {
    value = numeric_limits<std::int32_t>::min();
  }
  return (error == str) ? deflt : value;
}

 std::uint32_t ParseLeadingUInt32Value(const char *str, std::uint32_t deflt) {
  using std::numeric_limits;

  if (numeric_limits<unsigned long>::max() == numeric_limits<std::uint32_t>::max()) {
    // When long is 32 bits, we can use strtoul.
    char *error = NULL;
    const std::uint32_t value = strtoul(str, &error, 0);
    return (error == str) ? deflt : value;
  } else {
    // When long is 64 bits, we must use strto64 and handle limits
    // by hand.  The reason we cannot use a 64-bit strtoul is that
    // it would be impossible to differentiate "-2" (that should wrap
    // around to the value UINT_MAX-1) from a string with ULONG_MAX-1
    // (that should be pegged to UINT_MAX due to overflow).
    char *error = NULL;
    std::int64_t value = google::protobuf::strto64(str, &error, 0);
    if (value > numeric_limits<std::uint32_t>::max() ||
        value < -static_cast<std::int64_t>(numeric_limits<std::uint32_t>::max())) {
      value = numeric_limits<std::uint32_t>::max();
    }
    // Within these limits, truncation to 32 bits handles negatives correctly.
    return (error == str) ? deflt : value;
  }
}

// ----------------------------------------------------------------------
// ParseLeadingDoubleValue()
//    A simple parser for double values. Returns the parsed value
//    if a valid value is found; else returns deflt
// --------------------------------------------------------------------

double ParseLeadingDoubleValue(const char *str, double deflt) {
  char *error = NULL;
  errno = 0;
  const double value = strtod(str, &error);
  if (errno != 0 ||  // overflow/underflow happened
      error == str) {  // no valid parse
    return deflt;
  } else {
    return value;
  }
}

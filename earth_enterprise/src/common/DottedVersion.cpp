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

#include <cstdint>

#include "DottedVersion.h"
#include <stdlib.h>
#include <khstl.h>


DottedVersion::DottedVersion(const std::string &verstr) :
    parts_()
{
  split(verstr, ".", back_inserter(parts_));
}

int DottedVersion::compare(const DottedVersion &o) const
{
  unsigned int num_parts = std::min(parts_.size(), o.parts_.size());

  for (unsigned int i = 0; i < num_parts; ++i) {
    std::string my_part = parts_[i];
    std::string o_part = o.parts_[i];
    bool my_alpha = std::isalpha(my_part[0]);
    bool o_alpha  = std::isalpha(o_part[0]);
    if (my_alpha == o_alpha) {
      if (my_alpha) {
        if (my_part < o_part) {
          return -1;
        } else if (my_part > o_part) {
          return 1;
        }
      } else {
        std::uint64_t my_i = atoll(my_part.c_str());
        std::uint64_t o_i = atoll(o_part.c_str());
        if (my_i < o_i) {
          return -1;
        } else if (my_i > o_i) {
          return 1;
        }
      }
    } else {
      return my_alpha ? -1 : 1;
    }

  }
  if (parts_.size() < o.parts_.size()) {
    return -1;
  } if (parts_.size() == o.parts_.size()) {
    return 0;
  } else {
    return 1;
  }
}


std::string DottedVersion::ToString(void) const
{
  return join(parts_.begin(), parts_.end(), ".");
}

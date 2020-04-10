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


#include "khTypes.h"
#include <cstdint>
#include <string.h>
#include <khSimpleException.h>

struct StorageInfo {
  const char * const name;
  unsigned int              size;
};

static const StorageInfo StorageInfo_[] = {
  {"UInt8",   1},
  {"Int8",    1},
  {"UInt16",  2},
  {"Int16",   2},
  {"UInt32",  4},
  {"Int32",   4},
  {"UInt64",  8},
  {"Int64",   8},
  {"Float32", 4},
  {"Float64", 8},
};


const char *
khTypes::StorageName(khTypes::StorageEnum s)
{
  return StorageInfo_[(unsigned int)s].name;
}

uint
khTypes::StorageSize(khTypes::StorageEnum s)
{
  return StorageInfo_[(unsigned int)s].size;
}

khTypes::StorageEnum
khTypes::StorageNameToEnum(const char *n)
{
  for (unsigned int s = (unsigned int)UInt8; s <= (unsigned int)Float64; ++s) {
    if (strcasecmp(n, StorageInfo_[s].name) == 0) {
      return (StorageEnum)s;
    }
  }
  throw khSimpleException("Unrecognized type name: ") << n;
}

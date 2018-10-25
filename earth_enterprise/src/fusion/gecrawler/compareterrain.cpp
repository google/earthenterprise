// Copyright 2017 Google Inc.
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


#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <packet.h>
#include <third_party/rsa_md5/crc32.h>
#include <notify.h>
#include <khEndian.h>
#include "compareterrain.h"
#include "common/geterrain.h"

namespace gecrawler {

bool CompareTerrainPackets(EndianReadBuffer *buffer1,
                           EndianReadBuffer *buffer2,
                           const bool ignore_mesh,
                           std::ostream *snippet) {
  bool success = true;
  *snippet << std::setprecision(12) << std::dec;

  int mesh_number = 0;

  try {
    while (!buffer1->AtEnd() && !buffer2->AtEnd()) {
      if (!geterrain::CompareMesh(mesh_number, buffer1, buffer2, ignore_mesh, snippet)) {
        success = false;
      }
      ++mesh_number;
    }

    if (success && !buffer1->AtEnd()) {
      *snippet << geterrain::kLinePrefix
               << "Excess content at end of source 1 terrain packet, sizes=("
               << buffer1->size() << "," << buffer2->size() << ")";
      success = false;
    }
    if (success && !buffer2->AtEnd()) {
      *snippet << geterrain::kLinePrefix
               << "Excess content at end of source 2 terrain packet, sizes=("
               << buffer1->size() << "," << buffer2->size() << ")";
      success = false;
    }
  }
  catch (khSimpleException ex) {
    *snippet << geterrain::kLinePrefix << "ERROR: Caught exception comparing mesh "
             << mesh_number << ": " << ex.what();
    success = false;
  }

  return success;
}


} // namespace gecrawler

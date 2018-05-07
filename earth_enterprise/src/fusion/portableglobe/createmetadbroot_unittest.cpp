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

#include "fusion/portableglobe/createmetadbroot.h"

#include <cstdio>
#include <string>
// for next_permutation
#include <algorithm>
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <sstream>  // NOLINT(readability/streams)
#include <gtest/gtest.h>
#include "common/gedbroot/proto_dbroot.h"
#include "common/khFileUtils.h"

namespace fusion_portableglobe {

const char* kExampleLayerInfoTxtContents =
    "base_globe /home/jeffdonner/metadbroottest/gray_marble.glb IMAGE 1 0\n"
    "sf_for_glc /home/jeffdonner/metadbroottest/sf_for_glc.glb IMAGE 2 0\n"
    "city_limits /home/jeffdonner/metadbroottest/city_limits.glb VECTOR 3 0\n"
    "dbRoot metaDbRoot DBROOT 0 0\n"
    "info earth/info.txt FILE 0 0\n"
    "earth_local.html earth/earth_local.html FILE 0 0\n"
    "earth.json earth/earth.json FILE 0 0\n"
    "info.txt earth/info.txt FILE 0 0\n"
    "map_v3.html maps/map_v3.html FILE 0 0\n"
    "map_v3.html earth/earth.json FILE 0 0\n"
    "layerInfo earth/layer_info.txt FILE 0 0\n";

const char* kKmlExampleLayerInfoTxtContents =
    "base_globe /home/jeffdonner/metadbroottest/gray_marble.glb IMAGE 1 0\n"
    "my_kml /kml/my_kml.kml KML 2 0\n"
    "city_limits /home/jeffdonner/metadbroottest/city_limits.glb VECTOR 3 0\n"
    "dbRoot metaDbRoot DBROOT 0 0\n"
    "info earth/info.txt FILE 0 0\n"
    "earth_local.html earth/earth_local.html FILE 0 0\n"
    "earth.json earth/earth.json FILE 0 0\n"
    "info.txt earth/info.txt FILE 0 0\n"
    "map_v3.html maps/map_v3.html FILE 0 0\n"
    "map_v3.html earth/earth.json FILE 0 0\n"
    "layerInfo earth/layer_info.txt FILE 0 0\n";

const char* kExternalDbrootContents =
    "end_snippet {\n"
    "  model {\n"
    "    radius: 6371.009766\n"
    "  }\n"
    "}\n";

const char* kGoldenContentsWithExternalBaseDbroot =
    "imagery_present: false\n"
    "terrain_present: false\n"
    "nested_feature {\n"
    "  channel_id: 2\n"
    "  display_name {\n"
    "    value: \"sf_for_glc\"\n"
    "  }\n"
    "  database_url: \"./2/kh/\"\n"
    "}\n"
    "nested_feature {\n"
    "  kml_url {\n"
    "    value: \"./3/kmllayer/root.kml\"\n"
    "  }\n"
    "  channel_id: 3\n"
    "  display_name {\n"
    "    value: \"city_limits\"\n"
    "  }\n"
    "}\n"
    "end_snippet {\n"
    "  model {\n"
    "    radius: 6371.009766\n"
    "  }\n"
    "  search_config {\n"
    "    search_server {\n"
    "      url {\n"
    "        value: \"SearchServlet/ECV4Adapter\"\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "}\n"
    "proto_imagery: true\n";

// Use python to generate:
// fp = open("/out/tmp", "rb")
// data = fp.read()
// i = 0
// for ch in data:
//   print "0x%0.2X," % ord(ch),
//   i += 1
//   if i == 8:
//     print
//     i = 0
const unsigned char kGoldenEncodedOutput[] = {
  0x08, 0x00, 0x12, 0xF8, 0x07, 0x45, 0xF4, 0xBD,
  0x0B, 0x79, 0xE2, 0x6A, 0x45, 0x22, 0x05, 0x92,
  0x2C, 0x17, 0xCD, 0x06, 0x71, 0xF8, 0x49, 0x10,
  0x46, 0x67, 0x51, 0x00, 0x42, 0x25, 0xC6, 0xE8,
  0x61, 0x2C, 0x66, 0x29, 0x08, 0xC6, 0x34, 0xDC,
  0x6A, 0x62, 0x25, 0x79, 0x0A, 0x77, 0x1D, 0x6D,
  0x69, 0xD6, 0xF0, 0x9C, 0x6B, 0x93, 0xA1, 0xBD,
  0x4E, 0x75, 0xE0, 0x41, 0x04, 0x5B, 0xDF, 0x40,
  0x56, 0x0C, 0xD9, 0xBB, 0x72, 0x9B, 0x81, 0x7C,
  0x10, 0x33, 0x53, 0xEE, 0x4F, 0x6C, 0xD4, 0x71,
  0x05, 0xB0, 0x7B, 0xC0, 0x7F, 0x45, 0x03, 0x56,
  0x5A, 0xAD, 0x77, 0x55, 0x65, 0x0B, 0x33, 0x92,
  0x2A, 0xAC, 0x19, 0x6C, 0x35, 0x14, 0xC5, 0x1D,
  0x30, 0x73, 0xF8, 0x33, 0x3E, 0x6D, 0x46, 0x38,
  0x4A, 0xB4, 0xDD, 0xF0, 0x2E, 0xDD, 0x17, 0x75,
  0x16, 0xDA, 0x8C, 0x44, 0x74, 0x22, 0x06, 0xFA,
  0x61, 0x22, 0x0C, 0x33, 0x22, 0x53, 0x6F, 0xAF,
  0x39, 0x44, 0x0B, 0x8C, 0x0E, 0x39, 0xD9, 0x39,
  0x13, 0x4C, 0xB9, 0xBF, 0x7F, 0xAB, 0x5C, 0x8C,
  0x50, 0x5F, 0x9F, 0x22, 0x75, 0x78, 0x1F, 0xE9,
  0x07, 0x71, 0x91, 0x68, 0x3B, 0xC1, 0xC4, 0x9B,
  0x7F, 0xF0, 0x3C, 0x56, 0x71, 0x48, 0x82, 0x05,
  0x27, 0x55, 0x66, 0x59, 0x4E, 0x65, 0x1D, 0x98,
  0x75, 0xA3, 0x61, 0x46, 0x7D, 0x61, 0x3F, 0x15,
  0x41, 0x00, 0x9F, 0x14, 0x06, 0xD7, 0xB4, 0x34,
  0x4D, 0xCE, 0x13, 0x87, 0x46, 0xB0, 0x1A, 0xD5,
  0x05, 0x1C, 0xB8, 0x8A, 0x27, 0x7B, 0x8B, 0xDC,
  0x2B, 0xBB, 0x4D, 0x67, 0x30, 0xC8, 0xD1, 0xF6,
  0x5C, 0x8F, 0x50, 0xFA, 0x5B, 0x2F, 0x46, 0x9B,
  0x6E, 0x35, 0x18, 0x2F, 0x27, 0x43, 0x2E, 0xEB,
  0x0A, 0x0C, 0x5E, 0x10, 0x05, 0x10, 0xA5, 0x73,
  0x1B, 0x65, 0x34, 0xE5, 0x6C, 0x2E, 0x6A, 0x43,
  0x27, 0x63, 0x14, 0x23, 0x55, 0xA9, 0x3F, 0x71,
  0x7B, 0x67, 0x43, 0x7D, 0x3A, 0xAF, 0xCD, 0xE2,
  0x54, 0x55, 0x9C, 0xFD, 0x4B, 0xC6, 0xE2, 0x9F,
  0x2F, 0x28, 0xED, 0xCB, 0x5C, 0xC6, 0x2D, 0x66,
  0x07, 0x88, 0xA7, 0x3B, 0x2F, 0x18, 0x2A, 0x22,
  0x4E, 0x0E, 0xB0, 0x6B, 0x2E, 0xDD, 0x0D, 0x95,
  0x7D, 0x7D, 0x47, 0xBA, 0x43, 0xB2, 0x11, 0xB2,
  0x2B, 0x3E, 0x4D, 0xAA, 0x3E, 0x7D, 0xE6, 0xCE,
  0x49, 0x89, 0xC6, 0xE6, 0x78, 0x0C, 0x61, 0x31,
  0x05, 0x2D, 0x01, 0xA4, 0x4F, 0xA5, 0x7E, 0x71,
  0x20, 0x88, 0xEC, 0x0D, 0x31, 0xE8, 0x4E, 0x0B,
  0x00, 0x6E, 0x50, 0x68, 0x7D, 0x17, 0x3D, 0x08,
  0x0D, 0x17, 0x95, 0xA6, 0x6E, 0xA3, 0x68, 0x97,
  0x24, 0x5B, 0x6B, 0xF3, 0x17, 0x23, 0xF3, 0xB6,
  0x73, 0xB3, 0x0D, 0x0B, 0x40, 0xC0, 0x9F, 0xD8,
  0x04, 0x51, 0x5D, 0xFA, 0x1A, 0x17, 0x22, 0x2E,
  0x15, 0x6A, 0xDF, 0x49, 0x00, 0xB9, 0xA0, 0x77,
  0x55, 0xC6, 0xEF, 0x10, 0x6A, 0xBF, 0x7B, 0x47,
  0x4C, 0x7F, 0x83, 0x17, 0x05, 0xEE, 0xDC, 0xDC,
  0x46, 0x85, 0xA9, 0xAD, 0x53, 0x07, 0x2B, 0x53,
  0x34, 0x06, 0x07, 0xFF, 0x14, 0x94, 0x59, 0x19,
  0x02, 0xE4, 0x38, 0xE8, 0x31, 0x83, 0x4E, 0xB9,
  0x58, 0x46, 0x6B, 0xCB, 0x2D, 0x23, 0x86, 0x92,
  0x70, 0x00, 0x35, 0x88, 0x22, 0xCF, 0x31, 0xB2,
  0x26, 0x2F, 0xE7, 0xC3, 0x75, 0x2D, 0x36, 0x2C,
  0x72, 0x74, 0xB0, 0x23, 0x47, 0xB7, 0xD3, 0xD1,
  0x26, 0x16, 0x85, 0x37, 0x72, 0xE2, 0x00, 0x8C,
  0x44, 0xCF, 0x10, 0xDA, 0x33, 0x2D, 0x1A, 0xDE,
  0x60, 0x86, 0x69, 0x23, 0x69, 0x2A, 0x7C, 0xCD,
  0x4B, 0x51, 0x0D, 0x95, 0x54, 0x39, 0x77, 0x2E,
  0x29, 0xEA, 0x1B, 0xA6, 0x50, 0xA2, 0x6A, 0x8F,
  0x6F, 0x50, 0x99, 0x5C, 0x3E, 0x54, 0xFB, 0xEF,
  0x50, 0x5B, 0x0B, 0x07, 0x45, 0x17, 0x89, 0x6D,
  0x28, 0x13, 0x77, 0x37, 0x1D, 0xDB, 0x8E, 0x1E,
  0x4A, 0x05, 0x66, 0x4A, 0x6F, 0x99, 0x20, 0xE5,
  0x70, 0xE2, 0xB9, 0x71, 0x7E, 0x0C, 0x6D, 0x49,
  0x04, 0x2D, 0x7A, 0xFE, 0x72, 0xC7, 0xF2, 0x59,
  0x30, 0x8F, 0xBB, 0x02, 0x5D, 0x73, 0xE5, 0xC9,
  0x20, 0xEA, 0x78, 0xEC, 0x20, 0x90, 0xF0, 0x8A,
  0x7F, 0x42, 0x17, 0x7C, 0x47, 0x19, 0x60, 0xB0,
  0x16, 0xBD, 0x26, 0xB7, 0x71, 0xB6, 0xC7, 0x9F,
  0x0E, 0xD1, 0x33, 0x82, 0x3D, 0xD3, 0xAB, 0xEE,
  0x63, 0x99, 0xC8, 0x2B, 0x53, 0xA0, 0x44, 0x5C,
  0x71, 0x01, 0xC6, 0xCC, 0x44, 0x1F, 0x32, 0x4F,
  0x3C, 0xCA, 0xC0, 0x29, 0x3D, 0x52, 0xD3, 0x61,
  0x19, 0x58, 0xA9, 0x7D, 0x65, 0xB4, 0xDC, 0xCF,
  0x0D, 0xF4, 0x3D, 0xF1, 0x08, 0xA9, 0x42, 0xDA,
  0x23, 0x09, 0xD8, 0xBF, 0x5E, 0x50, 0x49, 0xF8,
  0x4D, 0xC0, 0xCB, 0x47, 0x4C, 0x1C, 0x4F, 0xF7,
  0x7B, 0x2B, 0xD8, 0x16, 0x18, 0xC5, 0x31, 0x92,
  0x3B, 0xB5, 0x6F, 0xDC, 0x6C, 0x0D, 0x92, 0x88,
  0x16, 0xD1, 0x9E, 0xDB, 0x3F, 0xE2, 0xE9, 0xDA,
  0x5F, 0xD4, 0x84, 0xE2, 0x46, 0x61, 0x5A, 0xDE,
  0x1C, 0x55, 0xCF, 0xA4, 0x00, 0xBE, 0xFD, 0xCE,
  0x67, 0xF1, 0x4A, 0x69, 0x1C, 0x97, 0xE6, 0x20,
  0x48, 0xD8, 0x5D, 0x7F, 0x7E, 0xAE, 0x71, 0x20,
  0x0E, 0x4E, 0xAE, 0xC0, 0x56, 0xA9, 0x91, 0x01,
  0x3C, 0x82, 0x1D, 0x0F, 0x72, 0xE7, 0x76, 0xEC,
  0x29, 0x49, 0xD6, 0x5D, 0x2D, 0x83, 0xE3, 0xDB,
  0x36, 0x06, 0xA9, 0x3B, 0x66, 0x13, 0x97, 0x87,
  0x6A, 0xD5, 0xB6, 0x3D, 0x50, 0x5E, 0x52, 0xB9,
  0x4B, 0xC7, 0x73, 0x57, 0x78, 0xC9, 0xF4, 0x2E,
  0x59, 0x07, 0x95, 0x93, 0x6F, 0xD0, 0x4B, 0x17,
  0x57, 0x19, 0x3E, 0x27, 0x27, 0xC7, 0x60, 0xDB,
  0x3B, 0xED, 0x9A, 0x0E, 0x53, 0x44, 0x16, 0x3E,
  0x3F, 0x8D, 0x92, 0x6D, 0x77, 0xA2, 0x0A, 0xEB,
  0x3F, 0x52, 0xA8, 0xC6, 0x55, 0x5E, 0x31, 0x49,
  0x37, 0x85, 0xF4, 0xC5, 0x1F, 0x26, 0x2D, 0xA9,
  0x1C, 0xBF, 0x8B, 0x27, 0x54, 0xDA, 0xC3, 0x6A,
  0x20, 0xE5, 0x2A, 0x78, 0x04, 0xB0, 0xD6, 0x90,
  0x70, 0x72, 0xAA, 0x8B, 0x68, 0xBD, 0x88, 0xF7,
  0x02, 0x5F, 0x48, 0xB1, 0x7E, 0xC0, 0x58, 0x4C,
  0x3F, 0x66, 0x1A, 0xF9, 0x3E, 0xE1, 0x65, 0xC0,
  0x70, 0xA7, 0xCF, 0x38, 0x69, 0xAF, 0xF0, 0x56,
  0x6C, 0x64, 0x49, 0x9C, 0x27, 0xAD, 0x78, 0x74,
  0x4F, 0xC2, 0x87, 0xDE, 0x56, 0x39, 0x00, 0xDA,
  0x77, 0x0B, 0xCB, 0x2D, 0x1B, 0x89, 0xFB, 0x35,
  0x4F, 0x02, 0xF5, 0x08, 0x51, 0x13, 0x60, 0xC1,
  0x0A, 0x5A, 0x47, 0x4D, 0x26, 0x1C, 0x33, 0x30,
  0x78, 0xDA, 0xC0, 0x9C, 0x46, 0x47, 0xE2, 0x5B,
  0x79, 0x60, 0x49, 0x6E, 0x37, 0x67, 0x53, 0x0A,
  0x3E, 0xE9, 0xEC, 0x46, 0x39, 0xB2, 0xF1, 0x34,
  0x0D, 0xC6, 0x84, 0x53, 0x75, 0x6E, 0xE1, 0x0C,
  0x59, 0xD9, 0x1E, 0xDE, 0x29, 0x85, 0x10, 0x7B,
  0x49, 0x49, 0xA5, 0x77, 0x79, 0xBE, 0x49, 0x56,
  0x2E, 0x36, 0xE7, 0x0B, 0x3A, 0xBB, 0x4F, 0x03,
  0x62, 0x7B, 0xD2, 0x4D, 0x31, 0x95, 0x2F, 0xBD,
  0x38, 0x7B, 0xA8, 0x4F, 0x21, 0xE1, 0xEC, 0x46,
  0x70, 0x76, 0x95, 0x7D, 0x29, 0x22, 0x78, 0x88,
  0x0A, 0x90, 0xDD, 0x9D, 0x5C, 0xDA, 0xDE, 0x19,
  0x51, 0xCF, 0xF0, 0xFC, 0x59, 0x52, 0x65, 0x7C,
  0x33, 0x13, 0xDF, 0xF3, 0x48, 0xDA, 0xBB, 0x2A,
  0x75, 0xDB, 0x60, 0xB2, 0x02, 0x15, 0xD4, 0xFC,
  0x19, 0xED, 0x1B, 0xEC, 0x7F, 0x35, 0xA8, 0xFF,
  0x28, 0x31, 0x07, 0x2D, 0x12, 0xC8, 0xDC, 0x88,
  0x46, 0x7C, 0x8A, 0x5B, 0x22, 0x1A, 0x85, 0x01,
  0x55, 0x97, 0x78, 0x32, 0xF4, 0x51, 0x00, 0x42,
  0x0F, 0x81, 0x8E, 0x09, 0xC6, 0x90, 0xC8, 0xF9,
  0xA9, 0xE1, 0xCE, 0xF2, 0x22, 0xB1, 0xC4, 0x01,
  0x80, 0x7C, 0x59, 0x05, 0x26, 0x56, 0xA3, 0x7C,
  0x83, 0xD2, 0xBD, 0xB8, 0x7D, 0xE3, 0xE0, 0xEB,
  0xA4, 0x78, 0x96, 0xB7, 0x69, 0x9D, 0x91, 0x5B,
  0x6B, 0x63, 0xF4, 0xD3, 0x6F, 0xA1, 0xCD, 0xE2,
  0xF2, 0x67, 0x8C, 0x1B, 0xA4, 0xC0, 0x80, 0x99,
  0x24, 0x29, 0xDB, 0xCF, 0xA2, 0x7D, 0x3C, 0xC1,
  0x49, 0x5F, 0xCC, 0x6D, 0x9A, 0x0A, 0xD6, 0xE2,
  0xCD, 0xD9, 0x17, 0xED, 0x73, 0x18, 0x44, 0x71,
  0xF6, 0x43, 0x04, 0xB1, 0x6B, 0x35, 0x2E, 0xBD,
  0xA4, 0x83, 0xBE, 0x65, 0x73, 0x61, 0xE0, 0xF0,
  0xAD, 0x30, 0x5C, 0x0A, 0x43, 0xA5, 0x20, 0xE0,
  0xD4, 0x1D, 0x99, 0x17, 0x2A, 0x27, 0xBA, 0x3B,
  0x3F, 0x6B, 0x03, 0x37, 0x4A, 0x82, 0x89, 0x08,
  0xEE, 0x94, 0x6B, 0x6C, 0xC1
};

const char* kExampleLayerGoldenOutput =
    "imagery_present: true\n"
    "terrain_present: false\n"
    // skips layer 1
    "nested_feature {\n"
    "  channel_id: 2\n"
    "  display_name {\n"
    "    value: \"sf_for_glc\"\n"
    "  }\n"
    "  database_url: \"./2/kh/\"\n"
    "}\n"
    "nested_feature {\n"
    "  kml_url {\n"
    "    value: \"./3/kmllayer/root.kml\"\n"
    "  }\n"
    "  channel_id: 3\n"
    "  display_name {\n"
    "    value: \"city_limits\"\n"
    "  }\n"
    "}\n"
    "end_snippet {\n"
    "  disable_authentication: true\n"
    "  search_config {\n"
    "    search_server {\n"
    "      url {\n"
    "        value: \"SearchServlet/ECV4Adapter\"\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "}\n"
    "proto_imagery: true\n";

const char* kKmlExampleLayerGoldenOutput =
    "imagery_present: true\n"
    "terrain_present: false\n"
    // skips layer 1
    "nested_feature {\n"
    "  kml_url {\n"
    "    value: \"/kml/my_kml.kml\"\n"
    "  }\n"
    "  channel_id: 2\n"
    "  display_name {\n"
    "    value: \"my_kml\"\n"
    "  }\n"
    "}\n"
    "nested_feature {\n"
    "  kml_url {\n"
    "    value: \"./3/kmllayer/root.kml\"\n"
    "  }\n"
    "  channel_id: 3\n"
    "  display_name {\n"
    "    value: \"city_limits\"\n"
    "  }\n"
    "}\n"
    "end_snippet {\n"
    "  disable_authentication: true\n"
    "  search_config {\n"
    "    search_server {\n"
    "      url {\n"
    "        value: \"SearchServlet/ECV4Adapter\"\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "}\n"
    "proto_imagery: true\n";

const char* kLayersWithLineTooShort =
    "sf_for_glc metadbroottest/sf_for_glc.glb IMAGE\n";

const char* kVettedChannelLayerLines[] = {
  // 0 + 1 should be vetted, 2 + 3 should be passed.
  "mystery /home/jeffdonner/metadbroottest/mystery IMAGE 0 0\n",
  "base_globe /home/jeffdonner/metadbroottest/gray_marble.glb IMAGE 1 0\n",
  "sf_for_glc /home/jeffdonner/metadbroottest/sf_for_glc.glb IMAGE 2 0\n",
  "city_limits /home/jeffdonner/metadbroottest/city_limits.glb IMAGE 3 0\n"
};

const char* kVettedGoldenPreamble = {
  "imagery_present: true\n"
  "terrain_present: false\n"
};

const char* kVettedGoldenContent[
    // (tie sizes together)
    sizeof(kVettedChannelLayerLines) /
    sizeof(kVettedChannelLayerLines[0])] = {
  "",
  "",
  "nested_feature {\n"
  "  channel_id: 2\n"
  "  display_name {\n"
  "    value: \"sf_for_glc\"\n"
  "  }\n"
  "  database_url: \"./2/kh/\"\n"
  "}\n",
  "nested_feature {\n"
  "  channel_id: 3\n"
  "  display_name {\n"
  "    value: \"city_limits\"\n"
  "  }\n"
  "  database_url: \"./3/kh/\"\n"
  "}\n"
};

const char* kVettedGoldenPostamble =
    "end_snippet {\n"
    "  disable_authentication: true\n"
    "  search_config {\n"
    "    search_server {\n"
    "      url {\n"
    "        value: \"SearchServlet/ECV4Adapter\"\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "}\n"
    "proto_imagery: true\n";

const char* kOutputOfLayersWithLineTooShort =
    // No layer lines
    "imagery_present: true\n"
    "terrain_present: false\n"
    "end_snippet {\n"
    "  disable_authentication: true\n"
    "  search_config {\n"
    "    search_server {\n"
    "      url {\n"
    "        value: \"SearchServlet/ECV4Adapter\"\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "}\n"
    "proto_imagery: true\n";

const char* kLayersWithEmptyLine =
    "sf_for_glc /home/jeffdonner/metadbroottest/sf_for_glc.glb IMAGE 2 0\n"
    "\n"
    "city_limits /home/jeffdonner/metadbroottest/city_limits.glb VECTOR 3 0\n";

const char* kOutputOfLayersWithEmptyLine =
    "imagery_present: true\n"
    "terrain_present: false\n"
    "nested_feature {\n"
    "  channel_id: 2\n"
    "  display_name {\n"
    "    value: \"sf_for_glc\"\n"
    "  }\n"
    "  database_url: \"./2/kh/\"\n"
    "}\n"
    "nested_feature {\n"
    "  kml_url {\n"
    "    value: \"./3/kmllayer/root.kml\"\n"
    "  }\n"
    "  channel_id: 3\n"
    "  display_name {\n"
    "    value: \"city_limits\"\n"
    "  }\n"
    "}\n"
    "end_snippet {\n"
    "  disable_authentication: true\n"
    "  search_config {\n"
    "    search_server {\n"
    "      url {\n"
    "        value: \"SearchServlet/ECV4Adapter\"\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "}\n"
    "proto_imagery: true\n";


// Calls out to diff, and prints that diff if there is any.
// TODO: -- wants to be a utility somewhere
bool files_are_same(const std::string& a_fname,
                    const std::string& b_fname,
                    bool want_diff_on_failure = true) {
  std::ostringstream oss_command;

  oss_command << "diff " << a_fname << " " << b_fname;
  FILE* diff_results_file = ::popen(oss_command.str().c_str(), "r");

  std::string diff_contents;
  char* line = NULL;
  size_t len_line = 0;
  ssize_t nread;
  // getline is a GNU extension
  while ((nread = ::getline(&line, &len_line, diff_results_file)) != -1) {
    diff_contents.append(line);
  }
  if (line)
    free(line);

  size_t size_of_diff = diff_contents.length();

  if (ferror(diff_results_file)) {
    std::cout << "error reading diff pipe... " << std::endl;
  }

  if (want_diff_on_failure && 0 < size_of_diff) {
    std::cout << "Diff: \n" << diff_contents << std::endl;
  }

  int rval = pclose(diff_results_file);
  if (rval < 0) {
    std::cout << "pclose error: " << strerror(errno) << std::endl;
  }
  return size_of_diff == 0;
}

// TODO: -- wants to be a utility somewhere
class ScopedTempFile {
 public:
  explicit ScopedTempFile(const std::string& base)
      : leave_in_place_(false),
        src_(khTmpFilename(base))
  {}

  // for debugging
  void LeaveInPlace() { leave_in_place_ = true; }

  std::string const& fname() const { return src_; }

  ~ScopedTempFile() {
    if (leave_in_place_) {
      std::cout << "leaving temp file: " << fname() << std::endl;
    } else {
      ::remove(src_.c_str());
    }
  }

 private:
  bool leave_in_place_;
  std::string src_;
};

class TestableMetaDbRootWriter : public MetaDbRootWriter {
 public:
  TestableMetaDbRootWriter(bool has_base_imagery,
                           bool has_proto_imagery,
                           bool has_terrain,
                           std::istream& in_layers,
                           const std::string& external_base = "")
      : MetaDbRootWriter(has_base_imagery, has_proto_imagery, has_terrain,
                         in_layers, external_base)
  {}

  bool Create() {
    return CreateMetaDbRoot();
  }

  const geProtoDbroot& DbRoot() const { return combinedDbRoot_; }
};


const ::keyhole::dbroot::NestedFeatureProto&
nth_layer(const geProtoDbroot& dbRootProto, int layer_index) {
  return dbRootProto.nested_feature(layer_index);
}

bool channel_id_equals(const geProtoDbroot& dbRootProto,
                       int layer_index, int id) {
  const ::keyhole::dbroot::NestedFeatureProto& nested_proto =
      nth_layer(dbRootProto, layer_index);
  return nested_proto.has_channel_id() && nested_proto.channel_id() == id;
}

bool kml_url_equals(const geProtoDbroot& dbRootProto,
                    int layer, const std::string& kml_url) {
  const ::keyhole::dbroot::NestedFeatureProto& nested_proto =
      nth_layer(dbRootProto, layer);
  if (nested_proto.has_kml_url()) {
    const ::keyhole::dbroot::StringIdOrValueProto& string_or_id =
        nested_proto.kml_url();
    // Note: this insists on a string, for good or ill
    // We could make a version with an 'int' signature...
    return string_or_id.has_value() && string_or_id.value() == kml_url;
  }
  return false;
}

bool database_url_equals(const geProtoDbroot& dbRootProto,
                         int layer, const std::string& database_url) {
  const ::keyhole::dbroot::NestedFeatureProto& nested_proto =
      nth_layer(dbRootProto, layer);
  return nested_proto.has_database_url() &&
      nested_proto.database_url() == database_url;
}

bool display_name_equals(const geProtoDbroot& dbRootProto,
                         int layer, const std::string& display_name) {
  const ::keyhole::dbroot::NestedFeatureProto& nested_proto =
      nth_layer(dbRootProto, layer);
  if (nested_proto.has_display_name()) {
    const ::keyhole::dbroot::StringIdOrValueProto& string_or_id =
        nested_proto.display_name();
    return string_or_id.has_value() && string_or_id.value() == display_name;
  }
  return false;
}

bool LayersInputProducesOutput(const char* layers_input,
                               bool has_imagery,
                               bool has_proto_imagery,
                               bool has_terrain,
                               const char* base_dbroot_content,
                               const char* output,
                               // for debugging
                               bool leave_files_in_place = false) {
  std::stringstream in_layers(std::stringstream::in | std::stringstream::out);
  in_layers << layers_input;

  std::string base_dbroot_fname;
  ScopedTempFile file_of_base_dbroot("/tmp/base_dbroot");
  if (base_dbroot_content && *base_dbroot_content) {
    base_dbroot_fname = file_of_base_dbroot.fname();
    std::ofstream out_base_dbroot(base_dbroot_fname.c_str());
    out_base_dbroot << base_dbroot_content;
    out_base_dbroot.close();
  }

  TestableMetaDbRootWriter writer(has_imagery,
                                  has_proto_imagery,
                                  has_terrain,
                                  in_layers,
                                  base_dbroot_fname);
  ScopedTempFile file_of_actual("/tmp/actual");
  // TODO: - should test all formats
  writer.SaveTo(file_of_actual.fname(), geProtoDbroot::kTextFormat);
  if (leave_files_in_place) {
    file_of_actual.LeaveInPlace();
  }

  ScopedTempFile file_of_canonical("/tmp/canonical");
  std::ofstream out_canon(file_of_canonical.fname().c_str());
  out_canon << output;
  out_canon.close();
  if (leave_files_in_place) {
    file_of_canonical.LeaveInPlace();
  }

  bool are_same = files_are_same(file_of_actual.fname(),
                                 file_of_canonical.fname());
  return are_same;
}

TEST(TestVisibleLayers, InsertionIndependentOfOrder) {
  // Important it start sorted
  LayerInfo input_layers[] = {
    {"one", "path1", LayerInfo::kImage, 1, 0},
    {"two", "path2", LayerInfo::kImage, 2, 1},
    {"three", "path3", LayerInfo::kImage, 3, 2},
  };
  const int num_input_layers = sizeof(input_layers) / sizeof(input_layers[0]);
  const int leaf_layer = input_layers[num_input_layers-1].layer;

  do {
    VisibleLayers db_root_layers;
    for (int i = 0; i < num_input_layers; ++i) {
      db_root_layers.insert(input_layers[i]);
    }
    db_root_layers.RemoveParentLayers();

    // leaf is there
    ASSERT_TRUE(db_root_layers.find(leaf_layer) != db_root_layers.end());
    // it's the only one
    ASSERT_EQ(db_root_layers.size(), static_cast<size_t>(1));
  } while (std::next_permutation(input_layers,
                                 input_layers + num_input_layers,
                                 LayerInfoIsLower()));
}

class CreateMetaDbRootTest : public testing::Test {
 protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};


TEST_F(CreateMetaDbRootTest, HappyPath) {
  bool are_same = LayersInputProducesOutput(
      kExampleLayerInfoTxtContents, true, true, false, "",
      kExampleLayerGoldenOutput);
  ASSERT_TRUE(are_same);
}

TEST_F(CreateMetaDbRootTest, HappyPathWithKml) {
  bool are_same = LayersInputProducesOutput(
      kKmlExampleLayerInfoTxtContents, true, true, false, "",
      kKmlExampleLayerGoldenOutput);
  ASSERT_TRUE(are_same);
}

TEST_F(CreateMetaDbRootTest, RejectsShortLines) {
  bool are_same = LayersInputProducesOutput(
      kLayersWithLineTooShort, true, true, false, "",
      kOutputOfLayersWithLineTooShort);
  ASSERT_TRUE(are_same);
}

TEST_F(CreateMetaDbRootTest, Layers0And1Vetted) {
  const size_t GOLDEN_BUF_SIZE = 5000;
  char buf_golden[GOLDEN_BUF_SIZE+1];
  for (unsigned i = 0;
       i < sizeof(kVettedChannelLayerLines) /
           sizeof(kVettedChannelLayerLines[0]);
       ++i) {
    size_t length_so_far = 0;
    length_so_far += snprintf(buf_golden + length_so_far,
                              GOLDEN_BUF_SIZE - length_so_far,
                              "%s", kVettedGoldenPreamble);
    ASSERT_LE(length_so_far, GOLDEN_BUF_SIZE);
    length_so_far += snprintf(buf_golden + length_so_far,
                              GOLDEN_BUF_SIZE - length_so_far,
                              "%s", kVettedGoldenContent[i]);
    ASSERT_LE(length_so_far, GOLDEN_BUF_SIZE);
    length_so_far += snprintf(buf_golden + length_so_far,
                              GOLDEN_BUF_SIZE - length_so_far,
                              "%s", kVettedGoldenPostamble);
    ASSERT_LE(length_so_far, GOLDEN_BUF_SIZE);

    bool are_same = LayersInputProducesOutput(
        kVettedChannelLayerLines[i], true, true, false, "", buf_golden);
    ASSERT_TRUE(are_same) << "index:" << i;
  }
}

TEST_F(CreateMetaDbRootTest, ToleratesEmptyLines) {
  bool are_same = LayersInputProducesOutput(
      kLayersWithEmptyLine, true, true, false, "",
      kOutputOfLayersWithEmptyLine);
  ASSERT_TRUE(are_same);
}

TEST_F(CreateMetaDbRootTest, SimpleAttributeAccess) {
  std::stringstream in_layers(std::stringstream::in | std::stringstream::out);
  in_layers << kExampleLayerInfoTxtContents;
  bool has_base_imagery = true;
  bool has_proto_imagery = true;
  bool has_terrain = false;

  TestableMetaDbRootWriter writer(
      has_base_imagery, has_proto_imagery, has_terrain, in_layers);
  bool success = writer.Create();
  ASSERT_TRUE(success) << "Creation was successful";

  const geProtoDbroot& dbRootProto = writer.DbRoot();
  ASSERT_TRUE(dbRootProto.IsInitialized()) << "Dbroot valid?";
  dbRootProto.CheckInitialized();

  ASSERT_TRUE(dbRootProto.has_imagery_present() &&
              dbRootProto.imagery_present() == has_base_imagery)
      << "has base imagery";

  ASSERT_TRUE(dbRootProto.has_proto_imagery() &&
              dbRootProto.proto_imagery() == has_proto_imagery)
      << "has proto imagery";

  ASSERT_TRUE(dbRootProto.has_terrain_present() &&
              dbRootProto.terrain_present() == has_terrain)
      << "has terrain present";

  int layer_index = 0;
  int channel_id = 2;
  ASSERT_TRUE(channel_id_equals(dbRootProto, layer_index, channel_id));
  ASSERT_TRUE(database_url_equals(dbRootProto, layer_index, "./2/kh/"));
  ASSERT_TRUE(display_name_equals(dbRootProto, layer_index, "sf_for_glc"));

  channel_id = 3;
  ++layer_index;
  ASSERT_TRUE(channel_id_equals(dbRootProto, layer_index, channel_id));
  ASSERT_TRUE(
      kml_url_equals(dbRootProto, layer_index, "./3/kmllayer/root.kml"));
  ASSERT_TRUE(display_name_equals(dbRootProto, layer_index, "city_limits"));
}

TEST_F(CreateMetaDbRootTest, UsesExternalDbRootBaseFile) {
  bool are_same = LayersInputProducesOutput(
      kExampleLayerInfoTxtContents,
      false, true, false,
      kExternalDbrootContents,
      kGoldenContentsWithExternalBaseDbroot);
  ASSERT_TRUE(are_same);
}

TEST_F(CreateMetaDbRootTest, EncodedFormatWorks) {
  std::stringstream in_layers(std::stringstream::in | std::stringstream::out);
  in_layers << kExampleLayerInfoTxtContents;
  bool has_base_imagery = false;
  bool has_proto_imagery = true;
  bool has_terrain = true;

  TestableMetaDbRootWriter writer(
      has_base_imagery, has_proto_imagery, has_terrain, in_layers);
  bool success = writer.Create();
  ASSERT_TRUE(success);

  ScopedTempFile file_of_actual("/tmp/actual");
  writer.SaveTo(file_of_actual.fname(), geProtoDbroot::kEncodedFormat);
  // Use when need to recreate kGoldenEncodedOutput:
  // writer.SaveTo("/tmp/dbroot", geProtoDbroot::kEncodedFormat);

  ScopedTempFile file_of_canonical("/tmp/canonical");
  std::ofstream out_canon(file_of_canonical.fname().c_str());
  out_canon.write(reinterpret_cast<const char *>(kGoldenEncodedOutput), sizeof(kGoldenEncodedOutput));
  out_canon.close();

  bool are_same = files_are_same(file_of_actual.fname(),
                                 file_of_canonical.fname(),
                                 // Do not want binary diff on failure.
                                 false);
  ASSERT_TRUE(are_same);
}

}  // end namespace fusion_portableglobe


int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

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

// Write images and terrains to disk.
// Ported from gemaskgen.cpp.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_TOOLS_GEIMAGEWRITER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_TOOLS_GEIMAGEWRITER_H_

#include <string>
#include <vector>

//#include "common/khTypes.h"
#include <cstdint>
#include "common/khExtents.h"

class GDALRasterBand;
class GDALDriver;

class khGeoExtents;
class khRasterProductLevel;

class BandInfo {
 public:
  GDALRasterBand *hostBand;
  khExtents<std::uint32_t> extract_extents_;
  khTypes::StorageEnum datatype;

  BandInfo(GDALRasterBand *band,
           const khExtents<std::uint32_t> &extents,
           khTypes::StorageEnum type);
};

class geImageWriter {
 public:
  static void
  ExtractLevelImage(const khRasterProductLevel &prodLevel,
                    const khExtents<std::uint32_t> &extract_extents,
                    GDALDriver *driver,
                    const std::string &output,
                    const khGeoExtents &outputGeoExtents,
                    const bool is_mercator,
                    char **output_options);

  static void
  WriteImageryPreview(const khRasterProductLevel &prodLevel,
                      const khExtents<std::uint32_t> &extract_extents,
                      unsigned char *alphaBuf,
                      GDALDriver *driver,
                      const std::string &output,
                      const khGeoExtents &outputGeoExtents,
                      const bool is_mercator,
                      char **output_options);

  static void
  WriteHeightmapPreview(const khRasterProductLevel &prodLevel,
                        const khExtents<std::uint32_t> &extract_extents,
                        unsigned char *alphaBuf,
                        GDALDriver *driver,
                        const std::string &output,
                        const khGeoExtents &outputGeoExtents,
                        const bool is_mercator,
                        char **output_options);

  static void
  WriteAlphaImage(const khSize<std::uint32_t> &extract_size,
                  unsigned char *alphaBuf,
                  GDALDriver *driver,
                  const std::string &output,
                  const khGeoExtents &outputGeoExtents,
                  const bool is_mercator,
                  char **output_options);

  static void
  WriteAlphaImage(const khSize<std::uint32_t> &extract_size,
                  unsigned char *alphaBuf,
                  GDALDriver *driver,
                  const std::string &output,
                  const khGeoExtents &outputGeoExtents,
                  const char* projection,
                  char **output_options);

  static void
  CopyAndWriteImage(const khSize<std::uint32_t>& rasterSize,
                    const std::vector<BandInfo> &bands,
                    GDALDriver *driver,
                    const std::string &output,
                    const khGeoExtents &outputGeoExtents,
                    const bool is_mercator,
                    char **output_options);

  static void
  CopyAndWriteImage(const khSize<std::uint32_t>& rasterSize,
                    const std::vector<BandInfo> &bands,
                    GDALDriver *driver,
                    const std::string &output,
                    const khGeoExtents &outputGeoExtents,
                    const char* projection,
                    char **output_options);
 private:
  // Do not instantiate
  geImageWriter() {}
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_TOOLS_GEIMAGEWRITER_H_

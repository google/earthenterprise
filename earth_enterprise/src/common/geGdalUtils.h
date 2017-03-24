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

//

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_GEGDALUTILS_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_GEGDALUTILS_H_

#include <gdal_priv.h>

class geGdalUtils {
 public:
  geGdalUtils();
  // Combines JPEG data and alpha channel to PNG.
  // If image_alpha is NULL, then just converts source image data to png format.
  // Returns PNG using pimage_blend string pointer.
  static void BlendToPng(const std::string& pimage_data,
                         const std::string* image_alpha,
                         std::string* pimage_blend);

  static void ConvertToJpeg(const std::string& pimage_data,
                            std::string* pimage_blend);
};

class geGdalVSI {
 public:
  // Mutex wrappers to encapsulate UniqueVSIFilename creation and GDAL open
  // functions preventing identical filename collisions.
  static GDALDataset* VsiGdalCreateWrap(GDALDriverH hdriver,
                                        std::string* const vsifile,
                                        int nx, int ny, int nbands,
                                        GDALDataType bandtype,
                                        char **papszoptions);

  static GDALDataset* VsiGdalCreateCopyWrap(GDALDriverH hdriver,
                                            std::string* const vsifile,
                                            GDALDatasetH hdataset,
                                            int bstrict,
                                            char **papszoptions,
                                            GDALProgressFunc progressfunc,
                                            void *progressdata);

  static GDALDataset* VsiGdalOpenInternalWrap(std::string* const vsifile,
                                              const std::string& image_alpha);

  // Initialize random seed for unique VSI file generation.
  static void InitVSIRandomSeed();

 private:
  static khMutex& GetMutex();
};


#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_GEGDALUTILS_H_

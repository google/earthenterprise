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


#include "khGDALBuffer.h"
#include "khgdal.h"


CPLErr
khGDALBuffer::Read(GDALDataset *srcDS,
                   const khExtents<std::uint32_t> &srcExtents)
{
  // convert some of my args to GDAL format
  std::vector<int> gdalBands(bands.size());
  for (unsigned int b = 0; b < bands.size(); ++b) {
    gdalBands[b] = bands[b]+1;
  }
  unsigned int pixelSize = khTypes::StorageSize(pixelType);
  GSpacing nPixelSpace = pixelStep * pixelSize;
  GSpacing nLineSpace  = lineStep  * pixelSize;
  GSpacing nBandSpace  = bandStep  * pixelSize;

  return srcDS->RasterIO(GF_Read,
                         srcExtents.beginX(), srcExtents.beginY(),
                         srcExtents.width(), srcExtents.height(),
                         buf,
                         srcExtents.width(), srcExtents.height(),
                         GDTFromStorageEnum(pixelType),
                         gdalBands.size(), &gdalBands[0],
                         nPixelSpace, nLineSpace, nBandSpace);
}

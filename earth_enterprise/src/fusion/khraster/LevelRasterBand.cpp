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


#include "LevelRasterBand.h"

// ****************************************************************************
// ***  LevelRasterBand
// ****************************************************************************
LevelRasterBand::LevelRasterBand(GDALDataset *ds, int gdalBand,
                                 unsigned int extractBand_,
                                 const khRasterProductLevel &plev,
                                 khTypes::StorageEnum type) :
    prodLevel(&plev),
    extractType(type),
    extractBand(extractBand_) {
  poDS = ds;
  // Set nBand if it is not free-standing raster band.
  if (poDS != NULL) {
    assert(gdalBand != 0);
    nBand = gdalBand;
  } else {
    assert(gdalBand == 0);
  }
  nRasterXSize = prodLevel->tileExtents().numCols() *
                 RasterProductTileResolution;
  nRasterYSize = prodLevel->tileExtents().numRows() *
                 RasterProductTileResolution;
  eAccess = GA_ReadOnly;
  eDataType = GDTFromStorageEnum(extractType);
  nBlockXSize = RasterProductTileResolution;
  nBlockYSize = RasterProductTileResolution;
}

CPLErr LevelRasterBand::IReadBlock(int x, int y, void *buf) {
  return prodLevel->ReadTileBandIntoBuf
    (prodLevel->tileExtents().beginRow() +
     prodLevel->tileExtents().numRows() - y - 1, /* row */
     prodLevel->tileExtents().beginCol() + x, /* col */
     extractBand,
     reinterpret_cast<unsigned char*>(buf),
     extractType,
     RasterProductTileResolution *
     RasterProductTileResolution *
     khTypes::StorageSize(extractType),
     true /* flipTopToBottom */)
    ? CE_None : CE_Failure;
}

GDALColorInterp LevelRasterBand::GetColorInterpretation() {
  switch (nBand) {
    case 1:
      return GCI_RedBand;
    case 2:
      return GCI_GreenBand;
    case 3:
      return GCI_BlueBand;
  }
  return GCI_Undefined;
}

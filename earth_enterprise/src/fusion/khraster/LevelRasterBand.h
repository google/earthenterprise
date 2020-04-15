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


#ifndef __LevelRasterBand_h
#define __LevelRasterBand_h

#include "khRasterProductLevel.h"
#include <cpl_error.h>
#include <khgdal/khgdal.h>


class LevelRasterBand : public GDALRasterBand
{
  const khRasterProductLevel *prodLevel;
  khTypes::StorageEnum extractType;
  unsigned int extractBand;

 public:
  LevelRasterBand(GDALDataset *ds, int gdalBand,
                  unsigned int extractBand_,
                  const khRasterProductLevel &plev,
                  khTypes::StorageEnum type);

  virtual CPLErr IReadBlock( int x, int y, void *buf );
  virtual GDALColorInterp GetColorInterpretation();
};



#endif /* __LevelRasterBand_h */

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

#ifndef __khgdal_h
#define __khgdal_h

#include <string>
#include <gdal.h>
#include <gdal_priv.h>
#include <cpl_conv.h>
#include <ogr_spatialref.h>
#include "common/notify.h"
#include "common/khTypes.h"
#include <cstdint>


// ****************************************************************************
// ***  Guard class to try and control GDAL's HORRIBLE memory ownership
// ***  semantics
// ****************************************************************************
class CPLStrGuard
{
  // it would be a waste to copy the strings
  CPLStrGuard(const CPLStrGuard &o) = delete;
  CPLStrGuard& operator=(const CPLStrGuard &o) = delete;

  char *str;
 public:
  inline CPLStrGuard(void) : str(0) { }
  inline explicit CPLStrGuard(char *s) : str(s) { }
  inline ~CPLStrGuard(void) {
    if (str)
      CPLFree(str);
  }

  inline operator char*(void) const { return str; }
  inline char* operator->(void) const { return str; }
  inline char& operator* (void) const { return *str; }
  inline char& operator[] (size_t i) const { return str[i]; }
};



// ****************************************************************************
// ***  Misc GDAL wrappers - to simplify API
// ****************************************************************************
extern void khGDALInit(std::uint32_t cacheMax = 400 * 1024 * 1024);
extern GDALDataType GDTFromStorageEnum(khTypes::StorageEnum storage);
extern GDALDataType GDTFromName(const std::string &name);
extern GDALColorInterp GCIFromName(const std::string &name);
// will return false if can't map, true otherwise
extern bool StorageEnumFromGDT(GDALDataType type,
                               khTypes::StorageEnum &storage);


inline bool IsTransformTopToBottom(const double geoTransform[6]) {
  return (geoTransform[5] < 0);
}
inline bool IsTransformLeftToRight(const double geoTransform[6]) {
  return (geoTransform[1] > 0);
}


const OGRSpatialReference& GetWGS84OGRSpatialReference(void);
const OGRSpatialReference& GetMercatorOGRSpatialReference(void);

const std::string& GetWGS84ProjString(void);
const std::string& GetMercatorProjString(void);

bool IsMercator(const char* projectionReference);

// Converts longitude value in degree to a meter coordinate so that it can be
// used for gdalwarp. Note that this meter unit is not actual meter. It is just
// the meter unit that gdal corresponds to the given degree lat-long.
void ConvertToGdalMeterForMercator(double* latitude, double* longitude);

extern bool CompatiblePixelSizes(double ps1, double ps2, std::uint32_t rasterSize);


extern void PrintGDALCreateFormats(FILE *out, const char *prefix);
extern void ValidateGDALCreateFormat(const std::string &fmt);


// These can throw exceptions
void LoadPRJFile(const std::string &fname, OGRSpatialReference &ogrSRS);
void InterpretSRSString(const std::string &buf, OGRSpatialReference &ogrSRS);
std::string GetWKT(const OGRSpatialReference &ogrSRS);

// ****************************************************************************
// ***  RasterBand to acces one band of data from a RAMBuffer
// ****************************************************************************
class BufferRasterBand : public GDALRasterBand
{
  const unsigned char* buf;
  const std::uint32_t pixelSize;

 public:
  BufferRasterBand(GDALDataset *ds, int gdalBand,
                   const unsigned char *const buf_,
                   const khSize<std::uint32_t> &rasterSize,
                   khTypes::StorageEnum extractType);

  virtual CPLErr IReadBlock(int x, int y, void *destBuf);
};


#endif /* __khgdal_h */

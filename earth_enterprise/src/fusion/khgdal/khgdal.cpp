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


#include "fusion/khgdal/khgdal.h"

#include <cmath>

#include "common/khstl.h"
#include <gdal_priv.h>
#include <gdalwarper.h>
#include "common/notify.h"
#include "common/khFileUtils.h"
#include "common/khGuard.h"
#include "common/khTileAddr.h"
#include "common/khException.h"
#include "common/khCalcHelper.h"
#include "common/khGeomUtils.h"


// ****************************************************************************
// ***  khGDALInit()
// ****************************************************************************
extern "C" void GDALRegister_KHM(void);
extern "C" void GDALRegister_KHVR(void);
void
khGDALInit(std::uint32_t cacheMax)
{
  static bool done = false;

  if (!done) {
    done = true;
    GDALAllRegister();
    GDALRegister_KHM();
    GDALRegister_KHVR();

    if (getenv("GDAL_CACHEMAX") == NULL) {
      if ((std::int64_t)GDALGetCacheMax64() < (std::int64_t)cacheMax) {
        GDALSetCacheMax64(cacheMax);
      }
    }
  }
}

const OGRSpatialReference& GetWGS84OGRSpatialReference(void) {
  static OGRSpatialReference* srs = NULL;
  if (!srs) {
    srs = new OGRSpatialReference;
    srs->SetWellKnownGeogCS("WGS84");
  }
  return *srs;
}

const OGRSpatialReference& GetMercatorOGRSpatialReference(void) {
  static OGRSpatialReference* srs = NULL;
  if (!srs) {
    srs = new OGRSpatialReference;
    // Got this string from http://proj.maptools.org/faq.html,
    srs->importFromProj4("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 "
                         "+lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m "
                         "+nadgrids=@null +wktext  +no_defs");
  }
  return *srs;
}

const std::string&
GetWGS84ProjString(void) {
  static std::string wgs84;
  if (wgs84.empty()) {
    wgs84 = GetWKT(GetWGS84OGRSpatialReference());
  }
  return wgs84;
}

const std::string&
GetMercatorProjString(void) {
  static std::string mercator;
  if (mercator.empty()) {
    mercator = GetWKT(GetMercatorOGRSpatialReference());
  }
  return mercator;
}

bool IsMercator(const char* projectionReference) {
  OGRSpatialReference srs;
  char* wkt = const_cast<char*>(projectionReference);
  srs.importFromWkt(&wkt);
  return GetMercatorOGRSpatialReference().IsSame(&srs);
}


void ConvertToGdalMeterForMercator(double* latitude, double* longitude) {
  OGRSpatialReference* const hProj = const_cast<OGRSpatialReference*>(
           &GetMercatorOGRSpatialReference());
  OGRSpatialReference* const hLatLong = const_cast<OGRSpatialReference*>(
           &GetWGS84OGRSpatialReference());
  OGRCoordinateTransformation* hTransform =
      reinterpret_cast<OGRCoordinateTransformation*>(
          OCTNewCoordinateTransformation(hLatLong, hProj));
  assert(hTransform != NULL);

  assert(*longitude > -360. && *longitude < 360.);
  if (*longitude < -180. || *longitude > 180.) {
    // if source degree longitude is out of [-180, 180] we calculate meter
    // longitude in two steps:
    // meter_longitude = meter_longitude_for({-,+}180) +
    //   meter_longitude_for_reminder(source_degree_longitude - {-,+}180).
    double tmp_longitude = (*longitude < -180.) ? -180. : 180.;
    double tmp_latitude = *latitude;
    *longitude -= tmp_longitude;
    OCTTransform(hTransform, 1, &tmp_longitude, &tmp_latitude, NULL);
    OCTTransform(hTransform, 1, longitude, latitude, NULL);
    *longitude += tmp_longitude;
  } else {
    OCTTransform(hTransform, 1, longitude, latitude, NULL);
  }

  // Now latitude and longitude in Meter units that GDAL understands as input
  OCTDestroyCoordinateTransformation(hTransform);
}


GDALDataType
GDTFromName(const std::string &name)
{
  for (int type = GDT_Unknown; type < GDT_TypeCount; ++type) {
    const char *GDTName = GDALGetDataTypeName((GDALDataType)type);
    if (GDTName && name == GDTName)
      return (GDALDataType)type;
  }
  return GDT_Unknown;
}

GDALColorInterp
GCIFromName(const std::string &name)
{
  for (int i = GCI_Undefined; i <= GCI_BlackBand; ++i) {
    if (name == GDALGetColorInterpretationName((GDALColorInterp)i   ))
      return (GDALColorInterp)i;
  }
  return GCI_Undefined;
}

GDALDataType
GDTFromStorageEnum(khTypes::StorageEnum storage)
{
  switch (storage) {
    case khTypes::UInt8:
      return GDT_Byte;
    case khTypes::Int8:
      return GDT_Byte;
    case khTypes::UInt16:
      return GDT_UInt16;
    case khTypes::Int16:
      return GDT_Int16;
    case khTypes::UInt32:
      return GDT_UInt32;
    case khTypes::Int32:
      return GDT_Int32;
    case khTypes::UInt64:
      return GDT_Unknown;
    case khTypes::Int64:
      return GDT_Unknown;
    case khTypes::Float32:
      return GDT_Float32;
    case khTypes::Float64:
      return GDT_Float64;
  };
  return GDT_Unknown;
}

// will return false if can't map, true otherwise
bool
StorageEnumFromGDT(GDALDataType type,
                   khTypes::StorageEnum &storage)
{
  switch (type) {
    case GDT_Byte:
      storage = khTypes::UInt8;
      break;
    case GDT_UInt16:
      storage = khTypes::UInt16;
      break;
    case GDT_Int16:
      storage = khTypes::Int16;
      break;
    case GDT_UInt32:
      storage = khTypes::UInt32;
      break;
    case GDT_Int32:
      storage = khTypes::Int32;
      break;
    case GDT_Float32:
      storage = khTypes::Float32;
      break;
    case GDT_Float64:
      storage = khTypes::Float64;
      break;
    case GDT_CInt16:
    case GDT_CInt32:
    case GDT_CFloat32:
    case GDT_CFloat64:
    case GDT_TypeCount:
    case GDT_Unknown:
      return false;
  }
  return true;
}


void
CalcBlockRange(const khExtents<std::uint32_t> &extents,
               const khExtents<std::uint32_t> &inset,
               const khSize<std::uint32_t> &insetBlocksize,
               std::uint32_t &beginX, std::uint32_t &endX,
               std::uint32_t &beginY, std::uint32_t &endY)
{
  std::uint32_t offx = inset.beginX() - extents.beginX();
  std::uint32_t offy = inset.beginY() - extents.beginY();

  beginX = offx / insetBlocksize.width;
  endX   = (offx + inset.width() + insetBlocksize.width - 1) /
           insetBlocksize.width;
  beginY = offy / insetBlocksize.height;
  endY   = (offy + inset.height() + insetBlocksize.height - 1) /
           insetBlocksize.height;
}


bool
CompatiblePixelSizes(double ps1, double ps2, std::uint32_t rasterSize)
{
  // check that both pixel sizes have the same sign
  if ((ps1 < 0) != (ps2 < 0))
    return false;

  double aps1 = fabs(ps1);
  double aps2 = fabs(ps2);

  // If the difference across the rasterSize is less than .5 of a pixel
  // they are compatible
#if 0
  return (fabs((aps1 * rasterSize) - (aps2 * rasterSize)) <
          (std::min(aps1, aps2) * 0.5));
#else
  return (fabs((aps1 * rasterSize) - (aps2 * rasterSize)) <
          (aps2 * 0.5));
#endif
}


void
PrintGDALCreateFormats(FILE *out, const char *prefix)
{
  khGDALInit();
  for (int iDr = 0; iDr < GDALGetDriverCount(); iDr++) {
    GDALDriverH hDriver = GDALGetDriver(iDr);
    if ((GDALGetMetadataItem(hDriver, GDAL_DCAP_CREATE, 0) != 0) ||
        (GDALGetMetadataItem(hDriver, GDAL_DCAP_CREATECOPY, 0) != 0)) {
      fprintf(out, "%s%s: %s\n",
              prefix,
              GDALGetDriverShortName(hDriver),
              GDALGetDriverLongName(hDriver));
    }
  }
}


void
ValidateGDALCreateFormat(const std::string &fmt)
{
  khGDALInit();
  GDALDriverH driver = GDALGetDriverByName(fmt.c_str());
  QString msg;
  if (!driver) {
    msg = kh::tr("%1 is not a recognized format").arg(fmt.c_str());
  } else if ((GDALGetMetadataItem(driver, GDAL_DCAP_CREATE, 0) == 0) &&
             (GDALGetMetadataItem(driver, GDAL_DCAP_CREATECOPY, 0) == 0)) {
    msg = kh::tr("%1 format does not support writing").arg(fmt.c_str());
  } else {
    return;
  }
  throw khException(msg);
}


void
LoadPRJFile(const std::string &fname, OGRSpatialReference &ogrSRS)
{
  std::string buf;
  if (khReadStringFromFile(fname, buf)) {
    try {
      InterpretSRSString(buf, ogrSRS);
    } catch (const std::exception &e) {
      throw khException(kh::tr("%1: %2").arg(fname.c_str()).arg(e.what()));
    } catch (...) {
      throw khException(kh::tr("%1: Unknown exception in LoadPRJFile").arg(fname.c_str()));
    }
  } else {
    // more specific message will already have been emitted with NFY_WARN
    throw khException(kh::tr("Unable to read %1").arg(fname.c_str()));
  }
}



void
InterpretSRSString(const std::string &buf, OGRSpatialReference &ogrSRS)
{

  ogrSRS.Clear();
  OGRErr err = OGRERR_NONE;
  if (buf[0] == '<') {
    err = ogrSRS.importFromXML(buf.c_str());
  } else if (StartsWith(buf, "+proj") ||
             StartsWith(buf, "+init")) {
    err = ogrSRS.importFromProj4(buf.c_str());
  } else if (StartsWith(buf, "PROJCS") ||
             StartsWith(buf, "GEOGCS") ||
             StartsWith(buf, "LOCAL_CS")) {
    // importFromWkt can change contents of string :-(
    // this copies and makes sure it's nul terminated
    std::string tmp = buf.c_str();
    char *wkt = &tmp[0];
    err = ogrSRS.importFromWkt(&wkt);
  } else {
    err = ogrSRS.SetFromUserInput(buf.c_str());
  }

  if (err == OGRERR_NONE) {
    err = ogrSRS.Validate();
    if (err != OGRERR_NONE) {
      if (err == OGRERR_CORRUPT_DATA) {
        throw khException(kh::tr("SRS not well formed"));
      } else if (err == OGRERR_UNSUPPORTED_SRS) {
        throw khException(kh::tr("Unsupported SRS"));
      } else {
        throw khException(kh::tr("Unknown SRS validation error"));
      }
    }
  } else {
    std::string trimmed = TrimWhite(buf);
    if (trimmed != buf) {
      return InterpretSRSString(trimmed, ogrSRS);
    }
    throw khException(kh::tr("Unable to interpret SRS"));
  }
}

std::string
GetWKT(const OGRSpatialReference &ogrSRS)
{
  char *wkt = 0;
  if (ogrSRS.exportToWkt(&wkt) != OGRERR_NONE) {
    throw khException(kh::tr("Unable to normalize SRS"));
  }
  std::string srs = wkt;
  CPLFree(wkt);
  return srs;
}


BufferRasterBand::BufferRasterBand(GDALDataset *ds, int gdalBand,
                                   const unsigned char *const buf_,
                                   const khSize<std::uint32_t> &rasterSize,
                                   khTypes::StorageEnum extractType)
    : buf(buf_),
      pixelSize(khTypes::StorageSize(extractType)) {
  poDS = ds;
  // Set nBand if it is not free-standing raster band.
  if (poDS != NULL) {
    assert(gdalBand != 0);
    nBand = gdalBand;
  } else {
    assert(gdalBand == 0);
  }
  nRasterXSize = rasterSize.width;
  nRasterYSize = rasterSize.height;
  eAccess = GA_ReadOnly;
  eDataType = GDTFromStorageEnum(extractType);
  nBlockXSize = rasterSize.width;
  nBlockYSize = 1;
}

CPLErr BufferRasterBand::IReadBlock(int x, int y, void *destBuf) {
  const unsigned char *srcBuf  = (buf +
                    nRasterXSize * pixelSize * y +
                    nBlockXSize * pixelSize * x);
  memcpy(destBuf, srcBuf, nBlockXSize * pixelSize);
  return CE_None;
}

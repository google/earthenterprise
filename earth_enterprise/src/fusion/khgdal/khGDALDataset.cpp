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


#include "fusion/khgdal/khGDALDataset.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gdalwarper.h>
#include <ogr_spatialref.h>

#include "fusion/khgdal/khgdal.h"
#include "fusion/khgdal/khAddGeoDataset.h"
#include "common/khException.h"
#include "common/khFileUtils.h"
#include "common/khGeomUtils.h"
#include "common/khTileAddr.h"
#include "common/khstl.h"
#include "common/khstrconv.h"


static khGeoExtents
ReadDotGeoFile(const std::string &filename,
               const khSize<std::uint32_t> &rasterSize);


// ****************************************************************************
// ***  khGDALDatasetImpl
// ****************************************************************************
khGDALDatasetImpl::khGDALDatasetImpl(const std::string &filename_,
                                     const std::string &overrideSRS,
                                     const khExtents<double> &overrideExtents,
                                     khTilespace::ProjectionType projType):
    filename(filename_),
    dataset(),
    maskDS(),
    checkedForMask(false),
    geoExtents(),
    ogrSRS(),
    haveNormalized(false),
    normTopLevel(0),
    alignedPixelExtents(),
    alignedGeoExtents(),
    croppedPixelExtents(),
    croppedGeoExtents(),
    needReproject(false),
    needSnapUp(false),
    projectionType(projType)
{
  khGDALInit();  // safe to call multiple times

  bool needAddGeo = false;

  try {
    // get the dataset
    dataset = TransferOwnership(
        (GDALDataset *) GDALOpen(filename.c_str(), GA_ReadOnly));
    if (!dataset) {
      throw khException(kh::tr("Unable to open"));
    }

    khSize<std::uint32_t> rasterSize(dataset->GetRasterXSize(),
                              dataset->GetRasterYSize());

    std::string  srs;
    // get the SRS from one of several sources
    if (!overrideSRS.empty()) {
      // convert the override SRS string to ogrSRS (will validate)
      needAddGeo = true;
      InterpretSRSString(overrideSRS, ogrSRS);
      srs = GetWKT(ogrSRS);
    } else if (khExists(khReplaceExtension(filename, ".prj-force"))) {
      needAddGeo = true;
      LoadPRJFile(khReplaceExtension(filename, ".prj-force"), ogrSRS);
      srs = GetWKT(ogrSRS);
    } else {
      srs = dataset->GetProjectionRef();
      if (srs.size()) {
        // convert the GDAL SRS string to ogrSRS (will validate)
        if ((ogrSRS.SetFromUserInput(srs.c_str()) != OGRERR_NONE) ||
            !ogrSRS.GetRoot()) {
          throw khException(kh::tr("Unrecognized SRS: %1").arg(srs.c_str()));
        }
      } else if (khExists(khReplaceExtension(filename, ".prj"))) {
        needAddGeo = true;
        LoadPRJFile(khReplaceExtension(filename, ".prj"), ogrSRS);
        srs = GetWKT(ogrSRS);
      } else if (khExists(khReplaceExtension(filename, ".geo"))) {
        // .geo files imply lat/lon WGS84 projection
        needAddGeo = true;
        srs = GetWGS84ProjString();
      } else {
        throw khException(kh::tr("No SRS"));
      }
    }
    if (srs.empty()) {
      throw khException(kh::tr("Internal Error: Constructor missing SRS"));
    }


    // get the geoExtents from one of several sources
    if (!overrideExtents.empty()) {
      needAddGeo = true;

      // We need to know this raster's orientation in order to build a
      // khGeoExtents.  These are by far the most common. Set them as
      // defaults
      bool topToBottom = true;

      // See if the dataset can tell us what the orientation really is
      double geoTransform[6];
      if (dataset->GetGeoTransform(geoTransform) == CE_None) {
        topToBottom = IsTransformTopToBottom(geoTransform);
        if (!IsTransformLeftToRight(geoTransform)) {
          throw khException(kh::tr("Unsupported right->left geoTransform"));
        }
      }

      geoExtents = khGeoExtents(overrideExtents, rasterSize,
                                topToBottom);
    } else {
      std::string ext = khExtension(filename, false /* incl dot */);
      std::string newext;
      if (!ext.empty()) {
        newext = ".";
        newext += ext[0];
        newext += ext[ext.length()-1];
        newext += 'w';
      }
      std::string overridefname =
        khReplaceExtension(filename,
                           LowerCaseString(newext + "-force"));

      double geoTransform[6];
      if (!newext.empty() && khExists(overridefname)) {
        if (!GDALReadWorldFile
            (khDropExtension(overridefname).c_str(),
             khExtension(overridefname).c_str(), geoTransform)) {
          throw khException
            (kh::tr("Unable to read/interpret %1")
             .arg(overridefname.c_str()));
        }
        needAddGeo = true;
        geoExtents = khGeoExtents(geoTransform, rasterSize);
      } else if (dataset->GetGeoTransform(geoTransform) == CE_None) {
        geoExtents = khGeoExtents(geoTransform, rasterSize);
      } else {
        bool foundExtents = false;
        if (!newext.empty()) {
          std::vector<std::string> sidecars;
          sidecars.push_back(khReplaceExtension
                             (filename, LowerCaseString(newext)));
          sidecars.push_back(khReplaceExtension
                             (filename, UpperCaseString(newext)));
          sidecars.push_back(khReplaceExtension(filename, ".tfw"));
          sidecars.push_back(khReplaceExtension(filename, ".TFW"));
          for (std::vector<std::string>::const_iterator sidecar =
                 sidecars.begin(); sidecar != sidecars.end();
               ++sidecar) {
            if (khExists(*sidecar)) {
              needAddGeo = true;
              foundExtents = true;
              if (!GDALReadWorldFile
                  (khDropExtension(*sidecar).c_str(),
                   khExtension(*sidecar).c_str(), geoTransform)) {
                throw khException
                  (kh::tr("Unable to read/interpret %1")
                   .arg(sidecar->c_str()));
              }
              geoExtents = khGeoExtents(geoTransform,rasterSize);
              break;
            }
          }
        }

        if (!foundExtents) {
          if (khExists(khReplaceExtension(filename, ".geo"))) {
            std::string geofile(khReplaceExtension(filename,
                                                   ".geo"));
            needAddGeo = true;
            geoExtents = ReadDotGeoFile(geofile, rasterSize);
            if (geoExtents.empty()) {
              throw khException
                (kh::tr("Unable to read %1").arg(geofile.c_str()));
            }
          } else {
            throw khException(kh::tr("No geographic extents"));
          }
        }
      }
    }
    if (geoExtents.empty()) {
      throw khException
        (kh::tr("Internal Error: Missing geograpic extents"));
    }

    // wrap the dataset if we got some info (SRS or geoExtents) from
    // somewhere other than the dataset. Extract the raw dataset out of the
    // delete guard with take() because the khAddGeoDataset will assume
    // ownership
    if (needAddGeo) {
      dataset = TransferOwnership(
          new khAddGeoDataset(TransferOwnership(dataset), srs, geoExtents));
    }

    // Defer looking for sidecar alpha mask until it's actually used
    // See getMaskDS below

  } catch (const std::exception &e) {
    throw khException(kh::tr("%1: %2").arg(filename.c_str()).arg(e.what()));
  } catch (...) {
    throw khException(kh::tr("%1: Unknown error while opening")
                      .arg(filename.c_str()));
  }
}

GDALDataset*
khGDALDatasetImpl::getMaskDS() const
{
  if (!maskDS && !checkedForMask) {
    khGDALDatasetImpl *self = const_cast<khGDALDatasetImpl*>(this);
    self->checkedForMask = true;

    // Look for sidecar alpha mask
    if (khExists(khReplaceExtension(filename, "-mask.tif")))
      self->maskDS = TransferOwnership(
          (GDALDataset *) GDALOpen(
              khReplaceExtension(filename, "-mask.tif").c_str(), GA_ReadOnly));
    else if (khExists(khReplaceExtension(filename, "-mask.img")))
      self->maskDS = TransferOwnership(
          (GDALDataset *) GDALOpen(
              khReplaceExtension(filename, "-mask.img").c_str(), GA_ReadOnly));
  }
  return maskDS;
}


void
khGDALDatasetImpl::EnsureKeyholeNormalizedInfo(void)
{
  if (haveNormalized) return;

  try {
    // By the time we get here the srs should already have been validated
    std::string srs = dataset->GetProjectionRef();
    if (srs.empty()) {
      throw khException(kh::tr("Internal Error: Normalize missing SRS"));
    }
    OGRSpatialReference ogrSRS;
    const char *wkt = srs.c_str();
    if ((ogrSRS.importFromWkt((char**)&wkt) != OGRERR_NONE) ||
        !ogrSRS.GetRoot()) {
      throw khException(kh::tr("Internal Error: Unrecognized SRS"));
    }

    // someplace to put the results before they are aligned and cropped
    khSize<std::uint32_t> tmpRasterSize;
    khGeoExtents   tmpGeoExtents;

    // See if it's already in keyhole standard format. That's a WGS84
    // projection _or_ Mercator projection and a simple geo transformation
    // (image row/col maps directly to geo lat/lon - no rotation)
    const OGRSpatialReference& khStdSrs = IsMercator() ?
        GetMercatorOGRSpatialReference() : GetWGS84OGRSpatialReference();

    if (khStdSrs.IsSame(&ogrSRS) && geoExtents.noRotation()) {
      // It's already how we want it.
      needReproject = false;

      // extract the pixel width from my geoExtents and use it
      // to find the top level
      double pixelWidth   = geoExtents.absPixelWidth();
      double pixelHeight  = geoExtents.absPixelHeight();
      unsigned int widthTopLevel  = IsMercator() ?
        RasterProductTilespaceMercator.LevelFromPixelSizeInMeters(pixelWidth) :
        RasterProductTilespaceFlat.LevelFromDegPixelSize(pixelWidth);
      unsigned int heightTopLevel = IsMercator() ?
        RasterProductTilespaceMercator.LevelFromPixelSizeInMeters(pixelHeight) :
        RasterProductTilespaceFlat.LevelFromDegPixelSize(pixelHeight);

      normTopLevel = std::max(widthTopLevel, heightTopLevel);
      double topLevelPixelSize = IsMercator() ?
        RasterProductTilespaceMercator.AveragePixelSizeInMercatorMeters(
            normTopLevel) :
        RasterProductTilespaceFlat.DegPixelSize(normTopLevel);

      // If the snapped up pixel size is different enough that it
      // will make more than a half a pixel difference over the
      // product tiles that will be generated, then we need to snap it up
      needSnapUp = false;
      khLevelCoverage tileCov(RasterProductTilespace(IsMercator()),
                              geoExtents.extents(),
                              normTopLevel /* fullresLevel */,
                              normTopLevel /* targetLevel */,
                              IsMercator());
      khSize<std::uint32_t> expandedRasterSize(tileCov.extents.width() *
                                        RasterProductTileResolution,
                                        tileCov.extents.height() *
                                        RasterProductTileResolution);

      if (!CompatiblePixelSizes(pixelWidth, topLevelPixelSize,
                                expandedRasterSize.width) ||
          !CompatiblePixelSizes(pixelHeight, topLevelPixelSize,
                                expandedRasterSize.height)) {
        needSnapUp = true;
      }

      if (needSnapUp) {
        // Found that our snapping function doesn't match with that of gdal. So
        // for the time being avoiding our snapping function.
        needSnapUp = false;
        needReproject = true;
        // TODO: correct snap transform and use that.
        ComputeReprojectSnapupExtents(dataset,
                                      srs, srs,
                                      tmpRasterSize,
                                      tmpGeoExtents,
                                      normTopLevel,
                                      IsMercator());
      } else {
        // normTopLevel is already set
        tmpRasterSize = rasterSize();
        tmpGeoExtents = geoExtents;
      }
    } else {  // Need reprojection.
      needSnapUp = false;
      needReproject = true;
      std::string projString = IsMercator() ? GetMercatorProjString()
                                            : GetWGS84ProjString();
      ComputeReprojectSnapupExtents(dataset,
                                    srs, projString,
                                    tmpRasterSize,
                                    tmpGeoExtents,
                                    normTopLevel,
                                    IsMercator());
    }
    if (khCutExtent::cut_extent != khCutExtent::world_extent) {
      // Cut to the boundaries provided
      if (IsMercator()) {
        const khExtents<double> cut_extent =
            khCutExtent::ConvertFromFlatToMercator(khCutExtent::cut_extent);
        tmpRasterSize = tmpGeoExtents.boundToExtent(cut_extent);
      } else {
        tmpRasterSize = tmpGeoExtents.boundToExtent(khCutExtent::cut_extent);
      }
      if (tmpRasterSize.width == 0 || tmpRasterSize.height == 0) {
        char buff[1024];
        snprintf(buff, sizeof(buff),
                 "Because of the boundaries %.20f (N) %.20f (S)"
                 " %.20f (E) %.20f (W) the resulting raster is"
                 " of zero area.",
                 khCutExtent::cut_extent.north(),
                 khCutExtent::cut_extent.south(),
                 khCutExtent::cut_extent.east(),
                 khCutExtent::cut_extent.west());
        throw khException(kh::tr(buff));
      }
    }

    // now that we've got the src on one or more of our levels, let's align it
    // with the pixels on that level and crop it at the world extents.
    AlignAndCropExtents(normTopLevel, tmpRasterSize, tmpGeoExtents,
                        IsMercator());
    haveNormalized = true;
  } catch (const std::exception &e) {
    throw khException(kh::tr("%1: %2").arg(filename.c_str()).arg(e.what()));
  } catch (...) {
    throw khException
      (kh::tr("%1: Unknown error calculating normalized geo extents")
       .arg(filename.c_str()));
  }
}


void
khGDALDatasetImpl::GetReprojectedPixelSizeInDegrees(
    GDALDataset* srcDataSet,
    double* pixelWidthInDegrees,
    double* pixelHeightInDegrees)
{
  const std::string srcProjectionReference = srcDataSet->GetProjectionRef();
  void* transformArg = GDALCreateGenImgProjTransformer(
      srcDataSet,
      srcProjectionReference.c_str(),
      NULL,                            /* destination DataSet */
      GetWGS84ProjString().c_str(),
      FALSE,                           /* use GCP if necessary */
      0.0,                             /* GCP error threshold */
      0                                /* maximum order for GCP polynomials */);
  if (!transformArg) {
    throw khException(kh::tr("Unable to determine reproject transform"));
  }
  khCallGuard<void*> transformArgGuard(GDALDestroyGenImgProjTransformer,
                                       transformArg);
  // estimate the pixel sizes and raster size after a reproject
  // the returned geoTransform is always top->bottom, left->right
  // (e.g. geoTransform[5] < 0)
  int guessNumPixels, guessNumLines;
  double geoTransform[6];
  if (GDALSuggestedWarpOutput(srcDataSet, GDALGenImgProjTransform,
                              transformArg, geoTransform,
                              &guessNumPixels, &guessNumLines) != CE_None) {
    throw khException(kh::tr("Unable to estimate reproject size "));
  }
  *pixelWidthInDegrees  = fabs(geoTransform[1]);
  *pixelHeightInDegrees = fabs(geoTransform[5]);
}

// TODO: Add a unit test case.
void
khGDALDatasetImpl::ComputeReprojectSnapupExtents(GDALDataset *srcDS,
                                                 const std::string &srcSRS,
                                                 const std::string &dstSRS,
                                                 khSize<std::uint32_t> &rasterSize,
                                                 khGeoExtents   &geoExtents,
                                                 unsigned int           &level,
                                                 const bool is_mercator)
{
  if (!is_mercator) {
    // The basic flow for this routine came from gdal_warp's
    // GDALWarpCreateOutput()

    // TODO: do refactoring of GetReprojectedPixelSizeInDegrees()
    // and use it here and below for mercator branch.

    // make a transform w/o the dstDS.
    void* transformArg = GDALCreateGenImgProjTransformer
                         (srcDS,          srcSRS.c_str(),
                          0 /* dstDS */,  dstSRS.c_str(),
                          FALSE,  /* use GCP if necessary */
                          0.0,    /* GCP error threshold */
                          0       /* maximum order for GCP polynomials */);

    if (!transformArg) {
      throw khException(kh::tr("Unable to determine reproject transform"));
    }
    khCallGuard<void*> transformArgGuard(GDALDestroyGenImgProjTransformer,
                                         transformArg);

    // estimate the pixel sizes and raster size after a reproject
    // the returned geoTransform is always top->bottom, left->right
    // (e.g. geoTransform[5] < 0)
    int guessNumPixels, guessNumLines;
    double geoTransform[6];
    if (GDALSuggestedWarpOutput(srcDS, GDALGenImgProjTransform,
                                transformArg, geoTransform,
                                &guessNumPixels, &guessNumLines) != CE_None) {
      throw khException(kh::tr("Unable to estimate reproject size "));
    }
    notify(NFY_DEBUG, "Calculated warp size (wh) %d,%d",
           guessNumPixels, guessNumLines);
    double guessPixelWidth  = geoTransform[1];
    double guessPixelHeight = -geoTransform[5];
    double guessImageWidth   = guessPixelWidth * guessNumPixels;
    double guessImageHeight  = guessPixelHeight * guessNumLines;

    // adjust the estimated sizes to snap up to one of our levels
    level = RasterProductTilespaceFlat.LevelFromDegPixelSize(guessPixelWidth);
    if (level !=
        RasterProductTilespaceFlat.LevelFromDegPixelSize(guessPixelHeight)) {
      throw khException(kh::tr("Incompatible x/y warped pixel sizes"));
    }
    double pixelSize = RasterProductTilespaceFlat.DegPixelSize(level);
    double halfPixelSize = pixelSize / 2.0;
    int numPixels = (int)((guessImageWidth + halfPixelSize) / pixelSize);
    int numLines  = (int)((guessImageHeight + halfPixelSize) / pixelSize);
    geoTransform[1] = pixelSize;
    geoTransform[5] = -pixelSize;

    notify(NFY_DEBUG, "Snapped up size (wh) %d,%d", numPixels, numLines);
    rasterSize = khSize<std::uint32_t>(numPixels /* width */,
                                numLines  /* height */);
    geoExtents = khGeoExtents(geoTransform, rasterSize);
  } else {  // Destination dataset has mercator projection.
    // We know gdal crops image in 'y' by default. So let us first find the
    // original image's extents

    // TODO: Verify latitude conversion in
    // ConvertToGdalMeterForMercator() and test do we need such two steps
    // procedure to calculate min/max latitude/longitude in case of destination
    // dataset has mercator projection.
    void* reprojTran = GDALCreateGenImgProjTransformer
                       (
                        srcDS,          srcSRS.c_str(),
                        0 /* dstDS */,  GetWGS84ProjString().c_str(),
                        FALSE,  /* use GCP if necessary */
                        0.0,    /* GCP error threshold */
                        0       /* maximum order for GCP polynomials */);
    if (!reprojTran) {
      throw khException(kh::tr("Unable to determine reproject transform"));
    }
    khCallGuard<void*> reprojTranGuard(GDALDestroyGenImgProjTransformer,
                                       reprojTran);
    int width, height;
    double geoTransform[6];
    if (GDALSuggestedWarpOutput(srcDS, GDALGenImgProjTransform,
                                reprojTran, geoTransform,
                                &width, &height) != CE_None) {
      throw khException(kh::tr("Unable to estimate reproject size "));
    }

    // latitude and longitude are in degree
    double maxLatitude = geoTransform[3];
    double minLatitude = geoTransform[3] + geoTransform[5] * height;
    maxLatitude = Clamp(maxLatitude,
      -khGeomUtilsMercator::khMaxLatitude, khGeomUtilsMercator::khMaxLatitude);
    minLatitude = Clamp(minLatitude,
      -khGeomUtilsMercator::khMaxLatitude, khGeomUtilsMercator::khMaxLatitude);
    double minLongitude = geoTransform[0];
    double maxLongitude = geoTransform[0] + geoTransform[1] * width;

    // convert latitude and longitude to meter
    ConvertToGdalMeterForMercator(&minLatitude, &minLongitude);
    ConvertToGdalMeterForMercator(&maxLatitude, &maxLongitude);

    // make a transform w/o the dstDS.
    void* transformArg = GDALCreateGenImgProjTransformer
                         (srcDS,          srcSRS.c_str(),
                          0 /* dstDS */,  dstSRS.c_str(),
                          FALSE,  /* use GCP if necessary */
                          0.0,    /* GCP error threshold */
                          0       /* maximum order for GCP polynomials */);

    if (!transformArg) {
      throw khException(kh::tr("Unable to determine reproject transform"));
    }
    khCallGuard<void*> transformArgGuard(GDALDestroyGenImgProjTransformer,
                                         transformArg);

    for (int i = 0; i < 6; ++i) {
      geoTransform[i] = 0;
    }
    // estimate the pixel sizes and raster size after a reproject
    // the returned geoTransform is always top->bottom, left->right
    // (e.g. geoTransform[5] < 0)
    int guessWidthInPxl, guessHeightInPxl;
    if (GDALSuggestedWarpOutput(
        srcDS, GDALGenImgProjTransform, transformArg, geoTransform,
        &guessWidthInPxl, &guessHeightInPxl) != CE_None) {
      throw khException(kh::tr("Unable to estimate reproject size "));
    }
    notify(NFY_DEBUG, "Calculated warp size (wh) %d,%d",
           guessWidthInPxl, guessHeightInPxl);

    double guessPixelWidth  = geoTransform[1];

    // TODO: pixel width and height can be different for mercator
    // projection. Below we use only pixel width. May be it's correct in this
    // case because it is used to calculate level and then average pixel size.
    // Need to verify it?! (e.g. use average of width/height. From other hand
    // they are almost equal and nothing is changed in further computing).

    // adjust the estimated sizes to snap up to one of our levels
    level = RasterProductTilespaceMercator.LevelFromPixelSizeInMeters(
        guessPixelWidth);
    double pixelSize =
        RasterProductTilespaceMercator.AveragePixelSizeInMercatorMeters(level);

    geoTransform[0] = minLongitude;
    geoTransform[1] = pixelSize;
    // geoTransform[2] is always 0 in Mercator projection
    // geoTransform[3] i.e max latitude in gdal meters
    geoTransform[3] = maxLatitude;
    // geoTransform[4] is always 0 in Mercator projection
    geoTransform[5] = -pixelSize;
    int widthInPxl = static_cast<int>(
        ((maxLongitude - minLongitude) / pixelSize) + 0.5);
    int heightInPxl = static_cast<int>(
        ((maxLatitude - minLatitude) / pixelSize) + 0.5);

    notify(NFY_DEBUG, "Snapped up size (wh) %d,%d, level %u\n",
           widthInPxl, heightInPxl, level);
    rasterSize = khSize<std::uint32_t>(widthInPxl, heightInPxl);
    geoExtents = khGeoExtents(geoTransform, rasterSize);
  }
}


// TODO: Add a unit test case.
void
khGDALDatasetImpl::AlignAndCropExtents(const unsigned int normTopLevel,
                                       const khSize<std::uint32_t> &inRasterSize,
                                       const khGeoExtents   &inGeoExtents,
                                       bool is_mercator)
{
  if (!is_mercator) {
    const double pixelSize = RasterProductTilespaceFlat.DegPixelSize(
        normTopLevel);
    double halfPixelSize = pixelSize / 2.0;
    const double offset = 360.0 / 2.0;

    // align w/ pixels on our level
    std::int64_t beginXPixel =
      std::int64_t((inGeoExtents.extents().beginX() + halfPixelSize + offset) /
            pixelSize);
    std::int64_t beginYPixel =
      std::int64_t((inGeoExtents.extents().beginY() + halfPixelSize + offset) /
            pixelSize);

    // full pixels extents at this level - can have negative values
    // if origGeoExtents.west() < 180.0
    alignedPixelExtents = khExtents<std::int64_t>(
        khOffset<std::int64_t>(XYOrder, beginXPixel, beginYPixel),
        khSize<std::int64_t>(inRasterSize.width, inRasterSize.height));

    // intersect with world extents
    croppedPixelExtents = khExtents<std::int64_t>(khExtents<std::int64_t>::Intersection(
        alignedPixelExtents,
        RasterProductTilespaceFlat.WorldPixelExtents(normTopLevel)));


    alignedGeoExtents = khGeoExtents(alignedPixelExtents,
                                     pixelSize,
                                     inGeoExtents.topToBottom(), offset);
    croppedGeoExtents = khGeoExtents(croppedPixelExtents,
                                     pixelSize,
                                     inGeoExtents.topToBottom(), offset);
  } else {
    const double pixelSize =
        RasterProductTilespaceMercator.AveragePixelSizeInMercatorMeters(
            normTopLevel);
    // align w/ pixels on our level
    const double offset = khGeomUtilsMercator::khEarthCircumference / 2.0;
    std::int64_t beginXPixel = static_cast<std::int64_t>(
        0.5 +  // For rounding rather than default floor
        ((inGeoExtents.extents().beginX() + offset) / pixelSize));
    std::int64_t beginYPixel = static_cast<std::int64_t>(
        0.5 +  // For rounding rather than default floor
        ((inGeoExtents.extents().beginY() + offset) / pixelSize));

    // full pixels extents at this level - can have negative values
    // if origGeoExtents.west() < 180.0
    alignedPixelExtents = khExtents<std::int64_t>(
        khOffset<std::int64_t>(XYOrder, beginXPixel, beginYPixel),
        khSize<std::int64_t>(inRasterSize.width, inRasterSize.height));

    // interset with world extents
    croppedPixelExtents = khExtents<std::int64_t>(khExtents<std::int64_t>::Intersection(
        alignedPixelExtents,
        RasterProductTilespaceMercator.WorldPixelExtentsMercator(
            normTopLevel)));


    alignedGeoExtents = khGeoExtents(alignedPixelExtents,
                                     pixelSize,
                                     inGeoExtents.topToBottom(), offset);
    croppedGeoExtents = khGeoExtents(croppedPixelExtents,
                                     pixelSize,
                                     inGeoExtents.topToBottom(), offset);
  }
  {
    const double * geoTransform = alignedGeoExtents.geoTransform();
    notify(NFY_DEBUG, "geoTransform[0] %.20f (minLongitude)", geoTransform[0]);
    notify(NFY_DEBUG, "geoTransform[1] %.20f (pixelSize)", geoTransform[1]);
    notify(NFY_DEBUG, "geoTransform[2] %.20f", geoTransform[2]);
    notify(NFY_DEBUG, "geoTransform[3] %.20f (maxLatitude)", geoTransform[3]);
    notify(NFY_DEBUG, "geoTransform[4] %.20f", geoTransform[4]);
    notify(NFY_DEBUG, "geoTransform[5] %.20f (-pixelSize)", geoTransform[5]);
  }
}


// ****************************************************************************
// ***  khGDALDataset
// ****************************************************************************
khGDALDataset::khGDALDataset(const std::string &filename,
                             const std::string &overrideSRS,
                             const khExtents<double> &overrideExtents,
                             khTilespace::ProjectionType projectionType)
    : impl(khRefGuardFromNew
           (new khGDALDatasetImpl(filename,
                                  overrideSRS,
                                  overrideExtents,
                                  projectionType)))
{
}


// ****************************************************************************
// ***  Helpers
// ****************************************************************************
static
khGeoExtents
ReadDotGeoFile(const std::string &filename,
               const khSize<std::uint32_t> &rasterSize)
{
  FILE *geofile;
  if((geofile = fopen(filename.c_str(), "r"))) {
    khFILECloser guard(geofile);

    // start with an implied coordinate mapping
    // from -180.0, 180.0, -90.0, 90.0
    double North = 90.0;
    double South = -90.0;
    double East = 180.0;
    double West = -180.0;

    char str[10][180];
    int geo_pad_to_edge = 1;
    fscanf(geofile, "%179s %179s %179s %179s %179s %179s %179s %179s %179s\n",
           str[0], str[1], str[2], str[3], str[4],
           str[5], str[6], str[7], str[8]);

    for (int i = 0; i < 9; i++) {
      if (strstr(str[i], "north")) {
        North = atof(str[i+1]);
      } else if (strstr(str[i], "south")) {
        South = atof(str[i+1]);
      } else if (strstr(str[i], "east")) {
        East = atof(str[i+1]);
      } else if (strstr(str[i], "west")) {
        West = atof(str[i+1]);
      } else if (strstr(str[i], "pixeledge")) {
        geo_pad_to_edge = 0;
      }
    }

    if (geo_pad_to_edge) {
      // .geo gave pixel centers we use pixel edges, so convert
      double Xpixelsize = (East-West)/(rasterSize.width-1);
      double Ypixelsize = (North-South)/(rasterSize.height-1);

      East  += Xpixelsize * 0.5;
      West  -= Xpixelsize * 0.5;
      North += Ypixelsize * 0.5;
      South -= Ypixelsize * 0.5;
    }

    // .geo files assumes top->bottom, left->right orientation
    return khGeoExtents(khExtents<double>(NSEWOrder, North, South,
                                          East, West),
                        rasterSize,
                        true /* topToBottom */);
  }

  // empty extents signifies failure
  return khGeoExtents();
}

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


#include "khGDALReader.h"
#include <khException.h>


// ****************************************************************************
// ***  khGDALReader
// ****************************************************************************
khGDALReader::khGDALReader(const khGDALDataset &srcDS_, unsigned int numbands)
  : srcDS(srcDS_),
    numBands(numbands),
    gdalDatatype(srcDS->GetRasterBand(1)->GetRasterDataType()),
    topToBottom(srcDS.normalizedGeoExtents().topToBottom()),
    no_data_set(false),
    sanitized_no_data(0),
    paletteSize(0),
    palette(0)
{
  unsigned int gdalRasterCount = srcDS->GetRasterCount();
  if (numBands == 0) {
    throw khException
      (kh::tr("Internal Error: khGDALReader has no bands"));
  }
  if (numBands > gdalRasterCount) {
    throw khException
      (kh::tr("Internal Error: khGDALReader w/ too many bands"));
  }

  if (!StorageEnumFromGDT(gdalDatatype, storage)) {
    throw khException(kh::tr("Unsupported GDAL datatype '%1'")
                      .arg(GDALGetDataTypeName(gdalDatatype)));
  }
  if (!srcDS.geoExtents().leftToRight()) {
    throw khException(kh::tr("Right to left images not supported"));
  }

  // populate list of gdal bands
  gdalBandIndexes.resize(numBands);
  const unsigned int num_orig_bands = srcDS->GetRasterCount();
  if (numBands == 1 || numBands == num_orig_bands) {
    // In both these cases none of the bands need to be dropped.
    for (unsigned int i = 0; i < numBands; ++i) {
      gdalBandIndexes[i] = i+1;
    }
  } else if (num_orig_bands == 4 && numBands == 3) {  // 4 band images
    // Ignore if only one alpha band or 4th band if none is alpha band
    for (unsigned int i = 0, j = 0, num_alpha = 0; i < num_orig_bands;) {
      ++i;
      if (srcDS->GetRasterBand(i)->GetColorInterpretation() == GCI_AlphaBand) {
        if (++num_alpha > 1) {
          throw khException(kh::tr("The image has more than one alpha band."));
        }
      } else if (j < numBands) {
        gdalBandIndexes[j] = i;
        ++j;
      }
    }
  } else {
    throw khException(kh::tr("Band list has wrong number of bands"));
  }


  // get palette information
  GDALRasterBand *band = srcDS->GetRasterBand(1);
  if (band->GetColorInterpretation() == GCI_PaletteIndex) {
    if (numBands > 1) {
      throw khException
        (kh::tr("Unsupported: palette image with multiple bands"));
    }
    GDALColorTable *ctbl = band->GetColorTable();
    if (!ctbl) {
      throw khException(kh::tr("Unable to get color table"));
    }
    paletteSize = (unsigned int)ctbl->GetColorEntryCount();
    palette = ctbl->GetColorEntry(0);
    if (!paletteSize || !palette) {
      throw khException(kh::tr("Unable to get color table"));
    }
  }
}

void khGDALReader::GetNoDataFromSrc(double & no_data, int & nodata_exists) {
  no_data = srcDS->GetRasterBand(1)->GetNoDataValue(&nodata_exists);
}

// ****************************************************************************
// ***  khGDALSimpleReader
// ****************************************************************************
void
khGDALSimpleReader::FetchPixels(const khExtents<std::uint32_t> &srcExtents)
{
  if (srcDS->RasterIO(GF_Read,
                      srcExtents.beginX(), srcExtents.beginY(),
                      srcExtents.width(),  srcExtents.height(),
                      &rawReadBuf[0],
                      srcExtents.width(), srcExtents.height(),
                      gdalDatatype,
                      numBands, &gdalBandIndexes[0],
                      0, 0, 0) != CE_None) {
    throw khException(kh::tr("Failed to read source data"));
  }
}


// ****************************************************************************
// ***  khGDALWarpingReader
// ****************************************************************************
TransformArgGuard&
TransformArgGuard::operator=(void *a)
{
  if (arg && (arg != a)) {
    destroyFunc(arg);
  }
  arg = a;
  return *this;
}
TransformArgGuard::~TransformArgGuard(void)
{
  if (arg) {
    destroyFunc(arg);
  }
}


int
SnapTransformFunc( void *pTransformArg, int bDstToSrc,
                   int nPointCount,
                   double *padfX, double *padfY, double *padfZ,
                   int *panSuccess )
{
  if (!bDstToSrc) {
    notify(NFY_FATAL, "Internal error: Wrong Snap direction");
  }

  SnapArg *snapArg = (SnapArg*)pTransformArg;
  for (int i = 0; i < nPointCount; ++i) {
    padfX[i] *= snapArg->xScale;
    padfY[i] *= snapArg->yScale;
    panSuccess[i] = 1;
  }

  return TRUE;
}

khGDALWarpingReader::khGDALWarpingReader(const khGDALDataset &srcDS,
                                         GDALResampleAlg resampleAlg,
                                         unsigned int numbands,
                                         double transformErrorThreshold,
                                         khTilespace::ProjectionType projType,
                                         const double* noData)
    : khGDALReader(srcDS, numbands),
      snapArg(srcDS.geoExtents().geoTransform(),
              srcDS.alignedGeoExtents().geoTransform()),
      genImgProjArg(GDALDestroyGenImgProjTransformer),
      approxArg(GDALDestroyApproxTransformer)
{
  std::string srcSRS(srcDS->GetProjectionRef());
  std::string dstSRS;
  if (projType == khTilespace::FLAT_PROJECTION) {
    dstSRS = GetWGS84ProjString();
  } else if (projType == khTilespace::MERCATOR_PROJECTION) {
    dstSRS = GetMercatorProjString();
  } else {
    throw khException(kh::tr("Unknown projection type"));
  }

  // ***** Make a dummy output dataset *****
  // We need a dataset in order to get the transformer. The
  // transformer will only use the geoTransform out of the dataset,
  // but you can't pass the geoTransform directly. :-( Lie about the
  // datasets size (1x1) since the we don't need it to know its size
  // and we don't want it to try to allocated enough mem for the whole
  // image (possible 100's of GB)
  GDALDriverH memDriver = GDALGetDriverByName("MEM");
  if (!memDriver) {
    throw khException(kh::tr("Unable to get GDAL MEM driver"));
  }
  khDeleteGuard<GDALDataset>
    dstDS(TransferOwnership(
              (GDALDataset*)
              GDALCreate(memDriver, "unused filename",
                         1 /* numCols */,
                         1 /* numRows */,
                         numBands,
                         gdalDatatype,
                         0 /* unused create options */)));
  if (!dstDS) {
    throw khException(kh::tr("Unable to create MEM dataset"));
  }
  assert(CE_None == dstDS->SetProjection(dstSRS.c_str()));
  dstDS->SetGeoTransform(const_cast<double*>
                         (srcDS.alignedGeoExtents().geoTransform()));
  GDALTransformerFunc transformFunc = 0;
  void* transformArg = 0;
  if (srcDS.needSnapUp()) {
    notify(NFY_NOTICE, "Using snap transform ...");
    transformArg = &snapArg;
    transformFunc = SnapTransformFunc;
  } else if (srcDS.needReproject()) {
    // ***** make a transform w/ the dummy dstDS *****
    genImgProjArg = GDALCreateGenImgProjTransformer
                    (srcDS,                srcSRS.c_str(),
                     (GDALDataset*)dstDS,  dstSRS.c_str(),
                     FALSE,  /* use src GCP if necessary */
                     0.0,    /* GCP error threshold */
                     0       /* maximum order for GCP polynomials */);
    if (!genImgProjArg) {
      throw khException(kh::tr
                        ("Unable to determine reproject transform"));
    }
    transformFunc = GDALGenImgProjTransform;
    transformArg = genImgProjArg;


    // ***** see if an approximate transformation is good enough *****
    if (transformErrorThreshold != 0.0) {
      notify(NFY_NOTICE, "Approximating transform ...");
      approxArg = GDALCreateApproxTransformer(GDALGenImgProjTransform,
                                              genImgProjArg,
                                              transformErrorThreshold);
      transformFunc = GDALApproxTransform;
      transformArg = approxArg;
    } else {
      notify(NFY_NOTICE, "Using exact transform ...");
    }
  } else {
    throw khException
      (kh::tr
       ("Internal Error: khGDALWarpingReader used when not needed"));
  }


  // ***** initialize GDALWarpOptions *****
  // see gdalwarper.h for explaination of options
  GDALWarpOptions *options = GDALCreateWarpOptions();
  khCallGuard<GDALWarpOptions*> optionGuard(GDALDestroyWarpOptions,
                                            options);
  options->eResampleAlg = resampleAlg;
  options->eWorkingDataType = gdalDatatype;
  options->hSrcDS = srcDS;
  // pass our dummy, GDALWarpOperation validates that it exists, even though,
  // we'll never use it by calling, WarpRegionToBuffer */
  options->hDstDS = (GDALDataset*)dstDS;
  options->nBandCount = numBands;
  options->panSrcBands = (int *)CPLMalloc(sizeof(int) * numBands);
  options->panDstBands = (int *)CPLMalloc(sizeof(int) * numBands);
  for (unsigned int i = 0; i < numBands; ++i) {
    options->panSrcBands[i] = gdalBandIndexes[i];
    options->panDstBands[i] = gdalBandIndexes[i];
  }
  options->pfnTransformer = transformFunc;
  options->pTransformerArg = transformArg;

  if (noData != NULL) {
    if (options->padfSrcNoDataReal == NULL) {
      options->padfSrcNoDataReal = (double *)
                    CPLMalloc(sizeof(double) * options->nBandCount);
      options->padfSrcNoDataImag = (double *)
                    CPLMalloc(sizeof(double) * options->nBandCount);
    }
    if (options->padfDstNoDataReal == NULL) {
      options->padfDstNoDataReal = (double *)
                    CPLMalloc(sizeof(double) * options->nBandCount);
      options->padfDstNoDataImag = (double *)
                    CPLMalloc(sizeof(double) * options->nBandCount);
    }
    for (unsigned int i = 0; i < numBands; ++i) {
      options->padfSrcNoDataImag[i] = 0.0;
      options->padfDstNoDataImag[i] = 0.0;
      options->padfSrcNoDataReal[i] = *noData;
      options->padfDstNoDataReal[i] = *noData;
    }
  }

  // ***** finally initialize the GDALWarpOperation object *****
  if (warpOperation.Initialize(options) != CE_None) {
    throw khException(kh::tr("Unable to initialize GDAL warper"));
  }
}


void
khGDALWarpingReader::FetchPixels(const khExtents<std::uint32_t> &srcExtents)
{
  if (warpOperation.WarpRegionToBuffer(srcExtents.beginX(),
                                       srcExtents.beginY(),
                                       srcExtents.width(),
                                       srcExtents.height(),
                                       &rawReadBuf[0],
                                       gdalDatatype) != CE_None) {
    throw khException(kh::tr("Failed to read source data"));
  }
}

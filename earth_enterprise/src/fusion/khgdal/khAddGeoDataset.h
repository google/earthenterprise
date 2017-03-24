/*
 * Copyright 2017 Google Inc.
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

#ifndef __khAddGeoDataset_h
#define __khAddGeoDataset_h

#include <gdal_priv.h>
#include <khGuard.h>
#include <string>

class khAddGeoDataset : public GDALDataset
{
  khDeleteGuard<GDALDriver> driver;
  khDeleteGuard<GDALDataset> srcDS;
  std::string projRef;
  double      geoTransform[6];

 protected:
  virtual CPLErr IBuildOverviews(const char *pszResampling, int nOverviews,
                                 int *panOverviewList, int nListBands,
                                 int *panBandList, GDALProgressFunc pfnProgress,
                                 void *pProgressData) {
    // srcDS->IBuildOverviews is protected. So call the high level
    // routine. It will sanity check the args again but that's OK.
    return srcDS->BuildOverviews(pszResampling, nOverviews, panOverviewList,
                                 nListBands, panBandList, pfnProgress,
                                 pProgressData);
  }

  virtual CPLErr IRasterIO(GDALRWFlag eRWFlag, int nXOff, int nYOff, int nXSize,
                           int nYSize, void *pData, int nBufXSize,
                           int nBufYSize, GDALDataType eBufType, int nBandCount,
                           int *panBandMap, GSpacing nPixelSpace,
                           GSpacing nLineSpace, GSpacing nBandSpace,
                           GDALRasterIOExtraArg* psExtraArg) {
    // srcDS->IRasterIO is protected. So call the high level
    // routine. It will sanity check the args again but that's OK.
    return srcDS->RasterIO(eRWFlag, nXOff, nYOff, nXSize, nYSize, pData,
                           nBufXSize, nBufYSize, eBufType, nBandCount,
                           panBandMap, nPixelSpace, nLineSpace, nBandSpace,
                           psExtraArg);
  }

 public:
  virtual ~khAddGeoDataset() {
    // clean up the pointers to my borrowed bands before the base
    // class tried to delete them
    if (nBands && papoBands) {
      CPLFree(papoBands);
      papoBands = 0;
      nBands = 0;
    }
  }

  virtual void FlushCache(void) {
    srcDS->FlushCache();
  }

  virtual const char *GetProjectionRef(void) {
    return projRef.c_str();
  }

  virtual CPLErr GetGeoTransform( double *trans ) {
    memcpy(trans, geoTransform, sizeof(geoTransform));
    return CE_None;
  }


  khAddGeoDataset(const khTransferGuard<GDALDataset> srcDS_,
                  const std::string &projRef_,
                  const khGeoExtents &geoExtents) :
      driver(TransferOwnership(new GDALDriver())),
      srcDS(srcDS_),
      projRef(projRef_)
  {
    std::string desc("khAddGeo(");
    desc += srcDS->GetDriver()->GetDescription();
    desc += ")";
    driver->SetDescription( desc.c_str() );

    std::string longname("Keyhole Add Geo(");
    longname += srcDS->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME);
    longname += ")";
    driver->SetMetadataItem( GDAL_DMD_LONGNAME, longname.c_str() );

    memcpy(geoTransform, geoExtents.geoTransform(), sizeof(geoTransform));

    poDriver     = driver;
    eAccess      = GA_ReadOnly;
    nRasterXSize = srcDS->GetRasterXSize();
    nRasterYSize = srcDS->GetRasterYSize();
    nBands       = srcDS->GetRasterCount();
    if (nBands) {
      papoBands = (GDALRasterBand **)
                  VSICalloc(sizeof(GDALRasterBand*), nBands);
      for (int i = 0; i < nBands; ++i) {
        papoBands[i] = srcDS->GetRasterBand(i+1);
      }
    }
  }
};


#endif /* __khAddGeoDataset_h */

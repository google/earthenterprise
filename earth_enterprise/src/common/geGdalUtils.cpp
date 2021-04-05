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



#include <vrtdataset.h>

#include "common/geFilePool.h"
#include "common/khTileAddr.h"
#include "common/geGdalUtils.h"

namespace {

// The geGdalUtils object to call GDAL drivers registration one time.
geGdalUtils gegdal_utils;

}  // namespace

geGdalUtils::geGdalUtils() {
  GDALAllRegister();
}

// TODO: refactor - extract reading from source buffer,
// saving to specified format?!
void geGdalUtils::BlendToPng(const std::string& image_data,
                             const std::string* image_alpha,
                             std::string* pimage_blend) {
  assert(!image_data.empty());
  VRTDataset *pvrt_ds;  // Pointer to a virtual dataset
  GDALDatasetH hdata_ds = NULL;  // Define source bands(uncut) GDAL dataset
  GDALDatasetH halpha_ds = NULL;  // Define alpha band(uncut) GDAL dataset
  std::string vsidatafile;
  std::string vsialphafile;
  hdata_ds = geGdalVSI::VsiGdalOpenInternalWrap(&vsidatafile, image_data);
  int src_band_count = GDALGetRasterCount(hdata_ds);
  // Create a new src dataset and update the bands:
  int src_xsize = ImageryQuadnodeResolution;
  int src_ysize = ImageryQuadnodeResolution;
  pvrt_ds = static_cast<VRTDataset *>(VRTCreate(src_xsize, src_ysize));
  // Process the new src bands.
  VRTSourcedRasterBand *pvrt_band;
  GDALRasterBand *psrc_band;
  GDALDataType eBandType;
  for (int i = 0; i < src_band_count; ++i) {
    psrc_band =
        (static_cast<GDALDataset*>(hdata_ds))->GetRasterBand(i + 1);
    eBandType = psrc_band->GetRasterDataType();  // Src data type.
    // Create band i+1:
    pvrt_ds->AddBand(eBandType, NULL);
    pvrt_band =
        static_cast<VRTSourcedRasterBand*>(pvrt_ds->GetRasterBand(i + 1));
    pvrt_band->AddSimpleSource(psrc_band,
                               0,
                               0,
                               src_xsize,
                               src_ysize,
                               0, 0, src_xsize, src_ysize);
  }
  // Now add the alpha band.
  if (image_alpha != NULL) {
    halpha_ds = geGdalVSI::VsiGdalOpenInternalWrap(&vsialphafile, *image_alpha);
    psrc_band =
        (static_cast<GDALDataset*>(halpha_ds))->GetRasterBand(1);
    eBandType = psrc_band->GetRasterDataType();  // Src data type.
    // Create band src_band_count + 1:
    pvrt_ds->AddBand(eBandType, NULL);
    pvrt_band = static_cast<VRTSourcedRasterBand*>
        (pvrt_ds->GetRasterBand(src_band_count + 1));
    pvrt_band->AddSimpleSource(psrc_band,
                               0,
                               0,
                               src_xsize,
                               src_ysize,
                               0, 0, src_xsize, src_ysize);
  }

  GDALDriverH hdriver;
  hdriver = GDALGetDriverByName("PNG");
  // Write the bands to a virtual source file.
  GDALDatasetH hdst_ds;
  std::string vsidstfile;
  char **papszOptions = NULL;
  // PNG ZLEVEL controls the compression, ZLEVEL="6" is the default.
  papszOptions = CSLSetNameValue(papszOptions, "ZLEVEL", "6");
  // bStrict is FALSE, copy may adapt as needed for output format.
  const bool bStrict = FALSE;
  hdst_ds = geGdalVSI::VsiGdalCreateCopyWrap(hdriver, &vsidstfile,
                                             static_cast<GDALDatasetH>(pvrt_ds),
                                             bStrict, papszOptions, NULL, NULL);
  GDALClose(hdst_ds);

  // VSIGetMemFileBuffer returns a pointer to the buffer underlying the vsi
  // file. If 3rd arg=TRUE the vsi file object will be deleted.
  GByte *membuf;
  vsi_l_offset buffer_length;
  membuf =  VSIGetMemFileBuffer(vsidstfile.c_str(), &buffer_length, TRUE);
  CSLDestroy(papszOptions);
  *pimage_blend = std::string(reinterpret_cast<char*>(membuf), buffer_length);

  CPLFree(membuf);
  VSIUnlink(vsidstfile.c_str());
  GDALClose(static_cast<GDALDatasetH>(pvrt_ds));

  // Close/Free source image/alpha datasets.
  GDALClose(hdata_ds);
  VSIUnlink(vsidatafile.c_str());
  if (halpha_ds != NULL) {
    GDALClose(halpha_ds);
    VSIUnlink(vsialphafile.c_str());
  }
}

void geGdalUtils::ConvertToJpeg(const std::string& image_data,
                                std::string* image_out) {
  assert(!image_data.empty());
  VRTDataset *pvrt_ds;  // Pointer to a virtual dataset
  GDALDatasetH hdata_ds = NULL;  // Define source bands(uncut) GDAL dataset
  std::string vsidatafile;
  hdata_ds = geGdalVSI::VsiGdalOpenInternalWrap(&vsidatafile, image_data);
  int src_band_count = GDALGetRasterCount(hdata_ds);
  // Create a new src dataset and update the bands:
  int src_xsize = ImageryQuadnodeResolution;
  int src_ysize = ImageryQuadnodeResolution;
  pvrt_ds = static_cast<VRTDataset *>(VRTCreate(src_xsize, src_ysize));
  // Process the new src bands.
  VRTSourcedRasterBand *pvrt_band;
  GDALRasterBand *psrc_band;
  GDALDataType eBandType;
  for (int i = 0; i < src_band_count; ++i) {
    psrc_band =
        (static_cast<GDALDataset*>(hdata_ds))->GetRasterBand(i + 1);
    eBandType = psrc_band->GetRasterDataType();  // Src data type.
    // Create band i+1:
    pvrt_ds->AddBand(eBandType, NULL);
    pvrt_band =
        static_cast<VRTSourcedRasterBand*>(pvrt_ds->GetRasterBand(i + 1));
    pvrt_band->AddSimpleSource(psrc_band,
                               0,
                               0,
                               src_xsize,
                               src_ysize,
                               0, 0, src_xsize, src_ysize);
  }

  GDALDriverH hdriver;
  hdriver = GDALGetDriverByName("JPEG");
  // Write the bands to a virtual source file.
  GDALDatasetH hdst_ds;
  char **papszOptions = NULL;
  std::string vsidstfile;
  // bStrict is FALSE, copy may adapt as needed for output format.
  const bool bStrict = FALSE;
  hdst_ds = geGdalVSI::VsiGdalCreateCopyWrap(hdriver, &vsidstfile,
                                             static_cast<GDALDatasetH>(pvrt_ds),
                                             bStrict, papszOptions, NULL, NULL);
  GDALClose(hdst_ds);

  // VSIGetMemFileBuffer returns a pointer to the buffer underlying the vsi
  // file. If 3rd arg=TRUE the vsi file object will be deleted.
  GByte *membuf;
  vsi_l_offset buffer_length;
  membuf =  VSIGetMemFileBuffer(vsidstfile.c_str(), &buffer_length, TRUE);
  CSLDestroy(papszOptions);
  *image_out = std::string(reinterpret_cast<char*>(membuf), buffer_length);

  CPLFree(membuf);
  VSIUnlink(vsidstfile.c_str());
  GDALClose(static_cast<GDALDatasetH>(pvrt_ds));

  // Close/Free source image dataset.
  GDALClose(hdata_ds);
  VSIUnlink(vsidatafile.c_str());
}

namespace {

const char * const kGdalAllowedDrivers[] = {"JPEG", "GTIFF", "PNG", NULL};

const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

char GetRandChar() {
  return alphanum[rand() % 62];
}

inline std::string GenRandomString(int n) {
  char randchar[n];
  std::generate(randchar, randchar+n, &GetRandChar);
  return std::string(randchar, n);
}

inline std::string UniqueVSIFilename() {
  int file_exists = 0;
  VSIStatBufL filestats;
  std::string vsidatafile;
  while (file_exists == 0) {
    vsidatafile = "/vsimem/motf_tmp" + GenRandomString(6);
    // VSIStatL returns 0 if the file exists, returns -1 if it does not exist.
    file_exists = VSIStatL(vsidatafile.c_str(), &filestats);
  }
  return vsidatafile;
}

}  // namespace

void geGdalVSI::InitVSIRandomSeed() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  srand((time_t)ts.tv_nsec);
}


GDALDataset* geGdalVSI::VsiGdalCreateWrap(GDALDriverH hdriver,
                                          std::string* const vsifile,
                                          int nx, int ny, int nbands,
                                          GDALDataType bandtype,
                                          char **papszoptions) {
  GDALDatasetH hvsi_ds;
  khMutex &mutex = GetMutex();
  khLockGuard lock(mutex);
  *vsifile = UniqueVSIFilename();
  hvsi_ds = GDALCreate(hdriver, (*vsifile).c_str(), nx, ny, nbands,
                       bandtype, papszoptions);
  return static_cast<GDALDataset*>(hvsi_ds);
}

GDALDataset* geGdalVSI::VsiGdalCreateCopyWrap(GDALDriverH hdriver,
                                              std::string* const vsifile,
                                              GDALDatasetH hdataset,
                                              int bstrict,
                                              char **papszoptions,
                                              GDALProgressFunc progressfunc,
                                              void *progressdata) {
  GDALDatasetH hvsi_ds;
  khMutex &mutex = GetMutex();
  khLockGuard lock(mutex);
  *vsifile = UniqueVSIFilename();
  hvsi_ds = GDALCreateCopy(hdriver, (*vsifile).c_str(), hdataset, bstrict,
                           papszoptions, progressfunc, progressdata);
  return static_cast<GDALDataset*>(hvsi_ds);
}

GDALDataset* geGdalVSI::VsiGdalOpenInternalWrap(
    std::string* const vsifile,
    const std::string& image_alpha) {
  GDALDatasetH hvsi_ds;
  khMutex &mutex = GetMutex();
  khLockGuard lock(mutex);
  *vsifile = UniqueVSIFilename();
  VSIFCloseL(VSIFileFromMemBuffer((*vsifile).c_str(),
                                  reinterpret_cast<GByte*>
                                  (const_cast<char*>(&(image_alpha[0]))),
                                   image_alpha.size(), FALSE));
  hvsi_ds = GDALOpenEx((*vsifile).c_str(), GA_ReadOnly | GDAL_OF_VERBOSE_ERROR,
                           kGdalAllowedDrivers, NULL, NULL);
  return static_cast<GDALDataset*>(hvsi_ds);
}

khMutex& geGdalVSI::GetMutex() {
  static khMutex mutex;
  return mutex;
}

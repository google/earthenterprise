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


#include "iconutil.h"
#include <khException.h>
#include <khFileUtils.h>
#include <khgdal/khgdal.h>
#include <cpl_string.h>
#include <vrtdataset.h>

namespace iconutil {

void Extract(const std::string &src, ExtractType type,
             const std::string &dest)
{
  // Make sure GDAL is initialized - safe to call multiple times.
  khGDALInit();

  if (!khEnsureParentDir(dest)) {
    throw khException(kh::tr("Unable to create parent directory"));
  }

  GDALDataset *srcDataset = (GDALDataset*)GDALOpen(src.c_str(), GA_ReadOnly );
  if (!srcDataset) {
    throw khException(kh::tr("Unable to open %1").arg(src.c_str()));
  }


  int srcXSize = srcDataset->GetRasterXSize();


  bool isPiType = ((srcDataset->GetRasterCount() == 1) &&
                   (srcDataset->GetRasterBand( 1 )->GetColorInterpretation() ==
                    GCI_PaletteIndex ));

  // Find output driver.
  GDALDriver *pngDriver = GetGDALDriverManager()->GetDriverByName( "PNG" );
  if (pngDriver == NULL) {
    throw khException(kh::tr("INTERNAL ERROR: Unable to find png driver"));
  }

  // Create a virtual dataset to represent the desired output image.
  int srcY = 0;
  int outWidth = 0;
  int outHeight = 0;
  switch (type) {
    case Legend:
      srcY = 0;
      outWidth = outHeight = 16;
      break;
    case Normal:
      srcY = srcXSize*2;
      outWidth = srcXSize;
      outHeight = srcXSize;
      break;
    case Highlight:
      srcY = srcXSize;
      outWidth = srcXSize;
      outHeight = srcXSize;
      break;
    case NormalHighlight:
      srcY = srcXSize;
      outWidth = srcXSize;
      outHeight = srcXSize*2;
      break;
  }

  VRTDataset *vds = new VRTDataset( outWidth, outHeight );

  // copy all bands from the source
  for ( int b = 1; b <= srcDataset->GetRasterCount(); ++b ) {
    vds->AddBand( GDT_Byte, NULL );
    VRTSourcedRasterBand *vrtBand =
      (VRTSourcedRasterBand*)vds->GetRasterBand( b );

    GDALRasterBand *srcBand = srcDataset->GetRasterBand( b );
    vrtBand->AddSimpleSource( srcBand,
                              0, srcY, outWidth, outHeight,
                              0, 0, outWidth, outHeight );
    if ( isPiType ) {
      vrtBand->SetColorInterpretation( srcBand->GetColorInterpretation() );
      vrtBand->SetColorTable( srcBand->GetColorTable() );
    }
  }


  // Write out all bands at once.
  GDALDataset *destDataset = pngDriver->CreateCopy( dest.c_str(), vds, false,
                                                    NULL, NULL, NULL );

  // Write failures are not detectable with this API.
  delete destDataset;

  delete vds;
  delete srcDataset;
}




} // namespace iconutil

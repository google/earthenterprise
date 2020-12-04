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


#include "khGDALImage.h"
#include <khException.h>
#include <khFileUtils.h>
#include <khTileAddr.h>
#include "khgdal.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <khstl.h>


// ****************************************************************************
// ***  khGDALImageImpl
// ****************************************************************************
khGDALImageImpl::khGDALImageImpl(const std::string &filename_) :
    filename(filename_),
    dataset()
{
  khGDALInit();  // safe to call multiple times

  // get the dataset
  dataset = TransferOwnership(
      (GDALDataset *) GDALOpen(filename.c_str(), GA_ReadOnly));
  if (!dataset) {
    throw khException(kh::tr("Unable to open %1").arg(filename.c_str()));
  }

  khSize<std::uint32_t> rasterSize(dataset->GetRasterXSize(),
                            dataset->GetRasterYSize());
}


// ****************************************************************************
// ***  khGDALImage
// ****************************************************************************
khGDALImage::khGDALImage(const std::string &filename)
    : impl(khRefGuardFromNew(new khGDALImageImpl(filename)))
{
}

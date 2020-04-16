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

#ifndef __khGDALImage_h
#define __khGDALImage_h

#include <cstdint>
#include <string>
#include <gdal_priv.h>
#include <khGuard.h>
#include <khRefCounter.h>
#include <notify.h>

// ****************************************************************************
// ***  khGDALImage
// ***
// ***  Smart pointer around GDALDataset
// ***    NOTE: This is a SIMPLIFIED interface used to load images w/o
// ***    geospatial information. If you want geospatial information you should
// ***    use khGDALDataset
// ***    - constructor & extended keyhole routines throw exceptions
// ***    - can be used almost like a GDALDataset*
// ***      - normal GDALDataset routines available through '->'
// ***      - extended keyhole routines available through '.'
// ***    - remembers filename :-)
// ****************************************************************************

// implementation class - never used directly
class khGDALImageImpl : public khRefCounter
{
  friend class khGDALImage;
 private:
  std::string filename;
  khDeleteGuard<GDALDataset> dataset;

  inline khSize<std::uint32_t> rasterSize(void) const {
    return khSize<std::uint32_t>(dataset->GetRasterXSize(),
                          dataset->GetRasterYSize());
  }

  // ALL THESE WILL THROW ON ERROR
  khGDALImageImpl(const std::string &filename_);
};


class khGDALImage
{
 private:
  khRefGuard<khGDALImageImpl> impl;

 public:
  void release(void) { impl = khRefGuard<khGDALImageImpl>(); }

  inline GDALDataset* operator->(void) const { return impl->dataset; }
  inline operator GDALDataset*(void) const {
    return impl ? (GDALDataset*)impl->dataset : 0;
  }

  // extended keyhole routines
  inline const std::string& filename(void) const { return impl->filename; }
  inline khSize<std::uint32_t> rasterSize(void) const {
    return impl->rasterSize();
  }

  inline khGDALImage(void) : impl() { }

  // WILL THROW ON ERROR
  khGDALImage(const std::string &filename);
};


#endif /* __khGDALImage_h */

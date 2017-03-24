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

#ifndef __KHistogram_h__
#define __KHistogram_h__

//
// khistogram.h -- the Keyhole histogram class
//      (intimate with the GDAL library)
//

#include <stdio.h>
#include "ktable.h"

#ifndef KHISTOGRAM_NONE
#define KHISTOGRAM_GDAL
#endif

#ifdef  KHISTOGRAM_GDAL
#include "gdal_priv.h"
#endif

class KHistogramBand : public KTable
{
 public:
  KHistogramBand(int bins=65536);
  ~KHistogramBand();

#ifdef  KHISTOGRAM_GDAL
  int histogramGDAL(GDALRasterBand *band);
  int histogram(GDALRasterBand *band);
  int apply(GDALRasterBand *band);
#endif

 private:

};

class KHistogram
{
 public:
  KHistogram(int bands=3, int bins=65536);
  ~KHistogram();

  int &size() {return _size;}

  void reset();

  void setBands(int bands) {_bands = bands;}
  int getBands(void) {return _bands;}
  int &bands(void) {return _bands;}

  KHistogramBand &band(int band) {return *_band[band];}

  int read(FILE *fp, int format=3);
  int read(const char *filename, int format=3);

  int write(FILE *fp, int format=3);
  int write(const char *filename, int format=3);

  int accumulate(KHistogram &h);

#ifdef  KHISTOGRAM_GDAL
  int histogramGDAL(GDALDataset *dataset);
  int histogramGDAL(const char *filename);

  int histogram(GDALDataset *dataset);
  int histogram(const char *filename);

  int apply(GDALDataset *dataset);
  int apply(const char *filename);

  int transform(GDALDataset *src, const char *dstName);
  int transform(const char *srcName, const char *dstName);
#endif

  void statistics();

 private:
  int _bands; // 1 for gray or 3 for RGB
  KHistogramBand **_band;
  int _size; // allocated size
};

#endif

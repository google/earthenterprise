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

#include "khistogram.h"

static int GetHistogram8(GDALRasterBand *poBand, int *panHistogram) {
  int nXBlocks, nYBlocks, nXBlockSize, nYBlockSize;
  int iXBlock, iYBlock;

  memset(panHistogram, 0, sizeof(int) * 256);

  CPLAssert(poBand->GetRasterDataType() == GDT_Byte);

  poBand->GetBlockSize(&nXBlockSize, &nYBlockSize);

  nXBlocks = (poBand->GetXSize() + nXBlockSize - 1) / nXBlockSize;
  nYBlocks = (poBand->GetYSize() + nYBlockSize - 1) / nYBlockSize;

  unsigned char *pabyData =
      (unsigned char *)CPLMalloc(nXBlockSize * nYBlockSize);

  for (iYBlock = 0; iYBlock < nYBlocks; iYBlock++) {
    for (iXBlock = 0; iXBlock < nXBlocks; iXBlock++) {
      int nXValid, nYValid;

      int error = poBand->ReadBlock(iXBlock, iYBlock, pabyData);
      if (error != CE_None) {
        printf( "error reading / writing raster block.\n");
        continue;
      }
      // histogram the portion of the block that is valid
      // for partial edge blocks.
      if (iXBlock == nXBlocks - 1)
        nXValid = poBand->GetXSize() - iXBlock * nXBlockSize;
      else
        nXValid = nXBlockSize;

      if (iYBlock == nYBlocks - 1)
        nYValid = poBand->GetYSize() - iYBlock * nYBlockSize;
      else
        nYValid = nYBlockSize;

      // Collect the histogram counts.
      for (int iY = 0; iY < nYValid; iY++) {
        for (int iX = 0; iX < nXValid; iX++) {
          panHistogram[pabyData[iX + iY * nXBlockSize]] += 1;
        }
      }
    }
  }

  CPLFree(pabyData);

  return 0;
}

static int GetHistogram16(GDALRasterBand *poBand, int *panHistogram) {
  int nXBlocks, nYBlocks, nXBlockSize, nYBlockSize;
  int iXBlock, iYBlock;

  memset(panHistogram, 0, sizeof(int) * 65536);

  CPLAssert(poBand->GetRasterDataType() == GDT_UInt16 ||
            poBand->GetRasterDataType() == GDT_Int16);

  poBand->GetBlockSize(&nXBlockSize, &nYBlockSize);

  nXBlocks = (poBand->GetXSize() + nXBlockSize - 1) / nXBlockSize;
  nYBlocks = (poBand->GetYSize() + nYBlockSize - 1) / nYBlockSize;

  unsigned short *pabyData =
      (unsigned short *)CPLMalloc(2 * nXBlockSize * nYBlockSize);

  for (iYBlock = 0; iYBlock < nYBlocks; iYBlock++) {
    for (iXBlock = 0; iXBlock < nXBlocks; iXBlock++) {
      int nXValid, nYValid;

      int error = poBand->ReadBlock(iXBlock, iYBlock, pabyData);
      if (error != CE_None) {
        printf( " error reading / writing raster block.\n ");
        continue;
      }

      // histogram the portion of the block that is valid
      // for partial edge blocks.
      if (iXBlock == nXBlocks - 1)
        nXValid = poBand->GetXSize() - iXBlock * nXBlockSize;
      else
        nXValid = nXBlockSize;

      if (iYBlock == nYBlocks - 1)
        nYValid = poBand->GetYSize() - iYBlock * nYBlockSize;
      else
        nYValid = nYBlockSize;

      // Collect the histogram counts.
      for (int iY = 0; iY < nYValid; iY++) {
        for (int iX = 0; iX < nXValid; iX++) {
          panHistogram[pabyData[iX + iY * nXBlockSize]] += 1;
        }
      }
    }
  }

  CPLFree(pabyData);

  return 0;
}

static int ApplyLUT8(GDALRasterBand *poBand, int *LUT) {
  int nXBlocks, nYBlocks, nXBlockSize, nYBlockSize;
  int iXBlock, iYBlock;

  CPLAssert(poBand->GetRasterDataType() == GDT_Byte);

  poBand->GetBlockSize(&nXBlockSize, &nYBlockSize);

  nXBlocks = (poBand->GetXSize() + nXBlockSize - 1) / nXBlockSize;
  nYBlocks = (poBand->GetYSize() + nYBlockSize - 1) / nYBlockSize;

  unsigned char *pixel = (unsigned char *)CPLMalloc(nXBlockSize * nYBlockSize);

  for (iYBlock = 0; iYBlock < nYBlocks; iYBlock++) {
    for (iXBlock = 0; iXBlock < nXBlocks; iXBlock++) {
      int nXValid, nYValid;

      int error = poBand->ReadBlock(iXBlock, iYBlock, pixel);
      if (error != CE_None) {
        printf( "error reading / writing raster block.\n");
        continue;
      }

      // histogram the portion of the block that is valid
      // for partial edge blocks.
      if (iXBlock == nXBlocks - 1)
        nXValid = poBand->GetXSize() - iXBlock * nXBlockSize;
      else
        nXValid = nXBlockSize;

      if (iYBlock == nYBlocks - 1)
        nYValid = poBand->GetYSize() - iYBlock * nYBlockSize;
      else
        nYValid = nYBlockSize;

      // apply the transformation
      for (int iY = 0; iY < nYValid; iY++) {
        for (int iX = 0; iX < nXValid; iX++) {
          pixel[iX + iY * nXBlockSize] =
              (unsigned char)LUT[pixel[iX + iY * nXBlockSize]];
        }
      }

      error = poBand->WriteBlock(iXBlock, iYBlock, pixel);
      printf(" error reading / writing block error code : %d \n ", error);
    }
  }

  CPLFree(pixel);

  return 0;
}

static int ApplyLUT16(GDALRasterBand *poBand, int *LUT) {
  int nXBlocks, nYBlocks, nXBlockSize, nYBlockSize;
  int iXBlock, iYBlock;

  CPLAssert(poBand->GetRasterDataType() == GDT_UInt16 ||
            poBand->GetRasterDataType() == GDT_Int16);

  poBand->GetBlockSize(&nXBlockSize, &nYBlockSize);

  nXBlocks = (poBand->GetXSize() + nXBlockSize - 1) / nXBlockSize;
  nYBlocks = (poBand->GetYSize() + nYBlockSize - 1) / nYBlockSize;

  unsigned short *pixel =
      (unsigned short *)CPLMalloc(2 * nXBlockSize * nYBlockSize);

  for (iYBlock = 0; iYBlock < nYBlocks; iYBlock++) {
    for (iXBlock = 0; iXBlock < nXBlocks; iXBlock++) {
      int nXValid, nYValid;

      int error = poBand->ReadBlock(iXBlock, iYBlock, pixel);
      if (error != CE_None) {
        printf( "error reading / writing raster block.\n");
        continue;
      }

      // histogram the portion of the block that is valid
      // for partial edge blocks.
      if (iXBlock == nXBlocks - 1)
        nXValid = poBand->GetXSize() - iXBlock * nXBlockSize;
      else
        nXValid = nXBlockSize;

      if (iYBlock == nYBlocks - 1)
        nYValid = poBand->GetYSize() - iYBlock * nYBlockSize;
      else
        nYValid = nYBlockSize;

      // Collect the histogram counts.
      for (int iY = 0; iY < nYValid; iY++) {
        for (int iX = 0; iX < nXValid; iX++) {
          pixel[iX + iY * nXBlockSize] =
              (unsigned short)LUT[pixel[iX + iY * nXBlockSize]];
        }
      }

      error = poBand->WriteBlock(iXBlock, iYBlock, pixel);
      printf("error reading / writing block error code : %d \n", error);
    }
  }

  CPLFree(pixel);

  return 0;
}

//
//      KHistogramBand
//

// constructors not needed with static bin array
KHistogramBand::KHistogramBand(int bins) : KTable(bins) {}

KHistogramBand::~KHistogramBand() {}

#ifdef KHISTOGRAM_GDAL
int KHistogramBand::histogramGDAL(GDALRasterBand *band) {
  int numBins;
  if (band->GetRasterDataType() == GDT_Byte)
    numBins = 256;
  else if (band->GetRasterDataType() == GDT_UInt16 ||
           band->GetRasterDataType() == GDT_Int16)
    numBins = 65536;
  else
    return 1;

  // set number of bins
  if (numBins > size()) return 2;
  bins() = numBins;

  // positioning centers bin windows at truncation points
  double minBinValue = -0.5;
  double maxBinValue = numBins - 0.5;  //"numBins - 1 + 0.5"

  if (band->GetHistogram(minBinValue, maxBinValue, numBins,
                         (GUIntBig *)getBinPointer(),  // this is the actual
                                                       // int* data array in the
                                                       // table class
                         FALSE, FALSE, GDALDummyProgress, NULL) != CE_None)
    return 2;

  return 0;
}
#endif

#ifdef KHISTOGRAM_GDAL
int KHistogramBand::histogram(GDALRasterBand *band) {
  int numBins;
  if (band->GetRasterDataType() == GDT_Byte)
    numBins = 256;
  else if (band->GetRasterDataType() == GDT_UInt16 ||
           band->GetRasterDataType() == GDT_Int16)
    numBins = 65536;
  else
    return 1;

  // set number of bins
  if (numBins > size()) return 2;
  bins() = numBins;

  if (numBins == 256) {
    if (GetHistogram8(band, getBinPointer()) != 0) return 3;
  } else if (numBins == 65536) {
    if (GetHistogram16(band, getBinPointer()) != 0) return 4;
  }

  return 0;
}

int KHistogramBand::apply(GDALRasterBand *band) {
  int numBins;
  if (band->GetRasterDataType() == GDT_Byte)
    numBins = 256;
  else if (band->GetRasterDataType() == GDT_UInt16 ||
           band->GetRasterDataType() == GDT_Int16)
    numBins = 65536;
  else
    return 1;

  // set number of bins
  if (numBins > size()) return 2;

  if (numBins == 256) {
    if (ApplyLUT8(band, getBinPointer()) != 0) return 3;
  } else if (numBins == 65536) {
    if (ApplyLUT16(band, getBinPointer()) != 0) return 4;
  }

  return 0;
}
#endif

//
// KHistogram
//

KHistogram::KHistogram(int bands, int bins) {
  _size = bands;

  _band = (KHistogramBand **)calloc(sizeof(KHistogramBand *), _size);
  for (int i = 0; i < _size; i++) _band[i] = new KHistogramBand(bins);

  _bands = _size;
}

KHistogram::~KHistogram() {
  for (int i = 0; i < _size; i++) delete _band[i];
}

void KHistogram::reset() {
  // reset number of active bands
  _bands = 0;

  // reset band objects
  for (int i = 0; i < _size; i++) band(i).reset();
}

int KHistogram::read(FILE *fp, int format) {
  reset();

  if (fp == NULL) return 1;

  if (format == 0 || format == 1 || format == 2 || format == 3) {
    // get number of bands
    int b = 0;
    if (fscanf(fp, "%d", &b) != 1) return 2;
    if (b < 1 || b > _size) return 3;
    _bands = b;

    // read the contents of each band
    for (b = 0; b < _bands; b++) {
      if (band(b).read(fp, format) != 0) {
        reset();
        return 4;
      }
    }
  } else if (format == 4) {
    // aubin format

    // get number of bands
    int b = 0;
    if (fscanf(fp, "%d", &b) != 1) return 2;
    if (b < 1 || b > _size) return 3;
    _bands = b;

    // get number of bins
    int n = 0;
    if (fscanf(fp, "%d", &n) != 1) return 4;
    if (n < 1) return 5;
    for (b = 0; b < _bands; b++)
      if (n > band(b).size()) return 6;

    // set the number of bins in each band
    for (b = 0; b < _bands; b++) band(b).bins() = n;

    // read the contents of each band
    for (b = 0; b < _bands; b++) {
      if (band(b).read(fp, format) != 0) {
        reset();
        return 7;
      }
    }
  } else
    return 7;

  return 0;
}

int KHistogram::read(const char *filename, int format) {
  FILE *fp = NULL;

  // open file
  if (filename == NULL || *filename == '\0') return 1;
  if ((fp = fopen(filename, "r")) == NULL) return 2;

  // read data
  KHistogram::read(fp, format);

  // close file
  fclose(fp);

  return 0;
}

int KHistogram::write(FILE *fp, int format) {
  if (fp == NULL) return 1;
  if (_bands == 0) return 2;

  // make sure that the bands have a valid number of bins
  int b;
  for (b = 0; b < _bands; b++)
    if (band(b).bins() < 1) return 3;

  if (format == 0 || format == 1 || format == 2 || format == 3) {
    // describe number of bands
    fprintf(fp, "%d\n", _bands);

    // write the contents of each band
    for (b = 0; b < _bands; b++) band(b).write(fp, format);
  } else if (format == 4) {
    // aubin format

    // make sure that the bands have the same number of bins
    for (b = 1; b < _bands; b++)
      if (band(b - 1).bins() != band(b).bins()) return 4;

    // describe number of bands     and uniform number of bins in each
    fprintf(fp, "%d %d\n", _bands, band(0).bins());

    // write the contents of each band
    for (b = 0; b < _bands; b++) band(b).write(fp, format);
  } else
    return 5;

  return 0;
}

int KHistogram::write(const char *filename, int format) {
  FILE *fp = NULL;

  // open file
  if (filename == NULL || *filename == '\0') return 1;
  if ((fp = fopen(filename, "w")) == NULL)  // "write" rather than "append"
    return 2;

  // write data
  KHistogram::write(fp, format);

  // close file
  fclose(fp);

  return 0;
}

int KHistogram::accumulate(KHistogram &h) {
  if (_bands != h.bands()) {
    return 1;
  }

  for (int i = 0; i < size(); i++) band(i).accumulate(h.band(i));

  return 0;
}

#ifdef KHISTOGRAM_GDAL
int KHistogram::histogramGDAL(GDALDataset *dataset) {
  int numBands = dataset->GetRasterCount();
  if (numBands < 1 || numBands > size()) return 1;
  size() = numBands;

  for (int i = 0; i < size(); i++)
    if (band(i).histogramGDAL(dataset->GetRasterBand(i + 1)) != 0) return 2;

  return 0;
}
#endif

#ifdef KHISTOGRAM_GDAL
int KHistogram::histogramGDAL(const char *filename) {
  // open file using GDAL
  GDALDataset *dataset = (GDALDataset *)GDALOpen(filename, GA_ReadOnly);
  if (dataset == NULL) return 1;

  // compute histogram of file contents
  histogramGDAL(dataset);

  // close file
  delete dataset;

  return 0;
}
#endif

#ifdef KHISTOGRAM_GDAL
int KHistogram::histogram(GDALDataset *dataset) {
  int numBands = dataset->GetRasterCount();
  if (numBands < 1) return 1;
  if (numBands <= size()) {
    size() = numBands;
  } else {
    // leave size alone & just process those the number asked for
  }

  for (int i = 0; i < size(); i++)
    if (band(i).histogram(dataset->GetRasterBand(i + 1)) != 0) return 2;

  return 0;
}
#endif

#ifdef KHISTOGRAM_GDAL
int KHistogram::histogram(const char *filename) {
  // open file using GDAL
  GDALDataset *dataset = (GDALDataset *)GDALOpen(filename, GA_ReadOnly);
  if (dataset == NULL) return 1;

  // compute histogram of file contents
  int ret = histogram(dataset);

  // close file
  delete dataset;

  return ret;
}
#endif

void KHistogram::statistics() {
  for (int i = 0; i < bands(); i++) band(i).statistics();
}

#ifdef KHISTOGRAM_GDAL
int KHistogram::apply(GDALDataset *dataset) {
  int numBands = dataset->GetRasterCount();
  if (numBands < 1 || numBands > size())  // bands in dataset
    return 1;
  if (numBands > size())  // bands in histogram/LUT
    numBands = size();

  for (int i = 0; i < numBands; i++)
    if (band(i).apply(dataset->GetRasterBand(i + 1)) != 0) return 2;

  return 0;
}
#endif

#ifdef KHISTOGRAM_GDAL
int KHistogram::apply(const char *filename) {
  // open file using GDAL
  GDALDataset *dataset = (GDALDataset *)GDALOpen(filename, GA_Update);
  if (dataset == NULL) return 1;

  // apply transformation to file contents
  apply(dataset);

  // close file
  delete dataset;

  return 0;
}
#endif

int KHistogram::transform(GDALDataset *src, const char *dstName) {
  // prepare "how to make my copy" options to the file driver
  char **options = NULL;
  // optionsForTIFF = CSLSetNameValue(optionsForTIFF, "TILED", "YES");
  // optionsForTIFF = CSLSetNameValue(optionsForTIFF, "BLOCKXSIZE", "512"); //
  // integer multiple of 16
  // optionsForTIFF = CSLSetNameValue(optionsForTIFF, "BLOCKYSIZE", "512"); //
  // integer multiple of 16
  // optionsForTIFF = CSLSetNameValue(optionsForTIFF, "INTERLEAVE", "BAND"); //
  // PIXEL or BAND
  // optionsForTIFF = CSLSetNameValue(optionsForTIFF, "COMPRESS", "ZIP"); //
  // JPEG, LZW, PACKBITS, DEFLATE, ZIP

  // create copy of input image as output image
  GDALDriver *driver = src->GetDriver();
  GDALDataset *dst = (GDALDataset *)driver->CreateCopy(dstName, src, false,
                                                       options, NULL, NULL);
  // CSLDestroy(options);

  // apply transformation table to output image data
  apply(dst);

  // close output file
  GDALClose(dst);

  // normal termination
  return 0;
}

int KHistogram::transform(const char *srcName, const char *dstName) {
  // open input image
  GDALDataset *src = (GDALDataset *)GDALOpen(srcName, GA_ReadOnly);
  if (src == NULL) return 1;

  if (transform(src, dstName) != 0) return 2;

  GDALClose(src);

  return 0;
}

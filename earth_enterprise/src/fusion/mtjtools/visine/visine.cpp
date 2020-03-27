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

#include "stdio.h"

// GDAL includes
#include "cpl_string.h"
#include "gdal_priv.h"

// KHistogram class
#include "khistogram.h"

void usage() {
  printf("usage: visine [-v][-m matrix][-i input][-o output]\n");
  printf("  -v  verbose (DEFAULT = OFF)\n");
  printf("  -a  use Amin transform (DEFAULT)\n");
  printf("  -j  use Jason transform\n");
  printf("  -m  matrix: 3x3 transform\n");
  printf("       r'=r*m00 + g*m01 + b*m02\n");
  printf("       g'=r*m10 + g*m11 + b*m12\n");
  printf("       b'=r*m20 + g*m21 + b*m22\n");
  printf("  -i  name of input image file\n");
  printf("  -o  name of output image file\n");

  exit(1);
}

int transformGDAL(GDALDataset *dataset, double matrix[3][3]);

//
// Apply LUT to an image
//
int main(int argc, char *argv[]) {
  int verbose = 0;
  char input[256] = {0};
  char output[256] = {0};
  double matrix[3][3] = {{0.0}, {0.0}, {0.0}};

  matrix[0][0] = 0.0;
  matrix[0][1] = 1.0;
  matrix[0][2] = 0.0;
  matrix[1][0] = 0.25;
  matrix[1][1] = 0.0;
  matrix[1][2] = 0.75;
  matrix[2][0] = -0.25;
  matrix[2][1] = 0.0;
  matrix[2][2] = 0.75;

  // parse command line 'option arguments'
  int cursor;
  for (cursor = 1; cursor < argc && argv[cursor][0] == '-'; cursor++) {
    if (strncmp(argv[cursor], "-v", 2) == 0) {
      verbose = 1;
    } else if (strncmp(argv[cursor], "-i", 2) == 0) {
      if (sscanf(argv[cursor], "-i%255s", input) != 1)
        sscanf(argv[++cursor], "%255s", input);
    } else if (strncmp(argv[cursor], "-a", 2) == 0) {
      // Subject: Cir to natural colors
      // Came across a method for transforming color infra-red imagery to
      // pseudo-natural colors.
      // This method has been used by NASA JPL on various multi-spectral
      // imagery.
      //
      // The transformations (surprisingly linear!) are:
      //
      // R_o  =  G_i
      // G_o  =  R_i * 0.25   +   B_i * 0.75
      // B_o  =  B_i * 0.75   -    R_i * 0.25
      //
      // you can look for more information at:
      //     http://www-dial.jpl.nasa.gov/~steven/cir2nat/

      matrix[0][0] = 0.0;
      matrix[0][1] = 1.0;
      matrix[0][2] = 0.0;
      matrix[1][0] = 0.25;
      matrix[1][1] = 0.0;
      matrix[1][2] = 0.75;
      matrix[2][0] = -0.25;
      matrix[2][1] = 0.0;
      matrix[2][2] = 0.75;
    } else if (strncmp(argv[cursor], "-j", 2) == 0) {
      matrix[0][0] = 1.0;
      matrix[0][1] = 0.0;
      matrix[0][2] = 0.0;
      matrix[1][0] = 0.0;
      matrix[1][1] = 1.0;
      matrix[1][2] = 0.0;
      matrix[2][0] = 0.0;
      matrix[2][1] = 0.0;
      matrix[2][2] = 1.0;
    } else if (strncmp(argv[cursor], "-m", 2) == 0) {
      for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
          if (sscanf(argv[cursor], "-m%lf", &matrix[row][col]) != 1 &&
              sscanf(argv[cursor], "%lf", &matrix[row][col]) != 1)
            sscanf(argv[++cursor], "%lf", &matrix[row][col]);
          ++cursor;
        }
      }
      --cursor;

    } else if (strncmp(argv[cursor], "-o", 2) == 0) {
      if (sscanf(argv[cursor], "-o%255s", output) != 1)
        sscanf(argv[++cursor], "%255s", output);
    } else {
      printf("error: unknown option '%s'\n\n", argv[cursor]);
      usage();
    }
  }

  if (input[0] == '\0' || output[0] == '\0') usage();

  // dump input matrix
  if (verbose) {
    printf("RGB transformation matrix\n");
    for (int row = 0; row < 3; row++) {
      printf("    %c = ", "RGB"[row]);
      for (int col = 0; col < 3; col++) {
        printf("%+9.6f*%c", matrix[row][col], "rgb"[col]);
      }
      printf("\n");
    }
    printf("\n");
  }

  // build matrix multiply LUTs
  // return 0;

  // initialize GDAL
  // GDALSetCacheMax(20*1024*1024);
  GDALAllRegister();

  // open input image
  GDALDataset *in = (GDALDataset *)GDALOpen(input, GA_ReadOnly);
  if (in == NULL) return 2;

  // prepare "how to make my copy" options to the file driver
  char **optionsForTIFF = NULL;
  // optionsForTIFF = CSLSetNameValue(optionsForTIFF, "TILED", "YES");
  // optionsForTIFF = CSLSetNameValue(optionsForTIFF, "BLOCKXSIZE", "512");
  // integer multiple of 16
  // optionsForTIFF = CSLSetNameValue(optionsForTIFF, "BLOCKYSIZE", "512");
  // integer multiple of 16
  // optionsForTIFF = CSLSetNameValue(optionsForTIFF, "INTERLEAVE", "BAND");
  // PIXEL or BAND
  // optionsForTIFF = CSLSetNameValue(optionsForTIFF, "COMPRESS", "ZIP");
  // JPEG, LZW, PACKBITS, DEFLATE, ZIP

  // create copy of input image as output image
  GDALDriver *driver = in->GetDriver();
  GDALDataset *out = (GDALDataset *)driver->CreateCopy(
      output, in, false, optionsForTIFF, NULL, NULL);
  CSLDestroy(optionsForTIFF);
  GDALClose(in);

  // transform pixels in copied image
  transformGDAL(out, matrix);

  // close output file
  GDALClose(out);

  // normal termination
  return 0;
}

int transformGDAL(GDALDataset *dataset, double matrix[3][3]) {
  if (dataset == NULL) return 1;
  if (matrix == NULL) return 2;

  int numBands = dataset->GetRasterCount();
  if (numBands != 3)  // bands in dataset
    return 3;

  GDALRasterBand *band[3];
  for (int i = 0; i < 3; i++) {
    band[i] = dataset->GetRasterBand(i + 1);
  }

  // transform 8-bit data
  if (band[0]->GetRasterDataType() == GDT_Byte) {
    int nXBlocks, nYBlocks, nXBlockSize, nYBlockSize;
    int iXBlock, iYBlock;

    band[0]->GetBlockSize(&nXBlockSize, &nYBlockSize);

    nXBlocks = (band[0]->GetXSize() + nXBlockSize - 1) / nXBlockSize;
    nYBlocks = (band[0]->GetYSize() + nYBlockSize - 1) / nYBlockSize;

    unsigned char *buffer[3] = {NULL};
    for (int i = 0; i < 3; i++) {
      buffer[i] = (unsigned char *)CPLMalloc(nXBlockSize * nYBlockSize);
    }

    for (iYBlock = 0; iYBlock < nYBlocks; iYBlock++) {
      for (iXBlock = 0; iXBlock < nXBlocks; iXBlock++) {
        int nXValid, nYValid;

        // determine valid extent within block
        if (iXBlock == nXBlocks - 1)
          nXValid = band[0]->GetXSize() - iXBlock * nXBlockSize;
        else
          nXValid = nXBlockSize;

        if (iYBlock == nYBlocks - 1)
          nYValid = band[0]->GetYSize() - iYBlock * nYBlockSize;
        else
          nYValid = nYBlockSize;

        // read a block from each band
        for (int i = 0; i < 3; i++) {
          int error = band[i]->ReadBlock(iXBlock, iYBlock, buffer[i]);
          (void)error;
        }

        // transform blocks
        for (int by = 0; by < nYValid; by++) {
          for (int bx = 0; bx < nXValid; bx++) {
#if 0
            // superimpose X and Y axis color ramps in Red and Green bands
            buffer[0][by*nXBlockSize + bx] = (iXBlock*nXBlockSize + bx) & 0xff;
            buffer[1][by*nXBlockSize + bx] = (iYBlock*nYBlockSize + by) & 0xff;
            //buffer[2][by*nXBlockSize + bx] = 0;
#endif

            double r = buffer[0][by * nXBlockSize + bx] / 255.0;
            double g = buffer[1][by * nXBlockSize + bx] / 255.0;
            double b = buffer[2][by * nXBlockSize + bx] / 255.0;

            double R = r * matrix[0][0] + g * matrix[0][1] + b * matrix[0][2];
            double G = r * matrix[1][0] + g * matrix[1][1] + b * matrix[1][2];
            double B = r * matrix[2][0] + g * matrix[2][1] + b * matrix[2][2];

            if (R < 0.0)
              R = 0.0;
            else if (R > 1.0)
              R = 1.0;
            if (G < 0.0)
              G = 0.0;
            else if (G > 1.0)
              G = 1.0;
            if (B < 0.0)
              B = 0.0;
            else if (B > 1.0)
              B = 1.0;

            buffer[0][by * nXBlockSize + bx] = (unsigned char)(255.0 * R);
            buffer[1][by * nXBlockSize + bx] = (unsigned char)(255.0 * G);
            buffer[2][by * nXBlockSize + bx] = (unsigned char)(255.0 * B);

            // buffer[2][by*nXBlockSize + bx] *= 2;
          }
        }

        // write a block to each band
        for (int i = 0; i < 3; i++) {
          int error = band[i]->WriteBlock(iXBlock, iYBlock, buffer[i]);
          (void)error;
        }
      }
    }

    for (int i = 0; i < 3; i++) {
      CPLFree(buffer[i]);
    }

    return 0;
  } else { // presume 16-bit short data
    int nXBlocks, nYBlocks, nXBlockSize, nYBlockSize;
    int iXBlock, iYBlock;

    band[0]->GetBlockSize(&nXBlockSize, &nYBlockSize);

    nXBlocks = (band[0]->GetXSize() + nXBlockSize - 1) / nXBlockSize;
    nYBlocks = (band[0]->GetYSize() + nYBlockSize - 1) / nYBlockSize;

    unsigned short *buffer[3] = {NULL};
    for (int i = 0; i < 3; i++) {
      buffer[i] = (unsigned short *)CPLMalloc(2 * nXBlockSize * nYBlockSize);
    }

    for (iYBlock = 0; iYBlock < nYBlocks; iYBlock++) {
      for (iXBlock = 0; iXBlock < nXBlocks; iXBlock++) {
        // read a block from each band
        for (int i = 0; i < 3; i++) {
          int error = band[i]->ReadBlock(iXBlock, iYBlock, buffer[i]);
          (void)error;
        }

        // write a block to each band
        for (int i = 0; i < 3; i++) {
          int error = band[i]->WriteBlock(iXBlock, iYBlock, buffer[i]);
          (void)error;
        }
      }
    }

    for (int i = 0; i < 3; i++) {
      CPLFree(buffer[i]);
    }

    return 0;
  }

  return 0;
}

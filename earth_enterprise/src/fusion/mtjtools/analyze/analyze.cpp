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

// TODO: High-level file comment.
#include "stdio.h"
#include "string.h"
#include "math.h"

// GDAL includes
#include "gdal_priv.h"

// KHistogram class
#include "khistogram.h"

void usage()
{
  printf("usage: analyze [-p 0/1][-z 0/1][-m mode][-s][-v][-q][-d depth][-f format]\n");
  printf("         [-g grayfile][-G gamma][-i input][-o output]\n");
  printf("  -p  protect data value 0 from analysis and transformation\n");
  printf("        NEEDED for DigitalGlobe images to protect 'off-image' area! (DEFAULT = ON)\n");
  printf("  -u  disregard unpopulated histogram areas above and below image data\n");
  printf("        Useful for 16-bit data placed in low-bits (DEFAULT = ON)\n");
  printf("  -m  set LUT-generation mode\n");
  printf("        1: linear stretch: -m1,left,right (default is populated area)\n");
  printf("        2: percent tail linear stretch: -m2,left,right (default is 0.3%%)\n");
  printf("        3: standard deviation linear stretch: -m3,left,right (default is 3 sigma)\n");
  printf("        4: F-stops from median: -m5,left,right (default is 2.5 for five stop dynamic range)\n");
  printf("        5: linear transform: -m4,r0,r1,g0,g1,b0,b1\n");
  printf("             r' = r0 + r1*r;\n");
  printf("             g' = g0 + g1*g;\n");
  printf("             b' = b0 + b1*b;\n");
  printf("  -g  linear transform derived from presumed gray samples: -g filename\n");
  printf("  -q  perform standard DigitalGlobe QuickBird data normalization on input (DEFAULT = OFF)\n");
  printf("        (from 'http://www.digitalglobe.com/downloads/Radiance Conversion of QuickBird Data.pdf')\n");
  printf("        r' = r*1.267350e-02/0.071;    r' = r*0.1785;    r' = r*1.2285;\n");
  printf("        g' = g*1.438470e-02/0.099; == g' = g*0.1453; or g' = g*1.0000;\n");
  printf("        b' = b*1.604120e-02/0.068;    b' = b*0.2359;    b' = b*1.6235;\n");
  printf("  -s  statistics (DEFAULT = OFF)\n");
  printf("  -v  verbose (DEFAULT = OFF)\n");
  printf("  -d  ouput depth, 8 for 2^8=256 or 16 for 2^16=65536 entries\n");
  printf("        Default depth is that of the input file\n");
  printf("  -f  output format\n");
  printf("        0: plain\n");
  printf("        1: compressed\n");
  printf("        2: run-length encoded\n");
  printf("        3: compressed and run-length encoded (DEFAULT)\n");
  printf("        4: Aubin format\n");
  printf("  -i  input histogram name (used by some modes)\n");
  printf("  -o  output LUT name (DEFAULT is 'analyze.lut')\n");
  printf("  -G  gamma encode value (DEFAULT is 1.0) Negate (-val) for 1/val\n");
  printf("  -U  uniform bounds for each channel based on MIN and MAX per-channel bounds\n");
  printf("        1: outside: [min(Lr,Lg,Lb)..max(Rr,Rg,Rb)]\n");
  printf("        2: inside:  [max(Lr,Lg,Lb)..min(Rr,Rg,Rb)]\n");
  printf("        3: minimum: [min(Lr,Lg,Lb)..min(Rr,Rg,Rb)]\n");
  printf("        4: maximum: [max(Lr,Lg,Lb)..max(Rr,Rg,Rb)]\n");
  printf("        5: use red: [Lr..Rr]\n");
  printf("        6: use green: [Lg..Rg]\n");
  printf("        7: use blue: [Lb..Rb]\n");

  exit(1);
}

#define ANALYZE_FP_DEFAULT (-1.0)

//
// Analyze histogram and generate at LUT
//
int main(int argc, char* argv[])
{
  int protectedMode = 1;
  int populatedMode = 1;
  int LUTmode = 0; // 0 means "unset"
  double c0[8] = {0.0};
  double c1[8] = {0.0};
  double left = ANALYZE_FP_DEFAULT;
  double right = ANALYZE_FP_DEFAULT;
  int bitDepth = 0; // 0 means "unset"
  int verbose = 0;
  int statistics = 0;
  int quickBird = 0;
  int format = 3; // most compressed
  char output[256] = "analyze.txt";
  char input[256] = {0};
  char gray[256] = {0};
  double quickBirdFactor[] = {1.2285, 1.0, 1.6235}; // r, g, b
  double gamma = 1.0;
  int uniform = 0;

  int points = 0;
  double sumR  = 0.0, sumG  = 0.0, sumB  = 0.0;
  double sumR2 = 0.0, sumG2 = 0.0, sumB2 = 0.0;
  double sumRG = 0.0, sumGB = 0.0, sumBR = 0.0;

  // parse command line 'option arguments'
  int cursor;
  for (cursor = 1; cursor < argc && argv[cursor][0] == '-'; cursor++)
  {
    if (strncmp(argv[cursor], "-p", 2) == 0)
    {
      if (sscanf(argv[cursor], "-p%d", &protectedMode) != 1)
        sscanf(argv[++cursor], "%d", &protectedMode);
    }
    else if (strncmp(argv[cursor], "-u", 2) == 0)
    {
      if (sscanf(argv[cursor], "-u%d", &populatedMode) != 1)
        sscanf(argv[++cursor], "%d", &populatedMode);
    }
    else if (strncmp(argv[cursor], "-m", 2) == 0)
    {
      int count = 0;
      double v[6] = {0.0};

      count = sscanf(argv[cursor], "-m%d,%lf,%lf,%lf,%lf,%lf,%lf", &LUTmode, v, v+1, v+2, v+3, v+4, v+5);
      if (count < 1)
        count = sscanf(argv[++cursor], "%d,%lf,%lf,%lf,%lf,%lf,%lf", &LUTmode, v, v+1, v+2, v+3, v+4, v+5);
      --count;

      if (LUTmode == 5)
      {
        c0[0] = v[0]; // r' = c0[0] + c1[0]*r
        c1[0] = v[1];

        c0[1] = v[2]; // g' = c0[1] + c1[1]*g
        c1[1] = v[3];

        c0[2] = v[4]; // b' = c0[2] + c1[2]*b
        c1[2] = v[5];
      }
      else if (count == 2)
      {
        left  = v[0];
        right = v[1];
      }
    }
    else if (strncmp(argv[cursor], "-s", 2) == 0)
    {
      statistics = 1;
    }
    else if (strncmp(argv[cursor], "-v", 2) == 0)
    {
      verbose = 1;
    }
    else if (strncmp(argv[cursor], "-q", 2) == 0)
    {
      quickBird = 1;
    }
    else if (strncmp(argv[cursor], "-f", 2) == 0)
    {
      if (sscanf(argv[cursor], "-f%d", &format) != 1)
        sscanf(argv[++cursor], "%d", &format);
    }
    else if (strncmp(argv[cursor], "-d", 2) == 0)
    {
      if (sscanf(argv[cursor], "-d%d", &bitDepth) != 1)
        sscanf(argv[++cursor], "%d", &bitDepth);
    }
    else if (strncmp(argv[cursor], "-g", 2) == 0)
    {
      // get file name
      gray[0] = '\0';
      if (sscanf(argv[cursor], "-g%255s", gray) != 1)
        sscanf(argv[++cursor], "%255s", gray);

      // read values
      FILE *fp = fopen(gray, "r");
      if (fp == NULL)
      {
        printf("error: unable to open '%s' for reading\n", gray);
      }
      else
      {
        int p = 0;
        double r, g, b;
        while (fscanf(fp, "%lf%lf%lf", &r, &g, &b) == 3)
        {
          // transform values
          // the program "optimize" finds no superior adjustment to 'no adjustment'

          // tabluate values needed for linear regression
          sumR  += r; sumR2 += r*r; sumRG += r*g;
          sumG  += g; sumG2 += g*g; sumGB += g*b;
          sumB  += b; sumB2 += b*b; sumBR += b*r;
          points++;
          p++;
        }

        if (verbose)
        {
          printf("%d points read from RGB gray-point file '%s'\n", p, gray);
        }

        fclose(fp);
      }
    }
    else if (strncmp(argv[cursor], "-i", 2) == 0)
    {
      if (sscanf(argv[cursor], "-i%255s", input) != 1)
        sscanf(argv[++cursor], "%255s", input);
    }
    else if (strncmp(argv[cursor], "-o", 2) == 0)
    {
      if (sscanf(argv[cursor], "-o%255s", output) != 1)
        sscanf(argv[++cursor], "%255s", output);
    }
    else if (strncmp(argv[cursor], "-G", 2) == 0)
    {
      if (sscanf(argv[cursor], "-G%lf", &gamma) != 1)
        sscanf(argv[++cursor], "%lf", &gamma);

      if (gamma < 0.0)
        gamma = 1.0/(-gamma);

      // gamma ENCODE means 1/value
      gamma = 1.0/gamma;
    }
    else if (strncmp(argv[cursor], "-U", 2) == 0)
    {
      if (sscanf(argv[cursor], "-U%d", &uniform) != 1)
        sscanf(argv[++cursor], "%d", &uniform);
    }
    else
    {
      printf("error: unknown option '%s'\n\n", argv[cursor]);
      usage();
    }
  }

  // determine color alignment mapping using first-order linear least-squares.
  // This is reasonable for Digital Globe data which is 11-bit linear data subject
  // to a simple (but unknown) per-channel linear mapping (c' = c*x + y)
  // our goal is to recover x and y for RED and BLUE, then align them with GREEN.
  if (points > 0)
  {
    //
    // compute R transform
    //
    double sRR = points*sumR2 - sumR*sumR; // "xx"
    double sGG = points*sumG2 - sumG*sumG; // "yy"
    double sRG = points*sumRG - sumR*sumG; // "xy"

    // Calculate the slope and y-intercept
    if (sRR < 1.0e-20)
    {
      printf("error: unable to determine RED component transformation\n");
      exit(3);
    }
    else
    {
      c1[0] = sRG/sRR; // slope
      c0[0] = (sumG - c1[0]*sumR)/double(points); // intercept
    }

    // Compute regression statistics
    double corr = sRG/sqrt(sRR*sGG);

    // report RED result
    if (verbose)
    {
      printf("neutralized red:  R' = %11.4f + %11.8f*R (r = %11.8f, r^2 = %11.8f)\n",
             c0[0], c1[0], corr, corr*corr);
    }

    // compute G transform: g' = g
    c0[1] = 0.0;
    c1[1] = 1.0;

    //
    // compute B transform
    //
    double sBB = points*sumB2 - sumB*sumB; // "xx"
    sGG = points*sumG2 - sumG*sumG; // "yy"
    double sBG = points*sumGB - sumB*sumG; // "xy"

    // Calculate the slope and y-intercept
    if (sBB < 1.0e-10)
    {
      printf("error: unable to determine BLUE component transformation\n");
      exit(3);
    }
    else
    {
      c1[2] = sBG/sBB; // slope
      c0[2] = (sumG - c1[2]*sumB)/double(points); // intercept
    }

    // Compute regression statistics
    corr = sBG/sqrt(sBB*sGG);

    // report BLUE solution
    if (verbose)
    {
      printf("neutralized blue: B' = %11.4f + %11.8f*B (r = %11.8f, r^2 = %11.8f)\n",
             c0[2], c1[2], corr, corr*corr);
    }

    // if otherwise unset, pretend that user entered this transform directly
    if (LUTmode == 0)
      LUTmode = 5;
  }

  // read histogram of input image
  KHistogram h;
  if (statistics || (LUTmode >= 1 && LUTmode <= 4))
  {
    if (h.read(input))
    {
      printf("error: unable to read histogram from file '%s'\n", input);
      return 1;
    }

    // neutralize color cast with linear mapping
    if (points > 0)
    {
      for (int b = 0; b < h.bands(); b++)
      {
        if (verbose)
        {
          const char *bandName[3] = {"red", "green", "blue"};
          printf("neutralizing %5s LUT with %c' = %9.2f + %9.6f*%c\n",
                 bandName[b], bandName[b][0], c0[b], c1[b], bandName[b][0]);
        }

        if (c0[b] != 0.0 || c1[b] != 1.0)
        {
          KTable& t = h.band(b);
          int *scaled = (int *)calloc(sizeof(int), t.bins());

          int i;
          int iMin = (protectedMode) ? 1 : 0;
          for (i = iMin; i < t.bins(); i++)
          {
            int to = int(c0[b] + double(i)*c1[b]);
            if (to < iMin)
              to = iMin;
            if (to > t.bins() - 1)
              to = t.bins() - 1;
            scaled[to] += t.bin(i);
          }

          for (i = iMin; i < t.bins(); i++)
          {
            t.bin(i) = scaled[i];
          }

          free(scaled);
        }
      }
    }

    // perform QuickBird mapping
    if (quickBird)
    {
      for (int b = 0; b < h.bands(); b++)
      {
        if (verbose)
        {
          printf("  scaling band[%d] LUT by %f\n", b, quickBirdFactor[b]);
        }

        if (quickBirdFactor[b] != 1.0)
        {
          KTable& t = h.band(b);
          int *scaled = (int *)calloc(sizeof(int), t.bins());

          int i;
          int iMin = (protectedMode) ? 1 : 0;
          for (i = iMin; i < t.bins(); i++)
          {
            int to = int(i*quickBirdFactor[b]);
            if (to < iMin)
              to = iMin;
            if (to > t.bins() - 1)
              to = t.bins() - 1;
            scaled[to] += t.bin(i);
          }

          for (i = iMin; i < t.bins(); i++)
          {
            t.bin(i) = scaled[i];
          }

          free(scaled);
        }
      }
    }

    // set statistical analysis window
    int b;
    for (b = 0; b < h.bands(); b++)
    {
      // inspect data for either full or partial range
      h.band(b).windowMin() = (protectedMode) ? 1 : 0;
      h.band(b).windowMax() = h.band(b).bins() - 1;

      if (populatedMode)
      {
        // locate extent of non-zero data
        h.statistics();

        // restrict statistics to the non-zero area (valid sensor data)
        h.band(b).windowMin() = h.band(b).minBin();
        h.band(b).windowMax() = h.band(b).maxBin();
      }
    }

    // compute histogram statistics
    h.statistics();

    // print statistics
    if (statistics)
    {
      for (b = 0; b < h.bands(); b++)
      {
        printf("band %d [%5d..%5d]: ",
               b,
               h.band(b).windowMin(),
               h.band(b).windowMax());
        printf("[%5d]=%5.0f ", h.band(b).minBin(), h.band(b).minVal());
        printf("[%5d]=%5.0f ", h.band(b).maxBin(), h.band(b).maxVal());
        printf(" ");
        printf("population=%8.0f ", h.band(b).population());
        printf("mean=%9.3f ", h.band(b).meanBin());
        printf("Var=%g ", h.band(b).varianceBin());
        printf("SD=%g ", h.band(b).sigmaBin());
        printf("\n");
      }
    }
  }

  // validate LUT mode
  if (LUTmode == 0 && statistics)
    return 0;
  else if (LUTmode < 1 || LUTmode > 5)
  {
    printf("error: bad LUTmode: %d\n\n", LUTmode);
    usage();
  }

  // while LUT is same size as input, the mapping chosen will
  //   create a smaller or larger image depth after application
  int mappedBins = 0;
  if (bitDepth == 0)
    mappedBins = h.band(0).bins();
  else if (bitDepth == 8 || bitDepth == 256)
    mappedBins = 256;
  else if (bitDepth == 16 || bitDepth == 65536)
    mappedBins = 65536;
  else
  {
    printf("error: bad bitDepth: %d\n\n", bitDepth);
    usage();
  }

  // create output LUT with same configuration as input histogram
  KHistogram LUT(h.bands(), h.band(0).bins());

  //
  // generate desired mapping specification for each band of input image
  //

  int lPoint[8];
  int rPoint[8];
  int b;
  for (b = 0; b < h.bands(); b++)
  {
    KTable& in = h.band(b);

    if (verbose)
    {
      const char *bandName[3] = {"red", "green", "blue"};
      if (points > 0)
      {

        printf("mapping %5s with %c' = %9.2f + %9.6f*%c\n",
               bandName[b], bandName[b][0], c0[b], c1[b], bandName[b][0]);
      }

      if (quickBird)
      {
        printf("mapping %5s with %c' = %9.6f*%c\n",
               bandName[b], bandName[b][0], quickBirdFactor[b], bandName[b][0]);
      }
    }

    // determine mapping parameters
    int fromLeft = 0;
    int fromRight = 0;
    if (LUTmode == 1)
    {
      // linear map from [al..ar] ==> [low..2^depth-1], where
      //   al and ar "absolute bin numbers" from [0..max] within input range
      //   depth is the output depth if specified or the input depth otherwise
      if (left == ANALYZE_FP_DEFAULT || right == ANALYZE_FP_DEFAULT)
      {
        fromLeft  = in.minBin();
        fromRight = in.maxBin();

        if (verbose)
        {
          printf("  minimum populated bin = %d\n", fromLeft);
          printf("  maximum populated bin = %d\n", fromRight);
        }
      }
      else
      {
        fromLeft  = int(left);
        fromRight = int(right);

        if (verbose)
        {
          printf("  specified minimum bin = %d\n", fromLeft);
          printf("  specified maximum bin = %d\n", fromRight);
        }
      }
    }
    else if (LUTmode == 2)
    {
      // linear map from [pl..pr] ==> [low..2^depth-1], where
      //   pl and pr are "percent of population in from the tail" from the left and right
      //   low is 0 or 1 depending on 'protectedMode'
      //   depth is the output depth if specified or the input depth otherwise
      double leftRatio = 0.0;
      double rightRatio = 0.0;
      if (left == ANALYZE_FP_DEFAULT || right == ANALYZE_FP_DEFAULT)
      {
        leftRatio  = 0.3/100.0;
        rightRatio = 0.3/100.0;
      }
      else
      {
        leftRatio = left/100.0;
        rightRatio = right/100.0;
      }

      // convert population ratio to bin index

      // start at proper place on left
      int index = 0;
      if (populatedMode)
        index = in.minBin();
      if (protectedMode && index == 0)
        index = 1;

      double tail = leftRatio*in.population();
      while (index < in.bins() && in.bin(index) <= tail)
        tail -= in.bin(index++);

      fromLeft = index;

      // start at proper place on right
      index = in.bins() - 1;
      if (populatedMode)
        index = in.maxBin();
      if (protectedMode && index == 0)
        index = 1;

      tail = rightRatio*in.population();
      while (index > 0 && in.bin(index) <= tail)
        tail -= in.bin(index--);

      fromRight = index;

      if (verbose)
      {
        printf("  %6.2f%% left  tail of (count = %.0f) ends at bin %d\n",
               100.0*leftRatio, leftRatio*in.population(), fromLeft);
        printf("  %6.2f%% right tail of (count = %.0f) ends at bin %d\n",
               100.0*rightRatio, rightRatio*in.population(), fromRight);
      }
    }
    else if (LUTmode == 3)
    {
      // linear map from [sl..sr] ==> [low..2^depth-1], where
      //   sl and sr are "multiples of sigma out from the median"
      //     if the input histogram reflects a normal distribution (why would it?) then
      //       1 sigma = 68.3% of population
      //       2 sigma = 95.4% of population
      //       3 sigma = 99.7% of population
      //   low is 0 or 1 depending on 'protectedMode'
      //   depth is the output depth if specified or the input depth otherwise
      double leftMultiple;
      double rightMultiple;
      if (left == ANALYZE_FP_DEFAULT || right == ANALYZE_FP_DEFAULT)
      {
        leftMultiple = 3.0;
        rightMultiple = 3.0;
      }
      else
      {
        leftMultiple = left;
        rightMultiple = right;
      }
      fromLeft = int(in.meanBin() - leftMultiple*in.sigmaBin());
      if (fromLeft < 0)
        fromLeft = 0;
      fromRight = int(in.meanBin() + rightMultiple*in.sigmaBin());
      if (fromRight > in.bins() - 1)
        fromRight = in.bins() - 1;

      if (verbose)
      {
        printf("  %6.2f sigma (%.2f) left of mean bin (%.2f) is bin %.2f (%d)\n",
               leftMultiple, in.sigmaBin(), in.meanBin(), in.meanBin() - leftMultiple*in.sigmaBin(), fromLeft);
        printf("  %6.2f sigma (%.2f) right of mean bin (%.2f) is bin %.2f (%d)\n",
               rightMultiple, in.sigmaBin(), in.meanBin(), in.meanBin() + rightMultiple*in.sigmaBin(), fromRight);
      }
    }
    else if (LUTmode == 4)
    {
      // linear map from [fl..fr] ==> [low..2^depth-1], where
      //   pl and pr are "F-stops left and right from the median"
      //   low is 0 or 1 depending on 'protectedMode'
      //   depth is the output depth if specified or the input depth otherwise
      double leftStops = 0.0;
      double rightStops = 0.0;
      if (left == ANALYZE_FP_DEFAULT || right == ANALYZE_FP_DEFAULT)
      {
        leftStops  = 2.5;
        rightStops = 2.5;
      }
      else
      {
        leftStops = left;
        rightStops = right;
      }

      // find median bin (NOT mean)
      int index = 0;
      if (populatedMode)
        index = in.minBin();
      if (protectedMode && index == 0)
        index = 1;
      double tail = 0.5*in.population();
      while (index < in.bins() && in.bin(index) <= tail)
        tail -= in.bin(index++);
      double medianBin = index;

      // move out from median bin by designated number of F-stops
      fromLeft  = int(medianBin/pow(2.0, leftStops));
      fromRight = int(medianBin*pow(2.0, rightStops));

      if (verbose)
      {
        printf("  %4.2f F-stops left  of median bin (%5.0f) ends at bin %5d\n",
               leftStops, medianBin, fromLeft);
        printf("  %4.2f F-stops right of median bin (%5.0f) ends at bin %5d\n",
               rightStops, medianBin, fromRight);
      }
    }

    lPoint[b] = fromLeft;
    rPoint[b] = fromRight;
  }

  // impose uniform bounds)
  if (uniform != 0)
  {
    int uL = lPoint[0];
    int uR = rPoint[0];

    if (uniform >= 1 && uniform <= 4)
    {
      for (b = 1; b < h.bands(); b++)
      {
        switch (uniform)
        {
          case 1:
            if (uL > lPoint[b]) uL = lPoint[b]; // min
            if (uR < rPoint[b]) uR = rPoint[b]; // max
            break;
          case 2:
            if (uL < lPoint[b]) uL = lPoint[b]; // max
            if (uR > rPoint[b]) uR = rPoint[b]; // min
            break;
          case 3:
            if (uL > lPoint[b]) uL = lPoint[b]; // min
            if (uR > rPoint[b]) uR = rPoint[b]; // min
            break;
          case 4:
            if (uL < lPoint[b]) uL = lPoint[b]; // max
            if (uR < rPoint[b]) uR = rPoint[b]; // max
            break;
        }
      }
    }
    else if (uniform >= 5 && uniform <= 7)
    {
      int chosenBand = uniform - 5; // 5->0 == red, 6->1 == green, 7->2 == blue
      uL = lPoint[chosenBand];
      uR = rPoint[chosenBand];
    }

    if (verbose)
    {
      const char *boundsTypeName[] = {"outer", "inner", "minimum", "maximum", "red", "green", "blue"};
      printf("uniform %s bounds are [%d..%d]\n", boundsTypeName[uniform - 1], uL, uR);
    }

    for (b = 0; b < h.bands(); b++)
    {
      lPoint[b] = uL;
      rPoint[b] = uR;
    }
  }

  // finally, do the mapping
  for (b = 0; b < h.bands(); b++)
  {
    KTable& in = h.band(b);
    KTable& out = LUT.band(b);

    int fromLeft = lPoint[b];
    int fromRight = rPoint[b];

    if (verbose)
    {
      if (gamma != 1.0)
      {
        const char *bandName[3] = {"red", "green", "blue"};
        printf("gamma mapping %5s using %c' = pow(%c, %9.6f) with [0..1]=[%d..%d] (note %g = 1/%g)\n",
               bandName[b], bandName[b][0], bandName[b][0], gamma, fromLeft, fromRight, gamma, 1.0/gamma);
      }
    }

    if (LUTmode >= 1 && LUTmode <= 4)
    {
      // sanity check 'from' parameters
      if (fromLeft < 0)
        fromLeft = 0;
      if (protectedMode && fromLeft == 0)
        fromLeft = 1;

      if (fromRight < fromLeft)
        fromRight = fromLeft;
      if (fromRight > in.bins() - 1)
        fromRight = in.bins() - 1;

      // determine 'to' parameters
      int toLeft = (protectedMode) ? 1 : 0;
      int toRight = mappedBins - 1;

      if (verbose)
      {
        const char *bandName[3] = {"red", "green", "blue"};
        printf("mapping %5s [%d..%d] to [%d..%d]\n", bandName[b], fromLeft, fromRight, toLeft, toRight);
      }

      // build loop-invariant constants
      double fL = fromLeft - 0.5;
      double fR = fromRight + 0.5;
      double tL = toLeft - 0.5;
      double tR = toRight + 0.5;

      // build mapping (remapping is virtual layers of mapping)
      for (int index = 0; index < in.bins(); index++)
      {
        // "natural" value
        double pixel = index;

        // remap pixel based on color-cast removal parameters
        if (points > 0)
        {
          pixel = c0[b] + c1[b]*pixel;
        }

        // remap pixel based on Digital Globe QuickBird parameters
        if (quickBird)
        {
          pixel *= quickBirdFactor[b];
        }

        // pixels below [fromLeft..fromRight] map to [toLow]
        if (pixel < fromLeft)
        {
          out.bin(index) = toLeft;
        }
        else if (pixel > fromRight)
        {
          out.bin(index) = toRight;
        }
        else
        {
          // determine position within old range [fL..fR]
          double f = double(pixel - fL)/double(fR - fL);

          // remap pixel based on gamma value
          if (gamma != 1.0)
            f = pow(f, gamma);

          // map f to new range [tL..tR]
          int newPixel = int(tL + f*(tR - tL));

          if (protectedMode && newPixel == 0)
            newPixel = 1;
          out.bin(index) = newPixel;
        }
      }
    }
    else if (LUTmode == 5)
    {
      // apply transformation
      for (int index = 0; index < in.bins(); index++)
      {
        double newPixel = c0[b] + c1[b]*double(index);

        // remap pixel based on gamma value
        //  ..don't worry about bin(0), it is restored elsewhere
        //  ..USER IS MAKING A PROBABLE MISTAKE SINCE RANGE IS FULL SCALE
        if (gamma != 1.0)
        {
          int iMin = (protectedMode) ? 1 : 0;
          int iMax = in.bins() - 1;

          double f = double(newPixel - iMin)/double(iMax - iMin);

          if (f < 0.0)
            f = 0.0;
          if (f > 1.0)
            f = 1.0;

          f = pow(f, gamma);

          newPixel = iMin + f*(iMax - iMin);
        }

        if (newPixel < 0.0)
          newPixel = 0.0;
        else if (newPixel >= mappedBins - 1)
          newPixel = mappedBins - 1;
        out.bin(index) = int(newPixel);
      }
    }

    // preserve the protected bin
    if (protectedMode)
    {
      // ...by mapping [0] to [0]
      out.bin(0) = 0;

      // ...by preventing other values from mapping to [0]
      for (int index = 1; index < in.bins(); index++)
      {
        if (out.bin(index) < 1)
          out.bin(index) = 1;
      }
    }
  }

  // write output file
  LUT.write(output, format);

  // normal termination
  return 0;
}

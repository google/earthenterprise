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

/* TODO: High-level file comment. */
/*
 *               Filtered Image Rescaling
 *
 *                 by Dale Schumacher
 *
 */

/*
  Additional changes by Ray Gardener, Daylon Graphics Ltd.
  December 4, 1999

  Summary:

  - Horizontal filter coverages are calculated on the fly,
  as each column is mapped from src to dst image. This lets
  us omit having to allocate a temporary full horizontal stretch
  of the src image.

  - If none of the src pixels within a sampling region differ,
  then the output pixel is forced to equal (any of) the source pixel.
  This ensures that filters do not corrupt areas of constant color.

  - Filter weight coverage results, after summing, are
  rounded to the nearest pixel color value instead of
  being casted to Pixel (usually an int or char). Otherwise,
  artifacting occurs.

  - All memory allocations checked for failure; zoom() returns
  error code. new_image() returns nullptr if unable to allocate
  pixel storage, even if Image struct can be allocated.
  Some assertions added.
*/

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#ifndef M_PI
#define M_PI     3.14159265359
#endif

#define USE_LUT
#define LUT_TYPE short
#define LUT_GUARD_BITS 3
#define LUT_BINARY_POINT (8+LUT_GUARD_BITS)

struct CONTRIB
{
  int             pixel;
#ifdef  USE_LUT
  LUT_TYPE weight;
#else
  float   weight;
#endif
};

class CLIST
{
 public:
  void add(int index, float weight, int size, bool& partial)
  {
    // remember pixels with non-zero coverage
    if (weight != 0.0f)
    {
#ifdef REFLECT
      // policy when sampling beyond edge of image is to reflect inward
      while (index < 0 || index >= srcSize)
      {
        if (index < 0)
          index = -index;                                 // reflect from lower edge
        if (index >= size)
          index = 2*size - index - 1;             // reflect from upper edge
      }
#else
      // policy is to only consider samples in the source image
      if (index < 0 || index >= size)
      {
        partial = true;
      }
      else
#endif
      {
        p[n].pixel = index;
#ifdef USE_LUT
        p[n].weight = LUT_TYPE((float(1<<LUT_BINARY_POINT))*weight);
#else
        p[n].weight = weight;
#endif
        ++n;
      }
    }
  }

  void normalize ()
  {
#ifdef USE_LUT
    int total = 0;

    int i;
    for (i = 0; i < n; i++)
    {
      total += p[i].weight;
    }

    if (total != 0 && total != (1<<LUT_BINARY_POINT))
    {
      float rescale = float(1<<LUT_BINARY_POINT)/float(total);

      for (i = 0; i < n; i++)
        p[i].weight = (int)(p[i].weight*rescale);
    }
#else
    float total = 0.0f;

    for (int i = 0; i < n; i++)
    {
      total += p[i].weight;
    }

    if (total != 0.0f && total != 1.0f)
    {
      float rescale = 1.0f/total;

      for (i = 0; i < n; i++)
        p[i].weight *= rescale;
    }
#endif
  }

 public:
  int n;                  // number of contributors
  CONTRIB *p;             // pointer to list of coverages
};

namespace Filter
{
#ifdef POINT_SAMPLE
typedef float (*Filter_t)(float);
#else
typedef float (*Filter_t)(float, float);
#endif

//
//      filter function definitions
//

//
// impulse function
//

const static float impulseSupport = 0.0f;

#ifdef POINT_SAMPLE
#else
// integrate impulse function between [lower..upper]
float impulseFilter(float lower, float upper)
{
  // impulse function
  //   f(x) = (x==0)? 1 : 0

  float area = 0.0f;

  if (lower <= 0.0f && 0.0f < upper)
    area = 1.0f;

  return area;
}
#endif

//
// box function
//

const static float boxSupport = 0.5f;

#ifdef POINT_SAMPLE
// sample box function at x
float boxFilter(float x)
{
  // box function
  //   [-1/2..1/2]: f(x) = 1, with anti-derivative g(x) = x

  float height = 0.0f;

  if (-boxSupport <= x && x < boxSupport)
    height = 1.0f;

  return height;
}
#else
// integrate box function between [lower..upper]
float boxFilter(float lower, float upper)
{
  // box function
  //   [-1/2..1/2]: f(x) = 1, with anti-derivative g(x) = x

  // no overlap with extent of filter
  if (upper <= -boxSupport || lower >= boxSupport)
    return 0.0f;

  // clamp limits to extent of filter
  if (lower < -boxSupport)
    lower = -boxSupport;
  if (upper >  boxSupport)
    upper =  boxSupport;

  float area = upper - lower;

  return area;
}
#endif

//
// triangle function (Bartlett window)
//

const static float triangleSupport = 1.0f;

#ifdef POINT_SAMPLE
// sample triangle function at x
float triangleFilter(float x)
{
  float height = 0.0;

  x = fabsf(x);
  if (x <= triangleSupport)
    height = 1.0f - x;

  return height;
}
#else
// integrate triangle function between [lower..upper]
float triangleFilter(float lower, float upper)
{
  // triangle function
  //   [-1..0]: f(x) = x + 1, with anti-derivative g(x) = (1/2)x^2 + x
  //   [ 0..1]: f(x) = 1 - x, with anti-derivative g(x) = x - (1/2)x^2

  // no overlap with extent of filter
  if (upper <= -triangleSupport || lower >= triangleSupport)
    return 0.0f;

  // clamp limits to extent of filter
  if (lower < -triangleSupport)
    lower = -triangleSupport;
  if (upper >  triangleSupport)
    upper =  triangleSupport;

  float area = 0.0f;

  if (upper <= 0.0f)
  {
    area = (upper - lower)*(1.0f + 0.5f*(upper + lower)); // both limits within [-1..0]
  }
  else if (lower >= 0.0f)
  {
    area = (upper - lower)*(1.0f - 0.5f*(upper + lower)); // both limits within [0..1]
  }
  else
  {
    // interval crosses discontinuity at zero - subtract triangles at end
    lower = 1.0f + lower; // distance from -1.0 to 'lower'
    upper = 1.0f - upper; // distance from 'upper' to 1.0

    area = 1.0f - 0.5f*(lower*lower + upper*upper);
  }

  return area;
}
#endif

// Mitchell-Netravali BC-Spline
//   [0..1]: f(x) = (12-9*b-6*c)/6*x^3 + (-18+12*b+6*c)/6*x^2 + (6-2*b)/6
//                       g(x) = 1/24*(12-9*b-6*c)*x^4 + 1/18*(-18+12*b+6*c)*x^3 + x-1/3*b*x
//                              with b=1/3, c=1/3, this simplifies to
//                       f(x) = 7/6*x^3 - 2*x^2 + 8/9
//                       g(x) = 7/24*x^4- 2/3*x^3 + 8/9*x
//
//   [1..2]: f(x) = (-b-6*c)/6*x^3 + (6*b+30*c)/6*x^2 + (-12*b-48*c)/6*x + (8*b+24*c)/6
//                       g(x) = 1/24*(-b-6*c)*x^4 + 1/18*(6*b+30*c)*x^3 + 1/12*(-12*b-48*c)*x^2 + 4*c*x + 4/3*b*x
//                              with b=1/3, c=1/3, this simplifies to
//                       f(x) = -7/18*x^3 + 2*x^2 - 10/3*x + 16/9
//                       g(x) = -7/72*x^4 + 2/3*x^3 - 5/3*x^2 + 16/9*x

const static float MitchellSupport = 2.0f;

#ifdef POINT_SAMPLE
// sample Mitchell-Netravali BC-Spline (with B=1/3, C=1/3) at x
float MitchellFilter(float x)
{
  float height = 0.0f;

  x = fabsf(x);
  if (x <= 1.0f)
  {
    // evaluate "7/6*x^3 - 2*x^2 + 8/9" in Horner form (3 mul, 2 add)
    height = (7.0f/6.0f*x - 2.0f)*x*x + 8.0f/9.0f;
  }
  else if (x <= 2.0f)
  {
    // evaluate "-7/18*x^3 + 2*x^2 - 10/3*x + 16/9" in Horner form (3 mul, 3 add)
    height = ((-7.0f/18.0f*x + 2.0f)*x - 10.0f/3.0f)*x + 16.0f/9.0f;
  }

  return height;
}
#else
// integrate Mitchell-Netravali BC-Spline with (B=1/3, C=1/3) between [lower..upper]
float MitchellFilter(float lower, float upper)
{
  float area = 0.0f;

  if (upper >= -2.0f && lower <= -1.0f) // handle interval from [-2..-1]
  {
    float L = (lower < -2.0f) ? -2.0f : lower;
    float R = (upper > -1.0f) ? -1.0f : upper;

    L = -L;
    R = -R;

    area += (((-7.0f/72.0f*L + 2.0f/3.0f)*L - 5.0f/3.0f)*L + 16.0f/9.0f)*L;
    area -= (((-7.0f/72.0f*R + 2.0f/3.0f)*R - 5.0f/3.0f)*R + 16.0f/9.0f)*R;
  }

  if (upper >= -1.0f && lower <= 0.0f) // handle interval from [-1..0]
  {
    float L = (lower < -1.0f) ? -1.0f : lower;
    float R = (upper >  0.0f) ?  0.0f : upper;

    L = -L;
    R = -R;

    area += ((7.0f/24.0f*L - 2.0f/3.0f)*L*L + 8.0f/9.0f)*L;
    area -= ((7.0f/24.0f*R - 2.0f/3.0f)*R*R + 8.0f/9.0f)*R;
  }

  if (upper >= 0.0f && lower <= 1.0f) // handle interval from [0..1]
  {
    float L = (lower <  0.0f) ?  0.0f : lower;
    float R = (upper >  1.0f) ?  1.0f : upper;

    area -= ((7.0f/24.0f*L - 2.0f/3.0f)*L*L + 8.0f/9.0f)*L;
    area += ((7.0f/24.0f*R - 2.0f/3.0f)*R*R + 8.0f/9.0f)*R;
  }

  if (upper >= 1.0f && lower <= 2.0f) // handle interval from [1..2]
  {
    float L = (lower < 1.0f) ? 1.0f : lower;
    float R = (upper > 2.0f) ? 2.0f : upper;

    area -= (((-7.0f/72.0f*L + 2.0f/3.0f)*L - 5.0f/3.0f)*L + 16.0f/9.0f)*L;
    area += (((-7.0f/72.0f*R + 2.0f/3.0f)*R - 5.0f/3.0f)*R + 16.0f/9.0f)*R;
  }

  return area;
}
#endif
}

class Image;
//class Resize;

int resize(Image& src, Image& dst,
           Filter::Filter_t filterFunction = Filter::boxFilter, float filterWidth = Filter::boxSupport,
           float xOffset = 0.0f, float yOffset = 0.0f, float xScale = 0.0f, float yScale = 0.0f);

typedef unsigned char Pixel;

class Image
{
 public:
  Image(int x, int y, Pixel *d = 0, int s = 0)
  {

    xsize = x;
    ysize = y;

    if (d == 0)
    {
      data = (Pixel *)calloc(sizeof(Pixel), xsize*ysize);
      span = xsize*sizeof(Pixel);
      dynamic = true;
    }
    else
    {
      data = d;
      span = (s == 0) ? xsize*sizeof(Pixel) : s;
      dynamic = false;
    }
  }

  Image(int x, int y, Image& source,
        Filter::Filter_t filterFunction = Filter::boxFilter, float filterWidth = Filter::boxSupport,
        float xo = 0.0f, float yo = 0.0f, float xs = 0.0f, float ys = 0.0f)
  {

    xsize = x;
    ysize = y;
    data = (Pixel *)calloc(sizeof(Pixel), xsize*ysize);
    span = xsize*sizeof(Pixel);
    dynamic = true;

    resize(source, *this, filterFunction, filterWidth, xo, yo, xs, ys);
  }

  ~Image()
  {
    if (dynamic)
      free(data);

  }

  Image(const Image&) = delete;
  Image(Image&&) = delete;
  Image& operator=(const Image&) = delete;
  Image& operator=(Image&&) = delete;

  Pixel getPixel(int x, int y)
  {
    static int yy = -1;
    static Pixel *p = nullptr;
#ifdef QUICK
    return ((Pixel *)(((char *)data) + y*span))[x]; // not quicker of compiler can't detect constant y
#endif
#if 0
    if ((x < 0) || (x >= xsize) || (y < 0) || (y >= ysize))
    {
      return 0;
    }
#endif
    if (yy != y)
    {
      yy = y;
      p = (Pixel *)(((char *)data) + y*span);
    }

    return p[x];
  }

  Pixel putPixel(int x, int y, Pixel d)
  {
    static int yy = -1;
    static Pixel *p = nullptr;
#ifdef QUICK
    return ((Pixel *)(((char *)data) + y*span))[x] = d;
#endif
    if ((x < 0) || (x >= xsize) || (y < 0) || (y >= ysize))
    {
      return 0;
    }

    if (yy != y)
    {
      yy = y;
      p = (Pixel *)(((char *)data) + y*span);
    }

    return p[x] = d;
  }

  void getRow(Pixel *row, int y)
  {
#if 0
    if ((y < 0) || (y >= ysize))
    {
      return;
    }
#endif
    memcpy(row, ((char *)data) + y*span, sizeof(Pixel)*xsize);
  }

  void getColumn(Pixel *column, int x)
  {
#ifdef QUICK
    return;
#endif
    if ((x < 0) || (x >= xsize))
    {
      return;
    }

    Pixel *p = data + x;
    for (int i = ysize; i-- > 0; p = (Pixel *)(((char *)p) + span))
    {
      *column++ = *p;
    }
  }

  void putColumn(Pixel *column, int x)
  {
#ifdef QUICK
    return;
#endif
    if ((x < 0) || (x >= xsize))
    {
      return;
    }

    Pixel *p = data + x;
    for (int i = ysize; i-- > 0; p = (Pixel *)(((char *)p) + span))
    {
      *p = *column++;
    }
  }

  int     xsize;          // horizontal size of the image in pixels
  int     ysize;          // vertical size of the image in pixels
  Pixel* data;    // pointer to first scanline of image

 private:


  int     span;           // byte offset between two scanlines
  bool dynamic;   // image data area was allocated dynamically
};

class Resize
{
 public:
  Resize(int srcXSize, int srcYSize, int dstXSize, int dstYSize,
         Filter::Filter_t filterFunction = Filter::boxFilter, float filterWidth = Filter::boxSupport,
         float xOffset = 0.0f, float yOffset = 0.0f, float xScale = 0.0f, float yScale = 0.0f)
  {
    // use relative image sizes as backup for scale factor determination
    if (xScale == 0.0f)
      xScale = (float)dstXSize/(float)srcXSize;
    if (yScale == 0.0f)
      yScale = (float)dstYSize/(float)srcYSize;

    // pre-calculate filter coverages
    contribY = (CLIST *)calloc(sizeof(CLIST), dstYSize);
    makeFilterTable(contribY, dstYSize, srcYSize, yScale, filterFunction, filterWidth, yOffset);

    contribX = (CLIST *)calloc(sizeof(CLIST), dstXSize);
    makeFilterTable(contribX, dstXSize, srcXSize, xScale, filterFunction, filterWidth, xOffset);

    // temporary data for apply()
    //   ...should be allocated there, but doing it here saves allocations with banded RGB images
#ifdef  USE_LUT
    intermediate = (LUT_TYPE*)calloc(sizeof(LUT_TYPE), srcYSize);
#else
    intermediate = (float*)calloc(sizeof(float), srcYSize);
#endif
    column = (Pixel *)calloc(sizeof(Pixel), dstYSize);
  }

  Resize(Image& src, Image& dst,
         Filter::Filter_t filterFunction = Filter::boxFilter, float filterWidth = Filter::boxSupport,
         float xOffset = 0.0f, float yOffset = 0.0f, float xScale = 0.0f, float yScale = 0.0f)
  {
    // use relative image sizes as backup for scale factor determination
    if (xScale == 0.0f)
      xScale = (float)dst.xsize/(float)src.xsize;
    if (yScale == 0.0f)
      yScale = (float)dst.ysize/(float)src.ysize;

    // pre-calculate filter coverages
    contribY = (CLIST *)calloc(sizeof(CLIST), dst.ysize);
    makeFilterTable(contribY, dst.ysize, src.ysize, yScale, filterFunction, filterWidth, yOffset);

    contribX = (CLIST *)calloc(sizeof(CLIST), dst.xsize);
    makeFilterTable(contribX, dst.xsize, src.xsize, xScale, filterFunction, filterWidth, xOffset);

    // temporary data for apply()
    //   ...should be allocated there, but doing it here saves allocations with banded RGB images
#ifdef  USE_LUT
    intermediate = (LUT_TYPE*)calloc(sizeof(LUT_TYPE), src.ysize);
#else
    intermediate = (float*)calloc(sizeof(float), src.ysize);
#endif
    column = (Pixel *)calloc(sizeof(Pixel), dst.ysize);
  }

  ~Resize()
  {
    free(column);                   // should be allocated/freed in apply()
    free(intermediate);             // should be allocated/freed in apply()

    free(contribX[0].p);    // all contributions are in this single allocation
    free(contribX);

    free(contribY[0].p);    // all contributions are in this single allocation
    free(contribY);
  }

  void apply(Image& src, Image& dst)
  {
    for (int dstX = 0; dstX < dst.xsize; dstX++)
    {
      // Apply horz filter to make dst column in tmp.
      for (int srcY = 0; srcY < src.ysize; ++srcY)
      {
        // 1D horizontal convolution of weights and source pixels
#ifdef  USE_LUT
        int sum = 0;
        for (int j = 0; j < contribX[dstX].n; ++j)
        {
          sum += int(contribX[dstX].p[j].weight) * src.getPixel(contribX[dstX].p[j].pixel, srcY);
        }

        intermediate[srcY] = LUT_TYPE((sum + (1<<(LUT_BINARY_POINT-1))) >> LUT_BINARY_POINT);
#else
        float sum = 0.0f;
        for (int j = 0; j < contribX[dstX].n; ++j)
        {
          sum += contribX[dstX].p[j].weight * src.getPixel(contribX[dstX].p[j].pixel, srcY);
        }
        intermediate[srcY] = sum;
#endif
      }

      // The temp column has been built. Now stretch it vertically into dst column.
      for (int dstY = 0; dstY < dst.ysize; ++dstY)
      {
#ifdef USE_LUT
        // 1D vertical convolution of weights and intermediate result
        int sum = 0;
        for (int j = 0; j < contribY[dstY].n; ++j)
        {
          sum += contribY[dstY].p[j].weight * intermediate[contribY[dstY].p[j].pixel];
        }

        // offset filtered value by +0.5 to remove bias from float->integer truncation
        sum = (sum + (1<<(LUT_BINARY_POINT-1))) >> LUT_BINARY_POINT;

        // clamp filtered value to range of valid Pixel values (necessary with negative lobed filters)
        // presumes TypeOf(sum)==float && TypeOf(Pixel)==unsigned char
        if (sum < 0)
        {
          sum = 0;
        }
        else if (sum > 255)
        {
          sum = 255;
        }

        // convert filtered value value to Pixel data type by truncation
        column[dstY] = (Pixel)sum;
#else
        // 1D vertical convolution of weights and intermediate result
        float sum = 0.0f;
        for (int j = 0; j < contribY[dstY].n; ++j)
        {
          sum += contribY[dstY].p[j].weight * intermediate[contribY[dstY].p[j].pixel];
        }

        // offset filtered value by +0.5 to remove bias from float->integer truncation
        sum += 0.5f;

        // clamp filtered value to range of valid Pixel values (necessary with negative lobed filters)
        // presumes TypeOf(sum)==float && TypeOf(Pixel)==unsigned char
        if (sum < 0.0f)
        {
          sum = 0.0f;
        }
        else if (sum > 255.0f)
        {
          sum = 255.0f;
        }

        // convert filtered value value to Pixel data type by truncation
        column[dstY] = (Pixel)sum;
#endif
      }

      dst.putColumn(column, dstX);
    }
  }

 private:
  int makeFilterTable(CLIST* clist, int dstSize, int srcSize, float scaleFactor,
                      Filter::Filter_t filterFunction, float filterWidth, float offset)
  {
    // construct and initialize contribution structures
    int support = (scaleFactor < 1.0f) ? (int)ceilf(filterWidth/scaleFactor) : (int)ceilf(filterWidth);
    int samples = 2*support + 1;

    clist[0].n = 0;
    clist[0].p = (CONTRIB *)calloc(sizeof(CONTRIB), samples*dstSize); // single large allocation
    if (clist[0].p == nullptr)
      return -1;

    for (int index = 1; index < dstSize; index++)
    {
      clist[index].n = 0;
      clist[index].p = &clist[0].p[samples*index]; // per-pixel lists are portions of single allocation
    }

    if (scaleFactor < 1.0f)
    {
      // calculate minification contributions
      float width = filterWidth/scaleFactor; // scale filter support up
      for (int index = 0; index < dstSize; index++)
      {
        float center = offset + ((float)index + 0.5f)/scaleFactor; // scale center position up
        int left = (int)floorf(center - width);
        int right = (int)ceilf(center + width);

        bool partial = false;
        for (int j = left; j <= right; ++j)
        {
          // determine coverage
#ifdef POINT_SAMPLE
          float where = ((float)j - center)*scaleFactor;  // scale sample position down
          float weight = (*filterFunction)(where)*scaleFactor;  // scale sample weight down
#else // INTEGRATE
          float lower = ((float)j - center - 0.5f)*scaleFactor; // scale sample position down
          float upper = ((float)j - center + 0.5f)*scaleFactor; // scale sample position down
          float weight = (*filterFunction)(lower, upper);
#endif
          clist[index].add(j, weight, srcSize, partial);
        }

        // ensure that weights sum to 1.0
#ifndef POINT_SAMPLE
        if (partial)
#endif
          clist[index].normalize();
      }
    }
    else
    {
      // calculate magnification contributions
      for (int index = 0; index < dstSize; index++)
      {
        float center = offset + ((float)index + 0.5f)/scaleFactor; // scale center position down
        int left = (int)floorf(center - filterWidth);
        int right = (int)ceilf(center + filterWidth);

        bool partial = false;
        for (int j = left; j <= right; ++j)
        {
          // determine weighted coverage
#ifdef POINT_SAMPLE
          float where = float(j) - center;
          float weight = (*filterFunction)(where);
#else // INTEGRATE
          float lower = float(j) - center - 0.5f;
          float upper = float(j) - center + 0.5f;
          float weight = (*filterFunction)(lower, upper);
#endif
          clist[index].add(j, weight, srcSize, partial);
        }

        // ensure that weights sum to 1.0
#ifndef POINT_SAMPLE
        if (partial)
#endif
          clist[index].normalize();
      }
    }

    return 0;
  }

  CLIST* contribY;
  CLIST* contribX;
#ifdef USE_LUT
  LUT_TYPE* intermediate;
#else
  float* intermediate;
#endif
  Pixel* column;
};

// zoom: resizes bitmaps while resampling them.
// Returns 0 if success, small integer error code otherwise
int resize(Image& src, Image& dst,
           Filter::Filter_t filterFunction, float filterWidth,
           float xOffset, float yOffset, float xScale, float yScale)
{
  Resize weights(src, dst, filterFunction, filterWidth, xOffset, yOffset, xScale, yScale);
  weights.apply(src, dst);
  return 0;
}

#if 0
====================================


Bessel   Blackman   Box
Catrom   Cubic      Gaussian
Hanning  Hermite    Lanczos
Mitchell Point      Quandratic
Sinc     Triangle

point           0.00
box             0.50
triangle        1.00

quadratic       1.50
cubic           2.00
catrom          2.00

mitchell        2.00

gaussian      1.25
sinc          4.00 (windowed by default)
    bessel        3.24 (windowed by default)
    hanning       1.00
hamming       1.00
blackman      1.00
kaiser        1.00 (kaiser: a=6.5 i0a=0.00940797)


    /*--------------- unit-area filters for unit-spaced samples ---------------*/

    /* all filters centered on 0 */

    double filt_box(double x, void *d) /* box, pulse, Fourier window, */
    /* 1st order (constant) b-spline */
{
  if (x<-.5) return 0.;
  if (x<.5) return 1.;
  return 0.;
}

double filt_triangle(double x, void *d) /* triangle, Bartlett window, */
    /* 2nd order (linear) b-spline */
{
  if (x<-1.) return 0.;
  if (x<0.) return 1.+x;
  if (x<1.) return 1.-x;
  return 0.;
}

double filt_quadratic(double x, void *d)        /* 3rd order (quadratic) b-spline */
{
  double t;

  if (x<-1.5) return 0.;
  if (x<-.5) {t = x+1.5; return .5*t*t;}
  if (x<.5) return .75-x*x;
  if (x<1.5) {t = x-1.5; return .5*t*t;}
  return 0.;
}

double filt_cubic(double x, void *d) /* 4th order (cubic) b-spline */
{
  double t;

  if (x<-2.) return 0.;
  if (x<-1.) {t = 2.+x; return t*t*t/6.;}
  if (x<0.) return (4.+x*x*(-6.+x*-3.))/6.;
  if (x<1.) return (4.+x*x*(-6.+x*3.))/6.;
  if (x<2.) {t = 2.-x; return t*t*t/6.;}
  return 0.;
}

/* Catmull-Rom spline, Overhauser spline */
double filt_catrom(double x, void *d)
{
  if (x<-2.) return 0.;
  if (x<-1.) return .5*(4.+x*(8.+x*(5.+x)));
  if (x<0.) return .5*(2.+x*x*(-5.+x*-3.));
  if (x<1.) return .5*(2.+x*x*(-5.+x*3.));
  if (x<2.) return .5*(4.+x*(-8.+x*(5.-x)));
  return 0.;
}

double filt_gaussian(double x, void *d) /* Gaussian (infinite) */
{
  return exp(-2.*x*x)*sqrt(2./PI);
}

/* Sinc, perfect lowpass filter (infinite) */
double filt_sinc(double x, void *d)
{
  return x==0. ? 1. : sin(PI*x)/(PI*x);
}

/* Bessel (for circularly symm. 2-d filt, inf)*/
double filt_bessel(double x, void *d)
{
  /*
   * See Pratt "Digital Image Processing" p. 97 for Bessel functions
   * zeros are at approx x=1.2197, 2.2331, 3.2383, 4.2411, 5.2428, 6.2439,
   * 7.2448, 8.2454
   */
  return x==0. ? PI/4. : j1(PI*x)/(2.*x);
}

/*-------------------- parameterized filters --------------------*/

/* Mitchell & Netravali's two-param cubic */
double filt_mitchell(double x, void *d)
{
  mitchell_data *m = (mitchell_data *)d;

  /*
   * see Mitchell&Netravali, "Reconstruction Filters in Computer Graphics",
   * SIGGRAPH 88
   */
  if (x<-2.) return 0.;
  if (x<-1.) return m->q0-x*(m->q1-x*(m->q2-x*m->q3));
  if (x<0.) return m->p0+x*x*(m->p2-x*m->p3);
  if (x<1.) return m->p0+x*x*(m->p2+x*m->p3);
  if (x<2.) return m->q0+x*(m->q1+x*(m->q2+x*m->q3));
  return 0.;
}

static void mitchell_init(double b, double c, void *d)
{
  mitchell_data *m;

  m = (mitchell_data *)d;
  m->p0 = (  6. -  2.*b        ) / 6.;
  m->p2 = (-18. + 12.*b +  6.*c) / 6.;
  m->p3 = ( 12. -  9.*b -  6.*c) / 6.;
  m->q0 = (            8.*b + 24.*c) / 6.;
  m->q1 = (         - 12.*b - 48.*c) / 6.;
  m->q2 = (            6.*b + 30.*c) / 6.;
  m->q3 = (     -     b -  6.*c) / 6.;
}

static void mitchell_print(void *d)
{
  mitchell_data *m;

  m = (mitchell_data *)d;
  fprintf(stderr,"mitchell: p0=%g p2=%g p3=%g q0=%g q1=%g q2=%g q3=%g\n",
          m->p0, m->p2, m->p3, m->q0, m->q1, m->q2, m->q3);
}

/*-------------------- window functions --------------------*/

double filt_hanning(double x, void *d)  /* Hanning window */
{
  return .5+.5*cos(PI*x);
}

double filt_hamming(double x, void *d)  /* Hamming window */
{
  return .54+.46*cos(PI*x);
}

double filt_blackman(double x, void *d) /* Blackman window */
{
  return .42+.50*cos(PI*x)+.08*cos(2.*PI*x);
}

/*-------------------- parameterized windows --------------------*/

double filt_kaiser(double x, void *d)   /* parameterized Kaiser window */
{
  /* from Oppenheim & Schafer, Hamming */
  kaiser_data *k;

  k = (kaiser_data *)d;
  return bessel_i0(k->a*sqrt(1.-x*x))*k->i0a;
}

static void kaiser_init(double a, double b, void *d)
{
  kaiser_data *k;

  k = (kaiser_data *)d;
  k->a = a;
  k->i0a = 1./bessel_i0(a);
}

static void kaiser_print(void *d)
{
  kaiser_data *k;

  k = (kaiser_data *)d;
  fprintf(stderr,"kaiser: a=%g i0a=%g\n", k->a, k->i0a);
}

double bessel_i0(double x)
{
  /*
   * modified zeroth order Bessel function of the first kind.
   * Are there better ways to compute this than the power series?
   */
  int i;
  double sum, y, t;

  sum = 1.;
  y = x*x/4.;
  t = y;
  for (i=2; t>EPSILON; i++) {
    sum += t;
    t *= (double)y/(i*i);
  }
  return sum;
}

/*--------------- filters for non-unit spaced samples ---------------*/

/* normal distribution (infinite) */
double
filt_normal(double x, void *d)
{
  /*
   * normal distribution: has unit area, but it's not for unit spaced samples
   * Normal(x) = Gaussian(x/2)/2
   */
  return exp(-x*x/2.)/sqrt(2.*PI);
}


case GAN_DOUBLE:
{
  double x, norm;

  norm = (mask->data.d[0] = 1.0);
  for ( i = (int)mask_size/2; i > 0; i-- )
  {
    x = (double) i;
    norm += 2.0*((mask->data.d[i] = exp(-0.5*x*x/(sigma*sigma))));
  }

  /* normalise mask so that elements add up to "scale" parameter */
  norm /= scale;
  for ( i = (int)mask_size/2; i >= 0; i-- )
    mask->data.d[i] /= norm;
}
break;



// const static double filterSupport;
const static double filterSupport = 1.0;
double filterFilter(double t)
{
  /* f(t) = 2|t|^3 - 3|t|^2 + 1, -1 <= t <= 1 */
  if (t < 0.0) t = -t;
  if (t < 1.0) return((2.0 * t - 3.0) * t * t + 1.0);
  return(0.0);
}

/* box (*) box (*) box */
const static double bellSupport = 1.5;
double bellFilter(double t)
{
  if (t < 0) t = -t;
  if (t < .5) return(.75 - (t * t));
  if (t < 1.5)
  {
    t = (t - 1.5);
    return(.5 * (t * t));
  }
  return(0.0);
}

/* box (*) box (*) box (*) box */
const static double BSplineSupport = 2.0;
double BSplineFilter(double t)
{
  double tt;

  if (t < 0)
    t = -t;
  if (t < 1)
  {
    tt = t * t;
    return((.5 * tt * t) - tt + (2.0 / 3.0));
  }
  else if (t < 2)
  {
    t = 2 - t;
    return((1.0 / 6.0) * (t * t * t));
  }
  return(0.0);
}

static double sinc(double x)
{
  x *= M_PI;
  if (x != 0)
    return(sin(x) / x);
  return(1.0);
}

const static double Lanczos3Support = 3.0;
double Lanczos3Filter(double t)
{
  if (t < 0) t = -t;
  if (t < 3.0) return(sinc(t) * sinc(t/3.0));
  return(0.0);
}

=======================================================================
#endif

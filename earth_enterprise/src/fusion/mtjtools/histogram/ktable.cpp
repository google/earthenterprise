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
#include <math.h>
#include "ktable.h"

//
//      KTable
//

KTable::KTable(int size)
{
  _size = size;
  _bin = (int *)calloc(sizeof(int), _size);
  _bins = _size;

  _wMin = 0;
  _wMax = _size - 1;
}

KTable::~KTable()
{
  free(_bin);
  _size = 0;
}

void KTable::reset()
{
  // reset active number of bins
  _bins = 0;

  // reset counts in all bins
  for (int i = 0; i < _size; i++)
    _bin[i] = 0;

  _wMin = 0;
  _wMax = _size - 1;

  _minBin = 0;
  _maxBin = 0;
  _minVal = 0.0;
  _maxVal = 0.0;

  _mean = 0.0;
  _population = 0.0;
  _variance = 0.0;
  _sigma = 0.0;
}

// format is:
//   total-bin-count lowest-non-zero-bin-index highest-non-zero-bin-index
//   bin-count
//     :
//

int KTable::read(FILE *fp, int format)
{
  if (fp == NULL)
    return 1;

  if (format == 0 || format == 1 || format == 2 || format == 3)
  {
    // get number of bins
    int n = 0;
    if (fscanf(fp, "%d", &n) != 1)
      return 2;
    if (n < 1 || n > _size)
      return 3;
    _bins = n;

    // get minimum non-zero count index
    int minIndex = 0;
    if (fscanf(fp, "%d", &minIndex) != 1)
      return 4;

    // get maximum non-zero count index
    int maxIndex = _bins - 1;
    if (fscanf(fp, "%d", &maxIndex) != 1)
      return 5;

    if (!(0 <= minIndex && minIndex <= maxIndex && maxIndex < _bins))
      return 6;

    // place zeroes in bins below and above designated non-zero area
    int i;
    for (i = 0; i < minIndex; i++)
      _bin[i] = 0;
    for (i = maxIndex + 1; i < _bins; i++)
      _bin[i] = 0;

    // read the (potentially) non-zero count area
    while (minIndex <= maxIndex)
    {
      int count = 1;

      // get value
      int data = 0;
      if (fscanf(fp, "%d", &data) != 1)
        return 7;

      // negative value signals a run
      if (data < 0)
      {
        count = -data;
        if (count > maxIndex - minIndex + 1)
          return 8;
        data = 0;
        if (fscanf(fp, "%d", &data) != 1)
          return 9;
      }

      // place value into bin array
      for (i = 0; i < count; i++)
        _bin[minIndex++] = data;
    }
  }
  else if (format == 4)
  {
    // aubin format

    // read the contents of band (bin count already established)
    for (int b = 0; b < _bins; b++)
    {
      if (fscanf(fp, "%d", &_bin[b]) != 1)
        return 2;
    }
  }
  else
    return 7;

  return 0;
}

int KTable::read(char *filename, int format)
{
  FILE *fp = NULL;

  // open file
  if (filename == NULL || *filename == '\0')
    return 1;
  if ((fp = fopen(filename, "r")) == NULL)
    return 2;

  // read data
  KTable::read(fp, format);

  // close file
  fclose(fp);

  return 0;
}

int KTable::write(FILE *fp, int format)
{
  if (fp == NULL)
    return 1;
  if (_bins == 0)
    return 2;

  //
  // support various formats (no binary dump yet...)
  //
  if (format == 0)
  {
    // dump full table (may be 99% zeros with 16-bit images!)

    // describe table array size and span of data
    fprintf(fp, "%d %d %d\n", _bins, 0, _bins-1);

    // describe counts
    for (int i = 0; i < _bins; i++)
      fprintf(fp, "%d\n", _bin[i]);
  }
  else if (format == 1)
  {
    // "compression" -- skip leading and trailing bins with zero count

    // locate region of non-zero counts
    int minIndex = 0;
    while (_bin[minIndex] == 0 && minIndex < (_bins - 1))
      minIndex++;

    int maxIndex = _bins - 1;
    while (_bin[maxIndex] == 0 && maxIndex > 0)
      maxIndex--;

    // is this an all zero table? if so, still write one zero
    if (maxIndex < minIndex)
    {
      minIndex = 0;
      maxIndex = 0;
    }

    // describe table size and location of non-zero counts
    fprintf(fp, "%d %d %d\n", _bins, minIndex, maxIndex);

    // describe counts
    for (int i = minIndex; i <= maxIndex; i++)
      fprintf(fp, "%d\n", _bin[i]);
  }
  else if (format == 2)
  {
    // "run length coding" -- compress two or more repeated values

    // describe table array size and span of data
    fprintf(fp, "%d %d %d\n", _bins, 0, _bins-1);

    for (int leftIndex = 0; leftIndex < _bins; )
    {
      // determine run length
      int rightIndex = leftIndex;
      while (rightIndex < _bins && _bin[leftIndex] == _bin[rightIndex])
        rightIndex++; // advances PAST end of run

      // describe counts
      if (rightIndex - leftIndex < 2)
        fprintf(fp, "%d\n", _bin[leftIndex]);
      else
        fprintf(fp, "%d %d\n", -(rightIndex - leftIndex), _bin[leftIndex]);

      leftIndex = rightIndex;
    }
  }
  else if (format == 3)
  {
    // "compression" and "run-length coding"

    // locate region of non-zero counts
    int minIndex = 0;
    while (_bin[minIndex] == 0 && minIndex < (_bins - 1))
      minIndex++;

    int maxIndex = _bins - 1;
    while (_bin[maxIndex] == 0 && maxIndex > 0)
      maxIndex--;

    // is this an all zero table? if so, still write one zero
    if (maxIndex < minIndex)
    {
      minIndex = 0;
      maxIndex = 0;
    }

    // describe table size and location of non-zero counts
    fprintf(fp, "%d %d %d\n", _bins, minIndex, maxIndex);

    // describe counts
    for (int leftIndex = minIndex; leftIndex <= maxIndex; )
    {
      // determine run length
      int rightIndex = leftIndex;
      while (rightIndex <= maxIndex && _bin[leftIndex] == _bin[rightIndex])
        rightIndex++; // advances PAST end of run

      // describe counts
      if (rightIndex - leftIndex < 2)
        fprintf(fp, "%d\n", _bin[leftIndex]);
      else
        fprintf(fp, "%d %d\n", -(rightIndex - leftIndex), _bin[leftIndex]);

      leftIndex = rightIndex;
    }
  }
  else if (format == 4)
  {
    // dump full table

    // don't describe number of bins -- already reported by KHistogram::write()

    // describe counts
    for (int i = 0; i < _bins; i++)
      fprintf(fp, "%d\n", _bin[i]);
  }
  else
    return 3;

  // success
  return 0;
}

int KTable::write(char *filename, int format)
{
  FILE *fp = NULL;

  // open file
  if (filename == NULL || *filename == '\0')
    return 1;
  if ((fp = fopen(filename, "w")) == NULL) // "write" rather than "append"
    return 2;

  // write data
  KTable::write(fp, format);

  // close file
  fclose(fp);

  return 0;
}

void KTable::accumulate(KTable &t)
{
  int i;
  _bins = t.bins();
  for (i = 0; i < _bins; i++)
  {
    _bin[i] += t.bin(i);
  }

}

// compute statistical data for the range [_wMin.._wMax]
void KTable::statistics()
{
  // find lowest non-zero bin's index and value
  for (_minBin = _wMin; _minBin <= _wMax && _bin[_minBin] == 0; _minBin++)
    ;
  if (_minBin > _wMax)
    _minBin = _wMin;
  _minVal = _bin[_minBin];

  // find highest non-zero bin's index and value
  for (_maxBin = _wMax; _maxBin >= _wMin && _bin[_maxBin] == 0; _maxBin--)
    ;
  if (_maxBin < _wMin)
    _maxBin = _wMin;
  _maxVal = _bin[_maxBin];

  _population = 0.0;
  int i;
  for (i = _minBin; i <= _maxBin; i++)
    _population += _bin[i];

  _mean = _population/double(_wMax - _wMin + 1);

  // population variance not sample variance
  double residual;
  double sumResidualSquared;
  for (i = _wMin, sumResidualSquared = 0.0; i <= _wMax; i++)
  {
    residual = double(_bin[i]) - _mean;
    sumResidualSquared += residual*residual;
  }
  _variance = sumResidualSquared/double(_wMax - _wMin + 1);
  _sigma = sqrt(_variance);

  //Mean = SUM[ bin center * bin value ] / SUM[ bin value ]
  //RMS = VARIANCE = SUM[ (bin center - mean)^2 * bin value ] / SUM[ bin value ]

  double weightedPopulation = 0;
  for ( i = _minBin; i <= _maxBin; i++)
    weightedPopulation += double(i)*double(_bin[i]);
  _meanBin = weightedPopulation/_population;

  for (i = _minBin, sumResidualSquared = 0.0; i <= _maxBin; i++)
  {
    residual = double(i) - _meanBin;
    sumResidualSquared += _bin[i]*residual*residual;
  }
  _varianceBin = sumResidualSquared/_population;
  _sigmaBin = sqrt(_varianceBin);
}

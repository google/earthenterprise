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

#ifndef __KTable_h__
#define __KTable_h__

//
// ktable.h -- the Keyhole data table class
//

#include <stdio.h>
#include <stdlib.h>

#define KTABLE_BIN_DEFAULT      65536

class KTable
{
 public:
  KTable(int size=KTABLE_BIN_DEFAULT);
  ~KTable();

  void reset();

  void setBins(int bins) {_bins = bins;}
  int getBins() {return _bins;}
  int &bins() {return _bins;}

  void setBin(int binIndex, int count) {_bin[binIndex] = count;}
  int getBin(int binIndex) {return _bin[binIndex];}
  int &bin(int binIndex) {return _bin[binIndex];}
  void load(unsigned char *v, int size)
  {
    if (size <= _size)
    {
      for (int i=0; i<size; i++)
        _bin[i] = v[i];
    }
  }
  void load(unsigned int *v, int size)
  {
    if (size <= _size)
    {
      for (int i=0; i<size; i++)
        _bin[i] = v[i];
    }
  }

  int &size() {return _size;}

  int *getBinPointer() {return _bin;}

  int read(FILE *fp, int format=3);
  int read(char *filename, int format=3);

  int write(FILE *fp, int format=3);
  int write(char *filename, int format=3);

  void accumulate(KTable &t);

  void statistics();

  int& windowMin() {return _wMin;}
  int& windowMax() {return _wMax;}

  const int& minBin() {return _minBin;}
  const int& maxBin() {return _maxBin;}
  const double& minVal() {return _minVal;}
  const double& maxVal() {return _maxVal;}

  const double& population() {return _population;}

  const double& mean() {return _mean;}
  const double& variance() {return _variance;}
  const double& sigma() {return _sigma;}

  const double& meanBin() {return _meanBin;}
  const double& varianceBin() {return _varianceBin;}
  const double& sigmaBin() {return _sigmaBin;}

 private:
  int _bins; // active size, likely 256 (2^8) or 65536 (2^16)
  int *_bin;
  int _size; // allocated size

  // statistical measures
  int _wMin; // statistics window: [_wMin .. _wMax]
  int _wMax;

  int _minBin;
  int _maxBin;
  double _minVal;
  double _maxVal;

  // data about bin contents
  double _population;
  double _mean;
  double _variance;
  double _sigma; // standard deviation

  // data about bin indices
  double _meanBin;
  double _varianceBin;
  double _sigmaBin; // standard deviation
};
#endif

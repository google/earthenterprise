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
#ifndef _HISTOGRAM_H_
#define _HISTOGRAM_H_

// histogram class
class Histogram {
 public:
  Histogram(int size = 65536) {
    _size = _sizeAlloc = size;
    _bin = (unsigned int *)calloc(_sizeAlloc * sizeof(unsigned int), 1);

    reset();
  }
  ~Histogram() { free(_bin); }
  void reset() {
    _count = 0;

    for (unsigned int i = 0; i < _size; i++) _bin[i] = 0;

    _skip = true;
    _skipCount = 1;
    _left = 0;
    _right = _size - 1;
  }

  unsigned int *getBinPointer() { return _bin; }

  void load(unsigned int *v) {
    _count = 0;
    for (unsigned int i = 0; i < _size; i++) {
      _bin[i] = v[i];
      _count += v[i];
    }
  }

  void accumulate(unsigned int *v) {
    for (unsigned int i = 0; i < _size; i++) {
      _bin[i] += v[i];
      _count += v[i];
    }
  }

  void setSkip(bool b) { _skip = b; }
  bool skip() { return _skip; }

  void setSkipCount(unsigned int count) { _skipCount = count; }
  unsigned int skipCount() { return _skipCount; }

  void setSize(unsigned size) {
    if (size <= _sizeAlloc) {
      _size = size;
      reset();
    }
  }
  int size() { return _size; }

  long count() { return _count; }

  unsigned int left() { return _left; }
  unsigned int right() { return _right; }

  unsigned int bin(unsigned int v) { return _bin[v]; }

  void add(unsigned int v) {
    _bin[v]++;
    _count++;
  }
  void add(unsigned char *d, int count) {
    for (int i = 0; i < count; i++) _bin[d[i]]++;
    _count += count;
  }
  void add(unsigned short *d, int count) {
    for (int i = 0; i < count; i++) _bin[d[i]]++;
    _count += count;
  }
  void remove(unsigned int v) {
    _bin[v]--;
    _count--;
  }

  void clip(double leftClipFactor, double rightClipFactor) {
    unsigned int adjustedTotal = _count;
    if (_skip) {
      unsigned int cursor;
      for (cursor = 0; cursor < _skipCount; cursor++) {
        adjustedTotal -= _bin[cursor];
        adjustedTotal -= _bin[_size - 1 - cursor];
      }
    }

    // clip left
    _left = (_skip) ? _skipCount : 0;  // skip "NO DATA" pixel value of 0
    unsigned int tail = (unsigned int)(leftClipFactor * adjustedTotal);
    while (_bin[_left] <= tail && _left < _size - 1) tail -= _bin[_left++];

    // clip right
    _right = (_skip) ? _size - 1 - _skipCount : _size - 1;
    tail = (unsigned int)(rightClipFactor * adjustedTotal);
    while (_bin[_right] <= tail && _right > 0) tail -= _bin[_right--];

    // handle problem case
    if (_right < _left) {
      _left = _skip ? 1 : 0;
      _right = _size - 1;
    }
  }

  int compute(GDALRasterBand *band) {
    int numBins;
    if (band->GetRasterDataType() == GDT_Byte)
      numBins = 256;
    else if (band->GetRasterDataType() == GDT_UInt16 ||
             band->GetRasterDataType() == GDT_Int16)
      numBins = 65536;
    else
      return 1;

    // positioning centers bin windows at truncation points
    double minBinValue = -0.5;
    double maxBinValue = numBins - 0.5;  //"numBins - 1 + 0.5"

    if (band->GetHistogram(minBinValue, maxBinValue, numBins, (GUIntBig*) _bin,
                           FALSE, FALSE, GDALDummyProgress, NULL) != CE_None)
      return 2;

    _count = band->GetXSize() * band->GetYSize();

    return 0;
  }

 private:
  unsigned int _size;       // active size
  unsigned int _sizeAlloc;  // size of array
  unsigned int _count;      // number of entries
  unsigned int *_bin;       // number of each entry

  bool _skip;               // skip bin 0 in clip calculations
  unsigned int _skipCount;  // number of bins to skip
  unsigned int _left;
  unsigned int _right;
};

#endif

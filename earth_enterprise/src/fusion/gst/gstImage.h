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

#ifndef KHSRC_FUSION_GST_GSTIMAGE_H__
#define KHSRC_FUSION_GST_GSTIMAGE_H__

#include <limits>
#include <gstTypes.h>

template<class Type>
class gstImageStats {
 public:
  gstImageStats(int n, const Type* buf, Type ignore = 0)
      : total_(0),
        ignore_(ignore) {
    // allocate space for histogram
    int sz = 0x01 << (8 * sizeof(Type));
    histogram_ = new uint64[sz];
    memset(histogram_, 0, sz);

    collectFromBuffer(n, buf);
  }

  ~gstImageStats() {
    delete [] histogram_;
  }

  Type getMin() const { return min_; }
  Type getMax() const { return max_; }
  uint64 getTotal() const { return total_; }

  uint64 getVal(Type n) const {
    return histogram_[bias(n)];
  }

 private:
  void collectFromBuffer(int n, const Type* buf) {
    min_ = std::numeric_limits<Type>::max();
    max_ = std::numeric_limits<Type>::min();

    for (int t = 0; t < n; ++t) {
      if (buf[t] == ignore_)
        continue;

      if (buf[t] < min_) {
        min_ = buf[t];
      } else if (buf[t] > max_) {
        max_ = buf[t];
      }

      histogram_[bias(buf[t])]++;
      ++total_;
    }
  }

  uint32 bias(Type v) const {
    static const uint32 shift =
      static_cast<uint32>(0 - std::numeric_limits<Type>::min());
    return v + shift;
  }

  Type min_;
  Type max_;
  uint64 total_;
  uint64* histogram_;
  Type ignore_;
};

template<class InType, class OutType>
class gstImageLut {
 public:
  gstImageLut()
      : empty(true),
        lutBuf(NULL) {
  }

  ~gstImageLut() {
    delete [] lutBuf;
  }

  OutType getVal(InType v) const {
    if (empty)
      return OutType(v);
    return lutBuf[bias(v)];
  }

  void setMinMax(const gstImageStats<InType>& stats) {
    empty = false;

    InType min = stats.getMin();
    InType max = stats.getMax();

    int sz = std::numeric_limits<InType>::max() -
             std::numeric_limits<InType>::min() + 1;
    lutBuf = new OutType[sz];

    double scale = double(std::numeric_limits<OutType>::max() -
                          std::numeric_limits<OutType>::min()) / (max - min);

    for (int t = min; t <= max; ++t) {
      lutBuf[bias(t)] = OutType(double(t - min) * scale);
    }
  }

  void setEqualize(const gstImageStats<InType>& stats) {
    empty = false;

    InType min = stats.getMin();
    InType max = stats.getMax();

    int sz = std::numeric_limits<InType>::max() -
             std::numeric_limits<InType>::min() + 1;
    lutBuf = new OutType[sz];

    double scale = double(std::numeric_limits<OutType>::max() -
                          std::numeric_limits<OutType>::min()) /
                   double(stats.getTotal());

    // compute equalize lut
    uint64 sum = 0;
    for (int t = min; t <= max; ++t) {
      sum += stats.getVal(t);
      lutBuf[bias(t)] = OutType((double(sum) * scale) + 0.5);
    }
  }

  void reset() {
    delete [] lutBuf;
    lutBuf = NULL;
    empty = true;
  }

  bool isEmpty() const { return empty; }

 private:
  bool empty;

  OutType *lutBuf;

  uint32 bias(InType v) const {
    static const uint32 shift =
      static_cast<uint32>(0 - std::numeric_limits<InType>::min());
    return v + shift;
  }
};

template<class OutType>
class gstImageLut<float, OutType> { };

template<class OutType>
class gstImageLut<uint32, OutType> { };

template<class OutType>
class gstImageLut<int32, OutType> { };

#endif  // !KHSRC_FUSION_GST_GSTIMAGE_H__

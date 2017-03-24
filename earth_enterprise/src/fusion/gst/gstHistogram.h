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

#ifndef KHSRC_FUSION_GST_GSTHISTOGRAM_H__
#define KHSRC_FUSION_GST_GSTHISTOGRAM_H__

#include <string.h>
#include <gstMisc.h>

class gstHistogram {
 public:
  gstHistogram(uint s)
      : size_(1 << log2up(s)), hash_mask_(size_ -1) {
    bins_ = new uint32[size_];
    clear();
  }
  ~gstHistogram() {
    delete [] bins_;
  }

  uint hash(uint id) const { return id & hash_mask_; }

  void clear() {
    memset(&bins_[0], 0, size_ * sizeof(uint32));
    max_ = 0;
  }

  uint32 operator[](int idx) { return bins_[idx]; }
  uint32 max() const { return max_; }
  int size() const { return size_; }

  void add(uint id) {
    uint idx = hash(id);
    bins_[idx]++;
    if (bins_[idx] > max_)
      max_ = bins_[idx];
  }

 private:
  uint32* bins_;
  uint32 max_;
  const uint size_;
  const uint hash_mask_;
};

#endif  // !KHSRC_FUSION_GST_GSTHISTOGRAM_H__

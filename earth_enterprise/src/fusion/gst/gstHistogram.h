/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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
  gstHistogram(unsigned int s)
      : size_(1 << log2up(s)), hash_mask_(size_ -1) {
    bins_ = new std::uint32_t[size_];
    clear();
  }
  ~gstHistogram() {
    delete [] bins_;
  }
  gstHistogram(const gstHistogram&) = delete;
  gstHistogram(gstHistogram&&) = delete;

  unsigned int hash(unsigned int id) const { return id & hash_mask_; }

  void clear() {
    memset(&bins_[0], 0, size_ * sizeof(std::uint32_t));
    max_ = 0;
  }

  std::uint32_t operator[](int idx) { return bins_[idx]; }
  std::uint32_t max() const { return max_; }
  int size() const { return size_; }

  void add(unsigned int id) {
    unsigned int idx = hash(id);
    bins_[idx]++;
    if (bins_[idx] > max_)
      max_ = bins_[idx];
  }

 private:
  std::uint32_t* bins_;
  std::uint32_t max_;
  const unsigned int size_;
  const unsigned int hash_mask_;
};

#endif  // !KHSRC_FUSION_GST_GSTHISTOGRAM_H__

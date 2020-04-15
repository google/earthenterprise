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

//

#ifndef COMMON_GESEGMENTEDARRAY_H__
#define COMMON_GESEGMENTEDARRAY_H__

#include <common/base/macros.h>
#include <cstddef>
#include <algorithm>
#include <vector>
#include <assert.h>


template <class T, class Alloc = std::allocator<T> >
class geSegmentedArray : private Alloc {
  friend class geSegmentedArrayTest;

 public:
  typedef std::size_t size_type;


  // Segment size specified as log2 to ensure
  // segment num/ sub addr can be computed using bit shifts and masks
  geSegmentedArray(unsigned int SegmentSizeLog2_);
  ~geSegmentedArray(void);

  inline T& operator[](size_type index) {
    return segments[Segment(index)][Remainder(index)];
  }
  inline const T& operator[](size_type index) const {
    return segments[Segment(index)][Remainder(index)];
  }
  inline size_type size(void) const { return count; }

  void push_back(const T& v);
 private:
  inline size_type Segment(size_type index) const {
    return index >> kSegmentSizeLog2;
  }
  inline size_type Remainder(size_type index) const {
    return index & kSegmentRemainderMask;
  }

  class DeallocGuard {
    geSegmentedArray *sa;
    T *t;
   public:
    DeallocGuard(geSegmentedArray *sa_, T* t_) : sa(sa_), t(t_) { }
    ~DeallocGuard(void) {
      if (t) {
        sa->deallocate(t, sa->kSegmentSize);
      }
    }
    T* take(void) {
      T* steal = t;
      t = 0;
      return steal;
    }
  };
  friend class DeallocGuard;

  const unsigned int kSegmentSizeLog2;
  const size_type kSegmentSize;
  const unsigned int kSegmentRemainderMask;

  std::vector<T*> segments;
  size_type count;

  DISALLOW_COPY_AND_ASSIGN(geSegmentedArray);
};


template <class T, class Alloc>
geSegmentedArray<T, Alloc>::geSegmentedArray(unsigned int SegmentSizeLog2_) :
    kSegmentSizeLog2(SegmentSizeLog2_),
    kSegmentSize(1U<<kSegmentSizeLog2),
    kSegmentRemainderMask(kSegmentSize - 1),
    segments(),
    count(0)
{
  segments.reserve(10);
}

template <class T, class Alloc>
geSegmentedArray<T, Alloc>::~geSegmentedArray(void) {
  for (size_type s = 0; s < segments.size(); ++s) {
    size_type num = std::min(count, kSegmentSize);
    std::_Destroy(&segments[s][0], &segments[s][num],
                  *static_cast<Alloc*>(this));
    this->deallocate(segments[s], kSegmentSize);
    count -= num;
  }
}

template <class T, class Alloc>
void geSegmentedArray<T,Alloc>::push_back(const T& v) {
  // we can be at most one segment short
  size_type newIndex = count;
  assert(Segment(newIndex) <= segments.size());

  if (Segment(newIndex) == segments.size()) {
    DeallocGuard guard(this, this->allocate(kSegmentSize));
    segments.resize(segments.size()+1);
    segments[Segment(newIndex)] = guard.take();
  }

  // copy construct into place
  this->construct(&operator[](newIndex), v);
  ++count;
}

#endif // COMMON_GESEGMENTEDARRAY_H__

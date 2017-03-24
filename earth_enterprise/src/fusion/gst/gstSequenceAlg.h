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

// Contains different sequence operations (functions used on ranges of
// elements).
//
// Functions ShrinkToSizeUsingSwap(), ShrinkToRemaining(),
// ShrinkToRemainingUsingSwap(), RemoveAtIndices(), RemoveAtIndicesUsingSwap()
// are moved from gstGeode.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSEQUENCEALG_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSEQUENCEALG_H_

#include <cassert>
#include <cstddef>
#include <vector>

template<class Container, class Iterator>
void ShrinkToSizeUsingSwap(Container* list) {
  const size_t list_size = list->size();
  Container tmp(list_size);
  size_t i = 0;
  for (Iterator ii = list->begin(), jj = tmp.begin(); i < list_size;
       ++i, ++ii, ++jj) {
    (*ii).swap(*jj);  // Since swap is cheaper for khRefGuard
  }
  tmp.swap(*list);
}

// Erases i'th element only if keep[i] != keep_type; Keeps otherwise. Container
// can be vector, deque. Assumes container->size() == keep.size().
template<class Container, class Iterator, bool keep_type>
void ShrinkToRemaining(const std::vector<bool>& keep,
                       Container* container) {
  assert(container->size() == keep.size());
  const size_t num_elements = keep.size();
  // Update i to the first item to be erased.
  size_t i = 0;
  for (; i < num_elements && keep[i] == keep_type; ++i) {
  }
  if (i == num_elements) {  // Means nothing to erase.
    return;
  }
  Iterator ii = container->begin() + i;
  Iterator jj = ii;
  for (++i, ++ii; i < num_elements; ++i, ++ii) {
    if (keep[i] == keep_type) {
      *jj = *ii;
      ++jj;
    }
  }
  // This is the new size as we have written container->begin() to (jj - 1).
  container->resize(jj - container->begin());
}


// Erases i'th element only iff keep[i] != keep_type; Keeps otherwise. Container
// can be vector, deque. Assumes container->size() == keep.size().
// Use ShrinkToRemainingUsingSwap rather than ShrinkToRemaining iff
// (*container)[i].swap((*container)[j] is better than
// (*container)[i] = (*container)[j]; we don't care about (*container)[j].
template<class Container, class Iterator, bool keep_type>
void ShrinkToRemainingUsingSwap(const std::vector<bool>& keep,
                       Container* container) {
  assert(container->size() == keep.size());
  const size_t num_elements = keep.size();
  // Update i to the first item to be erased.
  size_t i = 0;
  for (; i < num_elements && keep[i] == keep_type; ++i) {
  }
  if (i == num_elements) {  // Means nothing to erase.
    return;
  }
  Iterator ii = container->begin() + i;
  Iterator jj = ii;
  for (++i, ++ii; i < num_elements; ++i, ++ii) {
    if (keep[i] == keep_type) {
      (*jj).swap(*ii);
      ++jj;
    }
  }
  // This is the new size as we have written container->begin() to (jj - 1).
  container->resize(jj - container->begin());
}


// Removes the set of indices pointed to by remove which is sorted and without
// duplicates. RemoveContainerType can be any forward iterable class.
// Container can be either vector or queue. Assumes that remove is not empty.
template<class RemoveContainerType, class RemoveContainerConstIterator,
         class Container, class ContainerIterator>
void RemoveAtIndices(const RemoveContainerType& remove, Container* container) {
  assert(!remove.empty());
  RemoveContainerConstIterator iter = remove.begin();
  ContainerIterator ii = container->begin() + *(iter);
  ContainerIterator jj = ii;
  for (++iter; ; ++iter) {
    const bool is_remove_end = (iter == remove.end());
    ContainerIterator const ii_end = (is_remove_end ? container->end()
                                        : container->begin() + *(iter));
    for (++ii; ii < ii_end; ++ii) {
      *jj = *ii;
      ++jj;
    }
    if (is_remove_end) {
      break;
    }
  }
  // This is the new size as we have written container->begin() to (jj - 1).
  container->resize(jj - container->begin());
}


// Remove's the set of indices pointed to by remove which is sorted and without
// duplicates. RemoveContainerType can be any forward iterable class.
// Container can be either vector or queue. Assumes that remove is not empty.
// Use RemoveAtIndicesUsingSwap rather than RemoveAtIndices iff
// (*container)[i].swap((*container)[j] is better than
// (*container)[i] = (*container)[j]; we don't care about (*container)[j].
template<class RemoveContainerType, class RemoveContainerConstIterator,
         class Container, class ContainerIterator>
void RemoveAtIndicesUsingSwap(const RemoveContainerType& remove,
                              Container* container) {
  assert(!remove.empty());
  RemoveContainerConstIterator iter = remove.begin();
  ContainerIterator ii = container->begin() + *(iter);
  ContainerIterator jj = ii;
  for (++iter; ; ++iter) {
    const bool is_remove_end = (iter == remove.end());
    ContainerIterator const ii_end = (is_remove_end ? container->end()
                                        : container->begin() + *(iter));
    for (++ii; ii < ii_end; ++ii) {
      (*jj).swap(*ii);
      ++jj;
    }
    if (is_remove_end) {
      break;
    }
  }
  // This is the new size as we have written container->begin() to (jj - 1).
  container->resize(jj - container->begin());
}


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSEQUENCEALG_H_

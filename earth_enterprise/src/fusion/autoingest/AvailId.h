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

/******************************************************************************
File:        AvailId.h

Description: class to hand out IDs from possibly disjoint ranges. This will
             throw a std::out_of_range error if next() is called when all the
             ID's have been consumed

-------------------------------------------------------------------------------
******************************************************************************/
#ifndef __AvailId_h
#define __AvailId_h

#include <vector>
#include <khException.h>
#include <assert.h>

// #define DEBUG_AVAILID

#ifdef DEBUG_AVAILID
#include <stdio.h>
#endif

class AvailId {
  typedef std::pair<std::uint32_t, std::uint32_t> Range;

  std::uint32_t size_;
  std::vector<Range> avail;
 public:
#ifdef DEBUG_AVAILID
  // debug routine
  void dump(void) {
    for (std::vector<Range>::const_iterator i = avail.begin();
         i != avail.end(); ++i) {
      printf("[%u,%u[\n", i->first, i->second);
    }
    printf("avail = %u\n", size_);
  }
#endif


  // Note: like the STL classes, the range is [begin,end[
  AvailId(std::uint32_t begin, std::uint32_t end);
  std::uint32_t size(void) const { return size_; }
  std::uint32_t next(void);
  void exclude(std::uint32_t id, bool throwException=true);
};

#endif /* __AvailId_h */

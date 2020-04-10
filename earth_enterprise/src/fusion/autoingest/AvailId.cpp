// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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


#include "AvailId.h"

// Note: like the STL classes, the range is [begin,end[
AvailId::AvailId(std::uint32_t begin, std::uint32_t end) {
  if (begin > end)
    throw khException(kh::tr("AvailId: Internal error: Bad range"));
  size_ = end - begin;
  if (size_) {
    avail.push_back(std::make_pair(begin, end));
  }
#ifdef DEBUG_AVAILID
  printf("---------- construct ----------\n");
  dump();
#endif
}

 std::uint32_t AvailId::next(void) {
  if (!size_)
    throw khException(kh::tr("AvailId: All IDs exhausted"));
  assert(avail.back().first < avail.back().second);

  // get the next ID
  std::uint32_t next = avail.back().first++;
  --size_;

  // see if the range on the back is empty
  if (avail.back().first == avail.back().second) {
    // this range is now empty, discard it
    avail.pop_back();
  }

  // if my size_ counter is empty my list should be also
  // and visa-versa
  assert(bool(size_) == bool(avail.size()));

  return next;
}

void AvailId::exclude(std::uint32_t id, bool throwException) {
  for (std::vector<Range>::iterator range = avail.begin();
       range != avail.end(); ++range) {
    if (id == range->first) {
      if (++range->first == range->second) {
        // We consumed the last id in this range. Get rid of it.
        avail.erase(range);
      }
      --size_;
#ifdef DEBUG_AVAILID
      printf("---------- exclude %u ----------\n", id);
      dump();
#endif
      return;
    } else if (id > range->first) {
      if (id < range->second) {
        Range newrange(id+1, range->second);
        range->second = id;
        if (newrange.first < newrange.second) {
          // insert puts the new item before the given iterator,
          // we wanr it after range, so pass range+1
          avail.insert(range+1, newrange);
        }
        --size_;
#ifdef DEBUG_AVAILID
        printf("---------- exclude %u ----------\n", id);
        dump();
#endif
        return;
      } else {
        // we haven't gone far enough yet
      }
    } else {
      // we've gone too far
      break;
    }
  }
  // throwException will be false only if the ExpertChannelIds are assigned
  if ( throwException ) {
    throw khException(kh::tr("AvailId: Internal Error: Tried to exclude %1, which is no longer available")
                      .arg(id));
  }
}

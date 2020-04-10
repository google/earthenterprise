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

#ifndef COMMON_GEMULTIRANGE_H__
#define COMMON_GEMULTIRANGE_H__

#include <vector>
#include <geRange.h>
#include <khstrconv.h>

// end is "one beyond"
template <class T>
class geMultiRange {
  class Range {
   public:
    T begin;
    T end;

    inline Range(T b, T e) : begin(b), end(e) {
      // do degenerate or malformed ranges allowed
      assert(begin < end);
    }
  };

  std::vector<Range> ranges;

 public:
  geMultiRange(void) : ranges() { }
  geMultiRange(T begin, T end) : ranges(1, Range(begin, end)) { }
  geMultiRange(const geRange<T> &range) : ranges(1, Range(range.min,
                                                          range.max+1)) { }

  bool operator==(const geMultiRange &o) const {
    if (ranges.size() == o.ranges.size()) {
      for (unsigned int i = 0; i < ranges.size(); ++i) {
        if ((ranges[i].begin != o.ranges[i].begin) ||
            (ranges[i].end != o.ranges[i].end)) {
          return false;
        }
      }
      return true;
    } else {
      return false;
    }
  }

  std::string AsString(void) {
    std::ostringstream out;
    for (unsigned int i = 0; i < ranges.size(); ++i) {
      if (i > 0) {
        out << ", ";
      }
      out << "[" << ToString(ranges[i].begin) << "," <<
        ToString(ranges[i].end) << ")";
    }
    return out.str();
  }

  bool Contains(T t) const {
    for (typename std::vector<Range>::const_iterator r = ranges.begin();
         r != ranges.end(); ++r) {
      if (t < r->begin) {
        return false;
      } else if (t < r->end) {
        return true;
      }
    }
    return false;
  }

  static geMultiRange Union(const geMultiRange &a, const geMultiRange &b) {
    geMultiRange ret;

    typename std::vector<Range>::const_iterator ai = a.ranges.begin();
    typename std::vector<Range>::const_iterator bi = b.ranges.begin();

    // while there are ranges left in both
    while ((ai != a.ranges.end()) && (bi != b.ranges.end())) {
      if (ai->begin < bi->begin) {
        ret.AppendRange_(*ai);
        ++ai;
      } else {
        ret.AppendRange_(*bi);
        ++bi;
      }
    }

    // while there are ranges left in a
    while (ai != a.ranges.end()) {
      ret.AppendRange_(*ai);
      ++ai;
    }

    // while there are ranges left in b
    while (bi != b.ranges.end()) {
      ret.AppendRange_(*bi);
      ++bi;
    }

    return ret;
  }

 private:
  void AppendRange_(const Range &r) {
    if (!ranges.size()) {
      // first range
      ranges.push_back(r);
    } else {
      Range *lastRange = &ranges[ranges.size()-1];
      // make sure this really is an append operation
      assert(r.begin >= lastRange->begin);

      if (r.begin > lastRange->end) {
        // disjoint
        ranges.push_back(r);
      } else if (r.begin == lastRange->end) {
        // perfectly abutting
        lastRange->end = r.end;
      } else {
        // overlapping
        lastRange->end = std::max(lastRange->end, r.end);
      }
    }
  }
};

template <class T>
std::string
ToString(const geMultiRange<T> &value)
{
  return value.AsAstring();
}





#endif // COMMON_GEMULTIRANGE_H__

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
File:        khExtents.h
Description: This is the little sister to gstBBox4D.
             It is lighter weight but less functional.

-------------------------------------------------------------------------------
******************************************************************************/
#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_KHEXTENTS_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_KHEXTENTS_H_

/******************************************************************************
 ***  I want to eventually merge this class back into gstBBox4D but too many
 ***  places use gstBBox4D to change it at this stage in the game.
 ******************************************************************************/

/******************************************************************************
 ***  Motivations for this class:
 ***
 ***  1) I need a class that works for pixel extents (with implied width).
 ***  gstBBox4D defines width() as (e - w) implying that e is "one beyond"
 ***  the last valid pixel. But gstBBox4D defines intersect() with >= which
 ***  implies that e _is_ the last valid pixel. I think gstBBox4D's
 ***  implementation of intersect() is broken, but again, too many places
 ***  use it to change it just before a major release.
 ***
 ***  2) I want a class with "always valid" semantics.
 ******************************************************************************/

#include <assert.h>
#include <cmath>
#include <deque>
#include <limits>
#include <algorithm>
#include <vector>

#include "common/khTypes.h"
#include <cstdint>

template <class T>
class khExtents {
 private:
  T beginX_, endX_;
  T beginY_, endY_;

 public:
  // empty parens below tells compiler to initialize with whatever the
  // type considers a 'zero' value. -----------v
  khExtents(void) : beginX_(), endX_(), beginY_(), endY_() { }

  khExtents(const khOffset<T> &origin,
            const khSize<T> &size)
      : beginX_(origin.x()), endX_(origin.x()+size.width),
        beginY_(origin.y()), endY_(origin.y()+size.height) { }

  khExtents(CoordOrder o, T a, T b, T c, T d) {
    switch (o) {
      case XYOrder:
        beginX_ = a;  /* beginX */
        endX_   = b;  /* endX   */
        beginY_ = c;  /* beginY */
        endY_   = d;  /* endY   */
        break;
      case RowColOrder:
        beginY_ = a;  /* beginRow */
        endY_   = b;  /* endRow   */
        beginX_ = c;  /* beginCol */
        endX_   = d;  /* endCol   */
        break;
      case NSEWOrder:
        endY_   = a;  /* north */
        beginY_ = b;  /* south */
        endX_   = c;  /* east  */
        beginX_ = d;  /* west  */
        break;
    }
    if ((endX_ < beginX_) || (endY_ < beginY_)) {
      // invalid, set to empty
      *this = khExtents();
    }
  }

  bool operator==(const khExtents &o) const {
    return (beginX_ == o.beginX_ &&
            endX_ == o.endX_ &&
            beginY_ == o.beginY_ &&
            endY_ == o.endY_);
  }
  bool operator!=(const khExtents &o) const {
    return (!operator==(o));
  }


  inline T beginX(void) const { return beginX_;}
  inline T   endX(void) const { return   endX_;}
  inline T beginY(void) const { return beginY_;}
  inline T   endY(void) const { return   endY_;}

  inline T north(void) const { return endY(); }
  inline T south(void) const { return beginY(); }
  inline T east(void)  const { return endX(); }
  inline T west(void)  const { return beginX(); }

  inline T beginRow(void) const { return beginY(); }
  inline T endRow(void)   const { return endY(); }
  inline T beginCol(void) const { return beginX(); }
  inline T endCol(void)   const { return endX(); }

  inline T    width(void) const { return endX_ - beginX_; }
  inline T   height(void) const { return endY_ - beginY_; }
  inline T  numRows(void) const { return endY_ - beginY_; }
  inline T  numCols(void) const { return endX_ - beginX_; }
  inline bool degenerate(void) const {
    return (width() == T()) || (height() == T());
  }
  inline bool empty(void) const;

  inline khOffset<T> origin(void) const {
    return khOffset<T>(XYOrder, beginX_, beginY_);
  }
  inline khSize<T> size(void) const {
    return khSize<T>(endX_-beginX_, endY_-beginY_);
  }
  // connects determines if two bounding boxes intersect or are immediately
  // adjacent.
  inline bool connects(const khExtents &o) const;
  // intersects determines if two bounding boxes intersect.
  inline bool intersects(const khExtents &o) const;
  inline bool ContainsRow(T row) const;
  inline bool ContainsCol(T col) const;
  inline bool ContainsRowCol(T row, T col) const {
    return (ContainsRow(row) && ContainsCol(col));
  }
  inline bool ContainsXY(T x, T y) const {
    return ContainsRowCol(y, x);
  }
  bool contains(const khExtents &o) const {
    return ((beginX_ <= o.beginX_) && (endX_   >= o.endX_) &&
            (beginY_ <= o.beginY_) && (endY_   >= o.endY_));
  }

  void grow(const khExtents &o) {
    if (o.empty()) {
      return;
    } else if (empty()) {
      *this = o;
    } else {
      if (o.beginX_ < beginX_) beginX_ = o.beginX_;
      if (o.beginY_ < beginY_) beginY_ = o.beginY_;
      if (o.endX_ > endX_) endX_ = o.endX_;
      if (o.endY_ > endY_) endY_ = o.endY_;
    }
  }

  void expandBy(T t) {
    if (beginX_ - std::numeric_limits<T>::min() >= t)
      beginX_ -= t;
    else
      beginX_ = std::numeric_limits<T>::min();
    if (std::numeric_limits<T>::max() - endX_ >= t)
      endX_ += t;
    else
      endX_ = std::numeric_limits<T>::max();
    if (beginY_ - std::numeric_limits<T>::min() >= t)
      beginY_ -= t;
    else
      beginY_ = std::numeric_limits<T>::min();
    if (std::numeric_limits<T>::max() - endY_ >= t)
      endY_ += t;
    else
      endY_ = std::numeric_limits<T>::max();
  }

  void narrowBy(T t) {
    if (std::numeric_limits<T>::max() - beginX_ >= t)
      beginX_ += t;
    else
      beginX_ = std::numeric_limits<T>::max();

    if (endX_ - std::numeric_limits<T>::min() >= t)
      endX_ -= t;
    else
      endX_ = std::numeric_limits<T>::min();

    if (std::numeric_limits<T>::max() - beginY_ >= t)
      beginY_ += t;
    else
      beginY_ = std::numeric_limits<T>::max();

    if (endY_ - std::numeric_limits<T>::min() >= t)
      endY_ -= t;
    else
      endY_ = std::numeric_limits<T>::min();

    if ((endX_ < beginX_) || (endY_ < beginY_)) {
      // invalid, set to empty
      *this = khExtents();
    }
  }

  void alignBy(T t) {
    // Note: These operations will all preserve empty extents (all 0's)

    beginX_ -= beginX_ % t;
    endX_   += t - 1;
    endX_   -= endX_ % t;

    beginY_ -= beginY_ % t;
    endY_   += t - 1;
    endY_   -= endY_ % t;
  }

  // assumes you know what you are doing
  void makeRelativeTo(const khOffset<T> &origin) {
    if (!empty()) {
      beginX_ -= origin.x();
      endX_   -= origin.x();
      beginY_ -= origin.y();
      endY_   -= origin.y();
    }
  }


  // Note: Function is used for extents specified by row/col.
  // The extents in degree can be different for some insets but at the same
  // time for these insets we can get equal extents in "rows/columns".
  // To exclude skipping of these insets (with extents that are equal in
  // "row/col" but not equal in degree) we narrow by 1 (exclude borders) the
  // subtrahend-extents before calculate subtraction.
  // All borders are equivalent.
  template <class Iter>
  bool CoveredBy(Iter begin, Iter end,
                 const khExtents<double> degreeExtents) const {
    std::deque<khExtents> targets;
    targets.push_back(*this);
    for (Iter o = begin; o != end; ++o) {
      unsigned int numTargets = targets.size();
      if (numTargets == 0)
        break;

      // Narrow the subtrahend by 1 in order to exclude boundary.
      khExtents subtr = *o;
      subtr.narrowBy(1);
      if (subtr.empty()) {
        if (*this == *o && degreeExtents == *(o.degreeExtents())) {
          // This should be rare, otherwise could be made more efficient.
          // We return and continue checking higher levels even though we
          // know those will be covered.
          // TODO: Investigate efficiency of degree extents comparison.
          return true;
        }
        continue;
      }

      bool check_identical = true;
      for (unsigned int t = 0; t < numTargets; ++t) {
        khExtents target = targets.front();
        targets.pop_front();
        if (khExtents::Subtract(target, subtr, back_inserter(targets))) {
          // They intersected, any remainders were pushed onto
          // the back of targets.
          // Now check if the analyzed inset and the current subtrahend inset
          // have identical bounds.
          if (check_identical && *this == *o &&
              degreeExtents == *(o.degreeExtents())) {
            // This should be rare, otherwise could be made more efficient.
            // We return and continue checking higher levels even though we
            // know those will be covered.
            return true;
          }
          // Just check for identical assets at most once per iteration.
          check_identical = false;

        } else {
          // There was no intersection, add back in the whole target.
          // Put it on the back so I can look at the rest of them
          targets.push_back(target);
        }
      }
    }
    return targets.empty();
  }

  static inline khExtents Intersection(const khExtents &a, const khExtents &b);

  template <class InsertIter>
  static bool Subtract(const khExtents &a, const khExtents &b,
                       InsertIter remainder) {
    khExtents inter = Intersection(a, b);
    if (inter.empty()) {
      return false;
    } else {
      // Check each of the four possible remainders to see if it exists.
      khExtents left(XYOrder,
                     a.beginX(), inter.beginX(),
                     a.beginY(), a.endY());
      if (!left.degenerate()) {
        *remainder++ = left;
      }
      khExtents right(XYOrder,
                      inter.endX(), a.endX(),
                      a.beginY(), a.endY());
      if (!right.degenerate()) {
        *remainder++ = right;
      }
      khExtents top(XYOrder,
                    inter.beginX(), inter.endX(),
                    inter.endY(), a.endY());
      if (!top.degenerate()) {
        *remainder++ = top;
      }
      khExtents bottom(XYOrder,
                       inter.beginX(), inter.endX(),
                       a.beginY(), inter.beginY());
      if (!bottom.degenerate()) {
        *remainder++ = bottom;
      }
      return true;
    }
  }
};


template <class T>
inline bool khExtents<T>::empty(void) const {
  return degenerate();
}
template <>
inline bool khExtents<double>::empty(void) const {
  return ((beginX_ == double()) &&
          (endX_   == double()) &&
          (beginY_ == double()) &&
          (endY_   == double()));
}

// Determines if two bounding boxes intersect or are immediately adjacent.
template <class T>
inline bool khExtents<T>::connects(const khExtents &o) const {
  return ((std::min(endX_, o.endX_) >= std::max(beginX_, o.beginX_)) &&
          (std::min(endY_, o.endY_) >= std::max(beginY_, o.beginY_)));
}

// Note: declared to generate compilation error. Use a tolerance when
// implementing.
template <>
inline bool khExtents<double>::connects(const khExtents &o) const;

// Determines if two bounding boxes intersect.
template <class T>
inline bool khExtents<T>::intersects(const khExtents &o) const {
  return ((std::min(endX_, o.endX_) > std::max(beginX_, o.beginX_)) &&
          (std::min(endY_, o.endY_) > std::max(beginY_, o.beginY_)));
}
template <>
inline bool khExtents<double>::intersects(const khExtents &o) const {
  return ((std::min(endX_, o.endX_) >= std::max(beginX_, o.beginX_)) &&
          (std::min(endY_, o.endY_) >= std::max(beginY_, o.beginY_)));
}

template <class T>
inline bool  khExtents<T>::ContainsRow(T row) const {
  return ((row >= beginRow()) && (row < endRow()));
}
template <>
inline bool  khExtents<double>::ContainsRow(double row) const {
  return ((row >= beginRow()) && (row <= endRow()));
}

template <class T>
inline bool  khExtents<T>::ContainsCol(T col) const {
  return ((col >= beginCol()) && (col < endCol()));
}
template <>
inline bool  khExtents<double>::ContainsCol(double col) const {
  return ((col >= beginCol()) && (col <= endCol()));
}

template <class T>
inline khExtents<T> khExtents<T>::Intersection(
    const khExtents &a, const khExtents &b) {
  T new_beginX_ = std::max(a.beginX_, b.beginX_);
  T new_endX_   = std::min(a.endX_, b.endX_);
  T new_beginY_ = std::max(a.beginY_, b.beginY_);
  T new_endY_   = std::min(a.endY_, b.endY_);
  if ((new_endX_ < new_beginX_) && (new_endY_ < new_beginY_)) {
    return khExtents();
  } else {
    return khExtents(XYOrder, new_beginX_, new_endX_,
                     new_beginY_, new_endY_);
  }
}
template <>
inline khExtents<double> khExtents<double>::Intersection(
    const khExtents &a, const khExtents &b) {
  double new_beginX_ = std::max(a.beginX_, b.beginX_);
  double new_endX_   = std::min(a.endX_, b.endX_);
  double new_beginY_ = std::max(a.beginY_, b.beginY_);
  double new_endY_   = std::min(a.endY_, b.endY_);
  if ((new_endX_ >= new_beginX_) && (new_endY_ >= new_beginY_)) {
    return khExtents(XYOrder, new_beginX_, new_endX_,
                     new_beginY_, new_endY_);
  } else {
    return khExtents();
  }
}

class khCutExtent {
 public:
  static khExtents<double> cut_extent;
  static const khExtents<double> world_extent;
  static void ConvertFromFlatToMercator(khExtents<double>* extent);
  static khExtents<double> ConvertFromFlatToMercator(
      const khExtents<double>& deg_extent);
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_KHEXTENTS_H_

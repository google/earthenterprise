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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTBBOX_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTBBOX_H_

#include <cmath>
#include "fusion/gst/gstVertex.h"
#include "fusion/gst/gstMathUtils.h"
#include "fusion/gst/gstMisc.h"

// General purpose bounding box, tailored for
// geospatial applications


template<class Type> class gstBBox4D {
 public:
  // WARNING: setting these directly doesn't set the valid flag!
  Type n, s, e, w;

  // I just cant think in n,s,e,w
  Type minX(void) const { return w;}
  Type minY(void) const { return s;}
  Type maxX(void) const { return e;}
  Type maxY(void) const { return n;}

  enum GeoParams {
    Absolute,
    Relative,
    NormCoords,
    ScrnCoords
  };

  // Constructors
  gstBBox4D() {
    init(Type(0));
    valid_ = false;     // invalidate here since init() above makes it valid
  }
  explicit gstBBox4D(Type v) : valid_(false) {
    init(v);
  }
  // order follows glOrtho...
  gstBBox4D(Type w, Type e, Type s, Type n) : valid_(false) {
    init(w, e, s, n);
  }
  gstBBox4D(const gstBBox4D& that) : valid_(false) {
    init(that);
  }

  // Initialization
  const gstBBox4D& init(Type v) {
    n = s = w = e = v;
    valid_ = true;
    return *this;
  }

  const gstBBox4D& init(Type W, Type E, Type S, Type N) {
    if (W < E) {
      w = W;
      e = E;
    } else {
      w = E;
      e = W;
    }
    if (S < N) {
      s = S;
      n = N;
    } else {
      s = N;
      n = S;
    }
    valid_ = true;
    return *this;
  }

  // gstBBox4D& init(const gstBBox4D* b) { return init(*b); }
  const gstBBox4D& init(const gstBBox4D& b) {
    n = b.n;
    s = b.s;
    w = b.w;
    e = b.e;
    valid_ = b.valid_;
    return *this;
  }

  // Comparison operators
  bool operator==(const gstBBox4D& b) const {
    return n == b.n && s == b.s && w == b.w && e == b.e;
  }
  bool operator!=(const gstBBox4D& b) const {
    return n != b.n || s != b.s || w != b.w || e != b.e;
  }

  // Copying
  gstBBox4D& operator=(const gstBBox4D& b) {
    init(b);
    return *this;
  }

  // Arithmetic operators
  gstBBox4D& operator*=(Type c) {
    w *= c;
    e *= c;
    n *= c;
    s *= c;
    return *this;
  }

  gstBBox4D& operator*=(const gstBBox4D& b) {
    w *= b.w;
    e *= b.e;
    n *= b.n;
    s *= b.s;
    return *this;
  }

  gstBBox4D& operator/=(Type q) {
    w /= q;
    e /= q;
    n /= q;
    s /= q;
    return *this;
  }

  gstBBox4D& operator/=(const gstBBox4D& b) {
    w /= b.w;
    e /= b.e;
    n /= b.n;
    s /= b.s;
    return *this;
  }

  gstBBox4D& operator+=(Type a) {
    w += a;
    e += a;
    n += a;
    s += a;
    return *this;
  }

  gstBBox4D& operator+=(const gstBBox4D& b) {
    w += b.w;
    e += b.e;
    n += b.n;
    s += b.s;
    return *this;
  }

  gstBBox4D& operator-=(Type a) {
    w -= a;
    e -= a;
    n -= a;
    s -= a;
    return *this;
  }

  gstBBox4D& operator-=(const gstBBox4D& b) {
    w -= b.w;
    e -= b.e;
    n -= b.n;
    s -= b.s;
    return *this;
  }

  // Snap out to grid
  gstBBox4D snapOut(const Type& grid) const {
    Type west = gstRoundDown(w, grid);
    Type east = gstRoundUp(e, grid);
    Type south = gstRoundDown(s, grid);
    Type north = gstRoundUp(n, grid);
    return gstBBox4D(west, east, south, north);
  }

  Type Width() const {
    return e > w ? e - w : w - e;
  }
  Type Height() const {
    return n > s ? n - s : s - n;
  }
  Type Aspect() const {
    return Width() / Height();
  }
  Type Diameter() const {
    return std::sqrt(Width() * Width() + Height() * Height());
  }

  bool Intersect(const gstBBox4D& b) const {
    return ((std::min(n, b.n) >= std::max(s, b.s)) &&
            (std::min(e, b.e) >= std::max(w, b.w)));
  }

  bool Contains(const gstBBox4D& b) const {
    return (b.w >= w) && (b.e <= e) && (b.s >= s) && (b.n <= n);
  }

  bool Contains(const gstVertex& p) const {
    return (p.x >= w) && (p.x <= e) && (p.y >= s) && (p.y <= n);
  }

 private:
  // Liang-Barsky parametric line-clipping algorithm
  bool ClipTest(Type p, Type q, Type* u1, Type* u2) const {
    Type r;
    if (p < .0) {
      r = q / p;
      if (r > *u2)
        return false;
      else if (r > *u1)
        *u1 = r;
    } else if (p > .0) {
      r = q / p;
      if (r < *u1)
        return false;
      else if (r < *u2)
        *u2 = r;
    } else if (q < .0) {
      return false;
    }
    return true;
  }

  // Liang-Barsky parametric line-clipping algorithm with fraction result.
  bool ClipTestFraction(Type p, Type q,
                        Type* u1, Type* u1_denom,
                        Type* u2, Type* u2_denom) const {
    Type r;
    if (p < .0) {
      r = q / p;
      if (r > (*u2)/(*u2_denom))
        return false;
      else if (r > (*u1)/(*u1_denom)) {
        *u1 = q;
        *u1_denom = p;
      }
    } else if (p > .0) {
      r = q / p;
      if (r < (*u1)/(*u1_denom))
        return false;
      else if (r < (*u2)/(*u2_denom)) {
        *u2 = q;
        *u2_denom = p;
      }
    } else if (q < .0) {
      return false;
    }
    return true;
  }

 public:
  bool Intersect(const gstVertex& p0, const gstVertex& p1) const {
    if (Contains(p0) || Contains(p1))
      return true;

    Type dx = p1.x - p0.x;
    Type dy = p1.y - p0.y;

    if (dx == 0 && dy == 0)
      return false;

    Type u1 = 0;
    Type u2 = 1;

    if (ClipTest(-dx, p0.x - w, &u1, &u2) &&
        ClipTest(dx, e - p0.x, &u1, &u2) &&
        ClipTest(-dy, p0.y - s, &u1, &u2) &&
        ClipTest(dy, n - p0.y, &u1, &u2))
      return true;

    return false;
  }

  // This function will cut a line segment if it crosses
  // one of the edges of the bounding box
  // There are three possible return values:
  // 0: no intersection
  // 1: completely contained
  // 2: segment was cut
  int ClipLine(gstVertex* p0, gstVertex* p1) const {
    if (Contains(*p0) && Contains(*p1))
      return 1;

    Type dx = p1->x - p0->x;
    Type dy = p1->y - p0->y;

    // TODO: use Equals().
    if (dx == .0 && dy == .0)
      return 0;

    Type u1 = 0.0;
    Type u2 = 1.0;

    if (ClipTest(-dx, p0->x - w, &u1, &u2) &&
        ClipTest(dx, e - p0->x, &u1, &u2)) {
      if (ClipTest(-dy, p0->y - s, &u1, &u2) &&
          ClipTest(dy, n - p0->y, &u1, &u2)) {
        if (u2 < 1.0) {
          p1->x = p0->x + u2 * dx;
          p1->y = p0->y + u2 * dy;
        }
        if (u1 > 0.0) {
          p0->x += u1 * dx;
          p0->y += u1 * dy;
        }
        return 2;
      }
    }
    return 0;
  }

  // This function will cut a line segment if it crosses
  // one of the edges of the bounding box
  // There are three possible return values:
  // 0: no intersection
  // 1: completely contained
  // 2: segment was cut
  // Note: we use fraction calculation and long double for more
  // accurate calculation of intersection point.
  int ClipLineAccurate(gstVertex* p0, gstVertex* p1) const {
    if (Contains(*p0) && Contains(*p1))
      return 1;

    Type dx = p1->x - p0->x;
    Type dy = p1->y - p0->y;

    // TODO: use Equals().
    if (dx == .0 && dy == .0)
      return 0;

    Type u1 = 0.0;
    Type u1_denom = 1.0;
    Type u2 = 1.0;
    Type u2_denom = 1.0;

    if (ClipTestFraction(-dx, p0->x - w, &u1, &u1_denom, &u2, &u2_denom) &&
        ClipTestFraction(dx, e - p0->x, &u1, &u1_denom, &u2, &u2_denom)) {
      if (ClipTestFraction(-dy, p0->y - s, &u1, &u1_denom, &u2, &u2_denom) &&
          ClipTestFraction(dy, n - p0->y, &u1, &u1_denom, &u2, &u2_denom)) {
        if (u2/u2_denom < 1.0) {
          long double ddx =
              (static_cast<long double>(u2) * static_cast<long double>(dx)) /
              static_cast<long double>(u2_denom);
          long double ddy =
              (static_cast<long double>(u2) * static_cast<long double>(dy)) /
              static_cast<long double>(u2_denom);
          p1->x = p0->x + ddx;
          p1->y = p0->y + ddy;
        }
        if (u1/u1_denom > 0.0) {
          long double ddx =
              (static_cast<long double>(u1) * static_cast<long double>(dx)) /
              static_cast<long double>(u1_denom);
          long double ddy =
              (static_cast<long double>(u1) * static_cast<long double>(dy)) /
              static_cast<long double>(u1_denom);
          p0->x += ddx;
          p0->y += ddy;
        }
        return 2;
      }
    }
    return 0;
  }

  void Grow(const gstBBox4D& b) {
    if (!b.Valid())
      return;
    if (!Valid()) {
      init(b);
      return;
    }
    if (b.n > n) n = b.n;
    if (b.s < s) s = b.s;
    if (b.e > e) e = b.e;
    if (b.w < w) w = b.w;
  }
  void Grow(const gstVertex& v) { Grow(v.x, v.y); }
  void Grow(const Type& x, const Type& y) {
    if (!Valid()) {
      init(x, x, y, y);
      return;
    }
    if (x < w) w = x;
    if (x > e) e = x;
    if (y < s) s = y;
    if (y > n) n = y;
  }
  void ExpandBy(const Type& delta) {
    w -= delta;
    e += delta;
    s -= delta;
    n += delta;
  }

  Type CenterX() const { return (e + w) * 0.5; }
  Type CenterY() const { return (n + s) * 0.5; }
  gstVertex Center() const { return gstVertex(CenterX(), CenterY()); }

  void Invalidate() { valid_ = false; }
  bool Valid() const { return valid_; }

  // ------------------------------------------------------------------------

  static gstBBox4D Intersection(const gstBBox4D& a, const gstBBox4D& b) {
    if (!a.Intersect(b))
      return empty;

    return gstBBox4D(std::max(a.w, b.w),
                     std::min(a.e, b.e),
                     std::max(a.s, b.s),
                     std::min(a.n, b.n));
  }

  static gstBBox4D empty;

 private:
  bool valid_;
};

template<class Type> gstBBox4D <Type> gstBBox4D<Type>::empty;

typedef gstBBox4D<double> gstBBox;

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTBBOX_H_

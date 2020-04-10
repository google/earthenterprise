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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTVERTEX_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTVERTEX_H_

#include <math.h>
#include <stdio.h>
#include <vector>
#include <deque>
#include <cstdint>

#include "common/khTypes.h"

template<typename Type>
class Vert3 {
 public:
  Type x;
  Type y;
  Type z;

  Vert3(Type ix, Type iy, Type iz = 0.0) : x(ix), y(iy), z(iz) {}
  Vert3(const Vert3& v) : x(v.x), y(v.y), z(v.z) {}
  Vert3() : x(), y(), z() {}

  void set(Type ix, Type iy, Type iz = 0.0) {
    x = ix;
    y = iy;
    z = iz;
  }

  int operator==(const Vert3& v) const {
    return (x == v.x && y == v.y && z == v.z);
  }

  int operator!=(const Vert3& v) const {
    return (x != v.x || y != v.y || z != v.z);
  }

  void operator+=(const Vert3& v) {
    x += v.x;
    y += v.y;
    z += v.z;
  }

  Vert3 operator-(const Vert3& v) const {
    return Vert3(x - v.x, y - v.y, z - v.z);
  }

  Vert3 operator+(const Vert3& v) const {
    return Vert3(x + v.x, y + v.y, z + v.z);
  }

  Type squaredLength() const {
    return ((x*x) + (y*y) + (z*z));
  }

  Type length() const {
    return sqrt(squaredLength());
  }

  bool operator<(const Vert3& v) const {
    if (x != v.x) return (x < v.x);
    if (y != v.y) return (y < v.y);
    return z < v.z;
  }

  // result < 0 means *this < v, result > 0 means *this > v,
  // result == 0 means *this == v
  Type Cmp(const Vert3& v) const {
    Type result = x - v.x;
    if (0 == result) {
      result = y - v.y;
      if (0 == result) {
        result = z - v.z;
      }
    }
    return result;
  }

  bool AlmostEqual(const Vert3& v, Type epsilon) const {
    Type xdif = fabs(x - v.x);
    Type ydif = fabs(y - v.y);
    Type zdif = fabs(z - v.z);
    return ((xdif <= epsilon) && (ydif <= epsilon) && (zdif <= epsilon));
  }

  Vert3& Normalize() {
    Type len = length();
    if (len != 0.0) {
      x /= len;
      y /= len;
      z /= len;
    }
    return *this;
  }

  Type Distance(const Vert3& v) const {
    return sqrt((x-v.x)*(x-v.x) + (y-v.y)*(y-v.y) + (z-v.z)*(z-v.z));
  }

  Type SquaredDistance(const Vert3& v) const {
    return (x-v.x)*(x-v.x) + (y-v.y)*(y-v.y) + (z-v.z)*(z-v.z);
  }

  Type Distance2D(const Vert3& v) const {
    return sqrt((x-v.x)*(x-v.x) + (y-v.y)*(y-v.y));
  }

  Type SquaredDistance2D(const Vert3& v) const {
    return (x-v.x)*(x-v.x) + (y-v.y)*(y-v.y);
  }

  //////////////////////////////////////////////////////////////////////////
  //
  // ignores the Z for everything below
  //
  Type Distance(const Vert3& a, const Vert3& b) const {
    Type BxAx = b.x - a.x;
    Type ByAy = b.y - a.y;
    Type PxAx = x - a.x;
    Type PyAy = y - a.y;
    Type BxPx = b.x - x;
    Type ByPy = b.y - y;

    Type t = (PxAx*BxAx) + (PyAy*ByAy);

    if (t <= 0)  // off left end of segment
      return ((PxAx*PxAx) + (PyAy*PyAy));

    t = (BxPx*BxAx) + (ByPy*ByAy);

    if (t <= 0)  // off right end of segment
      return ((BxPx*BxPx) + (ByPy*ByPy));

    Type a2 = (PyAy*BxAx) - (PxAx*ByAy);
    return ((a2 * a2) / (BxAx*BxAx + ByAy*ByAy));
  }

  Vert3 CrossVector(const Vert3& v) const {
    Vert3 vcross;
    vcross.x = (y * v.z) - (v.y * z);
    vcross.y = (v.x * z) - (x * v.z);
    vcross.z = (x * v.y) - (v.x * y);
    return vcross;
  }
};

// A 3d point using triplet of doubles for x, y and z Cartesian coordinates
typedef Vert3<double> gstVertex;
typedef std::vector<gstVertex> gstVertexVector;
typedef std::deque<gstVertex> VertexList;

// ----------------------------------------------------------------------------

template<typename Type>
class Vert2 {
 public:
  Type x;
  Type y;

  Vert2(Type ix, Type iy) : x(ix), y(iy) {}
  // Create a 2D vertex from a 3D vertex, taking the x,y coordinates.
  explicit Vert2(const gstVertex& vertex_3d)
    : x(vertex_3d.x), y(vertex_3d.y) {}
  Vert2() : x(), y() {}

  // Returns the vector sum of this vertex and v.
  Vert2 Add(const Vert2& v) const {
    return Vert2(x + v.x, y + v.y);
  }

  // Returns the vector difference of this vertex and v.
  Vert2 Subtract(const Vert2& v) const {
    return Vert2(x - v.x, y - v.y);
  }

  // Returns the scaled vector by multiplying all components of this by factor.
  Vert2 Scale(double factor) const  { return Vert2(factor * x, factor * y); }

  bool operator==(const Vert2& b) const {
    return (x == b.x && y == b.y);
  }

  bool operator!=(const Vert2& b) const {
    return (x != b.x || y != b.y);
  }

  Type SquaredLength() const {
    return ((x*x) + (y*y));
  }

  Type Length() const {
    return sqrt(SquaredLength());
  }

  Vert2& Normalize() {
    Type len = Length();
    if (len != 0.0) {
      x /= len;
      y /= len;
    }
    return *this;
  }

  // Return true if the vertex is (0.0, 0.0)
  bool IsZero() const {
    return ((0.0 == x) && (0.0 == y));
  }

  // Return the Dot Product of two coordinates (as vectors)
  double Dot(const Vert2& other) const {
    return (x * other.x + y * other.y);
  }

  // Return the Cross Product of this vertex coordinates with another vertex.
  Type Cross(const Vert2& other) const {
    return ((x * other.y) - (other.x * y));
  }

  static Vert2 Intersect(const Vert2& A, const Vert2& B,
                         const Vert2& C, const Vert2& D) {
    Type A1 = A.y - B.y;
    Type B1 = B.x - A.x;
    Type C1 = (A.x * B.y) - (B.x * A.y);

    Type A2 = C.y - D.y;
    Type B2 = D.x - C.x;
    Type C2 = (C.x * D.y) - (D.x * C.y);

    Type x = ((C2 * B1) - (C1 * B2)) / ((A1 * B2) - (A2 * B1));
    Type y = ((C2 * A1) - (C1 * A2)) / ((B1 * A2) - (B2 * A1));

    return Vert2(x, y);
  }

  bool AlmostEqual(const Vert2& v, Type epsilon) const {
    Type xdif = fabs(x - v.x);
    Type ydif = fabs(y - v.y);
    return ((xdif <= epsilon) && (ydif <= epsilon));
  }
};

typedef Vert2<double> gstVert2D;
typedef Vert2<std::int16_t> gstVert2S;

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTVERTEX_H_

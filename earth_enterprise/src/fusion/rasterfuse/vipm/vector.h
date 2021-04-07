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

#ifndef _ET_VECTOR_H
#define _ET_VECTOR_H

#include <math.h>
#include <assert.h>

//////////////////////////////////////////////////////////////////////////////
template <class T>
class etVec3 {
 public:
  T elem[3];

  etVec3()
  { elem[0] = elem[1] = elem[2] = 0; }

  etVec3(T v)
  { elem[0] = elem[1] = elem[2] = v; }

  etVec3(T v0, T v1, T v2)
  { elem[0] = v0; elem[1] = v1; elem[2] = v2; }

  inline T length2() {
    return (T)(elem[0]*elem[0]+elem[1]*elem[1]+elem[2]*elem[2]);
  }

  inline T length() {
    return (T)sqrt(length2());
  }

  inline void normalize() {
    T dst = length();
    if(dst > 0) {
      dst = 1.0 / dst;
      elem[0] *= dst; elem[1] *= dst; elem[2] *= dst;
    } else
    {
      elem[0] = elem[1] = elem[2] = 0;
    }
  }

  // Cross product
  void cross(const etVec3<T> &v1, const etVec3<T> &v2)
  {
    elem[0] = v1[1] * v2[2] - v1[2] * v2[1];
    elem[1] = v1[2] * v2[0] - v1[0] * v2[2];
    elem[2] = v1[0] * v2[1] - v1[1] * v2[0];
  }

  // Dot product
  T dot(const etVec3<T> &v)
  {
    return elem[0] * v[0] + elem[1] * v[1] + elem[2] * v[2];
  }

  T& operator[](int index) { return elem[index]; }
  const T operator[](int index) const { return elem[index]; }
};

typedef etVec3<float>  etVec3f;
typedef etVec3<double> etVec3d;


//////////////////////////////////////////////////////////////////////////////
template <class T>
class etFace3 {
 public:
  T elem[3];

  etFace3()
  { elem[0] = elem[1] = elem[2] = 0; }

  etFace3(T v)
  { elem[0] = elem[1] = elem[2] = v; }

  etFace3(T v0, T v1, T v2)
  { elem[0] = v0; elem[1] = v1; elem[2] = v2; }

  void set(T v)
  { elem[0] = elem[1] = elem[2] = v; }

  void set(const etFace3<T>& v)
  { elem[0] = v.elem[0]; elem[1] = v.elem[1]; elem[2] = v.elem[2]; }

  void set(T v0, T v1, T v2)
  { elem[0] = v0; elem[1] = v1; elem[2] = v2; }

  void get(etFace3<T>& v)
  { v.elem[0] = elem[0]; v.elem[1] = elem[1]; v.elem[2] = elem[2]; }

  T& operator[](int index) { return elem[index]; }
  const T operator[](int index) const { return elem[index]; }
};

typedef etFace3<int>   etFace3i;


//////////////////////////////////////////////////////////////////////////////
template <class T>
class etBoundingBox3 {
 public:
  T min[3];
  T max[3];

  etBoundingBox3() {
    reset();
  }

  void reset() {
    min[0] = min[1] = min[2] = (T)( 1E9);
    max[0] = max[1] = max[2] = (T)(-1E9);
  }

  void add(const T x, const T y, const T z) {
    if(x > max[0]) max[0] = x;
    if(x < min[0]) min[0] = x;
    if(y > max[1]) max[1] = y;
    if(y < min[1]) min[1] = y;
    if(z > max[2]) max[2] = z;
    if(z < min[2]) min[2] = z;
  }
};


//////////////////////////////////////////////////////////////////////////////
// The matrix definition is Row major
//
template <class T>
class etMat3
{
 public:
  T elem[3][3];

  // constructors
  //  Declare an empty constructor so C++ do not fill with 0 by default
  etMat3(){};

  // Copy constructor
  etMat3(const etMat3<T>& _m) { memcpy(elem, _m.elem, sizeof(elem)); };

  inline void setRow(const etVec3<T>& _v, int i) {
    assert(i<3);
    elem[i][0] = _v[0], elem[i][1] = _v[1], elem[i][2] = _v[2];
  }

  inline void setCol(const etVec3<T>& _v, int i) {
    assert(i<3);
    elem[0][i] = _v[0], elem[1][i] = _v[1], elem[2][i] = _v[2];
  }

  inline void transform(const etVec3<T> &in, etVec3<T> &out)
  {
    T tmp[3];

    tmp[0] = in[0] * elem[0][0] + in[1] * elem[0][1] +
             in[2] * elem[0][2];
    tmp[1] = in[0] * elem[1][0] + in[1] * elem[1][1] +
             in[2] * elem[1][2];
    tmp[2] = in[0] * elem[2][0] + in[1] * elem[2][1] +
             in[2] * elem[2][2];
    out[0] = tmp[0]; out[1] = tmp[1]; out[2] = tmp[2];
  }
};

typedef etMat3<double> etMat3d;

#endif

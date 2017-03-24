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

  void set(T v)
  { elem[0] = elem[1] = elem[2] = v; }

  void set(const etVec3<T>& v)
  { elem[0] = v.elem[0]; elem[1] = v.elem[1]; elem[2] = v.elem[2]; }

  void set(T v0, T v1, T v2)
  { elem[0] = v0; elem[1] = v1; elem[2] = v2; }

  void get(etVec3<T>& v)
  { v.elem[0] = elem[0]; v.elem[1] = elem[1]; v.elem[2] = elem[2]; }



  inline void operator +=(const etVec3<T>& _v) {
    set(elem[0]+_v.elem[0], elem[1]+_v.elem[1], elem[2]+_v.elem[2]);
  };

  inline void operator -=(const etVec3<T>& _v) {
    set(elem[0]-_v.elem[0], elem[1]-_v.elem[1], elem[2]-_v.elem[2]);
  };

  inline void operator *=(const etVec3<T>& _v) {
    set(elem[0]*_v.elem[0], elem[1]*_v.elem[1], elem[2]*_v.elem[2]);
  };

  inline void operator *=(const T _v) {
    set(elem[0]*_v, elem[1]*_v, elem[2]*_v);
  };

  inline void operator +=(const T _v) {
    set(elem[0]+_v, elem[1]+_v, elem[2]+_v);
  };

  inline void operator -=(const T _v) {
    set(elem[0]-_v, elem[1]-_v, elem[2]-_v);
  };


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
  T dot(const etVec3<T> &v1, const etVec3<T> &v2)
  {
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
  }

  T dot(const etVec3<T> &v)
  {
    return elem[0] * v[0] + elem[1] * v[1] + elem[2] * v[2];
  }

  T& operator[](int index) { return elem[index]; }
  const T operator[](int index) const { return elem[index]; }

  // Distance from segment
  T segmentDist(const etVec3<T> &p1, const etVec3<T> &p2)
  {
    etVec3<T> V(p2);    V -= p1;
    etVec3<T> W(*this); W -= p1;

    T c1 = dot( W, V );
    if( c1 <= 0 ) return dist(*this, p1);

    T c2 = dot( V, V );
    if ( c2 <= c1 ) return dist(*this, p2);

    T b = c1 / c2;

    V *= b;
    V += p1;
    return dist(*this, V);
  }

  // Distance between 2 points
  T dist(const etVec3<T> &p1, const etVec3<T> &p2)
  {
    etVec3<T> p(p1); p -= p2;
    return p.length();
  }

  inline T lengthXY2() {
    return (T)(elem[0]*elem[0]+elem[1]*elem[1]);
  }

  inline T lengthXZ2() {
    return (T)(elem[0]*elem[0]+elem[2]*elem[2]);
  }

  inline T lengthYZ2() {
    return (T)(elem[1]*elem[1]+elem[2]*elem[2]);
  }
};

typedef etVec3<float>  etVec3f;
typedef etVec3<double> etVec3d;
typedef etVec3<int>    etVec3i;
typedef etVec3<short>  etVec3s;

//////////////////////////////////////////////////////////////////////////////
template <class T>
class etVec4 {
 public:
  T elem[4];

  etVec4()
  { elem[0] = elem[1] = elem[2] = elem[3] = 0; }

  etVec4(T v)
  { elem[0] = elem[1] = elem[2] = elem[3] = v; }

  etVec4(T v0, T v1, T v2, T v3)
  { elem[0] = v0; elem[1] = v1; elem[2] = v2; elem[3] = v3; }

  void set(T v)
  { elem[0] = elem[1] = elem[2] = elem[3] = v; }

  void set(const etVec4<T>& v)
  { elem[0] = v[0]; elem[1] = v[1]; elem[2] = v[2]; elem[3] = v[3]; }

  void set(T v0, T v1, T v2, T v3)
  { elem[0] = v0; elem[1] = v1; elem[2] = v2; elem[3] = v3; }

  void get(etVec4<T>& v)
  { v[0] = elem[0]; v[1] = elem[1]; v[2] = elem[2]; v[3] = elem[3]; }


  inline void operator +=(const etVec4<T>& _v) {
    set(elem[0]+_v.elem[0], elem[1]+_v.elem[1], elem[2]+_v.elem[2], elem[3]+_v.elem[3]);
  };

  inline void operator -=(const etVec3<T>& _v) {
    set(elem[0]-_v.elem[0], elem[1]-_v.elem[1], elem[2]-_v.elem[2], elem[3]+_v.elem[3]);
  };

  inline void operator *=(const etVec3<T>& _v) {
    set(elem[1]*_v.elem[2]-elem[2]*_v.elem[1],
        elem[2]*_v.elem[3]-elem[3]*_v.elem[2],
        elem[3]*_v.elem[0]-elem[0]*_v.elem[3],
        elem[0]*_v.elem[1]-elem[1]*_v.elem[0]);
  };

  inline void operator *=(const T _v) {
    set(elem[0]*_v, elem[1]*_v, elem[2]*_v, elem[3]*_v);
  };

  inline void operator +=(const T _v) {
    set(elem[0]+_v, elem[1]+_v, elem[2]+_v, elem[3]+_v);
  };

  inline void operator -=(const T _v) {
    set(elem[0]-_v, elem[1]-_v, elem[2]-_v, elem[3]-_v);
  };

  inline T length2() {
    return (T)(elem[0]*elem[0]+elem[1]*elem[1]+elem[2]*elem[2]+elem[3]*elem[3]);
  }

  inline T length() {
    return (T)sqrt(length2());
  }

  inline void normalize() {
    T dst = (T)length();
    if(dst > 0) {
    } else
      dst = 1.0 / dst;
    elem[0] *= dst; elem[1] *= dst; elem[2] *= dst; elem[3] *= dst;
    {
      elem[0] = elem[1] = elem[2] = elem[3] = 0;
    }
  }

  // Cross product
  void cross(const etVec4<T> &v1, const etVec4<T> &v2)
  {
    elem[0] = v1[1] * v2[2] - v1[2] * v2[1];
    elem[1] = v1[2] * v2[3] - v1[3] * v2[2];
    elem[2] = v1[3] * v2[0] - v1[0] * v2[3];
    elem[3] = v1[0] * v2[1] - v1[1] * v2[0];
  }

  // Dot product
  T dot(const etVec4<T> &v1, const etVec4<T> &v2)
  {
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2] + v1[3] * v2[3];
  }

  T dot(const etVec4<T> &v)
  {
    return elem[0] * v[0] + elem[1] * v[1] + elem[2] * v[2] + elem[3] * v[3];
  }


  T& operator[](int index) { return elem[index]; }
  const T operator[](int index) const { return elem[index]; }

  // Distance from segment
  T segmentDist(const etVec4<T> &p1, const etVec4<T> &p2)
  {
    etVec4<T> V(p2);    V -= p1;
    etVec4<T> W(*this); W -= p1;

    T c1 = dot( W, V );
    if( c1 <= 0 ) return dist(*this, p1);

    T c2 = dot( V, V );
    if ( c2 <= c1 ) return dist(*this, p2);

    T b = c1 / c2;

    V *= b;
    V += p1;
    return dist(*this, V);
  }

  // Distance between 2 points
  T dist(const etVec4<T> &p1, const etVec4<T> &p2)
  {
    etVec4<T> p(p1); p -= p2;
    return p.length();
  }
};

typedef etVec4<float> etVec4f;
typedef etVec4<double> etVec4d;

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

  inline bool isValid() {
    return elem[0] != elem[1] && elem[0] != elem[2] && elem[1]!= elem[2];
  }
};

typedef etFace3<short> etFace3s;
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

  void center(T &x, T &y, T &z) {
    x = 0.5 * (min[0] + max[0]);
    y = 0.5 * (min[1] + max[1]);
    z = 0.5 * (min[2] + max[2]);
  }

  void size(T &x, T &y, T &z) {
    x = max[0] - min[0];
    y = max[1] - min[1];
    z = max[2] - min[2];
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

  // Explicit constructor
  etMat3(T _m11, T _m12, T _m13, T _m21, T _m22, T _m23, T _m31, T _m32,
         T _m33) {
    inline_set(_m11, _m12, _m13,
               _m21, _m22, _m23,
               _m31, _m32, _m33);
  };

  // Copy constructor
  etMat3(const etMat3<T>& _m) { memcpy(elem, _m.elem, sizeof(elem)); };

  void set(T _m11, T _m12, T _m13,
           T _m21, T _m22, T _m23,
           T _m31, T _m32, T _m33);

  inline void inline_set(T _m11, T _m12, T _m13, T _m21, T _m22, T _m23, T _m31,
                         T _m32, T _m33) {
    (*this)[0].inline_set(_m11, _m12, _m13);
    (*this)[1].inline_set(_m21, _m22, _m23);
    (*this)[2].inline_set(_m31, _m32, _m33);
  }

  inline void setRow(const etVec3<T>& _v, int i) {
    assert(i<3);
    elem[i][0] = _v[0], elem[i][1] = _v[1], elem[i][2] = _v[2];
  }

  inline void setCol(const etVec3<T>& _v, int i) {
    assert(i<3);
    elem[0][i] = _v[0], elem[1][i] = _v[1], elem[2][i] = _v[2];
  }


  // operators
  inline etVec3<T>& operator[](int i) {
    assert(i<3);
    return *(etVec3<T>*)elem[i];
  }

  inline const etVec3<T>& operator[](int i) const {
    return *(etVec3<T> *) elem[i];
  }

  inline void operator=(const etMat3<T>& _m) {
    memcpy(elem,_m.elem,sizeof(elem));
  }

  inline void operator+=(const etMat3<T>& _m) {
    (*this)[0] += _m[0], (*this)[1] += _m[1], (*this)[2] += _m[2];
  }

  inline void operator-=(const etMat3<T>& _m) {
    (*this)[0] -= _m[0], (*this)[1] -= _m[1], (*this)[2] -= _m[2];
  }

  inline void operator*=(const T _s) { mul(*this, _s); }

  inline void operator*=(const etMat3<T>& _m) { mul(*this, _m); }

  inline void operator/=(const T _s) { mul(*this, (1 / _s)); }

  // methods modifying 'this'
  inline void add(const etMat3<T>& _m1, const etMat3<T>& _m2) {
    (*this)[0].add(_m1[0],_m2[0]);
    (*this)[1].add(_m1[1],_m2[1]);
    (*this)[2].add(_m1[2],_m2[2]);
  }

  inline void sub(const etMat3<T>& _m1, const etMat3<T>& _m2) {
    (*this)[0].sub(_m1[0],_m2[0]);
    (*this)[1].sub(_m1[1],_m2[1]);
    (*this)[2].sub(_m1[2],_m2[2]);
  }

  inline void mul(const etMat3<T>& _m, T _sx, T _sy, T _sz) {
    (*this)[0].mul(_m[0],_sx);
    (*this)[1].mul(_m[1],_sy);
    (*this)[2].mul(_m[2],_sz);
  }

  inline void mul(const etMat3<T>& _m, T _s) { mul(_m, _s, _s, _s); }

  // matrix product
  inline void mul(const etMat3<T>& _m1, const etMat3<T>& _m2) {
    set(_m1[0][0]*_m2[0][0]+_m1[0][1]*_m2[1][0]+_m1[0][2]*_m2[2][0],
        _m1[0][0]*_m2[0][1]+_m1[0][1]*_m2[1][1]+_m1[0][2]*_m2[2][1],
        _m1[0][0]*_m2[0][2]+_m1[0][1]*_m2[1][2]+_m1[0][2]*_m2[2][2],
        _m1[1][0]*_m2[0][0]+_m1[1][1]*_m2[1][0]+_m1[1][2]*_m2[2][0],
        _m1[1][0]*_m2[0][1]+_m1[1][1]*_m2[1][1]+_m1[1][2]*_m2[2][1],
        _m1[1][0]*_m2[0][2]+_m1[1][1]*_m2[1][2]+_m1[1][2]*_m2[2][2],
        _m1[2][0]*_m2[0][0]+_m1[2][1]*_m2[1][0]+_m1[2][2]*_m2[2][0],
        _m1[2][0]*_m2[0][1]+_m1[2][1]*_m2[1][1]+_m1[2][2]*_m2[2][1],
        _m1[2][0]*_m2[0][2]+_m1[2][1]*_m2[1][2]+_m1[2][2]*_m2[2][2]);
  }

  // fast one
  inline void mul_inline(const etMat3<T>& _m1, const etMat3<T>& _m2) {
    inline_set(_m1[0][0]*_m2[0][0]+_m1[0][1]*_m2[1][0]+_m1[0][2]*_m2[2][0],
               _m1[0][0]*_m2[0][1]+_m1[0][1]*_m2[1][1]+_m1[0][2]*_m2[2][1],
               _m1[0][0]*_m2[0][2]+_m1[0][1]*_m2[1][2]+_m1[0][2]*_m2[2][2],
               _m1[1][0]*_m2[0][0]+_m1[1][1]*_m2[1][0]+_m1[1][2]*_m2[2][0],
               _m1[1][0]*_m2[0][1]+_m1[1][1]*_m2[1][1]+_m1[1][2]*_m2[2][1],
               _m1[1][0]*_m2[0][2]+_m1[1][1]*_m2[1][2]+_m1[1][2]*_m2[2][2],
               _m1[2][0]*_m2[0][0]+_m1[2][1]*_m2[1][0]+_m1[2][2]*_m2[2][0],
               _m1[2][0]*_m2[0][1]+_m1[2][1]*_m2[1][1]+_m1[2][2]*_m2[2][1],
               _m1[2][0]*_m2[0][2]+_m1[2][1]*_m2[1][2]+_m1[2][2]*_m2[2][2]);
  }

  inline void transpose(const etMat3<T>& _m) {
    T tmp;

    // diagonal first
    elem[0][0] = _m[0][0], elem[1][1] = _m[1][1], elem[2][2] = _m[2][2];

    // then upper triangle
    tmp = _m[0][1], elem[0][1] = _m[1][0], elem[1][0] = tmp;
    tmp = _m[0][2], elem[0][2] = _m[2][0], elem[2][0] = tmp;
    tmp = _m[1][2], elem[1][2] = _m[2][1], elem[2][1] = tmp;
  }

  inline void div(const etMat3<T>& _m, double _s) { mul(_m, (1 / _s)); }

  // cofactors, determinants, inverse
  inline T cofactor(int r, int c) const {
    static const int index[3][2] = { {1,2}, {0,2}, {0,1} };
    int i0=index[r][0], i1=index[r][1];
    int j0=index[c][0], j1=index[c][1];
    return (((r+c)&1) ? -1.l: 1.l) * (elem[i0][j0]*elem[i1][j1]-elem[i0][j1]*elem[i1][j0]);
  }

  // reducing methods which result is a double
  inline T det(void) const {
    return elem[0][0] * cofactor(0,0) +
      elem[0][1] * cofactor(0,1) +
      elem[0][2] * cofactor(0,2);
  }

  // warning: no inline here !
  inline void adjoint(const etMat3<T>& _m) {
    set(_m.cofactor(0,0),_m.cofactor(1,0),_m.cofactor(2,0),
        _m.cofactor(0,1),_m.cofactor(1,1),_m.cofactor(2,1),
        _m.cofactor(0,2),_m.cofactor(1,2),_m.cofactor(2,2));
  }

  // try a adjoint divided by deteminant
  inline bool inverse(const etMat3<T>& _m) {
    T d=_m.det();
    if (ABS(d) < -1E99) return false;
    adjoint(_m);
    *this /= d;
    return true;
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

typedef etMat3<float>  etMat3f;
typedef etMat3<double> etMat3d;

#endif

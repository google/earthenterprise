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

#ifndef QUADRIC_H
#define QUADRIC_H

#include <assert.h>

//////////////////////////////////////////////////////////////////////////////
// General vector definition for the quadric error metric processing.
// Basically this will allow us to add multiple paramters for the error
// computation.
template <class T, int N>
class etQVector {
 public:
  T elem[N];

  etQVector() { reset(); }

  etQVector(const etQVector<T, N> &v) {
    int i;
    for(i=0;i<N;i++) elem[i] = v.elem[i];
  }

  T &operator[](int index) {
    assert(index < N);
    return elem[index];
  }

  const T &operator[](int index) const {
    assert(index < N);
    return elem[index];
  }

  // Vector operators
  void operator+=(const etQVector<T, N> &v) {
    int i;
    for(i=0;i<N;i++) elem[i] += v.elem[i];
  };

  void operator-=(const etQVector<T, N> &v) {
    int i;
    for(i=0;i<N;i++) elem[i] -= v.elem[i];
  };

  // Scalar operators
  void operator+=(const T v) {
    int i;
    for(i=0;i<N;i++) elem[i] += v;
  }

  void operator-=(const T v) {
    int i;
    for(i=0;i<N;i++) elem[i] -= v;
  }

  void operator*=(const T v) {
    int i;
    for(i=0;i<N;i++) elem[i] *= v;
  }

  T norm2() {
    T dst = 0.0;
    int i;
    for(i=0;i<N;i++) dst += elem[i]*elem[i];
    return dst;
  }

  T norm() { return (T)sqrt(norm2()); }

  // normalization function.
  T normalize() {
    T dst = norm2();
    if(dst > 0) {
      if(dst == 1.0) return dst;
      dst = sqrt(dst);
      *this *= (1.0 / dst);
      return dst;
    }
    else reset();

    return 0.0;
  }

  // Cross product
  void cross(const etQVector<T,N> &v1, const etQVector<T,N> &v2)
  {
    // return vector cannot be the same.
    assert(this != &v1 && this != &v2);

    int i;
    for(i=0;i<N;i++)
    {
      int idx1 = (i+1)%N, idx2 = (i+2)%N;
      elem[i] = v1.elem[idx1] * v2.elem[idx2] - v1.elem[idx2] * v2.elem[idx1];
    }
  }

  // Dot product
  T dot(const etQVector<T,N> &v1, const etQVector<T,N> &v2)
  {
    T dst = 0.0;
    int i;
    for(i=0;i<N;i++) dst += v1.elem[i] * v2.elem[i];
    return dst;
  }

  T dot(const etQVector<T,N> &v)
  {
    T dst = 0.0;
    int i;
    for(i=0;i<N;i++) dst += elem[i] * v.elem[i];
    return dst;
  }

  int base() const { return N; }

  // Distance from segment
  T segmentDist(const etQVector<T,N> &p1, const etQVector<T,N> &p2)
  {
    etQVector<T,N> V(p2);    V -= p1;
    etQVector<T,N> W(*this); W -= p1;

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
  T dist(const etQVector<T,N> &p1, const etQVector<T,N> &p2)
  {
    etQVector<T,N> p(p1); p -= p2;
    return p.norm();
  }

 protected:
  void reset()
  {
    int i;
    for(i=0;i<N;i++) elem[i] = (T)0.0;
  }
};


//////////////////////////////////////////////////////////////////////////////
// T is float or double
// N is the number of components of the quadric
//
template <class T, int N>
class etQuadric {
 public:
  T   elem[(N*(N+1))>>1];
  T   area;

  int base() const { return N; }
  int size() const { return (N*(N+1))>>1; }

  // Constructors
  etQuadric()
  {
    reset();
  }

  etQuadric(const etQVector<T,N-1> &v1,const etQVector<T,N-1> &v2,const etQVector<T,N-1> &v3,
            const double area = 1.0)
  {
    init(v1,v2,v3,area);
  }

  etQuadric(const etQVector<T,N> &v, const double area = 1.0)
  {
    init(v, area);
  }

  etQuadric(const etQuadric<T,N> &q)
  {
    assert(q.base() == base());
    int i;
    for(i=0;i<size();i++) elem[i] = q.elem[i];
    area = q.area;
  }


  // Initialization with a plane
  void init(const etQVector<T,N> &v, const double a)
  {
    int i;
    for(i = 0;i<N;i++)
    {
      int j;
      for(j = i;j<N;j++)
        val(i, j) = v.elem[i] * v.elem[j];
    }
    area = a;
  }

  // Initialization with vectors.
  void init(const etQVector<T,N-1> &v1,const etQVector<T,N-1> &v2,const etQVector<T,N-1> &v3)
  {
    etQVector<T,N-1> u(v2), v(v3), n;
    u -= v1;
    v -= v1;
    n.cross(u, v);
    double len = n.normalize();
    etQVector<T,N> res;
    int i;
    for(i=0;i<N-1;i++) res[i] = n[i];
    res[N-1] = -n.dot(v1);
    init(res, 0.5 * len);
  }

  // Arithmetic manipulations.
  void operator+=(const etQuadric<T, N> &q) {
    int i;
    for(i=0;i<size();i++) elem[i] += q.elem[i];
    area += q.area;
  }

  void operator-=(const etQuadric<T, N> &q) {
    int i;
    for(i=0;i<size();i++) elem[i] -= q.elem[i];
    area -= q.area;
  }

  void operator*=(const etQuadric<T, N> &q) {
    int i;
    for(i=0;i<size();i++) elem[i] *= q.elem[i];
  }

  void operator*=(const T &v) {
    int i;
    for(i=0;i<size();i++) elem[i] *= v;
  }

  // Evaluate a vector versus the quadric. The value returned
  // is the quadric error metric.
  double evaluate(const etQVector<T,N-1> &v) const
  {
    static const double factor[3][4] = {{1,2,2,2},{0,1,2,2},{0,0,1,2}};
    static const double mult[4][2] = {{1,0},{1,0},{1,0},{0,1}};
    double res = 0.0;

    int i;
    for(i = 0;i<N-1;i++)
    {
      int j;
      for(j = i;j<N-1;j++)
        res += factor[i][j] * getval(i, j) * v[i] * (mult[j][0] * v[j] + mult[j][1]);
      res += factor[i][N-1] * getval(i, N-1) * v[i] * (mult[j][1]);
    }
    res += getval(N-1, N-1);

    return res;
  }

  // reset everyting to 0
  void reset() {
    int i;
    for(i=0;i<size();i++) elem[i] = (T)0.0;
    area = (T)0.0;
  }

 protected:

  inline T& val(const int x, const int y)
  {
    static const int idx[4][4] = {{0,1,2,3},{1,4,5,6},{2,5,7,8},{3,6,8,9}};
    assert(x < N && y < N);
    return elem[idx[y][x]];
  }

  inline T getval(const int x, const int y) const
  {
    static const int idx[4][4] = {{0,1,2,3},{1,4,5,6},{2,5,7,8},{3,6,8,9}};
    assert(x < N && y < N);
    return elem[idx[y][x]];
  }

};


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Specific Vector & Quadric definition for geometry only Qems.
class etQVector3d {
 public:
  double elem[3];

  etQVector3d() { reset(); }

  etQVector3d(const etQVector3d &v) {
    elem[0] = v.elem[0];
    elem[1] = v.elem[1];
    elem[2] = v.elem[2];
  }

  inline double &operator[](int index) {
    assert(index < 3);
    return elem[index];
  }

  inline const double &operator[](int index) const {
    assert(index < 3);
    return elem[index];
  }

  // Vector operators
  void operator+=(const etQVector3d &v) {
    elem[0] += v[0];
    elem[1] += v[1];
    elem[2] += v[2];
  };

  void operator-=(const etQVector3d &v) {
    elem[0] -= v[0];
    elem[1] -= v[1];
    elem[2] -= v[2];
  };

  // Scalar operators
  void operator+=(const double v) {
    elem[0] += v;
    elem[1] += v;
    elem[2] += v;
  }

  void operator-=(const double v) {
    elem[0] -= v;
    elem[1] -= v;
    elem[2] -= v;
  }

  void operator*=(const double v) {
    elem[0] *= v;
    elem[1] *= v;
    elem[2] *= v;
  }

  double norm2() {
    double dst = 0.0;
    dst += elem[0]*elem[0];
    dst += elem[1]*elem[1];
    dst += elem[2]*elem[2];
    return dst;
  }

  double norm() { return sqrt(norm2()); }

  // normalization function.
  double normalize() {
    double dst = norm2();
    if(dst > 0) {
      if(dst == 1.0) return dst;
      dst = sqrt(dst);
      *this *= (1.0 / dst);
      return dst;
    }
    else reset();

    return 0.0;
  }

  // Cross product
  void cross(const etQVector3d &v1, const etQVector3d &v2)
  {
    // return vector cannot be the same.
    assert(this != &v1 && this != &v2);
    elem[0] = v1[1] * v2[2] - v1[2] * v2[1];
    elem[1] = v1[2] * v2[0] - v1[0] * v2[2];
    elem[2] = v1[0] * v2[1] - v1[1] * v2[0];
  }

  // Dot product
  double dot(const etQVector3d &v1, const etQVector3d &v2)
  {
    double dst = 0.0;
    dst += v1.elem[0] * v2.elem[0];
    dst += v1.elem[1] * v2.elem[1];
    dst += v1.elem[2] * v2.elem[2];
    return dst;
  }

  double dot(const etQVector3d &v)
  {
    double dst = 0.0;
    dst += elem[0] * v.elem[0];
    dst += elem[1] * v.elem[1];
    dst += elem[2] * v.elem[2];
    return dst;
  }

  int base() const { return 3; }

  // Distance from segment
  double segmentDist(const etQVector3d &p1, const etQVector3d &p2)
  {
    etQVector3d V(p2);    V -= p1;
    etQVector3d W(*this); W -= p1;

    double c1 = dot( W, V );
    if( c1 <= 0 ) return dist(*this, p1);

    double c2 = dot( V, V );
    if ( c2 <= c1 ) return dist(*this, p2);

    double b = c1 / c2;

    V *= b;
    V += p1;
    return dist(*this, V);
  }

  // Distance between 2 points
  double dist(const etQVector3d &p1, const etQVector3d &p2)
  {
    etQVector3d p(p1); p -= p2;
    return p.norm();
  }

 protected:
  void reset()
  {
    elem[0] = 0.0;
    elem[1] = 0.0;
    elem[2] = 0.0;
  }
};




#endif

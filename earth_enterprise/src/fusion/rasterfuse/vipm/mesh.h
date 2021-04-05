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

#ifndef _MESH_H
#define _MESH_H

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "vector.h"
#include "array.h"


//////////////////////////////////////////////////////////////////////////////
//
//  General mesh definition (Manifold mesh)
//
//////////////////////////////////////////////////////////////////////////////
template <class V, class T>
class etMesh {
 protected:
  etArray<V> points;
  etArray<T> faces;
  etArray<T> neighbours;

 public:
  etBoundingBox3<float> box;

  // generic constructor
  etMesh() {
    points.reset();
    faces.reset();
    neighbours.reset();
  }

  // destructor
  ~etMesh() { }

  // Mesh is valid if there are vertices and triangles.
  bool isValid() {
    return points.length && faces.length;
  }

  // Query data.
  inline const int nPoints()        const { return points.length;     }
  inline const int nFaces()         const { return faces.length;      }
  inline V        *getPoints()      const { return points.peek();     }
  inline T        *getFaces()       const { return faces.peek();      }
  inline T        *getNeighbours()  const { return neighbours.peek(); }

  // Reset function
  void reset() { points.reset(); faces.reset(); neighbours.reset(); }

  int addPoint(const float x, const float y, const float z) {
    V pt(x, y, z);
    int idx = points.add(&pt);
    box.add(x, y, z);
    return idx;
  }

  int addFace(const int p1, const int p2, const int p3) {
    T t(p1, p2, p3);
    int idx = faces.add(&t);
    return idx;
  }

  inline void getPoint(const int idx, float &x, float &y, float &z) {
    x = points[idx][0]; y = points[idx][1]; z = points[idx][2];
  }

  inline void getFace(const int idx, int &p1, int &p2, int &p3) {
    p1 = faces[idx][0]; p2 = faces[idx][1]; p3 = faces[idx][2];
  }

  inline void getNeighbour(const int idx, int &t1, int &t2, int &t3) {
    t1 = neighbours[idx][0]; t2 = neighbours[idx][1]; t3 = neighbours[idx][2];
  }

  void updateBox()
  {
    box.reset();
    int i;
    for(i=0;i<points.length;i++)
    {
      box.add(points[i][0],points[i][1],points[i][2]);
    }
  }

  void findNeighbours(); // VERIFIED

  bool loadSMF(const char *fname);
  bool saveSMF(const char *fname);

 private:
  int findAndUpdateNeighbour(int *all, unsigned char *table, const int p1,
                             const int p2, const int f);
};


//////////////////////////////////////////////////////////////////////////////
// Note: The methods implementation must be in the include file because it's
//       a template.
//////////////////////////////////////////////////////////////////////////////
#define MAX_INDICES 6
#define ALL(a,b) (all[(a)*MAX_INDICES+(b)])

template <class V, class T>
void etMesh<V,T>::findNeighbours() {

  neighbours.reset();

  int i;
  for(i=0;i<faces.length;i++) {
    T t(-1,-1,-1);
    neighbours.add(&t);
  }

  // Allocating a buffer to store the face indices plus the number of indices
  // in each list.
  unsigned char *table = new unsigned char[points.length];
  memset(table, 0, points.length);

  int *all = new int[points.length * MAX_INDICES];

  // For each face, I find the related neighbours.
  for(i=0;i<faces.length;i++) {
    int p1 = faces[i][0];
    int p2 = faces[i][1];
    int p3 = faces[i][2];

    // Find and update the neighbours
    int f;
    f = findAndUpdateNeighbour(all, table, p2, p1, i);
    if(f != -1) neighbours[i][0] = f;
    else ALL(p1,table[p1]++) = i;

    if(f != -1) {
#ifndef NDEBUG
      const int t1 = neighbours[f][0];
      const int t2 = neighbours[f][1];
      const int t3 = neighbours[f][2];
#endif
      assert(t1 == -1 || t2 == -1 || (t1>=0 && t2>=0 && t1 != t2));
      assert(t2 == -1 || t3 == -1 || (t2>=0 && t3>=0 && t2 != t3));
      assert(t3 == -1 || t1 == -1 || (t3>=0 && t1>=0 && t3 != t1));
    }

    f = findAndUpdateNeighbour(all, table, p3, p2, i);
    if(f != -1) neighbours[i][1] = f;
    else ALL(p2,table[p2]++) = i;

    if(f != -1) {
#ifndef NDEBUG
      const int t1 = neighbours[f][0];
      const int t2 = neighbours[f][1];
      const int t3 = neighbours[f][2];
#endif
      assert(t1 == -1 || t2 == -1 || (t1>=0 && t2>=0 && t1 != t2));
      assert(t2 == -1 || t3 == -1 || (t2>=0 && t3>=0 && t2 != t3));
      assert(t3 == -1 || t1 == -1 || (t3>=0 && t1>=0 && t3 != t1));
    }

    f = findAndUpdateNeighbour(all, table, p1, p3, i);
    if(f != -1) neighbours[i][2] = f;
    else ALL(p3,table[p3]++) = i;

    if(f != -1) {
#ifndef NDEBUG
      const int t1 = neighbours[f][0];
      const int t2 = neighbours[f][1];
      const int t3 = neighbours[f][2];
#endif
      assert(t1 == -1 || t2 == -1 || (t1>=0 && t2>=0 && t1 != t2));
      assert(t2 == -1 || t3 == -1 || (t2>=0 && t3>=0 && t2 != t3));
      assert(t3 == -1 || t1 == -1 || (t3>=0 && t1>=0 && t3 != t1));
    }

  }

  delete [] table;
  delete [] all;
}

template <class V, class T>
int etMesh<V, T>::findAndUpdateNeighbour(int *all, unsigned char *table,
                                         const int p1, const int p2,
                                         const int f) {
  int n = table[p1];
  int found_face = -1;

  int i;
  for(i=0;i<n;i++) {
    int idx = ALL(p1, i);

    int pp1 = faces[idx][0];
    int pp2 = faces[idx][1];
    int pp3 = faces[idx][2];

    if(pp1 == p1 && pp2 == p2) {
      neighbours[idx][0]= f;
      found_face = idx;
      break;
    }
    if(pp2 == p1 && pp3 == p2) {
      neighbours[idx][1]= f;
      found_face = idx;
      break;
    }
    if(pp3 == p1 && pp1 == p2) {
      neighbours[idx][2]= f;
      found_face = idx;
      break;
    }
  }

  // Decrement the list
  if(found_face != -1) {
    ALL(p1, i) = ALL(p1, --table[p1]);
  }

  return found_face;
}


//////////////////////////////////////////////////////////////////////////////
template <class V, class T>
bool etMesh<V,T>::loadSMF(const char *fname) {
  FILE *fp = fopen(fname, "rb");
  if(fp) {
    float x, y, z;
    int p1, p2, p3;

    points.reset();
    faces.reset();

    char buf[80];
    while(fgets(buf,80,fp)) {
      if(sscanf(buf, "v %f%f%f", &x, &y, &z) == 3) {
        addPoint(x,y,z);
      }
      else
        if(sscanf(buf, "f %d%d%d", &p1, &p2, &p3) == 3) {
          addFace(p1-1,p2-1,p3-1);
        }
    }

    fprintf(stderr, "Loaded mesh: %d points & %d triangles\n", points.length, faces.length);

    fclose(fp);

    return true;
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////////
template <class V, class T>
bool etMesh<V,T>::saveSMF(const char *fname) {
  FILE *fp = fopen(fname, "wb");
  if(fp) {
    float x, y ,z;
    int p1, p2, p3;

    int i;
    for(i=0;i<points.length;i++)
    {
      getPoint(i, x, y, z);
      fprintf(fp, "v %f %f %f\n", x, y, z);
    }

    for(i=0;i<faces.length;i++)
    {
      getFace(i, p1, p2, p3);
      fprintf(fp, "f %d %d %d\n", p1+1, p2+1, p3+1);
    }

    fclose(fp);

    return true;
  }

  return false;
}

#endif

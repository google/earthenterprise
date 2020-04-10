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


// 3D Polygon Simplification algorithm
// This is an implementation of a simple
// vertex clustering algorithm by
// Rossignac, Jarek and Borrel, Paul.

// The algorithm works by imposing a grid
// of a certain resolution and approximating
// each vertex to a representative one.

#include "fusion/gst/gstPolygonSimplifier.h"

#include <cmath>
#include <algorithm>
#include <vector>
#include <map>

#include <notify.h>

#include "fusion/gst/gstConstants.h"
#include "fusion/gst/gstGeomUtils.h"


namespace fusion_gst {

std::int64_t PolygonSimplifier::ComputeHashKey(const gstVertex &v) const {
  std::int64_t key = 0;
  key = static_cast<std::int64_t>((v.x - _gridExtents.min.x)/_res.x) &
      0x0000ffff;
  key |= (static_cast<std::int64_t>((v.y - _gridExtents.min.y)/_res.y) &
          0x0000ffff) << 16;
  key |= (static_cast<std::int64_t>((v.z - _gridExtents.min.z)/_res.z) &
          0x0000ffff) << 32;
  return key;
}


void PolygonSimplifier::ComputeRepVertex(const gstVertex &v) {
  std::int64_t key = ComputeHashKey(v);
  psRepVertex& rep = _repVerts[key];
  rep.v.x = ((rep.v.x * rep.numPoints) + v.x) / (rep.numPoints + 1);
  rep.v.y = ((rep.v.y * rep.numPoints) + v.y) / (rep.numPoints + 1);
  rep.v.z = ((rep.v.z * rep.numPoints) + v.z) / (rep.numPoints + 1);
  rep.numPoints++;
}


void PolygonSimplifier::ReducePolygon(psPolygon &polygon) const {
  unsigned int i;
  for (i = 0; i < polygon.v.size(); ++i) {
    //        if (polygon.internalVertex[i] == true)
    //            continue;
    std::int64_t key = ComputeHashKey(polygon.v[i]);
    std::map<std::int64_t, psRepVertex>::const_iterator it = _repVerts.find(key);
    if (it != _repVerts.end()) {
      polygon.v[i] = it->second.v;
    } else {
      notify(NFY_FATAL,
             "PolygonSimplifier: invalid vertex representation map.");
    }
  }

  std::vector<gstVertex> copyVerts;
  std::vector<bool> copyFlags;

  copyVerts.push_back(polygon.v[0]);
  copyFlags.push_back(polygon.internalVertex[0]);

  unsigned int j = 0;
  for (i = 1; i < polygon.v.size(); ++i) {
    if (copyVerts[j] != polygon.v[i]) {
      copyVerts.push_back(polygon.v[i]);
      copyFlags.push_back(polygon.internalVertex[i]);
      ++j;
    }
  }

  if (copyVerts[j] != copyVerts[0]) {
    copyVerts.push_back(copyVerts[0]);
    copyFlags.push_back(false);
    // TODO: should get flag from last vertex?!
  }

  polygon.v.swap(copyVerts);
  polygon.internalVertex.swap(copyFlags);
}

bool PolygonSimplifier::AreCongruent(const psPolygon& a,
                                     const psPolygon& b) const {
  if (a.v.size() != b.v.size())
    return false;

  for (unsigned int i = 0; i < a.v.size(); i++) {
    bool match = false;
    for (unsigned int j = 0; j < b.v.size(); j++) {
      if (a.v[i] == b.v[j]) {
        match = true;
        break;
      }
    }

    if (!match)
      return false;
  }

  return true;
}

void PolygonSimplifier::ReducePolygons(std::vector<psPolygon>& polygons) {
  // populate the hash table with representative vertices
  unsigned int i;
  for (i = 0; i < polygons.size(); i++) {
    std::vector<gstVertex>& verts = polygons[i].v;

    for (unsigned int j = 0; j < verts.size(); j++) {
      ComputeRepVertex(verts[j]);
    }
  }

  // remove duplicate vertices in each polygon
  for (i = 0; i < polygons.size(); i++) {
    ReducePolygon(polygons[i]);
  }

  // remove polygons that contain less than 4 verts
  // the first and last verts are the same
  // so this will retain line segments
  std::vector<psPolygon> copyPolys;
  for (i = 0; i < polygons.size(); ++i) {
    if (polygons[i].v.size() >= kMinCycleVertices) {
      copyPolys.push_back(polygons[i]);
    }
  }
  polygons.swap(copyPolys);
  copyPolys.clear();

  // remove overlapping polygons
  std::vector<bool> deletes(polygons.size(), false);

  for (i = 0; i < polygons.size(); ++i) {
    if (deletes[i])
      continue;

    copyPolys.push_back(polygons[i]);

    for (unsigned int j = (i + 1); j < polygons.size(); ++j) {
      if (deletes[j])
        continue;

      deletes[j] = AreCongruent(polygons[i], polygons[j]);
    }
  }

  polygons.swap(copyPolys);
}

// NOTE: currently functions are not used.
#if 0
bool PolygonSimplifier::MergeCoplanar(psPolygon& a, psPolygon& b) {
  bool adjacent = false;

  if ((a.v.size() < 3) || (b.v.size() < 3))
    return adjacent;

  psPlaneEquation eqnA = a._equation;
  psPlaneEquation eqnB = b._equation;

  if ((eqnA == eqnB) || (eqnA == eqnB*(-1))) {
    // if the normals are opposite then
    // reverse the order of b verts
    if (eqnA == eqnB*(-1))
      reverse(b.v.begin(), b.v.end());

    unsigned int i = 0;
    unsigned int j = 0;
    while ((i < (a.v.size() - 1)) && !adjacent) {
      j = 0;
      while ((j < (b.v.size()-1)) && !adjacent) {
        if ((a.v[i] == b.v[j+1]) && (a.v[i+1] == b.v[j]))
          adjacent = true;
        j++;
      }
      i++;
    }
    if (adjacent) {
      std::vector<gstVertex>::iterator it = a.v.begin();
      advance(it, i);
      std::vector<gstVertex>::iterator jt = b.v.begin();
      advance(jt, (j - 1));
      if (j >= 1)
        a.v.insert(it, b.v.begin(), jt);

      it = a.v.begin();
      advance(it, i);
      jt = b.v.begin();
      advance(jt, (j + 1));

      if ((j + 1) < b.v.size())
        a.v.insert(it, jt, b.v.end());
    }
  }

  return adjacent;
}

void PolygonSimplifier::MergeCoplanarPolygons(
    std::vector<psPolygon>& polygons) {
  unsigned int i;
  for (i = 0; i < polygons.size(); i++) {
    if (polygons[i].v.size() >= 3) {
      // TODO: check for returned value.
      ComputePlaneEquation(
          polygons[i].v[0], polygons[i].v[1], polygons[i].v[2],
          &(polygons[i]._equation.normal),
          &(polygons[i]._equation.distance));
    }
  }

  std::vector<psPolygon> newPolys;
  std::vector<bool> deletes(polygons.size(), false);

  for (i = 0; i < polygons.size(); i++) {
    if (deletes[i])
      continue;

    newPolys.push_back(polygons[i]);
    for (unsigned int j = (i + 1); j < polygons.size(); j++) {
      if (deletes[j])
        continue;

      deletes[j] = MergeCoplanar(polygons[i], polygons[j]);
    }
  }

  polygons.swap(newPolys);
}
#endif

void PolygonSimplifier::Decimate(std::vector<psPolygon>& polygons,
                                 int numCells) {
  // compute the grid extents for the entire surface
  for (unsigned int i = 0; i < polygons.size(); i++) {
    _gridExtents.grow(polygons[i].bbox());
  }

  _res.x = (_gridExtents.max.x - _gridExtents.min.x)/numCells;
  _res.y = (_gridExtents.max.y - _gridExtents.min.y)/numCells;
  _res.z = (_gridExtents.max.z - _gridExtents.min.z)/numCells;

  ReducePolygons(polygons);
  //  MergeCoplanarPolygons(polygons);
}

}  // namespace fusion_gst

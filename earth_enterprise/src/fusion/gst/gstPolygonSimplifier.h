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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTPOLYGONSIMPLIFIER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTPOLYGONSIMPLIFIER_H_

#include <cstdint>
#include <map>
#include <vector>

#include "fusion/gst/gstVertex.h"

// #include "common.h"

namespace fusion_gst {

// Defining my own bounding box struct
// coz BBox is essentially 2D and I
// need 3D bbox.
struct psBBox {
  psBBox()
      : min(10000,  10000,  10000),
        max(-10000, -10000, -10000) {
  }

  psBBox(gstVertex a, gstVertex b)
      : min(a), max(b) {
  }

  void grow(psBBox box) {
    if (box.min.x < min.x)
      min.x = box.min.x;
    if (box.min.y < min.y)
      min.y = box.min.y;
    if (box.min.z < min.z)
      min.z = box.min.z;
    if (box.max.x > max.x)
      max.x = box.max.x;
    if (box.max.y > max.y)
      max.y = box.max.y;
    if (box.max.z > max.z)
      max.z = box.max.z;
  }

  gstVertex min;
  gstVertex max;
};


struct psPlaneEquation {
  psPlaneEquation() : normal(), distance(0) {}

  psPlaneEquation(gstVertex _normal, double _distance)
  : normal(_normal), distance(_distance) {}

  psPlaneEquation operator*(int scale) const {
    gstVertex temp(normal.x*scale, normal.y*scale, normal.z*scale);
    return psPlaneEquation(temp, distance);
  }

  bool operator==(const psPlaneEquation& eq) const {
    return ((normal == eq.normal) && (distance == eq.distance));
  }

  gstVertex normal;
  double distance;
};


struct psPolygon {
  explicit psPolygon(const std::vector<gstVertex>& verts)
      : v(verts) {
    internalVertex.resize(v.size());
  }

  psPolygon() {}

  psBBox bbox() {
    psBBox box;
    for (unsigned int i = 0; i < v.size(); i++)
      box.grow(psBBox(v[i], v[i]));

    return box;
  }

  void AddVertex(const gstVertex& vert, bool isInternal) {
    v.push_back(vert);
    internalVertex.push_back(isInternal);
  }

  // TODO: currently it is not used.
  // Implementation should be the same as in gstGeode::IsInternalVertex().
  bool EdgeFlag(unsigned int i) const {
    if (i == (internalVertex.size() - 1))
      return internalVertex[i];
    else
      return (internalVertex[i] | internalVertex[i + 1]);
  }

  std::vector<gstVertex> v;
  std::vector<bool> internalVertex;
  psPlaneEquation _equation;
};


struct psRepVertex {
  psRepVertex(): v(), numPoints(0) {}

  gstVertex v;
  int numPoints;
};

class PolygonSimplifier {
 public:
  PolygonSimplifier()
      : _gridExtents(), _res() {}

  void Decimate(std::vector<psPolygon>& polygons, int numCells);

 private:
  std::int64_t ComputeHashKey(const gstVertex &v) const;

  void ComputeRepVertex(const gstVertex &v);

  void ReducePolygon(psPolygon& polygon) const;

  bool AreCongruent(const psPolygon& a, const psPolygon& b) const;

  void ReducePolygons(std::vector<psPolygon>& polygons);

// NOTE: currently functions are not used.
#if 0
  bool MergeCoplanar(psPolygon& a, psPolygon& b);
  void MergeCoplanarPolygons(std::vector<psPolygon>& polygons);
#endif

 private:
  psBBox _gridExtents;
  gstVertex _res;
  std::map<std::int64_t, psRepVertex> _repVerts;
};

}  // namespace fusion_gst

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTPOLYGONSIMPLIFIER_H_

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


#include "fusion/gst/gstGeode.h"

#include <unistd.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <math.h>

#include <vector>
#include <deque>
#include <utility>
#include <algorithm>

#include "common/notify.h"
#include "fusion/gst/font.h"
#include "fusion/gst/gstMisc.h"
#include "fusion/gst/gstPolygonClipper.h"
#include "fusion/gst/gstFeature.h"
#include "fusion/gst/gstJobStats.h"
#include "fusion/gst/gstPolygonUtils.h"
#include "fusion/gst/gstGeomUtils.h"
#include "fusion/gst/gstSequenceAlg.h"
#include "fusion/gst/gstMathUtils.h"

int gstGeodeImpl::gcount = 0;
int gstGeodeImpl::isectCount = 0;
int gstGeodeImpl::isectDeepCount = 0;

namespace {

template< typename gstVertex >
bool IsEqualReverseOther(const std::vector<gstVertex>& this_vec,
                         const std::vector<gstVertex>& that_vec) {
  for (size_t i = 0, size = this_vec.size(); size > 0; ++i) {
    if (this_vec[i] != that_vec[--size]) {
      return false;
    }
  }
  return true;
}

template< typename gstVertex >
bool IsEqualExcludeFirstLastReverseOther(
    const std::vector<gstVertex>& this_vec,
    const std::vector<gstVertex>& that_vec) {
  // compare from 1 to size-2 with size-2 to 1
  for (size_t i = 1, size = this_vec.size() - 1; --size >= 1; ++i) {
    if (this_vec[i] != that_vec[size]) {
      return false;
    }
  }
  return true;
}

template< typename gstVertex >
bool IsEqualExcludeFirstLast(
    const std::vector<gstVertex>& this_vec,
    const std::vector<gstVertex>& that_vec) {
  // compare from size-2 to 1
  for (size_t size = this_vec.size() - 1; --size >= 1;) {
    if (this_vec[size] != that_vec[size]) {
      return false;
    }
  }
  return true;
}

// Reads single type geometry from raw representation.
// buf - pointer to buffer to read from.
// geodeh - returned geometry.
// Returns pointer to position in buffer after last read record.
const char* ReadPart(const char* buf, gstGeodeHandle *geodeh) {
  if (!buf) {
    return NULL;
  }

  // validate header is one of ours
  const gstGeodeImpl::RecHeader *hdr =
      reinterpret_cast<const gstGeodeImpl::RecHeader*>(buf);
  if (hdr->size < sizeof(gstGeodeImpl::RecHeader))
    return NULL;

  *geodeh = gstGeodeImpl::Create(gstPrimType(hdr->type));

  const char* fbuf = buf + sizeof(gstGeodeImpl::RecHeader);
  const std::uint32_t vert_size_xy = sizeof(double) * 2;
  const std::uint32_t vert_size_xyz = sizeof(double) * 3;

  // iterate through each feature part
  for (unsigned int ii = 0; ii < hdr->count; ++ii) {
    switch (hdr->type) {
      case gstUnknown:
        geodeh->release();
        notify(NFY_WARN, "Invalid geometry type.");
        return NULL;

      case gstPoint:
        {
          gstGeode *geode = static_cast<gstGeode*>(&(**geodeh));
          const double *pbuf = reinterpret_cast<const double*>(fbuf);
          geode->AddVertexToPart0(gstVertex(pbuf[0], pbuf[1]));
          fbuf += vert_size_xy;
        }
        break;

      case gstPoint25D:
        {
          gstGeode *geode = static_cast<gstGeode*>(&(**geodeh));
          const double *pbuf = reinterpret_cast<const double*>(fbuf);
          geode->AddVertexToPart0(gstVertex(pbuf[0], pbuf[1], pbuf[2]));
          fbuf += vert_size_xyz;
        }
        break;

      case gstPolyLine:
      case gstStreet:
      case gstPolygon:
        {
          gstGeode *geode = static_cast<gstGeode*>(&(**geodeh));
          std::uint32_t verts = *(reinterpret_cast<const std::uint32_t*>(fbuf));
          fbuf += sizeof(std::uint32_t) * 2;
          // false means that it isn't 2.5D extension.
          geode->SetVertexes(reinterpret_cast<const double*>(fbuf), verts,
                             false);


          fbuf += (vert_size_xy * verts);
        }
        break;

      case gstPolyLine25D:
      case gstStreet25D:
      case gstPolygon25D:
      case gstPolygon3D:
        {
          gstGeode *geode = static_cast<gstGeode*>(&(**geodeh));
          std::uint32_t verts = *(reinterpret_cast<const std::uint32_t*>(fbuf));
          fbuf += sizeof(std::uint32_t) * 2;
          geode->SetVertexes(reinterpret_cast<const double*>(fbuf), verts,
                             true);  // true means that it is 2.5D extension.
          fbuf += (vert_size_xyz * verts);
        }
        break;

      case gstMultiPolygon:
      case gstMultiPolygon25D:
      case gstMultiPolygon3D:
        notify(NFY_FATAL, "%s: Improper geometry type %d",
               __func__, hdr->type);
        break;

      default:
        notify(NFY_FATAL, "Invalid geometry type %d", hdr->type);
        break;
    }
  }

  return fbuf;  // Position after last read record.
}

}  // namespace

gstGeodeImpl::gstGeodeImpl(gstPrimType t)
    : prim_type_(static_cast<std::int8_t>(t)) {
  ++gcount;
  InvalidateCachedData(false);  // No need to invalidate the bounding box here.
}

gstGeodeImpl::~gstGeodeImpl() {
  --gcount;
}

gstGeodeHandle gstGeodeImpl::Create(const gstPrimType gtype) {
  switch (gtype) {
    case gstUnknown:
      // TODO: How should we handle gstUnknown-type?
      // Should we create geode of this type? Currently such geodes are skipped
      // in further processing (see ToRaw()).
      notify(NFY_WARN, "Invalid geometry type: %u.", gtype);
      return gstGeodeHandle();

    case gstPoint:
    case gstPoint25D:
    case gstPolyLine:
    case gstPolyLine25D:
    case gstStreet:
    case gstStreet25D:
    case gstPolygon:
    case gstPolygon25D:
    case gstPolygon3D:
      return gstGeodeHandle(khRefGuardFromNew(new gstGeode(gtype)));
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      return gstGeodeHandle(khRefGuardFromNew(new gstMultiPoly(gtype)));
      break;

    default:
      notify(NFY_FATAL, "Invalid geometry type: %u", gtype);
      return gstGeodeHandle();
      break;
  }
}

gstGeodeHandle gstGeodeImpl::Create(const gstBBox& box,
                                    const gstPrimType gtype,
                                    const double altitude) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gtype);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  geode->AddPart(5);
  geode->AddVertex(gstVertex(box.w, box.s, altitude));
  geode->AddVertex(gstVertex(box.e, box.s, altitude));
  geode->AddVertex(gstVertex(box.e, box.n, altitude));
  geode->AddVertex(gstVertex(box.w, box.n, altitude));
  geode->AddVertex(gstVertex(box.w, box.s, altitude));
  return geodeh;
}

gstGeodeHandle gstGeodeImpl::Create(const gstBBox& box,
                                    const gstEdgeType edge_type,
                                    const gstPrimType gtype,
                                    const double altitude) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gtype);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  geode->AddPart(5);
  geode->AddVertexAndEdgeFlag(gstVertex(box.w, box.s, altitude), edge_type);
  geode->AddVertexAndEdgeFlag(gstVertex(box.e, box.s, altitude), edge_type);
  geode->AddVertexAndEdgeFlag(gstVertex(box.e, box.n, altitude), edge_type);
  geode->AddVertexAndEdgeFlag(gstVertex(box.w, box.n, altitude), edge_type);
  geode->AddVertexAndEdgeFlag(gstVertex(box.w, box.s, altitude), edge_type);
  return geodeh;
}

// gstGeode
gstGeode::gstGeode(gstPrimType t)
    : gstGeodeImpl(t), clipped_(-1) {
  parts_.reserve(1);  // Assumption is that mostly parts_[0] will be used
}

gstGeode::~gstGeode() {
}

gstGeodeHandle gstGeode::Duplicate() const {
  gstGeodeHandle new_geodeh = gstGeodeImpl::Create(PrimType());
  gstGeode *new_geode = static_cast<gstGeode*>(&(*new_geodeh));

  for (unsigned int p = 0; p < NumParts(); ++p) {
    new_geode->AddPart(VertexCount(p));
    for (unsigned int v = 0; v < VertexCount(p); ++v) {
      new_geode->AddVertex(GetVertex(p, v));
    }
  }
  // TODO: copy edge_flag.
  return new_geodeh;
}

void gstGeode::ChangePrimType(gstPrimType new_type,
                              gstGeodeHandle *new_geodeh) {
  // TODO: remove this temporary solution after support
  // multi-polyline.
#ifdef MULTI_POLYGON_ON
  if (gstPolyLine == FlatPrimType() &&
      gstPolygon == ToFlatPrimType(new_type) &&
      NumParts() > 1) {
    // Create multi-geometry geode.
    gstGeodeHandle new_multi_geodeh =
        gstGeodeImpl::Create(GetMultiGeodeTypeFromSingle(new_type));
    gstGeodeCollection *new_multi_geode =
        static_cast<gstGeodeCollection*>(&(*new_multi_geodeh));
    for (unsigned int p = 0; p < NumParts(); ++p) {
      gstGeodeHandle new_geodeh = gstGeodeImpl::Create(new_type);
      gstGeode *new_geode = static_cast<gstGeode*>(&(*new_geodeh));
      for (unsigned int v = 0; v < VertexCount(p); ++v) {
        new_geode->AddVertex(GetVertex(p, v));
      }
      new_multi_geode->AddGeode(new_geodeh);
    }
    // return new multi-geometry geode as result of converting.
    if (new_geodeh) {
      (*new_geodeh).swap(new_multi_geodeh);
    }
    return;
  }
#endif

  if (gstPolygon25D == prim_type_ && gstPolygon == new_type) {
    // Set z-coordinate to 0.
    for (unsigned int p = 0; p < NumParts(); ++p) {
      for (unsigned int v = 0; v < VertexCount(p); ++v) {
        gstVertex vpt = GetVertex(p, v);
        vpt.z = .0;
        ModifyVertex(p, v, vpt);
      }
    }
  }

  prim_type_ = static_cast<std::int8_t>(new_type);
}

unsigned int gstGeode::NumParts() const {
  return parts_.size();
}

unsigned int gstGeode::TotalVertexCount() const {
  unsigned int total = 0;
  for (gstGeodeParts::const_iterator it = parts_.begin();
       it != parts_.end(); ++it)
    total += it->size();
  return total;
}

void gstGeode::Clear() {
  parts_.clear();
  edge_flags_.clear();
  InvalidateCachedData(true);
}

bool gstGeode::IsEmpty() const {
  switch (PrimType()) {
    case gstUnknown:
      return true;
      break;

    case gstPoint:
    case gstPoint25D:
    case gstPolyLine:
    case gstPolyLine25D:
    case gstStreet:
    case gstStreet25D:
      if (parts_.size() == 0)
        return true;
      for (unsigned int p = 0; p < parts_.size(); ++p) {
        if (parts_[p].size() != 0)
          return false;
      }
      break;

    case gstPolygon:
    case gstPolygon25D:
    case gstPolygon3D:
#ifdef MULTI_POLYGON_ON
      return (parts_.size() == 0 || VertexCount(0) == 0);
#else
      if (parts_.size() == 0)
        return true;
      for (unsigned int p = 0; p < parts_.size(); ++p) {
        if (parts_[p].size() != 0)
          return false;
      }
#endif
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      assert(false);
      break;
  }

  return true;
}

bool gstGeode::IsDegenerate() const {
  if (IsEmpty()) {
    return true;  // degenerate;
  }

  bool degenerate = false;
  switch (PrimType()) {
    case gstUnknown:
      degenerate = true;
      break;

    case gstPoint:
    case gstPoint25D:
      //  Checked by IsEmpty().
      break;

    case gstPolyLine:
    case gstPolyLine25D:
    case gstStreet:
    case gstStreet25D:
      // TODO: check only first part is not correct
      // for multi-polyline. Such case is impossible because of degenerate
      // polylines should be skipped during data extraction.
      // degenerate = (VertexCount(0) < kMinPolylineVertices);
      for (unsigned int p = 0; p < parts_.size(); ++p) {
        if (parts_[p].size() < kMinPolylineVertices)
          notify(NFY_WARN, "degenerate poly-line primitive");
      }
      break;

    case gstPolygon:
    case gstPolygon25D:
    case gstPolygon3D:
      degenerate = (VertexCount(0) < kMinCycleVertices);
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      assert(false);
      break;
  }
  return degenerate;
}

bool gstGeode::Intersect(const gstBBox& b, uint* part) const {
  ++isectCount;
  if (!BoundingBox().Valid())
    return false;
  if (!b.Intersect(BoundingBox()))
    return false;

  ++isectDeepCount;
  if (part == NULL) {
    unsigned int ignore;
    return IntersectDeep(b, &ignore);
  } else {
    return IntersectDeep(b, part);
  }
}

void gstGeode::ApplyScaleFactor(double scale) {
    for (gstGeodeParts::iterator p = parts_.begin();
         p != parts_.end(); ++p) {
      for (gstGeodePart::iterator v = p->begin();
           v != p->end(); ++v) {
        v->z = v->z * scale;
      }
    }
  }

double gstGeode::Volume() const {
  // this method for determining volume is
  // incorrect because x,y are in normalized degree units while
  // z is in meters. however for our simplification purposes
  // we need this. this way we will never eliminate tall
  // buildings
  if (VertexCount(0) == 0)
    return 0;
  return Area(0) * parts_[0][0].z;
}

bool gstGeode::Equals(const gstGeodeHandle &bh, bool reverse_ok) const {
  if (bh->PrimType() == gstMultiPolygon ||
      bh->PrimType() == gstMultiPolygon25D ||
      bh->PrimType() == gstMultiPolygon3D) {
    notify(NFY_FATAL, "%s: Incorrect geode type.", __func__);
  }

  const gstGeode *b = static_cast<const gstGeode*>(&(*bh));

  if (PrimType() != b->PrimType())
    return false;

  if (NumParts() != b->NumParts())
    return false;

  for (unsigned int part = 0; part < NumParts(); ++part) {
    if (VertexCount(part) != b->VertexCount(part))
      return false;
  }

  for (unsigned int part = 0; part < NumParts(); ++part) {
    const gstGeodePart& a_part = parts_[part];
    const gstGeodePart& b_part = b->parts_[part];
    if (a_part != b_part &&
        (!reverse_ok || !IsEqualReverseOther(a_part, b_part))) {
      return false;
    }
  }

  return true;
}

// Assumes the first and last points are same, only parts_[0] and num vertices
// are equal
// NOTE: it is specific for gstPolyLine/gstStreet and used in PolylineJoiner.
bool gstGeode::IsEqual(const gstGeodeImpl* b, bool check_reverse) const {
  if (FlatPrimType() != gstPolyLine &&
      FlatPrimType() != gstStreet && NumParts() > 1) {
    notify(NFY_FATAL, "%s: Improper geometry type.", __func__);
  }

  if (b->FlatPrimType() != gstPolyLine && b->FlatPrimType() != gstStreet &&
      b->NumParts() > 1) {
    notify(NFY_FATAL, "%s: Improper geometry type.", __func__);
  }

  const gstGeodePart& a_part = parts_[0];
  const gstGeodePart& b_part = (static_cast<const gstGeode*>(b))->parts_[0];
  if (check_reverse ? !IsEqualExcludeFirstLastReverseOther(a_part, b_part)
                    : !IsEqualExcludeFirstLast(a_part, b_part)) {
    return false;
  }

  return true;
}

void gstGeode::ComputeBounds() const {
  if (IsEmpty())
    return;

  switch (PrimType()) {
    case gstUnknown:
      break;

    case gstPoint:
    case gstPoint25D:
    case gstPolyLine:
    case gstPolyLine25D:
    case gstStreet:
    case gstStreet25D:
      // Bounding box is calculated based on all parts because gstGeode can be
      // multi-point, multi-line.
      for (std::vector<gstGeodePart>::const_iterator p = parts_.begin();
           p != parts_.end(); ++p) {
        for (std::vector<gstVertex>::const_iterator v = p->begin();
             v != p->end(); ++v) {
          bounding_box_.Grow(*v);
        }
      }
      break;

    case gstPolygon:
    case gstPolygon25D:
    case gstPolygon3D:
#ifdef MULTI_POLYGON_ON
      // Bounding box of polygon is calculated based on outer cycle.
      for (std::vector<gstVertex>::const_iterator v = parts_[0].begin();
           v != parts_[0].end(); ++v) {
        bounding_box_.Grow(*v);
      }
#else
      for (std::vector<gstGeodePart>::const_iterator p = parts_.begin();
           p != parts_.end(); ++p) {
        for (std::vector<gstVertex>::const_iterator v = p->begin();
             v != p->end(); ++v) {
          bounding_box_.Grow(*v);
        }
      }
#endif
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      assert(false);
      break;
  }
}

gstBBox gstGeode::BoundingBoxOfPart(unsigned int p) const {
  assert(p < NumParts());

  gstBBox part_box_;
  const gstGeodePart& part = parts_[p];
  for (std::vector<gstVertex>::const_iterator v = part.begin();
       v != part.end(); ++v) {
    part_box_.Grow(*v);
  }

  return part_box_;
}

//
// Centroid and area of a polygon
//
// Assumes polygon is closed
// Area is positive for counterclockwise ordering of vertices,
// otherwise negative.
//
// Return values: 0 for normal execution; 1 if the polygon
// is degenerate (num of vertices < 3); and 2 if area = 0
//
// Graphics Gems IV, p3
// Gerard Bashein, Paul R. Detmer
//
// "The algebraic signs of the area will be positive when the
// vertices are ordered in a counterclockwise direction in the
// x-y plane, and negative otherwise."
//
int gstGeode::Centroid(unsigned int part, double* x, double* y, double* area) const {
  if (VertexCount(part) < kMinCycleVertices) {
    return 1;
  }
  // To avoid precision issues, need to offset the data using the center of the
  // bounding box, compute the centroid on the offset data, then add back the
  // offset.
  const gstVertex offset = BoundingBox().Center();
  double atmp = 0;
  double xtmp = 0;
  double ytmp = 0;
  // This counts the first and last vertex as the same.
  const unsigned int vertex_count = VertexCount(part);

  // The algorithm takes the cross product of the previous and current vertices.
  // Start with the ith vertex being the 0th vertex.
  // Note we iterate from 1 to vertex_count since vertex[0] == vertex[last].
  const gstVertex& vertex_last = GetVertex(part, 0);
  // Offset the vertex coordinates.
  double x_prev = vertex_last.x - offset.x;
  double y_prev = vertex_last.y - offset.y;
  for (unsigned int i = 1; i < vertex_count; ++i) {
    const gstVertex& vertex_i = GetVertex(part, i);
    // Offset the vertex coordinates.
    double x_i = vertex_i.x - offset.x;
    double y_i = vertex_i.y - offset.y;

    double ai = (x_prev * y_i) - (x_i * y_prev);
    atmp += ai;
    xtmp += (x_prev + x_i) * ai;
    ytmp += (y_prev + y_i) * ai;
    x_prev = x_i;
    y_prev = y_i;
  }

  *area = atmp / 2.0;
  if (atmp == 0) {
    return 2;
  }
  // The result coordinates are the weighted averages plus the original offset.
  *x = (xtmp / (3.0 * atmp)) + offset.x;
  *y = (ytmp / (3.0 * atmp)) + offset.y;
  return 0;
}

// TODO: What should be returned in case of some invalidity
// (e.g. degenerate primitive)?
// In gst/vectorprep/DisplayRule::GetSiteLocation() we return as center:
// (BoundingBox().CenterX() + offset_x_, BoundingBox().CenterY() + offset_y_),
//
// In tools/gevectorpoi.cpp: (BoundingBox().CenterX(), BoundingBox().CenterY())
//
gstVertex gstGeode::Center() const {
  if (IsDegenerate()) {
    //    center_ = gstVertex(BoundingBox().CenterX(), BoundingBox().CenterY());
    return center_;
  }

  switch (PrimType()) {
    case gstUnknown:
      // center_ = gstVertex(BoundingBox().CenterX(), BoundingBox().CenterY());
      break;

    case gstPoint:
    case gstPoint25D:
      center_ = GetFirstVertex(0);
      break;

    case gstPolyLine:
    case gstPolyLine25D:
    case gstStreet:
    case gstStreet25D:
      // TODO: gstGeode can contain more than one polyline?
      // How to calclulate a center in this case?
      center_ = GetVertex(0, VertexCount(0) >> 1);
      break;

    case gstPolygon:
    case gstPolygon25D:
    case gstPolygon3D: {
#ifdef MULTI_POLYGON_ON
      if (center_is_valid_)
        return center_;

      double area;
      double x, y;

      // First parameter of Centroid() is 0 (index of polygon's outer cycle);
      if (Centroid(0, &x, &y, &area) == 0) {
        gstVertex centroid = gstVertex(x, y);
        // First parameter of GetPointInPolygon() is 0 (index of polygon's
        // outer cycle);
        center_ = GetPointInPolygon(0, centroid);
      }
      // if (center_ == gstVertex()) {
      //  center_ = gstVertex(BoundingBox().CenterX(), BoundingBox().CenterY());
      // }
      center_is_valid_ = true;

#else
      if (center_is_valid_)
        return center_;
      double area;
      double largest_area = 0.0;
      double x, y;
      int largest_part_index = 0;
      gstVertex centroid_of_largest_part;
      for (unsigned int p = 0; p < NumParts(); ++p) {
        if (Centroid(p, &x, &y, &area) == 0) {
          if (fabs(area) > largest_area) {
            largest_area = fabs(area);
            centroid_of_largest_part = gstVertex(x, y);
            largest_part_index = p;
          }
        }
      }
      // Must return a point that actually lies in the largest polygon.
      center_ = GetPointInPolygon(largest_part_index, centroid_of_largest_part);
      center_is_valid_ = true;
#endif
    }
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      assert(false);
      break;
  }

  return center_;
}

bool gstGeode::IsConvex(unsigned int part_index) const {
  if (part_index >= parts_.size())
    return true;  // Empty is convex for our purposes.
  return gstPolygonUtils::IsConvex(parts_[part_index]);
}

gstVertex gstGeode::GetPointInPolygon(unsigned int part_index,
                                      const gstVertex& centroid) const {
  if (part_index >= parts_.size())
    return centroid;  // No part for this index. Return the centroid.

  const std::vector<gstVertex>& vertex_list_orig = parts_[part_index];
  // We need to catch the case where the vertex_list is not closed.
  // We do allow rendering lines as polygons.
  std::vector<gstVertex> vertex_list_copy;
  bool use_copy = false;
  if (vertex_list_orig.size() > 0 &&
      vertex_list_orig[0] != vertex_list_orig[vertex_list_orig.size()-1]) {
    // This is unclosed line, copy the vertices and close it.
    vertex_list_copy = vertex_list_orig;
    vertex_list_copy.push_back(vertex_list_orig[0]);
    use_copy = true;
  }
  const std::vector<gstVertex>& vertex_list =
    use_copy ? vertex_list_copy : vertex_list_orig;

  // We first test if the polygon is convex...if so the centroid is adequate.
  if (gstPolygonUtils::IsConvex(vertex_list))
    return centroid;  // No need to move, centroid is sufficient.

  // If it's a concave polygon, we now have to find a point inside the polygon
  // that is near the center of the polygon.
  // gstPolygonIntersector gives us a simple approximation that is fast
  // to compute.
  // NOTE: there will be some special cases which are not perfect (i.e.,
  // the point in the polygon may be too close to an edge (aesthetics), but this
  // will get a point in the polygon in any case.
  gstVertex vertex =
    gstPolygonUtils::FindPointInPolygonRelativeToPoint(vertex_list,
                                                       centroid);
  return vertex;
}

void gstGeode::InsertVertex(unsigned int p, unsigned int v, const gstVertex& vert) {
  std::vector<gstVertex>::iterator pos = parts_[p].begin() + v;
  parts_[p].insert(pos, vert);


  // Update the bbox only if valid.
  if (bounding_box_.Valid()) {
    // We're simply expanding the bbox...don't waste time to check containment.
    bounding_box_.Grow(vert);
  }
  InvalidateCachedData(false);  // No need to invalidate the bounding box here.
}

void gstGeode::DeleteVertex(unsigned int p, unsigned int v) {
  if ((p < parts_.size()) && (v < parts_[p].size())) {
    std::vector<gstVertex>::iterator pos = parts_[p].begin() + v;
    parts_[p].erase(pos);
  } else {
    assert(!"Attempting to delete invalid vertex!");
    return;
  }

  // We must reset the bbox since removing any vertex basically means we have
  // to recompute the entire bbox from scratch. We'll defer that to the
  // next call of BoundingBox().
  InvalidateCachedData(true);
}

void gstGeode::ModifyVertex(unsigned int p, unsigned int v, const gstVertex& vert) {
  assert((p < parts_.size()) && (v < parts_[p].size()));
  parts_[p][v] = vert;

  // We must reset the bbox since modifying any vertex basically means we have
  // to recompute the entire bbox from scratch. We'll defer that to the
  // next call of BoundingBox().
  InvalidateCachedData(true);
}

void gstGeode::AddVertex(const gstVertex& v) {
  if (parts_.size() == 0) {
    parts_.push_back(gstGeodePart());
  }
  parts_.back().push_back(v);
  // Update the bbox only if valid.
  if (bounding_box_.Valid()) {
    // We're simply expanding the bbox...don't waste time to check containment.
    bounding_box_.Grow(v);
  }
  InvalidateCachedData(false);  // No need to invalidate the bounding box here.
}

void gstGeode::AddVertexToPart0(const gstVertex& v) {
  if (parts_.size() == 0) {
    parts_.push_back(gstGeodePart());
  }
  parts_[0].push_back(v);
  // Update the bbox only if valid.
  if (bounding_box_.Valid()) {
    // We're simply expanding the bbox...don't waste time to check containment.
    bounding_box_.Grow(v);
  }
  InvalidateCachedData(false);  // No need to invalidate the bounding box here.
}

void gstGeode::AddPart(int sz) {
  parts_.push_back(gstGeodePart());
  parts_.back().reserve(sz);
}

void gstGeode::Draw(const gstDrawState& state,
                    const gstFeaturePreviewConfig &preview_config) const {
  float c[] = { 1.0, 1.0, 0, 1.0 };
  float line_width = 3;

  if (!(state.mode & DrawSelected)) {
    preview_config.glLineColor(c);
    line_width = preview_config.line_width_;
    glDisable(GL_LINE_STIPPLE);
  } else {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(3, 61166);
  }
  glColor4fv(c);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  switch (PrimType()) {
    case gstPoint:
    case gstPoint25D:
      glPointSize(3);
      glBegin(GL_POINTS);
      for (std::vector<gstGeodePart>::const_iterator p = parts_.begin();
           p != parts_.end(); ++p) {
        for (std::vector<gstVertex>::const_iterator v = p->begin();
             v != p->end(); ++v) {
          glVertex2d(v->x - state.frust.w, v->y - state.frust.s);
        }
      }
      glEnd();
      break;

    case gstPolyLine:
    case gstPolyLine25D:
    case gstStreet:
    case gstStreet25D:
      glLineWidth(line_width);
      for (std::vector<gstGeodePart>::const_iterator p = parts_.begin();
           p != parts_.end(); ++p) {
        glBegin(GL_LINE_STRIP);
        for (std::vector<gstVertex>::const_iterator v = p->begin();
             v != p->end(); ++v) {
          glVertex2d(v->x - state.frust.w, v->y - state.frust.s);
        }
        glEnd();
      }
      break;

    case gstPolygon:
    case gstPolygon25D:
    case gstPolygon3D:
      if (state.mode & DrawFilled) {
        float fill_color[4];
        preview_config.glPolyColor(fill_color);
        glColor4fv(fill_color);
        for (std::vector<gstGeodePart>::const_iterator p = parts_.begin();
             p != parts_.end(); ++p) {
          glBegin(GL_POLYGON);
          for (std::vector<gstVertex>::const_iterator v = p->begin();
               v != p->end(); ++v) {
            glVertex2d(v->x - state.frust.w, v->y - state.frust.s);
          }
          glEnd();
        }
        glColor4fv(c);
      }

      glLineWidth(line_width);
      for (std::vector<gstGeodePart>::const_iterator p = parts_.begin();
           p != parts_.end(); ++p) {
        glBegin(GL_LINE_LOOP);
        for (std::vector<gstVertex>::const_iterator v = p->begin();
             v != p->end(); ++v) {
          glVertex2d(v->x - state.frust.w, v->y - state.frust.s);
        }
        glEnd();
      }
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      assert(false);
      break;

    default:
      notify(NFY_WARN, "unknown primitive type: %d", PrimType());
      break;
  }

  if (state.mode & DrawVertexes) {
    glPointSize(5);
    glColor4f(1.0, 0.0, 0.8, 1.0);
    glBegin(GL_POINTS);
    for (std::vector<gstGeodePart>::const_iterator p = parts_.begin();
         p != parts_.end(); ++p) {
      for (std::vector<gstVertex>::const_iterator v = p->begin();
           v != p->end(); ++v) {
        glVertex2d(v->x - state.frust.w, v->y - state.frust.s);
      }
    }
    glEnd();
  }

  glLineWidth(1);

  if (state.mode & DrawBBoxes) {
    glColor3f(1.0, 1.0, 0.0);
    DRAW_BOX_2D(BoundingBox().w - state.frust.w,
                BoundingBox().e - state.frust.w,
                BoundingBox().s - state.frust.s,
                BoundingBox().n - state.frust.s);
  }
  glDisable(GL_BLEND);
  glDisable(GL_LINE_STIPPLE);
}

void gstGeodeImpl::DrawSelectedVertex(const gstDrawState& state,
                                      const gstVertex& vertex) {
  // size of circles around vertexes
  double sz = state.Scale()*10.0;
  glColor3f(1, 1, 0);
  glLineWidth(1);

  glPushMatrix();
  glTranslatef(vertex.x - state.frust.w, vertex.y - state.frust.s, 0);
  glScalef(sz, sz, 1);
  glCallList(VERTEX_SELECT);
  glPopMatrix();
}


// only one intersection is needed
bool gstGeode::IntersectDeep(const gstBBox& box, uint* part) const {
  *part = 0;
  switch (PrimType()) {
    case gstUnknown:
      /* NoOp */
      break;

    case gstPoint:
    case gstPoint25D:
      for (std::vector<gstGeodePart>::const_iterator p = parts_.begin();
           p != parts_.end(); ++p, ++(*part)) {
        for (std::vector<gstVertex>::const_iterator v = p->begin();
             v != p->end(); ++v) {
          if (box.Contains(*v))
            return true;
        }
      }
      break;

    case gstPolyLine:
    case gstPolyLine25D:
    case gstStreet:
    case gstStreet25D:
      for (unsigned int p = 0; p < NumParts(); ++p, ++(*part)) {
        for (unsigned int v = 0; v < VertexCount(p) - 1; ++v) {
          if (box.Intersect(GetVertex(p, v), GetVertex(p, v + 1)))
            return true;
        }
      }
      break;

    case gstPolygon:
    case gstPolygon25D:
    case gstPolygon3D:
      for (unsigned int p = 0; p < NumParts(); ++p, ++(*part)) {
        for (unsigned int v = 0; v < VertexCount(p) - 1; ++v) {
          if (box.Intersect(GetVertex(p, v), GetVertex(p, v + 1)))
            return true;
        }
        if (box.Intersect(GetFirstVertex(p), GetLastVertex(p)))
          return true;
      }

      //
      // XXX need to also check the interior of polygon
      //
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      assert(false);
      break;
  }

  return false;
}   // End gstGeode::intersect_deep()

//
// cut this geode by the supplied box
// consider all subparts as equal
//
// return the number of new geodes produced by the cut
// cut pieces are placed in supplied vector
//
#ifdef JOBSTATS_ENABLED
enum {
  JOBSTATS_INTERSECT, JOBSTATS_POINT, JOBSTATS_POLYLINE,
  JOBSTATS_POLYGON, JOBSTATS_POLYGON_WHOLE, JOBSTATS_POLYGON_PART,
  JOBSTATS_POLYGON_PART1, JOBSTATS_POLYGON_PART2,
  JOBSTATS_POLYGON_PART3, JOBSTATS_POLYGON_PART4,
  JOBSTATS_POLYGON3D
};

static gstJobStats::JobName JobNames[] = {
  {JOBSTATS_INTERSECT,     "Intersect   "},
  {JOBSTATS_POINT,         "Point       "},
  {JOBSTATS_POLYLINE,      "Polyline    "},
  {JOBSTATS_POLYGON,       "Polygon     "},
  {JOBSTATS_POLYGON_WHOLE, "+ Whole     "},
  {JOBSTATS_POLYGON_PART,  "+ Part      "},
  {JOBSTATS_POLYGON_PART1, "+ + Part 1  "},
  {JOBSTATS_POLYGON_PART2, "+ + Part 2  "},
  {JOBSTATS_POLYGON_PART3, "+ + Part 3  "},
  {JOBSTATS_POLYGON_PART4, "+ + Part 4  "},
  {JOBSTATS_POLYGON3D,     "Polygon3D   "}
};
gstJobStats* boxcut_stats = new gstJobStats("Geode Box Cut", JobNames, 11);
#endif


unsigned int gstGeode::Flatten(GeodeList* pieces) const {
  switch (PrimType()) {
    case gstUnknown:
    case gstPoint:
    case gstPoint25D:
      break;

    case gstPolyLine:
    case gstPolyLine25D:
      {
        // iterate across all segments, one at a time
        for (unsigned int p = 0; p < NumParts(); ++p) {
          for (unsigned int v = 0; v < VertexCount(p) - 1; ++v) {
            gstGeodeHandle new_geodeh = gstGeodeImpl::Create(PrimType());
            pieces->push_back(new_geodeh);

            gstGeode *new_geode = static_cast<gstGeode*>(&(*new_geodeh));
            new_geode->AddPart(2);  // To optimize with vector reserve.
            const gstVertex& v0 = GetVertex(p, v);
            new_geode->AddVertexToPart0(gstVertex(v0.x, v0.y, 0.0));
            const gstVertex v1 = GetVertex(p, v + 1);
            new_geode->AddVertexToPart0(gstVertex(v1.x, v1.y, 0.0));
          }
        }
      }
      break;

    case gstStreet:
    case gstStreet25D:
      {
        // iterate across all segments, one at a time
        for (unsigned int p = 0; p < NumParts(); ++p) {
          if (VertexCount(p) == 0) {
            continue;
          }
          gstGeodeHandle new_geodeh = gstGeodeImpl::Create(PrimType());
          pieces->push_back(new_geodeh);

          gstGeode *new_geode = static_cast<gstGeode*>(&(*new_geodeh));
          unsigned int num_vertices = VertexCount(p);
          new_geode->AddPart(num_vertices);  // To optimize with vector reserve.
          for (unsigned int v = 0; v < num_vertices; ++v) {
            const gstVertex& v0 = GetVertex(p, v);
            new_geode->AddVertexToPart0(gstVertex(v0.x, v0.y, 0.0));
          }
        }
      }
      break;


    case gstPolygon:
    case gstPolygon25D:
    case gstPolygon3D:
      {
        for (unsigned int part = 0; part < NumParts(); ++part) {
          gstGeodeHandle new_geodeh = gstGeodeImpl::Create(PrimType());
          pieces->push_back(new_geodeh);

          gstGeode *new_geode = static_cast<gstGeode*>(&(*new_geodeh));
          for (unsigned int i = 0; i < VertexCount(part); ++i) {
            new_geode->AddVertexToPart0(GetVertex(part, i));
            new_geode->AddEdgeFlag(kNormalEdge);
          }
        }
      }
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      assert(false);
      break;
  }

  return pieces->size();
}

// TODO: move this functionality to gstBoxCutter.
unsigned int gstGeode::BoxCutDeep(const gstBBox& box, GeodeList* pieces) const {
  JOBSTATS_SCOPED(boxcut_stats, JOBSTATS_INTERSECT);

  switch (PrimType()) {
    case gstUnknown:
    case gstPoint:
    case gstPoint25D:
      // do nothing
      {
      JOBSTATS_SCOPED(boxcut_stats, JOBSTATS_POINT);
      }
      break;

    case gstPolyLine:
    case gstStreet:
    case gstPolyLine25D:
    case gstStreet25D:
      break;

    case gstPolygon:
    case gstPolygon25D:
      {
      JOBSTATS_SCOPED(boxcut_stats, JOBSTATS_POLYGON);
      // multi-polygon workaround. we have to make the
      // box cutter do the right thing eventually
      // cut all parts of the mutipolygon separately
      for (unsigned int part = 0; part < NumParts(); ++part) {
        // check to see if the geode is completely
        // contained inside the quad
        if (box.Contains(BoundingBoxOfPart(part))) {
          JOBSTATS_SCOPED(boxcut_stats, JOBSTATS_POLYGON_WHOLE);
          gstGeodeHandle new_geodeh = gstGeodeImpl::Create(PrimType());
          gstGeode *new_geode = static_cast<gstGeode*>(&(*new_geodeh));
          for (unsigned int i = 0; i < VertexCount(part); ++i) {
            new_geode->AddVertexToPart0(GetVertex(part, i));
            new_geode->AddEdgeFlag(kNormalEdge);
          }
          pieces->push_back(new_geodeh);
          continue;
        }

        JOBSTATS_SCOPED(boxcut_stats, JOBSTATS_POLYGON_PART);

        const gstVertex alt = GetVertex(part, 0);
        pcPolygon in_poly;
        JOBSTATS_BEGIN(boxcut_stats, JOBSTATS_POLYGON_PART1);
        in_poly.Reserve(VertexCount(part) + 1);
        for (unsigned int v = 0; v < VertexCount(part); ++v) {
          in_poly.AddVertex(GetVertex(part, v));
        }
        JOBSTATS_END(boxcut_stats, JOBSTATS_POLYGON_PART1);

        JOBSTATS_BEGIN(boxcut_stats, JOBSTATS_POLYGON_PART2);
        pcPolygon out_poly;
        in_poly.ClipPolygon(&out_poly, box.w, box.e, box.s, box.n);
        JOBSTATS_END(boxcut_stats, JOBSTATS_POLYGON_PART2);

        unsigned int vert = 0;
        JOBSTATS_BEGIN(boxcut_stats, JOBSTATS_POLYGON_PART3);
        while (vert < out_poly.vertex_list.size()) {
          gstGeodeHandle new_geodeh = gstGeodeImpl::Create(PrimType());
          gstGeode *new_geode = static_cast<gstGeode*>(&(*new_geodeh));

          gstVertex prev_vertex(out_poly.vertex_list[vert].x,
                                out_poly.vertex_list[vert].y, alt.z);
          new_geode->AddVertexAndEdgeFlag(
              prev_vertex,
              out_poly.internal_edge[vert] ? kClipEdge : kNormalEdge);
          ++vert;
          gstVertex curr_vertex(out_poly.vertex_list[vert].x,
                                out_poly.vertex_list[vert].y, alt.z);
          while (curr_vertex != prev_vertex &&
                 vert < out_poly.vertex_list.size()) {
            new_geode->AddVertexAndEdgeFlag(
                curr_vertex,
                out_poly.internal_edge[vert] ? kClipEdge : kNormalEdge);
            prev_vertex = curr_vertex;
            ++vert;
            curr_vertex = gstVertex(out_poly.vertex_list[vert].x,
                                    out_poly.vertex_list[vert].y, alt.z);
          }
          pieces->push_back(new_geodeh);
          if (new_geode->VertexCount(0) <= 5) {
            JOBSTATS_SCOPED(boxcut_stats, JOBSTATS_POLYGON_PART4);
          }
          ++vert;
        }
        JOBSTATS_END(boxcut_stats, JOBSTATS_POLYGON_PART3);
      }
      }
      break;

    case gstPolygon3D:
      {
      // TODO: Find out why BoxCutDeep never looked into other parts of
      // the Geode. (i.e is it possible to have multi-polygon-3D for this case?)
      JOBSTATS_SCOPED(boxcut_stats, JOBSTATS_POLYGON3D);
      // check to see if the geode is completely
      // contained inside the quad
      if (box.Contains(BoundingBox())) {
          gstGeodeHandle new_geodeh = gstGeodeImpl::Create(PrimType());
          gstGeode *new_geode = static_cast<gstGeode*>(&(*new_geodeh));
          for (unsigned int i = 0; i < VertexCount(0); i++) {
            new_geode->AddVertexToPart0(GetVertex(0, i));
            new_geode->AddEdgeFlag(kNormalEdge);
          }
          pieces->push_back(new_geodeh);
        break;
      }
      if ( VertexCount(0) < 3 )
        break;

      // for 3D polygons we perform clipping in 2D and
      // compute z-values for clipped coords from the
      // plane equation.
      Vert3<double> N;
      double D;
      double north = box.n;
      double south = box.s;
      double west = box.w;
      double east = box.e;

      if (!ComputePlaneEquation(&N, &D)) {
        break;  // Plane equation couldn't be computed, so geode is invalid.
        // Such geodes should be filtered on stage of vector-import
        // so just break without notification.
      }

      pcPolygon in_poly;
      bool swapX = true;
      // in case of a vertical polygon we will swap either
      // the x with z or y with z depending upon the
      // the corresponding component of the normal and
      // then clip against the rotated
      if (!N.z && !N.x)
        swapX = false;

      for (unsigned int v = 0; v < VertexCount(0); ++v) {
        gstVertex vert = GetVertex(0, v);
        if (!N.z && swapX) {
          in_poly.AddVertex(pcVertex(vert.z, vert.y, 0, false));
          west = MinZ(0);
          east = MaxZ(0);
        } else if (!N.z && !swapX) {
          in_poly.AddVertex(pcVertex(vert.x, vert.z, 0, false));
          south = MinZ(0);
          north = MaxZ(0);
        } else {
          in_poly.AddVertex(pcVertex(vert.x, vert.y, 0, false));
        }
      }

      // clip the polygon in 2D
      pcPolygon out_poly;
      in_poly.ClipPolygon(&out_poly, west, east, south, north);

      // compute the 3rd component from the plane equation
      for (unsigned int i = 0; i < out_poly.vertex_list.size(); ++i) {
        if (!N.z && swapX) {
          double temp = (D - N.y * out_poly.vertex_list[i].y) / N.x;
          out_poly.vertex_list[i].z = out_poly.vertex_list[i].x;
          out_poly.vertex_list[i].x = temp;
        } else if (!N.z && !swapX) {
          double temp = (D - N.x * out_poly.vertex_list[i].x) / N.y;
          out_poly.vertex_list[i].z = out_poly.vertex_list[i].y;
          out_poly.vertex_list[i].y = temp;
        } else {
          out_poly.vertex_list[i].z = (D - N.x * out_poly.vertex_list[i].x -
                                       N.y * out_poly.vertex_list[i].y) / N.z;
        }
      }
      unsigned int i = 0;
      while (i < out_poly.vertex_list.size()) {
        gstGeodeHandle new_geodeh = gstGeodeImpl::Create(PrimType());
        gstGeode *new_geode = static_cast<gstGeode*>(&(*new_geodeh));
        gstVertex prevVertex(out_poly.vertex_list[i].x,
                             out_poly.vertex_list[i].y,
                             out_poly.vertex_list[i].z);
        new_geode->AddVertexToPart0(prevVertex);
        new_geode->AddEdgeFlag(
            out_poly.internal_edge[i] ? kClipEdge : kNormalEdge);
        ++i;
        gstVertex currVertex(out_poly.vertex_list[i].x,
                             out_poly.vertex_list[i].y,
                             out_poly.vertex_list[i].z);
        while (currVertex != prevVertex && i < out_poly.vertex_list.size()) {
          new_geode->AddVertexToPart0(currVertex);
          new_geode->AddEdgeFlag(
              out_poly.internal_edge[i] ? kClipEdge : kNormalEdge);
          prevVertex = currVertex;
          ++i;
          currVertex = gstVertex(out_poly.vertex_list[i].x,
                                 out_poly.vertex_list[i].y,
                                 out_poly.vertex_list[i].z);
        }
        pieces->push_back(new_geodeh);
        ++i;
      }
      }
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      assert(false);
      break;
  }

  return pieces->size();
}

// Simplify this geode so only the vertices appearing in the passed
// list remain.  Returns the number of vertices removed.
int gstGeode::Simplify(const std::vector<int>& vertices) {
  std::vector<bool> remaining(VertexCount(0), false);
  for (unsigned int i = 0; i < vertices.size(); ++i) {
    remaining[vertices[i]] = true;
  }

  const size_t orig_size = parts_[0].size();
  ShrinkToRemaining<gstGeodePart, gstGeodePart::iterator, true>(
      remaining, &parts_[0]);
  const size_t removed = orig_size - parts_[0].size();

  // Fix up the edge flags if necessary.
  if (PrimType() == gstPolygon) {
    std::vector<std::int8_t> new_edge_flags;
    new_edge_flags.reserve(parts_[0].size());
    for (unsigned int i = 0; i < remaining.size(); ++i) {
      if (remaining[i]) {
        new_edge_flags.push_back(edge_flags_[i]);
      }
    }
    std::swap(edge_flags_, new_edge_flags);
  }
  return static_cast<int>(removed);
}


// Every vertex must be within specified epsilon
// of at least one of our line segments
// Returns true if this is the case, otherwise returns false
// NOTE: Only operates on first subpart of geode!
//       This is true for any cut geometry during processing
//
// NOTE: it is specific for gstPolyLine/gstStreet and used
// in gstSelector and DisplayRule.
bool gstGeode::Overlaps(const gstGeode *that, double epsilon) {
  if ((FlatPrimType() != gstPolyLine &&
       FlatPrimType() != gstStreet) ||
      (that->FlatPrimType() != gstPolyLine &&
       that->FlatPrimType() != gstStreet))
    return false;

  if (NumParts() != 1 || that->NumParts() != 1)
    return false;

  // enlarge our bounding box in all four directions by epsilon
  // then confirm it is bigger than incoming geode's bbox
  gstBBox expand_box(BoundingBox().w + epsilon,
                     BoundingBox().e + epsilon,
                     BoundingBox().s + epsilon,
                     BoundingBox().n + epsilon);

  if (!expand_box.Contains(that->BoundingBox()))
    return false;

  //
  // every vertex must be within epsilon
  //
  // Compare every vertex of 'that' to every pair of
  // my vertexes until one is found within tolerance.
  // If any single vertex gets through all pairs without
  // an acceptable epsilon, abort.
  //
  {
    const gstVertex& last = that->GetLastVertex(0);
    bool have_winner = false;
    for (unsigned int v2 = 0; v2 < VertexCount(0) - 1; ++v2) {
      // if any vertex fails the test, we're done
      if (last.Distance(GetVertex(0, v2), GetVertex(0, v2 + 1)) < epsilon) {
        have_winner = true;
        break;
      }
    }
    if (have_winner == false)
      return false;
  }

  for (unsigned int v1 = 0; v1 < that->VertexCount(0) - 1; ++v1) {
    const gstVertex& vert1 = that->GetVertex(0, v1);
    bool have_winner = false;
    for (unsigned int v2 = 0; v2 < VertexCount(0) - 1; ++v2) {
      // if any vertex fails the test, we're done
      if (vert1.Distance(GetVertex(0, v2), GetVertex(0, v2 + 1)) < epsilon) {
        have_winner = true;
        break;
      }
    }
    if (have_winner == false)
      return false;
  }

  return true;
}

// TODO: function is not used
#if 0
bool gstGeode::Join(const gstGeodeHandle fromh) {
  // attempt to join two geodes
  // at least one end-point must match between the two
  //
  // this segment: A---B
  // from segment: Y---Z
  //
  // will attempt join in the following order
  //
  // 1.  A---B <> Y---Z
  // 2.  Y---Z <> A---B
  // 3.  A---B <> Z---Y
  // 4.  Z---Y <> A---B
  //
  // NOTE: this function only operates on part 0
  //

  // trivial reject if bounding boxes don't intersect
  // This is subject to floating point error in the bounding box code,
  // and a reject isn't necessary since this method is now only called
  // with segments guaranteed to share an endpoint.
  // if (!BoundingBox().intersect(from->BoundingBox()))
  // return false;

  if ((FlatPrimType() != gstPolyLine &&
       FlatPrimType() != gstStreet) ||
      (thath->FlatPrimType != gstPolyLine &&
       thath->FlatPrimType() != gstStreet))
    return false;

  const gstGeode *from = static_cast<const gstGeode*>(&(*fromh));

  if (GetLastVertex(0) == from->GetFirstVertex(0)) {
    parts_[0].reserve(parts_[0].size() + from->VertexCount(0) - 1);
    for (unsigned int v = 1; v < from->VertexCount(0); ++v)
      AddVertexToPart0(from->GetVertex(0, v));
  } else if (from->GetLastVertex(0) == GetFirstVertex(0)) {
    gstGeodePart part0;
    part0.swap(parts_[0]);
    parts_[0].reserve(part0.size() + from->VertexCount(0) - 1);
    for (unsigned int v = 0; v < from->VertexCount(0); ++v)
      AddVertexToPart0(from->GetVertex(0, v));
    for (unsigned int v = 1; v < part0.size(); ++v)
      AddVertexToPart0(part0[v]);
  } else if (GetLastVertex(0) == from->GetLastVertex(0)) {
    parts_[0].reserve(parts_[0].size() + from->VertexCount(0) - 1);
    for (int v = from->VertexCount(0) - 2; v >= 0; --v)
      AddVertexToPart0(from->GetVertex(0, v));
  } else if (from->GetFirstVertex(0) == GetFirstVertex(0)) {
    gstGeodePart part0;
    part0.swap(parts_[0]);
    parts_[0].reserve(part0.size() + from->VertexCount(0) - 1);
    for (unsigned int v = from->VertexCount(0) - 1; v != 0; --v)
      AddVertexToPart0(from->GetVertex(0, v));
    for (unsigned int v = 0; v < part0.size(); ++v)
      AddVertexToPart0(part0[v]);
  } else {
    return false;
  }

  return true;
}
#endif

void gstGeode::Join(const std::deque<std::pair<gstGeodeImpl*, bool> >&
                    to_join, const bool join_at_last_vertex) {
  if (FlatPrimType() != gstPolyLine &&
      FlatPrimType() != gstStreet &&
      NumParts() > 1) {
    notify(NFY_FATAL, "%s: Improper geometry type.", __func__);
  }

  size_t num_new_vertices = parts_[0].size();
  for (std::deque<std::pair<gstGeodeImpl*, bool> >::const_iterator i =
           to_join.begin(); i != to_join.end(); ++i) {
    if ((i->first)->FlatPrimType() != gstPolyLine &&
        (i->first)->FlatPrimType() != gstStreet &&
       (i->first)->NumParts() > 1) {
      notify(NFY_FATAL, "%s: Incorrect geode.", __func__);
    }
    const gstGeode* from = static_cast<const gstGeode*>(i->first);
    num_new_vertices += (from->VertexCount(0) - 1);
  }

  if (join_at_last_vertex) {
    parts_[0].reserve(num_new_vertices);
    for (std::deque<std::pair<gstGeodeImpl*, bool> >::const_iterator i =
             to_join.begin(); i != to_join.end(); ++i) {
      const gstGeode *from = static_cast<const gstGeode*>(i->first);
      const unsigned int v_end = from->VertexCount(0);
      const bool join_this_segment_at_first_vertex = i->second;
      if (join_this_segment_at_first_vertex) {
        // Add n-1 vertices [1 .. v_end-1]
        for (unsigned int v = 1; v < v_end; ++v) {
          AddVertexToPart0(from->GetVertex(0, v));
        }
      } else {  // Add n-1 vertices [v_end-2 .. 0]
        for (unsigned int v = v_end - 1; v > 0; ) {
          --v;
          AddVertexToPart0(from->GetVertex(0, v));
        }
      }
    }
  } else {
    // This is the case of adding in front. Since we have a vector need
    // to use a. backup, b. insert at beginning and c. append backup at end.
    gstGeodePart part0;
    part0.swap(parts_[0]);  // back-up parts_[0] to part0 and clear part0
    parts_[0].reserve(num_new_vertices);  // Allocate total memory required
    for (std::deque<std::pair<gstGeodeImpl*, bool> >::const_reverse_iterator i =
             to_join.rbegin(); i != to_join.rend(); ++i) {
      const gstGeode *from = static_cast<const gstGeode*>(i->first);
      const unsigned int v_end_minus_1 = from->VertexCount(0) - 1;
      const bool join_this_segment_at_first_vertex = i->second;
      if (join_this_segment_at_first_vertex) {
        // Add n-1 vertices [v_end-1 .. 1]
        for (unsigned int v = v_end_minus_1; v > 0; --v) {
          AddVertexToPart0(from->GetVertex(0, v));
        }
      } else {  // Add n-1 vertices [0 .. v_end-2]
        for (unsigned int v = 0; v < v_end_minus_1; ++v) {
          AddVertexToPart0(from->GetVertex(0, v));
        }
      }
    }
    // Append elements of part0 to end of parts_[0]
    parts_[0].insert(parts_[0].end(), part0.begin(), part0.end());
  }
}


 std::uint32_t gstGeode::RawSize() const {
  std::uint32_t size = sizeof(RecHeader);
  std::uint32_t vert_size_xy = sizeof(double) * 2;
  // The vertex has 3 coordinates for a 2.5d extensions.
  std::uint32_t vert_size_xyz = sizeof(double) * 3;

  switch (PrimType()) {
    case gstUnknown:
      break;
    case gstPoint:
      // all parts have exactly one vertex.
      size += (vert_size_xy * NumParts());
      break;

    case gstPoint25D:
      // all parts have exactly one vertex.
      size += (vert_size_xyz * NumParts());
      break;

    case gstPolyLine:
    case gstStreet:
    case gstPolygon:
      size += (sizeof(std::uint32_t) * 2) * NumParts();
      size += vert_size_xy * TotalVertexCount();
      break;

    case gstPolyLine25D:
    case gstStreet25D:
    case gstPolygon25D:
    case gstPolygon3D:
      size += sizeof(std::uint32_t) * 2 * NumParts();        // length is a double
      size += vert_size_xyz * TotalVertexCount();
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      assert(false);
      break;
  }

  return size;
}

char* gstGeode::ToRaw(char* buf) const {
  int buff_size = RawSize();

  // passing NULL means we allocate space
  if (buf == NULL)
    buf = reinterpret_cast<char*>(malloc(buff_size * sizeof(char)));

  RecHeader *hdr = reinterpret_cast<RecHeader*>(buf);
  hdr->type = PrimType();
  hdr->count = NumParts();
  hdr->size = buff_size;                 // size of this object
  hdr->fid = 0;                          // feature id

  char *tbuf = buf + sizeof(RecHeader);
  const std::uint32_t vert_size_xy = sizeof(double) * 2;
  const std::uint32_t vert_size_xyz = sizeof(double) * 3;

  // iterate through each feature
  for (unsigned int part = 0; part < NumParts(); ++part) {
    switch (PrimType()) {
      case gstUnknown:
        break;

      case gstPoint:
        {
          assert(VertexCount(part) == 1);
          double* pbuf = reinterpret_cast<double*>(tbuf);
          const gstVertex &vert = GetVertex(part, 0);
          pbuf[0] = vert.x;
          pbuf[1] = vert.y;
          tbuf += vert_size_xy;
        }
        break;

      case gstPoint25D:
        {
          assert(VertexCount(part) == 1);
          double* pbuf = reinterpret_cast<double*>(tbuf);
          const gstVertex &vert = GetVertex(part, 0);
          pbuf[0] = vert.x;
          pbuf[1] = vert.y;
          pbuf[2] = vert.z;
          tbuf += vert_size_xyz;
        }
        break;

      case gstPolyLine:
      case gstStreet:
      case gstPolygon:
        {
          std::uint32_t* num = reinterpret_cast<std::uint32_t*>(tbuf);
          *num = VertexCount(part);
          tbuf += sizeof(std::uint32_t) * 2;
          for (unsigned int v = 0; v < VertexCount(part); ++v) {
            double* pbuf = reinterpret_cast<double*>(tbuf);
            const gstVertex &vert = GetVertex(part, v);
            pbuf[0] = vert.x;
            pbuf[1] = vert.y;
            tbuf += vert_size_xy;
          }
        }
        break;

      case gstPolyLine25D:
      case gstStreet25D:
      case gstPolygon25D:
      case gstPolygon3D:
        {
          std::uint32_t* num = reinterpret_cast<std::uint32_t*>(tbuf);
          *num = VertexCount(part);
          tbuf += sizeof(std::uint32_t) * 2;
          for (unsigned int v = 0; v < VertexCount(part); ++v) {
            double* pbuf = reinterpret_cast<double*>(tbuf);
            const gstVertex &vert = GetVertex(part, v);
            pbuf[0] = vert.x;
            pbuf[1] = vert.y;
            pbuf[2] = vert.z;
            tbuf += vert_size_xyz;
          }
        }
        break;

      case gstMultiPolygon:
      case gstMultiPolygon25D:
      case gstMultiPolygon3D:
        assert(false);
        break;
    }
  }

  return buf;
}

void gstGeode::SetVertexes(const double* buf, std::uint32_t count, bool is25D) {
  AddPart(count);
  if (!is25D) {
    for (std::uint32_t v = 0; v < count; ++v) {
      AddVertex(gstVertex(buf[0], buf[1]));
      buf += 2;
    }
  } else {
    for (std::uint32_t v = 0; v < count; ++v) {
      AddVertex(gstVertex(buf[0], buf[1], buf[2]));
      buf += 3;
    }
  }
}


gstGeodeHandle gstGeodeImpl::FromRaw(const char* buf) {
  gstGeodeHandle new_geodeh;

  // validate header is one of ours
  const RecHeader *hdr = reinterpret_cast<const RecHeader*>(buf);
  if (hdr->size < sizeof(RecHeader))
    return new_geodeh;

  switch (gstPrimType(hdr->type)) {
    case gstUnknown:
      notify(NFY_WARN, "Invalid geometry type.");
      return new_geodeh;

    case gstPoint:
    case gstPoint25D:
    case gstPolyLine:
    case gstPolyLine25D:
    case gstStreet:
    case gstStreet25D:
    case gstPolygon:
    case gstPolygon25D:
    case gstPolygon3D:
      ReadPart(buf, &new_geodeh);
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      {
        new_geodeh = gstGeodeImpl::Create(gstPrimType(hdr->type));
        gstGeodeCollection *new_geode =
            static_cast<gstGeodeCollection*>(&(*new_geodeh));
        const char* fbuf = buf + sizeof(RecHeader);
        for (unsigned int p = 0; p < hdr->count; ++p) {
          gstGeodeHandle geode_part;
          fbuf = ReadPart(fbuf, &geode_part);
          if (fbuf && geode_part) {
            new_geode->AddGeode(geode_part);
          } else {
            new_geodeh.release();
            break;
          }
        }
      }
      break;

    default:
      notify(NFY_FATAL, "Invalid geometry type: %u", gstPrimType(hdr->type));
      break;
  }

  return new_geodeh;
}

bool gstGeode::IsClipped() const {
  if (clipped_ >= 0)
    return static_cast<bool>(clipped_);

  for (unsigned int i = 0; i < edge_flags_.size(); i++) {
    if (kClipEdge == edge_flags_[i]) {
      clipped_ = 1;
      return true;
    }
  }

  clipped_ = 0;
  return false;
}

bool gstGeode::ComputePlaneEquation(gstVertex *normal,
                                    double *distance) const {
  assert(PrimType() == gstPolygon3D);

  if (IsDegenerate()) {
    return false;
  }

  // Choose 3 not collinear points from outer boundary of polygon.
  // 1st point.
  gstVertex a(GetVertex(0, 0));

  // Get 2nd point  from polygon outer cycle.
  std::uint32_t i = 1;
  gstVertex b;
  std::uint32_t num_verts = VertexCount(0);
  for ( ; i < num_verts; ++i) {
    const gstVertex &cur_v(GetVertex(0, i));
    const gstVertex vec = a - cur_v;
    if (!fusion_gst::Equals(vec.squaredLength(), .0)) {
      b = cur_v;
      break;
    }
  }

  // Get 3rd point from polygon outer cycle.
  i++;
  gstVertex c;
  for ( ; i < num_verts; ++i) {
    const gstVertex &cur_v(GetVertex(0, i));
    if (!fusion_gst::Collinear3D(a, b, cur_v)) {
      c = cur_v;
      break;
    }
  }

  if (i < num_verts) {  // 3 not collinear points is found.
    return fusion_gst::ComputePlaneEquation(a, b, c, normal, distance);
  }

  return false;  // couldn't find 3 not collinear points to compute
  // plane equation.
}


// TODO: make a refactoring of processing
// cycle(last point processing) and process other parts.
// NOTE: Works only on part 0.
int gstGeode::RemoveCollinearVerts() {
  // remove collinear vertices in the source.
  assert(PrimType() == gstPolygon || PrimType() == gstPolygon25D);
  assert(NumParts());

  const int num_verts = TotalVertexCount();

  // make sure the first vertex is not duplicated
  while (VertexCount(0) != 0 && GetFirstVertex(0) == GetLastVertex(0))
    parts_[0].resize(parts_[0].size() - 1);

  // first vertex is not duplicated, so we check with (kMinCycleVertices - 1)
  if (VertexCount(0) < (kMinCycleVertices - 1)) {
    Clear();
    return num_verts;
  }

  std::vector< unsigned int>  remove_verts;
  gstVertex a = GetLastVertex(0);
  gstVertex b, c;
  int consecutive = 0;
  for (unsigned int v = 0; v < VertexCount(0) - 1; ++v) {
    b = GetVertex(0, v);
    c = GetVertex(0, v + 1);
    // keep every sixth consecutive collinear vertex
    // this is to preserve curved features
    if (fusion_gst::Collinear(a, b, c) && consecutive < 5) {
      consecutive++;
      remove_verts.push_back(v);
    } else {
      a = b;
      consecutive = 0;
    }
  }

  // special case for the last vertex
  b = GetLastVertex(0);
  c = GetFirstVertex(0);
  if (fusion_gst::Collinear(a, b, c)) {
    remove_verts.push_back(VertexCount(0) - 1);
  }

  if (remove_verts.size()) {
    RemoveAtIndices<std::vector< unsigned int> , std::vector< unsigned int> ::const_iterator,
                    gstGeodePart, gstGeodePart::iterator>(
      remove_verts, &parts_[0]);
  }

  if (VertexCount(0) < (kMinCycleVertices - 1)) {
    Clear();
    return num_verts;
  }

  AddVertexToPart0(GetFirstVertex(0));

  return remove_verts.size();
}


// TODO: function is not used!?
//
// Compute the angle between two geodes which share the given endpoint.
// Returns angles in radians between 0 and PI (inclusive)
double ComputeAngle(gstGeodeHandle ah, gstGeodeHandle bh, const gstVertex& v) {
  gstVertex va, vb;

  if (ah->PrimType() == gstMultiPolygon ||
      ah->PrimType() == gstMultiPolygon25D ||
      ah->PrimType() == gstMultiPolygon3D ||
      bh->PrimType() == gstMultiPolygon ||
      bh->PrimType() == gstMultiPolygon25D ||
      bh->PrimType() == gstMultiPolygon3D) {
    notify(NFY_FATAL, "%s: Improper geode type.", __func__);
  }

  const gstGeode *a = static_cast<const gstGeode*>(&(*ah));
  const gstGeode *b = static_cast<const gstGeode*>(&(*bh));

  if (a->GetFirstVertex(0) == v) {
    va = a->GetVertex(0, 1) - v;
  } else {
    va = a->GetVertex(0, a->VertexCount(0)-2) - v;
  }
  va.Normalize();

  if (b->GetFirstVertex(0) == v) {
    vb = b->GetVertex(0, 1) - v;
  } else {
    vb = b->GetVertex(0, b->VertexCount(0)-2) - v;
  }
  vb.Normalize();

  double scalar = va.x*vb.x + va.y*vb.y + va.z*vb.z;
  // acos() can only accept values between -1 to 1
  if (scalar > 1.0) {
    scalar = 1.0;
  } else if (scalar < -1.0) {
    scalar = -1.0;
  }
  return acos(scalar);
}

// GeodeCreator
void GeodeCreator::Report(gstGeodeHandle* const geodeh) const {
  assert(geodeh);
  switch (PrimType()) {
    case gstUnknown:
    case gstPoint:
    case gstPoint25D:
      break;

    case gstPolyLine:
    case gstStreet:
    case gstPolyLine25D:
    case gstStreet25D:
      break;

    case gstPolygon:
    case gstPolygon25D:
    case gstPolygon3D:
      if (geode_list_.empty()) {
        return;
      } else if (geode_list_.size() == 1) {
        *geodeh = geode_list_[0];
      } else {
        *geodeh = gstGeodeImpl::Create(GetMultiGeodeTypeFromSingle(gtype_));
        gstMultiPoly *multi_geode = static_cast<gstMultiPoly*>(&(**geodeh));
        for (GeodeList::const_iterator it = geode_list_.begin();
             it != geode_list_.end(); ++it) {
          multi_geode->AddGeode(*it);
        }
      }
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      {
        *geodeh = gstGeodeImpl::Create(gtype_);
        gstMultiPoly *multi_geode = static_cast<gstMultiPoly*>(&(**geodeh));
        for (GeodeList::const_iterator it = geode_list_.begin();
             it != geode_list_.end(); ++it) {
          multi_geode->AddGeode(*it);
        }
      }
      break;
  }
}

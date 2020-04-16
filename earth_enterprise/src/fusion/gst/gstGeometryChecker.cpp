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


#include "fusion/gst/gstGeometryChecker.h"

#include <assert.h>

#include "common/notify.h"
#include "fusion/gst/gstGeomUtils.h"


namespace fusion_gst {



template <typename Point>
bool IsIdenticalXY(const Point &left, const Point &right,
                   double tolerance = kXYToleranceNorm) {
  return EqualsXY(left, right, tolerance);
}

GeometryChecker::GeometryChecker()
    : less_xy_() {
}

GeometryChecker::~GeometryChecker() {
}

void GeometryChecker::Run(gstGeodeHandle* const geodeh) {
  notify(NFY_VERBOSE, "GeometryChecker::Run() ...");
  switch ((*geodeh)->PrimType()) {
    case gstUnknown:
    case gstPoint:
    case gstPoint25D:
      break;

    case gstPolyLine:
    case gstStreet:
    case gstPolyLine25D:
    case gstStreet25D:
      // TODO: clean consecutive identical vertices.
      break;

    case gstPolygon:
    case gstPolygon25D:
      ProcessPolygon(geodeh);
      break;

    case gstPolygon3D:
      ProcessPolygon3D(geodeh);
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
      {
        gstGeodeCollection *multi_geode =
            static_cast<gstGeodeCollection*>(&(**geodeh));
        for (unsigned int p = 0; p < multi_geode->NumParts(); ) {
          gstGeodeHandle &cur_geodeh = multi_geode->GetGeode(p);
          ProcessPolygon(&cur_geodeh);
          if (cur_geodeh->IsEmpty()) {
            // If part was empty or invalid (and forced empty), we remove it.
            notify(NFY_DEBUG, "Multi-geode part was empty or invalid"
                   " (and forced empty): skipped.");
            multi_geode->ErasePart(p);
          } else {
            ++p;
          }
        }
      }
      break;

    case gstMultiPolygon3D:
      {
        gstGeodeCollection *multi_geode =
            static_cast<gstGeodeCollection*>(&(**geodeh));
        for (unsigned int p = 0; p < multi_geode->NumParts(); ) {
          gstGeodeHandle &cur_geodeh = multi_geode->GetGeode(p);
          ProcessPolygon3D(&cur_geodeh);
          if (cur_geodeh->IsEmpty()) {
            // If part was empty or invalid (and forced empty), we remove it.
            notify(NFY_DEBUG, "Multi-geode part was empty or invalid"
                   " (and forced empty): skipped.");
            multi_geode->ErasePart(p);
          } else {
            ++p;
          }
        }
      }
      break;
  }

  notify(NFY_VERBOSE, "GeometryChecker::Run() done.");
}

void GeometryChecker::ProcessPolygon(gstGeodeHandle* const geodeh) {
  assert((*geodeh)->PrimType() == gstPolygon ||
         (*geodeh)->PrimType() == gstPolygon25D);

  if ((*geodeh)->IsDegenerate()) {
    (*geodeh)->Clear();
    notify(NFY_DEBUG,
           "Degenerate outer cycle of polygon.");
    return;  // geode is not valid;
  }

  // Remove coincident vertices.
  RemoveCoincidentVertices(geodeh);

  // Remove spikes.
  RemoveSpikes(geodeh);

  // Check and fix self-intersecting cycles: simple check - we check only
  // segments pair that have common joining segment. To fix the issue that are
  // specific for source data sets that have quad-partitioned polygons.
  CheckForSelfIntersection(geodeh);

  // Note: Check and fix incorrect cycle orientation functionality
  // is switched off because PolygonCleaner does it.
#if 0
  // Checks and Fixes cycle orientation.
  CheckAndFixCycleOrientation(geodeh);
#endif
}

void GeometryChecker::ProcessPolygon3D(gstGeodeHandle* const geodeh) {
  assert((*geodeh)->PrimType() == gstPolygon3D);

  if ((*geodeh)->IsDegenerate()) {
    (*geodeh)->Clear();
    return;
  }

  gstGeode* geode = static_cast<gstGeode*>(&(**geodeh));
  Vert3<double> normal;
  double distance;
  if (!geode->ComputePlaneEquation(&normal, &distance)) {
    notify(NFY_DEBUG, "Degenerate 3D polygon.");
    (*geodeh)->Clear();
    return;
  }

  bool swapX = true;
  // In case of a vertical polygon we will swap either the x with z or y with z
  // depending upon the corresponding component of the normal.
  if (!normal.z && !normal.x) {
    swapX = false;
  }

  // Convert to 2D-polygon.
  gstGeodeHandle geodeh_2d = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode_2d = static_cast<gstGeode*>(&(*geodeh_2d));
  for (unsigned int part = 0; part < geode->NumParts(); ++part) {
    geode_2d->AddPart(geode->VertexCount(part));
    for (unsigned int v = 0; v < geode->VertexCount(part); ++v) {
      const gstVertex &vert = geode->GetVertex(part, v);
      if (!normal.z && swapX) {  // TODO: optimize.
        geode_2d->AddVertex(gstVertex(vert.z, vert.y, .0));
      } else if (!normal.z && !swapX) {
        geode_2d->AddVertex(gstVertex(vert.x, vert.z, .0));
      } else {
        geode_2d->AddVertex(gstVertex(vert.x, vert.y, .0));
      }
    }
  }

  // Process polygon.
  ProcessPolygon(&geodeh_2d);

  // Convert back from processed 2D-polygon to 3D-polygon.
  geodeh_2d->ChangePrimType(gstPolygon3D);
  geode_2d = static_cast<gstGeode*>(&(*geodeh_2d));
  for (unsigned int part = 0; part < geode_2d->NumParts(); ++part) {
    for (unsigned int i = 0; i < geode_2d->VertexCount(part); ++i) {
      const gstVertex &vert = geode_2d->GetVertex(part, i);

      if (!normal.z && swapX) {  // TODO: optimize
        double new_x = (distance - normal.y * vert.y) / normal.x;
        geode_2d->ModifyVertex(part, i, gstVertex(new_x, vert.y, vert.x));
      } else if (!normal.z && !swapX) {
        double new_y = (distance - normal.x * vert.x) / normal.y;
        geode_2d->ModifyVertex(part, i, gstVertex(vert.x, new_y, vert.y));
      } else {
        double new_z =
            (distance - (normal.x * vert.x) - (normal.y * vert.y)) / normal.z;
        geode_2d->ModifyVertex(part, i, gstVertex(vert.x, vert.y, new_z));
      }
    }
  }

  (*geodeh).swap(geodeh_2d);
}


void GeometryChecker::RemoveCoincidentVertices(
    gstGeodeHandle* const geodeh) const {
  assert((*geodeh)->PrimType() == gstPolygon ||
         (*geodeh)->PrimType() == gstPolygon25D);

  if ((*geodeh)->IsDegenerate()) {
    (*geodeh)->Clear();
    return;
  }

  gstGeodeHandle new_geodeh = gstGeodeImpl::Create((*geodeh)->PrimType());
  gstGeode* new_geode = static_cast<gstGeode*>(&(*new_geodeh));

  gstGeode* geode = static_cast<gstGeode*>(&(**geodeh));
  for (unsigned int part = 0; part < geode->NumParts(); ++part) {
    unsigned int num_verts = geode->VertexCount(part);

    // check for min number of vertices in cycle.
    if (num_verts < kMinCycleVertices) {
      notify(NFY_WARN, "Degenerate polygon's inner cycle: skipped.");
      continue;  // Skip invalid inner cycle.
    }

#ifndef NDEBUG
    gstVertex tmp_pt = geode->GetVertex(part, 0);
#endif

    new_geode->AddPart(num_verts);

    // remove coincident vertices.
    gstVertex pt1 = geode->GetFirstVertex(part);
    new_geode->AddVertex(pt1);
    for (unsigned int v = 1; v < num_verts; ++v) {
      gstVertex pt2 = geode->GetVertex(part, v);
      if (!IsIdenticalXY(pt1, pt2)) {
        new_geode->AddVertex(pt2);
        pt1 = pt2;
      }
    }

    // Check new cycle (after removing coincident vertices) for min. number of
    // vertices in cycle
    if (new_geode->VertexCount(new_geode->NumParts()-1) < kMinCycleVertices) {
      if (0 == part) {   // Invalid outer cycle.
        notify(NFY_DEBUG,
               "Degenerate outer cycle after coincident vertices removing.");
        geode->Clear();
        return;  // Skip geode;
      } else {   // Invalid inner cycle.
        notify(NFY_WARN,
               "Degenerate polygon's inner cycle after removing of coincident"
               " vertices: skipped.");
#ifndef NDEBUG
        notify(NFY_WARN, "first point of degenerate cycle: (%.10f, %.10f)",
               khTilespace::Denormalize(tmp_pt.y),
               khTilespace::Denormalize(tmp_pt.x));
#endif
        // Skip invalid inner cycle.
        new_geode->EraseLastPart();
        continue;
      }
    }
  }

  (*geodeh).swap(new_geodeh);
  new_geodeh.release();
}

void GeometryChecker::RemoveSpikes(gstGeodeHandle* const geodeh) const {
  assert((*geodeh)->PrimType() == gstPolygon ||
         (*geodeh)->PrimType() == gstPolygon25D);

  if ((*geodeh)->IsDegenerate()) {
    (*geodeh)->Clear();
    return;
  }

  gstGeodeHandle new_geodeh = gstGeodeImpl::Create((*geodeh)->PrimType());
  gstGeode *new_geode = static_cast<gstGeode*>(&(*new_geodeh));

  gstGeode *geode = static_cast<gstGeode*>(&(**geodeh));

  for (unsigned int part = 0; part < geode->NumParts(); ++part) {
#ifndef NDEBUG
    gstVertex tmp_pt = geode->GetVertex(part, 0);
#endif

    if (geode->VertexCount(part) < kMinCycleVertices) {
      notify(NFY_WARN, "Degenerate inner cycle: skipped.");
      continue;
    }

    gstGeodePart res_part;
    RemoveSpikesInPart(*geode, part, &res_part);

    if (res_part.size() < kMinCycleVertices) {
      if (0 == part) {   // Invalid outer cycle.
        notify(NFY_DEBUG,
               "Degenerate outer cycle after spikes removing.");
        geode->Clear();
        return;  // Skip geode;
      } else {   // Invalid inner cycle.
        notify(NFY_WARN, "Degenerate polygon's inner cycle after spikes"
               " removing: skipped.");
#ifndef NDEBUG
        notify(NFY_WARN, "first point of degenerate cycle: (%.10f, %.10f)",
               khTilespace::Denormalize(tmp_pt.y),
               khTilespace::Denormalize(tmp_pt.x));
#endif
        // Just skip invalid inner cycle.
        continue;
      }
    } else {  // accept processed part.
      new_geode->AddPart(res_part.size());
      // TODO: AddVertex(begin, end);
      for (unsigned int v = 0; v < res_part.size(); ++v) {
        new_geode->AddVertex(res_part[v]);
      }
    }
  }

  (*geodeh).swap(new_geodeh);
  new_geodeh.release();
}

void GeometryChecker::RemoveSpikesInPart(const gstGeode &geode,
                                         unsigned int part,
                                         gstGeodePart *res_part) const {
  assert(res_part);
  unsigned int num_verts = geode.VertexCount(part);

  if (num_verts < kMinCycleVertices) {
    return;
  }

  gstGeodePart &cur_part = *res_part;

  cur_part.clear();
  cur_part.reserve(num_verts);
  for (unsigned int v = 0; v < geode.VertexCount(part); ++v) {
    cur_part.push_back(geode.GetVertex(part, v));
  }

  gstGeodePart new_part;
  new_part.reserve(num_verts);
  // Get last vertex (n-2) of cycle: (n-1) is a duplicate
  // of 1st vertex.
  gstVertex pt1 = cur_part[num_verts - 2];
  gstVertex pt2 = cur_part[0];
  gstVertex pt3;
  for (unsigned int v = 0; v < num_verts - 1; ) {
    pt3 = (v < num_verts - 2) ? cur_part[v + 1] : new_part[0];

    if (!IsSpike(pt1, pt2, pt3)) {
      new_part.push_back(pt2);
      pt1 = pt2;
      ++v;
      pt2 = cur_part[v];
    } else {
      // Check whether we get identical point after removing spike.
      if (IsIdenticalXY(pt1, pt3)) {
        ++v;  // skip pt3.
      }

      // Get last added point to check once more with new pt3
      // because current one we used to check was removed as spike.
      if (new_part.empty()) {
        pt1 = cur_part[num_verts - 2];
        ++v;
        pt2 = cur_part[v];
      } else {
        pt2 = pt1;
        assert(EqualsXY(pt2, new_part.back()));
        new_part.pop_back();
        pt1 = new_part.empty() ? cur_part[num_verts - 2] : new_part.back();
      }
    }
  }
  // Duplicate first point.
  new_part.push_back(new_part[0]);
  cur_part.swap(new_part);
  num_verts = cur_part.size();
}

bool GeometryChecker::IsSpike(const gstVertex &a,
                              const gstVertex &b,
                              const gstVertex &c) const {
  bool is_collinear = Collinear(b, a, c);

  if (is_collinear) {
    gstVert2D u(a.x - b.x, a.y - b.y);
    gstVert2D v(c.x - b.x, c.y - b.y);
    double dot = u.Dot(v);
    return (dot > .0);  // spike if acute angle (dot product > 0).
  }
  return false;  // not spike
}

void GeometryChecker::CheckForSelfIntersection(
    gstGeodeHandle* const geodeh) const {
  assert((*geodeh)->PrimType() == gstPolygon ||
         (*geodeh)->PrimType() == gstPolygon25D);

  if ((*geodeh)->IsDegenerate()) {
    (*geodeh)->Clear();
    return;
  }

  gstGeodeHandle new_geodeh = gstGeodeImpl::Create((*geodeh)->PrimType());
  gstGeode *new_geode = static_cast<gstGeode*>(&(*new_geodeh));

  gstGeode *geode = static_cast<gstGeode*>(&(**geodeh));
  unsigned int new_cur_p;
  for (unsigned int part = 0; part < geode->NumParts(); ++part) {
    unsigned int num_verts = geode->VertexCount(part);

    if (num_verts < kMinCycleVertices) {
      notify(NFY_WARN, "Degenerate inner cycle: skipped.");
      continue;
    }

    new_geode->AddPart(num_verts);
    new_cur_p = new_geode->NumParts() - 1;
#ifndef NDEBUG
    gstVertex tmp_pt = geode->GetVertex(part, 0);
#endif

    // Get last vertex (n-2) of cycle: (n-1) is a duplicate
    // of 1st vertex.
    gstVertex pt1 = geode->GetVertex(part, (num_verts - 2));
    gstVertex pt2 = geode->GetVertex(part, 0);
    gstVertex pt3;
    gstVertex pt4;

    for (unsigned int v = 0; v < num_verts - 1; ++v) {
      pt3 = (v + 1 < (num_verts - 1)) ? geode->GetVertex(part, (v + 1)) :
          new_geode->GetVertex(new_cur_p, (v + 1 - (num_verts - 1)));
      pt4 = (v + 2 < (num_verts - 1)) ?
          geode->GetVertex(part, (v + 2)) :
          new_geode->GetVertex(new_cur_p, (v + 2 - (num_verts - 1)));

      if (AreSegmentsIntersecting(pt1, pt2, pt3, pt4)) {
        // calculate intersection point.
        gstVertex intersection_pt = LinesIntersection2D(pt1, pt2, pt3, pt4);

        if (IsIdenticalXY(pt1, intersection_pt) ||
            IsIdenticalXY(pt4, intersection_pt)) {
          // skip intersection point if it is identical to pt1 or pt4.
          pt2 = pt4;
          ++v;
        } else {
          pt2 = intersection_pt;
        }
      } else {
        new_geode->AddVertex(pt2);
        pt1 = pt2;
        pt2 = pt3;
      }
    }

    // duplicate first vertex.
    new_geode->AddVertex(new_geode->GetVertex(new_cur_p, 0));

    if (new_geode->VertexCount(new_cur_p) < kMinCycleVertices) {
      if (0 == part) {   // Invalid outer cycle.
        notify(NFY_DEBUG,
               "Degenerate outer cycle after self-intersection fixing.");
        geode->Clear();
        return;  // Skip geode;
      } else {   // Invalid inner cycle.
        notify(NFY_WARN,
               "Degenerate polygon's inner cycle after self-intersection"
               " fixing: skipped.");
#ifndef NDEBUG
        notify(NFY_WARN, "first point of degenerate cycle: (%.10f, %.10f)",
               khTilespace::Denormalize(tmp_pt.y),
               khTilespace::Denormalize(tmp_pt.x));
#endif
        // Just skip invalid inner cycle.
        new_geode->EraseLastPart();
        continue;
      }
    }
  }

  (*geodeh).swap(new_geodeh);
  new_geodeh.release();
}

void GeometryChecker::CheckAndFixCycleOrientation(
    gstGeodeHandle* const geodeh) const {
  assert((*geodeh)->PrimType() == gstPolygon ||
         (*geodeh)->PrimType() == gstPolygon25D);

  if ((*geodeh)->IsDegenerate()) {
    (*geodeh)->Clear();
    return;
  }

  gstGeode *geode = static_cast<gstGeode*>(&(**geodeh));
  for (unsigned int part = 0; part < geode->NumParts(); ++part) {
    if (geode->VertexCount(part) < kMinCycleVertices) {
      // Invalidity of outer cycle is checked above in IsDegenerate().
      assert(part > 0);
      notify(NFY_WARN, "Degenerate inner cycle: skipped.");
      continue;
    }

    CycleOrientation cycle_orient = CalcCycleOrientation(geode, part);

    // Outer cycle (part == 0) should be counterclockwise.
    // Inner cycles (part > 0) should be clockwise.
    if ((0 == part && kClockWiseCycle == cycle_orient) ||
        (0 != part && kCounterClockWiseCycle == cycle_orient)) {
      notify(NFY_DEBUG,
             "Invalid cycle orientation (part %d): fixed.", part);
      // Fix cycle orientation.
      geode->ReversePart(part);
    }
  }
}

CycleOrientation GeometryChecker::CalcCycleOrientation(
    gstGeode* const geode, unsigned int part) const {
  // Calculate min vertex (it is most left lower one).
  gstVertex minv = geode->GetFirstVertex(part);
  unsigned int minv_idx = 0;
  unsigned int num_verts = geode->VertexCount(part);
  assert(num_verts != 0);
  for (unsigned int v = 1; v < (num_verts - 1); ++v) {
    const gstVertex &cur_v = geode->GetVertex(part, v);
    // Note: Here, the check should be w/o tolerance because
    // we need absolute most left-lower point.
    if (less_xy_(cur_v, minv)) {
      minv = cur_v;
      minv_idx = v;
    }
  }

  const gstVertex &a =
      geode->GetVertex(part,
                       (0 == minv_idx) ? num_verts - 2 : minv_idx - 1);
  const gstVertex &b = geode->GetVertex(part, minv_idx);
  const gstVertex &c =
      geode->GetVertex(part, minv_idx + 1);

  // Coincident points are removed before.
  assert(!IsIdenticalXY(a, b) && !IsIdenticalXY(b, c));

  const gstVert2D u((c.x - b.x), (c.y - b.y));
  const gstVert2D v((a.x - b.x), (a.y - b.y));

  const double cross = u.Cross(v);
  // Should be always not zero because we remove coincident points and spikes
  // before.
  assert(!Equals(cross, .0, kDblEpsilon));

  return (cross < .0 ? kClockWiseCycle : kCounterClockWiseCycle);
}

}  // namespace fusion_gst

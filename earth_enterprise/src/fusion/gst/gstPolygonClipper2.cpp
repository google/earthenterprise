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


#include "fusion/gst/gstPolygonClipper2.h"

#include <assert.h>
#include <algorithm>
#include <functional>

#include "fusion/gst/gstGeomUtils.h"
#include "fusion/gst/gstMathUtils.h"

#include "common/notify.h"
#include "common/khTileAddr.h"


namespace fusion_gst {

const PcInsideAreaLocationType PolygonClipper::kQuadEdgeAreaLoc_[
    kNumQuadEdges_] = {
  kInsideAreaLocBelow,  // kQuadEdgeLeft
  kInsideAreaLocAbove,  // kQuadEdgeRight
  kInsideAreaLocAbove,  // kQuadEdgeBottom
  kInsideAreaLocBelow   // kQuadEdgeTop
};

void PolygonClipper::PcTurningPoint::CalculateDirectionLeftRight(
    const PcHalfedgeHandle &he,
    const PcInsideAreaLocationType quad_edge_area_loc) {
  double dx = he->Right().x - he->Left().x;
  if (.0 != dx) {  // Not vertical halfedge.
    direction = (kInsideAreaLocAbove == he->inside_area_loc()) ?
        kDirectionUp : kDirectionDown;
  } else {  // Vertical halfedge - edge-touch case.
    CalculateDirectionLeftRightEdgeTouch(he, quad_edge_area_loc);
  }
}

void PolygonClipper::PcTurningPoint::CalculateDirectionBottomTop(
    const PcHalfedgeHandle &he,
    const PcInsideAreaLocationType quad_edge_area_loc) {
  double dy =  he->Right().y - he->Left().y;
  if (.0 != dy) {  // Not horizontal halfedge.
    CalculateDirectionBottomTopGeneric(he);
  } else {  // Horizontal halfedge - edge-touch case.
    CalculateDirectionBottomTopEdgeTouch(he, quad_edge_area_loc);
  }
}

void PolygonClipper::PcTurningPoint::CalculateDirectionLeftRightEdgeTouch(
    const PcHalfedgeHandle &he,
    const PcInsideAreaLocationType quad_edge_area_loc) {
  // Handling "edge touch"-case (polygon halfedge and quad edge are
  // coincident).
  const PcVertex &opp_edge_pt = Equals(y, he->v.y) ? he->pair->v : he->v;
  double dy_pt = opp_edge_pt.y - y;
  if (quad_edge_area_loc == he->inside_area_loc()) {  // the same direction.
    direction = (dy_pt < .0) ? kDirectionUp :  kDirectionDown;
  } else {
    // Set direction to None to skip adding turning points in case of
    // polygon halfedge and quad edge have opposite direction.
    direction = kDirectionNone;
    // Note: an alternative solution is to set proper direction
    // to build halfedge with opposite direction. So, the coincident
    // halfedges (one edge from clipping, another one from turning points)
    // will be skipped as a degenerate cycle. But in such case we need
    // to keep in turning point set all the turning points that should be
    // sorted based on scanline order of halfedges producing that turning
    // points (coordinates + direction).
    // direction = (dy_pt < .0) ? kDirectionDown :  kDirectionUp;
  }
}

void PolygonClipper::PcTurningPoint::CalculateDirectionBottomTopGeneric(
    const PcHalfedgeHandle &he) {
  // Generic case for bottom/top edges: halfedge is not horizontal.
  const PcVertex &opp_edge_pt = Equals(y, he->Left().y) ?
      he->Right() : he->Left();
  double dx = he->Right().x - he->Left().x;
  if (.0 == dx) {  // Vertical halfedge.
    direction = (kInsideAreaLocAbove == he->inside_area_loc()) ?
        kDirectionLeft : kDirectionRight;
    return;
  }

  double dy_pt = opp_edge_pt.y - y;
  double dx_pt = opp_edge_pt.x - x;
  if (kInsideAreaLocAbove == he->inside_area_loc()) {
    // Polygon inside area is above halfedge.
    if (dx_pt < .0) {
      direction = (dy_pt < .0) ? kDirectionLeft : kDirectionRight;
    } else {
      direction = (dy_pt < .0) ? kDirectionRight : kDirectionLeft;
    }
  } else {  // Polygon inside area is below halfedge.
    assert(kInsideAreaLocBelow == he->inside_area_loc());

    if (dx_pt < .0) {
      direction = (dy_pt < .0) ? kDirectionRight : kDirectionLeft;
    } else {
      direction = (dy_pt < .0) ? kDirectionLeft : kDirectionRight;
    }
  }
}

void PolygonClipper::PcTurningPoint::CalculateDirectionBottomTopEdgeTouch(
    const PcHalfedgeHandle &he,
    const PcInsideAreaLocationType quad_edge_area_loc) {
  // Handling "edge touch"-case (polygon halfedge and quad edge are
  // coincident).
  const PcVertex &opp_edge_pt = Equals(x, he->v.x) ? he->pair->v : he->v;
  double dx_pt = opp_edge_pt.x - x;
  if (quad_edge_area_loc == he->inside_area_loc()) {  // the same direction.
    direction = (dx_pt < 0) ? kDirectionRight : kDirectionLeft;
  } else {
    // Set direction to None to skip adding turning points in case of
    // polygon halfedge and quad edge have opposite direction.
    direction = kDirectionNone;
    // Note: an alternative solution is to set proper direction
    // to build halfedge with opposite direction. So, the coincident
    // halfedges (one edge from clipping, another one from turning points)
    // will be skipped as a degenerate cycle. But in such case we need
    // to keep in turning point set all the turning points that should be
    // sorted based on scanline order of halfedges producing that turning
    // points (coordinates + direction).
    // direction = (dx_pt < 0) ? kDirectionLeft : kDirectionRight;
  }
}


PolygonClipper::PolygonClipper(bool cut_holes)
    : in_halfedges_acceptor_(&in_halfedges_),
      polygon_builder_(cut_holes
                       ? PolygonBuilderOptions::CUT_HOLES_NO_CLEAN()
                       : PolygonBuilderOptions::NO_CUT_HOLES_NO_CLEAN()) {
  Init();
}

PolygonClipper::PolygonClipper(const gstBBox &_box, bool cut_holes)
    : in_halfedges_acceptor_(&in_halfedges_),
      polygon_builder_(cut_holes
                       ? PolygonBuilderOptions::CUT_HOLES_NO_CLEAN()
                       : PolygonBuilderOptions::NO_CUT_HOLES_NO_CLEAN()) {
  Init();
  SetClipRect(_box);
}

PolygonClipper::~PolygonClipper() {
}

void PolygonClipper::Run(const gstGeodeHandle &geodeh,
                         GeodeList *pieces,
                         bool *completely_covered) {
  notify(NFY_VERBOSE, "PolygonClipper::Run() ...");
  assert(pieces);
  assert(completely_covered);

  notify(NFY_VERBOSE, "Clipping quad:\n"
         "\tw: %f, e: %f,\n\ts: %f, n: %f",
         khTilespace::Denormalize(wx1_), khTilespace::Denormalize(wx2_),
         khTilespace::Denormalize(wy1_), khTilespace::Denormalize(wy2_));

  notify(NFY_VERBOSE, "Clipping quad normalized:\n"
         "\twx1: %.20f, wx2: %.20f\n\twy1: %.20f, wy2: %.20f",
         wx1_, wx2_, wy1_, wy2_);

  if (geodeh->PrimType() == gstMultiPolygon ||
      geodeh->PrimType() == gstMultiPolygon25D ||
      geodeh->PrimType() == gstMultiPolygon3D) {
    // Check for bounding boxes intersection.
    if (!box_.Intersect(geodeh->BoundingBox())) {
      return;  // multi-polygon is outside of clipping window.
    }

    const gstGeodeCollection *multi_geode =
        static_cast<const gstGeodeCollection*>(&(*geodeh));
    // Process independently each part(polygon) of multi-polygon geometry.
    for (unsigned int p = 0; p < multi_geode->NumParts(); ++p) {
      bool _completely_covered = false;
      const gstGeodeHandle &cur_geodeh = multi_geode->GetGeode(p);
      const gstGeode *cur_geode = static_cast<const gstGeode*>(&(*cur_geodeh));
      RunPolygon(cur_geode, pieces, &_completely_covered);
      *completely_covered = *completely_covered || _completely_covered;
    }
  } else {
    const gstGeode *geode = static_cast<const gstGeode*>(&(*geodeh));
    RunPolygon(geode, pieces, completely_covered);
  }
  notify(NFY_VERBOSE, "PolygonClipper::Run() done.");
}

void PolygonClipper::RunPolygon(const gstGeode *geode,
                                GeodeList *pieces,
                                bool *completely_covered) {
  notify(NFY_VERBOSE, "%s ...", __func__);
  assert(geode);
  assert(pieces);
  assert(completely_covered);

  if (geode->IsDegenerate()) {
    notify(NFY_WARN, "Degenerate polygon.");
    return;
  }

  // Check for bounding boxes intersection.
  if (!box_.Intersect(geode->BoundingBox())) {
    return;  // polygon is outside of clipping window.
  }

  // 1. Reset internal intermediate structures.
  Reset();

  // 2. Accept input data and convert from Geode to internal representation.
  if (AcceptPolygon(geode, pieces)) {
    // Polygon lies inside the clipping window (further processing is not
    // required).
    return;
  }

  // 3. Polygon Clipping step.
  ClipPolygon();

  // 4. Preprocessing for special cases.
  bool is_clipped = false;
  for (int i = 0; i < kNumQuadEdges_; i++) {
    is_clipped |= quad_edges_[i].has_halfedge;
  }

  notify(NFY_VERBOSE, "is_clipped: %d", is_clipped);

  if (is_clipped) {
    // Create halfedges based on quad's corners that lie inside polygon.
    CreateHalfedgesFromInnerQuadCorners();
  } else if (!in_halfedges_.empty()) {
    // It's the case when clipping window lies completely inside or outside
    // a polygon.
    int ret = ProcessSpecialCases();
    notify(NFY_VERBOSE,
           "Special case (clipping quad is outside-0/inside-1,2 polygon): %d",
           ret);
    if (0 == ret) {
      Reset();
      return;
    } else if (1 == ret) {
      gstGeodeHandle new_geodeh =
          gstGeodeImpl::Create(box_, kClipEdge, geode->PrimType(), height25D_);
      pieces->push_back(new_geodeh);
      *completely_covered = true;
      Reset();
      return;
    }

    assert(2 == ret);
  }

  // 5. Polygon Reconstruction step.
  polygon_builder_.SetPrimType(geode->PrimType());
  polygon_builder_.Run(pieces);

  // 7. Reset internal intermediate structures.
  Reset();
  notify(NFY_VERBOSE, "%s done.", __func__);
}

void PolygonClipper::Init() {
  height25D_ = .0;

  turning_point_sets_.resize(kNumQuadEdges_);

  // Initialize quad edge to corner mapping array;
  quad_edges_[kQuadEdgeLeft] =
      QuadEdge(kQuadCornerBottomLeft, kQuadCornerTopLeft);

  quad_edges_[kQuadEdgeRight] =
      QuadEdge(kQuadCornerBottomRight, kQuadCornerTopRight);

  quad_edges_[kQuadEdgeBottom] =
      QuadEdge(kQuadCornerBottomLeft, kQuadCornerBottomRight);

  quad_edges_[kQuadEdgeTop] =
      QuadEdge(kQuadCornerTopLeft, kQuadCornerTopRight);
}

void PolygonClipper::Reset() {
  // Clear input halfedges;
  ClearHalfedgesList(&in_halfedges_);

  // Reset flags in auxiliary data structures.
  for (int i = 0; i < kNumQuadEdges_; i++) {
    quad_edges_[i].has_halfedge = false;
    quad_corners_[i].is_inside = false;
  }

  // Clear turning point sets;
  std::for_each(turning_point_sets_.begin(), turning_point_sets_.end(),
                std::mem_fun_ref(&PcTurningPointSet::clear));

  special_sweep_status_.Clear();

  polygon_builder_.Reset();
}

bool PolygonClipper::AcceptPolygon(const gstGeode *geode, GeodeList *pieces) {
  notify(NFY_VERBOSE, "%s ...", __func__);

  assert(!geode->IsDegenerate());
  // Get height.
  height25D_ = geode->GetVertex(0, 0).z;

  // Check if geode is completely inside clipping window.
  if (box_.Contains(geode->BoundingBox())) {
    notify(NFY_VERBOSE, "Polygon is completely inside quad.");

    if (geode->NumParts() == 1) {
      // optimization: do not run processor in case a polygon lies inside
      // clipping window and doesn't have holes.
      // TODO: add gstGeode::DuplicateAndAddEdgeFlag(bool);
      gstGeodeHandle new_geodeh = gstGeodeImpl::Create(geode->PrimType());
      gstGeode *new_geode = static_cast<gstGeode*>(&(*new_geodeh));

      new_geode->AddPart(geode->VertexCount(0));
      for (unsigned int v = 0; v < geode->VertexCount(0); ++v) {
        new_geode->AddVertexAndEdgeFlag(
            geode->GetVertex(0, v), kNormalEdge);
      }
      pieces->push_back(new_geodeh);
      return true;  // Polygon lies inside a clipping window (further
      // processing is not required).
    }

    // Accept data for further processing.
    for (unsigned int part = 0; part < geode->NumParts(); ++part) {
      notify(NFY_VERBOSE, "number vertexes in geode part %d: %d", part,
             geode->VertexCount(part));
      polygon_builder_.AcceptPolygonPart(geode, part);
    }
  } else {
    for (unsigned int part = 0; part < geode->NumParts(); ++part) {
      notify(NFY_VERBOSE, "number vertexes in geode part %d: %d", part,
             geode->VertexCount(part));
      if (box_.Contains(geode->BoundingBoxOfPart(part))) {
        notify(NFY_VERBOSE, "Quad contains polygon's inner cycle.");
         polygon_builder_.AcceptPolygonPart(geode, part);
      } else {
        in_halfedges_acceptor_.Accept(geode, part);
      }
    }
  }
  notify(NFY_VERBOSE, "%s done.", __func__);

  return false;  // Further processing is required.
}

void PolygonClipper::ClipPolygon() {
  notify(NFY_VERBOSE, "%s ...", __func__);

  // Clip all halfedges and insert clipped ones into output halfedges set.
  for (PcHalfedgeHandleList::iterator it = in_halfedges_.begin();
       it != in_halfedges_.end(); ++it) {
    const PcHalfedgeHandle &source_he = *it;
    PcHalfedgeHandle clipped_he;
    PcVertex intersection;
    bool is_intersection_point = false;

    // Clip only left halfedge from pair.
    if (kDominatingLeft == source_he->dominating) {
      // Skip source halfedges that do not intersect quad by checking their
      // end points: vertex-touch case is considered no intersection,
      // while edge-touch case is an intersection.
      // Note: we need to run cutting for edge-touch case to provide setting
      // of correct type for edges in resulting polygon.
      double ymin = 0;
      double ymax = 0;
      if (source_he->v.y < source_he->pair->v.y) {
        ymin = source_he->v.y;
        ymax = source_he->pair->v.y;
      } else {
        ymin = source_he->pair->v.y;
        ymax = source_he->v.y;
      }
      if ((source_he->pair->v.x <= wx1_ && source_he->v.x < wx1_) ||
          (source_he->v.x >= wx2_ && source_he->pair->v.x > wx2_) ||
          (ymax <= wy1_ && ymin < wy1_) ||
          (ymin >= wy2_ && ymax > wy2_)) {
        notify(NFY_VERBOSE,
               "Source he: (%.20f, %.20f)-(%.20f, %.20f), inside_above: %d,"
               " no intersection",
               source_he->v.x, source_he->v.y,
               source_he->pair->v.x, source_he->pair->v.y,
               kInsideAreaLocAbove == source_he->inside_area_loc());
        continue;
      }

      notify(NFY_VERBOSE,
             "Source he: (%.20f, %.20f)-(%.20f, %.20f), inside_above: %d",
             source_he->v.x, source_he->v.y,
             source_he->pair->v.x, source_he->pair->v.y,
             kInsideAreaLocAbove == source_he->inside_area_loc());

      if (ClipHalfedge(source_he, &clipped_he,
                       &intersection, &is_intersection_point)) {
        if (is_intersection_point) {
          EvaluateTurningPoint(intersection, source_he);
        } else {
          notify(NFY_VERBOSE, "Clipped he: (%.20f, %.20f)-(%.20f, %.20f)",
                 clipped_he->v.x, clipped_he->v.y,
                 clipped_he->pair->v.x, clipped_he->pair->v.y);
          polygon_builder_.AcceptHalfedge(clipped_he);
          polygon_builder_.AcceptHalfedge(clipped_he->pair);
          EvaluateTurningPoint(clipped_he->Left(), clipped_he);
          EvaluateTurningPoint(clipped_he->Right(), clipped_he);
        }
      }
    }
  }

  // Create halfedges from turning point.
  // Process independently turning points that lie on the left/right/top/bottom
  // edge of clipping window.
  for (int i = 0; i < kNumQuadEdges_; i++) {
    CreateHalfedgesFromTurningPoints(static_cast<QuadEdgeType>(i));
  }

  notify(NFY_VERBOSE, "%s done.", __func__);
}

bool PolygonClipper::ClipHalfedge(const PcHalfedgeHandle &in_he,
                                  PcHalfedgeHandle *const out_he,
                                  PcVertex *const intersection,
                                  bool *const is_intersection_point) const {
  notify(NFY_VERBOSE, "%s ...", __func__);
  assert(out_he != NULL);
  assert(intersection != NULL);
  assert(is_intersection_point != NULL);

  *is_intersection_point = false;
  const PcVertex &v1 = in_he->v;
  const PcVertex &v2 = in_he->pair->v;
  gstVertex pt1(v1.x, v1.y, v1.z);
  gstVertex pt2(v2.x, v2.y, v2.z);

  gstVertexVector intersections;
  // TODO: leave only one branch.
#if 0
  if (segment_clipper_.Run(pt1, pt2, intersections)) {
    assert(intersections.size() == 2);
    if (Equals(intersections[0].x, intersections[1].x) &&
        Equals(intersections[0].y, intersections[1].y)) {
      *intersection = PcVertex(intersections[0]);
      *is_intersection_point = true;
    } else {                          // Intersection is segment;
      *out_he = PcHalfedge::CreatePair(
          intersections[0], intersections[1], in_he);
    }
    return true;
  }
#else
  int clipped = 0;
  if (clipped = box_.ClipLineAccurate(&pt1, &pt2)) {
    if (2 == clipped) {  // Segment was cut.
      // Calculate z-coordinate for clipping points.
      GetLineZFromXY(v1, v2, &pt1);
      GetLineZFromXY(v1, v2, &pt2);
    }

    if (Equals(pt1.x, pt2.x) && Equals(pt1.y, pt2.y)) {
      *intersection = PcVertex(pt1);
      *is_intersection_point = true;
    } else {                          // Intersection is segment;
      *out_he = PcHalfedge::CreatePair(pt1, pt2, in_he);
    }
    return true;
  }
#endif

  return false;  // No intersection.
}

void PolygonClipper::EvaluateTurningPoint(const PcVertex &pt,
                                          const PcHalfedgeHandle &he) {
  PcTurningPoint tpt(pt);

  std::pair<PcTurningPointSet::iterator, bool> ret;

  notify(NFY_VERBOSE, "EvaluateTurningPoint: (%.20f, %.20f)", tpt.x, tpt.y);

  if (Equals(tpt.x, wx1_)) {           // Left edge.
    notify(NFY_VERBOSE, "Add turning point to left edge...");

    tpt.CalculateDirectionLeftRight(he, kQuadEdgeAreaLoc_[kQuadEdgeLeft]);

    if (tpt.direction != kDirectionNone) {
      notify(NFY_VERBOSE, " (%.20f, %.20f), direction: %d",
             tpt.x, tpt.y, tpt.direction);

      ret = turning_point_sets_[kQuadEdgeLeft].insert(tpt);

      // Remove turning point if it is duplicated and has opposite direction
      // in order to exclude degenerate edges. Do the same for all quad's edges.
      if (!ret.second && ret.first->direction != tpt.direction) {
        turning_point_sets_[kQuadEdgeLeft].erase(ret.first);
      }
    }
  } else if (Equals(tpt.x, wx2_)) {   // Right edge.
    notify(NFY_VERBOSE, "Add turning point to right edge...");

    tpt.CalculateDirectionLeftRight(he, kQuadEdgeAreaLoc_[kQuadEdgeRight]);

    if (tpt.direction != kDirectionNone) {
      notify(NFY_VERBOSE, " (%.20f, %.20f), direction: %d",
             tpt.x, tpt.y, tpt.direction);

      ret = turning_point_sets_[kQuadEdgeRight].insert(tpt);
      if (!ret.second && ret.first->direction != tpt.direction) {
        turning_point_sets_[kQuadEdgeRight].erase(ret.first);
      }
    }
  }

  if (Equals(tpt.y, wy1_)) {          // Bottom edge
    notify(NFY_VERBOSE, "Add turning point to bottom edge...");

    tpt.CalculateDirectionBottomTop(he, kQuadEdgeAreaLoc_[kQuadEdgeBottom]);

    if (tpt.direction != kDirectionNone) {
      notify(NFY_VERBOSE, " (%.20f, %.20f), direction: %d",
             tpt.x, tpt.y, tpt.direction);

      ret = turning_point_sets_[kQuadEdgeBottom].insert(tpt);
      if (!ret.second && ret.first->direction != tpt.direction) {
        turning_point_sets_[kQuadEdgeBottom].erase(ret.first);
      }
    }
  } else if (Equals(tpt.y, wy2_)) {   // Top edge
    notify(NFY_VERBOSE, "Add turning point to top edge...");

    tpt.CalculateDirectionBottomTop(he, kQuadEdgeAreaLoc_[kQuadEdgeTop]);

    if (tpt.direction != kDirectionNone) {
      notify(NFY_VERBOSE, " (%.20f, %.20f), direction: %d",
             tpt.x, tpt.y, tpt.direction);

      ret = turning_point_sets_[kQuadEdgeTop].insert(tpt);
      if (!ret.second && ret.first->direction != tpt.direction) {
        turning_point_sets_[kQuadEdgeTop].erase(ret.first);
      }
    }
  }
}

void PolygonClipper::CreateHalfedgesFromTurningPoints(
    const QuadEdgeType quad_edge_type) {
  notify(NFY_VERBOSE, "%s ...", __func__);

  // Get the set of turning points of current quad edge.
  const PcTurningPointSet &tpt_set = turning_point_sets_[quad_edge_type];

  if (tpt_set.empty()) {
    return;  // There are no turning points.
  }

  gstVertex pt_begin;
  gstVertex pt_end;
  PcInsideAreaLocationType inside_area_loc;
  // Initialize begin/end points and inside_area_location of edge based on
  // quad_edge_type.
  GetQuadEdgeByType(quad_edge_type, &pt_begin, &pt_end, &inside_area_loc);

  PcTurningPointSet::const_iterator it_cur = tpt_set.begin();
  // Check for segment existance from first turning point in list to begin
  // point of quad edge and create pair of halfedges if so.
  const PcTurningPoint &tpt_beg = *it_cur;
  if ((kDirectionLeft == tpt_beg.direction) ||
      (kDirectionDown == tpt_beg.direction)) {
    // Create new pair of halfedges.
    PcHalfedgeHandle new_he = PcHalfedge::CreateInternalPair(
        gstVertex(tpt_beg.x, tpt_beg.y, tpt_beg.z), pt_begin,
        inside_area_loc, kClipEdge);

    if (!new_he->IsDegenerate()) {  // To skip degenerate edges.
      polygon_builder_.AcceptHalfedge(new_he);
      polygon_builder_.AcceptHalfedge(new_he->pair);
    }
    ++it_cur;

    notify(NFY_VERBOSE,
           "TPT beg he: (%.20f, %.20f)-(%.20f, %.20f), IsDegenerate: %d",
           new_he->v.x, new_he->v.y, new_he->pair->v.x, new_he->pair->v.y,
           new_he->IsDegenerate());

    // Set flag "quad edge has halfedge".
    quad_edges_[quad_edge_type].has_halfedge = true;
    // Do not set "is inside" for quad edge corner in case of a degenerate
    // halfedge (a vertex-touch case is considered no intersection).
    if (!new_he->IsDegenerate()) {
      // Set flag "quad corner is inside polygon".
      quad_corners_[
          quad_edges_[quad_edge_type].begin_corner_idx].is_inside = true;
    }
  }

  if (tpt_set.end() == it_cur) {
    return;  // All turning points are processed.
  }

  // Check for segment existance from last turning point in list to end point
  // of quad edge and create pair of halfedges if so.
  PcTurningPointSet::const_iterator it_end = tpt_set.end();
  --it_end;
  const PcTurningPoint &tpt_end = *it_end;
  if ((kDirectionRight == tpt_end.direction) ||
      (kDirectionUp == tpt_end.direction)) {
    // Create new pair of halfedges.
    PcHalfedgeHandle new_he = PcHalfedge::CreateInternalPair(
        gstVertex(tpt_end.x, tpt_end.y, tpt_end.z), pt_end,
        inside_area_loc, kClipEdge);

    if (!new_he->IsDegenerate()) {  // To skip degenerate edges.
      polygon_builder_.AcceptHalfedge(new_he);
      polygon_builder_.AcceptHalfedge(new_he->pair);
    }

    notify(NFY_VERBOSE,
           "TPT end he: (%.20f, %.20f)-(%.20f, %.20f), IsDegenerate: %d",
           new_he->v.x, new_he->v.y, new_he->pair->v.x, new_he->pair->v.y,
           new_he->IsDegenerate());

    // Set flag "quad edge has halfedge".
    quad_edges_[quad_edge_type].has_halfedge = true;
    // Do not set "is inside" for quad edge corner in case of a degenerate
    // halfedge (vertex-touch case is considered no intersection).
    if (!new_he->IsDegenerate()) {
      // Set flag "quad corner is inside polygon".
      quad_corners_[
          quad_edges_[quad_edge_type].end_corner_idx].is_inside = true;
    }

    if (it_cur == it_end) {
      return;  // All turning points are processed.
    }
    --it_end;
  }

  if (it_cur == it_end) {
    return;  // All turning points are processed.
  }

  // Process remaining turning points pairs and create pair of halfedges.
  PcTurningPointSet::iterator it_next = it_cur;
  ++it_next;
  while ((it_cur != it_end)) {
    // It can be odd turning point with wrong direction generated by edges
    // that lie on clipping window's edges. Just skip it.
    if (((kDirectionRight == it_cur->direction) &&
         (kDirectionLeft == it_next->direction)) ||
        ((kDirectionUp == it_cur->direction) &&
         (kDirectionDown == it_next->direction))) {
      // Create new pair of halfedges.
      const PcTurningPoint &tpt0 = *it_cur;
      const PcTurningPoint &tpt1 = *it_next;
      PcHalfedgeHandle new_he = PcHalfedge::CreateInternalPair(
          gstVertex(tpt0.x, tpt0.y, tpt0.z),
          gstVertex(tpt1.x, tpt1.y, tpt1.z),
          inside_area_loc, kClipEdge);
      polygon_builder_.AcceptHalfedge(new_he);
      polygon_builder_.AcceptHalfedge(new_he->pair);

      notify(NFY_VERBOSE,
             "TPT he: (%.20f, %.20f)-(%.20f, %.20f), IsDegenerate: %d",
             new_he->v.x, new_he->v.y, new_he->pair->v.x, new_he->pair->v.y,
             new_he->IsDegenerate());

      // Set flag "quad edge has halfedge".
      quad_edges_[quad_edge_type].has_halfedge = true;

      if (it_next == it_end) {
        break;
      }

      it_cur = it_next;
      ++it_next;
    }

    ++it_cur;
    ++it_next;
  }

  notify(NFY_VERBOSE, "%s done.", __func__);
}

void PolygonClipper::CreateHalfedgesFromInnerQuadCorners() {
  notify(NFY_VERBOSE, "%s ...", __func__);

  bool is_created;

  // If quad edge has no halfedges created from its turning points and
  // at least one of its end point lies inside a polygon, new halfedges
  // should be created based on this quad edge.
  // It's the case when clipping window is inside outer cycle of polygon and
  // has intersection with inner cycle(s).
  do {
    is_created = false;
    for (int i = 0; i < kNumQuadEdges_; i++) {
      if ((!quad_edges_[i].has_halfedge) &&
          (quad_corners_[quad_edges_[i].begin_corner_idx].is_inside ||
           quad_corners_[quad_edges_[i].end_corner_idx].is_inside)) {
        // Create halfedges.
        CreateHalfedgeFromQuadEdge(static_cast<QuadEdgeType>(i));

        // Set quad edge and quad corner attributes;
        quad_edges_[i].has_halfedge = true;
        quad_corners_[quad_edges_[i].begin_corner_idx].is_inside = true;
        quad_corners_[quad_edges_[i].end_corner_idx].is_inside = true;

        is_created = true;
      }
    }
  } while (is_created);

  notify(NFY_VERBOSE, "%s done.", __func__);
}

int PolygonClipper::ProcessSpecialCases() {
  assert(!in_halfedges_.empty());
  // Process the case when clipping window lies completely inside or outside
  // outer cycle of polygon and has no intersection with polygon's edges.
  notify(NFY_VERBOSE, "%s ...", __func__);
  notify(NFY_VERBOSE, "in_halfedges set is empty: %d.",
         in_halfedges_.empty());
  notify(NFY_VERBOSE, "out_halfedges set is empty: %d.",
         polygon_builder_.Empty());

  assert(special_sweep_status_.Empty());
  int ret = 0;
  // Check whether clipping quad inside or outside polygon: it's based on
  // location of quad's bottom edge in sweep line status.
  for (PcHalfedgeHandleList::iterator it = in_halfedges_.begin();
       it != in_halfedges_.end(); ++it) {
    const PcHalfedgeHandle &he = (*it);
    if (he->IsLeft() &&
        LessOrEquals(he->v.x,  wx1_, .0) &&   // epsilon = .0
        Less(wx1_, he->pair->v.x, .0)) {      // epsilon = .0
      // Note: we do not insert vertical halfedges
      // into sweep_status.
      special_sweep_status_.Update(he);
    }
  }

  PcHalfedgeHandle quad_he = PcHalfedge::CreateInternalPair(
      gstVertex(wx1_, wy1_), gstVertex(wx2_, wy1_),
      kInsideAreaLocAbove, kClipEdge);
  PcHalfedgeHandle prev = special_sweep_status_.PrevOfEdge(quad_he);
  quad_he->Clean();
  special_sweep_status_.Clear();
#if 0
  // Note: use it for debugging.
  notify(NFY_VERBOSE, "prev exists %u", prev.refcount());
  if (prev) {
    notify(NFY_VERBOSE, "prev inside area loc is above: %d",
           prev->inside_area_loc() == kInsideAreaLocAbove);
    const PcVertex& lft = prev->Left();
    const PcVertex& rht = prev->Right();

    notify(NFY_NOTICE, "prev: (%f, %f) - (%f, %f)",
           khTilespace::Denormalize(lft.x), khTilespace::Denormalize(lft.y),
           khTilespace::Denormalize(rht.x), khTilespace::Denormalize(rht.y));
  }
#endif
  if (prev && prev->inside_area_loc() == kInsideAreaLocAbove) {
    ret = 1;  // Clipping quad is inside the polygon.
  } else {
    ret = 0;  // Clipping quad is outside the polygon.
  }

  if (ret && !polygon_builder_.Empty()) {
    // The clipping quad lies inside polygon's outer cycle but has an
    // intersection with polygon's inner cycles.
    // Create halfedges based on quad edges (outer cycle for resulting polygon).
    for (int i = 0; i < kNumQuadEdges_; i++) {
      CreateHalfedgeFromQuadEdge(static_cast<QuadEdgeType>(i));
    }

    // inside outer cycle but has an intersection with inner cycle(s).
    ret = 2;
  }

  notify(NFY_VERBOSE, "%s done.", __func__);
  return ret;
}

void PolygonClipper::CreateHalfedgeFromQuadEdge(
    const QuadEdgeType quad_edge_type) {
  gstVertex pt_begin;
  gstVertex pt_end;
  PcInsideAreaLocationType inside_area_loc;

  // Get quad edge based on quad_edge_type.
  GetQuadEdgeByType(quad_edge_type, &pt_begin, &pt_end, &inside_area_loc);

  // Create halfedges based on quad edge and insert to output halfedges list.
  PcHalfedgeHandle new_he = PcHalfedge::CreateInternalPair(
      pt_begin, pt_end, inside_area_loc, kClipEdge);
  polygon_builder_.AcceptHalfedge(new_he);
  polygon_builder_.AcceptHalfedge(new_he->pair);
}

void PolygonClipper::GetQuadEdgeByType(
    const QuadEdgeType quad_edge_type,
    gstVertex *const pt_begin,
    gstVertex *const pt_end,
    PcInsideAreaLocationType *const inside_area_loc) const {
  assert(pt_begin != NULL);
  assert(pt_end != NULL);
  assert(inside_area_loc != NULL);
  // Determine end points;
  switch (quad_edge_type) {
    case kQuadEdgeLeft:
      *pt_begin = gstVertex(wx1_, wy1_, height25D_);
      *pt_end = gstVertex(wx1_, wy2_, height25D_);
      break;
    case kQuadEdgeRight:
      *pt_begin = gstVertex(wx2_, wy1_, height25D_);
      *pt_end = gstVertex(wx2_, wy2_, height25D_);
      break;
    case kQuadEdgeBottom:
      *pt_begin = gstVertex(wx1_, wy1_, height25D_);
      *pt_end = gstVertex(wx2_, wy1_, height25D_);
      break;
    case kQuadEdgeTop:
      *pt_begin = gstVertex(wx1_, wy2_, height25D_);
      *pt_end = gstVertex(wx2_, wy2_, height25D_);
      break;
    default:
      notify(NFY_FATAL, "Invalid quad edge type.");
      break;
  }

  // Determine polygon inside area location.
  if ((kQuadEdgeTop == quad_edge_type) ||
      (kQuadEdgeLeft == quad_edge_type)) {  // Left or Top quad edge.
    *inside_area_loc = kInsideAreaLocBelow;
  } else {  //  Right or Bottom quad edge;
    *inside_area_loc = kInsideAreaLocAbove;
  }
}

}  // namespace fusion_gst

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

//
// Processor for clipping polygon with holes against rectangle window.
//
// Implementation is based on proposed in [1] and [2]Azevedo, Gutting in
// "Polygon Clipping and Polygon Reconstruction".
// Algorithm contains two main steps:
// 1. Polygon Clipping
// 2. Polygon Reconstruction
//
// Polygon Clipping step has an input a set of halfedges of source polygon and
// produces a set of halfedges corresponding to the portion of polygon's
// halfedges that are inside the clipping window and new halfedges generated
// based on turning points. Turning point is a point at the intersection of
// polygon halfedge and clipping window edge.
// Liang-Barsky line clipping algorithm is used to clip halfedges against
// rectangle window.
// Polygon Clipping steps:
// FOR all segments DO
//   CLIP segment against clipping window using Liang-Barsky line clipping
// algorithm. Result of clipping is halfedge or intersection point;
//   INSERT halfedge into output halfedges set;
//   EVALUATE candidates to turning points (halfedge end points, intersection
// point) and collect turning points in sets of turning points (Each clipping
// window edge (Left, Right, Top, Bottom) has its own set of turning points);
// END-FOR
// CREATE new halfedges based on turning points and insert into output
// halfedges set;
//
// Polygon Reconstruction step (see class PolygonBuilder).

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTPOLYGONCLIPPER2_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTPOLYGONCLIPPER2_H_

#include <vector>
#include <set>

#include "fusion/gst/gstPolygonBuilder.h"
#include "fusion/gst/gstBBox.h"
#include "fusion/gst/gstGeode.h"

//#include "common/khTypes.h"
#include <cstdint>
#include "common/base/macros.h"


// Overview of classes:
// class PolygonClipper implements algorithm of clipping polygon with holes
// against clipping rectangle window. It uses auxiliary class SweepLineStatus
// that represents a state of intersection of sweep line with geometric
// structure being swept at the current sweep line position and
// class PolygonBuilder for polygon reconstruction.

namespace fusion_gst {

// Polygon Clipper processor.
// Implements polygon clipping against rectangle window.
// It accepts source polygon and produces polygon(s) that is(are) part(s)
// of source polygon which lie(s) inside clipping window.
//
// using:
//   PolygonClipper poly_clipper;  - create PolygonClipper.
//   SetClipRect(...);   - set clipping rectangle.
//   Run();              - run polygon clipping.
//   Run();              - run polygon clipping for new input data;
class PolygonClipper {
 public:
  // cut_holes specifies whether execute holes cutting.
  explicit PolygonClipper(bool cut_holes);

  // _box specifies clipping window rectangle.
  // cut_holes specifies whether execute holes cutting.
  PolygonClipper(const gstBBox &_box, bool cut_holes);
  ~PolygonClipper();

  // Sets clipping window rectangle.
  // wx1, wx2, wy1, wy2 are coordinates of clipping window.
  void SetClipRect(double wx1, double wx2,
                   double wy1, double wy2) {
    wx1_ = wx1;
    wx2_ = wx2;
    wy1_ = wy1;
    wy2_ = wy2;
    box_.init(wx1_, wx2_, wy1_, wy2_);
    // segment_clipper_.SetClipRect(wx1_, wx2_, wy1_, wy2_);
  }

  // Sets clipping window specified by bounding box.
  // _box is a clipping window rectangle.
  void SetClipRect(const gstBBox & _box) {
    box_ = _box;
    wx1_ = box_.minX();
    wx2_ = box_.maxX();
    wy1_ = box_.minY();
    wy2_ = box_.maxY();
    // segment_clipper_.SetClipRect(wx1_, wx2_, wy1_, wy2_);
  }

  // Runs polygon clipping processor for geode and Adds clipped pieces
  // to existing geode list. Function accept geode and returns clipped pieces.
  //
  // geodeh is an input geometry;
  // pieces is a list of output polygons;
  // completely_covered - returns whether quad is completely covered by input
  // polygon.
  void Run(const gstGeodeHandle &geodeh,
           GeodeList *pieces, bool *completely_covered);

  inline void Run(const gstGeodeHandle &geodeh, GeodeList *pieces) {
    bool completely_covered = false;
    Run(geodeh, pieces, &completely_covered);
  }

 private:
  // Quad's edge types.
  enum QuadEdgeType {
    kQuadEdgeLeft = 0,
    kQuadEdgeRight = 1,
    kQuadEdgeBottom = 2,
    kQuadEdgeTop = 3
  };

  // Quad's corner types.
  enum QuadCornerType {
    kQuadCornerBottomLeft = 0,
    kQuadCornerBottomRight = 1,
    kQuadCornerTopRight = 2,
    kQuadCornerTopLeft = 3
  };

  // Direction types of turning point.
  // Direction type of turning point is defined in correspondence with
  // location of polygon's area relative to the turning point.
  enum DirectionType {
    kDirectionNone  = 0,   // direction is undefined.
    kDirectionLeft  = 1,   // polygon inside area to the left.
    kDirectionRight = 2,   // polygon inside area to the right.
    kDirectionUp    = 3,   // polygon inside area is above.
    kDirectionDown  = 4    // polygon inside area is under.
  };

  // Polygon Clipper Turning point.
  // A turning point is a point at the intersection of polygon's edge with
  // clipping window's edge.
  class PcTurningPoint : public PcVertex {
   public:
    // Direction of turning point.
    // Direction of turning point is a location of the polygon's area
    // relative to the turning point.
    DirectionType direction;

    PcTurningPoint() : direction(kDirectionNone) {
    }

    explicit PcTurningPoint(const PcVertex &_pt)
        : PcVertex(_pt), direction(kDirectionNone) {
    }

    virtual ~PcTurningPoint() { }

    // Computes the direction of turning points which lie on the left/right
    // clipping window's edge.
    //
    // he - halfedge which gives this turning point as result of
    // intersection with clipping window's edge.
    void CalculateDirectionLeftRight(
        const PcHalfedgeHandle &he,
        const PcInsideAreaLocationType quad_edge_area_loc);

    // Computes the direction of turning points which lie on the top/bottom
    // clipping window's edge.
    //
    // he - halfedge which gives this turning point as result of
    // intersection with clipping window's edge.
    void CalculateDirectionBottomTop(
        const PcHalfedgeHandle &he,
        const PcInsideAreaLocationType quad_edge_area_loc);

   private:
    void CalculateDirectionLeftRightEdgeTouch(
        const PcHalfedgeHandle &he,
        const PcInsideAreaLocationType quad_edge_area_loc);
    void CalculateDirectionBottomTopGeneric(const PcHalfedgeHandle &he);
    void CalculateDirectionBottomTopEdgeTouch(
        const PcHalfedgeHandle &he,
        const PcInsideAreaLocationType quad_edge_area_loc);
  };

  typedef std::set<
    PcTurningPoint,
    XYLexicographicLessWithTolerance<PcTurningPoint> > PcTurningPointSet;

  // QuadEdge attributes. It is auxiliary struct used for mapping quad's edge
  // to quad's corner and processing special cases when clipping window lies
  // inside/outside a polygon.
  struct QuadEdge {
    QuadEdge() {}
    QuadEdge(const QuadCornerType _begin_corner_idx,
             const QuadCornerType _end_corner_idx)
        : begin_corner_idx(_begin_corner_idx),
          end_corner_idx(_end_corner_idx),
          has_halfedge(false) {
    }
    // Index of begin corner.
    std::uint8_t begin_corner_idx;
    // Index of end corner.
    std::uint8_t end_corner_idx;
    // Flag is true if halfedges are created from turning points of
    // this quad's edge.
    bool has_halfedge;
  };

  // QuadCorner attributes. It is auxiliary struct used for mapping quad's
  // corner to quad's edge and processing special cases when clipping window
  // lies inside a polygon.
  struct QuadCorner {
    QuadCorner()
        : is_inside(false) {
    }

    // Flag is true if this quad's corner lies inside a polygon.
    bool is_inside;
  };

  // Runs polygon clipping processor for one polygon and Adds clipped pieces
  // to existing geode list.
  // geode - input polygon;
  // pieces - output polygons;
  // completely_covered - returns whether quad is completely covered by input
  // polygon.
  void RunPolygon(const gstGeode *geode,
                  GeodeList *pieces, bool *completely_covered);

  // Initializes internal structures of PolygonClipper processor.
  void Init();

  // Resets internal intermediate data structures.
  void Reset();

  // Accepts input polygon for processing.
  // return true if polygon lies inside a clipping window (futher processing
  // is not required), otherwise it returns false.
  bool AcceptPolygon(const gstGeode *geode, GeodeList *pieces);

  // Implements polygon clipping step. Clips all edges of polygon and produces
  // parts of input polygon's edges that lie inside clipping rectangle.
  void ClipPolygon();

  // Clips halfedge against rectangle window.
  //
  // in_he - an input halfedge
  // out_he - a result pairs of halfedges.
  // intersection - an intersection point.
  // is_intersection_point - it is true if intersection is point.
  //
  // return true - if there is an intersection, otherwise it returns false.
  bool ClipHalfedge(const PcHalfedgeHandle &in_he,
                    PcHalfedgeHandle *const out_he,
                    PcVertex *const intersection,
                    bool *const is_intersection_point) const;

  // Evaluates if intersection point is turning point
  // and inserts into set of turning points.
  //
  // pt is a candidate to turning point.
  // he is a corresponding halfedge.
  void EvaluateTurningPoint(const PcVertex &pt, const PcHalfedgeHandle &he);

  // Creates halfedges from turning points of specified quad's edge
  // and inserts them into output halfedges list.
  //
  // quad_edge_type - specifies quad edge to process.
  void CreateHalfedgesFromTurningPoints(const QuadEdgeType quad_edge_type);

  // Create halfedges from quad's corners that lie inside polygon.
  void CreateHalfedgesFromInnerQuadCorners();

  // Processes the case when there are no clipped edges and clipping
  // window lies completely inside or outside of polygon.
  // return 0 if clipping window is completely outside of polygon.
  // return 1 if clipping window is completely inside of polygon.
  // return 2 if clipping window lies inside outer cycle
  // but contains inner cycle.
  int ProcessSpecialCases();

  // Creates halfedge from quad edge specified by input parameter and inserts
  // them into output halfedges list.
  //
  // quad_edge_type - specifies quad edge to process.
  void CreateHalfedgeFromQuadEdge(const QuadEdgeType quad_edge_type);

  // Gets quad edge end points based on clipping window
  // coordinates and quad edge type.
  void GetQuadEdgeByType(const QuadEdgeType quad_edge_type,
                         gstVertex *const pt_begin,
                         gstVertex *const pt_end,
                         PcInsideAreaLocationType *const inside_area_loc) const;

 private:
  // Size of turning points sets array.
  static const int kNumQuadEdges_ = 4;
  static const PcInsideAreaLocationType kQuadEdgeAreaLoc_[kNumQuadEdges_];

  // Quad's edge to corners mapping array.
  QuadEdge quad_edges_[kNumQuadEdges_];

  // Quad's corner to edges mapping array.
  QuadCorner quad_corners_[kNumQuadEdges_];

  // Clipping rectangle window.
  double wx1_;
  double wx2_;
  double wy1_;
  double wy2_;

  // TODO: should be left one of them (box_);
  // Segment Clipper.
  //  SegmentClipper segment_clipper_;
  gstBBox box_;

  // Height of 2.5D polygon. It is used to determine z-coordinate for new
  // points created based on quad edges. For these points we just get
  // z-coordinate from first point of outer cycle.
  double height25D_;

  // Storage for hafedges that should be checked for intersection with quad.
  PcHalfedgeHandleList in_halfedges_;
  // Auxiliary objects to accept source polygons for processing.
  CycleAcceptor<PcHalfedgeHandleList> in_halfedges_acceptor_;

  // Turning point sets.
  // Each clipping rectangle window edge has own corresponding
  // turning point set.
  std::vector<PcTurningPointSet> turning_point_sets_;

  // Sweep line status for processing special cases.
  SweepLineStatus special_sweep_status_;

  // Polygon builder.
  PolygonBuilder polygon_builder_;

  DISALLOW_COPY_AND_ASSIGN(PolygonClipper);
};

}  // namespace fusion_gst

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTPOLYGONCLIPPER2_H_

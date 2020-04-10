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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTPOLYGONBUILDER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTPOLYGONBUILDER_H_

// Polygon builder processor.
//
// Polygon's structures are based on proposed in [1]Gutting and Schneider
// "Implementation of the ROSE Algebra: Efficient Algorithms for
// Realm-Based Spatial Data Types", 1995.
// Each edge of source polygon is mapped to two halfedges.
// A halfedge is oriented edge. Each halfedge contains its "dominating"
// (starting) point, reference to opposite (paired) halfedge and reference to
// predecessor and successor halfedges incident the same node.
// A set of halfedges can be regarded as a planar graph G = (V, E) where
// the vertex set V is set of all halfedge endpoints and edge set E is set of
// all halfedges of object.
//
// Implementation is based on proposed in [1] and [2]Azevedo, Gutting in
// "Polygon Clipping and Polygon Reconstruction".
//
// Polygon Reconstruction step has an input a set of halfedges and
// produces polygons (faces). It scans halfedges set and assign the face number
// and cycle number to each halfedge and create faces.
// Face is composed by the cycles (outer cycle and inner cycles (holes)).
// Cycle contains pointers to first and last halfedge in cycle.
// Polygon Reconstruction steps:
// SORT the halfedges in corresponding with X,Y-lexicographic order by
// "dominating" point;
// LINK halfedges incident the same node in counterclockwise order (it is
// basically X,Y-lexicographic order by "dominating" point);
// SCAN set of ordered, linked halfedges and compute cycles using graph
// primitive operations and use plane sweep to determine for each hole segment
// the outer cycle (face number);
//
// Using PolygonBuilder:
// PolygonBuilder polygon_builder(PolygonBuilderOptions::CUT_HOLES_NO_CLEAN());
// polygon_builder.SetPrimType(geode.PrimType());
// polygon_builder.AcceptPolygon(geode);
// [polygon_builder.AcceptPolygonPart(geode,n);]
// [polygon_builder.AcceptHalfedge(he);]
// polygon_builder.Run(output_geode_list);
//
// Using PolygonBuilder to clean completely overlapped edges:
// PolygonBuilder polygon_builder(PolygonBuilderOptions::NO_CUT_HOLES_CLEAN());
// polygon_builder.SetPrimType(geode.PrimType());
// polygon_builder.AcceptPolygon(geode);
// polygon_builder.Run(output_geode_list);

// Auxiliary classes:
// class PcVertex defines 3D point.
//
// class PcHalfedge defines basic structure that describes oriented edge
// and allows to construct planar graph with usual incidence relationship
// between nodes and edges. And also it allows to define the neighbourhood
// relationship among edges incident to the same node.
//
// class PcCycle defines outer or inner cycle of polygon by pointers to first
// and last halfedge in cycle.
//
// class PcFace defines polygon. It is composed by the outer and inner cycles.


#include <assert.h>
#include <cmath>
#include <limits>
#include <vector>
#include <deque>
#include <set>
#include <utility>
#include <functional>

#include "fusion/gst/gstConstants.h"
#include "fusion/gst/gstMathUtils.h"
#include "fusion/gst/gstVertex.h"
#include "fusion/gst/gstGeode.h"

#include "common/khRefCounter.h"
//#include "common/khTypes.h"
#include <cstdint>
#include "common/base/macros.h"

namespace fusion_gst {

// Polygon Builder Vertex.
class PcVertex {
 public:
  typedef PcVertex Self;

  PcVertex()
      : x(.0), y(.0), z(.0) { }

  PcVertex(double _x, double _y, double _z)
      : x(_x), y(_y), z(_z) {
  }

  explicit PcVertex(const gstVertex& v)
      : x(v.x), y(v.y), z(v.z) {
  }

  virtual ~PcVertex() { }

  bool IsNaN() const {
    return (std::isnan(x) || std::isnan(y) || std::isnan(z));
  }

  static Self NaN() {
    return Self(std::numeric_limits<double>::quiet_NaN(),
                std::numeric_limits<double>::quiet_NaN(),
                std::numeric_limits<double>::quiet_NaN());
  }

  // Equality comparison operation.
  // It compares points for equality by X,Y coordinates with tolerance.
  inline bool EqualsXY(const PcVertex &right, double epsilon = kEpsilon) const {
    return (Equals(x, right.x, epsilon) && Equals(y, right.y, epsilon));
  }

  // "Less than" inequality operation by x,y coordinates in according with
  // vertical sweep line.
  inline bool LessXY(const PcVertex& right) const {
    return XYLexicographicLess<Self>()(*this, right);
  }

  // Coordinates.
  double x;
  double y;
  double z;
};

// Types of "dominating" (starting) point of halfedge.
// kDominatingLeft means that left (smaller according to
// vertical sweep line concept) point of corresponding segment(edge) is
// "dominating" point of halfedge and this halfedge is called left halfedge.
// kDominatingRight means that right (bigger) point of edge is "dominating"
// point of halfedge and this halfedge is called right halfedge.
enum PcDominatingType {
  kDominatingNone = 0,  // "dominating" point is undefined.
  kDominatingLeft = 1,
  kDominatingRight = 2
};

// Types of polygon inside area location.
// If segment is vertical line it has inside above area location
// when area inside polygon is on the left.
enum PcInsideAreaLocationType {
  kInsideAreaLocNone = 0,   // area location is undefined.
  kInsideAreaLocBelow = 1,  // area inside the polygon lies below segment.
  kInsideAreaLocAbove = 2   // area inside the polygon lies above segment.
};

class PcHalfedge;
typedef khRefGuard<PcHalfedge> PcHalfedgeHandle;
typedef std::deque<PcHalfedgeHandle> PcHalfedgeHandleList;
// TODO: class PcHalfedgeHandleList with std::deque-composition.

// Polygon Builder Halfedge.
// Halfedge is oriented edge between two vertices.
// Segment vertexes are considered as left and right end points in
// corresponding with XY lexicographic order in vertical sweep line concept.
// Each segment (edge) is mapped to two halfedges (left and right).
// Left halfedge has left endpoint of edge as its starting point.
// Right halfedge has right endpoint of edge as its starting point.
class PcHalfedge : public khRefCounter {
 public:
  static PcHalfedgeHandle Create() {
    return khRefGuardFromNew(new PcHalfedge());
  }
  static PcHalfedgeHandle Create(const gstVertex &_v) {
    return khRefGuardFromNew(new PcHalfedge(_v));
  }

  // Creates pair of halfedges on begin/end points.
  static PcHalfedgeHandle CreatePair(const gstVertex &v0,
                                     const gstVertex &v1);

  // Creates pair of halfedges on begin/end points and
  // initializes attributes based on source halfedge.
  //
  // v0, v1 are points on source halfedge that are result of
  // clipping source halfedge against rectangle window.
  // he is a source halfedge.
  //
  // Returns created halfedge.
  static PcHalfedgeHandle CreatePair(const gstVertex &v0,
                                     const gstVertex &v1,
                                     const PcHalfedgeHandle& he);

  // Creates pair of "internal" halfedges on begin/end points
  // and set attributes. "Internal" edge is edge introduced in polygon because
  // of clipping or holes-cutting.
  // TODO: can be optimized for pair of turning points,
  // because "dominating" point is known.
  //
  // v0, v1 are end points of edge.
  // inside_area_loc is a polygon inside area location attribute of edge.
  //
  // Returns created halfedge.
  static PcHalfedgeHandle CreateInternalPair(
      const gstVertex &v0, const gstVertex &v1,
      const PcInsideAreaLocationType inside_area_loc,
      const gstEdgeType edge_type);

  // Cleans all linking data.
  inline void Clean() {
    pair.release();
    pred.release();
    succ.release();
  }

  // Returns true if it is left halfedge, otherwise it returns false.
  inline bool IsLeft() const {
    return (kDominatingLeft == dominating);
  }

  // Returns true if it is right halfedge, otherwise it returns false.
  inline bool IsRight() const {
    return (kDominatingRight == dominating);
  }

  // Returns true if halfedge is degenerate, otherwise it returns false.
  inline bool IsDegenerate() const {
    assert(pair);
    return v.EqualsXY(pair->v);
  }

  // Gets left end point of edge.
  // "left point" < "right point" in concept of vertical sweep line
  // (see XYLexicographicLess()).
  inline const PcVertex& Left() const {
    assert(pair);
    return IsLeft() ? v : pair->v;
  }

  inline PcVertex& Left() {
    assert(pair);
    return IsLeft() ? v : pair->v;
  }

  // Gets right end point of edge.
  inline const PcVertex& Right() const {
    assert(pair);
    return IsRight() ? v : pair->v;
  }

  inline PcVertex& Right() {
    assert(pair);
    return IsRight() ? v : pair->v;
  }

  // Links pair halfedges.
  inline void LinkPair(PcHalfedgeHandle* opposite) {
    pair = *opposite;
    (*opposite)->pair = khRefGuardFromThis();
  }

  // Gets "inside_area_location"- attribute.
  inline PcInsideAreaLocationType inside_area_loc() const {
    return inside_area_loc_;
  }

  // Sets "inside_area_location"- attribute.
  inline void set_inside_area_loc(PcInsideAreaLocationType val) {
    assert(pair);
    inside_area_loc_ = pair->inside_area_loc_ = val;
  }

  // Gets edge type.
  inline gstEdgeType edge_type() const {
    return edge_type_;
  }

  // Sets edge type.
  inline void set_edge_type(gstEdgeType val) {
    assert(pair);
    edge_type_ = pair->edge_type_ = val;
  }

  // Gets "visited"-flag.
  inline bool visited() const {
    return visited_;
  }

  // Sets "visited"-flag.
  inline void set_visited(bool val) {
    assert(pair);
    visited_ = pair->visited_ = val;
  }

  // Gets face number.
  inline unsigned int face_num() const {
    return face_num_;
  }

  // Gets cycle number.
  inline unsigned int cycle_num() const {
    return cycle_num_;
  }

  // Sets face attributes (face number, cycle number).
  inline void SetFaceAttr(const unsigned int _face_num, const unsigned int _cycle_num) {
    assert(pair);
    face_num_ = pair->face_num_ = _face_num;
    cycle_num_ = pair->cycle_num_ = _cycle_num;
  }

  inline void CopyAttr(const PcHalfedgeHandle& from) {
    assert(pair);
    set_inside_area_loc(from->inside_area_loc());
    set_edge_type(from->edge_type());
    SetFaceAttr(from->face_num(), from->cycle_num());
  }

  // Computes "dominating" point.
  // It is called for first halfedge of edge being added.
  // Note pair halfedge member should be initialized.
  void CalculateDominatingPoint();

  // TODO: implement by sweepline pass.
  // Computes polygon inside area location.
  // It is called for first halfedge of edge being added.
  // Note Calculation is based on prior knowledge: outer cycle edges are
  // oriented in counterclockwise order, holes edges are in clockwise order.
  // Note "Dominating" point should be initialized before.
  // Note Pair halfedge should be initialized before.
  void CalculateInsideAreaLoc();

  bool operator==(const PcHalfedge &other) const;

  // Starting point.
  PcVertex v;

  // Defines type of halfedge starting point and accordingly halfedge type
  // (left or right halfedge).
  PcDominatingType dominating;

  // Reference to pair (opposite) halfedge (left halfedge for right halfedge
  // of corresponding edge and vice versa).
  PcHalfedgeHandle pair;

  // Predecessor/Successor halfedges in counterclockwise direction around node
  // in corresponding with lexicographic order by "dominating" point.
  PcHalfedgeHandle pred;
  PcHalfedgeHandle succ;

 private:
  PcHalfedge()
      : v(),
        dominating(kDominatingNone),
        pair(),
        pred(),
        succ(),
        inside_area_loc_(kInsideAreaLocNone),
        edge_type_(kNormalEdge),
        visited_(false),
        face_num_(static_cast< unsigned int> (-1)),
        cycle_num_(static_cast< unsigned int> (-1)) {
  }

  explicit PcHalfedge(const gstVertex &_v)
      : v(_v),
        dominating(kDominatingNone),
        pair(),
        pred(),
        succ(),
        inside_area_loc_(kInsideAreaLocNone),
        edge_type_(kNormalEdge),
        visited_(false),
        face_num_(static_cast< unsigned int> (-1)),
        cycle_num_(static_cast< unsigned int> (-1)) {
  }

  // Location of area inside the polygon;
  PcInsideAreaLocationType inside_area_loc_;

  // Edge flag is type of edge. It is used to make a difference between edge
  // of source polygon, edge introduced as result of clipping (new edge from
  // quad boundary) and hole-cutting edge (new edge that connect the
  // holes to the outer cycle).
  gstEdgeType edge_type_;

  // Auxiliary field that is used in polygon reconstruction to check
  // if this halfedge already has a face number and a cycle number.
  bool visited_;

  // Face number that halfedge belongs.
  unsigned int face_num_;

  // Cycle/hole number that halfedge belongs.
  unsigned int cycle_num_;

  DISALLOW_COPY_AND_ASSIGN(PcHalfedge);
};

inline bool PcHalfedge::operator==(const PcHalfedge &other) const {
  assert(pair && other.pair);
  if (dominating == other.dominating &&
      v.EqualsXY(other.v) &&
      pair->v.EqualsXY(other.pair->v)) {
    return true;
  }
  return false;
}

// Function object for "less than" inequality operation for halfedges by
// XY-lexicographic order of "dominating" point.
struct PcHalfedgeDPXYLexicographicLess
    : public std::binary_function<PcHalfedgeHandle, PcHalfedgeHandle, bool> {
  bool operator()(const PcHalfedgeHandle &he1,
                  const PcHalfedgeHandle &he2) const {
    const PcVertex& dp_he1 = he1->v;
    const PcVertex& dp_he2 = he2->v;

    if (dp_he1.LessXY(dp_he2)) {
      return true;
    } else if (dp_he1.EqualsXY(dp_he2)) {
      if (he1->IsRight() && he2->IsLeft()) {
        return true;
      } else if (he1->dominating == he2->dominating) {
        assert(he1->pair);
        assert(he2->pair);
        const PcHalfedgeHandle& he1_pair = he1->pair;
        const PcHalfedgeHandle& he2_pair = he2->pair;
        gstVert2D v1((he1_pair->v.x - he1->v.x), (he1_pair->v.y - he1->v.y));
        gstVert2D v2((he2_pair->v.x - he2->v.x), (he2_pair->v.y - he2->v.y));
        double cross_product = v1.Cross(v2);
        return (cross_product > .0);
      }
    }

    return false;
  }
};

typedef std::multiset<
  PcHalfedgeHandle, PcHalfedgeDPXYLexicographicLess> PcHalfedgeHandleMultiSet;

// Clears halfedges list.
template<class Container>
void ClearHalfedgesList(Container* const halfedges) {
  // Linking data should be cleaned before clearing of collection.
  for (typename Container::iterator it = (*halfedges).begin();
       it != (*halfedges).end(); ++it) {
    (*it)->Clean();
  }
  (*halfedges).clear();
}

// Polygon Cycle.
// Describes outer or inner cycle (hole).
class PcCycle {
 public:
  PcCycle() {}
  // TODO: !?add bounding box for optimization.
  PcHalfedgeHandle first_halfedge;
  PcHalfedgeHandle last_halfedge;
};

// Polygon Face.
// A face is composed by the cycles (outer cycle and inner cycles (holes)).
class PcFace {
 public:
  PcFace() : halfedges_(NULL) { }

  PcFace(PcHalfedgeHandleList *halfedges_storage, int cycles_size)
      : halfedges_(halfedges_storage) {
    cycles_.reserve(cycles_size);
  }

  bool IsValid() const {
    return (CyclesSize() != 0 && GetCycle(0).first_halfedge);
  }

  // Converts from Face to gstGeode.
  void ConvertTo(gstGeode *geode) const;

  // Adds new cycle.
  void AddCycle() {
    cycles_.push_back(PcCycle());
  }

  // Gets number of cycles.
  unsigned int CyclesSize() const {
    return cycles_.size();
  }

  // Gets cycle n.
  const PcCycle& GetCycle(unsigned int n) const {
    assert(n < cycles_.size());
    return cycles_[n];
  }

  PcCycle& GetCycle(unsigned int n) {
    assert(n < cycles_.size());
    return cycles_[n];
  }

  // Gets last cycle.
  PcCycle& GetLastCycle() {
    return cycles_.back();
  }

 private:
  // Reference to halfedges storage.
  PcHalfedgeHandleList *halfedges_;

  // Cycles/Holes array.
  // First cycle in array is outer cycle, other ones are inner(holes).
  std::vector<PcCycle> cycles_;
};

// Auxiliary class used to accept geode parts and convert them into internal
// representation (set of halfedges).
template<class Container>
class CycleAcceptor {
 public:
  explicit CycleAcceptor(Container *halfedges_storage)
      : halfedges_(halfedges_storage) {
  }

  // Accepts geode part.
  void Accept(const gstGeode *geode, unsigned int part);

 private:
  // Creates pair of halfedges based on begin/end points and inserts into
  // halfedges storage.
  void AddHalfedgePair(const gstVertex &v0, const gstVertex &v1);

  // Reference to halfedges storage.
  Container *const halfedges_;
};

template<class Container>
void  CycleAcceptor<Container>::Accept(const gstGeode *geode, unsigned int part) {
  unsigned int num_verts = geode->VertexCount(part);
  if (num_verts < kMinCycleVertices) {
    notify(NFY_WARN, "Invalid polygon's cycle: skipped.\n");
    return;
  }

  for (unsigned int i = 0; i < (num_verts - 1); ++i) {
    AddHalfedgePair(geode->GetVertex(part, i), geode->GetVertex(part, i + 1));
  }
}

// Sweep line status.
// It is state of intersection of sweep line with geometric structure being
// swept at the current sweep line position.
class SweepLineStatus {
  friend class PolygonBuilder;
 public:
  SweepLineStatus() {}

  // Clears internal data;
  inline void Clear() {
    status_.clear();
  }

  inline bool Empty() const {
    return status_.empty();
  }

  // Updates sweep line status.
  inline void Update(const PcHalfedgeHandle& he) {
    if (kDominatingLeft == he->dominating) {  // Left halfedge.
      std::pair<PcHalfedgeHandleSet::iterator, bool> p = status_.insert(he);
      notify(NFY_VERBOSE,
             "SweepLineStatus::Update() insert: (%.20f, %.20f)-(%.20f, %.20f)"
             " face num: %u, inserted: %d",
             he->v.x, he->v.y, he->pair->v.x, he->pair->v.y,
             he->face_num(),
             p.second);
    } else {  // Right halfedge.
      unsigned int num_erased = static_cast< unsigned int> (status_.erase(he->pair));
      notify(NFY_VERBOSE,
             "SweepLineStatus::Update() remove: "
             "(%.20f, %.20f)-(%.20f, %.20f) face num: %u, removed: %u",
             he->pair->v.x, he->pair->v.y, he->v.x, he->v.y,
             he->face_num(),
             num_erased);
    }
  }

  // Gets previous of input edge from sweep line status.
  PcHalfedgeHandle PrevOfEdge(const PcHalfedgeHandle& he) const {
    notify(NFY_VERBOSE, "SweepLineStatus::PrevOfEdge(): status size(): %u",
           static_cast< unsigned int> (status_.size()));

    if (!status_.empty()) {
      PcHalfedgeHandleSet::iterator lower = status_.lower_bound(he);
      // Note: additional check for coincident edges.
      // if (lower != status_.end()) {
      //   if (!status_.value_comp()(he, *lower)) {
      //     // The lower halfedge and specific halfedge are coincident edges.
      //     return (*lower);
      //   }
      // }

      if (status_.begin() != lower) {
        --lower;
        return (*lower);
      }
    }

    return PcHalfedgeHandle();
  }

  std::int32_t CountEdges(const PcHalfedgeHandle& he) const {
    notify(NFY_VERBOSE, "SweepLineStatus::CountEdges(): status size(): %u",
           static_cast< unsigned int> (status_.size()));
    std::int32_t count = 0;
    for (PcHalfedgeHandleSet::iterator it = status_.begin();
         it != status_.end(); ++it) {
      ++count;
      if ((*it) == he)
        break;
    }
#ifdef NDEBUG
    if (count != 0)
      assert((*it) == he);
#endif
    return count;
  }

 private:
  struct SweepLess
      : public std::binary_function<PcHalfedgeHandle,
                                    PcHalfedgeHandle, bool> {
    bool operator()(const PcHalfedgeHandle &l,
                    const PcHalfedgeHandle &r) const {
      const PcHalfedgeHandle &l_pair = l->pair;
      const PcHalfedgeHandle &r_pair = r->pair;
#ifndef NDEBUG
      // Note: Less() with no tolerance (epsilon = .0).
      if (Less(l_pair->v.x, r->v.x, .0) || Less(r_pair->v.x, l->v.x, .0)) {
        notify(NFY_NOTICE, "l_he: (%.10f, %.10f)-(%.10f, %.10f)",
               khTilespace::Denormalize(l->v.y),
               khTilespace::Denormalize(l->v.x),
               khTilespace::Denormalize(l_pair->v.y),
               khTilespace::Denormalize(l_pair->v.x));
        notify(NFY_NOTICE,
               "normalized l_he: (%.20f, %.20f)-(%.20f, %.20f), left: %d",
               l->v.x, l->v.y, l_pair->v.x, l_pair->v.y, l->IsLeft());

        notify(NFY_NOTICE, "r_he: (%10f, %.10f)-(%.10f, %.10f)",
               khTilespace::Denormalize(r->v.y),
               khTilespace::Denormalize(r->v.x),
               khTilespace::Denormalize(r_pair->v.y),
               khTilespace::Denormalize(r_pair->v.x));
        notify(NFY_NOTICE,
               "normalized r_he: (%.20f, %.20f)-(%.20f, %.20f), left: %d",
               r->v.x, r->v.y, r_pair->v.x, r_pair->v.y, r->IsLeft());
        notify(NFY_WARN, "SweepLess: Sweep line status is invalid");
      }
#endif
      gstVert2D v1;
      gstVert2D v2;
      bool is_left_less = false;

      if (l->v.LessXY(r->v)) {
        is_left_less = true;
        v1 = gstVert2D((l_pair->v.x - l->v.x), (l_pair->v.y - l->v.y));
        v2 = gstVert2D((r->v.x - l->v.x), (r->v.y - l->v.y));
      } else {
        v1 = gstVert2D((l->v.x - r->v.x), (l->v.y - r->v.y));
        v2 = gstVert2D((r_pair->v.x - r->v.x), (r_pair->v.y - r->v.y));
      }

      double cross_product1 = v1.Cross(v2);

      if (cross_product1 > .0) {
        return true;
      } else if (Equals(cross_product1, .0)) {
        if (is_left_less) {
          v2 = gstVert2D((r_pair->v.x - l->v.x), (r_pair->v.y - l->v.y));
        } else {
          v1 = gstVert2D((l_pair->v.x - r->v.x), (l_pair->v.y - r->v.y));
        }

        double cross_product2 = v1.Cross(v2);
        if (cross_product2 > .0) {
          return true;
        }
      }
      return false;
    }
  };
  typedef std::set<PcHalfedgeHandle, SweepLess> PcHalfedgeHandleSet;

#ifndef NDEBUG
  // Note: auxiliary debug function.
  void PrintSweepStatus() const {
    notify(NFY_NOTICE, "Sweep line status:");
    for (PcHalfedgeHandleSet::const_iterator it = status_.begin();
         it != status_.end(); ++it) {
      notify(NFY_NOTICE, "\t(%.20f, %.20f)-(%.20f, %.20f)",
             (*it)->v.x, (*it)->v.y, (*it)->pair->v.x, (*it)->pair->v.y);
    }
  }
#endif

  // Halfedges of sweep line status.
  PcHalfedgeHandleSet status_;
};

// Class for assembling PolygonBuilder options.
class PolygonBuilderOptions {
 public:
  PolygonBuilderOptions()
      : cut_holes_(false), clean_overlapped_edges_(false) {}

  // These are options that should be used for building polygons
  // without hole-cutting and without cleaning overlapped edges.
  static PolygonBuilderOptions NO_CUT_HOLES_NO_CLEAN();

  // These are options that should be used for building polygons
  // without hole-cutting and with cleaning overlapped edges.
  static PolygonBuilderOptions NO_CUT_HOLES_CLEAN();

  // These are options that should be used for building polygons
  // with hole-cutting and without cleaning overlapped edges.
  static PolygonBuilderOptions CUT_HOLES_NO_CLEAN();

  inline bool cut_holes() const { return cut_holes_; }
  void set_cut_holes(const bool cut_holes);

  inline bool clean_overlapped_edges() const { return clean_overlapped_edges_; }
  void set_clean_overlapped_edges(const bool clean_overlapped_edges);

 private:
  // option specifies whether to apply hole-cutting.
  bool cut_holes_;
  // option specifies whether to clean overlapped edges.
  bool clean_overlapped_edges_;
};

// Class PolygonBuilder builds polygons from a set of halfedges.
// It can execute holes cutting or just report holes as inner cycles.
class PolygonBuilder {
 public:
  // cut_holes specifies whether execute holes cutting.
  explicit PolygonBuilder(const PolygonBuilderOptions &options);

  ~PolygonBuilder();

  // Sets primary type for output polygons.
  inline void SetPrimType(gstPrimType gtype) {
    gtype_ = gtype;
  }

  // Accepts input polygon for processing.
  void AcceptPolygon(const gstGeode *geode);

  // Accepts a specified part of input polygon for processing.
  inline void AcceptPolygonPart(const gstGeode *geode, const unsigned int part) {
    out_halfedges_acceptor_.Accept(geode, part);
  }

  // Accepts a halfedge for processing.
  inline void AcceptHalfedge(const PcHalfedgeHandle &he) {
    out_halfedges_.insert(he);
  }

  // Return whether we have a data for recostruction.
  inline bool Empty() {
    return out_halfedges_.empty();
  }

  // Runs polygon builder for geode.
  // pieces is a container to put result polygons;
  void Run(GeodeList *pieces);

  // Resets internal intermediate data structures.
  void Reset();

 private:
  // Links halfedges around the node (common vertex) in counterclockwise order.
  void LinkHalfedges();

  // Reconstructs polygon from ordered and linked haldedges and skip not valid
  // cycles.
  void ReconstructPolygonAndSkipNotValidCycles();

  // Gets face number for halfedge based on sweep line status.
  unsigned int GetFaceNumber(const PcHalfedgeHandle &he,
                     PcHalfedgeHandle *const outer_he);

  // Connects inner cycle to outer one by introducing cut-edge.
  // he - first halfedge of inner cycle;
  // outer_he - halfedge of outer cycle to connect;
  void AddHoleToOuterCycle(PcHalfedgeHandle &he, PcHalfedgeHandle &outer_he);

  void ReportPolygons(GeodeList *pieces);

  // TODO: make a policy for cleaner.
  // Links halfedges around the node (common vertex) in counterclockwise order
  // and remove equal halfedges.
  void LinkHalfedgesAndCleanOverlapped();

  // Reconstructs polygon from ordered and linked haldedges.
  void ReconstructPolygon();

 private:
  const PolygonBuilderOptions options_;
  gstPrimType gtype_;

  // Sweep line status.
  SweepLineStatus sweep_status_;

  // Cut-halfedges storage. It contains halfeges created during holes-cutting.
  // We do not need to process them in main reconstruction cycle.
  // But we need to store them in order to properly release after run.
  PcHalfedgeHandleList cut_halfedges_;

  // Output halfedges.
  PcHalfedgeHandleMultiSet out_halfedges_;
  // Auxiliary objects to accept source polygons for processing.
  CycleAcceptor<PcHalfedgeHandleMultiSet> out_halfedges_acceptor_;

  // Output polygons.
  std::deque<PcFace> out_faces_;

  DISALLOW_COPY_AND_ASSIGN(PolygonBuilder);
};

}  // namespace fusion_gst

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTPOLYGONBUILDER_H_

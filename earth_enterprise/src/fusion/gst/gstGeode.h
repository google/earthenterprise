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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTGEODE_H__
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTGEODE_H__

#include <deque>
#include <list>
#include <utility>
#include <vector>
#include <algorithm>

#include "common/base/macros.h"
#include "common/khRefCounter.h"
//#include "common/khTypes.h"
#include <cstdint>
#include "common/khMTTypes.h"

#include "fusion/gst/gstLimits.h"
#include "fusion/gst/gstConstants.h"
#include "fusion/gst/gstTypes.h"
#include "fusion/gst/gstVertex.h"
#include "fusion/gst/gstBBox.h"

#define MULTI_POLYGON_ON 1

class gstFeaturePreviewConfig;
class gstGeodeTest;

typedef std::uint32_t gstDrawModes;

class gstDrawState {
 public:
  explicit gstDrawState(bool is_mercator_preview) :
      is_mercator_preview_(is_mercator_preview) {}
  gstDrawModes mode;
  double level;
  int offset;
  gstBBox frust;
  gstBBox select;
  gstBBox cullbox;

  void SetWidthHeight(int w, int h) {
    width_ = w;
    height_ = h;
  }
  int Width() const { return width_; }
  int Height() const { return height_; }

  double Scale() const { return frust.Width() / Width(); }
  void SetIsMercatorPreview(bool is_mercator_preview) {
    is_mercator_preview_ = is_mercator_preview;
  }
  bool IsMercatorPreview() const { return is_mercator_preview_; }

 private:
  int width_;
  int height_;
  bool is_mercator_preview_;
};

// draw modes
enum {
  DrawVertexes  = 1 << 0,
  DrawBBoxes    = 1 << 1,
  DrawGrids     = 1 << 2,
  DrawQuadCells = 1 << 3,
  DrawTextures  = 1 << 4,
  DrawInvalid   = 1 << 5,
  DrawLabels    = 1 << 6,
  DrawSelected  = 1 << 7,
  ShowCullFrust = 1 << 8,
  SelectBox     = 1 << 9,
  DrawInactive  = 1 << 10,
  AutoLevel     = 1 << 11,
  DrawEnhanced  = 1 << 12,
  DrawDebug     = 1 << 13,
  DrawTileMsg   = 1 << 14,
  // only use filled when you are certain the polygon is convex
  // such as drawing geodes built from bounding boxes
  DrawFilled    = 1 << 15
};

// each source db has these states (up to 32)
enum {
  DB_SHOW       = 1 << 0,
  DB_LOCK       = 1 << 1
};


#define DRAW_BOX_2D(w, e, s, n) \
  glBegin(GL_LINE_LOOP); \
    glVertex2f(w, s); \
    glVertex2f(w, n); \
    glVertex2f(e, n); \
    glVertex2f(e, s); \
  glEnd();

#define VERTEX_NORMAL 1
#define VERTEX_SELECT 2

// Polygon's edge types.
enum gstEdgeType {
  kNormalEdge = 0,  // Regular edge which is present in source polygon.
  kClipEdge = 1,    // The edge which is introduced as result of quad clipping.
  kCutEdge = 2      // The edge which is introduced as result of hole cutting.
};

// A set of gstVertexes
typedef std::vector<gstVertex> gstGeodePart;
typedef std::vector<gstGeodePart> gstGeodeParts;

class gstGeodeImpl;
typedef khRefGuard<gstGeodeImpl> gstGeodeHandle;
typedef std::deque<gstGeodeHandle> GeodeList;
typedef GeodeList::const_iterator GeodeListIterator;
typedef std::vector<gstGeodeHandle> GeodeVector;

// Class gstGeodeImpl is a base class for all geometry-types.
// It defines interface and contains some generic data.
class gstGeodeImpl : public khMTRefCounter {
 public:
  struct RecHeader {
    std::uint32_t type;
    std::uint32_t count;
    std::uint32_t size;
    std::uint32_t fid;      // unique feature id
  };

  virtual ~gstGeodeImpl();

  // determine amount of memory used by getGeodeImpl
  std::uint64_t GetSize() {
    return sizeof(gcount)
    + sizeof(isectCount)
    + sizeof(isectDeepCount)
    + sizeof(prim_type_)
    + sizeof(center_is_valid_)
    + sizeof(center_)
    + sizeof(bounding_box_);
  }

  static gstGeodeHandle Create(const gstPrimType gtype);
  static gstGeodeHandle Create(const gstBBox& box,
                               const gstPrimType gtype = gstPolygon,
                               const double altitude = .0);
  static gstGeodeHandle Create(const gstBBox& box,
                               const gstEdgeType edge_type,
                               const gstPrimType gtype = gstPolygon,
                               const double altitude = .0);
  static gstGeodeHandle FromRaw(const char* buf);

  // TODO: remove to somewhere.
  static void DrawSelectedVertex(const gstDrawState& state,
                                 const gstVertex& vertex);

  virtual gstGeodeHandle Duplicate() const = 0;

  // Note: accept only single geometry-types as parameter, but
  // for geode-collection converts all elements;
  // TODO: remove new_geodeh parameter after support
  // multi-polyline. it is temporary solution to support conversion
  // from multi-polygon to polyline and vice versa.
  virtual void ChangePrimType(gstPrimType new_type,
                              gstGeodeHandle *new_geodeh = NULL) = 0;

  virtual bool IsEmpty() const = 0;

  // Checks if geode is degenerate. Degenerate means that geode is empty or
  // has not enough vertices.
  //
  // @return true if geode is degenerate, otherwise it returns false.
  virtual bool IsDegenerate() const = 0;

  virtual unsigned int NumParts() const = 0;
  virtual unsigned int TotalVertexCount() const = 0;

  virtual void ErasePart(unsigned int part) = 0;

  virtual void Clear() = 0;

  virtual gstBBox BoundingBoxOfPart(unsigned int part) const = 0;

  virtual bool Equals(const gstGeodeHandle &bh,
                      bool reverse_ok = false) const = 0;

  // Computes center of mass and area for specified part.
  // @param part the part index.
  // @param x, y returned coordinates of center of mass.
  // @param area returned area value.
  virtual int Centroid(unsigned int part,
                       double *x, double *y, double *area) const = 0;

  // Moves the given point into the specified polygon.
  // This is useful for ensuring that a point is on the inside of a concave
  // polygon for example.
  // @param part the index of the polygon in parts_ that we are dealing
  // with.
  // @param centroid the centroid of the "part"th polygon.
  // @return a point inside the polygon.
  virtual gstVertex GetPointInPolygon(unsigned int part,
                                      const gstVertex& centroid) const = 0;

  // Computes the "Center" of the geode.
  // For multi-polygon will use the largest area polygon.
  // For concave polygons will find a point inside the polygon
  // (not the centroid).
  virtual gstVertex Center() const = 0;

  virtual void Draw(const gstDrawState& state,
                    const gstFeaturePreviewConfig &preview_config) const = 0;

  virtual void ApplyScaleFactor(double scale) = 0;

  // Flattens a single geode to a set of geodes. For all polygon types flatten
  // multi polygons to multiple single polygon geodes.
  // For polyline flatten multiple polylines into segments(exactly two vertex)
  // and create one geode for each segment. For streets(roads) flatten multiple
  // polylines into multiple single polyline geodes.
  virtual unsigned int Flatten(GeodeList* pieces) const = 0;

  virtual int RemoveCollinearVerts() = 0;

  virtual bool Intersect(const gstBBox& b, uint* part = NULL) const = 0;

  // raw interface
  virtual std::uint32_t RawSize() const = 0;
  virtual char* ToRaw(char* buf) const = 0;

  gstPrimType PrimType() const {
    return static_cast<gstPrimType>(prim_type_);
  }

  gstPrimType FlatPrimType() const {
    return ToFlatPrimType(PrimType());
  }

  const gstBBox& BoundingBox() const {
    if (!bounding_box_.Valid())
      ComputeBounds();
    return bounding_box_;
  }

  void SetBoundingBox(double w, double e, double s, double n) {
    bounding_box_.init(w, e, s, n);
  }

  void ResetBBox() {
    bounding_box_.Invalidate();
  }

  // Invalidates cached data (center and bounding box).
  // @param invalidate_bbox if true, will invalidate the bounding box cache.
  void InvalidateCachedData(bool invalidate_bbox) {
    center_is_valid_ = false;
    if (invalidate_bbox) {
      bounding_box_.Invalidate();
    }
  }

  // count geodes currently in memory
  static int gcount;

  // track intersections
  static int isectCount;
  static int isectDeepCount;

 protected:
  explicit gstGeodeImpl(gstPrimType t);

  virtual void ComputeBounds() const = 0;

  // Defines what this gstGeodeImpl implements. One of gstUnknown, gstPoint,
  // gstPolyLine, gstStreet, gstPolygon, gstPolygon25D, gstPolygon3D, ...
  std::uint8_t prim_type_;  // Though this is gstPrimType we use char for mem-opt

  mutable bool center_is_valid_;

  // Maintain a cache of the center computations.
  // These fields are mutable, since the operation that computes and returns
  // the center is const (i.e., the caller shouldn't need to know about the
  // cache).  Any operation that invalidates the accuracy of the cached
  // center (and bounding_box_ above), must mark them as invalid.
  // Note: that as of 3.2, center handles concave polygons appropriately and
  // puts the point "IN THE POLYGON". This is an expensive calculation so
  // we want to make sure its cached.
  mutable gstVertex center_;

  mutable gstBBox bounding_box_;

 private:
  DISALLOW_COPY_AND_ASSIGN(gstGeodeImpl);
};

// Class gstGeode describes single geometry types: Point, PolyLine, Polygon
// and also Point-collection, PolyLine-collection.
// TODO: add class for Point/PolyLine-collections.
class gstGeode : public gstGeodeImpl {
  friend gstGeodeHandle gstGeodeImpl::Create(gstPrimType gtype);
  friend gstGeodeHandle gstGeodeImpl::FromRaw(const char* buf);

 public:
  // Segment is a line segment having exactly two gstVertex
  class Segment {
   public:
    gstVertex v0;
    gstVertex v1;
    Segment(const gstVertex& _v0, const gstVertex& _v1)
        : v0(_v0), v1(_v1) {}
  };

  virtual ~gstGeode();

  virtual gstGeodeHandle Duplicate() const;

  // backdoor for editor
  // NOTE: Here we can get multi-geometry type from gstPolyLine.
  virtual void ChangePrimType(gstPrimType new_type,
                              gstGeodeHandle *new_geodeh = NULL);

  virtual unsigned int NumParts() const;

  virtual unsigned int TotalVertexCount() const;

  virtual bool IsEmpty() const;
  virtual bool IsDegenerate() const;

  virtual void Clear();

  virtual gstBBox BoundingBoxOfPart(unsigned int part) const;

  virtual bool Equals(const gstGeodeHandle &bh, bool reverse_ok = false) const;

  // Computes center of mass and area for specified part.
  // @param part the part index.
  // @param x, y returned coordinates of center of mass.
  // @param area returned area value.
  virtual int Centroid(unsigned int part, double* x, double* y, double* area) const;

  // Moves the given point into the specified polygon.
  // This is useful for ensuring that a point is on the inside of a concave
  // polygon for example.
  // @param part the index of the polygon in parts_ that we are dealing with.
  // @param centroid the centroid of the "part"th polygon
  // @return a point inside the polygon
  virtual gstVertex GetPointInPolygon(unsigned int part,
                                      const gstVertex& centroid) const;

  // Computes the "Center" of the geode.
  // For concave polygons will find a point inside the polygon
  // (not the centroid).
  virtual gstVertex Center() const;

  virtual void Draw(const gstDrawState& state,
                    const gstFeaturePreviewConfig &preview_config) const;

  virtual void ApplyScaleFactor(double scale);

  // Flatten a single geode to a set of geodes. For all polygon types flatten
  // multi polygons to multiple single polygon geodes.
  // For polyline flatten multiple polylines into segments(exactly two vertex)
  // and create one geode for each segment. For streets(roads) flatten multiple
  // polylines into multiple single polyline geodes.
  virtual unsigned int Flatten(GeodeList* pieces) const;

  virtual int RemoveCollinearVerts();

  virtual bool Intersect(const gstBBox& b, uint* part = NULL) const;

  // raw interface
  virtual std::uint32_t RawSize() const;
  virtual char* ToRaw(char* buf) const;

  // Assumes the first and last points are same, only parts_[0] and num vertices
  // are equal
  bool IsEqual(const gstGeodeImpl* b, bool check_reverse) const;

  unsigned int VertexCount(unsigned int p) const {
    assert(p < parts_.size());
    if (p < parts_.size())
      return parts_[p].size();
    return 0;
  }

  const gstVertex& GetVertex(unsigned int p, unsigned int v) const {
    assert(p < parts_.size());
    assert(v < parts_[p].size());
    return parts_[p][v];
  }

  const gstVertex& GetFirstVertex(unsigned int p) const {
    assert(p < parts_.size());
    assert(!parts_[p].empty());
    return parts_[p].front();
  }

  const gstVertex& GetLastVertex(unsigned int p) const {
    assert(p < parts_.size());
    assert(!parts_[p].empty());
    return parts_[p].back();
  }

  double MinZ(unsigned int p) const {
    assert(p < parts_.size());
    double minZ = 180;
    for (unsigned int i = 0; i < parts_[p].size(); i++) {
      if (parts_[p][i].z < minZ)
        minZ = parts_[p][i].z;
    }
    return minZ;
  }

  double MaxZ(unsigned int p) const {
    assert(p < parts_.size());
    double maxZ = -180;
    for (unsigned int i = 0; i < parts_[p].size(); ++i) {
      if (parts_[p][i].z > maxZ)
        maxZ = parts_[p][i].z;
    }
    return maxZ;
  }

// TODO: function is not used
#if 0
  // Assumptions:
  //   1. This gstGeode and other gstGeode share one end point
  //   2. The underlying type is gstPolyLine representing an acyclic set of
  //      segments. The resulting gstPolyLine may be a cycle with first and
  //      last vertex being same.
  // Notes: this function only operates on part 0
  // Details:
  // this segment: A---B
  // from segment: Y---Z
  //
  // will attempt join in the following order
  //
  // 1.  A---B <> Y---Z
  // 2.  Y---Z <> A---B
  // 3.  A---B <> Z---Y
  // 4.  Z---Y <> A---B
  // One end point of this gstGeode (gstPolyLine) remains unchanged.
  bool Join(const gstGeodeHandle fromh);
#endif

  // Similar to the above, except rather than Y--Z we have a set of segments
  // and join_at_last_vertex true means B is the common point,
  // i.e !join_at_last_vertex means A is the common point.
  // We recreate Y--Z by adding the set of segments as per the direction
  // (the 2nd param of the pair).
  // One end point of this gstGeode (gstPolyLine) remains unchanged.
  void Join(const std::deque<std::pair<gstGeodeImpl*, bool> >& to_join,
            const bool join_at_last_vertex);

  void SetVertexes(const double* buf, std::uint32_t count, bool is25D);

  // Note: invalidates internal data of geode(center and
  // bounding box).
  void ModifyVertex(unsigned int p, unsigned int v, const gstVertex& vertex);

  // Note: invalidates only center data of geode and does not invalidate
  // bounding box.
  void InsertVertex(unsigned int p, unsigned int v, const gstVertex& vertex);

  // Note: invalidates internal data of geode(center and
  // bounding box).
  void DeleteVertex(unsigned int p, unsigned int v);

  // Note: invalidates only center data of geode and does not invalidate
  // bounding box.
  void AddVertex(const gstVertex& v);

  // Note: invalidates only center data of geode and does not invalidate
  // bounding box.
  void AddVertexToPart0(const gstVertex& v);

  void AddPart(int sz);

  // Note: it's not effective because we use vector-container,
  // but it's not used in gstGeode. I added it in gstGeode because I need this
  // functionality in gstGeodeCollection.
  virtual void ErasePart(unsigned int part) {
    parts_.erase(parts_.begin() + part);
  }

  void EraseLastPart() {
    parts_.pop_back();

    if (PrimType() == gstPolygon || PrimType() == gstPolygon25D ||
        PrimType() == gstPolygon3D) {
      // For polygon-types should be invalidated only if we remove outer
      // boundary (geode is empty).
      if (IsEmpty()) {
        InvalidateCachedData(true);
      }
    } else {
      // For other types (except polygons) should be invalidated every time
      // because they can contain multi-geometry features.
      InvalidateCachedData(true);
    }
  }

  // Reverses part p: convert cycle from clockwise orientation to
  // counterclockwise or vice versa.
  // Note: should be used for geode w/o edge-flags (only source
  // geometry).
  void ReversePart(unsigned int p) {
    assert(edge_flags_.empty());
    assert(p < parts_.size());
    gstGeodePart &part = parts_[p];
    std::reverse(part.begin(), part.end());
  }

  // TODO: move this functionality to gstBoxCutter.
  unsigned int BoxCut(const gstBBox& box, GeodeList* pieces) const {
    if (!box.Intersect(BoundingBox()))
      return 0;
    return BoxCutDeep(box, pieces);
  }

  // Simplify the geode such that only vertices whose indices appear
  // in the passed list remain.  Returns the number of vertices erased.
  int Simplify(const std::vector<int>& vertices);

  void AddEdgeFlag(gstEdgeType edge_type) {
    edge_flags_.push_back(static_cast<std::int8_t>(edge_type));
  }

  void AddVertexAndEdgeFlag(const gstVertex& v, gstEdgeType edge_type) {
    AddVertex(v);
    AddEdgeFlag(edge_type);
  }

  bool IsInternalVertex(int i) const {
    assert(i < static_cast<int>(edge_flags_.size()));

    int prev_edge_idx = (i != 0) ? (i - 1) :
        (static_cast<int>(edge_flags_.size()) - 1);

    return (edge_flags_[prev_edge_idx] != kNormalEdge ||
            edge_flags_[i] != kNormalEdge);
  }

  const std::vector<std::int8_t>& EdgeFlags() const {
    return edge_flags_;
  }

  bool Overlaps(const gstGeode *that, double epsilon);

  /////////////////////////////////////////////////////////////////////////

  double Area(unsigned int p) const {
    double area;
    double x, y;
    Centroid(p, &x, &y, &area);
    return area;
  }

  double Volume() const;

  bool IsClipped() const;

  // Computes plane equation.
  //
  // @param normal computed unit normal vector to polygon-plane.
  // @param distance computed distance from polygon-plane to origin.
  // @return true if plane equation is computed, otherwise it returns
  // false (degenerate polygon - couldn't find 3 not collinear points to
  // compute plane equation).
  bool ComputePlaneEquation(gstVertex *normal, double *distance) const;

 protected:
  friend class gstGeodeTest;  // Test class.

  virtual void ComputeBounds() const;

  // Return true if the "part_index"th part is a convex polygon.
  bool IsConvex(unsigned int part_index) const;

 private:
  friend class khRefGuard<gstGeodeImpl>;

  explicit gstGeode(gstPrimType t);

  unsigned int BoxCutDeep(const gstBBox& box, GeodeList* pieces) const;

  bool IntersectDeep(const gstBBox& b, uint* part) const;

  mutable std::int8_t clipped_;    // This takes -1, 0, 1 values

  gstGeodeParts parts_;

  // edge flags specify whether the edge is internal or external
  std::vector<std::int8_t> edge_flags_;

  DISALLOW_COPY_AND_ASSIGN(gstGeode);
};

// Class gstGeodeCollection is a base class for multi-geometry types.
class gstGeodeCollection : public gstGeodeImpl {
 public:
  virtual ~gstGeodeCollection();

  virtual gstGeodeHandle Duplicate() const;

  // backdoor for editor
  // Note: accept only single geometry-types as parameter, but
  // for geode-collection converts all elements;
  virtual void ChangePrimType(gstPrimType new_type,
                              gstGeodeHandle *new_geodeh = NULL);

  virtual bool IsEmpty() const;
  virtual bool IsDegenerate() const;

  virtual unsigned int NumParts() const;
  virtual unsigned int TotalVertexCount() const;

  // Note: it's not effective because we use deque-container.
  // But currently it is called from GeometryChecker only and if we
  // get invalid source geometry and also our deque contains gstGeodeHandle-s.
  virtual void ErasePart(unsigned int part) {
    geodes_.erase(geodes_.begin() + part);
  }

  virtual void Clear();

  virtual gstBBox BoundingBoxOfPart(unsigned int part) const;

  virtual bool Equals(const gstGeodeHandle &bh, bool reverse_ok = false) const;

  // Computes center of mass and area for specified part.
  // @param part the part index.
  // @param x, y returned coordinates of center of mass.
  // @param area returned area value.
  virtual int Centroid(unsigned int part, double* x, double* y, double* area) const;

  // Moves the given point into the specified polygon.
  // This is useful for ensuring that a point is on the inside of a concave
  // polygon for example.
  // @param part the index of the polygon in geodes_ that we are dealing with.
  // @param centroid the centroid of the "part"th polygon.
  // @return a point inside the polygon.
  virtual gstVertex GetPointInPolygon(unsigned int part,
                                      const gstVertex& centroid) const;

  // Compute the "Center" of the geode.
  // For multi-polygon will use the largest area polygon.
  // For concave polygons will find a point inside the polygon
  // (not the centroid).
  virtual gstVertex Center() const;

  virtual void Draw(const gstDrawState& state,
                    const gstFeaturePreviewConfig &preview_config) const;

  virtual void ApplyScaleFactor(double scale);

  // Flatten a single geode to a set of geodes. For all polygon types flatten
  // multi polygons to multiple single polygon geodes.
  // For polyline flatten multiple polylines into segments(exactly two vertex)
  // and create one geode for each segment. For streets(roads) flatten multiple
  // polylines into multiple single polyline geodes.
  virtual unsigned int Flatten(GeodeList* pieces) const;

  virtual int RemoveCollinearVerts();

  virtual bool Intersect(const gstBBox& b, uint* part = NULL) const;

  // raw interface
  virtual std::uint32_t RawSize() const;
  virtual char* ToRaw(char* buf) const;

  // GeodeCollecion interface.
  inline void AddGeode(gstGeodeHandle geode) {
    geodes_.push_back(geode);
  }

  inline gstGeodeHandle& GetGeode(unsigned int part) {
    assert(part < NumParts());
    return geodes_[part];
  }

  inline const gstGeodeHandle& GetGeode(unsigned int part) const {
    assert(part < NumParts());
    return geodes_[part];
  }

 protected:
  explicit gstGeodeCollection(gstPrimType t);

  virtual void ComputeBounds() const;

  GeodeList geodes_;

 private:
  DISALLOW_COPY_AND_ASSIGN(gstGeodeCollection);
};

// Class gstMultiPoly describes multi-polygon geometry types.
class gstMultiPoly : public gstGeodeCollection {
  friend gstGeodeHandle gstGeodeImpl::Create(gstPrimType gtype);
  friend gstGeodeHandle gstGeodeImpl::FromRaw(const char* buf);

 public:
  virtual ~gstMultiPoly();

 private:
  explicit gstMultiPoly(gstPrimType t);

  DISALLOW_COPY_AND_ASSIGN(gstMultiPoly);
};

double ComputeAngle(gstGeodeHandle ah, gstGeodeHandle bh, const gstVertex& v);

////////////////////////////////////////////////////////////////////////////////

class gstRawGeodeImpl;
typedef khRefGuard<gstRawGeodeImpl> gstRawGeode;

class gstRawGeodeImpl : public khRefCounter {
 public:
  static gstRawGeode Create(char* s) {
    return khRefGuardFromNew(new gstRawGeodeImpl(s));
  }

  virtual ~gstRawGeodeImpl() { free(storage); }

  gstGeodeHandle FromRaw() const {
    return gstGeodeImpl::FromRaw(storage);
  }

 private:
  explicit gstRawGeodeImpl(char* s) : storage(s) {}

  char* storage;
};

// Class GeodeCreator accepts geodes and based on accepted geode list and
// specified primary type creates Geode object (single- or multi-geode).
class GeodeCreator {
 public:
  GeodeCreator() {}
  ~GeodeCreator() {}

  gstPrimType PrimType() const {
    return gtype_;
  }

  void SetPrimType(gstPrimType gtype) {
    gtype_ = gtype;
  }

  // Accepts geode.
  inline void Accept(const gstGeodeHandle &geodeh) {
    geode_list_.push_back(geodeh);
  }

  // Reports geode list into specified handle.
  void Report(gstGeodeHandle *geodeh) const;

  // Cleans internal geode list.
  inline void Clean() {
    geode_list_.clear();
  }

 private:
  gstPrimType gtype_;
  GeodeList geode_list_;

  DISALLOW_COPY_AND_ASSIGN(GeodeCreator);
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTGEODE_H__

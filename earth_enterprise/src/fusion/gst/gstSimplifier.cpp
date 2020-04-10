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


// Changes:
// Modified simplifier algorithm: Introduced check for edge length to keep
// the vertices that have at least one coincident edge with length more than
// length threshold (1/8*min(geode_bbox.width, geode_bbox.height).
// It handles very specific case. If source polygons are preliminary partitioned
// and contain holes we get our polygons oversimplified at lower resolution
// levels. It looks especially ugly if vertices coincident to right angle are
// reduced. So main goal is to keep these vertices.
// To be sure that we cull the vertices that are insignificant for
// display we introduce "weak" simplification threshold. The "weak"
// simplification threshold is calculated based on resolution at level:
// current_level + 6 (+3 more levels below the level for calculating
// simplification threshold). Max resolution level is 18. So at max resolution
// level we calculate the "weak" simplification threshold based on level 24
// (Max visibility level).
//

#include "fusion/gst/gstSimplifier.h"

#include <vector>

#include "fusion/gst/gstConstants.h"
#include "common/khTileAddr.h"


// Implementation of Douglas-Peucker line simplification - start with
// a base segment, then refine it by iteratively adding back the vertex
// farthest from its approximating segment.

// A line segment represents two vertices from a geode which are
// connected in the current approximation.  Implicitly, every vertex
// falling between them is approximated by the line connecting them.
// A line segment also records the vertex which is worst-approximated
// by it.  Note that this code is written to allow wrapping of vertex
// indices (eg, a segment of a geode with 10 vertices could start
// at vertex 8 and end at vertex 2, meaning it covers 8, 9, 0, 1, 2).
// This allows the same code to handle polygon simplification as well.
class LineSegment {
  friend class gstSimplifier;

 public:
  LineSegment() { }
  LineSegment(int start, int end) : start_(start), end_(end) { }
  LineSegment(const LineSegment& seg) { *this = seg; }

  const LineSegment& operator=(const LineSegment& seg) {
    start_ = seg.start_;
    end_ = seg.end_;
    max_dist_ = seg.max_dist_;
    max_v_ = seg.max_v_;
    return *this;
  }

  double max_dist() const { return max_dist_; }
  int max_v() const { return max_v_; }

  void Update(const gstGeode *geode);
  void Split(const gstGeode *geode, LineSegment *out);
  bool IsSplittable() const;

 private:
  void set_start(int start) { start_ = start; }
  void set_end(int end) { end_ = end; }

  unsigned int start_, end_;
  double max_dist_;
  int max_v_;
};


// The line segment has changed; recompute the worst-approximated point.
void LineSegment::Update(const gstGeode *geode) {
  assert(start_ < geode->VertexCount(0));
  assert(end_ < geode->VertexCount(0));
  assert(start_ < end_);

  max_dist_ = 0;
  max_v_ = start_;

  if (!IsSplittable()) return;

  const gstVertex& v1 = geode->GetVertex(0, start_);
  const gstVertex& v2 = geode->GetVertex(0, end_);
  for ( unsigned int i = start_ + 1; i != end_; ++i ) {
    double dist = sqrt(geode->GetVertex(0, i).Distance(v1, v2));
    if (dist <= max_dist_) continue;
    max_dist_ = dist;
    max_v_ = i;
  }

  // Now account for curvature of the earth.  The maximum error will occur
  // at the midpoint of the segment.  If this error is smaller than the
  // largest vertex error, we're fine.  This formula is derived from taking
  // the chord connecting v1 to v2, finding the midpoint, and constructing
  // the triangle from the center of the earth to v1 to the midpoint, using
  // pythagorous to get the length of the edge from earth center to the
  // midpoint, and subtracting from the earth radius to get the distance
  // from the midpoint to the earth surface.  The earth radius is derived
  // from the circumference of the earth being 1.0 units in our
  // parameterization.
  // Also note this error calculation overestimates error at higher
  // latitudes (near the poles) due to the parametric distortion.  And,
  // an underlying assumption here is that ensuring a pixel-level error
  // in the z direction (out from the center of the earth) will be good
  // enough to avoid occlusion errors from the curvature of the earth.
  // This is not guaranteed.

  const double earth_radius = 1.0 / (2.0 * M_PI);
  double half = v1.Distance2D(v2) / 2.0;
  double error = earth_radius - sqrt(earth_radius*earth_radius - half*half);
  if (error <= max_dist_) return;

  // We reassign the maximum error to be the curvature error, and blame it
  // on the vertex closest to the midpoint.  Note that this doesn't guarantee
  // the curvature error will eventually be satisfied, since the source
  // geode may need refinement in order to do so.

  max_dist_ = error;

  gstVertex midpoint(v1.x + 0.5*(v2.x - v1.x),
                     v1.y + 0.5*(v2.y - v1.y),
                     .0);

  max_v_ = start_ + 1;
  double best = geode->GetVertex(0, max_v_).Distance2D(midpoint);
  for (unsigned int i = start_ + 2; i != end_; ++i) {
    double dist = geode->GetVertex(0, i).Distance2D(midpoint);
    if (dist < best) {
      best = dist;
      max_v_ = i;
    }
  }
}


// Split the line segment into two at the worst-approximated point.
void LineSegment::Split(const gstGeode *geode, LineSegment* out) {
  out->set_start(max_v_);
  out->set_end(end_);
  out->Update(geode);

  end_ = max_v_;
  this->Update(geode);
}


// A line segment is splittable if it has at least one vertex being
// approximated by it.
bool LineSegment::IsSplittable() const {
  return (start_ + 1 != end_);
}


// Compute the tolerable error at a given level of detail.
// This formula assumes:
// - the resolution doubles with each additional level
// - objects may be drawn up to 3 levels below the one at which
//   they are processed (fade-out is hardcoded to begin 2 levels
//   below, and lasts for 1 level)
// - the parameterization of the earth is within the unit square
//   [0,1] x [0,1].
//
// For example,
// The circumference of the earth at level 0 is 256 pixels.
// The half-pixel error at level 0 would be 0.5 / 256,
// since level 0 has 256 pixels covering the parameter space of [0,1]
// (each pixel is 1/256 units).
//
// The loopcount is for progressively destroying features when there's
// too many to fit in a packet.  Each additional loop doubles the
// effective threshold and of course rapidly introduces pixel level error.
void gstSimplifier::ComputeThreshold(int level, int loopcount) {
  threshold_ = allowable_error_ /
      8.0 / ClientVectorTilespace.pixelsAtLevel0 / pow(2.0, level);

  threshold_ *= pow(2.0, loopcount);

  threshold_weak_ = threshold_ / 8.0;  // +3 more levels below.

  notify(NFY_DEBUG, "level: %d, loopcount: %d, threshold_: %.20f",
         level, loopcount, threshold_);
}


// Simplify the given geode according to the Simplifier's current
// threshold.  On return, log is populated with a list of vertex indices.
// Each vertex with an index in the list should appear in the approximation.
// Further, this list is sorted in order according to importance.  In other
// words, a prefix of the list of length n is the best approximation
// with n vertices.  The simplification algorithm applied here is
// Douglas-Peucker line simplification.
double gstSimplifier::Simplify(gstGeodeHandle geodeh,
                               std::vector<int>* log) const {
  // Note: Simplifier does not handle multi-polygon
  // features. And we can't get such geodes after qt partitioning in 3D fuse
  // pipeline.
  if (geodeh->PrimType() == gstMultiPolygon ||
      geodeh->PrimType() == gstMultiPolygon25D ||
      geodeh->PrimType() == gstMultiPolygon3D) {
    notify(NFY_FATAL, "%s: Improper geometry type %d.",
           __func__, geodeh->PrimType());
  }

  // Note: Simplifier does not handle multi-part geometry.
  // And we can't get such geodes after qt partitioning in 3D fuse pipeline.
  if (geodeh->NumParts() > 1) {
    notify(NFY_FATAL, "%s: Multi-part geometry is not supported.", __func__);
  }

  log->clear();

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  // Do not simplify degenerate geodes.
  if (geode->IsDegenerate())
    return 0.0;  // No simplification done.

  std::vector<LineSegment> segments;

  // First compute the distance from each uninserted vertex to the current
  // segments.

  switch (geode->PrimType()) {
    case gstPolyLine:
    case gstPolyLine25D:
    case gstStreet:
    case gstStreet25D:
      {
        // Do not simplify polylines with kMinPolylineVertices vertices.
        if (geode->VertexCount(0) <= kMinPolylineVertices) {
          for (unsigned int i = 0; i < geode->VertexCount(0); ++i)
            log->push_back(i);
          return 0.0;  // No simplification done.
        }
        // For polylines and streets, we have to preserve the endpoints.

        LineSegment segment(0, geode->VertexCount(0) - 1);
        segment.Update(geode);
        segments.push_back(segment);

        log->push_back(0);
        log->push_back(geode->VertexCount(0) - 1);
      }
      break;

    case gstPolygon:
    case gstPolygon25D:
      {
        // For polygons, we have to preserve any edge which has its
        // edge flag set - these edges lie on quad boundaries or they are
        // cutting hole edges, and will introduce cracks if changed. We also
        // have to preserve the edge from (VertexCount-1) to 0.
        const std::vector<std::int8_t>& edge_flags = geode->EdgeFlags();

        if (geode->GetFirstVertex(0) != geode->GetLastVertex(0)) {
          geode->AddVertex(geode->GetFirstVertex(0));
          geode->AddEdgeFlag(static_cast<gstEdgeType>(edge_flags.back()));
        }

        assert(geode->VertexCount(0) >= kMinCycleVertices);

        // Do not simplify polygons with kMinCycleVertices vertices.
        if (geode->VertexCount(0) <= kMinCycleVertices) {
          for (unsigned int i = 0; i < geode->VertexCount(0); ++i)
            log->push_back(i);
          return 0.0;  // No simplification done.
        }

        unsigned int start = 0;
        bool have_start = false;
        for (unsigned int i = 0; i < geode->VertexCount(0) - 1; ++i) {
          if (kNormalEdge == edge_flags[i]) {
            if (!have_start) {
              start = i;
              have_start = true;
              log->push_back(i);
            }

            continue;
          }

          log->push_back(i);

          if (have_start) {
            if (i - start > 1) {
              LineSegment segment(start, i);
              segment.Update(geode);
              segments.push_back(segment);
            }
            have_start = false;
          }
        }

        // Close off the final segment, if there is one, using the
        // last vertex of the polygon (which is in fact equal to
        // vertex 0 and must be preserved).

        if (have_start && (start < geode->VertexCount(0) - 2)) {
          LineSegment segment(start, geode->VertexCount(0) - 1);
          segment.Update(geode);
          segments.push_back(segment);
        }

        log->push_back(geode->VertexCount(0) - 1);
        if (segments.empty()) {
          assert(log->size() == geode->VertexCount(0));
          return 0.0;  // No simplification done.
        }
      }

      break;

    default:
      for (unsigned int i = 0; i < geode->VertexCount(0); ++i) {
        log->push_back(i);
      }
      return 0.0;  // No simplification done.
  }

  // Find the segment with the worst vertex.
  // TODO: use a heap or something here instead of doing this linear
  // search all the time.

  int best = 0;
  for (unsigned int i = 1; i < segments.size(); ++i) {
    if (segments[i].max_dist() > segments[best].max_dist()) {
      best = i;
    }
  }

  // Iteratively insert the vertex with largest distance until all
  // remaining vertices are within the threshold of their approximating
  // segments or length of remaining edges within the length threshold.
  // Note that for polygon simplification, we must additionally
  // ensure we have at least 4 vertices in order to have a valid polygon.

  // Note: Check for edge length is introduced to handle
  // very specific case. If source polygons are preliminary partitioned
  // and contain holes we get our polygons oversimplified at lower resolution
  // levels. It looks especially ugly if vertices coincident to right angle are
  // reduced. So, with this additional check we try to keep these vertices.

  // Edge length threshold is 1/8 of minimal from geode's bounding box
  // dimensions.
  double edge_length_threshold =
      (geode->BoundingBox().Width() < geode->BoundingBox().Height()) ?
      geode->BoundingBox().Width() : geode->BoundingBox().Height();

  edge_length_threshold /= 8.0;

  while (!segments.empty() &&
         (segments[best].max_dist() > threshold_ ||
          (segments[best].max_dist() > threshold_weak_ &&
           (geode->GetVertex(0, segments[best].start_).Distance2D(
              geode->GetVertex(0, segments[best].max_v_)) >
            edge_length_threshold ||
            geode->GetVertex(0, segments[best].end_).Distance2D(
                geode->GetVertex(0, segments[best].max_v_)) >
            edge_length_threshold)) ||
          (geode->FlatPrimType() == gstPolygon &&  // gstPolygon/gstPolygon25D
           log->size() < kMinCycleVertices))) {
    assert(segments[best].IsSplittable());

    log->push_back(segments[best].max_v());

    LineSegment next;
    segments[best].Split(geode, &next);

    // Remove any side of the split segment that cannot be split anymore.
    if (!segments[best].IsSplittable())
      segments.erase(segments.begin() + best);
    if (next.IsSplittable())
      segments.push_back(next);

    // Find the next worst vertex.
    best = 0;
    for (unsigned int i = 1; i < segments.size(); ++i) {
      if (segments[i].max_dist() > segments[best].max_dist()) {
        best = i;
      }
    }
  }

  if (segments.empty()) {
    assert(log->size() == geode->VertexCount(0));
    return 0.0;  // No simplification done.
  }

  // Return the maximum error of the simplified geode.
  double max_simplification_error = segments[best].max_dist();
  return max_simplification_error;
}


// Test for whether a given geode's size is below the current
// simplification threshold.  This can be used to cull the geode
// when it is too small to affect the display.
bool gstSimplifier::IsSubpixelFeature(const gstGeodeHandle &geodeh) const {
  switch ( geodeh->PrimType() ) {
    case gstPolyLine:
    case gstPolyLine25D:
    case gstStreet:
    case gstStreet25D:
    case gstPolygon:
    case gstPolygon25D: {
      gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
      if (geode->VertexCount(0) <= 1) return true;

      return (geode->BoundingBox().Diameter() < threshold_);
    }
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      // TODO: currently we can't get multi-polygon features here.
      notify(NFY_FATAL, "%s: Improper geometry type %d.",
             __func__, geodeh->PrimType());
      return (geodeh->BoundingBox().Diameter() < threshold_);
      break;

    default:
      return false;
      break;
  }
}

// class gstFeatureCuller
// Compute the tolerable error at a given level of detail.
// This formula assumes:
// - the resolution doubles with each additional level.
void gstFeatureCuller::ComputeThreshold(int level) {
  threshold_ = allowable_error_ /
      ClientVectorTilespace.pixelsAtLevel0 / pow(2.0, level);
}

bool gstFeatureCuller::IsSubpixelFeature(const gstGeodeHandle &geodeh) const {
  switch (geodeh->PrimType()) {
    case gstPolyLine:
    case gstPolyLine25D:
    case gstStreet:
    case gstStreet25D:
    case gstPolygon:
    case gstPolygon25D: {
      gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
      if (geode->VertexCount(0) <= 1) return true;

      return (geode->BoundingBox().Diameter() < threshold_);
    }
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      // TODO: currently we can't get multi-polygon features here.
      notify(NFY_FATAL, "%s: Improper geometry type %d.",
             __func__, geodeh->PrimType());
      return (geodeh->BoundingBox().Diameter() < threshold_);
      break;

    default:
      return false;
      break;
  }
}

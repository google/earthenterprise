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

// Provides methods to solve geometric problems connected with lines,
// segments operations:
//
// 1) Class SegmentClipper implements segment clipping against rectangle window
// using Liang-Barsky line clipping algorithm.
// Algorithm was implemented in class PolygonClipper.
//
// Changes: Implementation of segment clipping is extracted to separate class
// Added unit tests, fixed bugs, commented.
//
// 2) Function ComputePlaneEquation()- computes plane equation through
// 3 given point (function is moved from gstGeode).
//
// 3) Function Collinear()- estimates whether points are approximately
// collinear in 2D (function is moved from gstGeode).
//
// 4) Function Collinear3D()- estimates whether points are approximately
// collinear in 3D.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTGEOMUTILS_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTGEOMUTILS_H_

#include "fusion/gst/gstVertex.h"
#include "fusion/gst/gstMathUtils.h"

namespace fusion_gst {

// Computes plane equation through 3 given point.
//
// @param a, b, c three input points.
// @param normal computed unit normal vector to plane.
// @param distance computed distance to origin.
// @return true if plane equation is computed, otherwise it returns false
// (collinear points - plane equation can't be computed).
bool ComputePlaneEquation(const Vert3<double> &a,
                          const Vert3<double> &b,
                          const Vert3<double> &c,
                          Vert3<double> *normal,
                          double *distance);

// Estimates whether points are approximately (with specified tolerance)
// collinear in 2D.
//
// @param a, b, c three input points.
// @param epsilon tolerance value. it equals sin(x) where x is the maximum
// acceptable angle between two vectors derived from input points that can
// still be considered collinear.
// @return true if points are collinear, otherwise it returns false.
bool Collinear(const gstVertex &a, const gstVertex &b, const gstVertex &c,
               const double epsilon = 1e-5);

// Estimates whether points are approximately (with specified tolerance)
// collinear in 3D.
//
// @param a, b, c three input points.
// @param epsilon tolerance value. it equals sin(x) where x is the maximum
// acceptable angle between two vectors derived from input points that can
// still be considered collinear.
// @return true if points are collinear in 3D, otherwise it returns false.
bool Collinear3D(const gstVertex &a, const gstVertex &b, const gstVertex &c,
                 const double epsilon = 1e-5);

// Computes y-coordinate of point on line by its x-coordinate.
// Note: line should not be parallel to y-axis.
// pt1, pt2 - line points.
// x - x-coordinate of point on line.
// return y coordinate.
template<class Point>
double GetLineYFromX(const Point &pt1, const Point &pt2, const double x) {
  double dx = pt2.x - pt1.x;
  assert(!Equals(dx, .0));
  double dy = pt2.y - pt1.y;
  return (pt1.y + dy * (x - pt1.x) / dx);
}

// Computes x-coordinate of point on line by its y-coordinate.
// Note: line should not be parallel to x-axis.
// pt1, pt2 - line points.
// y - y-coordinate of point on line.
// return x coordinate.
template<class Point>
double GetLineXFromY(const Point &pt1, const Point &pt2, const double y) {
  double dy = pt2.y - pt1.y;
  assert(!Equals(dy, .0));
  double dx = pt2.x - pt1.x;
  return (pt1.x + dx * (y - pt1.y) / dy);
}

// Computes z-coordinate of point on line by its xy-coordinates.
// pt1, pt2 - line points.
// out - output point.
template<class InPoint, class OutPoint>
void GetLineZFromXY(const InPoint &pt1, const InPoint &pt2,
                    OutPoint *out) {
  double dz = pt2.z - pt1.z;
  double dx = pt2.x - pt1.x;

  if (!Equals(dx, .0)) {
    out->z = pt1.z + dz * (out->x - pt1.x) / dx;
  } else {
    double dy = pt2.y - pt1.y;
    if (!Equals(dy, .0)) {
      out->z = pt1.z + dz * (out->y - pt1.y) / dy;
    } else {  // degenerate line in 2D plane.
      out->z = pt1.z;
    }
  }
}

// Checks location of point c relative to directed line specified
// by points: a, b.
// Function returns orientation code:
//   0 - point lies on the line;
//   1 - point is on the left from the directed line;
//  -1 - point is on the right from the directed line.
int Orientation(const gstVertex &a, const gstVertex &b, const gstVertex &c);

// Checks for intersection segments specified by endpoints.
// Note: segment coincidence is not considered.
bool AreSegmentsIntersecting(const gstVertex &a, const gstVertex &b,
                             const gstVertex &c, const gstVertex &d);

// Calculates intersection point of lines (a, b) vs (c, d) in 2D space.
// Note: z-coordinate is ignored with respect to intersection
// but is filled in based on value of z on line (a, b) at intersection point.
// Note: before calculating lines intersection, we should always
// check if they are intersecting.
gstVertex LinesIntersection2D(const gstVertex &a, const gstVertex &b,
                              const gstVertex &c, const gstVertex &d);

// SegmentClipper processor.
// Implements segment clipping against rectange window using
// Liang-Barsky line clipping algorithms.
//
// Note Do report as intersection if segment touches clipping
// window or intersects clipping window corner. But it can be modified if
// required (see comments in implementation).
//
// Using:
//   SegmentClipper segm_clipper;   - create SegmentClipper.
//   segm_clipper.SetClipRect(...); - set clipping rectangle.
//   segm_clipper.Run();            - run clipping.
class SegmentClipper {
 public:
  SegmentClipper() : wx1_(.0), wx2_(.0), wy1_(.0), wy2_(.0) {}

  SegmentClipper(double _wx1, double _wx2, double _wy1, double _wy2)
      : wx1_(_wx1), wx2_(_wx2), wy1_(_wy1), wy2_(_wy2) {
  }

  // Sets clipping window.
  //
  // @param wx1, wx2, wy1, wy2 rectange window coordinates.
  void SetClipRect(double wx1, double wx2,
                   double wy1, double wy2) {
    wx1_ = wx1;
    wx2_ = wx2;
    wy1_ = wy1;
    wy2_ = wy2;
  }

  // Runs segment clipping.
  // If the input segment completely lies inside the clipping window, then adds
  // the two end points of the segment that represents the intersection of the
  // input segment and the clipping window to the intersections parameter.
  // If the segment completely crosses the clipping window, then both
  // intersection points (end points of clipped edge) will be on the clipping
  // window's edges. Otherwise, one of the intersection points will be the one
  // of the end points from the input segment, which lies within the clipping
  // window and another one will be on the clipping window's edge.
  //
  // @param v1 first end point of input segment.
  // @param v2 second end point of input segment.
  // @param intersections intersection points list (result of clipping).
  // Note The intersection points in list are ordered along an input segment
  // from first to second end point.
  //
  // @return true if there is an intersection.
  //         false if there is no intersection.
  bool Run(const gstVertex &v1, const gstVertex &v2,
           gstVertexVector &intersections) const;

 private:
  // Clipping rectangle window coordinates.
  double wx1_;
  double wx2_;
  double wy1_;
  double wy2_;
};

}  // namespace fusion_gst

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTGEOMUTILS_H_

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

// gstPolygonUtils is a static algorithm class which provides methods to
// solve the following geometric problems:
// 1) Finding the closest point on the boundary of a polygon to a given point
// 2) Fast computation of a point inside the polygon given a point within the
//    polygons bounding box.
// 3) Taking vertical and horizontal slices through the polygon and
//    determining the largest piece of overlap of the slice line with the
//    interior of the polygon (used by 2).

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTPOLYGONUTILS_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTPOLYGONUTILS_H_

#include <vector>

#include "fusion/gst/gstVertex.h"

class gstPolygonUtilsTest;

class gstPolygonUtils {
 private:
    gstPolygonUtils() {}  // This is a static class.
 public:

  // Given a 2D polygon (specified by a list of vertices) and point somewhere
  // near the polygon, find a point inside the polygon that lies along either
  // the horizontal or vertical line passing through the specified point.
  //
  // vertex_list: the list of 2D vertices of the polygon
  //              assumes first and last point are the same
  //              not a degenerate polygon (self intersecting)
  //              may have redundant/duplicated points
  // origin: specifies the x and y origin along which we'll search for the
  //         point in the polygon. We will search along the horizontal and
  //         vertical line passing through the origin point.  To get a point
  //         within the polygon, the origin needs to lie on a horizontal or
  //         vertical line that intersects the interior of the polygon.
  // return: a vertex which lies inside the polygon and along either the
  //         x- or y-axis passing through the specified origin.
  //         for degenerate inputs (or where the origin does not lie on
  //         an intersecting horizontal or vertical line), origin is returned.
  // Note: This algorithm is designed to give a result quickly rather than some
  // notion of the best result (which would require an expensive triangulation
  // of the polygon).
  // There may be polygons with which this algorithm gives points that are
  // aesthetically too close to the polygon edge, but these will be fairly rare
  // occurences.
  static gstVertex FindPointInPolygonRelativeToPoint(
    const std::vector<gstVertex>& vertex_list,
    const gstVertex& origin);

  // Given a 2D point and a polygon (specified as a list of 2D vertices),
  // compute the closest distance from the point to the edge of the polygon.
  // vertex_list: the list of 2D vertices of the polygon
  //              assumes first and last point are the same
  //              not a degenerate polygon (self intersecting)
  //              may have redundant/duplicated points
  // point: the point with which we'll compute the closest point on the polygon
  // closest_point: on output: the closest point to "point" on the boundary of
  //                the polygon
  // distance: on output: the distance between point and closest_point
  static void GetClosestPointOnPolygonBoundary(
    const std::vector<gstVertex>& vertex_list,
    const gstVertex& point, gstVertex* closest_point, double* distance);

  // Return true if the polygon specified by vertex_list is a convex polygon.
  // vertex_list: the list of 2D vertices of the polygon
  //              assumes first and last point are the same
  //              not a degenerate polygon (self intersecting)
  //              may have redundant/duplicated points
  static bool IsConvex(const std::vector<gstVertex>& vertex_list);

  // Intersects the polygon with a horizontal (y == constant) line
  // and compute the center and length of the largest piece of overlap
  // of the line with the polygon.
  // vertex_list: the list of 2D vertices of the polygon
  //              assumes first and last point are the same
  //              not a degenerate polygon (self intersecting)
  //              may have redundant/duplicated points
  // y_intercept: The y-intercept of the horizontal line to intersect the
  //              polygon with
  // coordinate: ouput the center of the largest piece of overlap
  //             of the line with the polygon.
  // length_of_largest_piece: the length of the largest piece of overlap.
  // On degenerate inputs (invalid polygons or when no overlap is found),
  // the output values are set to 0.
  static void HorizontalOverlap(const std::vector<gstVertex>& vertex_list,
                                double y_intercept,
                                double* coordinate,
                                double* length_of_largest_piece);

  // Intersect the polygon with a vertical (x == constant) line
  // and compute the center and length of the largest piece of overlap
  // of the line with the polygon.
  // vertex_list: the list of 2D vertices of the polygon
  //              assumes first and last point are the same
  //              not a degenerate polygon (self intersecting)
  //              may have redundant/duplicated points
  // x_intercept: The x-intercept of the vertical line to intersect the
  //              polygon with
  // coordinate: ouput the center of the largest piece of overlap
  //             of the line with the polygon.
  // length_of_largest_piece: the length of the largest piece of overlap.
  // On degenerate inputs (invalid polygons or when no overlap is found),
  // the output values are set to 0.
  static void VerticalOverlap(const std::vector<gstVertex>& vertex_list,
                              double x_intercept,
                              double* coordinate,
                              double* length_of_largest_piece);

  // We need to keep track of certain information about each edge crossing
  // of the line being intersected.  We've boiled it down to 4 cases.
  enum CrossingType {
    kCrossing,  // The edge crosses the line normally.
    kCollinearFromBelow,  // The edge meets the line at a vertex
                          // from below the line.
    kCollinearFromAbove,  // The edge meets the line at a vertex
                          // from above the line.
    kTouchingButNotCrossing,  // Two edges intersect at the line but do not
                              // cross the line.
  };

  // A "struct" to help track of intersections.
  class PolygonCrossing {
   public:
    // Construct a PolygonCrossing object.
    // point: the coordinate of the crossing.
    // type: the type of the crossing
    PolygonCrossing(double coordinate, CrossingType type)
    : coordinate(coordinate), crossing_type(type) {}

    // Overloaded operator== needed for gtest.
    bool operator==(const PolygonCrossing& other) const {
      return (coordinate == other.coordinate &&
              crossing_type == other.crossing_type);
    }

    double coordinate;
    CrossingType crossing_type;

    // Intersections are sorted by coordinate only.
    // Our code assumes no degenerate (self-intersecting polygons), so
    // that there should not be overlapping coordinates in collections of
    // crossings along a line.
    static bool LessThan(const PolygonCrossing& a,
                         const PolygonCrossing& b) {
      return a.coordinate < b.coordinate;
    }
  };

 protected:
  friend class gstPolygonUtilsTest;
  // Intersect the polygon with a horizontal (y = constant) line
  // returning a list of crossing coordinates (unsorted).
  // Each coordinate will be marked to indicate the type of crossing (to
  // be used for further analysis of the overlap of the line and the polygon).
  // vertex_list: the list of 2D vertices of the polygon
  //              assumes first and last point are the same
  //              not a degenerate polygon
  //              may have redundant/duplicated points
  // y_intercept: The y-intercept of the horizontal line to intersect the
  //              polygon with
  // crossings: output vector of crossing coordinates and types.
  //            The elements of crossings will be sorted by their
  //            PolygonCrossing::coordinate value.
  static void ComputeHorizontalCrossings(
    const std::vector<gstVertex>& vertex_list,
    double y_intercept,
    std::vector<gstPolygonUtils::PolygonCrossing>* crossings);

  // Given a set of PolygonCrossings (representing the crossing points of
  // edges with a line going through the polygon), compute the
  // center and length of the largest piece of overlap.
  // crossings: vector of crossing coordinates and types
  //            The elements of crossings MUST BE SORTED before
  //            calling this routine.
  // center_of_longest_piece: ouput the center of the largest piece of overlap
  //                           of the line with the polygon.
  // length_of_largest_piece: the length of the largest piece of overlap.
  static void LargestOverlapStats(
    std::vector<PolygonCrossing>& crossings,
    double* center_of_longest_piece,
    double* length_of_longest_piece);
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTPOLYGONUTILS_H_

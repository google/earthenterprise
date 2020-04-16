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


// Unit Tests for gstGeode

#include <algorithm>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "fusion/gst/gstPolygonUtils.h"
#include "fusion/gst/gstVertex.h"


// gtest needs to be able to print out classes we're comparing
std::ostream &operator<<(std::ostream &strm,
                         const gstPolygonUtils::PolygonCrossing &v) {
  strm << v.crossing_type << " : " << v.coordinate;
  return strm;
}
std::ostream &operator<<(std::ostream &strm, const gstVertex &v) {
  strm << "(" << v.x << ", " << v.y << ")";
  return strm;
}

// gstPolygonUtilsTest test class which
// provides some access to protected methods of gstPolygonUtils.
class gstPolygonUtilsTest : public testing::Test {
 public:
  // Wrapper to test protected method:
  // gstPolygonUtils::ComputeHorizontalCrossings().
  // see docs for that method as parameters are the same.
  static void ComputeHorizontalCrossings(
    const std::vector<gstVertex>& vertex_list,
    double y_intercept,
    std::vector<gstPolygonUtils::PolygonCrossing>* crossings) {
    gstPolygonUtils::ComputeHorizontalCrossings(vertex_list,
                                                y_intercept,
                                                crossings);
  }

  // Wrapper to test protected method:
  // gstPolygonUtils::LargestOverlapStats().
  // see docs for that method as parameters are the same.
  static void LargestOverlapStats(
    std::vector<gstPolygonUtils::PolygonCrossing>& crossings,
    double* center_of_longest_piece,
    double* length_of_longest_piece) {
      gstPolygonUtils::LargestOverlapStats(crossings,
                                           center_of_longest_piece,
                                           length_of_longest_piece);
  }
};

// Utility method for checking the results of GetClosestPointOnPolygonBoundary.
// vertex_list: defining the polygon
// point: the point from which we're searching for the closest point on
//        the polygon boundary
// expected_closest_point: the expected closest point
// message: the additional error message for failures to help identify the
//          failed test.
void CheckClosestPoint(
  const std::vector<gstVertex>& vertex_list,
  const gstVertex& point,
  const gstVertex& expected_closest_point,
  const std::string& message) {
  // Test GetClosestPointOnPolygonBoundary
  gstVertex closest_point;
  double distance;
  gstPolygonUtils::GetClosestPointOnPolygonBoundary(vertex_list, point,
                                                    &closest_point,
                                                    &distance);
  ASSERT_EQ(expected_closest_point, closest_point) << message;
  double expected_distance = point.Distance(expected_closest_point);
  ASSERT_EQ(expected_distance, distance) << message;
}

TEST_F(gstPolygonUtilsTest, TestGetClosestPointOnPolygonBoundary) {
  gstVertex point;
  gstVertex expected_closest_point;
  { // Triangle...can test all of our cases with just this.
    std::vector<gstVertex> vertex_list;
    vertex_list.push_back(gstVertex(0.0, 0.0));
    vertex_list.push_back(gstVertex(4.0, 0.0));
    vertex_list.push_back(gstVertex(2.0, 2.0));
    vertex_list.push_back(vertex_list[0]);
    // Point outside polygon nearest a vertex.
    point = gstVertex(-1, -1);
    expected_closest_point = gstVertex(0, 0);
    CheckClosestPoint(vertex_list, point,
                      expected_closest_point, "outside triangle near vertex 1");
    // Point just outside polygon nearest a vertex...will test our
    // bounding box and projection computation optimizations.
    point = gstVertex(2.1, 2.1);
    expected_closest_point = gstVertex(2, 2);
    CheckClosestPoint(vertex_list, point,
                      expected_closest_point, "outside triangle near vertex 2");
    // Point outside polygon nearest an edge...test edge #2.
    point = gstVertex(0.75, 1.25);
    expected_closest_point = gstVertex(1, 1);
    CheckClosestPoint(vertex_list, point,
                      expected_closest_point, "outside triangle near edge 1");
    // Point outside polygon nearest an edge.
    point = gstVertex(1.25, 1.75);
    expected_closest_point = gstVertex(1.5, 1.5);
    CheckClosestPoint(vertex_list, point,
                      expected_closest_point, "outside triangle near edge 2");
    // Point outside polygon nearest an edge....test edge #1.
    point = gstVertex(3.25, 1.25);
    expected_closest_point = gstVertex(3, 1);
    CheckClosestPoint(vertex_list, point,
                      expected_closest_point, "outside triangle near edge 3");
    // Point inside polygon nearest an edge.
    point = gstVertex(1.25, 0.75);
    expected_closest_point = gstVertex(1, 1);
    CheckClosestPoint(vertex_list, point,
                      expected_closest_point, "inside triangle near edge 1");
    // Point inside polygon nearest an edge...test edge #0.
    point = gstVertex(.75, .25);
    expected_closest_point = gstVertex(0.75, 0.0);
    CheckClosestPoint(vertex_list, point,
                      expected_closest_point, "inside triangle near edge 2");
  }
}  // end TestGetClosestPointOnPolygonBoundary

// Utility method for validating crossing values given a polygon and y-intercept
// compared to expected results.
// y-intercept: the y-value defining a horizontal line used to intersect the
//              polygon
// vertex_list: defining the polygon
// origin: the origin used for the FindPointInPolygonRelativeToPoint test
// expected_center: expected center result of FindPointInPolygonRelativeToPoint
//                  given the vertex_list polygon and origin point
// message: the additional error message for failures to help identify the
//          failed test.
void CheckPointInPolygon(
  const std::vector<gstVertex>& vertex_list,
  const gstVertex& origin,
  const gstVertex& expected_center,
  const std::string& message) {
  // Test FindPointInPolygonRelativeToPoint (the highest level routine)
  gstVertex center =
    gstPolygonUtils::FindPointInPolygonRelativeToPoint(vertex_list, origin);
  ASSERT_EQ(expected_center, center) << message;
}

// Here we test gstPolygonUtils::FindPointInPolygonRelativeToPoint
// which is the principal method used in gstPolygonUtils
// We devote much of the special case testing to the low level routines which
// do all the work in TestComputeCrossings.
// FindPointInPolygonRelativeToPoint is a simple wrapper of those and here we
// just want to test the basic behavior of the wrapper.
TEST_F(gstPolygonUtilsTest, TestFindPointInPolygonRelativeToPoint) {
  gstVertex expected_center;
  gstVertex origin;
  { // Triangle: try FindPointInPolygonRelativeToPoint on a triangle with
    // various origin parameters.
    std::vector<gstVertex> vertex_list;
    vertex_list.push_back(gstVertex(0.0, 0.0));
    vertex_list.push_back(gstVertex(4.0, 0.0));
    vertex_list.push_back(gstVertex(2.0, 2.0));
    vertex_list.push_back(vertex_list[0]);

    origin = gstVertex(2.0, 1.0);
    expected_center = gstVertex(2, 1.0);
    CheckPointInPolygon(vertex_list, origin, expected_center, "Triangle");

    origin = gstVertex(1.0, 1.0);
    expected_center = gstVertex(2, 1.0);
    CheckPointInPolygon(vertex_list, origin, expected_center, "Triangle2");

    origin = gstVertex(2.0, 1.5);
    expected_center = gstVertex(2, 1.0);
    CheckPointInPolygon(vertex_list, origin, expected_center, "Triangle3");

    origin = gstVertex(3.0, 1.5);
    expected_center = gstVertex(2, 1.5);
    CheckPointInPolygon(vertex_list, origin, expected_center, "Triangle4");
  }
}  // end TestFindPointInPolygonRelativeToPoint

// Here we test gstPolygonUtils::FindPointInPolygonRelativeToPoint with a few
// degenerate polygons.
TEST_F(gstPolygonUtilsTest, TestDegeneracies) {
  gstVertex origin(0, 0);
  // Note: on degenerate inputs, FindPointInPolygonRelativeToPoint returns
  // the origin.
  gstVertex expected_center = origin;

  { // Triangle: try FindPointInPolygonRelativeToPoint on a triangle with
    // various origin
    // parameters.
    std::vector<gstVertex> vertex_list;
    // Empty vertex_list
    CheckPointInPolygon(vertex_list, origin, expected_center, "empty");

    // single vertex
    vertex_list.push_back(gstVertex(1.0, 1.0));
    CheckPointInPolygon(vertex_list, origin, expected_center, "one vertex");

    // Unfinished polygon
    vertex_list.push_back(gstVertex(5.0, 1.0));
    vertex_list.push_back(gstVertex(3.0, 3.0));
    CheckPointInPolygon(vertex_list, origin, expected_center, "3 vertices");

    // Origin not overlapping.
    vertex_list.push_back(vertex_list[0]);
    origin = gstVertex(20, 20);
    expected_center = origin;
    CheckPointInPolygon(vertex_list, origin, expected_center, "bad origin");

    // Origin overlapping 1 point
    vertex_list.push_back(vertex_list[0]);
    origin = gstVertex(5, 20);
    expected_center = origin;
    CheckPointInPolygon(vertex_list, origin, expected_center, "bad origin");
  }
}  // end TestDegeneracies

// Utility method for validating crossing values given a polygon and y-intercept
// compared to expected results.
// y-intercept: the y-value defining a horizontal line used to intersect the
//              polygon
// vertex_list: defining the polygon
// expected_crossings: the expected crossing values
// expected_center_of_largest_overlap: expected center of the largest overlap
//                                     of the line with the polygon
// expected_length_of_largest_overlap: expected length of the largest overlap
//                                     of the line with the polygon
// origin: the origin used for the FindPointInPolygonRelativeToPoint test
// expected_center: expected center result of FindPointInPolygonRelativeToPoint
//                  given the vertex_list polygon and origin point
// message: the additional error message for failures to help identify the
//          failed test.
void CheckCrossings(
  double y_intercept,
  const std::vector<gstVertex>& vertex_list,
  const std::vector<gstPolygonUtils::PolygonCrossing>& expected_crossings,
  double expected_center_of_largest_overlap,
  double expected_length_of_largest_overlap,
  const gstVertex& origin,
  const gstVertex& expected_center,
  const std::string& message) {
  { // Test the lower level routines first.
    // Test ComputeHorizontalCrossings
    std::vector<gstPolygonUtils::PolygonCrossing> crossings;
    gstPolygonUtilsTest::ComputeHorizontalCrossings(vertex_list,
                                                          y_intercept,
                                                          &crossings);
    ASSERT_EQ(expected_crossings.size(), crossings.size()) << message;

    for (unsigned int i = 0; i < crossings.size(); ++i) {
      ASSERT_EQ(expected_crossings[i], crossings[i]) << message;
    }

    // Check that the overlap stats are correct from LargestOverlapStats.
    double center = 0.0;
    double length = 0.0;
    gstPolygonUtilsTest::LargestOverlapStats(crossings, &center, &length);
    ASSERT_EQ(expected_center_of_largest_overlap, center) << message;
    ASSERT_EQ(expected_length_of_largest_overlap, length) << message;

    // Finally, check the main call that we will use.
    gstPolygonUtils::HorizontalOverlap(vertex_list, y_intercept,
                                             &center, &length);
    ASSERT_EQ(expected_center_of_largest_overlap, center) << message;
    ASSERT_EQ(expected_length_of_largest_overlap, length) << message;

    // Test VerticalOverlap using transposed vertices.
    // transpose the coordinates
    std::vector<gstVertex> vertex_list_transposed(vertex_list.size());
    for (unsigned int i = 0; i < vertex_list.size(); ++i) {
      const gstVertex& vertex = vertex_list[i];
      vertex_list_transposed[i] = gstVertex(vertex.y, vertex.x);
    }
    // Finally, check the main call that we will use.
    gstPolygonUtils::VerticalOverlap(vertex_list_transposed, y_intercept,
                                             &center, &length);
    // Need to indicate the vertical test with a different message.
    std::string message_vertical = message + ": Vertical";
    ASSERT_EQ(expected_center_of_largest_overlap, center) << message_vertical;
    ASSERT_EQ(expected_length_of_largest_overlap, length) << message_vertical;
  }

  CheckPointInPolygon(vertex_list, origin, expected_center, message);
}

// Test the following gstPolygonUtils methods
// (including protected methods):
// ComputeHorizontalCrossings (does all the work to identify crossings)
// LargestOverlapStats (processes the results of ComputeHorizontalCrossings)
// HorizontalOverlap
// VerticalOverlap
// FindPointInPolygonRelativeToPoint
// We spend some time here to test several of the special cases including:
// normal crossings, peak and trough vertex intersections, collinear edges.
TEST_F(gstPolygonUtilsTest, TestComputeCrossings) {
  double y_intercept = 1.0;
  double expected_center_of_largest_overlap = 0.0;
  double expected_length_of_largest_overlap = 0.0;
  gstVertex expected_center;
  gstVertex origin;
  // Walk through each crossing test:
  { // Triangle
    std::vector<gstVertex> vertex_list;
    vertex_list.push_back(gstVertex(0.0, 0.0));
    vertex_list.push_back(gstVertex(4.0, 0.0));
    vertex_list.push_back(gstVertex(2.0, 2.0));
    vertex_list.push_back(vertex_list[0]);
    std::vector<gstPolygonUtils::PolygonCrossing> expected_crossings;
    expected_crossings.push_back(
      gstPolygonUtils::PolygonCrossing(1.0,
        gstPolygonUtils::kCrossing));
    expected_crossings.push_back(
      gstPolygonUtils::PolygonCrossing(3.0,
        gstPolygonUtils::kCrossing));

    expected_center_of_largest_overlap = 2;
    expected_length_of_largest_overlap = 2.0;
    origin = gstVertex(expected_center_of_largest_overlap, y_intercept);
    expected_center = gstVertex(2, y_intercept);
    CheckCrossings(y_intercept, vertex_list, expected_crossings,
                   expected_center_of_largest_overlap,
                   expected_length_of_largest_overlap,
                   origin, expected_center, "Triangle");
  }

  { // Polygon with vertex on the intercept
    std::vector<gstVertex> vertex_list;
    vertex_list.push_back(gstVertex(0.0, 0.0));
    vertex_list.push_back(gstVertex(4.0, 0.0));
    vertex_list.push_back(gstVertex(2.0, 2.0));
    vertex_list.push_back(gstVertex(0.5, 1.0));
    vertex_list.push_back(vertex_list[0]);
    std::vector<gstPolygonUtils::PolygonCrossing> expected_crossings;
    expected_crossings.push_back(
      gstPolygonUtils::PolygonCrossing(0.5,
        gstPolygonUtils::kCrossing));
    expected_crossings.push_back(
      gstPolygonUtils::PolygonCrossing(3.0,
        gstPolygonUtils::kCrossing));

    expected_center_of_largest_overlap = 1.75;  // between 0.5 and 3
    expected_length_of_largest_overlap = 2.5;
    origin = gstVertex(3, y_intercept);
    expected_center = gstVertex(expected_center_of_largest_overlap,
                                y_intercept);
    CheckCrossings(y_intercept, vertex_list, expected_crossings,
                   expected_center_of_largest_overlap,
                   expected_length_of_largest_overlap,
                   origin, expected_center, "Vertex on Intercept");
  }
  { // Sawtooth : we'll create a sawtooth with three teeth.
    std::vector<gstVertex> vertex_list;
    unsigned int tooth_count = 3;
    double tooth_width = 4.0;
    vertex_list.push_back(gstVertex(0.0, 0.0));
    double x_coordinate = tooth_count * tooth_width;
    vertex_list.push_back(gstVertex(tooth_count * tooth_width, 0.0));
    for (unsigned int i = 0; i < tooth_count; ++i) {
      x_coordinate -= tooth_width/2.0;
      vertex_list.push_back(gstVertex(x_coordinate, 2.0));  // peak
      x_coordinate -= tooth_width/2.0;
      vertex_list.push_back(gstVertex(x_coordinate, 0.0));  // trough
    }
    vertex_list.push_back(vertex_list[0]);

    std::vector<gstPolygonUtils::PolygonCrossing> expected_crossings;
    for (unsigned int i = 0; i < tooth_count; ++i) {
      double crossing_coordinate = i * tooth_width + tooth_width/4.0;
      expected_crossings.push_back(
         gstPolygonUtils::PolygonCrossing(crossing_coordinate,
                                                gstPolygonUtils::kCrossing));
      crossing_coordinate = i * tooth_width + 3 * tooth_width/4.0;
      expected_crossings.push_back(
         gstPolygonUtils::PolygonCrossing(crossing_coordinate,
                                                gstPolygonUtils::kCrossing));
    }
    expected_center_of_largest_overlap = 2.0;  // between 0 and tooth_width = 4
    expected_length_of_largest_overlap = 2.0;
    origin = gstVertex(2, y_intercept);
    expected_center = gstVertex(expected_center_of_largest_overlap,
                                y_intercept);
    { // Check the basic sawtooth
      CheckCrossings(y_intercept, vertex_list, expected_crossings,
                     expected_center_of_largest_overlap,
                     expected_length_of_largest_overlap,
                     origin, expected_center, "Sawtooth");
    }
    { // Check Sawtooth with the middle tooth that peaks at the y_intercept
      vertex_list[4].y = y_intercept;
      double crossing_coordinate = tooth_count * tooth_width / 2.0;
      std::vector<gstPolygonUtils::PolygonCrossing> expected_crossings2 =
        expected_crossings;
      expected_crossings2[2] =
        gstPolygonUtils::PolygonCrossing(crossing_coordinate,
          gstPolygonUtils::kTouchingButNotCrossing);
      expected_crossings2.erase(expected_crossings2.begin() + 3);

      CheckCrossings(y_intercept, vertex_list, expected_crossings2,
                     expected_center_of_largest_overlap,
                     expected_length_of_largest_overlap,
                     origin, expected_center, "Sawtooth intersect peak");
    }
    { // Check Sawtooth with 3rd trough intersecting the y_intercept
      vertex_list[4].y = 2.0;  // put the middle peak back to 2
      vertex_list[3].y = y_intercept;  // put the 3rd trough at the y_intercept
      double crossing_coordinate = 2 * tooth_width;
      std::vector<gstPolygonUtils::PolygonCrossing> expected_crossings2 =
        expected_crossings;
      expected_crossings2[3] =
        gstPolygonUtils::PolygonCrossing(crossing_coordinate,
          gstPolygonUtils::kTouchingButNotCrossing);
      expected_crossings2.erase((expected_crossings2.begin() + 4));

      expected_center_of_largest_overlap = 6.5;  // between 5 and 8
      expected_length_of_largest_overlap = 3.0;
      origin = gstVertex(2, y_intercept);
      expected_center = gstVertex(2, 1);
      CheckCrossings(y_intercept, vertex_list, expected_crossings2,
                     expected_center_of_largest_overlap,
                     expected_length_of_largest_overlap,
                     origin, expected_center, "Sawtooth intersect trough");
    }
  }  // end Sawtooth tests



  { // Stairsteps: Check collinear edge from above and below.
    y_intercept = 2.0;
    std::vector<gstVertex> vertex_list;
    vertex_list.push_back(gstVertex(0.0, 0.0));
    vertex_list.push_back(gstVertex(5.0, 0.0));
    vertex_list.push_back(gstVertex(5.0, 3.0));
    vertex_list.push_back(gstVertex(4.0, 3.0));
    vertex_list.push_back(gstVertex(4.0, y_intercept));
    vertex_list.push_back(gstVertex(3.0, y_intercept));
    vertex_list.push_back(gstVertex(3.0, 1.0));
    vertex_list.push_back(gstVertex(2.0, 1.0));
    vertex_list.push_back(gstVertex(2.0, y_intercept));
    vertex_list.push_back(gstVertex(1.0, y_intercept));
    vertex_list.push_back(gstVertex(1.0, 3.0));
    vertex_list.push_back(gstVertex(0.0, 3.0));
    vertex_list.push_back(vertex_list[0]);

    std::vector<gstPolygonUtils::PolygonCrossing> expected_crossings;
    expected_crossings.push_back(
       gstPolygonUtils::PolygonCrossing(0.0,
                                        gstPolygonUtils::kCrossing));
    expected_crossings.push_back(
       gstPolygonUtils::PolygonCrossing(1.0,
                                        gstPolygonUtils::kCollinearFromAbove));
    expected_crossings.push_back(
       gstPolygonUtils::PolygonCrossing(2.0,
                                        gstPolygonUtils::kCollinearFromBelow));
    expected_crossings.push_back(
       gstPolygonUtils::PolygonCrossing(3.0,
                                        gstPolygonUtils::kCollinearFromBelow));
    expected_crossings.push_back(
       gstPolygonUtils::PolygonCrossing(4.0,
                                        gstPolygonUtils::kCollinearFromAbove));
    expected_crossings.push_back(
       gstPolygonUtils::PolygonCrossing(5.0, gstPolygonUtils::kCrossing));
    expected_center_of_largest_overlap = 0.5;  // between 0 and 2
    expected_length_of_largest_overlap = 1.0;
    origin = gstVertex(0.5, y_intercept);
    expected_center = gstVertex(0.5, 1.5);
    // Check the basic stairstep
    CheckCrossings(y_intercept, vertex_list, expected_crossings,
                   expected_center_of_largest_overlap,
                   expected_length_of_largest_overlap,
                   origin, expected_center, "Stairsteps");

    // insert vertices on both of the collinear edges (should be ignored).
    vertex_list.insert(vertex_list.begin() + 5, gstVertex(3.5, y_intercept));
    vertex_list.insert(vertex_list.begin() + 10, gstVertex(1.5, y_intercept));
    CheckCrossings(y_intercept, vertex_list, expected_crossings,
                   expected_center_of_largest_overlap,
                   expected_length_of_largest_overlap,
                   origin, expected_center,
                   "Stairsteps collinear");
  }  // end Stairstep tests
}  // end TestComputeCrossings


// Utility to test IsConvex for the polygon specified by vertex_list
// This will construct the geode from the vertex list.
// It will construct the geode using each vertex as the starting vertex
// (to ensure that the result is not affected by start vertex.
// expected_convexity_result: the expected result of each test
// vertex_list : list of 2d vertices forming the polygon (note: the last vertex
//               is not to be the same as the first, we will add that to the
//               geode we construct)
// message: string to output with failed test.
void CheckPolygonConvexityBasic(bool expected_convexity_result,
                                std::vector<gstVertex>& vertex_list,
                                const std::string& message) {
  // Run the test with a different starting index for the geode vertices.
  for (unsigned int start_index = 0; start_index < vertex_list.size(); ++start_index) {
    std::vector<gstVertex> vertex_list_rotated;
    // Add vertices starting with the current start_index.
    for (unsigned int i = start_index; i < vertex_list.size(); ++i) {
      vertex_list_rotated.push_back(vertex_list[i]);
    }
    // Continue from the beginning.
    // If append_first_vertex_to_end, then we repeat the start index'th vertex.
    for (unsigned int i = 0; i <= start_index; ++i) {
      vertex_list_rotated.push_back(vertex_list[i]);
    }

    EXPECT_EQ(expected_convexity_result,
              gstPolygonUtils::IsConvex(vertex_list_rotated)) << message;
  }
}

// Utility to test IsConvex for the polygon specified by vertex_list
// This will call CheckPolygonConvexityBasic twice to test the convexity of
// the polygon vertex list in both forward and reverse order.
// expected_convexity_result: the expected result of each test
// vertex_list : list of 2d vertices forming the polygon (note: the last vertex
//               is not to be the same as the first, we will add that to the
//               geode we construct)
// message: string to output with failed test.
void CheckPolygonConvexity(bool expected_convexity_result,
                           std::vector<gstVertex>& vertex_list,
                           const std::string& message) {
  CheckPolygonConvexityBasic(expected_convexity_result, vertex_list, message);

  // Reverse the list and try it again...still better give the same result.
  std::string reverse_message = message + ": Reversed List";
  std::vector<gstVertex> reverse_vertex_list = vertex_list;
  reverse(reverse_vertex_list.begin(), reverse_vertex_list.end());
  CheckPolygonConvexityBasic(expected_convexity_result,
                            reverse_vertex_list, reverse_message);
}

TEST_F(gstPolygonUtilsTest, TestIsConvex) {
  // Test 1: a triangle.
  {
    // We have a simple list of 3 vertices defining a triangle.
    // Need to make sure it's clockwise.
    std::vector<gstVertex> vertex_list;
    vertex_list.push_back(gstVertex(0.0, 0.0));
    vertex_list.push_back(gstVertex(1.0, 0.0));
    vertex_list.push_back(gstVertex(1.0, 1.0));
    CheckPolygonConvexity(true, vertex_list, "Triangle Test");
  }

  // Test 2: a pentagon with: convex, concave and collinear tops.
  {
    // We have a simple list of 5 vertices defining a pentagon.
    std::vector<gstVertex> vertex_list;
    vertex_list.push_back(gstVertex(0.0, 0.0));
    vertex_list.push_back(gstVertex(2.0, 0.0));
    vertex_list.push_back(gstVertex(2.0, 2.0));
    vertex_list.push_back(gstVertex(1, 3.0));  // Point 3 is the top point.
    vertex_list.push_back(gstVertex(0.0, 2.0));
    // A) test convex.
    CheckPolygonConvexity(true, vertex_list, "Convex Pentagon");
    // B) test concave.
    vertex_list[3].y = 1.0;  // Move the top point down to form the concavity.
    CheckPolygonConvexity(false, vertex_list, "Concave Pentagon");
    // C) test collinear.
    vertex_list[3].y = 2.0;  // Move the top point to make the top flat.
    CheckPolygonConvexity(true, vertex_list, "Colinear Pentagon");
  }

  // Test 3: pentagon with repeated points, convex, concave and collinear tops.
  // Repeated points is degenerate condition but would not want it to break
  // our test.
  {
    // We have a simple list of 6 vertices defining a pentagon with the
    // upper right corner repeated.
    std::vector<gstVertex> vertex_list;
    vertex_list.push_back(gstVertex(0.0, 0.0));
    vertex_list.push_back(gstVertex(2.0, 0.0));
    vertex_list.push_back(gstVertex(2.0, 2.0));
    vertex_list.push_back(gstVertex(2.0, 2.0));
    vertex_list.push_back(gstVertex(1, 3.0));  // Point 3 is the top point.
    vertex_list.push_back(gstVertex(0.0, 2.0));
    // A) test convex.
    CheckPolygonConvexity(true, vertex_list,
                          "Convex Pentagon: repeated points");
    // B) test concave.
    vertex_list[4].y = 1.0;  // Move the top point down to form the concavity.
    CheckPolygonConvexity(false, vertex_list,
                          "Concave Pentagon: repeated points");
    // C) test collinear.
    vertex_list[4].y = 2.0;  // Move the top point to make the top flat.
    CheckPolygonConvexity(true, vertex_list,
                          "Colinear Pentagon: repeated points");
    // D) test concave where the repeated point is at the concavity.
    vertex_list[4].y = 1.0;  // Move the top point down to form the concavity.
    vertex_list[3] = vertex_list[4];  // copy the point to position 4.
    CheckPolygonConvexity(false, vertex_list,
                          "Concave Pentagon: repeated points");
  }
}  // end TestIsConvex

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

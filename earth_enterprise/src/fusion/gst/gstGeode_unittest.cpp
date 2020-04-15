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

#include <cstdio>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "fusion/gst/gstBBox.h"
#include "fusion/gst/gstGeode.h"

// gtest needs to be able to print out classes we're comparing
std::ostream &operator<<(std::ostream &strm, const gstBBox &v) {
  strm << "[(" << v.minX() << ", " << v.minY() << "), ("
    << v.maxX() << ", " << v.maxY() << ")]";
  return strm;
}
std::ostream &operator<<(std::ostream &strm, const gstVertex &v) {
  strm << "(" << v.x << ", " << v.y << ")";
  return strm;
}

// Our test class, provides some access to protected methods of gstGeode.
class gstGeodeTest : public testing::Test {
 public:
  // Wrapper to test protected method: gstGeodeImpl::IsConvex().
  // geode is the handle to the geode
  // part_index: is the part of the geode to be tested.
  static bool gstGeode_IsConvex(const gstGeodeHandle& geode, unsigned int part_index) {
    return static_cast<gstGeode*>(&(*geode))->IsConvex(part_index);
  }

  // Wrapper to test protected method: gstGeodeImpl::IsConvex().
  // geode is the handle to the geode
  static gstVertex gstGeode_Center(const gstGeodeHandle& geode) {
    return geode->Center();
  }
};

// Utility to create a Geode with a polygon specified by the vertex_list and
// a start_index (allows caller to rotate the order of the vertices easily).
// vertex_list: list of vertices defining the polygon (the last vertex need
//              not be the same as the first), we will ensure that here.
// start_index: use the "start_index"th point as the first point in the polygon.
// append_first_vertex_to_end: copy the first vertex to the end (to tie the
//                             polygon into a loop if necessary).
gstGeodeHandle CreateGeode(const std::vector<gstVertex>& vertex_list,
                           unsigned int start_index,
                           bool append_first_vertex_to_end) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  if (vertex_list.empty())
    return geodeh;

  // Add vertices starting with the current start_index.
  for (unsigned int i = start_index; i < vertex_list.size(); ++i) {
    geode->AddVertex(vertex_list[i]);
  }
  // Continue from the beginning.
  // If append_first_vertex_to_end, then we repeat the start index'th vertex.
  int start = start_index;
  if (!append_first_vertex_to_end)
    start--;  // No need to duplicate the start_index.
  for (int i = 0; i <= start; ++i) {
    geode->AddVertex(vertex_list[i]);
  }
  return geodeh;
}

// Simple struct to wrap up gstGeode::Centroid() result data.
class CentroidResult {
 public:
  CentroidResult() : x_(0), y_(0), area_(0), result_(0) {}
  // Overload of operator== needed for test code.
  bool operator==(const CentroidResult& other) const {
    return (x_ == other.x_) && (y_ == other.y_) && (area_ == other.area_) &&
    (result_ == other.result_);
  }
  double x_;
  double y_;
  double area_;
  int result_;
};
// Need to be able to output the CentroidResult class for test routines.
std::ostream &operator<<(std::ostream &strm, const CentroidResult &v) {
  strm << "(" << v.x_ << ", " << v.y_ << ", " << v.area_ << ", " << v.result_
    << ")";
  return strm;
}
// Handy wrapper for the gstGeode Centroid() call.
CentroidResult GetCentroid(const gstGeodeHandle& geode) {
  CentroidResult result;
  result.result_ = geode->Centroid(0, &result.x_, &result.y_, &result.area_);
  return result;
}

// Utility to check the gstGeode cache behavior with Center() and Centroid().
// to avoid cache effects.
void CheckCenterAndCentroid(const gstGeodeHandle& geodeh,
                            const std::string& message) {
  if (geodeh->IsEmpty()) {
    fprintf(stderr, "%s: Input geode is empty.\n", __func__);
    return;
  }

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  // Create the fresh geode using the first polygon (assuming only 1).
  // Copy the vertex list out to create the new geode.
  std::vector<gstVertex> vertex_list;
  for (unsigned int i = 0; i < geode->VertexCount(0); ++i) {
    vertex_list.push_back(geode->GetVertex(0, i));
  }
  gstGeodeHandle fresh_geode = CreateGeode(vertex_list, 0,
                                           false /* don't copy first vertex */);

  // Note: the redundant calls on a fresh and not-fresh geode are to
  // ensure that the cache of Center() results is behaving properly.
  // Get the centroid and center from the original geode (cache may be set).
  CentroidResult centroid_result = GetCentroid(geodeh);
  gstVertex center = geode->Center();

  // compute the expected centroid and center using a fresh geode
  // eliminating the cache effect.
  CentroidResult expected_centroid_result = GetCentroid(fresh_geode);
  gstVertex expected_center = fresh_geode->Center();

  // Check that cached and uncached values are equal.
  EXPECT_EQ(centroid_result, expected_centroid_result) << message;
  EXPECT_EQ(center, expected_center) << message;
}

// Utility to validate the caching behavior of gstGeode.
// gstGeode caches BoundingBox, Centroid and Center.
// gstGeode will recompute an invalid bbox when requesting BoundingBox().
// We want to check
// 1) that the internal box is invalid or accurate
// 2) that the recomputed box is accurate
// 3) that the returned Center is always fresh (not a stale cache)
// On error the specified message will be included to help identify the failure.
void CheckGeodeData(const gstGeodeHandle& geode,
                    const gstBBox& internal_box,
                    gstBBox& expected_box,
                    const std::string& message) {
  if (internal_box.Valid()) {
    EXPECT_EQ(internal_box, expected_box) << message;
  }
  gstBBox box = geode->BoundingBox();
  EXPECT_EQ(box, expected_box) << message;

  CheckCenterAndCentroid(geode, message);
}

// Test gstGeode's Add,Delete,Insert and Modify Vertex methods and their
// maintenance of the internal bounding box of the geode.
TEST_F(gstGeodeTest, TestVertexModification) {
  // We have a simple list of 4 vertices...the first 3 define a triangle
  // and the 4th vertex lies within the triangle.
  std::vector<gstVertex> vertex_list;
  vertex_list.push_back(gstVertex(0.0, 0.0));
  vertex_list.push_back(gstVertex(1.0, 0.0));
  vertex_list.push_back(gstVertex(1.0, 1.0));
  vertex_list.push_back(gstVertex(0.7, 0.2));
  gstVertex& v0 = vertex_list[0];
  gstVertex& v1 = vertex_list[1];
  gstVertex& v2 = vertex_list[2];
  gstVertex& v3 = vertex_list[3];
  int part = 0;  // For this test we're simply concentrating on a single part
                 // geode.
  // Need to validate that the bounding boxes are appropriately updated
  // by the corresponding operation.

  // On creation the bbox should be invalid.
  {
    gstGeodeHandle geode = gstGeodeImpl::Create(gstPolygon);
    const gstBBox& box = geode->BoundingBox();
    gstBBox expected_box;
    CheckGeodeData(geode, box, expected_box, "Creation of bbox");
  }

  // AddVertex should always grow the box.
  {
    gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
    gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

    const gstBBox& box = geode->BoundingBox();
    gstBBox expected_box;
    for (unsigned int i = 0; i < vertex_list.size(); ++i) {
      geode->AddVertex(vertex_list[i]);
      expected_box.Grow(vertex_list[i]);
      CheckGeodeData(geodeh, box, expected_box, "AddVertex");
    }
  }

  // AddVertexToPart0 should always grow the box.
  {
    gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
    gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

    const gstBBox& box = geode->BoundingBox();
    gstBBox expected_box;
    for (unsigned int i = 0; i < vertex_list.size(); ++i) {
      geode->AddVertexToPart0(vertex_list[i]);
      expected_box.Grow(vertex_list[i]);
      CheckGeodeData(geodeh, box, expected_box, "AddVertexToPart0");
    }
  }
  // InsertVertex should always grow the box.
  {
    gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
    gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

    const gstBBox& box = geode->BoundingBox();
    gstBBox expected_box;
    geode->AddVertex(v0);
    expected_box.Grow(v0);
    geode->AddVertex(v1);
    expected_box.Grow(v1);

    geode->InsertVertex(part, 1, v2);
    expected_box.Grow(v2);
    CheckGeodeData(geodeh, box, expected_box, "InsertVertex v2");

    geode->InsertVertex(part, 1, v3);
    expected_box.Grow(v3);
    CheckGeodeData(geodeh, box, expected_box, "InsertVertex v3");
  }
  // DeleteVertex should always invalidate the box or make the box correct.
  {
    gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
    gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

    const gstBBox& box = geode->BoundingBox();
    for (unsigned int i = 0; i < vertex_list.size(); ++i) {
      geode->AddVertex(vertex_list[i]);
    }
    geode->DeleteVertex(part, 2);
    if (box.Valid()) {
      gstBBox expected_box;
      expected_box.Grow(v0);
      expected_box.Grow(v1);
      expected_box.Grow(v3);
      CheckGeodeData(geodeh, box, expected_box, "DeleteVertex");
    }

    // Adding it back should keep it invalid or correct.
    {
      geode->AddVertex(v2);
      if (box.Valid()) {
        gstBBox expected_box;
        expected_box.Grow(v0);
        expected_box.Grow(v1);
        expected_box.Grow(v2);
        expected_box.Grow(v3);
        CheckGeodeData(geodeh, box, expected_box, "AddVertex after Delete");
      }
    }
  }

  // ModifyVertex should always invalidate the box or make the box correct.
  {
    // Test 1: modify == collapsing the bbox
    gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
    gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

    const gstBBox& box = geode->BoundingBox();
    for (unsigned int i = 0; i < vertex_list.size(); ++i) {
      geode->AddVertex(vertex_list[i]);
    }
    geode->ModifyVertex(part, 2, v1);
    if (box.Valid()) {
      gstBBox expected_box;
      expected_box.Grow(v0);
      expected_box.Grow(v1);
      expected_box.Grow(v3);
      CheckGeodeData(geodeh, box, expected_box, "ModifyVertex contracting box");
    }
  }
  {
    // Test 2: modify == keeping bbox the same
    gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
    gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

    const gstBBox& box = geode->BoundingBox();
    for (unsigned int i = 0; i < vertex_list.size(); ++i) {
      geode->AddVertex(vertex_list[i]);
    }
    geode->ModifyVertex(part, 3, gstVertex(0.6, 0.4));
    if (box.Valid()) {
      gstBBox expected_box;
      expected_box.Grow(v0);
      expected_box.Grow(v1);
      expected_box.Grow(v3);
      CheckGeodeData(geodeh, box, expected_box, "ModifyVertex same box");
    }
  }
  {
    // Test 3: modify == expanding the bbox
    gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
    gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

    for (unsigned int i = 0; i < vertex_list.size(); ++i) {
      geode->AddVertex(vertex_list[i]);
    }
    gstVertex vnew(-2, -2);  // Expand in the opposite direction so v3 is not
                             // contained in the resulting box.
    const gstBBox& box = geode->BoundingBox();
    geode->ModifyVertex(part, 2, vnew);
    if (box.Valid()) {
      gstBBox expected_box;
      expected_box.Grow(v0);
      expected_box.Grow(v1);
      expected_box.Grow(v3);
      expected_box.Grow(vnew);
      CheckGeodeData(geodeh, box, expected_box, "ModifyVertex expanding box");
    }
  }
}  // end TestVertexModification

// Utility to test convexity of a geode (testing all combinations of
// start indices for the given polygon when used to create the geode).
// Note that the "Basic" version isn't that basic.
// It runs a test for each possible starting vertex index in the polygon.
// It will construct N geodes (for a polygon of N vertices).
// Each geode is constructed using a different starting vertex
// to ensure that the result is not affected by start vertex.
// expected_convexity_result: the expected result of each test
// vertex_list : list of 2d vertices forming the polygon (note: the last vertex
//               is not to be the same as the first, we will add that to the
//               geode we construct)
// message: string to output with failed test.
void CheckGeodeConvexityBasic(bool expected_convexity_result,
                               std::vector<gstVertex>& vertex_list,
                               const std::string& message) {
  // Run the test with a different starting index for the geode vertices.
  for (unsigned int start_index = 0; start_index < vertex_list.size(); ++start_index) {
    gstGeodeHandle geode = CreateGeode(vertex_list, start_index, true);
    EXPECT_EQ(expected_convexity_result,
              gstGeodeTest::gstGeode_IsConvex(geode, 0)) << message;
  }
}

// Utility to test convexity of a geode (testing both forward and reverse
// order of polygon vertices used to create the geode).
// This is the main call for our Test routines to use.
// This will call CheckGeodeConvexityBasic twice to test the convexity of
// the polygon vertex list in both forward and reverse order.
// expected_convexity_result: the expected result of each test
// vertex_list : list of 2d vertices forming the polygon (note: the last vertex
//               is not to be the same as the first, we will add that to the
//               geode we construct)
// message: string to output with failed test.
void CheckGeodeConvexityBidirectional(bool expected_convexity_result,
                                      std::vector<gstVertex>& vertex_list,
                                      const std::string& message) {
  CheckGeodeConvexityBasic(expected_convexity_result, vertex_list, message);

  // Reverse the list and try it again...still better give the same result.
  std::string reverse_message = message + ": Reversed List";
  std::vector<gstVertex> reverse_vertex_list = vertex_list;
  reverse(reverse_vertex_list.begin(), reverse_vertex_list.end());
  CheckGeodeConvexityBasic(expected_convexity_result,
                            reverse_vertex_list, reverse_message);
}

TEST_F(gstGeodeTest, TestIsConvex) {
  // Test 1: a triangle.
  {
    // We have a simple list of 3 vertices defining a triangle.
    // Need to make sure it's clockwise.
    std::vector<gstVertex> vertex_list;
    vertex_list.push_back(gstVertex(0.0, 0.0));
    vertex_list.push_back(gstVertex(1.0, 0.0));
    vertex_list.push_back(gstVertex(1.0, 1.0));
    CheckGeodeConvexityBidirectional(true, vertex_list, "Triangle Test");
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
    CheckGeodeConvexityBidirectional(true, vertex_list, "Convex Pentagon");
    // B) test concave.
    vertex_list[3].y = 1.0;  // Move the top point down to form the concavity.
    CheckGeodeConvexityBidirectional(false, vertex_list, "Concave Pentagon");
    // C) test collinear.
    vertex_list[3].y = 2.0;  // Move the top point to make the top flat.
    CheckGeodeConvexityBidirectional(true, vertex_list, "Colinear Pentagon");
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
    CheckGeodeConvexityBidirectional(true, vertex_list,
                                     "Convex Pentagon: repeated points");
    // B) test concave.
    vertex_list[4].y = 1.0;  // Move the top point down to form the concavity.
    CheckGeodeConvexityBidirectional(false, vertex_list,
                                     "Concave Pentagon: repeated points");
    // C) test collinear.
    vertex_list[4].y = 2.0;  // Move the top point to make the top flat.
    CheckGeodeConvexityBidirectional(true, vertex_list,
                                     "Colinear Pentagon: repeated points");
    // D) test concave where the repeated point is at the concavity.
    vertex_list[4].y = 1.0;  // Move the top point down to form the concavity.
    vertex_list[3] = vertex_list[4];  // copy the point to position 4.
    CheckGeodeConvexityBidirectional(false, vertex_list,
                                     "Concave Pentagon: repeated points");
  }
}  // end TestIsConvex

// Utility to test Center() method of gstGeode.
// This will construct the geode from the vertex list.
// It will construct the geode using each vertex as the starting vertex
// (to ensure that the result is not affected by start vertex).
// expected_center: the expected result of each test
// vertex_list : list of 2d vertices forming the polygon (note: the last vertex
//               is not to be the same as the first, we will add that to the
//               geode we construct)
// message: string to output with failed test.
void CheckGeodeCenterBasic(const gstVertex& expected_center,
                            std::vector<gstVertex>& vertex_list,
                            const std::string& message) {
  // Run the test with a different starting index for the geode vertices.
  for (unsigned int start_index = 0; start_index < vertex_list.size(); ++start_index) {
    gstGeodeHandle geode = CreateGeode(vertex_list, start_index, true);
    EXPECT_EQ(expected_center,
              gstGeodeTest::gstGeode_Center(geode)) << message;
  }
}

// Utility to test convexity of a geode.
// This will call CheckGeodeConvexityBasic twice to test the convexity of
// the polygon vertex list in both forward and reverse order.
// expected_center: the expected result of each test
// vertex_list : list of 2d vertices forming the polygon (note: the last vertex
//               is not to be the same as the first, we will add that to the
//               geode we construct)
// message: string to output with failed test.
void CheckGeodeCenter(const gstVertex& expected_center,
                      std::vector<gstVertex>& vertex_list,
                      const std::string& message) {
  CheckGeodeCenterBasic(expected_center, vertex_list, message);

  // Reverse the list and try it again...still better give the same result.
  std::string reverse_message = message + ": Reversed List";
  std::vector<gstVertex> reverse_vertex_list = vertex_list;
  reverse(reverse_vertex_list.begin(), reverse_vertex_list.end());
  CheckGeodeCenterBasic(expected_center,
                         reverse_vertex_list, reverse_message);
}

TEST_F(gstGeodeTest, TestCenter) {
  // Test 1: a triangle == centroid
  {
    // We have a simple list of 3 vertices defining a triangle.
    std::vector<gstVertex> vertex_list;
    vertex_list.push_back(gstVertex(0.0, 0.0));
    vertex_list.push_back(gstVertex(3.0, 0.0));
    vertex_list.push_back(gstVertex(3.0, 3.0));
    gstVertex expected_center(2.0, 1.0);  // Centroid of the triangle.
    CheckGeodeCenter(expected_center, vertex_list, "Triangle");

    // Repeated vertices shouldn't change it.
    vertex_list.push_back(gstVertex(1.0, 1.0));
    vertex_list.push_back(gstVertex(1.0, 1.0));
    vertex_list.push_back(gstVertex(1.0, 1.0));
    CheckGeodeCenter(expected_center, vertex_list,
                     "Triangle:  Repeated Points");
  }

  // Test 2: a convex polygon == centroid
  {
    // We have a simple list of 5 vertices defining a convex polygon.
    std::vector<gstVertex> vertex_list;
    vertex_list.push_back(gstVertex(0.0, 0.0));
    vertex_list.push_back(gstVertex(3.0, 0.0));
    vertex_list.push_back(gstVertex(3.0, 3.0));
    vertex_list.push_back(gstVertex(2.0, 4.0));
    vertex_list.push_back(gstVertex(0.0, 3.0));
    gstGeodeHandle geode = CreateGeode(vertex_list, 0, true);
    gstVertex expected_center;  // Centroid of the polygon.
    double area;  // Ignored.
    // Check that the Centroid is the same as the Center().
    geode->Centroid(0, &expected_center.x, &expected_center.y, &area);
    CheckGeodeCenter(expected_center, vertex_list, "Convex Polygon");

    // Repeated vertices shouldn't change it.
    vertex_list.push_back(gstVertex(0.0, 3.0));
    vertex_list.push_back(gstVertex(0.0, 3.0));
    vertex_list.push_back(gstVertex(0.0, 3.0));
    // Check that the Centroid is the same as the Center().
    CheckGeodeCenter(expected_center, vertex_list,
                     "Convex Polygon:  Repeated Points");
  }
}  // end TestCenter


// Create a scaled and translated copy of the vertex.
// v_out = (scale * v_in) + translation
// vertex : the input vertex
// scale: the scale factor to be applied to the input vertex.
// x_translation: the x coordinate translation to be applied to the input
//                vertex.
// y_translation: the y coordinate translation to be applied to the input
//                vertex.
// return: the scaled then translated vertex copy
gstVertex TranslateAndScaleVertex(const gstVertex& vertex,
                                  double scale,
                                  double x_translation,
                                  double y_translation) {
  double x = scale * vertex.x + x_translation;
  double y = scale * vertex.y + y_translation;
  return gstVertex(x, y);
}
// Create a scaled and translated copy of the vertex list.
// v_out[i] = (scale * v_in[i]) + translation
// vertex_list : list of 2d vertices forming the polygon (note: the last vertex
//               is not to be the same as the first, we will add that to the
//               geode we construct)
// scale: the scale factor to be applied to the input vertices.
// x_translation: the x coordinate translation to be applied to the input
//                vertices.
// y_translation: the y coordinate translation to be applied to the input
//                vertices.
// return: the scaled then translated copy of the vertex list
std::vector<gstVertex> TranslateAndScaleVertexList(
       const std::vector<gstVertex>& vertex_list,
       double scale, double x_translation, double y_translation) {
  std::vector<gstVertex> ouput_vertex_list;
  for(unsigned int i = 0; i < vertex_list.size(); ++i) {
    ouput_vertex_list.push_back(TranslateAndScaleVertex(vertex_list[i],
                                                        scale, x_translation,
                                                        y_translation));
  }
  return ouput_vertex_list;
}

// Utility to test the arithmetic precision of the
// gstGeode::Centroid() method.
// This takes a roughly unit scale polygon as input, and scales
// and translates the polygon then tests that the centroid of the scaled
// and transformed polygon is as expected.
// This will construct the geode from the vertex list.
// It will construct the geode using each vertex as the starting vertex
// (to ensure that the result is not affected by start vertex).
// expected_centroid: the expected result of each test
// vertex_list : list of 2d vertices forming the polygon (note: the last vertex
//               is not to be the same as the first, we will add that to the
//               geode we construct)
// message: string to output with failed test.
void CheckGeodeCentroidPrecision(const gstVertex& expected_centroid,
                                 std::vector<gstVertex>& vertex_list,
                                 const std::string& message) {
  std::vector<double> scales;
  std::vector<gstVertex> translations;
  // Test Identity transform for sanity.
  scales.push_back(1.0);
  translations.push_back(gstVertex(0,0));
  // Test scale down, with no translation.
  scales.push_back(1e-6);
  translations.push_back(gstVertex(0,0));
  // Test scale down to small resolution and large translation.
  scales.push_back(1e-6);
  translations.push_back(gstVertex(100,200));

  // Run the test with a different starting index for the geode vertices.
  for (unsigned int i = 0; i < scales.size(); ++i) {
    std::vector<gstVertex> scaled_vertex_list =
      TranslateAndScaleVertexList(vertex_list,
                                  scales[i],
                                  translations[i].x,
                                  translations[i].y);
    gstGeodeHandle geode = CreateGeode(scaled_vertex_list, 0, true);
    CentroidResult result = GetCentroid(geode);
    gstVertex centroid(result.x_, result.y_);
    gstVertex expected_result = TranslateAndScaleVertex(expected_centroid,
                                                        scales[i],
                                                        translations[i].x,
                                                        translations[i].y);
    EXPECT_DOUBLE_EQ(expected_result.x, centroid.x) << message;
    EXPECT_DOUBLE_EQ(expected_result.y, centroid.y) << message;
  }
}

TEST_F(gstGeodeTest, TestCentroid) {
  // We have to test the sensitivity of the Centroid algorithm
  // to arithmetic precision. We've identified this problem the
  // hard way. Some processor's (64 bit) have worse precision than
  // other processors and data like parcels will show this problem.
  // We test by creating a polygon near the origin of small width,
  // then translate the polygon by a large amount.
  // The 2 centroids should differ by this transform.
  {
    // We have a simple list of 3 vertices defining a triangle.
    std::vector<gstVertex> vertex_list;
    vertex_list.push_back(gstVertex(0.0, 0.0));
    vertex_list.push_back(gstVertex(3.0, 0.0));
    vertex_list.push_back(gstVertex(3.0, 3.0));
    gstVertex expected_center(2.0, 1.0);  // Centroid of the triangle.
    CheckGeodeCentroidPrecision(expected_center, vertex_list, "Triangle");
  }

  // Test 2: a rectangle
  {
    // We have a simple list of 4 vertices defining a rectangle.
    std::vector<gstVertex> vertex_list;
    vertex_list.push_back(gstVertex(0.0, 0.0));
    vertex_list.push_back(gstVertex(3.0, 0.0));
    vertex_list.push_back(gstVertex(3.0, 5.0));
    vertex_list.push_back(gstVertex(0.0, 5.0));
    gstGeodeHandle geode = CreateGeode(vertex_list, 0, true);
    gstVertex expected_center(1.5, 2.5);  // Center of the rectangle.
    CheckGeodeCentroidPrecision(expected_center, vertex_list, "Convex Polygon");
  }
  // Test 3: a 5-sided convex polygon
  {
    // We have a simple list of 5 vertices defining a convex polygon.
    std::vector<gstVertex> vertex_list;
    vertex_list.push_back(gstVertex(0.0, 0.0));
    vertex_list.push_back(gstVertex(3.0, 0.0));
    vertex_list.push_back(gstVertex(3.0, 3.0));
    vertex_list.push_back(gstVertex(2.0, 4.0));
    vertex_list.push_back(gstVertex(0.0, 3.0));
    gstGeodeHandle geode = CreateGeode(vertex_list, 0, true);
    gstVertex expected_center;  // Centroid of the polygon.
    double area;  // Ignored.
    // Check that the Centroid is the same as the Center().
    geode->Centroid(0, &expected_center.x, &expected_center.y, &area);
    CheckGeodeCentroidPrecision(expected_center, vertex_list, "Convex Polygon");
  }
}  // end TestCentroid

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

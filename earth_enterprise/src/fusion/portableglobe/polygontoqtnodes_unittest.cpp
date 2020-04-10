// Copyright 2017 Google Inc.
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


// Unit Tests for PolyMask

#include <limits.h>
#include <string>
#include <utility>
#include <vector>
#include <fstream>
#include <gtest/gtest.h>
#include "common/khFileUtils.h"
#include "fusion/portableglobe/polygontoqtnodes.h"
#include "fusion/portableglobe/quadtree/qtutils.h"

namespace fusion_portableglobe {

// Test class for Grid and Polygon.
class PolygonToQtNodesTest : public testing::Test {
 public:
  static std::vector<std::pair<std::int32_t, std::int32_t> > known_polygon;
  static std::vector<std::pair<std::int32_t, std::int32_t> > degenerate_polygon;

 protected:
  Grid* grid0_;
  std::string kml_file_;
  std::string kml_string_;

  virtual void SetUp() {
    grid0_ = new Grid(-180.0, -90.0);
    kml_file_ = "fusion/testdata/polygon_roi.kml";

    std::ifstream fin(kml_file_);
    kml_string_ = 
      std::string((std::istreambuf_iterator<char>(fin)), 
                  std::istreambuf_iterator<char>());
    fin.close();

    InitKnownPolygonCoordinates();
    InitDegeneratePolygonCoordinates();
  }

  virtual void TearDown() {
    delete grid0_;
  }

  /**
   * Helper routine to set up known polygon shape that can then
   * be scaled and translated onto the globe. Vertex are as
   * follows:
   *    -  -  3  4  -  -  -  5
   *    2  9  -  -  -  8  -  -
   *    - 10 11  -  -  -  -  -
   *    1  -  -  -  -  -  -  -
   *    -  -  - 12  -  7  6  -
   *    0  -  -  -  -  -  -  -
   *
   * At this resolution the fill pattern should be:
   *    -  x  3  x  x  x  x  5
   *    2  9  x  x  x  8  x  x
   *    x 10 11  -  -  x  x  x
   *    1  x  x  x  -  x  x  -
   *    x  x  x 12  -  7  6  -
   *    0  x  -  -  -  -  -  -
   */
  void InitKnownPolygonCoordinates() {
    known_polygon.clear();
    known_polygon.push_back(std::pair<std::int32_t, std::int32_t>(0, 0));
    known_polygon.push_back(std::pair<std::int32_t, std::int32_t>(0, 2));
    known_polygon.push_back(std::pair<std::int32_t, std::int32_t>(0, 4));
    known_polygon.push_back(std::pair<std::int32_t, std::int32_t>(2, 5));
    known_polygon.push_back(std::pair<std::int32_t, std::int32_t>(3, 5));
    known_polygon.push_back(std::pair<std::int32_t, std::int32_t>(7, 5));
    known_polygon.push_back(std::pair<std::int32_t, std::int32_t>(6, 1));
    known_polygon.push_back(std::pair<std::int32_t, std::int32_t>(5, 1));
    known_polygon.push_back(std::pair<std::int32_t, std::int32_t>(5, 4));
    known_polygon.push_back(std::pair<std::int32_t, std::int32_t>(1, 4));
    known_polygon.push_back(std::pair<std::int32_t, std::int32_t>(1, 3));
    known_polygon.push_back(std::pair<std::int32_t, std::int32_t>(2, 3));
    known_polygon.push_back(std::pair<std::int32_t, std::int32_t>(3, 1));
    known_polygon.push_back(std::pair<std::int32_t, std::int32_t>(0, 0));
  }

  /**
   * Helper routine to set up a degenerate polygon with no area.
   * Vertex are as
   * follows:
   *    -  -  -  -  -  -  -  1
   *    -  -  -  -  -  -  -  -
   *    -  -  -  -  -  -  -  -
   *    -  -  -  -  -  -  -  -
   *    -  -  -  -  -  -  -  -
   *    0  -  -  -  -  -  -  -
   *
   * At this resolution the fill pattern should be:
   *    -  -  -  -  -  -  -  -
   *    -  -  -  -  1  -  -  -
   *    -  -  -  x  -  -  -  -
   *    -  -  x  -  -  -  -  -
   *    -  x  -  -  -  -  -  -
   *    0  -  -  -  -  -  -  -
   */
  void InitDegeneratePolygonCoordinates() {
    degenerate_polygon.clear();
    degenerate_polygon.push_back(std::pair<std::int32_t, std::int32_t>(0, 0));
    degenerate_polygon.push_back(std::pair<std::int32_t, std::int32_t>(4, 4));
    degenerate_polygon.push_back(std::pair<std::int32_t, std::int32_t>(0, 0));
  }

  /**
   * Helper routine to build known polygon at given level. Parameters
   * give the south west corner. Vertex coordinates are
   * centered in the qtnodes.
   */
  void InitLevelPolygon(const std::vector<std::pair<std::int32_t, std::int32_t> >& coords,
                        Polygon* polygon,
                        std::int32_t level,
                        const double west,
                        double south,
                        double offset) {
    double degrees_per_qtnode = 360.0 / static_cast<double>(1 << level);

    std::vector<Vertex>* vertices = polygon->Vertices();
    for (size_t i = 0; i < coords.size(); ++i) {
      double x = west + degrees_per_qtnode
          * static_cast<double>(coords[i].first) + offset;
      double y = south + degrees_per_qtnode
          * static_cast<double>(coords[i].second) + offset;
      vertices->push_back(Vertex(x, y));
    }
  }

  /**
   * Helper routine to build known polygon at given level. Parameters
   * give the south west corner. Vertex coordinates are
   * centered in the qtnodes.
   */
  void InitLevelMercatorPolygon(
      const std::vector<std::pair<std::int32_t, std::int32_t> >& coords,
      Polygon* polygon,
      std::int32_t level,
      const double west,
      double south,
      double offset,
      Grid& grid) {
    double degrees_per_qtnode = 360.0 / static_cast<double>(1 << level);

    std::vector<Vertex>* vertices = polygon->Vertices();
    double y0 = MercatorLatToY(south);
    double dy = 2.0 * PI / static_cast<double>(1 << level);
    for (size_t i = 0; i < coords.size(); ++i) {
      double x = west + degrees_per_qtnode
          * static_cast<double>(coords[i].first) + offset;
      double y2 = y0 + (coords[i].second + offset) * dy;
      // Since we are calculating nonlinearities and are right on the boundary,
      // add (because we use floor) a small safety fudge to produce reliable
      // results.
      double y = MercatorYToLat(y2) + 0.0000000001;
      vertices->push_back(Vertex(x, y));
    }
  }

  /**
   * Helper that tests that we get same list of nodes independent of which
   * vertex of the polygon we start with.
   */
  void TestAllStartingPoints(Polygon* polygon, int max_level) {
    Grid grid0(-180.0, -90.0);

    // Use initial ordering to create a gold standard.
    std::stringstream str0_out;
    Polygons polygons0(*polygon);
    grid0.AddPolygons(&polygons0);
    grid0.CalculateQuadtreeNodes(max_level, false, str0_out);
    std::vector<Vertex>* vertices = polygon->Vertices();

    int last_idx = static_cast<int>(vertices->size()) - 1;
    for (int i = 0; i < last_idx - 1; ++i) {
      // Rotate the vertices
      // Last vertex is always the same as the first vertex.
      (*vertices)[last_idx] = (*vertices)[last_idx - 1];
      for (int j = last_idx - 1; j > 0; --j) {
        (*vertices)[j] = (*vertices)[j - 1];
      }
      (*vertices)[0] = (*vertices)[last_idx];

      // Recalculate the quadtree nodes addresses.
      Grid grid1(-180.0, -90.0);
      std::stringstream str1_out;
      Polygons polygons1(*polygon);
      grid1.AddPolygons(&polygons1);
      grid1.CalculateQuadtreeNodes(max_level, false, str1_out);

      // Should be the same independent of vertices order.
      EXPECT_EQ(str0_out.str(), str1_out.str());
    }

    // Reverse order of vertices and test all starting
    // points again.
    int num_vertices = last_idx + 1;
    for (int i = 0; i < num_vertices / 2; ++i) {
       Vertex tmp = (*vertices)[i];
       (*vertices)[i] = (*vertices)[last_idx - i];
       (*vertices)[last_idx - i] = tmp;
    }

    // Use initial ordering to create a gold standard.
    // The need for a second gold standard is a little subtle.
    // If the slope is 0.1, we may get a y value of
    // 3.5 which is rounded to the pixel value 4. When
    // we reverse the ordering, we start from the other
    // side with a slope of -0.1, but this time when we reach
    // 3.5, it is rounded down to 3, resulting in a slightly
    // different set of quadtree nodes. The reason is that
    // since 0.1 can't be represented exactly as a binary
    // number, the first 3.5 is actually slightly greater
    // than 3.5 because 0.1 is slightly greater than 0.1,
    // which means that when we are subtracting it, 3.5
    // is actually slightly less than 3.5 and is rounded
    // down. Since neither answer is "better", it is probably
    // not worth adding more complexity to try to avoid this issue.
    Grid grid2(-180.0, -90.0);
    std::stringstream str2_out;
    Polygons polygons3(*polygon);
    grid2.AddPolygons(&polygons3);
    grid2.CalculateQuadtreeNodes(max_level, false, str2_out);

    for (int i = 0; i < last_idx - 1; ++i) {
      // Rotate the vertices
      // Last vertex is always the same as the first vertex.
      (*vertices)[last_idx] = (*vertices)[last_idx - 1];
      for (int j = last_idx - 1; j > 0; --j) {
        (*vertices)[j] = (*vertices)[j - 1];
      }
      (*vertices)[0] = (*vertices)[last_idx];

      // Recalculate the quadtree nodes addresses.
      Grid grid1(-180.0, -90.0);
      std::stringstream str1_out;
      Polygons polygons4(*polygon);
      grid1.AddPolygons(&polygons4);
      grid1.CalculateQuadtreeNodes(max_level, false, str1_out);

      // Should be the same independent of vertices order.
      EXPECT_EQ(str2_out.str(), str1_out.str());
    }
  }
};

std::vector<std::pair<std::int32_t, std::int32_t> > PolygonToQtNodesTest::known_polygon;
std::vector<std::pair<std::int32_t, std::int32_t> > PolygonToQtNodesTest::degenerate_polygon;


// Tests reading in polygon from kml file.
TEST_F(PolygonToQtNodesTest, PolygonReadFromFileTest) {
  Polygons polygons(kml_file_);
  Polygon* polygon = polygons.GetPolygon(0);
  std::vector<Vertex>* vertices = polygon->Vertices();

  // 9 vertices, last vertex should be the same as the first vertex.
  EXPECT_EQ(10, static_cast<int>(polygon->Vertices()->size()));
  EXPECT_EQ((*vertices)[0].y_,
            (*vertices)[9].y_);
  EXPECT_EQ((*vertices)[0].x_,
            (*vertices)[9].x_);

  // Test values on one vertex for reasonable precision.
  // Vertex 2: -121.6655059010093,37.96161308965004
  EXPECT_NEAR(-121.6655059, (*vertices)[2].x_, 0.000001);
  EXPECT_NEAR(37.9616131, (*vertices)[2].y_, 0.000001);
}

// Tests reading in polygon from kml string.
TEST_F(PolygonToQtNodesTest, PolygonReadFromStringTest) {
  const kml_as_string dummy;
  Polygons polygons(dummy, kml_string_);
  Polygon* polygon = polygons.GetPolygon(0);
  std::vector<Vertex>* vertices = polygon->Vertices();

  // 9 vertices, last vertex should be the same as the first vertex.
  EXPECT_EQ(10, static_cast<int>(polygon->Vertices()->size()));
  EXPECT_EQ((*vertices)[0].y_,
            (*vertices)[9].y_);
  EXPECT_EQ((*vertices)[0].x_,
            (*vertices)[9].x_);

  // Test values on one vertex for reasonable precision.
  // Vertex 2: -121.6655059010093,37.96161308965004
  EXPECT_NEAR(-121.6655059, (*vertices)[2].x_, 0.000001);
  EXPECT_NEAR(37.9616131, (*vertices)[2].y_, 0.000001);
}

// Tests bounding box area for polygon where area
// doesn't cross break for either grid.
TEST_F(PolygonToQtNodesTest, PolygonBoundingBoxTest) {
  Polygon polygon0;
  Polygon polygon1;
  std::vector<Vertex>* vertices0 = polygon0.Vertices();
  std::vector<Vertex>* vertices1 = polygon1.Vertices();

  vertices0->push_back(Vertex(0.0, -5.0));
  vertices0->push_back(Vertex(0.0, 5.0));
  vertices0->push_back(Vertex(10.0, 5.0));
  vertices0->push_back(Vertex(10.0, -5.0));
  vertices0->push_back(Vertex(0.0, -5.0));
  EXPECT_EQ(5, static_cast<int>(polygon0.Vertices()->size()));

  for (int i = 0; i < 5; ++i) {
    vertices1->push_back(Vertex(
        (*vertices0)[i].x_, (*vertices0)[i].y_));
  }

  Grid grid0(-180.0, -90.0);
  Grid grid1(0.0, -90.0);

  Polygons polygons0(polygon0);
  Polygons polygons1(polygon1);
  grid0.AddPolygons(&polygons0);
  grid1.AddPolygons(&polygons1);

  EXPECT_NEAR(100.0, grid0.BoundingBoxArea(), 0.000001);
  EXPECT_NEAR(100.0, grid1.BoundingBoxArea(), 0.000001);
}

// Tests bounding box area for polygon where area
// crosses break for the standard grid.
TEST_F(PolygonToQtNodesTest, PolygonBoundingBoxTest2) {
  Polygon polygon0;
  Polygon polygon1;
  std::vector<Vertex>* vertices0 = polygon0.Vertices();
  std::vector<Vertex>* vertices1 = polygon1.Vertices();

  vertices0->push_back(Vertex(175.0, -5.0));
  vertices0->push_back(Vertex(175.0, 5.0));
  vertices0->push_back(Vertex(-175.0, 5.0));
  vertices0->push_back(Vertex(-175.0, -5.0));
  vertices0->push_back(Vertex(175.0, -5.0));
  EXPECT_EQ(5, static_cast<int>(polygon0.Vertices()->size()));

  for (int i = 0; i < 5; ++i) {
    vertices1->push_back(Vertex(
        (*vertices0)[i].x_, (*vertices0)[i].y_));
  }

  Grid grid0(-180.0, -90.0);
  Grid grid1(0.0, -90.0);

  Polygons polygons0(polygon0);
  Polygons polygons1(polygon1);
  grid0.AddPolygons(&polygons0);
  grid1.AddPolygons(&polygons1);

  EXPECT_GT(grid0.BoundingBoxArea(), grid1.BoundingBoxArea());
  EXPECT_NEAR(100.0, grid1.BoundingBoxArea(), 0.000001);
}

// Tests multiple polygons produce same qtnodes as the
// union of the separated polygons.
TEST_F(PolygonToQtNodesTest, MultiPolygonTest) {
  Polygon polygon_a;
  Polygon polygon_b;

  std::vector<Vertex>* vertices_a = polygon_a.Vertices();
  vertices_a->push_back(Vertex(-122.0, 37.0));
  vertices_a->push_back(Vertex(-121.0, 38.0));
  vertices_a->push_back(Vertex(-120.0, 35.0));
  vertices_a->push_back(Vertex(-121.0, 36.0));
  vertices_a->push_back(Vertex(-122.0, 37.0));
  EXPECT_EQ(5, static_cast<int>(polygon_a.Vertices()->size()));

  std::vector<Vertex>* vertices_b = polygon_b.Vertices();
  vertices_b->push_back(Vertex(122.0, -37.0));
  vertices_b->push_back(Vertex(121.0, -38.0));
  vertices_b->push_back(Vertex(120.0, -35.0));
  vertices_b->push_back(Vertex(121.0, -36.0));
  vertices_b->push_back(Vertex(122.0, -37.0));
  EXPECT_EQ(5, static_cast<int>(polygon_b.Vertices()->size()));

  std::string qtnodes_a;
  double area_a;
  {
    std::stringstream stream;
    Grid grid(-180.0, -90.0);
    Polygons polygons_a(polygon_a);
    grid.AddPolygons(&polygons_a);
    grid.CalculateQuadtreeNodes(12, false, stream);
    qtnodes_a = stream.str();
    area_a = grid.BoundingBoxArea();
  }

  std::string qtnodes_b;
  double area_b;
  {
    std::stringstream stream;
    Grid grid(-180.0, -90.0);
    Polygons polygons_b(polygon_b);
    grid.AddPolygons(&polygons_b);
    grid.CalculateQuadtreeNodes(12, false, stream);
    qtnodes_b = stream.str();
    area_b = grid.BoundingBoxArea();
  }

  // Two non-overlapping polygons should produce the
  // concatenation of the qtnodes when they are generated
  // separately.
  double area_ab;
  {
    std::stringstream stream;
    Grid grid(-180.0, -90.0);
    Polygons polygons_ab(polygon_a);
    polygons_ab.AddPolygon(polygon_b);
    grid.AddPolygons(&polygons_ab);
    grid.CalculateQuadtreeNodes(12, false, stream);
    std::string qtnodes_ab = stream.str();
    std::string expected_str = qtnodes_a + qtnodes_b;
    EXPECT_EQ(expected_str, qtnodes_ab);
    area_ab = grid.BoundingBoxArea();
    // Demonstrates why we do polygons one at a time
    // rather than as a set. I.e. you can just use
    // a grid that encompasses each polygon rather
    // than a grid that encompasses all of the
    // polygons, which may be massive, even if the
    // polygons themselves are small.
    EXPECT_LE(area_a + area_b, area_ab);
  }

  // Make sure order doesn't matter, other than
  // to reverse the order of the two lists of
  // qtnodes.
  double area_ba;
  {
    std::stringstream stream;
    Grid grid(-180.0, -90.0);
    Polygons polygons_ba(polygon_b);
    polygons_ba.AddPolygon(polygon_a);
    grid.AddPolygons(&polygons_ba);
    grid.CalculateQuadtreeNodes(12, false, stream);
    std::string qtnodes_ba = stream.str();
    std::string expected_str = qtnodes_b + qtnodes_a;
    EXPECT_EQ(expected_str, qtnodes_ba);
    area_ba = grid.BoundingBoxArea();
    EXPECT_EQ(area_ab, area_ba);
  }
}

// Tests no polygon.
TEST_F(PolygonToQtNodesTest, NoPolygonTest) {
  Grid grid(-180.0, -90.0);
  std::stringstream str_out;

  grid.CalculateQuadtreeNodes(18, false, str_out);

  EXPECT_EQ("", str_out.str());
}

// Tests independence of starting point on polygon. I.e.
// we should get the same quadtree nodes no matter how
// the polygon is drawn.
TEST_F(PolygonToQtNodesTest, PolygonStartingPointTestFromFile) {
  Polygons polygons(kml_file_);
  for (int level = 3; level <= 20; ++level) {
    TestAllStartingPoints(polygons.GetPolygon(0), level);
  }
}

// Same as the previous test but the KML is provided 
// as a string.
TEST_F(PolygonToQtNodesTest, PolygonStartingPointTestFromString) {
  const kml_as_string dummy;
  Polygons polygons(dummy, kml_string_);
  for (int level = 3; level <= 20; ++level) {
    TestAllStartingPoints(polygons.GetPolygon(0), level);
  }
}

// If polygon does not cross a break, then both grids
// should produce the same quadtree nodes.
TEST_F(PolygonToQtNodesTest, PolygonMultigridTestFromFile) {
  Polygons polygons0(kml_file_);
  Polygons polygons1(kml_file_);
  std::stringstream str0_out;
  std::stringstream str1_out;

  Grid grid0(-180.0, -90.0);
  Grid grid1(0.0, -90.0);
  grid0.AddPolygons(&polygons0);
  grid1.AddPolygons(&polygons1);

  EXPECT_NEAR(grid0.BoundingBoxArea(), grid1.BoundingBoxArea(), 0.000001);

  grid0.CalculateQuadtreeNodes(18, false, str0_out);
  grid1.CalculateQuadtreeNodes(18, false, str1_out);

  EXPECT_EQ(str0_out.str(), str1_out.str());
}

// Same as the previous test but the KML is provided 
// as a string.
TEST_F(PolygonToQtNodesTest, PolygonMultigridTestFromString) {
  const kml_as_string dummy;
  Polygons polygons0(dummy, kml_string_);
  Polygons polygons1(dummy, kml_string_);
  std::stringstream str0_out;
  std::stringstream str1_out;

  Grid grid0(-180.0, -90.0);
  Grid grid1(0.0, -90.0);
  grid0.AddPolygons(&polygons0);
  grid1.AddPolygons(&polygons1);

  EXPECT_NEAR(grid0.BoundingBoxArea(), grid1.BoundingBoxArea(), 0.000001);

  grid0.CalculateQuadtreeNodes(18, false, str0_out);
  grid1.CalculateQuadtreeNodes(18, false, str1_out);

  EXPECT_EQ(str0_out.str(), str1_out.str());
}

// Tests known quadtree nodes.
// Starting at SW corner of 310 quadtree node
// and using the known pattern for the polygon:
//
//   -  x  3  x    x  x  x  5
//   2  9  x  x    x  8  x  x
//
//   x 10 11  -    -  x  x  x
//   1  x  x  x    -  x  x  -
//   x  x  x 12    -  7  6  -
//   0  x  -  -    -  -  -  -
//
// We should get base of 310:
//  012 013 020 021 023 00 03
//  102 113 120 122 123 131 132
//  20 21
//  300 301 302 31
//
// Note that these nodes are independent of exactly where the
// polygon vertices land in each pixel because the edge
// drawing is done between two pixels. So no matter if
// a vertex is at the top or bottom of the pixel, it
// will be treated the same way once it is converted to
// a pixel. The decision whether a node is part of an
// edge is made therefore based on pixel line drawing
// where either x or y changes by 1 and the other
// dimension changes by less than 1.
//
// For degenerate case:
//   -  -  -  -    -  -  -  -
//   -  -  -  -    1  -  -  -
//
//   -  -  -  x    -  -  -  -
//   -  -  x  -    -  -  -  -
//   -  x  -  -    -  -  -  -
//   0  -  -  -    -  -  -  -
//
// We should get base of 310:
//  000 002 020 022 200
//
TEST_F(PolygonToQtNodesTest, KnownQuadtreeNodesTest) {
  Polygon polygon;
  int level = 6;

  const char* expected_strings[] = {
    "012", "013", "020", "021", "023", "00", "03",
    "102", "113", "120", "122", "123", "131", "132",
    "20", "21", "300", "301", "302", "31"};
  std::string expected_string;
  for (int i = 0; i < 20; ++i) {
    expected_string.append("310");
    expected_string.append(expected_strings[i]);
    expected_string.append("\n");
  }

  // Build polygon in node 310
  InitLevelPolygon(known_polygon, &polygon, level, -90.0, 0.0, 0.0);
  EXPECT_EQ(14, static_cast<int>(polygon.Vertices()->size()));

  Grid grid(-180.0, -90.0);
  Polygons polygons(polygon);
  grid.AddPolygons(&polygons);
  std::stringstream str_out;
  grid.CalculateQuadtreeNodes(level, false, str_out);

  EXPECT_EQ(expected_string, str_out.str());

  TestAllStartingPoints(&polygon, level);

  // Offset all of the nodes by just under a pixel width.
  // Answers shouldn't change.
  Polygon polygon2;
  double degrees_per_qtnode = 360.0 / static_cast<double>(1 << level);
  InitLevelPolygon(known_polygon,
                   &polygon2, level, -90.0, 0.0, degrees_per_qtnode * 0.99);
  EXPECT_EQ(14, static_cast<int>(polygon2.Vertices()->size()));

  Grid grid2(-180.0, -90.0);
  Polygons polygons2(polygon2);
  grid2.AddPolygons(&polygons2);
  std::stringstream str2_out;
  grid2.CalculateQuadtreeNodes(level, false, str2_out);

  EXPECT_EQ(expected_string, str2_out.str());

  TestAllStartingPoints(&polygon2, level);

  // Offset all of the nodes by just over a pixel width.
  // Answers should change a lot.
  Polygon polygon3;
  InitLevelPolygon(known_polygon, &polygon3, level,
                   -90.0, 0.0, degrees_per_qtnode * 1.01);
  EXPECT_EQ(14, static_cast<int>(polygon3.Vertices()->size()));

  Grid grid3(-180.0, -90.0);
  Polygons polygons3(polygon3);
  grid3.AddPolygons(&polygons3);
  std::stringstream str3_out;
  grid3.CalculateQuadtreeNodes(level, false, str3_out);

  EXPECT_NE(expected_string, str3_out.str());

  // Test the degenerate polygon case.
  const char* expected_degenerate_strings[] = {
    "000", "002", "020", "022", "200"};
  std::string expected_degenerate_string;
  for (int i = 0; i < 5; ++i) {
    expected_degenerate_string.append("310");
    expected_degenerate_string.append(expected_degenerate_strings[i]);
    expected_degenerate_string.append("\n");
  }

  Polygon polygon4;
  InitLevelPolygon(degenerate_polygon, &polygon4, level, -90.0, 0.0, 0.0);
  EXPECT_EQ(3, static_cast<int>(polygon4.Vertices()->size()));

  Grid grid4(-180.0, -90.0);
  Polygons polygons4(polygon4);
  grid4.AddPolygons(&polygons4);
  std::stringstream str4_out;
  grid4.CalculateQuadtreeNodes(level, false, str4_out);

  EXPECT_EQ(expected_degenerate_string, str4_out.str());
  TestAllStartingPoints(&polygon, level);
}

// Tests known quadtree nodes.
// Similar to the above test, but uses Mercator coordinates
// and interpolation differs.
// Starting at SW corner of 310 quadtree node
// and using the known pattern for the polygon:
//
//   -  x  3  x    x  x  x  5
//   2  9  x  x    x  8  x  x
//
//   x 10 11  -    -  x  x  x
//   1  x  x  x    -  x  x  -
//   x  x  x 12    -  7  6  -
//   0  x  -  -    -  -  -  -
//
// We should get base of 310:
//  012 013 020 021 023 00 03
//  102 113 120 122 123 131 132
//  20 21
//  300 301 302 31
TEST_F(PolygonToQtNodesTest, KnownQuadtreeNodesMercatorTest) {
  Polygon polygon;
  int level = 6;

  const char* expected_strings[] = {
    "012", "013", "020", "021", "023", "00", "03",
    "102", "113", "120", "122", "123", "131", "132",
    "20", "21", "300", "301", "302", "31"};
  std::string expected_string;
  for (int i = 0; i < 20; ++i) {
    expected_string.append("310");
    expected_string.append(expected_strings[i]);
    expected_string.append("\n");
  }

  // Build polygon in node 310
  Grid grid(-180.0, -90.0);
  InitLevelMercatorPolygon(known_polygon, &polygon, level,
                           -90.0, 0.0, 0.0, grid);
  EXPECT_EQ(14, static_cast<int>(polygon.Vertices()->size()));

  Polygons polygons(polygon);
  grid.AddPolygons(&polygons);
  std::stringstream str_out;
  grid.CalculateQuadtreeNodes(level, true, str_out);

  EXPECT_EQ(expected_string, str_out.str());

  TestAllStartingPoints(&polygon, level);
}

TEST_F(PolygonToQtNodesTest, TestPolygonInit) {
  Polygon polygon;
  int level = 6;

  const char* expected_strings[] = {
    "012", "013", "020", "021", "023", "00", "03",
    "102", "113", "120", "122", "123", "131", "132",
    "20", "21", "300", "301", "302", "31"};
  std::string expected_string;
  for (int i = 0; i < 20; ++i) {
    expected_string.append("310");
    expected_string.append(expected_strings[i]);
    expected_string.append("\n");
  }

  // Build polygon in node 310
  InitLevelPolygon(known_polygon, &polygon, level,
                   -90.0, 0.0, 0.0);
  EXPECT_EQ(14, static_cast<int>(polygon.Vertices()->size()));

  Polygons polygons(polygon);
  Polygon*  polygon2 = polygons.GetPolygon(0);
  std::vector<Vertex>* vertices = polygon.Vertices();
  std::vector<Vertex>* vertices2 = polygon2->Vertices();
  for (size_t i = 0; i < vertices->size(); ++i) {
    Vertex v = (*vertices)[i];
    Vertex v2 = (*vertices2)[i];
    EXPECT_EQ(v.x_, v2.x_);
    EXPECT_EQ(v.y_, v2.y_);
  }
}
}  // namespace fusion_portableglobe

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


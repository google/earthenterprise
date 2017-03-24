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


// Unit tests for geometry utils.
//
// 1) gstGeomUtilsClipSegmentTest - unit tests for SegmentClipper.
// Notation of quarters around clipping rectangle for coding position
// of segment end points:
//     Q7 | Q8  | Q9
//    ----|-----|----
//     Q4 | Q5  | Q6
//    ----|-----|----
//     Q1 | Q2  | Q3
//
// Q5 is clipping rectangle.
// Q1->Q3 - Segment with first end point in Q1 and second one in Q3.
// Q1Q2->Q3 - Segment with first end point on the boundary of Q1 and Q2,
// and second one in Q3.
// Q4->Q5->Q8 - Segment with end points in Q4 and Q8 that passes through Q5.

#include <gtest/gtest.h>

#include <string>

#include "fusion/gst/gstGeomUtils.h"
#include "fusion/gst/gstVertex.h"
#include "fusion/gst/gstBBox.h"
#include "fusion/gst/gstUnitTestUtils.h"

namespace fusion_gst {

// Collinear3D test class.
class gstGeomUtilsCollinear3DTest : public testing::Test {
 public:
};

void CheckCollinear3D(const gstVertex &a,
                      const gstVertex &b,
                      const gstVertex &c,
                      const bool expected_result,
                      const std::string &message) {
  bool result = Collinear3D(a, b, c);
  ASSERT_EQ(expected_result, result) << message;
}


// Collinear3D() test.
TEST_F(gstGeomUtilsCollinear3DTest, Collinear3DTest1) {
  gstVertex a;
  gstVertex b;
  gstVertex c;

  {
    a = gstVertex(0.1, 0.1, 0.1);
    b = gstVertex(0.2, 0.2, 0.2);
    c = gstVertex(0.3, 0.3, 0.3);

    CheckCollinear3D(a, b, c, true, /* collinear expected result */
                     "Collinear points test1");
  }

  {
    a = gstVertex(0.01, 0.01, 0.01);
    b = gstVertex(0.02, 0.02, 0.02);
    c = gstVertex(0.03, 0.03, 0.03);

    CheckCollinear3D(a, b, c, true, /* collinear expected result */
                     "Collinear points test2");
  }

  {
    a = gstVertex(0.001, 0.001, 0.001);
    b = gstVertex(0.002, 0.002, 0.002);
    c = gstVertex(0.003, 0.003, 0.003);

    CheckCollinear3D(a, b, c, true, /* collinear expected result */
                     "Collinear points test3");
  }

  {
    a = gstVertex(0.0001, 0.0001, 0.0001);
    b = gstVertex(0.0002, 0.0002, 0.0002);
    c = gstVertex(0.0003, 0.0003, 0.0003);

    CheckCollinear3D(a, b, c, true, /* collinear expected result */
                     "Collinear points test4");
  }

  {
    a = gstVertex(0.00001, 0.00001, 0.00001);
    b = gstVertex(0.00002, 0.00002, 0.00002);
    c = gstVertex(0.00003, 0.00003, 0.00003);

    CheckCollinear3D(a, b, c, true, /* collinear expected result */
                     "Collinear points test4");
  }

  {
    a = gstVertex(0.099, 0.1, 0.1);
    b = gstVertex(0.2, 0.2, 0.2);
    c = gstVertex(0.3, 0.3, 0.3);

    CheckCollinear3D(a, b, c, true, /* collinear expected result */
                     "Collinear points test5");
  }

  {
    a = gstVertex(0.0985, 0.1, 0.1);
    b = gstVertex(0.2, 0.2, 0.2);
    c = gstVertex(0.3, 0.3, 0.3);

    CheckCollinear3D(a, b, c, false, /* collinear expected result */
                     "Not collinear points test1");
  }

  {
    a = gstVertex(0.00000356, 0.00000767, 0.00000978);
    b = gstVertex(0.00000956, 0.00000538, 0.00000256);
    c = gstVertex(0.00000356, 0.00000678, 0.00000789);

    CheckCollinear3D(a, b, c, false, /* collinear expected result */
                     "Not collinear points test2");
  }
}

// Collinear3D() test with real data 1
TEST_F(gstGeomUtilsCollinear3DTest, Collinear3DTestReal1) {
  gstVertex a = gstVertex(0.2858158956478184520300090,
                          0.6082067522253089553174732, .0);
  gstVertex b = gstVertex(0.2858158751407541275924018,
                          0.6082067114167076793407318, .0);
  gstVertex c = gstVertex(0.2858158956478184520300090,
                          0.6082067522253089553174732, .0);

  CheckCollinear3D(a, b, c, true, /* collinear expected result */
                     "Collinear points test with real data 1");
}

// Collinear3D() test with real data 2
TEST_F(gstGeomUtilsCollinear3DTest, Collinear3DTestReal2) {
  gstVertex a = gstVertex(0.28581590613775487152992127448670,
                          0.60820677313153470588957816289621, .0);
  gstVertex b = gstVertex(0.28581589564781845203000898436585,
                          0.60820675222530895531747319182614, .0);
  gstVertex c = gstVertex(0.28581589564781845203000898436585,
                          0.60820675222530895531747319182614, .0);

  CheckCollinear3D(a, b, c, true, /* collinear expected result */
                     "Collinear points test with real data 2");
}


// Collinear3D() test with real data 3
TEST_F(gstGeomUtilsCollinear3DTest, Collinear3DTestReal3) {
  gstVertex a = gstVertex(0.28581589564781845203000898436585,
                          0.60820675222530895531747319182614, .0);
  gstVertex b = gstVertex(0.28581586046212187346426958356460,
                          0.60820668200136196457350479249726, .0);
  gstVertex c = gstVertex(0.28581587514075412759240180093911,
                          0.60820671141670767934073182914290, .0);

  CheckCollinear3D(a, b, c, true, /* collinear expected result */
                     "Collinear points test with real data 3");
}


// IntersectLines() test class.
class gstGeomUtilsIntersectLinesTest : public testing::Test {
 protected:
  void CheckIntersectLines(const gstVertex &a,
                           const gstVertex &b,
                           const gstVertex &c,
                           const gstVertex &d,
                           const gstVertex &expected_pt,
                           const std::string &message) {
    gstVertex pt = LinesIntersection2D(a, b, c, d);
    ASSERT_PRED2(TestUtils::gstVertexEqualXY, expected_pt, pt) << message;
  }
};

TEST_F(gstGeomUtilsIntersectLinesTest, IntersectLinesTest1) {
  gstVertex a = gstVertex(.1, .1, .0);
  gstVertex b = gstVertex(.9, .1, .0);
  gstVertex c = gstVertex(.3, .05, .0);
  gstVertex d = gstVertex(.3, .3, .0);
  gstVertex expected_pt = gstVertex(.3, .1, .0);
  CheckIntersectLines(a, b, c, d, expected_pt,
                      "Lines are orthogonal, first one is parallel to OX,"
                      " second one is parallel to OY.");
}

TEST_F(gstGeomUtilsIntersectLinesTest, IntersectLinesTest2) {
  gstVertex a = gstVertex(.3, .05, .0);
  gstVertex b = gstVertex(.3, .3, .0);
  gstVertex c = gstVertex(.1, .1, .0);
  gstVertex d = gstVertex(.9, .1, .0);
  gstVertex expected_pt = gstVertex(.3, .1, .0);
  CheckIntersectLines(a, b, c, d, expected_pt,
                      "Lines are orthogonal, first one is parallel to OY,"
                      " second one is parallel to OX.");
}

TEST_F(gstGeomUtilsIntersectLinesTest, IntersectLinesTest3) {
  gstVertex a = gstVertex(-.1, -.1, .5);
  gstVertex b = gstVertex(.5, .5, .5);
  gstVertex c = gstVertex(.2, .0, .5);
  gstVertex d = gstVertex(-.2, .4, .5);
  gstVertex expected_pt = gstVertex(.1, .1, .5);
  CheckIntersectLines(a, b, c, d, expected_pt,
                      "Lines are orthogonal, first one is Q3Q1 45degree,"
                      " second one is Q1Q2 45degree.");
}

TEST_F(gstGeomUtilsIntersectLinesTest, IntersectLinesTest4) {
  gstVertex a = gstVertex(-.1, -.1, .5);
  gstVertex b = gstVertex(.5, .5, .5);
  gstVertex c = gstVertex(.2, .0, .5);
  gstVertex d = gstVertex(.2, .4, .5);
  gstVertex expected_pt = gstVertex(.2, .2, .5);
  CheckIntersectLines(a, b, c, d, expected_pt,
                      "First line is Q3Q1 45degree,"
                      " second one is parallel to OY.");
}

TEST_F(gstGeomUtilsIntersectLinesTest, IntersectLinesTest5) {
  gstVertex a = gstVertex(-.1, -.1, .5);
  gstVertex b = gstVertex(.5, .5, .5);
  gstVertex c = gstVertex(.4, .3, .5);
  gstVertex d = gstVertex(-.2, .3, .5);
  gstVertex expected_pt = gstVertex(.3, .3, .5);
  CheckIntersectLines(a, b, c, d, expected_pt,
                      "First line is Q3Q1 45degree,"
                      " second one is parallel to OX.");
}

TEST_F(gstGeomUtilsIntersectLinesTest, IntersectLinesTest6) {
  gstVertex a = gstVertex(-.2, .0, .0);
  gstVertex b = gstVertex(-.2, .4, .0);
  gstVertex c = gstVertex(.1, .0, .0);
  gstVertex d = gstVertex(-.4, .5, .0);
  gstVertex expected_pt = gstVertex(-.2, .3, .0);
  CheckIntersectLines(a, b, c, d, expected_pt,
                      "First line is parallel to OY,"
                      " second one is Q1Q2 45degree.");
}

TEST_F(gstGeomUtilsIntersectLinesTest, IntersectLinesTest7) {
  gstVertex a = gstVertex(.0, -.3, .0);
  gstVertex b = gstVertex(.5, -.3, .0);
  gstVertex c = gstVertex(.1, .0, .0);
  gstVertex d = gstVertex(-.4, .5, .0);
  gstVertex expected_pt = gstVertex(.4, -.3, .0);
  CheckIntersectLines(a, b, c, d, expected_pt,
                      "First line is parallel to OX,"
                      " second one is Q1Q2 45degree.");
}

TEST_F(gstGeomUtilsIntersectLinesTest, IntersectLinesTest8) {
  gstVertex a = gstVertex(.0, -.4, .0);
  gstVertex b = gstVertex(.5, -.4, .0);
  gstVertex c = gstVertex(.1, .0, .0);
  gstVertex d = gstVertex(-.3, .8, .0);
  gstVertex expected_pt = gstVertex(.3, -.4, .0);
  CheckIntersectLines(a, b, c, d, expected_pt,
                      "First line is parallel to OX,"
                      " second one is Q1Q2 anyangle.");
}

TEST_F(gstGeomUtilsIntersectLinesTest, IntersectLinesTest9) {
  gstVertex a = gstVertex(-.1, -.4, .0);
  gstVertex b = gstVertex(-.1, -.2, .0);
  gstVertex c = gstVertex(.1, .0, .0);
  gstVertex d = gstVertex(-.3, .8, .0);
  gstVertex expected_pt = gstVertex(-.1, .4, .0);
  CheckIntersectLines(a, b, c, d, expected_pt,
                      "First line is parallel to OY,"
                      " second one is Q1Q2 anyangle.");
}

TEST_F(gstGeomUtilsIntersectLinesTest, IntersectLinesTest10) {
  gstVertex a = gstVertex(.1, .0, .0);
  gstVertex b = gstVertex(-.3, .8, .0);
  gstVertex c = gstVertex(.0, -.4, .0);
  gstVertex d = gstVertex(.5, -.4, .0);
  gstVertex expected_pt = gstVertex(.3, -.4, .0);
  CheckIntersectLines(a, b, c, d, expected_pt,
                      "First line is Q1Q2 anyangle,"
                      " second one is parallel to OX.");
}

TEST_F(gstGeomUtilsIntersectLinesTest, IntersectLinesTest11) {
  gstVertex a = gstVertex(.1, .0, .0);
  gstVertex b = gstVertex(-.3, .8, .0);
  gstVertex c = gstVertex(-.1, -.4, .0);
  gstVertex d = gstVertex(-.1, -.2, .0);
  gstVertex expected_pt = gstVertex(-.1, .4, .0);
  CheckIntersectLines(a, b, c, d, expected_pt,
                      "First line is Q1Q2 anyangle,"
                      " second one is parallel to OY.");
}

TEST_F(gstGeomUtilsIntersectLinesTest, IntersectLinesTest12) {
  gstVertex a = gstVertex(.1, .0, .0);
  gstVertex b = gstVertex(-.3, .8, .0);
  gstVertex c = gstVertex(.1, .8, .0);
  gstVertex d = gstVertex(-.3, -.0, .0);
  gstVertex expected_pt = gstVertex(-.1, .4, .0);
  CheckIntersectLines(a, b, c, d, expected_pt,
                      "Any angle intersection.");
}

// ClipSegment test class.
class gstGeomUtilsClipSegmentTest : public testing::Test {
 public:
};

#if 1
// Tests SegmentClipper.
//
// Utility method for checking the results of Segment Clipping.
// v0, v1 - input segment end points.
// wx1, wx2, wy1, wy2 - coordinates of clipping rectangle window;
// expected_intersections - the expected intersection points.
// message - the error message for failures to help identify the
// failed test.
void CheckSegmentClipping(const gstVertex &v0, const gstVertex &v1,
                          double wx1, double wx2,
                          double wy1, double wy2,
                          const gstVertexVector &expected_intersections,
                          const std::string &message) {
  gstVertexVector intersections;
  SegmentClipper segm_clipper(wx1, wx2, wy1, wy2);
  segm_clipper.Run(v0, v1, intersections);
  ASSERT_EQ(expected_intersections.size(), intersections.size()) << message;

  gstVertexVector::iterator it = intersections.begin();
  gstVertexVector::iterator it_end = intersections.end();

  gstVertexVector::const_iterator it_expected = expected_intersections.begin();
  gstVertexVector::const_iterator it_expected_end =
      expected_intersections.end();

  for (; (it != it_end) && (it_expected != it_expected_end);
       ++it, ++it_expected) {
    const gstVertex &pt = *it;
    const gstVertex &expected_pt = *it_expected;
    ASSERT_DOUBLE_EQ(expected_pt.x, pt.x) << message;
    ASSERT_DOUBLE_EQ(expected_pt.y, pt.y) << message;
    ASSERT_DOUBLE_EQ(expected_pt.z, pt.z) << message;
  }
}

#else

// Tests gstBBox::ClipLine().
//
// Utility method for checking the results of Segment Clipping.
// v0, v1 - input segment end points.
// wx1, wx2, wy1, wy2 - coordinates of clipping rectangle window;
// expected_intersections - the expected intersection points.
// message - the error message for failures to help identify the
// failed test.
void CheckSegmentClipping(const gstVertex &_v0, const gstVertex &_v1,
                          double wx1, double wx2,
                          double wy1, double wy2,
                          const gstVertexVector &expected_intersections,
                          const std::string &message) {
  gstVertexVector intersections;
  gstVertex v0(_v0);
  gstVertex v1(_v1);

  gstBBox bbox(wx1, wx2, wy1, wy2);
  int ret = bbox.ClipLine(&v0, &v1);

  if (1 == ret || 2 == ret) {
    ASSERT_EQ(static_cast<int>(expected_intersections.size()), 2) << message;
    intersections.push_back(v0);
    intersections.push_back(v1);
  } else {
    ASSERT_EQ(static_cast<int>(expected_intersections.size()), 0) << message;
  }

  gstVertexVector::iterator it = intersections.begin();
  gstVertexVector::iterator it_end = intersections.end();

  gstVertexVector::const_iterator it_expected = expected_intersections.begin();
  gstVertexVector::const_iterator it_expected_end =
      expected_intersections.end();

  for (; (it != it_end) && (it_expected != it_expected_end);
       ++it, ++it_expected) {
    const gstVertex pt = *it;
    const gstVertex &expected_pt = *it_expected;
    ASSERT_DOUBLE_EQ(expected_pt.x, pt.x) << message;
    ASSERT_DOUBLE_EQ(expected_pt.y, pt.y) << message;
    ASSERT_DOUBLE_EQ(expected_pt.z, pt.z) << message;
  }
}

#endif

// Clip horizontal segment test.
// Segment end points lie in Q1, Q2, Q3 rectangles.
TEST_F(gstGeomUtilsClipSegmentTest, ClipHorizontalSegmentQ1Q2Q3Test) {
  double wx1 = 20.;
  double wy1 = 20.;
  double wx2 = 100.;
  double wy2 = 100.;

  double x1, y1;

  gstVertex v0;
  gstVertex v1;
  gstVertexVector expected_intersections;

  {
    x1 = .0;
    y1 = 10.;
    {
      // Horizontal segment to right Q1->Q1.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(18., 10., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to right Q1->Q1.");
    }

    {
      // Horizontal segment to left Q1->Q1.
      v0 = gstVertex(18., 10., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to left Q1->Q1.");
    }

    {
      // Horizontal segment Q1->Q1Q2.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(20., 10., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q1->Q1Q2.");
    }

    {
      // Horizontal segment Q2Q1->Q1.
      v0 = gstVertex(20, 10., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q2Q1->Q1.");
    }
    {
      // Horizontal segment Q1->Q2
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(70., 10., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q1->Q2.");
    }

    {
      // Horizontal segment Q2->Q1
      v0 = gstVertex(70., 10., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q1->Q2.");
    }

    {
      // Horizontal Q1->Q2Q3.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(100, 10., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q1->Q2Q3");
    }

    {
      // Horizontal segment Q3Q2->Q1.
      v0 = gstVertex(100, 10., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal Q3Q2->Q1.");
    }

    {
      // Horizontal segment Q1->Q3
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 10., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q1->Q3.");
    }

    {
      // Horizontal segment Q3->Q1.
      v0 = gstVertex(120., 10., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q3->Q1.");
    }
  }

  {
    x1 = 20.;
    y1 = 10.;
    {
      // Horizontal segment Q1Q2->Q2.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(70, 10., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q1Q2->Q2.");
    }

    {
      // Horizontal segment Q2->Q2Q1.
      v0 = gstVertex(70, 10., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q2->Q2Q1.");
    }

    {
      // Horizontal segment Q1Q2->Q2Q3.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(100, 10., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q1Q2->Q2Q3.");
    }

    {
      // Horizontal segment Q3Q2->Q2Q1.
      v0 = gstVertex(100, 10., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q3Q2->Q2Q1.");
    }

    {
      // Horizontal segment Q1Q2->Q3.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120, 10., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q1Q2->Q3.");
    }

    {
      // Horizontal segment Q3->Q2Q1.
      v0 = gstVertex(120, 10., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q3->Q2Q1.");
    }
  }

  {
    x1 = 40.;
    y1 = 10.;
    {
      // Horizontal segment to right Q2->Q2.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(70., 10., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to right Q2->Q2.");
    }

    {
      // Horizontal segment to left Q2->Q2.
      v0 = gstVertex(70., 10., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to left Q2->Q2.");
    }

    {
      // Horizontal segment Q2->Q2Q3
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(100., 10., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q2->Q2Q3.");
    }

    {
      // Horizontal segment Q3Q2->Q2
      v0 = gstVertex(100., 10., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q3Q2->Q2.");
    }

    {
      // Horizontal segment Q2->Q3
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 10., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q2->Q3.");
    }

    {
      // Horizontal segment Q3->Q2
      v0 = gstVertex(120., 10., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q3->Q2.");
    }
  }

  {
    x1 = 100.;
    y1 = 10.;
    {
      // Horizontal segment Q2Q3->Q3.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 10., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q2Q3->Q3.");
    }

    {
      // Horizontal segment Q3->Q3Q2.
      v0 = gstVertex(120, 10., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q3->Q3Q2.");
    }
  }

  {
    x1 = 120.;
    y1 = 10.;
    {
      // Horizontal segment to right Q3->Q3.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(150., 10., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to right Q3->Q3.");
    }

    {
      // Horizontal segment to left Q3->Q3.
      v0 = gstVertex(150., 10., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to left Q3->Q3.");
    }
  }
}

// Clip horizontal segment test.
// Segment end points lie in Q4, Q5 or Q6 rectangles.
TEST_F(gstGeomUtilsClipSegmentTest, ClipHorizontalSegmentQ4Q5Q6Test) {
  double wx1 = 20.;
  double wy1 = 20.;
  double wx2 = 100.;
  double wy2 = 100.;

  double x1, y1;

  gstVertex v0;
  gstVertex v1;
  gstVertexVector expected_intersections;

  {
    x1 = .0;
    y1 = 50.;
    {
      // Horizontal segment to right Q4->Q4.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(18., 50., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to right Q4->Q4.");
    }

    {
      // Horizontal segment to left Q4->Q4.
      v0 = gstVertex(18., 50., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to left Q4->Q4.");
    }
    { // Horizontal segment Q4->Q4Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(20., 50., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q4->Q4Q5.");
    }

    {
      // Horizontal segment Q5Q4->Q4.
      v0 = gstVertex(20, 50., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q5Q4->Q4.");
    }

    {
      // Horizontal segment Q4->Q5
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(70., 50., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(20., 50., .0));
      expected_intersections.push_back(gstVertex(70., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q4->Q5.");
    }

    {
      // Horizontal segment Q5->Q4.
      v0 = gstVertex(70., 50., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(70., 50., .0));
      expected_intersections.push_back(gstVertex(20., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q5->Q4.");
    }

    {
      // Horizontal segment Q4->Q5Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(100, 50., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(20., 50., .0));
      expected_intersections.push_back(gstVertex(100., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q4->Q5Q6.");
    }

    {
      // Horizontal segment Q6Q5->Q4.
      v0 = gstVertex(100, 50., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(100., 50., .0));
      expected_intersections.push_back(gstVertex(20., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q6Q5->Q4.");
    }

    {
      // Horizontal segment Q4->Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 50., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(20., 50., .0));
      expected_intersections.push_back(gstVertex(100., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q4->Q6");
    }

    {
      // Horizontal segment Q6->Q4.
      v0 = gstVertex(120., 50., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(100., 50., .0));
      expected_intersections.push_back(gstVertex(20., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q6->Q4.");
    }
  }

  {
    x1 = 20.;
    y1 = 50.;
    { // Horizontal segment Q4Q5->Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(70, 50., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(20., 50., .0));
      expected_intersections.push_back(gstVertex(70., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q4Q5->Q5.");
    }

    {
      // Horizontal segment Q5->Q5Q4.
      v0 = gstVertex(70, 50., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(70., 50., .0));
      expected_intersections.push_back(gstVertex(20., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q5->Q5Q4.");
    }

    { // Horizontal segment Q4Q5->Q5Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(100, 50., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(20., 50., .0));
      expected_intersections.push_back(gstVertex(100., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q4Q5->Q5Q6.");
    }

    {
      // Horizontal segment Q6Q5->Q5Q4.
      v0 = gstVertex(100, 50., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(100., 50., .0));
      expected_intersections.push_back(gstVertex(20., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q6Q5->Q5Q4.");
    }

    {
      // Horizontal segment Q4Q5->Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120, 50., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(20., 50., .0));
      expected_intersections.push_back(gstVertex(100., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q4Q5->Q6.");
    }

    {
      // Horizontal segment Q6->Q5Q4.
      v0 = gstVertex(120, 50., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(100., 50., .0));
      expected_intersections.push_back(gstVertex(20., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q6->Q5Q4.");
    }
  }

  {
    {
      // Horizontal segment to right Q5->Q5.
      v0 = gstVertex(40., 50., .0);
      v1 = gstVertex(70., 50., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 50., .0));
      expected_intersections.push_back(gstVertex(70., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to right Q4->Q4.");
    }

    {
      // Horizontal segment to left Q5->Q5.
      v0 = gstVertex(70., 50., .0);
      v1 = gstVertex(40., 50., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(70., 50., .0));
      expected_intersections.push_back(gstVertex(40., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to left Q4->Q4.");
    }
  }

  {
    x1 = 70.;
    y1 = 50.;
    {
      // Horizontal segment Q5->Q5Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(100., 50., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(70., 50., .0));
      expected_intersections.push_back(gstVertex(100., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q5->Q5Q6.");
    }

    {
      // Horizontal segment Q6Q5->Q5.
      v0 = gstVertex(100., 50., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(100., 50., .0));
      expected_intersections.push_back(gstVertex(70., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q6Q5->Q5.");
    }

    {
      // Horizontal segment Q5->Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 50., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(70., 50., .0));
      expected_intersections.push_back(gstVertex(100., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q5->Q6.");
    }

    {
      // Horizontal segment Q6->Q5.
      v0 = gstVertex(120., 50., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(100., 50., .0));
      expected_intersections.push_back(gstVertex(70., 50., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q6->Q5.");
    }
  }

  {
    x1 = 100.;
    y1 = 50.;
    {
      // Horizontal segment Q5Q6->Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 50., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q5Q6->Q6.");
    }

    {
      // Horizontal segment Q6->Q5Q6.
      v0 = gstVertex(120, 50., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q6->Q5Q6.");
    }
  }

  {
    {
      // Horizontal segment to right Q6->Q6.
      v0 = gstVertex(120., 50., .0);
      v1 = gstVertex(150., 50., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to right Q6->Q6.");
    }

    {
      // Horizontal segment to left Q6->Q6.
      v0 = gstVertex(150., 50., .0);
      v1 = gstVertex(120., 50., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to left Q6->Q6.");
    }
  }
}

// Clip horizontal segment test.
// Segment end points lie in Q7, Q8 or Q9 rectangles.
TEST_F(gstGeomUtilsClipSegmentTest, ClipHorizontalSegmentQ7Q8Q9Test) {
  double wx1 = 20.;
  double wy1 = 20.;
  double wx2 = 100.;
  double wy2 = 100.;
  gstVertex v0;
  gstVertex v1;
  gstVertexVector expected_intersections;

  {
    {
      // Horizontal segment to right Q7->Q7.
      v0 = gstVertex(0., 110., .0);
      v1 = gstVertex(18., 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to right Q7->Q7.");
    }

    {
      // Horizontal segment to left Q7->Q7.
      v0 = gstVertex(18., 110., .0);
      v1 = gstVertex(0., 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to left Q7->Q7.");
    }

    { // Horizontal segment Q7->Q7Q8.
      v0 = gstVertex(0., 110., .0);
      v1 = gstVertex(20., 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q7->Q7Q8.");
    }

    {
      // Horizontal segment Q8Q7->Q7.
      v0 = gstVertex(20, 110., .0);
      v1 = gstVertex(.0, 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q8Q7->Q7.");
    }

    {
      // Horizontal segment Q7->Q8
      v0 = gstVertex(.0, 110., .0);
      v1 = gstVertex(70., 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q7->Q8.");
    }

    {
      // Horizontal segment Q8->Q7.
      v0 = gstVertex(70., 110., .0);
      v1 = gstVertex(.0, 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q8->Q7.");
    }

    {
      // Horizontal segment Q7->Q8Q9.
      v0 = gstVertex(.0, 110., .0);
      v1 = gstVertex(100, 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q7->Q8Q9.");
    }

    {
      // Horizontal segment Q9Q8->Q7.
      v0 = gstVertex(100, 110., .0);
      v1 = gstVertex(.0, 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q9Q8->Q7.");
    }

    {
      // Horizontal segment Q7->Q9.
      v0 = gstVertex(.0, 110., .0);
      v1 = gstVertex(120., 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q7->Q9");
    }

    {
      // Horizontal segment Q9->Q7.
      v0 = gstVertex(120., 110., .0);
      v1 = gstVertex(.0, 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q9->Q7.");
    }
  }

  {
    {
      // Horizontal segment Q7Q8->Q8.
      v0 = gstVertex(20., 110., .0);
      v1 = gstVertex(70, 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q7Q8->Q8Q9.");
    }

    {
      // Horizontal segment Q8->Q8Q7.
      v0 = gstVertex(70, 110., .0);
      v1 = gstVertex(20, 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q8->Q8Q7.");
    }

    {
      // Horizontal segment Q7Q8->Q8Q9.
      v0 = gstVertex(20., 110., .0);
      v1 = gstVertex(100, 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q7Q8->Q8Q9.");
    }

    {
      // Horizontal segment Q9Q8->Q8Q7.
      v0 = gstVertex(100, 110., .0);
      v1 = gstVertex(20, 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q9Q8->Q8Q7.");
    }

    {
      // Horizontal segment Q7Q8->Q9.
      v0 = gstVertex(20., 110., .0);
      v1 = gstVertex(120, 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q7Q8->Q9.");
    }

    {
      // Horizontal segment Q9->Q8Q7.
      v0 = gstVertex(120, 110., .0);
      v1 = gstVertex(20, 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q9->Q8Q7.");
    }
  }

  {
    {
      // Horizontal segment to right Q8->Q8.
      v0 = gstVertex(40., 110., .0);
      v1 = gstVertex(70., 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to right Q8->Q8.");
    }

    {
      // Horizontal segment to left Q8->Q8.
      v0 = gstVertex(70., 110., .0);
      v1 = gstVertex(40., 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to left Q8->Q8.");
    }

    {
      // Horizontal segment Q8->Q8Q9.
      v0 = gstVertex(70., 110., .0);
      v1 = gstVertex(100., 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q8->Q8Q9.");
    }

    {
      // Horizontal segment Q9Q8->Q8.
      v0 = gstVertex(100., 110., .0);
      v1 = gstVertex(70., 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q9Q8->Q8.");
    }

    {
      // Horizontal segment Q8->Q9.
      v0 = gstVertex(70., 110., .0);
      v1 = gstVertex(120., 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q8->Q9.");
    }

    {
      // Horizontal segment Q9->Q8.
      v0 = gstVertex(120., 110., .0);
      v1 = gstVertex(70., 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q9->Q8.");
    }
  }

  {
    {
      // Horizontal segment Q8Q9->Q9.
      v0 = gstVertex(100., 110., .0);
      v1 = gstVertex(120., 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q8Q9->Q9.");
    }

    {
      // Horizontal segment Q9->Q9Q8.
      v0 = gstVertex(120, 110., .0);
      v1 = gstVertex(100, 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment Q9->Q9Q8.");
    }
  }

  {
    {
      // Horizontal segment to right Q9->Q9.
      v0 = gstVertex(120., 110., .0);
      v1 = gstVertex(150., 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to right Q9->Q9.");
    }

    {
      // Horizontal segment to left Q9->Q9.
      v0 = gstVertex(150., 110., .0);
      v1 = gstVertex(120., 110., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Horizontal segment to left Q9->Q9.");
    }
  }
}

// Clip vertical segment test.
// Segment end points lie in Q1, Q4, Q7.
TEST_F(gstGeomUtilsClipSegmentTest, ClipVerticalSegmentQ1Q4Q7Test) {
  double wx1 = 20.;
  double wy1 = 20.;
  double wx2 = 100.;
  double wy2 = 100.;
  gstVertex v0;
  gstVertex v1;
  gstVertexVector expected_intersections;

  {
    { // Vertical segment up Q1->Q1.
      v0 = gstVertex(10., 0., .0);
      v1 = gstVertex(10., 18., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment up Q1->Q1.");
    }

    { // Vertical segment down Q1->Q1.
      v0 = gstVertex(10., 18., .0);
      v1 = gstVertex(10., 0., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment down Q1->Q1.");
    }

    {
      // Vertical segment Q1->Q1Q4.
      v0 = gstVertex(10., .0, .0);
      v1 = gstVertex(10., 20., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q1->Q1Q4.");
    }

    {
      // Vertical segment Q4Q1->Q1.
      v0 = gstVertex(10., 20., .0);
      v1 = gstVertex(10., .0, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q4Q1->Q1.");
    }

    {
      // Vertical segment Q1->Q4.
      v0 = gstVertex(10., .0, .0);
      v1 = gstVertex(10., 70., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q1->Q4.");
    }

    {
      // Vertical segment Q4->Q1.
      v0 = gstVertex(10., 70., .0);
      v1 = gstVertex(10., .0, .0);


      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q4->Q1.");
    }

    {
      // Vertical segment Q1->Q4Q7.
      v0 = gstVertex(10., .0, .0);
      v1 = gstVertex(10., 100, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q1->Q4Q7.");
    }

    {
      // Vertical segment Q7Q4->Q1.
      v0 = gstVertex(10., 100., .0);
      v1 = gstVertex(10., .0, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q7Q4->Q1.");
    }

    {
      // Vertical segment Q1->Q7.
      v0 = gstVertex(10., .0, .0);
      v1 = gstVertex(10., 120., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q1->Q7.");
    }

    {
      // Vertical segment Q7->Q1.
      v0 = gstVertex(10.,  120., .0);
      v1 = gstVertex(10., .0, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q7->Q1");
    }
  }

  {
    {
      // Vertical segment Q1Q4->Q4Q7.
      v0 = gstVertex(10., 20., .0);
      v1 = gstVertex(10., 100., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q1Q4->Q4Q7.");
    }

    {
      // Vertical segment Q7Q4->Q4Q1.
      v0 = gstVertex(10., 100., .0);
      v1 = gstVertex(10., 20., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q7Q4->Q4Q1.");
    }

    {
      // Vertical segment Q1Q4->Q7.
      v0 = gstVertex(10., 20., .0);
      v1 = gstVertex(10., 120., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q1Q4->Q7.");
    }

    {
      // Vertical segment Q7->Q4Q1.
      v0 = gstVertex(10., 120., .0);
      v1 = gstVertex(10., 20., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q7->Q4Q1.");
    }
  }

  {
    {
      // Vertical segment up Q4->Q4.
      v0 = gstVertex(10., 40., .0);
      v1 = gstVertex(10., 70., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment up Q4->Q4.");
    }

    {
      // Vertical segment down Q4->Q4.
      v0 = gstVertex(10., 70., .0);
      v1 = gstVertex(10., 40., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment down Q4->Q4.");
    }

    {
      // Vertical segment Q4->Q7.
      v0 = gstVertex(10., 70., .0);
      v1 = gstVertex(10., 120., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q4->Q7.");
    }

    {
      // Vertical segment Q7->Q4.
      v0 = gstVertex(10., 120., .0);
      v1 = gstVertex(10., 70., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q7->Q4.");
    }
  }

  {
    {
      // Vertical segment Q4Q7->Q7.
      v0 = gstVertex(10., 100., .0);
      v1 = gstVertex(10., 120., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q4Q7->Q7.");
    }

    {
      // Vertical segment Q7->Q7Q4.
      v0 = gstVertex(10., 120., .0);
      v1 = gstVertex(10., 100., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q7->Q7Q4.");
    }
  }

  {
    {
      // Vertical segment up Q7->Q7.
      v0 = gstVertex(10., 120., .0);
      v1 = gstVertex(10., 150., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment up Q7->Q7.");
    }

    {
      // Vertical segment down Q7->Q7.
      v0 = gstVertex(10., 150., .0);
      v1 = gstVertex(10., 120., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment down Q7->Q7.");
    }
  }
}

// Clip vertical segment test.
// Segment end points lie in Q2, Q5, Q8.
TEST_F(gstGeomUtilsClipSegmentTest, ClipVerticalSegmentQ2Q5Q8Test) {
  double wx1 = 20.;
  double wy1 = 20.;
  double wx2 = 100.;
  double wy2 = 100.;
  gstVertex v0;
  gstVertex v1;
  gstVertexVector expected_intersections;

  {
    { // Vertical segment up Q2->Q2.
      v0 = gstVertex(50., 0., .0);
      v1 = gstVertex(50., 18., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment up Q2->Q2.");
    }

    { // Vertical segment down Q2->Q2.
      v0 = gstVertex(50., 18., .0);
      v1 = gstVertex(50., 0., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment down Q2->Q2.");
    }

    {
      // Vertical segment Q2->Q2Q5.
      v0 = gstVertex(50., .0, .0);
      v1 = gstVertex(50., 20., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q2->Q2Q5.");
    }

    {
      // Vertical segment Q5Q2->Q2.
      v0 = gstVertex(50., 20., .0);
      v1 = gstVertex(50., .0, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q5Q2->Q2.");
    }

    {
      // Vertical segment Q2->Q5.
      v0 = gstVertex(50., .0, .0);
      v1 = gstVertex(50., 70., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(50., 20., .0));
      expected_intersections.push_back(gstVertex(50., 70., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q2->Q5.");
    }

    {
      // Vertical segment Q5->Q2.
      v0 = gstVertex(50., 70., .0);
      v1 = gstVertex(50., .0, .0);


      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(50., 70., .0));
      expected_intersections.push_back(gstVertex(50., 20., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q5->Q2.");
    }

    {
      // Vertical segment Q2->Q5Q8.
      v0 = gstVertex(50., .0, .0);
      v1 = gstVertex(50., 100, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(50., 20., .0));
      expected_intersections.push_back(gstVertex(50., 100., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q2->Q5Q8.");
    }

    {
      // Vertical segment Q8Q5->Q2.
      v0 = gstVertex(50., 100., .0);
      v1 = gstVertex(50., .0, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(50., 100., .0));
      expected_intersections.push_back(gstVertex(50., 20., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q8Q5->Q2.");
    }

    {
      // Vertical segment Q2->Q8.
      v0 = gstVertex(50., .0, .0);
      v1 = gstVertex(50., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(50., 20., .0));
      expected_intersections.push_back(gstVertex(50., 100., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q2->Q8.");
    }

    {
      // Vertical segment Q8->Q2.
      v0 = gstVertex(50.,  120., .0);
      v1 = gstVertex(50., .0, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(50., 100., .0));
      expected_intersections.push_back(gstVertex(50., 20., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q8->Q2");
    }
  }

  {
    {
      // Vertical segment Q2Q5->Q5Q8.
      v0 = gstVertex(50., 20., .0);
      v1 = gstVertex(50., 100., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(50., 20., .0));
      expected_intersections.push_back(gstVertex(50., 100., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q2Q5->Q5Q8.");
    }

    {
      // Vertical segment Q8Q5->Q5Q2.
      v0 = gstVertex(50., 100., .0);
      v1 = gstVertex(50., 20., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(50., 100., .0));
      expected_intersections.push_back(gstVertex(50., 20., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q8Q5->Q5Q2.");
    }

    {
      // Vertical segment Q2Q5->Q8.
      v0 = gstVertex(50., 20., .0);
      v1 = gstVertex(50., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(50., 20., .0));
      expected_intersections.push_back(gstVertex(50., 100., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q2Q5->Q8.");
    }

    {
      // Vertical segment Q8->Q5Q2.
      v0 = gstVertex(50., 120., .0);
      v1 = gstVertex(50., 20., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(50., 100., .0));
      expected_intersections.push_back(gstVertex(50., 20., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q8->Q5Q2.");
    }
  }

  {
    {
      // Vertical segment up Q5->Q5.
      v0 = gstVertex(50., 40., .0);
      v1 = gstVertex(50., 70., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(50., 40., .0));
      expected_intersections.push_back(gstVertex(50., 70., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment up Q5->Q5.");
    }

    {
      // Vertical segment down Q5->Q5.
      v0 = gstVertex(50., 70., .0);
      v1 = gstVertex(50., 40., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(50., 70., .0));
      expected_intersections.push_back(gstVertex(50., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment down Q5->Q5.");
    }

    {
      // Vertical segment Q5->Q8.
      v0 = gstVertex(50., 70., .0);
      v1 = gstVertex(50., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(50., 70., .0));
      expected_intersections.push_back(gstVertex(50., 100., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q5->Q8.");
    }

    {
      // Vertical segment Q8->Q5.
      v0 = gstVertex(50., 120., .0);
      v1 = gstVertex(50., 70., .0);


      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(50., 100., .0));
      expected_intersections.push_back(gstVertex(50., 70., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q8->Q5.");
    }
  }

  {
    {
      // Vertical segment Q5Q8->Q8.
      v0 = gstVertex(50., 100., .0);
      v1 = gstVertex(50., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q5Q8->Q8.");
    }

    {
      // Vertical segment Q8->Q8Q5.
      v0 = gstVertex(50., 120., .0);
      v1 = gstVertex(50., 100., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q8->Q8Q5.");
    }
  }

  {
    {
      // Vertical segment up Q8->Q8.
      v0 = gstVertex(50., 120., .0);
      v1 = gstVertex(50., 150., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment up Q8->Q8.");
    }

    {
      // Vertical segment down Q8->Q8.
      v0 = gstVertex(50., 150., .0);
      v1 = gstVertex(50., 120., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment down Q8->Q8.");
    }
  }
}

// Clip vertical segment test.
// Segment end points lie in Q3, Q6, Q9.
TEST_F(gstGeomUtilsClipSegmentTest, ClipVerticalSegmentQ3Q6Q9Test) {
  double wx1 = 20.;
  double wy1 = 20.;
  double wx2 = 100.;
  double wy2 = 100.;
  gstVertex v0;
  gstVertex v1;
  gstVertexVector expected_intersections;

  {
    { // Vertical segment up Q3->Q3.
      v0 = gstVertex(120., 0., .0);
      v1 = gstVertex(120., 18., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment up Q3->Q3.");
    }

    { // Vertical segment down Q3->Q3.
      v0 = gstVertex(120., 18., .0);
      v1 = gstVertex(120., 0., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment down Q3->Q3.");
    }

    {
      // Vertical segment Q3->Q3Q6.
      v0 = gstVertex(120., .0, .0);
      v1 = gstVertex(120., 20., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q3->Q3Q6.");
    }

    {
      // Vertical segment Q6Q3->Q3.
      v0 = gstVertex(120., 20., .0);
      v1 = gstVertex(120., .0, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q6Q3->Q3.");
    }

    {
      // Vertical segment Q3->Q6.
      v0 = gstVertex(120., .0, .0);
      v1 = gstVertex(120., 70., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q3->Q6.");
    }

    {
      // Vertical segment Q6->Q3.
      v0 = gstVertex(120., 70., .0);
      v1 = gstVertex(120., .0, .0);


      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q6->Q3.");
    }

    {
      // Vertical segment Q3->Q6Q9.
      v0 = gstVertex(120., .0, .0);
      v1 = gstVertex(120., 100, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q3->Q6Q9.");
    }

    {
      // Vertical segment Q9Q6->Q3.
      v0 = gstVertex(120., 100., .0);
      v1 = gstVertex(120., .0, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q9Q6->Q3.");
    }

    {
      // Vertical segment Q3->Q9.
      v0 = gstVertex(120., .0, .0);
      v1 = gstVertex(120., 120., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q3->Q9.");
    }

    {
      // Vertical segment Q9->Q3.
      v0 = gstVertex(120.,  120., .0);
      v1 = gstVertex(120., .0, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q9->Q3");
    }
  }

  {
    {
      // Vertical segment Q3Q6->Q6Q9.
      v0 = gstVertex(120., 20., .0);
      v1 = gstVertex(120., 100., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q3Q6->Q6Q9.");
    }

    {
      // Vertical segment Q9Q6->Q6Q3.
      v0 = gstVertex(120., 100., .0);
      v1 = gstVertex(120., 20., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q9Q6->Q6Q3.");
    }

    {
      // Vertical segment Q3Q6->Q9.
      v0 = gstVertex(120., 20., .0);
      v1 = gstVertex(120., 120., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q3Q6->Q9.");
    }

    {
      // Vertical segment Q9->Q6Q3.
      v0 = gstVertex(120., 120., .0);
      v1 = gstVertex(120., 20., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q9->Q6Q3.");
    }
  }

  {
    {
      // Vertical segment up Q6->Q6.
      v0 = gstVertex(120., 40., .0);
      v1 = gstVertex(120., 70., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment up Q6->Q6.");
    }

    {
      // Vertical segment down Q6->Q6.
      v0 = gstVertex(120., 70., .0);
      v1 = gstVertex(120., 40., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment down Q6->Q6.");
    }

    {
      // Vertical segment Q6->Q9.
      v0 = gstVertex(120., 70., .0);
      v1 = gstVertex(120., 120., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q6->Q9.");
    }

    {
      // Vertical segment Q9->Q6.
      v0 = gstVertex(120., 120., .0);
      v1 = gstVertex(120., 70., .0);


      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q9->Q6.");
    }
  }

  {
    {
      // Vertical segment Q6Q9->Q9.
      v0 = gstVertex(120., 100., .0);
      v1 = gstVertex(120., 120., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q6Q9->Q9.");
    }


    {
      // Vertical segment Q9->Q9Q6.
      v0 = gstVertex(120., 120., .0);
      v1 = gstVertex(120., 100., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment Q9->Q9Q6.");
    }
  }

  {
    {
      // Vertical segment up Q9->Q9.
      v0 = gstVertex(120., 120., .0);
      v1 = gstVertex(120., 150., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment up Q9->Q9.");
    }

    {
      // Vertical segment down Q9->Q9.
      v0 = gstVertex(120., 150., .0);
      v1 = gstVertex(120., 120., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Vertical segment down Q9->Q9.");
    }
  }
}


// Clip slope45 segment.
// Segment end point lie in Q2, Q3, Q6.
TEST_F(gstGeomUtilsClipSegmentTest, ClipSlope45SegmentQ2Q3Q6Test) {
  double wx1 = 40.;
  double wy1 = 40.;
  double wx2 = 120.;
  double wy2 = 120.;

  double x1, y1;

  gstVertex v0;
  gstVertex v1;
  gstVertexVector expected_intersections;

  {
    // left end point in Q2;
    // right end point in Q2, Q3, Q6;
    x1 = 100.;
    y1 = .0;
    {  // Slope45 segment Q2->Q2Q3.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 20., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2->Q2Q3.");
    }

    {  // Slope45 segment Q3Q2->Q2.
      v0 = gstVertex(120., 20., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q3Q2->Q2.");
    }

    {  // Slope45 segment Q2->Q3.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(130., 30., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2->Q3.");
    }

    {  // Slope45 segment Q3->Q2.
      v0 = gstVertex(130., 30., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q3->Q2.");
    }

    {  // Slope45 segment Q2->Q3Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 40., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2->Q3Q6.");
    }

    {  // Slope45 segment Q6Q3->Q2.
      v0 = gstVertex(140., 40., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6Q3->Q2.");
    }

    {  // Slope45 segment Q2->Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(160., 60., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2->Q6.");
    }

    {  // Slope45 segment Q6->Q2.
      v0 = gstVertex(160., 60., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6->Q2.");
    }
  }

  {
    // left end point on Q2Q3
    // right end point in Q3,Q6
    x1 = 120.;
    y1 = 20.;
    {  // Slope45 segment Q2Q3->Q3.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(130., 30., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2Q3->Q3.");
    }

    {  // Slope45 segment Q3->Q3Q2.
      v0 = gstVertex(130., 30., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q3->Q3Q2.");
    }

    {  // Slope45 segment Q2Q3->Q3Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 40., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2Q3->Q3Q6.");
    }

    {  // Slope45 segment Q6Q3->Q3Q2.
      v0 = gstVertex(140., 40., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6Q3->Q3Q2.");
    }

    {  // Slope45 segment Q2Q3->Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(160., 60., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2Q3->Q6.");
    }

    {  // Slope45 segment Q6->Q3Q2.
      v0 = gstVertex(160., 60., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6->Q3Q2.");
    }
  }

  {
    // left end point in Q3,
    // right end point in Q3, Q6
    x1 = 130.;
    y1 = 30;

    {  // Slope45 segment Q3->Q3Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 40., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q3->Q3Q6.");
    }

    {  // Slope45 segment Q6Q3->Q3.
      v0 = gstVertex(140., 40., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6Q3->Q3.");
    }

    {  // Slope45 segment Q3->Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(160.0, 60., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q3->Q6.");
    }

    {  // Slope45 segment Q6->Q3.
      v0 = gstVertex(160.0, 60., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6->Q3.");
    }
  }

  {
    // left end point on Q3Q6;
    // right end point in Q6;
    x1 = 140.;
    y1 = 40.;
    {  // Slope45 segment Q3Q6->Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(160., 60., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q3Q6->Q6.");
    }

    {  // Slope45 segment Q6->Q6Q3.
      v0 = gstVertex(160., 60., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6->Q6Q3.");
    }
  }
}

// Clip Slope45 segment Q2Q6 (diagonal)
TEST_F(gstGeomUtilsClipSegmentTest, ClipSlope45SegmentQ2Q6Test) {
  double wx1 = 40.;
  double wy1 = 40.;
  double wx2 = 120.;
  double wy2 = 120.;

  double x1, y1;

  gstVertex v0;
  gstVertex v1;
  gstVertexVector expected_intersections;

  {
    // left end point in Q2;
    // right end point in Q6
    x1 = 100.;
    y1 = 20.;

    {  // Slope45 segment Q2->Q2Q6 touches corner.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 40., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2->Q2Q6 touches corner.");
    }

    {  // Slope45 segment Q6Q2->Q2 touches corner.
      v0 = gstVertex(120., 40., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6Q2->Q2 touches corner.");
    }

    {  // Slope45 segment Q2->Q6 intersects corner.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 60., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 40., .0));
      expected_intersections.push_back(gstVertex(120., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2->Q6 intersects corner.");
    }

    {  // Slope45 segment Q6->Q2 intersects corner.
      v0 = gstVertex(140., 60., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 40., .0));
      expected_intersections.push_back(gstVertex(120., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6->Q2 intersects corner.");
    }
  }

  {
    // left end point on Q2Q6;
    // right end point in Q6;
    x1 = 120.;
    y1 = 40.;
    {  // Slope45 segment Q2Q6->Q6 touches corner.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 60., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2Q6->Q6 touches corner.");
    }

    {  // Slope45 segment Q6->Q6Q2 touches corner.
      v0 = gstVertex(140., 60., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6->Q6Q2 touches corner.");
    }
  }
}

// Clip slope45 segment.
// Segment end points lie in Q1,Q2,Q5,Q6,Q9.
TEST_F(gstGeomUtilsClipSegmentTest, ClipSlope45SegmentQ1Q2Q5Q6Q9Test) {
  double wx1 = 40.;
  double wy1 = 40.;
  double wx2 = 120.;
  double wy2 = 120.;

  double x1, y1;

  gstVertex v0;
  gstVertex v1;
  gstVertexVector expected_intersections;

  {  // Left end point lies in Q1
    // Right end point lies in Q1,Q2,Q5,Q6,Q9.
    x1 = 20;
    y1 = -20;
    {  // Slope45 segment up Q1->Q1.
      v0 = gstVertex(0., -40., .0);
      v1 = gstVertex(20., -20., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment up Q1->Q1.");
    }

    {  // Slope45 segment down Q1->Q1.
      v0 = gstVertex(20., -20., .0);
      v1 = gstVertex(0., -40., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment down Q1->Q1.");
    }

    {  // Slope45 segment Q1->Q1Q2.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(40., .0, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q1Q2.");
    }

    {  // Slope45 segment Q2Q1->Q1.
      v0 = gstVertex(40., .0, .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2Q1->Q1.");
    }

    {  // Slope45 segment Q1->Q2.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(60., 20., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q2.");
    }

    {  // Slope45 segment Q2->Q1.
      v0 = gstVertex(60., 20., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2->Q1.");
    }

    {  // Slope45 segment Q1->Q2Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(80., 40., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q2Q5.");
    }

    {  // Slope45 segment Q5Q2->Q1.
      v0 = gstVertex(80., 40., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5Q2->Q1.");
    }

    {  // Slope45 segment Q1->Q2->Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(100., 60., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(100., 60., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q2->Q5.");
    }

    {  // Slope45 segment Q5->Q2->Q1.
      v0 = gstVertex(100., 60., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(100., 60., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q2->Q1.");
    }

    {  // Slope45 segment Q1->Q5Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 80., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q5Q6.");
    }

    {  // Slope45 segment Q6Q5->Q1.
      v0 = gstVertex(120., 80., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6Q5->Q1.");
    }

    {  // Slope45 segment Q1->Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 100., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q6.");
    }

    {  // Slope45 segment Q6->Q1.
      v0 = gstVertex(140., 100., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6->Q1.");
    }

    {  // Slope45 segment Q1->Q6Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(160., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q6Q9.");
    }

    {  // Slope45 segment Q9Q6->Q1.
      v0 = gstVertex(160., 120., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9Q6->Q1.");
    }

    {  // Slope45 segment Q1->Q2->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(180., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q2->Q9.");
    }

    {  // Slope45 segment Q9->Q2->Q1.
      v0 = gstVertex(180., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q2->Q1.");
    }
  }

  {
    // Left end point lies on the boundary Q1Q2
    // Right end point lies in Q2,Q5,Q6,Q9.
    x1 = 40.;
    y1 = .0;
    {  // Slope45 segment Q1Q2->Q2.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(60., 20., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1Q2->Q2.");
    }

    {  // Slope45 segment Q2->Q2Q1.
      v0 = gstVertex(60., 20., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2->Q2Q1.");
    }

    {  // Slope45 segment Q1Q2->Q2Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(80., 40., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1Q2->Q2Q5.");
    }

    {  // Slope45 segment Q5Q2->Q2Q1.
      v0 = gstVertex(80., 40., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5Q2->Q2Q1.");
    }

    {  // Slope45 segment Q1Q2->Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(100., 60., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(100., 60., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1Q2->Q5.");
    }

    {  // Slope45 segment Q5->Q2Q1.
      v0 = gstVertex(100., 60., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(100., 60., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q2Q1.");
    }

    {  // Slope45 segment Q1Q2->Q5Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 80., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1Q2->Q5Q6.");
    }

    {  // Slope45 segment Q6Q5->Q2Q1.
      v0 = gstVertex(120., 80., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6Q5->Q2Q1.");
    }

    {  // Slope45 segment Q1Q2->Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 100., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1Q2->Q6.");
    }

    {  // Slope45 segment Q6->Q2Q1.
      v0 = gstVertex(140., 100., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6->Q2Q1.");
    }

    {  // Slope45 segment Q1Q2->Q6Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(160., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1Q2->Q6Q9.");
    }

    {  // Slope45 segment Q9Q6->Q2Q1.
      v0 = gstVertex(160., 120., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9Q6->Q2Q1.");
    }

    {  // Slope45 segment Q1Q2->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(180., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1Q2->Q9.");
    }

    {  // Slope45 segment Q9->Q2Q1.
      v0 = gstVertex(180., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q2Q1.");
    }
  }

  {
    // Left end point lies in Q2
    // Right end point lies in Q2,Q5,Q6,Q9.
    x1 = 60.;
    y1 = 20.;

    {  // Slope45 segment Q2->Q2 up.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(75., 35., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2->Q2 up.");
    }

    {  // Slope45 segment Q2->Q2 down.
      v0 = gstVertex(75., 35., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2->Q2 down.");
    }

    {  // Slope45 segment Q2->Q2Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(80., 40., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2->Q2Q5.");
    }

    {  // Slope45 segment Q5Q2->Q2.
      v0 = gstVertex(80., 40., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5Q2->Q2.");
    }

    {  // Slope45 segment Q2->Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(100., 60., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(100., 60., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2->Q5.");
    }

    {  // Slope45 segment Q5->Q2.
      v0 = gstVertex(100., 60., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(100., 60., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q2.");
    }

    {  // Slope45 segment Q2->Q5Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 80., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2->Q5Q6.");
    }

    {  // Slope45 segment Q6Q5->Q2.
      v0 = gstVertex(120., 80., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6Q5->Q2.");
    }

    {  // Slope45 segment Q2->Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 100., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2->Q6.");
    }

    {  // Slope45 segment Q6->Q2.
      v0 = gstVertex(140., 100., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6->Q2.");
    }

    {  // Slope45 segment Q2->Q6Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(160., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2->Q6Q9.");
    }

    {  // Slope45 segment Q9Q6->Q2.
      v0 = gstVertex(160., 120., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9Q6->Q2.");
    }

    {  // Slope45 segment Q2->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(180., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2->Q9.");
    }

    {  // Slope45 segment Q9->Q2.
      v0 = gstVertex(180., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q2.");
    }
  }

  {
    // Left end point lies on boundary Q2Q5.
    // Right end point lies in Q5,Q6,Q9.
    x1 = 80.;
    y1 = 40.;
    {  // Slope45 segment Q2Q5->Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(100., 60., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(100., 60., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2Q5->Q5.");
    }

    {  // Slope45 segment Q5->Q5Q2.
      v0 = gstVertex(100., 60., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(100., 60., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q5Q2.");
    }

    {  // Slope45 segment Q2Q5->Q5Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 80., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2Q5->Q5Q6.");
    }

    {  // Slope45 segment Q6Q5->Q5Q2.
      v0 = gstVertex(120., 80., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6Q5->Q5Q2.");
    }

    {  // Slope45 segment Q2Q5->Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 100., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2Q5->Q6.");
    }

    {  // Slope45 segment Q6->Q5Q2.
      v0 = gstVertex(140., 100., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6->Q5Q2.");
    }

    {  // Slope45 segment Q2Q5->Q6Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(160., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2Q5->Q6Q9.");
    }

    {  // Slope45 segment Q9Q6->Q5Q2.
      v0 = gstVertex(160., 120., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9Q6->Q5Q2.");
    }

    {  // Slope45 segment Q2Q5->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(180., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q2Q5->Q9.");
    }

    {  // Slope45 segment Q9->Q5Q2.
      v0 = gstVertex(180., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q5Q2.");
    }
  }

  {
    // Left end point lies in Q5.
    // Right end point lies in Q5,Q6,Q9.
    x1 = 100.;
    y1 = 60.;
    {  // Slope45 segment Q5->Q5 up.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(115., 75., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(100., 60., .0));
      expected_intersections.push_back(gstVertex(115., 75., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q5 up.");
    }

    {  // Slope45 segment Q5->Q5 down.
      v0 = gstVertex(115., 75., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(115., 75., .0));
      expected_intersections.push_back(gstVertex(100., 60., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q5 down.");
    }

    {  // Slope45 segment Q5->Q5Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 80., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(x1, y1, .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q5Q6.");
    }

    {  // Slope45 segment Q6Q5->Q5.
      v0 = gstVertex(120., 80., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(x1, y1, .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6Q5->Q5.");
    }

    {  // Slope45 segment Q5->Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 100., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(x1, y1, .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q6.");
    }

    {  // Slope45 segment Q6->Q5.
      v0 = gstVertex(140., 100., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(x1, y1, .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6->Q5.");
    }

    {  // Slope45 segment Q5->Q6Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(160., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(x1, y1, .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q6Q9.");
    }

    {  // Slope45 segment Q9Q6->Q5.
      v0 = gstVertex(160., 120., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(x1, y1, .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9Q6->Q5.");
    }

    {  // Slope45 segment Q5->Q6->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(180., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(x1, y1, .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q6->Q9.");
    }

    {  // Slope45 segment Q9->Q6->Q5.
      v0 = gstVertex(180., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(x1, y1, .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q6->Q5.");
    }
  }

  {
    // Left end point lies on boundary Q5Q6.
    // Right end point lies in Q6,Q9.
    x1 = 120.;
    y1 = 80.;
    {  // Slope45 segment Q5Q6->Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 100., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5Q6->Q6.");
    }

    {  // Slope45 segment Q6->Q6Q5.
      v0 = gstVertex(140., 100., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6->Q6Q5.");
    }

    {  // Slope45 segment Q5Q6->Q6Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(160., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5Q6->Q6Q9.");
    }

    {  // Slope45 segment Q9Q6->Q6Q5.
      v0 = gstVertex(160., 120., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9Q6->Q6Q5.");
    }

    {  // Slope45 segment Q5Q6->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(180., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5Q6->Q9.");
    }

    {  // Slope45 segment Q9->Q6Q5.
      v0 = gstVertex(180., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q6Q5.");
    }
  }

  {
    // Left end point lies in Q6.
    // Right end point lies in Q6,Q9.
    x1 = 140.;
    y1 = 100.;
    {  // Slope45 segment Q6->Q6 up.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(155., 115., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6->Q6 up.");
    }

    {  // Slope45 segment Q6->Q6 down.
      v0 = gstVertex(155., 115., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6->Q6 down.");
    }

    {  // Slope45 segment Q6->Q6Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(160., 120., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6->Q6Q9.");
    }

    {  // Slope45 segment Q9Q6->Q6.
      v0 = gstVertex(160., 120., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9Q6->Q6.");
    }

    {  // Slope45 segment Q6->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(180., 140., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6->Q9.");
    }

    {  // Slope45 segment Q9->Q6.
      v0 = gstVertex(180., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q6.");
    }
  }

  {
    // Left end point lies on boundary Q6Q9.
    // Right end point lies in Q9.
    x1 = 160.;
    y1 = 120.;
    {  // Slope45 segment Q6Q9->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(180., 140., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q6Q9->Q9.");
    }

    {  // Slope45 segment Q9->Q9Q6.
      v0 = gstVertex(180., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q9Q6.");
    }
  }

  {  // Segment end points lie in Q9.
    x1 = 180.;
    y1 = 140.;
    {  // Slope45 segment Q9->Q9 up.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(200., 160., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q9 up.");
    }

    {  // Slope45 segment Q9->Q9 down.
      v0 = gstVertex(200., 160., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q9 down.");
    }
  }
}

// Clip slope45 segment Q1Q5Q9 (diagonal).
TEST_F(gstGeomUtilsClipSegmentTest, ClipSlope45SegmentQ1Q5Q9Test) {
  double wx1 = 40.;
  double wy1 = 40.;
  double wx2 = 120.;
  double wy2 = 120.;

  double x1, y1;

  gstVertex v0;
  gstVertex v1;
  gstVertexVector expected_intersections;

  {  // Left end point lies in Q1,
    // Right end point lies Q1, Q5, Q9.
    x1 = 20.;
    y1 = 20.;

    {  // Slope45 segment Q1->Q1Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(40., 40., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q1Q5.");
    }

    {  // Slope45 segment Q5Q1->Q1.
      v0 = gstVertex(40., 40., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5Q1->Q1.");
    }

    {  // Slope45 segment Q1->Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(80., 80., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 40., .0));
      expected_intersections.push_back(gstVertex(80., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q5.");
    }

    {  // Slope45 segment Q5->Q1.
      v0 = gstVertex(80., 80., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 80., .0));
      expected_intersections.push_back(gstVertex(40., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q1.");
    }

    {  // Slope45 segment Q1->Q5Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 40., .0));
      expected_intersections.push_back(gstVertex(120., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q5Q9.");
    }

    {  // Slope45 segment Q9Q5->Q1.
      v0 = gstVertex(120., 120., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 120., .0));
      expected_intersections.push_back(gstVertex(40., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9Q5->Q1.");
    }

    {  // Slope45 segment Q1->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 40., .0));
      expected_intersections.push_back(gstVertex(120., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q9.");
    }

    {  // Slope45 segment Q9->Q1.
      v0 = gstVertex(140., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 120., .0));
      expected_intersections.push_back(gstVertex(40., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q1.");
    }
  }

  {  // Left end point lies on Q1Q5,
    // Right end point lies Q5,Q9.
    x1 = 40.;
    y1 = 40.;
    {  // Slope45 segment Q1Q5->Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(80., 80., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 40., .0));
      expected_intersections.push_back(gstVertex(80., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1Q5->Q5.");
    }

    {  // Slope45 segment Q5Q1->Q1.
      v0 = gstVertex(80., 80., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 80., .0));
      expected_intersections.push_back(gstVertex(40., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5Q1->Q1.");
    }

    {  // Slope45 segment Q1Q5->Q5Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 40., .0));
      expected_intersections.push_back(gstVertex(120., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1Q5->Q5Q9.");
    }

    {  // Slope45 segment Q9Q5->Q5Q1.
      v0 = gstVertex(120., 120., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 120., .0));
      expected_intersections.push_back(gstVertex(40., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9Q5->Q5Q1.");
    }

    {  // Slope45 segment Q1Q5->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 40., .0));
      expected_intersections.push_back(gstVertex(120., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1Q5->Q9.");
    }

    {  // Slope45 segment Q9->Q5Q1.
      v0 = gstVertex(140., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 120., .0));
      expected_intersections.push_back(gstVertex(40., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q5Q1.");
    }
  }

  {  // Left end point lies in Q5,
    // Right end point lies Q5,Q9.
    x1 = 80.;
    y1 = 80.;
    {  // Slope45 segment Q5->Q5Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(x1, y1, .0));
      expected_intersections.push_back(gstVertex(120., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q5Q9.");
    }

    {  // Slope45 segment Q9Q5->Q5.
      v0 = gstVertex(120., 120., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 120., .0));
      expected_intersections.push_back(gstVertex(x1, y1, .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9Q5->Q5.");
    }

    {  // Slope45 segment Q5->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(x1, y1, .0));
      expected_intersections.push_back(gstVertex(120., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q9.");
    }

    {  // Slope45 segment Q9->Q5.
      v0 = gstVertex(140., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 120., .0));
      expected_intersections.push_back(gstVertex(x1, y1, .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q5.");
    }
  }

  {  // Left end point lies on Q5Q9,
    // Right end point lies in Q9.
    x1 = 120.;
    y1 = 120.;

    {  // Slope45 segment Q5Q9->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5Q9->Q9.");
    }

    {  // Slope45 segment Q9->Q9Q5.
      v0 = gstVertex(140., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q9Q5.");
    }
  }
}


// Clip slope45 segment.
// Segment end points lie in Q1,Q4,Q5,Q8,Q9.
TEST_F(gstGeomUtilsClipSegmentTest, ClipSlope45SegmentQ1Q4Q5Q8Q9Test) {
  double wx1 = 40.;
  double wy1 = 40.;
  double wx2 = 120.;
  double wy2 = 120.;

  double x1, y1;

  gstVertex v0;
  gstVertex v1;
  gstVertexVector expected_intersections;

  {  // Left end point lies in Q1
    // Right end point lies in Q2,Q5,Q6,Q9.
    x1 = -20;
    y1 = 20;
    {  // Slope45 segment Q1->Q1Q4.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(.0, 40., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q1Q4.");
    }

    {  // Slope45 segment Q4Q1->Q1.
      v0 = gstVertex(.0, 40., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4Q1->Q1.");
    }

    {  // Slope45 segment Q1->Q4.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(20., 60., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q4.");
    }

    {  // Slope45 segment Q4->Q1.
      v0 = gstVertex(20., 60., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4->Q1.");
    }

    {  // Slope45 segment Q1->Q4Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(40., 80., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q4Q5.");
    }

    {  // Slope45 segment Q5Q4->Q1.
      v0 = gstVertex(40., 80., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5Q4->Q1.");
    }

    {  // Slope45 segment Q1->Q4->Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(60., 100., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(60., 100., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q4->Q5.");
    }

    {  // Slope45 segment Q5->Q4->Q1.
      v0 = gstVertex(60., 100., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(60., 100., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q4->Q1.");
    }

    {  // Slope45 segment Q1->Q5Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(80., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q5Q8.");
    }

    {  // Slope45 segment Q8Q5->Q1.
      v0 = gstVertex(80., 120., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8Q5->Q1.");
    }

    {  // Slope45 segment Q1->Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(100., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q8.");
    }

    {  // Slope45 segment Q8->Q1.
      v0 = gstVertex(100., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8->Q1.");
    }

    {  // Slope45 segment Q1->Q8Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 160., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q8Q9.");
    }

    {  // Slope45 segment Q9Q8->Q1.
      v0 = gstVertex(120., 160., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9Q8->Q1.");
    }

    {  // Slope45 segment Q1->Q4->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 180., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1->Q4->Q9.");
    }

    {  // Slope45 segment Q9->Q4->Q1.
      v0 = gstVertex(140., 180., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q4->Q1.");
    }
  }

  {
    // Left end point lies on the boundary Q1Q4
    // Right end point lies in Q4,Q5,Q8,Q9.
    x1 = .0;
    y1 = 40.;
    {  // Slope45 segment Q1Q4->Q4.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(20., 60., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1Q4->Q4.");
    }

    {  // Slope45 segment Q4->Q4Q1.
      v0 = gstVertex(20., 60., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4->Q4Q1.");
    }

    {  // Slope45 segment Q1Q4->Q4Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(40., 80., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1Q4->Q4Q5.");
    }

    {  // Slope45 segment Q5Q4->Q4Q1.
      v0 = gstVertex(40., 80., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5Q4->Q4Q1.");
    }

    {  // Slope45 segment Q1Q4->Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(60., 100., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(60., 100., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1Q4->Q5.");
    }

    {  // Slope45 segment Q5->Q4Q1.
      v0 = gstVertex(60., 100., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(60., 100., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q4Q1.");
    }

    {  // Slope45 segment Q1Q4->Q5Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(80., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1Q4->Q5Q8.");
    }

    {  // Slope45 segment Q8Q5->Q4Q1.
      v0 = gstVertex(80., 120., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8Q5->Q4Q1.");
    }

    {  // Slope45 segment Q1Q4->Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(100., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1Q4->Q8.");
    }

    {  // Slope45 segment Q8->Q4Q1.
      v0 = gstVertex(100., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8->Q4Q1.");
    }

    {  // Slope45 segment Q1Q4->Q8Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 160., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1Q4->Q8Q9.");
    }

    {  // Slope45 segment Q9Q8->Q4Q1.
      v0 = gstVertex(120., 160., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9Q8->Q4Q1.");
    }

    {  // Slope45 segment Q1Q4->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 180., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q1Q4->Q9.");
    }

    {  // Slope45 segment Q9->Q4Q1.
      v0 = gstVertex(140., 180., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q4Q1.");
    }
  }

  {
    // Left end point lies in Q4
    // Right end point lies in Q4,Q5,Q6,Q9.
    x1 = 20.;
    y1 = 60.;

    {  // Slope45 segment Q4->Q4 up.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(35., 75., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4->Q4 up.");
    }

    {  // Slope45 segment Q4->Q4 down.
      v0 = gstVertex(35., 75., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4->Q4 down.");
    }

    {  // Slope45 segment Q4->Q4Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(40., 80., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4->Q4Q5.");
    }

    {  // Slope45 segment Q5Q4->Q4.
      v0 = gstVertex(40., 80., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5Q4->Q4.");
    }

    {  // Slope45 segment Q4->Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(60., 100., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(60., 100., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4->Q5.");
    }

    {  // Slope45 segment Q5->Q4.
      v0 = gstVertex(60., 100., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(60., 100., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q4.");
    }

    {  // Slope45 segment Q4->Q5Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(80., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4->Q5Q8.");
    }

    {  // Slope45 segment Q8Q5->Q4.
      v0 = gstVertex(80., 120., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8Q5->Q4.");
    }

    {  // Slope45 segment Q4->Q5->Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(100., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4->Q5->Q8.");
    }

    {  // Slope45 segment Q8->Q5>Q4.
      v0 = gstVertex(100., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8->Q5->Q4.");
    }

    {  // Slope45 segment Q4->Q8Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 160., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4->Q8Q9.");
    }

    {  // Slope45 segment Q9Q8->Q4.
      v0 = gstVertex(120., 160., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9Q8->Q4.");
    }

    {  // Slope45 segment Q4->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 180., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4->Q9.");
    }

    {  // Slope45 segment Q9->Q4.
      v0 = gstVertex(140., 180., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q4.");
    }
  }

  {
    // Left end point lies on boundary Q4Q5.
    // Right end point lies in Q5,Q6,Q9.
    x1 = 40.;
    y1 = 80.;
    {  // Slope45 segment Q4Q5->Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(60., 100., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(60., 100., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4Q5->Q5.");
    }

    {  // Slope45 segment Q5->Q5Q4.
      v0 = gstVertex(60., 100., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(60., 100., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q5Q4.");
    }

    {  // Slope45 segment Q4Q5->Q5Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(80., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4Q5->Q5Q8.");
    }

    {  // Slope45 segment Q8Q5->Q5Q4.
      v0 = gstVertex(80., 120., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8Q5->Q5Q4.");
    }

    {  // Slope45 segment Q4Q5->Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(100., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4Q5->Q8.");
    }

    {  // Slope45 segment Q8->Q5Q4.
      v0 = gstVertex(100., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8->Q5Q4.");
    }

    {  // Slope45 segment Q4Q5->Q8Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 160., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4Q5->Q8Q9.");
    }

    {  // Slope45 segment Q9Q8->Q5Q4.
      v0 = gstVertex(120., 160., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9Q8->Q5Q4.");
    }

    {  // Slope45 segment Q4Q5->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 180., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4Q5->Q9.");
    }

    {  // Slope45 segment Q9->Q5Q4.
      v0 = gstVertex(140., 180., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q5Q4.");
    }
  }

  {
    // Left end point lies in Q5.
    // Right end point lies in Q5,Q8,Q9.
    x1 = 60.;
    y1 = 100.;
    {  // Slope45 segment Q5->Q5Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(80., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(x1, y1, .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q5Q8.");
    }

    {  // Slope45 segment Q8Q5->Q5.
      v0 = gstVertex(80., 120., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(x1, y1, .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8Q5->Q5.");
    }

    {  // Slope45 segment Q5->Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(100., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(x1, y1, .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q8.");
    }

    {  // Slope45 segment Q8->Q5.
      v0 = gstVertex(100., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(x1, y1, .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8->Q5.");
    }

    {  // Slope45 segment Q5->Q8Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 160., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(x1, y1, .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q8Q9.");
    }

    {  // Slope45 segment Q9Q8->Q5.
      v0 = gstVertex(120., 160., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(x1, y1, .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9Q8->Q5.");
    }

    {  // Slope45 segment Q5->Q8->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 180., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(x1, y1, .0));
      expected_intersections.push_back(gstVertex(80., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5->Q8->Q9.");
    }

    {  // Slope45 segment Q9->Q8->Q5.
      v0 = gstVertex(140., 180., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 120., .0));
      expected_intersections.push_back(gstVertex(x1, y1, .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q8->Q5.");
    }
  }

  {
    // Left end point lies on boundary Q5Q8.
    // Right end point lies in Q8,Q9.
    x1 = 80.;
    y1 = 120.;
    {  // Slope45 segment Q5Q8->Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(100., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5Q8->Q8.");
    }

    {  // Slope45 segment Q8->Q8Q5.
      v0 = gstVertex(100., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8->Q8Q5.");
    }

    {  // Slope45 segment Q5Q8->Q8Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 160., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5Q8->Q8Q9.");
    }

    {  // Slope45 segment Q9Q8->Q8Q5.
      v0 = gstVertex(120., 160., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9Q8->Q8Q5.");
    }

    {  // Slope45 segment Q5Q8->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 180., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q5Q8->Q9.");
    }

    {  // Slope45 segment Q9->Q8Q5.
      v0 = gstVertex(140., 180., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q8Q5.");
    }
  }

  {
    // Left end point lies in Q8.
    // Right end point lies in Q8,Q9.
    x1 = 100.;
    y1 = 140.;
    {  // Slope45 segment Q8->Q8 up.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(115., 155., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8->Q8 up.");
    }

    {  // Slope45 segment Q8->Q8 down.
      v0 = gstVertex(115., 155., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8->Q8 down.");
    }

    {  // Slope45 segment Q8->Q8Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(120., 160., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8->Q8Q9.");
    }

    {  // Slope45 segment Q9Q8->Q8.
      v0 = gstVertex(120., 160., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9Q8->Q8.");
    }

    {  // Slope45 segment Q8->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 180., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8->Q9.");
    }

    {  // Slope45 segment Q9->Q8.
      v0 = gstVertex(140., 180., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q8.");
    }
  }

  {
    // Left end point lies on boundary Q8Q9.
    // Right end point lies in Q9.
    x1 = 120.;
    y1 = 160.;
    {  // Slope45 segment Q8Q9->Q9.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 180., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8Q9->Q9.");
    }

    {  // Slope45 segment Q9->Q9Q8.
      v0 = gstVertex(140., 180., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q9->Q9Q8.");
    }
  }
}


// Clip slope45 segment.
// Segment end points lie in Q4, Q7, Q8.
TEST_F(gstGeomUtilsClipSegmentTest, ClipSlope45SegmentQ4Q7Q8Test) {
  double wx1 = 40.;
  double wy1 = 40.;
  double wx2 = 120.;
  double wy2 = 120.;

  double x1, y1;

  gstVertex v0;
  gstVertex v1;
  gstVertexVector expected_intersections;

  {
    // left end point in Q4;
    // right end point in Q4, Q7, Q8;
    x1 = .0;
    y1 = 100.;
    {  // Slope45 segment Q4->Q4Q7.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(20., 120., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4->Q4Q7.");
    }

    {  // Slope45 segment Q7Q4->Q4.
      v0 = gstVertex(20., 120., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q7Q4->Q4.");
    }

    {  // Slope45 segment Q4->Q7.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(30., 130., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4->Q7.");
    }

    {  // Slope45 segment Q7->Q4.
      v0 = gstVertex(30., 130., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q7->Q4.");
    }

    {  // Slope45 segment Q4->Q7Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(40., 140., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4->Q7Q8.");
    }

    {  // Slope45 segment Q8Q7->Q4.
      v0 = gstVertex(40., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8Q7->Q4.");
    }

    {  // Slope45 segment Q4->Q7->Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(60., 160., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4->Q7->Q8.");
    }

    {  // Slope45 segment Q8->Q7->Q4.
      v0 = gstVertex(60., 160., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8->Q7->Q4.");
    }
  }

  {
    // left end point on Q4Q7;
    // right end point in Q7, Q8;
    x1 = 20.;
    y1 = 120.;
    {  // Slope45 segment Q4Q7->Q7.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(30., 130., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4Q7->Q7.");
    }

    {  // Slope45 segment Q7->Q7Q4.
      v0 = gstVertex(30., 130., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q7->Q7Q4.");
    }

    {  // Slope45 segment Q4Q7->Q7Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(40., 140., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4Q7->Q7Q8.");
    }

    {  // Slope45 segment Q8Q7->Q7Q4.
      v0 = gstVertex(40., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8Q7->Q7Q4.");
    }

    {  // Slope45 segment Q4Q7->Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(60., 160., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4Q7->Q8.");
    }

    {  // Slope45 segment Q8->Q7Q4.
      v0 = gstVertex(60., 160., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8->Q7Q4.");
    }
  }

  {
    // left end point in Q7,
    // right end point in Q7, Q8
    x1 = 30.;
    y1 = 130;
    {  // Slope45 segment Q7->Q7Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(40., 140., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q7->Q7Q8.");
    }

    {  // Slope45 segment Q8Q7->Q7.
      v0 = gstVertex(40., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8Q7->Q7.");
    }

    {  // Slope45 segment Q7->Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(60.0, 160., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q7->Q8.");
    }

    {  // Slope45 segment Q8->Q7.
      v0 = gstVertex(60.0, 160., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8->Q7.");
    }
  }

  {
    // left end point on Q7Q8;
    // right end point in Q8;
    x1 = 40.;
    y1 = 140.;
    {  // Slope45 segment Q7Q8->Q8.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(60., 160., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q7Q8->Q8.");
    }

    {  // Slope45 segment Q8->Q8Q7.
      v0 = gstVertex(60., 160., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8->Q8Q7.");
    }
  }
}

// Clip Slope45 segment Q4Q8 (diagonal)
TEST_F(gstGeomUtilsClipSegmentTest, ClipSlope45SegmentQ4Q8Test) {
  double wx1 = 40.;
  double wy1 = 40.;
  double wx2 = 120.;
  double wy2 = 120.;

  double x1, y1;

  gstVertex v0;
  gstVertex v1;
  gstVertexVector expected_intersections;

  {
    // left end point in Q4;
    // right end point in Q4, Q8
    x1 = 20.;
    y1 = 100.;

    {  // Slope45 segment Q4->Q4Q8 touches corner.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(40., 120., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4->Q4Q8 touches corner.");
    }

    {  // Slope45 segment Q8Q4->Q4 touches corner.
      v0 = gstVertex(40., 120., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8Q4->Q4 touches corner.");
    }

    {  // Slope45 segment Q4->Q8 intersects corner.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(60., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 120., .0));
      expected_intersections.push_back(gstVertex(40., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4->Q8 intersects corner.");
    }

    {  // Slope45 segment Q8->Q4 intersects corner.
      v0 = gstVertex(60., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 120., .0));
      expected_intersections.push_back(gstVertex(40., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8->Q4 intersects corner.");
    }
  }

  {
    // left end point on Q4Q8;
    // right end point in Q8;
    x1 = 40.;
    y1 = 120.;
    {  // Slope45 segment Q4Q8->Q8 touches corner.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(60., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q4Q8->Q8 touches corner.");
    }

    {  // Slope45 segment Q8->Q8Q4 touches corner.
      v0 = gstVertex(60., 140., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope45 segment Q8->Q8Q4 touches corner.");
    }
  }
}

// Clip slope135 segment.
TEST_F(gstGeomUtilsClipSegmentTest, ClipSlope135SegmentTest) {
  double wx1 = 40.;
  double wy1 = 40.;
  double wx2 = 120.;
  double wy2 = 120.;

  double x1, y1;

  gstVertex v0;
  gstVertex v1;
  gstVertexVector expected_intersections;

  {
    x1 = .0;
    y1 = 60.;
    {  // Slope135 segment Q4->Q1->Q2.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(60., .0, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q4->Q1->Q2.");
    }

    {  // Slope135 segment Q2->Q1->Q4.
      v0 = gstVertex(60., .0, .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q2->Q1->Q4.");
    }
  }

  {
    x1 = 20.;
    y1 = 60.;
    {  // Slope135 segment Q4->Q4Q2 corner touch.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(40., 40., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q4->Q4Q2.");
    }

    {  // Slope135 segment Q2Q4->Q4 corner touch.
      v0 = gstVertex(40., 40., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q2Q4->Q4.");
    }

    {  // Slope135 segment Q4->Q2 diagonal.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(60., 20., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 40., .0));
      expected_intersections.push_back(gstVertex(40., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q4->Q2.");
    }

    {  // Slope135 segment Q2->Q4.
      v0 = gstVertex(60., 20., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 40., .0));
      expected_intersections.push_back(gstVertex(40., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q2->Q4.");
    }
  }

  {
    x1 = -20.;
    y1 = 140;
    {  // Slope135 segment Q7->Q4->Q3.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., -20., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q4->Q1->Q2.");
    }

    {  // Slope135 segment Q7->Q4->Q3.
      v0 = gstVertex(140., -20., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q2->Q1->Q4.");
    }
  }

  {
    x1 = 20.;
    y1 = 100.;

    {  // Slope135 segment Q4->Q4Q5.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(40., 80., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q4->Q4Q5.");
    }

    {  // Slope135 segment Q5Q4->Q4.
      v0 = gstVertex(40., 80., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q5Q4->Q4.");
    }

    {  // Slope135 segment Q4->Q5->Q2.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(100., 20., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q4->Q5->Q2.");
    }

    {  // Slope135 segment Q2->Q5->Q4.
      v0 = gstVertex(100., 20., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q2->Q5->Q4.");
    }
  }

  {
    x1 = 40.;
    y1 = 80.;
    {  // Slope135 segment Q4Q5->Q5Q2.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(80., 40., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q4Q5->Q5Q2.");
    }

    {  // Slope135 segment Q2Q5->Q5Q4.
      v0 = gstVertex(80., 40., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q2Q5->Q5Q4.");
    }
  }

  {
    x1 = 100.;
    y1 = 100.;
    {  // Slope135 segment Q5->Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 60., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(100., 100., .0));
      expected_intersections.push_back(gstVertex(120., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q5->Q6.");
    }

    {  // Slope135 segment Q6->Q5.
      v0 = gstVertex(140., 60., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 80., .0));
      expected_intersections.push_back(gstVertex(100., 100., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q6->Q5.");
    }
  }

  {
    x1 = 120.;
    y1 = 80.;
    {  // Slope135 segment Q5Q6->Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 60., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q5Q6->Q6.");
    }

    {  // Slope135 segment Q6->Q6Q5.
      v0 = gstVertex(140., 60., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q6->Q6Q5.");
    }
  }

  {
    x1 = 100.;
    y1 = 140.;
    {  // Slope135 segment Q8->Q6 diagonal.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(140., 100., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 120., .0));
      expected_intersections.push_back(gstVertex(120., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q8->Q6.");
    }

    {  // Slope135 segment Q6->Q8 diagonal.
      v0 = gstVertex(140., 100., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 120., .0));
      expected_intersections.push_back(gstVertex(120., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q6->Q8.");
    }
  }

  {
    x1 = 100.;
    y1 = 160.;
    {  // Slope135 segment Q8->Q9->Q6.
      v0 = gstVertex(x1, y1, .0);
      v1 = gstVertex(160., 100., .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q8->Q9->Q6.");
    }

    {  // Slope135 segment Q6->Q9->Q8.
      v0 = gstVertex(160., 100., .0);
      v1 = gstVertex(x1, y1, .0);

      expected_intersections.clear();

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "Slope135 segment Q6->Q9->Q8.");
    }
  }
}

// Clip SlopeAny segment.
TEST_F(gstGeomUtilsClipSegmentTest, ClipSlopeAnySegmentTest) {
  double wx1 = 40.;
  double wy1 = 40.;
  double wx2 = 120.;
  double wy2 = 120.;

  //  double x1, y1;

  gstVertex v0;
  gstVertex v1;
  gstVertexVector expected_intersections;

  {
    {  // SlopeAny segment Q1->Q2->Q5->Q6.
      v0 = gstVertex(20., 20., .0);
      v1 = gstVertex(140., 60, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(120., (40 + (40./3.)), .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "SlopeAny segment Q2->Q5->Q6.");
    }

    {  // SlopeAny segment Q6->Q5->Q2->Q1.
      v0 = gstVertex(140., 60, .0);
      v1 = gstVertex(20., 20., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., (40 + (40./3.)), .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "SlopeAny segment Q6->Q5->Q2->Q1.");
    }
  }

  {
    {  // SlopeAny segment Q4->Q5->Q2->Q3.
      v0 = gstVertex(20., 60., .0);
      v1 = gstVertex(140., 20, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., (40 + (40./3.)), .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));


      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "SlopeAny segment Q2->Q5->Q6.");
    }

    {  // SlopeAny segment Q3->Q2->Q5->Q4.
      v0 = gstVertex(140., 20, .0);
      v1 = gstVertex(20., 60., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(40., (40 + (40./3.)), .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "SlopeAny segment Q3->Q2->Q5->Q4.");
    }
  }

  {
    {  // SlopeAny segment Q1->Q4->Q5->Q8.
      v0 = gstVertex(20., 20., .0);
      v1 = gstVertex(60., 140, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex((20. + (100./3.)), 120., .0));


      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "SlopeAny segment Q2->Q5->Q6.");
    }

    {  // SlopeAny segment Q8->Q5->Q4->Q1.
      v0 = gstVertex(60., 140, .0);
      v1 = gstVertex(20., 20., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex((20. + (100./3.)), 120., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "SlopeAny segment Q8->Q5->Q4->Q1.");
    }
  }

  {
    {  // SlopeAny segment Q7->Q4->Q5->Q2.
      v0 = gstVertex(20., 140., .0);
      v1 = gstVertex(60., 20, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 80., .0));
      expected_intersections.push_back(gstVertex((20. + (100./3.)), 40., .0));


      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "SlopeAny segment Q7->Q4->Q5->Q2.");
    }

    {  // SlopeAny segment Q2->Q5->Q4->Q7.
      v0 = gstVertex(60., 20, .0);
      v1 = gstVertex(20., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex((20. + (100./3.)), 40., .0));
      expected_intersections.push_back(gstVertex(40., 80., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "SlopeAny segment Q2->Q5->Q4->Q7.");
    }
  }

  {
    {  // SlopeAny segment Q2->Q5->Q6.
      v0 = gstVertex(70., 10., .0);
      v1 = gstVertex(130., 100, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(90., 40., .0));
      expected_intersections.push_back(gstVertex(120., 85., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "SlopeAny segment Q2->Q5->Q6.");
    }

    {  // SlopeAny segment Q6->Q5->Q2.
      v0 = gstVertex(130., 100, .0);
      v1 = gstVertex(70., 10., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(120., 85., .0));
      expected_intersections.push_back(gstVertex(90., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "SlopeAny segment Q6->Q5->Q2.");
    }
  }

  {
    {  // SlopeAny segment Q7->Q5Q2.
      v0 = gstVertex(30., 140., .0);
      v1 = gstVertex(80., 40, .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(40., 120., .0));
      expected_intersections.push_back(gstVertex(80., 40., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "SlopeAny segment Q2->Q5->Q6.");
    }

    {  // SlopeAny segment Q2Q5->Q7.
      v0 = gstVertex(80., 40, .0);
      v1 = gstVertex(30., 140., .0);

      expected_intersections.clear();
      expected_intersections.push_back(gstVertex(80., 40., .0));
      expected_intersections.push_back(gstVertex(40., 120., .0));

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "SlopeAny segment Q2Q5->Q7.");
    }
  }

  {
    {  // SlopeAny segment Q7->Q8Q5.
      v0 = gstVertex(20., 180., .0);
      v1 = gstVertex(60., 120, .0);

      expected_intersections.clear();
      expected_intersections.push_back(v1);
      expected_intersections.push_back(v1);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "SlopeAny segment Q7->Q8Q5.");
    }

    {  // SlopeAny segment Q5Q8->Q7.
      v0 = gstVertex(60., 120, .0);
      v1 = gstVertex(20., 180., .0);

      expected_intersections.clear();
      expected_intersections.push_back(v0);
      expected_intersections.push_back(v0);

      CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                           "SlopeAny segment Q5Q8->Q7.");
    }
  }
}

// Clip SlopeAny segment generic test.
TEST_F(gstGeomUtilsClipSegmentTest, ClipSlopeAnySegmentGenericTest) {
  double wx1 = 0.164062;
  double wy1 = 0.605469;
  double wx2 = 1.164062;
  double wy2 = 1.605469;

  gstVertex v0;
  gstVertex v1;
  gstVertexVector expected_intersections;

  {
    v0 = gstVertex(0.163954, 0.605469, .0);
    v1 = gstVertex(0.165194, 0.605806, .0);

    expected_intersections.clear();
    double dx = v1.x - v0.x;
    double dy = v1.y - v0.y;


    expected_intersections.push_back(
        gstVertex(wx1,
                  (v0.y + (wx1 - v0.x)*dy/dx), .0));
    expected_intersections.push_back(v1);

    CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                         "SlopeAny segment generic Q1->Q4->Q5.");
  }

  {
    v0 = gstVertex(0.165194, 0.605806, .0);
    v1 = gstVertex(0.163954, 0.605469, .0);

    expected_intersections.clear();
    double dx = v1.x - v0.x;
    double dy = v1.y - v0.y;

    expected_intersections.push_back(v0);
    expected_intersections.push_back(
        gstVertex(wx1,
                  (v0.y + (wx1 - v0.x)*dy/dx), .0));


    CheckSegmentClipping(v0, v1, wx1, wx2, wy1, wy2, expected_intersections,
                         "SlopeAny segment generic Q5->Q4->Q1.");
  }
}

}  // namespace fusion_gst

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

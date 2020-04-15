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


// Unit tests for PolygonClipper2.

// 1) DPXYLexicographicLess...Test - unit tests for
// PcHalfedgeDPXYLexicographicLess function object.
// Quadrant codes for halfedge notation:
//         Y|
//     Q2   |   Q1
//          |
//   -------|-------X
//         O|
//     Q3   |   Q4
//          |
//
// Q1 - halfedge with "dominating" point (first end point) in the origin
// and second end point in quadrant Q1;
//
// 2) HolesCuttingTest - unit tests for holes cutting functionality.
//
// 3) ComputeCycleTest - unit tests for compute cycle functionality.
//
// 4) ClippingTest - unit tests for clipping functionality.


#include <string>
#include <functional>
#include <cmath>

#include <gtest/gtest.h>

#include "fusion/gst/gstPolygonClipper2.h"
#include "fusion/gst/gstVertex.h"
#include "fusion/gst/gstGeode.h"
#include "fusion/gst/gstMathUtils.h"
#include "fusion/gst/gstUnitTestUtils.h"
#include "common/khTileAddr.h"

// TODO: add unit tests for clipping w/o holes cutting.

// Note: it is used for debugging.
// #define REPORT_POLYGON 1

namespace fusion_gst {

class PolygonClipper2Test : public testing::Test {
 public:
};

// Utility method for checking the results of PcHalfedgeDPXYLexicographicLess
// function object (generic case).
// v0, v1 - first segment end points;
// v2, v3 - second segment end points;
// expected_result - the expected compare result;
// message - the error message for failures to help identify the
// failed test.
void CheckDPXYLexicographicLess(const gstVertex &v0,
                                const gstVertex &v1,
                                const gstVertex &v2,
                                const gstVertex &v3,
                                const bool expected_result,
                                const std::string &message) {
  PcHalfedgeDPXYLexicographicLess less;
  PcHalfedgeHandle he1;
  PcHalfedgeHandle he2;

  he1 = PcHalfedge::CreateInternalPair(v0, v1,
                                       kInsideAreaLocNone, kNormalEdge);
  he2 = PcHalfedge::CreateInternalPair(v2, v3,
                                       kInsideAreaLocNone, kNormalEdge);

  bool result = less(he1, he2);
  ASSERT_EQ(expected_result, result) << message;
}

// Utility method for checking the results of PcHalfedgeDPXYLexicographicLess
// function object. The input halfedges have common "dominating" point.
// v0, v1 - first segment end points;
// v0, v2 - second segment end points;
// expected_result - the expected compare result;
// message - the error message for failures to help identify the
// failed test.
void CheckDPXYLexicographicLessCommonDP(const gstVertex &v0,
                                        const gstVertex &v1,
                                        const gstVertex &v2,
                                        const bool expected_result,
                                        const std::string &message) {
  PcHalfedgeDPXYLexicographicLess less;
  PcHalfedgeHandle he1;
  PcHalfedgeHandle he2;

  he1 = PcHalfedge::CreateInternalPair(v0, v1,
                                       kInsideAreaLocNone, kNormalEdge);
  he2 = PcHalfedge::CreateInternalPair(v0, v2,
                                       kInsideAreaLocNone, kNormalEdge);

  bool result = less(he1, he2);
  ASSERT_EQ(expected_result, result) << message;
}

// Utility method for checking Polygon Clipping.
// geode - source polygon;
// quad - clippimng rectangle;
// expected_geodes - the expected geodes;
// message - the error message for failures to help identify the
// failed test.
void CheckPolygonClipping(const gstGeodeHandle &geode,
                          const gstBBox &quad,
                          const GeodeList &expected_geodes,
                          const std::string &message,
                          const bool denormalize = false) {
  PolygonClipper clipper(quad, true);  // cut_holes is true.
  GeodeList pieces;

  clipper.Run(geode, &pieces);

#if REPORT_POLYGON
  notify(NFY_NOTICE, "number of pieces: %Zu", pieces.size());
  for (unsigned int i = 0; i < pieces.size(); ++i) {
    const gstGeode* piece =
        static_cast<const gstGeode*>(&(*pieces[i]));
    notify(NFY_NOTICE, "piece[%u] degenerate: %d", i, piece->IsDegenerate());
    if (piece->IsDegenerate())
      continue;

    assert(piece->NumParts() == 1);
    const std::vector<std::int8_t>& edge_flags = piece->EdgeFlags();
    assert(edge_flags.size() == piece->VertexCount(0));

    for (unsigned int j = 0; j < piece->VertexCount(0) ; ++j) {
      const gstVertex &pt = piece->GetVertex(0, j);
      notify(NFY_NOTICE, "pt[%u]: %f, %f, is_internal: %d, edge flag: %d",
             j,
             denormalize ? khTilespaceBase::Denormalize(pt.x) : pt.x,
             denormalize ? khTilespaceBase::Denormalize(pt.y) : pt.y,
             piece->IsInternalVertex(j),
             edge_flags[j]);
    }
  }

  notify(NFY_NOTICE, "number of expected geodes: %Zu", expected_geodes.size());
  for (unsigned int i = 0; i < expected_geodes.size(); ++i) {
    const gstGeode* exp_geode =
        static_cast<const gstGeode*>(&(*expected_geodes[i]));
    notify(NFY_NOTICE, "exp_geode[%u] degenerate: %d",
           i, exp_geode->IsDegenerate());
    if (exp_geode->IsDegenerate())
      continue;

    assert(exp_geode->NumParts() == 1);
    const std::vector<std::int8_t>& edge_flags = exp_geode->EdgeFlags();
    assert(edge_flags.size() == exp_geode->VertexCount(0));

    for (unsigned int j = 0; j < exp_geode->VertexCount(0) ; ++j) {
      const gstVertex &pt = exp_geode->GetVertex(0, j);
      notify(NFY_NOTICE, "exp_pt[%u]: %f, %f, is_internal: %d, edge flag: %d",
             j,
             denormalize ? khTilespaceBase::Denormalize(pt.x) : pt.x,
             denormalize ? khTilespaceBase::Denormalize(pt.y) : pt.y,
             exp_geode->IsInternalVertex(j),
             edge_flags[j]);
    }
  }
#endif

  EXPECT_EQ(expected_geodes.size(), pieces.size()) << message;

  for (unsigned int i = 0; (i < expected_geodes.size()) && (i <  pieces.size()); ++i) {
    EXPECT_EQ(expected_geodes[i]->NumParts(), (unsigned int)1) << message;
    EXPECT_EQ(pieces[i]->NumParts(), (unsigned int)1) << message;

    EXPECT_EQ(expected_geodes[i]->TotalVertexCount(),
              pieces[i]->TotalVertexCount()) << message;

    const gstGeode* exp_geode =
        static_cast<const gstGeode*>(&(*expected_geodes[i]));
    const gstGeode* piece =
        static_cast<const gstGeode*>(&(*pieces[i]));

    const std::vector<std::int8_t>& piece_edge_flags = piece->EdgeFlags();
    const std::vector<std::int8_t>& exp_geode_edge_flags = exp_geode->EdgeFlags();
    ASSERT_EQ(piece_edge_flags.size(), piece->VertexCount(0)) << message;
    ASSERT_EQ(exp_geode_edge_flags.size(),
              exp_geode->VertexCount(0)) << message;

    for (unsigned int j = 0; (j < exp_geode->VertexCount(0)) &&
             (j < piece->VertexCount(0)) ; ++j) {
      const gstVertex &expected_pt = exp_geode->GetVertex(0, j);
      const gstVertex &pt = piece->GetVertex(0, j);

#if REPORT_POLYGON
      notify(NFY_NOTICE, "pt[%u]: %f, %f, is_internal: %d",
             j,
             denormalize ? khTilespaceBase::Denormalize(pt.x) : pt.x,
             denormalize ? khTilespaceBase::Denormalize(pt.y) : pt.y,
             piece->IsInternalVertex(j));
#endif
      EXPECT_EQ(exp_geode->IsInternalVertex(j),
                piece->IsInternalVertex(j)) << "j=" << j << ". "<< message;
      EXPECT_EQ(exp_geode_edge_flags[j],
                piece_edge_flags[j]) << "j=" << j << ". " << message;

      if (denormalize) {
        gstVertex denorm_exp_pt =
            gstVertex(khTilespaceBase::Denormalize(expected_pt.x),
                      khTilespaceBase::Denormalize(expected_pt.y),
                      expected_pt.z);
        gstVertex denorm_pt = gstVertex(khTilespaceBase::Denormalize(pt.x),
                                        khTilespaceBase::Denormalize(pt.y),
                                        pt.z);
        EXPECT_PRED2(TestUtils::gstVertexEqualXY,
                     denorm_exp_pt, denorm_pt) << message;
      } else {
        EXPECT_PRED2(TestUtils::gstVertexEqualXY, expected_pt, pt) << message;
      }
    }
  }
}


// Generic tests. Halfedges do not have common "dominating" point.
TEST_F(PolygonClipper2Test, DPXYLexicographicLessGenericTest) {
  gstVertex v0;
  gstVertex v1;
  gstVertex v2;
  gstVertex v3;

  {
    // Both halfedges are left. "Dominating" points have different X,Y
    // coordinates.
    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(100., 50., .0);
      v2 = gstVertex(100., 50., .0);
      v3 = gstVertex(200., 75., .0);

      CheckDPXYLexicographicLess(
          v0, v1 , v2, v3, true,
          "Generic test 1. he1 < he2");
    }

    {
      v0 = gstVertex(100., 50., .0);
      v1 = gstVertex(200., 75., .0);
      v2 = gstVertex(.0, .0, .0);
      v3 = gstVertex(100., 50., .0);

      CheckDPXYLexicographicLess(
          v0, v1 , v2, v3, false,
          "Generic test 2. he2 < he1.");
    }
  }

  {
    // One halfedge is left, another one is right. "Dominating" points have
    // different X,Y- coordinates.
    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(100., 50., .0);
      v2 = gstVertex(100., 50., .0);
      v3 = gstVertex(-100., 75., .0);

      CheckDPXYLexicographicLess(
          v0, v1 , v2, v3, true,
          "Generic test 3. he1 < he2");
    }

    {
      v0 = gstVertex(100., 50., .0);
      v1 = gstVertex(-100., 75., .0);
      v2 = gstVertex(.0, .0, .0);
      v3 = gstVertex(100., 50., .0);

      CheckDPXYLexicographicLess(
          v0, v1 , v2, v3, false,
          "Generic test 4. he2 < he1.");
    }
  }

  {
    // Both halfedges are left, "dominating" points have equal X-coordinates.
    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(100., 50., .0);
      v2 = gstVertex(.0, 20., .0);
      v3 = gstVertex(100., 75., .0);
      CheckDPXYLexicographicLess(
          v0, v1 , v2, v3, true,
          "Generic test 5. he1 < he2");
    }

    {
      v0 = gstVertex(.0, 20., .0);
      v1 = gstVertex(100., 75., .0);
      v2 = gstVertex(.0, .0, .0);
      v3 = gstVertex(100., 50., .0);


      CheckDPXYLexicographicLess(
          v0, v1 , v2, v3, false,
          "Generic test 6. he2 < he1.");
    }
  }

  {
    // Both halfedges are left, "dominating" points have equal Y-coordinates.
    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(100., 50., .0);
      v2 = gstVertex(20., .0, .0);
      v3 = gstVertex(100., 75., .0);
      CheckDPXYLexicographicLess(
          v0, v1 , v2, v3, true,
          "Generic test 7. he1 < he2");
    }

    {
      v0 = gstVertex(20., .0, .0);
      v1 = gstVertex(100., 75., .0);
      v2 = gstVertex(.0, .0, .0);
      v3 = gstVertex(100., 50., .0);

      CheckDPXYLexicographicLess(
          v0, v1 , v2, v3, false,
          "Generic test 8. he2 < he1.");
    }
  }
}

// Test cases where halfedges have common "dominating" point.
// One halfedge is located in Q1, another one in Q1, Q2, Q3, Q4 and vice versa.
TEST_F(PolygonClipper2Test, DPXYLexicographicLessQ1Test) {
  gstVertex v0;
  gstVertex v1;
  gstVertex v2;

  {
    // Both halfedges are located in Q1.
    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(100., 50., .0);
      v2 = gstVertex(100., 75., .0);
      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, true,
          "Both halfedges are in Q1. he1 < he2");
    }

    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(100., 75., .0);
      v2 = gstVertex(100., 50., .0);

      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, false,
          "Both halfedges are in Q1. he2 < he1.");
    }
  }

  {
    // One halfedge is located in Q1, another one in Q2.
    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(100., 50., .0);
      v2 = gstVertex(-100., 75., .0);
      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, false,
          "First halfedge in Q1, second one in Q2. he1 < he2");
    }

    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(-100., 75., .0);
      v2 = gstVertex(100., 50., .0);

      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, true,
          "First halfedge in Q2, second one in Q1. he2 < he1.");
    }
  }

  {
    // One halfedge is located in Q1, another one in Q3.
    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(100., 50., .0);
      v2 = gstVertex(-100., -75., .0);
      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, false,
          "First halfedge in Q1, second one in Q3.");
    }

    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(-100., -75., .0);
      v2 = gstVertex(100., 50., .0);

      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, true,
          "First halfedge in Q3, second one in Q1.");
    }
  }

  {
    // One halfedge is located in Q1, another one in Q4.
    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(100., 50., .0);
      v2 = gstVertex(100., -75., .0);
      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, false,
          "First halfedge is located in Q1, second one in Q4.");
    }

    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(100., -75., .0);
      v2 = gstVertex(100., 50., .0);

      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, true,
          "First halfedge is located in Q4, second one in Q1.");
    }
  }
}

// Test cases where halfedges have common "dominating" point.
// One halfedge is located in Q2, another one in Q2, Q3, Q4 and vice versa.
TEST_F(PolygonClipper2Test, DPXYLexicographicLessQ2Test) {
  gstVertex v0;
  gstVertex v1;
  gstVertex v2;

  {
    // Both halfedges are located in Q2.
    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(-100., 75., .0);
      v2 = gstVertex(-100., 50., .0);

      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, true,
          "Both halfedges in Q2. he1 < he2");
    }

    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(-100., 50., .0);
      v2 = gstVertex(-100., 75., .0);

      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, false,
          "Both halfedges in Q2. he2 < he1.");
    }
  }

  {
    // One halfedge is located in Q2, another one in Q3.
    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(-100., 50., .0);
      v2 = gstVertex(-100., -75., .0);
      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, true,
          "First halfedge in Q2, second one in Q3.");
    }

    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(-100., -75., .0);
      v2 = gstVertex(-100., 50., .0);

      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, false,
          "First halfedge in Q3, second one in Q2.");
    }
  }

  {
    // One halfedge is located in Q2, another one in Q4.
    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(-50., 100., .0);
      v2 = gstVertex(100., -50., .0);
      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, true,
          "First halfedge in Q2, second one in Q4.");
    }

    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(100., -50., .0);
      v2 = gstVertex(-50., 100., .0);

      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, false,
          "First halfedge in Q4, second one in Q2.");
    }
  }
}

// Test cases where halfedges have common "dominating" point.
// One halfedge is located in Q3, another one in Q3, Q4 and vice versa.
TEST_F(PolygonClipper2Test, DPXYLexicographicLessQ3Test) {
  gstVertex v0;
  gstVertex v1;
  gstVertex v2;

  {
    // Both halfedges are located in Q3.
    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(-100., -50., .0);
      v2 = gstVertex(-100., -75., .0);

      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, true,
          "Both halfedges in Q3. he1 < he2");
    }

    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(-100., -75., .0);
      v2 = gstVertex(-100., -50., .0);

      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, false,
          "Both halfedges in Q3. he2 < he1.");
    }
  }

  {
    // One halfedge is located in Q3, another one in Q4.
    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(-100., -50., .0);
      v2 = gstVertex(100., -75., .0);
      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, true,
          "First halfedge in Q3, second one in Q4.");
    }

    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(100., -75., .0);
      v2 = gstVertex(-100., 50., .0);

      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, false,
          "First halfedge in Q4, second one in Q3.");
    }
  }
}

// Test cases where halfedges have common "dominating" point.
// Both halfedges are located in Q4.
TEST_F(PolygonClipper2Test, DPXYLexicographicLessQ4Test) {
  gstVertex v0;
  gstVertex v1;
  gstVertex v2;

  {
    // Both halfedges are located in Q4.
    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(100., -75., .0);
      v2 = gstVertex(100., -50., .0);

      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, true,
          "Both halfedges in Q4. he1 < he2");
    }

    {
      v0 = gstVertex(.0, .0, .0);
      v1 = gstVertex(100., -50., .0);
      v2 = gstVertex(100., -75., .0);

      CheckDPXYLexicographicLessCommonDP(
          v0, v1 , v2, false,
          "Both halfedges in Q4. he2 < he1.");
    }
  }
}

// Tests holes cutting functionality. Polygon lies completely inside quad.
// No clipping.
TEST_F(PolygonClipper2Test, HolesCuttingTest) {
  gstBBox quad(-20, 1000, -20, 1000);
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  GeodeList expected_geodes;

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  {
    // Polygon lies completely inside quad. Cut to edge.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(5);
    geode->AddVertex(gstVertex(100., 100., .0));
    geode->AddVertex(gstVertex(220., 100., .0));
    geode->AddVertex(gstVertex(220., 200., .0));
    geode->AddVertex(gstVertex(100., 200., .0));
    geode->AddVertex(gstVertex(100., 100., .0));
    geode->AddPart(5);
    geode->AddVertex(gstVertex(120., 120., .0));
    geode->AddVertex(gstVertex(120., 180., .0));
    geode->AddVertex(gstVertex(200., 180., .0));
    geode->AddVertex(gstVertex(200., 120., .0));
    geode->AddVertex(gstVertex(120., 120., .0));

    exp_geode->Clear();
    exp_geode->AddPart(12);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kCutEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 120., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 120., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 120., .0), kCutEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(220., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(220., 200., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 200., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 100., .0), kNormalEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(geodeh, quad, expected_geodes,
                         "Polygon is completely inside quad. Cut to edge.");
  }

  {
    // Polygon lies completely inside quad. Cut to vertex.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(5);
    geode->AddVertex(gstVertex(120., 100., .0));
    geode->AddVertex(gstVertex(220., 100., .0));
    geode->AddVertex(gstVertex(220., 200., .0));
    geode->AddVertex(gstVertex(100., 200., .0));
    geode->AddVertex(gstVertex(120., 100., .0));
    geode->AddPart(5);
    geode->AddVertex(gstVertex(120., 120., .0));
    geode->AddVertex(gstVertex(120., 180., .0));
    geode->AddVertex(gstVertex(200., 180., .0));
    geode->AddVertex(gstVertex(200., 120., .0));
    geode->AddVertex(gstVertex(120., 120., .0));

    exp_geode->Clear();
    exp_geode->AddPart(11);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 200., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kCutEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 120., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 120., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 120., .0), kCutEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(220., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(220., 200., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 200., .0), kNormalEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(geodeh, quad, expected_geodes,
                         "Polygon is completely inside quad. Cut to vertex.");
  }

  {
    // Polygon lies completely inside quad. Cut to vertex.
    // Edge from outer boundary and cut-edge are collinear.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(6);
    geode->AddVertex(gstVertex(120., 100., .0));
    geode->AddVertex(gstVertex(220., 100., .0));
    geode->AddVertex(gstVertex(220., 200., .0));
    geode->AddVertex(gstVertex(100., 200., .0));
    geode->AddVertex(gstVertex(120., 120., .0));
    geode->AddVertex(gstVertex(120., 100., .0));
    geode->AddPart(5);
    geode->AddVertex(gstVertex(120., 140., .0));
    geode->AddVertex(gstVertex(120., 180., .0));
    geode->AddVertex(gstVertex(200., 180., .0));
    geode->AddVertex(gstVertex(200., 140., .0));
    geode->AddVertex(gstVertex(120., 140., .0));

    exp_geode->Clear();
    exp_geode->AddPart(12);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 200., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 120., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kCutEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 140., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 140., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 140., .0), kCutEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(220., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(220., 200., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 200., .0), kNormalEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon is completely inside quad. Cut to vertex."
        " Edge from outer boundary and cut edge are collinear.");
  }

  {
    // Polygon lies completely inside quad.
    // It has two holes that are vertically aligned.
    // Cut to vertex. Edge from outer boundary and cut edge are collinear.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(6);
    geode->AddVertex(gstVertex(120., 100., .0));
    geode->AddVertex(gstVertex(600., 100., .0));
    geode->AddVertex(gstVertex(600., 400., .0));
    geode->AddVertex(gstVertex(100., 400., .0));
    geode->AddVertex(gstVertex(120., 120., .0));
    geode->AddVertex(gstVertex(120., 100., .0));

    geode->AddPart(5);
    geode->AddVertex(gstVertex(120., 140., .0));
    geode->AddVertex(gstVertex(120., 160., .0));
    geode->AddVertex(gstVertex(160., 160., .0));
    geode->AddVertex(gstVertex(160., 140., .0));
    geode->AddVertex(gstVertex(120., 140., .0));

    geode->AddPart(5);
    geode->AddVertex(gstVertex(120., 180., .0));
    geode->AddVertex(gstVertex(120., 200., .0));
    geode->AddVertex(gstVertex(140., 200., .0));
    geode->AddVertex(gstVertex(140., 180., .0));
    geode->AddVertex(gstVertex(120., 180., .0));

    exp_geode->Clear();
    exp_geode->AddPart(18);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 120., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kCutEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 140., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 160., .0), kCutEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 200., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 200., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 180., .0), kCutEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 160., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(160., 160., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(160., 140., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 140., .0), kCutEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(600., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(600., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 400., .0), kNormalEdge);
    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon is completely inside quad."
        " Two holes are vertically aligned. Cut to vertex."
        " Edge from outer boundary and cut-edge are collinear.");
  }

  {
    // Polygon lies completely inside quad.
    // It has two holes: hole2 is shifted to right relative to hole1.
    // Cut to vertex. Edge from outer boundary and cut-edge are collinear.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(6);
    geode->AddVertex(gstVertex(120., 100., .0));
    geode->AddVertex(gstVertex(600., 100., .0));
    geode->AddVertex(gstVertex(600., 400., .0));
    geode->AddVertex(gstVertex(100., 400., .0));
    geode->AddVertex(gstVertex(120., 120., .0));
    geode->AddVertex(gstVertex(120., 100., .0));

    geode->AddPart(5);
    geode->AddVertex(gstVertex(120., 140., .0));
    geode->AddVertex(gstVertex(120., 160., .0));
    geode->AddVertex(gstVertex(160., 160., .0));
    geode->AddVertex(gstVertex(160., 140., .0));
    geode->AddVertex(gstVertex(120., 140., .0));

    geode->AddPart(5);
    geode->AddVertex(gstVertex(140., 180., .0));
    geode->AddVertex(gstVertex(140., 200., .0));
    geode->AddVertex(gstVertex(160., 200., .0));
    geode->AddVertex(gstVertex(160., 180., .0));
    geode->AddVertex(gstVertex(140., 180., .0));


    exp_geode->Clear();
    exp_geode->AddPart(19);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 120., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kCutEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 140., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 160., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 160., .0), kCutEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 200., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(160., 200., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(160., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 180., .0), kCutEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 160., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(160., 160., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(160., 140., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 140., .0), kCutEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(600., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(600., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 400., .0), kNormalEdge);
    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon is completely inside quad."
        " Two holes: H2 is shifted to right relative to H1. Cut to vertex."
        " Edge from outer boundary and cut-edge are collinear.");
  }

  {
    // Polygon lies completely inside quad.
    // Two holes: Hole1 is shifted to right relative to Hole2. Cut to edge.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(6);
    geode->AddVertex(gstVertex(120., 100., .0));
    geode->AddVertex(gstVertex(600., 100., .0));
    geode->AddVertex(gstVertex(600., 400., .0));
    geode->AddVertex(gstVertex(100., 400., .0));
    geode->AddVertex(gstVertex(120., 120., .0));
    geode->AddVertex(gstVertex(120., 100., .0));

    geode->AddPart(5);
    geode->AddVertex(gstVertex(160., 140., .0));
    geode->AddVertex(gstVertex(160., 160., .0));
    geode->AddVertex(gstVertex(240., 160., .0));
    geode->AddVertex(gstVertex(240., 140., .0));
    geode->AddVertex(gstVertex(160., 140., .0));

    geode->AddPart(5);
    geode->AddVertex(gstVertex(140., 180., .0));
    geode->AddVertex(gstVertex(140., 200., .0));
    geode->AddVertex(gstVertex(200., 200., .0));
    geode->AddVertex(gstVertex(200., 180., .0));
    geode->AddVertex(gstVertex(140., 180., .0));


    exp_geode->Clear();
    exp_geode->AddPart(20);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 120., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 100., .0), kCutEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 200., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 200., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 180., .0), kCutEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(160., 100., .0), kCutEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(160., 140., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(160., 160., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(240., 160., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(240., 140., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(160., 140., .0), kCutEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(160., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(600., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(600., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 400., .0), kNormalEdge);
    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon is completely inside quad."
        " Two holes: H1 is shifted to right relative to H2. Cut to edge.");
  }

  {
    // Polygon lies completely inside quad.
    // It has two holes: Hole1 touch Hole2 by vertex. Cut to edge.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(6);
    geode->AddVertex(gstVertex(120., 100., .0));
    geode->AddVertex(gstVertex(600., 100., .0));
    geode->AddVertex(gstVertex(600., 400., .0));
    geode->AddVertex(gstVertex(100., 400., .0));
    geode->AddVertex(gstVertex(120., 120., .0));
    geode->AddVertex(gstVertex(120., 100., .0));

    geode->AddPart(5);
    geode->AddVertex(gstVertex(200., 140., .0));
    geode->AddVertex(gstVertex(200., 180., .0));
    geode->AddVertex(gstVertex(240., 180., .0));
    geode->AddVertex(gstVertex(240., 140., .0));
    geode->AddVertex(gstVertex(200., 140., .0));

    geode->AddPart(5);
    geode->AddVertex(gstVertex(140., 180., .0));
    geode->AddVertex(gstVertex(140., 200., .0));
    geode->AddVertex(gstVertex(200., 200., .0));
    geode->AddVertex(gstVertex(200., 180., .0));
    geode->AddVertex(gstVertex(140., 180., .0));

    exp_geode->Clear();
    exp_geode->AddPart(17);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 120., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 100., .0), kCutEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 200., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 200., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 180., .0), kNormalEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(240., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(240., 140., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 140., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 180., .0), kNormalEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 180., .0), kCutEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(600., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(600., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 400., .0), kNormalEdge);
    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon is completely inside quad. Two holes: H1 touch H2 by vertex."
        " Cut to edge.");
  }
}

// Tests compute cycle functionality.
TEST_F(PolygonClipper2Test, ComputeCycleTest) {
  gstBBox quad(-20, 1000, -20, 1000);
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  GeodeList expected_geodes;

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  {
    // Polygon lies completely inside quad.
    // Outer cycle has sharing vertex. Cut to edge.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(5);
    geode->AddVertex(gstVertex(100., 100., .0));
    geode->AddVertex(gstVertex(600., 100., .0));
    geode->AddVertex(gstVertex(600., 400., .0));
    geode->AddVertex(gstVertex(500., 350., .0));
    geode->AddVertex(gstVertex(450., 300., .0));
    geode->AddVertex(gstVertex(400., 350., .0));
    geode->AddVertex(gstVertex(600., 400., .0));
    geode->AddVertex(gstVertex(100., 400., .0));
    geode->AddVertex(gstVertex(100., 100., .0));

    geode->AddPart(5);
    geode->AddVertex(gstVertex(120., 120., .0));
    geode->AddVertex(gstVertex(120., 180., .0));
    geode->AddVertex(gstVertex(200., 180., .0));
    geode->AddVertex(gstVertex(200., 120., .0));
    geode->AddVertex(gstVertex(120., 120., .0));

    exp_geode->Clear();
    exp_geode->AddPart(16);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kCutEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 120., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 120., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 120., .0), kCutEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(600., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(600., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(500., 350., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(450., 300., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(400., 350., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(600., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 100., .0), kNormalEdge);
    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(geodeh, quad, expected_geodes,
                         "Polygon is completely inside quad."
                         " Outer cycle has sharing vertex. Cut to edge.");
  }

  {
    // Polygon lies completely inside quad.
    // Outer and inner cycles have sharing vertex. Cut to edge."
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(5);
    geode->AddVertex(gstVertex(100., 100., .0));
    geode->AddVertex(gstVertex(600., 100., .0));
    geode->AddVertex(gstVertex(600., 400., .0));
    geode->AddVertex(gstVertex(500., 350., .0));
    geode->AddVertex(gstVertex(450., 300., .0));
    geode->AddVertex(gstVertex(400., 350., .0));
    geode->AddVertex(gstVertex(600., 400., .0));
    geode->AddVertex(gstVertex(100., 400., .0));
    geode->AddVertex(gstVertex(100., 100., .0));

    geode->AddPart(9);
    geode->AddVertex(gstVertex(120., 120., .0));
    geode->AddVertex(gstVertex(120., 180., .0));
    geode->AddVertex(gstVertex(130., 165., .0));
    geode->AddVertex(gstVertex(140., 160., .0));
    geode->AddVertex(gstVertex(130., 175., .0));
    geode->AddVertex(gstVertex(120., 180., .0));
    geode->AddVertex(gstVertex(200., 180., .0));
    geode->AddVertex(gstVertex(200., 120., .0));
    geode->AddVertex(gstVertex(120., 120., .0));

    exp_geode->Clear();
    exp_geode->AddPart(16);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kCutEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 120., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 120., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 120., .0), kCutEdge);

    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(600., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(600., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(500., 350., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(450., 300., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(400., 350., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(600., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 100., .0), kNormalEdge);
    expected_geodes.push_back(exp_geodeh);

    exp_geodeh.release();
    exp_geodeh = gstGeodeImpl::Create(gstPolygon);
    exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

    exp_geode->AddPart(5);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(130., 165., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(140., 160., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(130., 175., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 180., .0), kNormalEdge);
    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon is completely inside quad."
        " Outer/inner cycles have sharing vertex. Cut to edge.");
  }
}

// Tests polygon clipping functionality.
TEST_F(PolygonClipper2Test, ClippingGenericTest) {
  gstBBox quad(-20, 500, -20, 1000);
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  GeodeList expected_geodes;

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  {
    // Quad clips outer cycle of polygon by its right edge.
    // Hole is inside quad.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(5);
    geode->AddVertex(gstVertex(100., 100., .0));
    geode->AddVertex(gstVertex(600., 100., .0));
    geode->AddVertex(gstVertex(600., 400., .0));
    geode->AddVertex(gstVertex(100., 400., .0));
    geode->AddVertex(gstVertex(100., 100., .0));
    geode->AddPart(5);
    geode->AddVertex(gstVertex(120., 120., .0));
    geode->AddVertex(gstVertex(120., 180., .0));
    geode->AddVertex(gstVertex(200., 180., .0));
    geode->AddVertex(gstVertex(200., 120., .0));
    geode->AddVertex(gstVertex(120., 120., .0));

    exp_geode->Clear();
    exp_geode->AddPart(12);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kCutEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 120., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 120., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 120., .0), kCutEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(500., 100., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(500., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 100., .0), kNormalEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Quad clips outer cycle of polygon by its right edge."
        " Hole is inside quad.");
  }

  {
    // Quad clips outer cycle of polygon by its right edge.
    // Hole is outside quad.
    quad.init(220, 500, -20, 1000);
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(5);
    geode->AddVertex(gstVertex(100., 100., .0));
    geode->AddVertex(gstVertex(600., 100., .0));
    geode->AddVertex(gstVertex(600., 400., .0));
    geode->AddVertex(gstVertex(100., 400., .0));
    geode->AddVertex(gstVertex(100., 100., .0));
    geode->AddPart(5);
    geode->AddVertex(gstVertex(120., 120., .0));
    geode->AddVertex(gstVertex(120., 180., .0));
    geode->AddVertex(gstVertex(200., 180., .0));
    geode->AddVertex(gstVertex(200., 120., .0));
    geode->AddVertex(gstVertex(120., 120., .0));

    exp_geode->Clear();
    exp_geode->AddPart(5);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(220., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(500., 100., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(500., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(220., 400., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(220., 100., .0), kClipEdge);
    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Quad clips outer cycle of polygon by its right edge."
        " Hole is outside quad.");
  }

  {
    // Quad clips outer and inner cycle of polygon by its right edge.
    quad.init(-20, 180, -20, 1000);
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(5);
    geode->AddVertex(gstVertex(100., 100., .0));
    geode->AddVertex(gstVertex(600., 100., .0));
    geode->AddVertex(gstVertex(600., 400., .0));
    geode->AddVertex(gstVertex(100., 400., .0));
    geode->AddVertex(gstVertex(100., 100., .0));
    geode->AddPart(5);
    geode->AddVertex(gstVertex(120., 120., .0));
    geode->AddVertex(gstVertex(120., 180., .0));
    geode->AddVertex(gstVertex(200., 180., .0));
    geode->AddVertex(gstVertex(200., 120., .0));
    geode->AddVertex(gstVertex(120., 120., .0));

    exp_geode->Clear();
    exp_geode->AddPart(9);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 100., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(180., 100., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(180., 120., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 120., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(180., 180., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(180., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 400., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 100., .0), kNormalEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Quad clips outer and inner cycle of polygon by its right edge.");
  }

  {
    // Quad clips outer and inner cycle of polygon by its bottom and top edges.
    quad.init(-20, 800, 140, 180);
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(5);
    geode->AddVertex(gstVertex(100., 100., .0));
    geode->AddVertex(gstVertex(600., 100., .0));
    geode->AddVertex(gstVertex(600., 400., .0));
    geode->AddVertex(gstVertex(100., 400., .0));
    geode->AddVertex(gstVertex(100., 100., .0));
    geode->AddPart(5);
    geode->AddVertex(gstVertex(120., 120., .0));
    geode->AddVertex(gstVertex(120., 200., .0));
    geode->AddVertex(gstVertex(200., 200., .0));
    geode->AddVertex(gstVertex(200., 120., .0));
    geode->AddVertex(gstVertex(120., 120., .0));

    exp_geode->Clear();
    exp_geode->AddPart(5);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 140., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 140., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(120., 180., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 140., .0), kNormalEdge);
    expected_geodes.push_back(exp_geodeh);

    exp_geodeh.release();
    exp_geodeh = gstGeodeImpl::Create(gstPolygon);
    gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

    exp_geode->AddPart(5);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 140., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(600., 140., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(600., 180., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 140., .0), kNormalEdge);
    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(geodeh, quad, expected_geodes,
                         "Quad clips outer and inner cycle of polygon"
                         " by its bottom and top edges.");
  }
}

// Tests polygon clipping functionality: special cases when polygon's vertex
// lies on quad's edge and there is clipping.
TEST_F(PolygonClipper2Test, ClippingSpecialCasesVertexOnQuadEdgeTest) {
  gstBBox quad(-20, 500, -20, 1000);
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  GeodeList expected_geodes;

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  {
    // Polygon with vertex that lies on left quad edge.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(6);
    geode->AddVertex(gstVertex(-200., .0, .0));
    geode->AddVertex(gstVertex(200., .0, .0));
    geode->AddVertex(gstVertex(200., 500., .0));
    geode->AddVertex(gstVertex(-20., 700., .0));
    geode->AddVertex(gstVertex(-200., 500., .0));
    geode->AddVertex(gstVertex(-200., .0, .0));

    exp_geode->Clear();
    exp_geode->AddPart(5);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(-20., .0, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., .0, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(200., 500., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(-20., 700., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(-20., .0, .0), kClipEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon with vertex that lies on left quad edge.");
  }

  {
    // Polygon with vertex that lies on right quad edge.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(6);
    geode->AddVertex(gstVertex(300., .0, .0));
    geode->AddVertex(gstVertex(700., .0, .0));
    geode->AddVertex(gstVertex(700., 500., .0));
    geode->AddVertex(gstVertex(500., 700., .0));
    geode->AddVertex(gstVertex(300., 500., .0));
    geode->AddVertex(gstVertex(300., .0, .0));

    exp_geode->Clear();
    exp_geode->AddPart(5);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(300., .0, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(500., .0, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(500., 700., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(300., 500., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(300., .0, .0), kNormalEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon with vertex that lie on right quad edge.");
  }

  {
    // Polygon with vertex that lies on bottom quad edge.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(6);
    geode->AddVertex(gstVertex(.0, -300., .0));
    geode->AddVertex(gstVertex(300., -300.0, .0));
    geode->AddVertex(gstVertex(400., -20., .0));
    geode->AddVertex(gstVertex(300., 300., .0));
    geode->AddVertex(gstVertex(.0, 300., .0));
    geode->AddVertex(gstVertex(.0, -300., .0));

    exp_geode->Clear();
    exp_geode->AddPart(5);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, -20, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(400., -20., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(300., 300., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, 300., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, -20., .0), kNormalEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon with vertex that lie on bottom quad edge.");
  }

  {
    // Polygon with vertex that lies on top quad edge.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(6);
    geode->AddVertex(gstVertex(.0, 700., .0));
    geode->AddVertex(gstVertex(300., 700., .0));
    geode->AddVertex(gstVertex(400., 1000., .0));
    geode->AddVertex(gstVertex(300., 1200., .0));
    geode->AddVertex(gstVertex(.0, 1200., .0));
    geode->AddVertex(gstVertex(.0, 700., .0));

    exp_geode->Clear();
    exp_geode->AddPart(5);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, 700, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(300., 700., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(400., 1000., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, 1000., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, 700., .0), kNormalEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon with vertex that lies on top quad edge.");
  }
}

// Tests polygon clipping functionality: special cases when polygon's
// outer boundary touches quad by vertex.
TEST_F(PolygonClipper2Test, ClippingSpecialCasesOuterTouchByVertexTest) {
  gstBBox quad(-20, 500, -20, 1000);
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  GeodeList expected_geodes;

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  {
    // Polygon touches left quad edge by vertex of outer cycle.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(4);
    geode->AddVertex(gstVertex(-100., -300., .0));
    geode->AddVertex(gstVertex(-20., 300.0, .0));
    geode->AddVertex(gstVertex(-100., 520., .0));
    geode->AddVertex(gstVertex(-100., -300., .0));

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon touches left quad edge by vertex of outer cycle."
        " No clipping.");
  }

  {
    // Polygon touches right quad edge by vertex of outer cycle.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(4);
    geode->AddVertex(gstVertex(500., 400., .0));
    geode->AddVertex(gstVertex(700., 200.0, .0));
    geode->AddVertex(gstVertex(710., 520., .0));
    geode->AddVertex(gstVertex(500., 400., .0));

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon touches right quad edge by vertex of outer cycle."
        " No clipping.");
  }

  {
    // Polygon touches bottom quad edge by vertex of outer cycle.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(4);
    geode->AddVertex(gstVertex(.0, -300., .0));
    geode->AddVertex(gstVertex(300., -300.0, .0));
    geode->AddVertex(gstVertex(400., -20., .0));
    geode->AddVertex(gstVertex(.0, -300., .0));

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        " Polygon touches bottom quad edge by vertex of outer cycle."
        " No clipping.");
  }

  {
    // Polygon touches top quad edge by vertex of outer cycle.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(4);
    geode->AddVertex(gstVertex(.0, 1200., .0));
    geode->AddVertex(gstVertex(300., 1000., .0));
    geode->AddVertex(gstVertex(400., 1500., .0));
    geode->AddVertex(gstVertex(.0, 1200., .0));

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon touches top quad edge by vertex of outer cycle. No clipping.");
  }

  {
    // Polygon touches bottom left quad vertex by edge of outer cycle.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(4);
    geode->AddVertex(gstVertex(-100., -300., .0));
    geode->AddVertex(gstVertex(280., -320.0, .0));
    geode->AddVertex(gstVertex(-320., 280., .0));
    geode->AddVertex(gstVertex(-100., -300., .0));

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon touches bottom left quad vertex by edge of outer cycle."
        " No clipping.");
  }

  {
    // Polygon touches top left quad vertex by edge of outer cycle.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(4);
    geode->AddVertex(gstVertex(-220., 800., .0));
    geode->AddVertex(gstVertex(180., 1200., .0));
    geode->AddVertex(gstVertex(-220., 1200., .0));
    geode->AddVertex(gstVertex(-220., 800., .0));

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon touch top left quad vertex by edge of outer cycle."
        " No clipping.");
  }

  {
    // Polygon touches bottom right quad vertex by edge of outer cycle.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(4);
    geode->AddVertex(gstVertex(400., -120., .0));
    geode->AddVertex(gstVertex(600., -120., .0));
    geode->AddVertex(gstVertex(600., 80., .0));
    geode->AddVertex(gstVertex(400., -120., .0));

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon touches bottom right quad vertex by edge of outer cycle."
        " No clipping.");
  }

  {
    // Polygon touches top right quad vertex by edge of outer cycle.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(4);
    geode->AddVertex(gstVertex(300., 1200., .0));
    geode->AddVertex(gstVertex(700., 800., .0));
    geode->AddVertex(gstVertex(700., 1200., .0));
    geode->AddVertex(gstVertex(300., 1200., .0));

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon touches top right quad vertex by edge of outer cycle."
        " No clipping.");
  }

  {
    // Polygon's outer boundary and quad have shared vertex
    // There is clipping.
    geode->Clear();
    expected_geodes.clear();

    geode->AddPart(8);
    geode->AddVertex(gstVertex(300., -100., .0));
    geode->AddVertex(gstVertex(500., -100., .0));
    geode->AddVertex(gstVertex(500., -20., .0));
    geode->AddVertex(gstVertex(600., 100., .0));
    geode->AddVertex(gstVertex(600., 200., .0));
    geode->AddVertex(gstVertex(300., 200., .0));
    geode->AddVertex(gstVertex(300., -100., .0));

    exp_geode->Clear();
    exp_geode->AddPart(5);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(300., -20., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(500., -20., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(500., 200., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(300., 200., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(300., -20., .0), kNormalEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon's outer boundary and quad have shared vertex."
        " There is clipping");
  }
}

// Tests polygon clipping functionality: special cases when polygon's
// inner boundary touches quad by vertex.
TEST_F(PolygonClipper2Test, ClippingSpecialCasesInnerTouchByVertexTest) {
  gstBBox quad(-20, 500, -20, 1000);
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(quad, kClipEdge);
  GeodeList expected_geodes;

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  {
    // Polygon touches left quad edge by vertex of inner cycle.
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(5);
    geode->AddVertex(gstVertex(-500., -500., .0));
    geode->AddVertex(gstVertex(1000., -500., .0));
    geode->AddVertex(gstVertex(1000., 1500., .0));
    geode->AddVertex(gstVertex(-500., 1500., .0));
    geode->AddVertex(gstVertex(-500., -500., .0));

    // Add inner cycle
    geode->AddPart(4);
    geode->AddVertex(gstVertex(-100., -300., .0));
    geode->AddVertex(gstVertex(-100., 520., .0));
    geode->AddVertex(gstVertex(-20., 300.0, .0));
    geode->AddVertex(gstVertex(-100., -300., .0));

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon touches left quad edge by vertex of inner cycle."
        " No edge clipping.");
  }

  {
    // Polygon touches right quad edge by vertex of inner cycle.
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(5);
    geode->AddVertex(gstVertex(-500., -500., .0));
    geode->AddVertex(gstVertex(1000., -500., .0));
    geode->AddVertex(gstVertex(1000., 1500., .0));
    geode->AddVertex(gstVertex(-500., 1500., .0));
    geode->AddVertex(gstVertex(-500., -500., .0));

    // Add inner cycle
    geode->AddPart(4);
    geode->AddVertex(gstVertex(500., 400., .0));
    geode->AddVertex(gstVertex(710., 520., .0));
    geode->AddVertex(gstVertex(700., 200.0, .0));
    geode->AddVertex(gstVertex(500., 400., .0));

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        " Polygon touches right quad edge by vertex of inner cycle."
        " No edge clipping.");
  }

  {
    // Polygon touches bottom quad edge by vertex of inner cycle.
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(5);
    geode->AddVertex(gstVertex(-500., -500., .0));
    geode->AddVertex(gstVertex(1000., -500., .0));
    geode->AddVertex(gstVertex(1000., 1500., .0));
    geode->AddVertex(gstVertex(-500., 1500., .0));
    geode->AddVertex(gstVertex(-500., -500., .0));

    // Add inner cycle
    geode->AddPart(4);
    geode->AddVertex(gstVertex(.0, -300., .0));
    geode->AddVertex(gstVertex(400., -20., .0));
    geode->AddVertex(gstVertex(300., -300.0, .0));
    geode->AddVertex(gstVertex(.0, -300., .0));

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        " Polygon touches bottom quad edge by vertex of inner cycle."
        " No edge clipping.");
  }


  {
    // Polygon touches top quad edge by vertex of inner cycle.
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(5);
    geode->AddVertex(gstVertex(-500., -500., .0));
    geode->AddVertex(gstVertex(1000., -500., .0));
    geode->AddVertex(gstVertex(1000., 1500., .0));
    geode->AddVertex(gstVertex(-500., 1500., .0));
    geode->AddVertex(gstVertex(-500., -500., .0));

    // Add inner cycle
    geode->AddPart(4);
    geode->AddVertex(gstVertex(.0, 1200., .0));
    geode->AddVertex(gstVertex(400., 1500., .0));
    geode->AddVertex(gstVertex(300., 1000., .0));
    geode->AddVertex(gstVertex(.0, 1200., .0));

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        " Polygon touches top quad edge by vertex of inner cycle."
        " No edge clipping.");
  }


  {
    // Polygon touches bottom left quad vertex by edge of inner cycle.
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(5);
    geode->AddVertex(gstVertex(-500., -500., .0));
    geode->AddVertex(gstVertex(1000., -500., .0));
    geode->AddVertex(gstVertex(1000., 1500., .0));
    geode->AddVertex(gstVertex(-500., 1500., .0));
    geode->AddVertex(gstVertex(-500., -500., .0));

    // Add inner cycle
    geode->AddPart(4);
    geode->AddVertex(gstVertex(-100., -300., .0));
    geode->AddVertex(gstVertex(-320., 280., .0));
    geode->AddVertex(gstVertex(280., -320.0, .0));
    geode->AddVertex(gstVertex(-100., -300., .0));

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        " Polygon touches bottom left quad vertex by edge of inner cycle."
        " No edge clipping.");
  }

  {
    // Polygon touches top left quad vertex by edge of inner cycle.
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(5);
    geode->AddVertex(gstVertex(-500., -500., .0));
    geode->AddVertex(gstVertex(1000., -500., .0));
    geode->AddVertex(gstVertex(1000., 1500., .0));
    geode->AddVertex(gstVertex(-500., 1500., .0));
    geode->AddVertex(gstVertex(-500., -500., .0));

    // Add inner cycle
    geode->AddPart(4);
    geode->AddVertex(gstVertex(-220., 800., .0));
    geode->AddVertex(gstVertex(-220., 1200., .0));
    geode->AddVertex(gstVertex(180., 1200., .0));
    geode->AddVertex(gstVertex(-220., 800., .0));

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        " Polygon touches top left quad vertex by edge of inner cycle."
        " No edge clipping.");
  }

  {
    // Polygon touches bottom right quad vertex by edge of inner cycle.
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(5);
    geode->AddVertex(gstVertex(-500., -500., .0));
    geode->AddVertex(gstVertex(1000., -500., .0));
    geode->AddVertex(gstVertex(1000., 1500., .0));
    geode->AddVertex(gstVertex(-500., 1500., .0));
    geode->AddVertex(gstVertex(-500., -500., .0));

    // Add inner cycle
    geode->AddPart(4);
    geode->AddVertex(gstVertex(400., -120., .0));
    geode->AddVertex(gstVertex(600., 80., .0));
    geode->AddVertex(gstVertex(600., -120., .0));
    geode->AddVertex(gstVertex(400., -120., .0));

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        " Polygon touches bottom right quad vertex by edge of inner cycle."
        " No edge clipping.");
  }

  {
    // Polygon touches top right quad vertex by edge of inner cycle.
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(5);
    geode->AddVertex(gstVertex(-500., -500., .0));
    geode->AddVertex(gstVertex(1000., -500., .0));
    geode->AddVertex(gstVertex(1000., 1500., .0));
    geode->AddVertex(gstVertex(-500., 1500., .0));
    geode->AddVertex(gstVertex(-500., -500., .0));

    // Add inner cycle
    geode->AddPart(4);
    geode->AddVertex(gstVertex(300., 1200., .0));
    geode->AddVertex(gstVertex(700., 1200., .0));
    geode->AddVertex(gstVertex(700., 800., .0));
    geode->AddVertex(gstVertex(300., 1200., .0));

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        " Polygon touches top right quad vertex by edge of inner cycle."
        " No edge clipping.");
  }

  {
    // Polygon's inner boundary and quad have shared vertex
    // There is clipping.
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(5);
    geode->AddVertex(gstVertex(-500., -500., .0));
    geode->AddVertex(gstVertex(1000., -500., .0));
    geode->AddVertex(gstVertex(1000., 1500., .0));
    geode->AddVertex(gstVertex(-500., 1500., .0));
    geode->AddVertex(gstVertex(-500., -500., .0));

    // Add inner cycle
    geode->AddPart(8);
    geode->AddVertex(gstVertex(300., -100., .0));
    geode->AddVertex(gstVertex(300., 200., .0));
    geode->AddVertex(gstVertex(600., 200., .0));
    geode->AddVertex(gstVertex(600., 100., .0));
    geode->AddVertex(gstVertex(500., -20., .0));
    geode->AddVertex(gstVertex(500., -100., .0));
    geode->AddVertex(gstVertex(300., -100., .0));

    exp_geode->Clear();
    exp_geode->AddPart(7);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(-20., -20., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(300., -20., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(300., 200., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(500., 200., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(500., 1000., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(-20., 1000., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(-20., -20., .0), kClipEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon's inner boundary and quad have shared vertex."
        " There is clipping");
  }
}

// Tests polygon clipping functionality: special cases when quad's corners
// inside polygon.
TEST_F(PolygonClipper2Test, ClippingSpecialCasesInnerQuadCornersTest) {
  gstBBox quad(-20, 500, -20, 1000);
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  GeodeList expected_geodes;

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  {
    // Quad's bottom edge is inside polygon.
    // Polygon intersects left and right quad edges by outer cycle.
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(7);
    geode->AddVertex(gstVertex(-100., -100., .0));
    geode->AddVertex(gstVertex(600., -100., .0));
    geode->AddVertex(gstVertex(600., -20., .0));
    geode->AddVertex(gstVertex(400., 180., .0));
    geode->AddVertex(gstVertex(100., 180., .0));
    geode->AddVertex(gstVertex(-100., -20., .0));
    geode->AddVertex(gstVertex(-100., -100., .0));

    exp_geode->Clear();
    exp_geode->AddPart(7);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(-20., -20., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(500., -20., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(500., 80., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(400., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(-20., 60, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(-20., -20., .0), kClipEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Quad's bottom edge is inside polygon. Polygon intersects left and"
        " right quad edge by outer cycle.");
  }

  {
    // Quad's top edge is inside polygon
    // Polygon intersects left and right quad edges by inner cycle.
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(5);
    geode->AddVertex(gstVertex(-500., -500., .0));
    geode->AddVertex(gstVertex(1000., -500., .0));
    geode->AddVertex(gstVertex(1000., 1500., .0));
    geode->AddVertex(gstVertex(-500., 1500., .0));
    geode->AddVertex(gstVertex(-500., -500., .0));

    // Add inner cycle
    geode->AddPart(7);
    geode->AddVertex(gstVertex(-100., -100., .0));
    geode->AddVertex(gstVertex(-100., -20., .0));
    geode->AddVertex(gstVertex(100., 180., .0));
    geode->AddVertex(gstVertex(400., 180., .0));
    geode->AddVertex(gstVertex(600., -20., .0));
    geode->AddVertex(gstVertex(600., -100., .0));
    geode->AddVertex(gstVertex(-100., -100., .0));

    exp_geode->Clear();
    exp_geode->AddPart(7);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(-20., 60, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(100., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(400., 180., .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(500., 80., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(500., 1000., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(-20., 1000., .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(-20., 60., .0), kClipEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Quad's top edge is inside polygon. Polygon intersects left and right"
        " quad edge by inner cycle.");
  }
}

// Tests polygon clipping functionality: special cases when polygon touch
// quad by edge of outer cycle.
TEST_F(PolygonClipper2Test, ClippingSpecialCasesOuterEdgeTouchTest) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  GeodeList expected_geodes;

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  {
    // Polygon's outer cycle edge lies on quad's right edge (Q1).
    gstBBox quad(.0, 0.500, -0.500, .0);
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(6);
    geode->AddVertex(gstVertex(-0.100, -0.010, .0));
    geode->AddVertex(gstVertex(0.500, -0.010, .0));
    geode->AddVertex(gstVertex(0.500, 0.520, .0));
    geode->AddVertex(gstVertex(-0.050, 0.700, .0));
    geode->AddVertex(gstVertex(-0.100, 0.700, .0));
    geode->AddVertex(gstVertex(-0.100, -0.010, .0));

    exp_geode->Clear();
    exp_geode->AddPart(5);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, -0.010, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, -0.010, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, .0, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, .0, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, -0.010, .0), kClipEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon's outer cycle edge lies on quad's right edge (Q1). ");
  }

  {
    // Polygon's outer cycle edge lies on quad's right edge (Q3).
    gstBBox quad(.0, 0.500, .0, 0.500);
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(6);
    geode->AddVertex(gstVertex(-0.100, -0.010, .0));
    geode->AddVertex(gstVertex(0.500, -0.010, .0));
    geode->AddVertex(gstVertex(0.500, 0.520, .0));
    geode->AddVertex(gstVertex(-0.050, 0.700, .0));
    geode->AddVertex(gstVertex(-0.100, 0.700, .0));
    geode->AddVertex(gstVertex(-0.100, -0.010, .0));

    exp_geode->Clear();
    exp_geode->AddPart(5);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, .0, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, .0, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, 0.500, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, 0.500, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, .0, .0), kClipEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon's outer cycle edge lies on quad's right edge (Q3).");
  }

  {
    // Polygon's outer cycle edge lies on quad's right edge (Q5).
    gstBBox quad(0., 0.500, 0.500, 1.000);
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(5);
    geode->AddVertex(gstVertex(-0.100, -0.010, .0));
    geode->AddVertex(gstVertex(0.500, -0.010, .0));
    geode->AddVertex(gstVertex(0.500, 0.700, .0));
    geode->AddVertex(gstVertex(-0.100, 1.300, .0));
    geode->AddVertex(gstVertex(-0.100, -0.010, .0));

    exp_geode->Clear();
    exp_geode->AddPart(5);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, 0.500, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, 0.500, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, 0.700, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.200, 1.000, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, 1.000, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, 0.500, .0), kClipEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon's outer cycle edge lies on quad's right edge (Q5).");
  }

  {
    // Polygon's outer cycle edge lies on quad's left edge (Q2).
    // Result: degenerate polygon.
    gstBBox quad(0.500, 1.000, -0.500, .0);
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(6);
    geode->AddVertex(gstVertex(-0.100, -0.010, .0));
    geode->AddVertex(gstVertex(0.500, -0.010, .0));
    geode->AddVertex(gstVertex(0.500, 0.520, .0));
    geode->AddVertex(gstVertex(-0.050, 0.700, .0));
    geode->AddVertex(gstVertex(-0.100, 0.700, .0));
    geode->AddVertex(gstVertex(-0.100, -0.010, .0));

    expected_geodes.clear();

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon's outer cycle edge lies on quad's left edge (Q2)."
        " Result: degenerate polygon.");
  }

  {
    // Polygon's outer cycle edge lies on quad's left edge (Q4).
    // Result: degenerate polygon.
    gstBBox quad(0.500, 1.000, .0, 0.500);
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(6);
    geode->AddVertex(gstVertex(-0.100, -0.010, .0));
    geode->AddVertex(gstVertex(0.500, -0.010, .0));
    geode->AddVertex(gstVertex(0.500, 0.520, .0));
    geode->AddVertex(gstVertex(-0.050, 0.700, .0));
    geode->AddVertex(gstVertex(-0.100, 0.700, .0));
    geode->AddVertex(gstVertex(-0.100, -0.010, .0));

    expected_geodes.clear();

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon's outer cycle edge lies on quad's left edge (Q4)."
        " Result: degenerate polygon.");
  }

  {
    // Polygon's outer cycle edge lies on quad's left edge (Q6).
    // Result:  degenerate polygon.
    gstBBox quad(0.500, 1.000, 0.500, 1.000);
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(5);
    geode->AddVertex(gstVertex(-0.100, -0.010, .0));
    geode->AddVertex(gstVertex(0.500, -0.010, .0));
    geode->AddVertex(gstVertex(0.500, 0.700, .0));
    geode->AddVertex(gstVertex(-0.100, 1.300, .0));
    geode->AddVertex(gstVertex(-0.100, -0.010, .0));

    expected_geodes.clear();

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon's outer cycle edge lies on quad's left edge (Q6)."
        " Result: degenerate polygon.");
  }
}

// Tests polygon clipping functionality: special case when polygon outer
// cycle and quad are completely coincide.
TEST_F(PolygonClipper2Test, ClippingSpecialCasesOuterCoincideTest) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  GeodeList expected_geodes;

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  {
    // Polygon outer cycle and quad are completely coincide.
    gstBBox quad(.0, 0.500, .0, 0.500);
    geode->Clear();
    expected_geodes.clear();

    // Add outer cycle
    geode->AddPart(5);
    geode->AddVertex(gstVertex(.0, .0, .0));
    geode->AddVertex(gstVertex(0.500, .0, .0));
    geode->AddVertex(gstVertex(0.500, 0.500, .0));
    geode->AddVertex(gstVertex(.0, 0.500, .0));
    geode->AddVertex(gstVertex(.0, .0, .0));

    exp_geode->Clear();
    exp_geode->AddPart(5);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, .0, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, .0, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, 0.500, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, 0.500, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, .0, .0), kNormalEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon outer cycle and quad are completely coincide.");
  }
}

// Tests polygon clipping functionality: special cases when polygon touch
// quad by edge of inner cycle.
TEST_F(PolygonClipper2Test, ClippingSpecialCasesInnerEdgeTouchTest) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  GeodeList expected_geodes;

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  // Add outer cycle
  geode->AddPart(5);
  geode->AddVertex(gstVertex(-0.200, -0.600, .0));
  geode->AddVertex(gstVertex(1.200, -0.600, .0));
  geode->AddVertex(gstVertex(1.200, 1.400, .0));
  geode->AddVertex(gstVertex(-0.200, 1.400, .0));
  geode->AddVertex(gstVertex(-0.200, -0.600, .0));

  // Add inner cycle
  geode->AddPart(5);
  geode->AddVertex(gstVertex(-0.100, -0.010, .0));
  geode->AddVertex(gstVertex(-0.100, 1.300, .0));
  geode->AddVertex(gstVertex(0.500, 0.700, .0));
  geode->AddVertex(gstVertex(0.500, -0.010, .0));
  geode->AddVertex(gstVertex(-0.100, -0.010, .0));

  {
    // Polygon's inner cycle edge lies on quad's right edge (Q1).
    gstBBox quad(.0, 0.500, -0.500, .0);
    expected_geodes.clear();

    exp_geode->Clear();
    exp_geode->AddPart(5);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, -0.500, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, -0.500, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, -0.010, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, -0.010, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(.0, -0.500, .0), kClipEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon's inner cycle edge lies on quad's right edge (Q1). ");
  }

  {
    // Polygon's inner cycle edge lies on quad's right edge (Q3).
    gstBBox quad(.0, 0.500, .0, 0.500);
    expected_geodes.clear();

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon's inner cycle edge lies on quad's right edge (Q3)."
        " Result: degenerate polygon.");
  }

  {
    // Polygon's inner cycle edge lies on quad's right edge (Q5).
    gstBBox quad(0., 0.500, 0.500, 1.000);
    expected_geodes.clear();

    exp_geode->Clear();
    exp_geode->AddPart(5);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.200, 1.000, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, 0.700, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, 1.000, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.200, 1.000, .0), kClipEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon's inner cycle edge lies on quad's right edge (Q5).");
  }

  {
    // Polygon's inner cycle edge lies on quad's left edge (Q2).
    gstBBox quad(0.500, 1.000, -0.500, .0);
    expected_geodes.clear();

    exp_geode->Clear();
    exp_geode->AddPart(3);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, -0.500, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(1.000, -0.500, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(1.000, .0, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, .0, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, -0.010, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, -0.500, .0), kClipEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon's inner cycle edge lies on quad's left edge (Q2).");
  }

  {
    // Polygon's inner cycle edge lies on quad's left edge (Q4).
    gstBBox quad(0.500, 1.000, .0, 0.500);
    expected_geodes.clear();

    exp_geode->Clear();
    exp_geode->AddPart(3);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, .0, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(1.000, .0, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(1.000, 0.500, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, 0.500, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, .0, .0), kNormalEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon's inner cycle edge lies on quad's left edge (Q4).");
  }

  {
    // Polygon's inner cycle edge lies on quad's left edge (Q6).
    gstBBox quad(0.500, 1.000, 0.500, 1.000);
    expected_geodes.clear();

    exp_geode->Clear();
    exp_geode->AddPart(3);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, 0.500, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(1.000, 0.500, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(1.000, 1.000, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, 1.000, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, 0.700, .0), kNormalEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, 0.500, .0), kNormalEdge);

    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Polygon's inner cycle edge lies on quad's right edge (Q6).");
  }
}


TEST_F(PolygonClipper2Test, ClippingQuadInsideOutsidePolygonTest) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  GeodeList expected_geodes;

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  {
    gstBBox quad(.400, 0.500, .100, 0.200);
    expected_geodes.clear();

    geode->Clear();
    geode->AddPart(5);
    geode->AddVertex(gstVertex(.300, .0, .0));
    geode->AddVertex(gstVertex(0.600, .0, .0));
    geode->AddVertex(gstVertex(0.600, 0.300, .0));
    geode->AddVertex(gstVertex(0.300, 0.300, .0));
    geode->AddVertex(gstVertex(0.300, .0, .0));

    exp_geode->Clear();
    exp_geode->AddPart(5);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.400, 0.100, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, 0.100, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, 0.200, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.400, 0.200, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.400, 0.100, .0), kClipEdge);
    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes, "Quad inside rectangle.");
  }

  {
    gstBBox quad(.400, 0.500, .400, 0.500);
    expected_geodes.clear();

    geode->Clear();
    geode->AddPart(5);
    geode->AddVertex(gstVertex(.200, .450, .0));
    geode->AddVertex(gstVertex(0.450, .200, .0));
    geode->AddVertex(gstVertex(0.700, 0.450, .0));
    geode->AddVertex(gstVertex(0.450, 0.700, .0));
    geode->AddVertex(gstVertex(0.200, .450, .0));

    exp_geode->Clear();
    exp_geode->AddPart(5);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.400, 0.400, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, 0.400, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.500, 0.500, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.400, 0.500, .0), kClipEdge);
    exp_geode->AddVertexAndEdgeFlag(gstVertex(0.400, 0.400, .0), kClipEdge);
    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(geodeh, quad, expected_geodes,
                         "Quad inside rhombus.");
  }

  {
    gstBBox quad(.700, 0.900, .100, 0.200);
    expected_geodes.clear();

    CheckPolygonClipping(geodeh, quad, expected_geodes,
                         "Quad outside rectangle.");
  }

  {
    gstBBox quad(.400, 0.500, .100, 0.200);
    expected_geodes.clear();

    geode->Clear();
    geode->AddPart(5);
    geode->AddVertex(gstVertex(.300, .0, .0));
    geode->AddVertex(gstVertex(0.600, .0, .0));
    geode->AddVertex(gstVertex(0.600, .050, .0));
    geode->AddVertex(gstVertex(0.350, .050, .0));
    geode->AddVertex(gstVertex(0.350, .550, .0));
    geode->AddVertex(gstVertex(0.600, .550, .0));
    geode->AddVertex(gstVertex(0.600, 0.300, .0));
    geode->AddVertex(gstVertex(0.300, 0.300, .0));
    geode->AddVertex(gstVertex(0.300, .0, .0));

    CheckPolygonClipping(geodeh, quad, expected_geodes,
                         "Quad outside complex polygon.");
  }
}

TEST_F(PolygonClipper2Test, ClippingSpecialCaseQuadOverlapTest1) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  GeodeList expected_geodes;

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  std::vector<gstVertex> verts;
  verts.push_back(gstVertex(khTilespaceBase::Normalize(.0),
                            khTilespaceBase::Normalize(22.5),
                            .0));
  verts.push_back(gstVertex(khTilespaceBase::Normalize(45.),
                            khTilespaceBase::Normalize(22.5),
                            .0));
  verts.push_back(gstVertex(khTilespaceBase::Normalize(45.),
                            khTilespaceBase::Normalize(45.),
                            .0));
  verts.push_back(gstVertex(khTilespaceBase::Normalize(.0),
                            khTilespaceBase::Normalize(45.),
                            .0));
  verts.push_back(verts[0]);

  {
    gstBBox quad(verts[0].x, verts[2].x, verts[0].y, verts[2].y);
    expected_geodes.clear();

    geode->Clear();
    geode->AddPart(verts.size());
    for (size_t i = 0; i < verts.size(); ++i) {
      geode->AddVertex(verts[i]);
    }

    exp_geode->Clear();
    exp_geode->AddPart(verts.size());
    for (size_t i = 0; i < verts.size(); ++i) {
      exp_geode->AddVertexAndEdgeFlag(verts[i], kNormalEdge);
    }
    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Quad inside rectangle (boundaries overlap).", true);
  }
}

TEST_F(PolygonClipper2Test, ClippingSpecialCaseQuadOverlapTest2) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  GeodeList expected_geodes;

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  std::vector<gstVertex> verts;
  verts.push_back(gstVertex(khTilespaceBase::Normalize(.0),
                            khTilespaceBase::Normalize(22.5),
                            .0));
  verts.push_back(gstVertex(khTilespaceBase::Normalize(45.),
                            khTilespaceBase::Normalize(22.5),
                            .0));
  verts.push_back(gstVertex(khTilespaceBase::Normalize(45.),
                            khTilespaceBase::Normalize(45.),
                            .0));
  verts.push_back(gstVertex(khTilespaceBase::Normalize(.0),
                            khTilespaceBase::Normalize(45.),
                            .0));
  verts.push_back(verts[0]);
  geode->Clear();
  geode->AddPart(verts.size());
  for (size_t i = 0; i < verts.size(); ++i) {
    geode->AddVertex(verts[i]);
  }

  {
    expected_geodes.clear();
    gstBBox quad(
        khTilespaceBase::Normalize(.0), khTilespaceBase::Normalize(22.5),
        khTilespaceBase::Normalize(22.5), khTilespaceBase::Normalize(45.0));
    gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(quad);
    gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));
    exp_geode->AddEdgeFlag(kNormalEdge);
    exp_geode->AddEdgeFlag(kClipEdge);
    exp_geode->AddEdgeFlag(kNormalEdge);
    exp_geode->AddEdgeFlag(kNormalEdge);
    exp_geode->AddEdgeFlag(kNormalEdge);
    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Quad inside rectangle (boundaries overlap).", true);
  }

  {
    expected_geodes.clear();
    gstBBox quad(
        khTilespaceBase::Normalize(22.5), khTilespaceBase::Normalize(45.0),
        khTilespaceBase::Normalize(22.5), khTilespaceBase::Normalize(45.0));

    gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(quad);
    gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));
    exp_geode->AddEdgeFlag(kNormalEdge);
    exp_geode->AddEdgeFlag(kNormalEdge);
    exp_geode->AddEdgeFlag(kNormalEdge);
    exp_geode->AddEdgeFlag(kClipEdge);
    exp_geode->AddEdgeFlag(kClipEdge);
    expected_geodes.push_back(exp_geodeh);

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Quad inside rectangle (boundaries overlap).", true);
  }
}

TEST_F(PolygonClipper2Test, ClippingSpecialCaseQuadEdgeTouchTest) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  GeodeList expected_geodes;

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  std::vector<gstVertex> verts;
  verts.push_back(gstVertex(khTilespaceBase::Normalize(.0),
                            khTilespaceBase::Normalize(22.5),
                            .0));
  verts.push_back(gstVertex(khTilespaceBase::Normalize(22.5),
                            khTilespaceBase::Normalize(22.5),
                            .0));
  verts.push_back(gstVertex(khTilespaceBase::Normalize(22.5),
                            khTilespaceBase::Normalize(45.),
                            .0));
  verts.push_back(gstVertex(khTilespaceBase::Normalize(.0),
                            khTilespaceBase::Normalize(45.0),
                            .0));
  verts.push_back(verts[0]);

  geode->Clear();
  geode->AddPart(verts.size());
  for (size_t i = 0; i < verts.size(); ++i) {
    geode->AddVertex(verts[i]);
  }

  {
    expected_geodes.clear();
    gstBBox quad(
        khTilespaceBase::Normalize(-22.5), khTilespaceBase::Normalize(0),
        khTilespaceBase::Normalize(22.5), khTilespaceBase::Normalize(45.0));

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Quad outside rectangle - west edge touch.", true);
  }

  {
    expected_geodes.clear();
    gstBBox quad(
        khTilespaceBase::Normalize(.0), khTilespaceBase::Normalize(22.5),
        khTilespaceBase::Normalize(45.0), khTilespaceBase::Normalize(67.0));

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Quad outside rectangle - north edge touch.", true);
  }

  {
    expected_geodes.clear();
    gstBBox quad(
        khTilespaceBase::Normalize(22.5), khTilespaceBase::Normalize(45.0),
        khTilespaceBase::Normalize(22.5), khTilespaceBase::Normalize(45.0));

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Quad outside rectangle - east edge touch.", true);
  }

  {
    expected_geodes.clear();
    gstBBox quad(
        khTilespaceBase::Normalize(.0), khTilespaceBase::Normalize(22.5),
        khTilespaceBase::Normalize(.0), khTilespaceBase::Normalize(22.5));

    CheckPolygonClipping(
        geodeh, quad, expected_geodes,
        "Quad outside rectangle - south edge touch.", true);
  }
}



}  // namespace fusion_gst


int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

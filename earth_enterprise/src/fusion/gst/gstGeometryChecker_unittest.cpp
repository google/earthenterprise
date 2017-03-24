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


// Unit tests for GeometryChecker functionality.

#include <gtest/gtest.h>

#include <string>

#include "fusion/gst/gstVertex.h"
#include "fusion/gst/gstGeode.h"
#include "fusion/gst/gstMathUtils.h"
#include "fusion/gst/gstGeometryChecker.h"
#include "fusion/gst/gstUnitTestUtils.h"

namespace fusion_gst {

// GeometryChecker test class.
class GeometryCheckerTest : public testing::Test {
 protected:
  // Utility method for checking the result of
  // RemoveCoincidentVertex()-function.
  void CheckRemoveCoincidentVertex(const gstGeodeHandle &geodeh,
                                   const gstGeodeHandle &expected_geodeh,
                                   const std::string &message) const;

  // Utility method for checking the result of RemoveSpike()-function.
  void CheckRemoveSpikes(const gstGeodeHandle &geodeh,
                         const gstGeodeHandle &expected_geodeh,
                         const std::string &message) const;

  // Utility method for checking the result of
  // CheckAndFixCycleOrientation()-function.
  void CheckAndFixCycleOrientation(const gstGeodeHandle &geodeh,
                                   const gstGeodeHandle &expected_geodeh,
                                   const std::string &message) const;

 private:
  GeometryChecker checker_;
};

// Utility method for checking the result of RemoveCoincidentVertex-function.
void GeometryCheckerTest::CheckRemoveCoincidentVertex(
    const gstGeodeHandle &geodeh,
    const gstGeodeHandle &expected_geodeh,
    const std::string &message) const {
  gstGeodeHandle gh = geodeh->Duplicate();

  checker_.RemoveCoincidentVertices(&gh);

  TestUtils::CompareGeodes(gh, expected_geodeh, message);
}

// Utility method for checking the result of RemoveSpikes-function.
void GeometryCheckerTest::CheckRemoveSpikes(
    const gstGeodeHandle &geodeh,
    const gstGeodeHandle &expected_geodeh,
    const std::string &message) const {
  gstGeodeHandle gh = geodeh->Duplicate();

  checker_.RemoveSpikes(&gh);

  TestUtils::CompareGeodes(gh, expected_geodeh, message);
}

// Utility method for checking the result of CheckAndFixCycleOrientation()
// -fucntion.
void GeometryCheckerTest::CheckAndFixCycleOrientation(
    const gstGeodeHandle &geodeh,
    const gstGeodeHandle &expected_geodeh,
    const std::string &message) const {
  gstGeodeHandle gh = geodeh->Duplicate();

  checker_.CheckAndFixCycleOrientation(&gh);

  TestUtils::CompareGeodes(gh, expected_geodeh, message);
}

// Tests RemoveCoincidentVertex-functionality.
TEST_F(GeometryCheckerTest, RemoveCoincidentVertexTest) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.100, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));
  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));

  {
    geode->Clear();

    geode->AddPart(6);
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddPart(6);
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));

    CheckRemoveCoincidentVertex(
        geodeh, exp_geodeh,
        "Coincident vertex removing: coincident vertices at the beginning.");
  }


  {
    geode->Clear();

    geode->AddPart(6);
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddPart(6);
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));

    CheckRemoveCoincidentVertex(
        geodeh, exp_geodeh,
        "Coincident vertex removing: coincident vertices at the end.");
  }

  {
    geode->Clear();

    geode->AddPart(8);
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.200, .0));
    geode->AddVertex(gstVertex(0.220, 0.200, .0));
    geode->AddVertex(gstVertex(0.220, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddPart(8);
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));

    CheckRemoveCoincidentVertex(
        geodeh, exp_geodeh,
        "Coincident vertex removing: coincident vertices in the middle.");
  }

  {
    geode->Clear();

    geode->AddPart(11);
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.200, .0));
    geode->AddVertex(gstVertex(0.220, 0.200, .0));
    geode->AddVertex(gstVertex(0.220, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddPart(11);
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));

    CheckRemoveCoincidentVertex(
        geodeh, exp_geodeh,
        "Coincident vertex removing: coincident vertices everewhere.");
  }
}

// CheckRemoveSpikesTest1 - generic test 1.
TEST_F(GeometryCheckerTest, CheckRemoveSpikesTest1) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.100, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));
  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));

  {
    geode->Clear();

    geode->AddPart(7);
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.320, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.50, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddPart(7);
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.220, .0));
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.050, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));

    CheckRemoveSpikes(
        geodeh, exp_geodeh,
        "Spikes removing: generic test 1.");
  }
}

// CheckRemoveSpikesTest2 - generic test 2.
TEST_F(GeometryCheckerTest, CheckRemoveSpikesTest2) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.100, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));
  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));

  {
    geode->Clear();

    geode->AddPart(9);
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.320, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.400, .0));
    geode->AddVertex(gstVertex(0.220, 0.200, .0));
    geode->AddVertex(gstVertex(0.040, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.50, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddPart(9);
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.220, .0));
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.350, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.110, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.050, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));

    CheckRemoveSpikes(
        geodeh, exp_geodeh,
        "Spikes removing: generic test 2.");
  }
}

// CheckRemoveSpikesTest3 - generic test 3.
TEST_F(GeometryCheckerTest, CheckRemoveSpikesTest3) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.220, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.220, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));
  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.160, 0.160, .0));
  exp_geode->AddVertex(gstVertex(0.160, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));

  {
    geode->Clear();

    geode->AddPart(7);
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.280, 0.280, .0));
    geode->AddVertex(gstVertex(0.220, 0.220, .0));
    geode->AddVertex(gstVertex(0.040, 0.220, .0));
    geode->AddVertex(gstVertex(0.100, 0.220, .0));
    geode->AddVertex(gstVertex(0.100, 0.50, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddPart(7);
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.180, 0.180, .0));
    geode->AddVertex(gstVertex(0.160, 0.160, .0));
    geode->AddVertex(gstVertex(0.160, 0.120, .0));
    geode->AddVertex(gstVertex(0.180, 0.120, .0));
    geode->AddVertex(gstVertex(0.050, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));

    CheckRemoveSpikes(
        geodeh, exp_geodeh,
        "Spikes removing: generic test 3.");
  }
}

// CheckRemoveSpikesTest4 - polygon cycles with collinear points.
TEST_F(GeometryCheckerTest, CheckRemoveSpikesTest4) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));
  exp_geode->AddVertex(gstVertex(0.170, 0.100, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.100, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.150, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.180, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.130, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));
  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.140, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.170, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.145, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.135, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));

  {
    geode->Clear();

    geode->AddPart(7);
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.170, 0.100, .0));
    geode->AddVertex(gstVertex(0.320, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.150, .0));
    geode->AddVertex(gstVertex(0.220, 0.200, .0));
    geode->AddVertex(gstVertex(0.180, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.130, .0));
    geode->AddVertex(gstVertex(0.100, 0.050, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddPart(7);
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.140, .0));
    geode->AddVertex(gstVertex(0.120, 0.220, .0));
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.170, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.145, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.135, 0.120, .0));
    geode->AddVertex(gstVertex(0.050, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));

    CheckRemoveSpikes(
        geodeh, exp_geodeh,
        "Spikes removing: collinear points test.");
  }
}

// CheckRemoveSpikesTest5 - small triangle from real data set.
TEST_F(GeometryCheckerTest, CheckRemoveSpikesTest5) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  gstVertex pt1 = gstVertex(
      khTilespaceBase::Normalize(-90.6035304620),
      khTilespaceBase::Normalize(34.9918161022),
      .0);

  gstVertex pt2 = gstVertex(
      khTilespaceBase::Normalize(-90.6035310000),
      khTilespaceBase::Normalize(34.9918180000),
      .0);

  gstVertex pt3 = gstVertex(
      khTilespaceBase::Normalize(-90.6035342156),
      khTilespaceBase::Normalize(34.9918172140),
      .0);

  exp_geode->AddPart(4);
  exp_geode->AddVertex(pt1);
  exp_geode->AddVertex(pt2);
  exp_geode->AddVertex(pt3);
  exp_geode->AddVertex(pt1);

  {
    geode->Clear();

    geode->AddPart(4);
    geode->AddVertex(pt1);
    geode->AddVertex(pt2);
    geode->AddVertex(pt3);
    geode->AddVertex(pt1);

    CheckRemoveSpikes(
        geodeh, exp_geodeh,
        "Spikes removing: small triangle from real data set.");
  }
}

// CheckRemoveSpikesTest6 - polygon w/ spike from real data set 1.
TEST_F(GeometryCheckerTest, CheckRemoveSpikesTest6) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  gstVertex pt1 = gstVertex(
      khTilespaceBase::Normalize(106.8182506368),
      khTilespaceBase::Normalize(10.6542182242),
      .0);

  gstVertex pt2 = gstVertex(
      khTilespaceBase::Normalize(106.8124851861),
      khTilespaceBase::Normalize(10.6542509147),
      .0);

  gstVertex pt3 = gstVertex(
      khTilespaceBase::Normalize(106.8155637637),
      khTilespaceBase::Normalize(10.6542318523),
      .0);

  gstVertex pt4 = gstVertex(
      khTilespaceBase::Normalize(106.8159899627),
      khTilespaceBase::Normalize(10.6527421661),
      .0);

  exp_geode->AddPart(5);
  exp_geode->AddVertex(pt1);
  //  exp_geode->AddVertex(pt2);   // spike vertex.
  exp_geode->AddVertex(pt3);
  exp_geode->AddVertex(pt4);
  exp_geode->AddVertex(pt1);

  {
    geode->Clear();

    geode->AddPart(5);
    geode->AddVertex(pt1);
    geode->AddVertex(pt2);
    geode->AddVertex(pt3);
    geode->AddVertex(pt4);
    geode->AddVertex(pt1);

    CheckRemoveSpikes(
        geodeh, exp_geodeh,
        "Spikes removing: polygon w/ spike from real data set 1.");
  }
}

// CheckRemoveSpikesTest7 - polygon w/ spike from real data set 2.
TEST_F(GeometryCheckerTest, CheckRemoveSpikesTest7) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  gstVertex pt1 = gstVertex(
      khTilespaceBase::Normalize(-8.9993160617),
      khTilespaceBase::Normalize(43.2081516536),
      .0);

  gstVertex pt2 = gstVertex(
      khTilespaceBase::Normalize(-8.9994751791),
      khTilespaceBase::Normalize(43.2077953989),
      .0);

  gstVertex pt3 = gstVertex(
      khTilespaceBase::Normalize(-8.9994724853),
      khTilespaceBase::Normalize(43.2078014303),
      .0);

  gstVertex pt4 = gstVertex(
      khTilespaceBase::Normalize(-8.9993440068),
      khTilespaceBase::Normalize(43.2074263067),
      .0);

  exp_geode->AddPart(5);
  exp_geode->AddVertex(pt1);
  //  exp_geode->AddVertex(pt2);  // spike vertex.
  exp_geode->AddVertex(pt3);
  exp_geode->AddVertex(pt4);
  exp_geode->AddVertex(pt1);

  {
    geode->Clear();

    geode->AddPart(5);
    geode->AddVertex(pt1);
    geode->AddVertex(pt2);
    geode->AddVertex(pt3);
    geode->AddVertex(pt4);
    geode->AddVertex(pt1);

    CheckRemoveSpikes(
        geodeh, exp_geodeh,
        "Spikes removing: simple polygon w/ spike from real data set 2.");
  }
}

// CheckRemoveSpikesTest8 - collinear vertices at the end of cycle.
TEST_F(GeometryCheckerTest, CheckRemoveSpikesTest8) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.100, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));

  {
    geode->Clear();

    geode->AddPart(9);
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.100, .0));
    geode->AddVertex(gstVertex(0.320, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.100, 0.50, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));

    CheckRemoveSpikes(
        geodeh, exp_geodeh,
        "Spikes removing: collinear vertices at the end of cycle 8.");
  }
}

// CheckAndFixCycleOrientationTest1 - single-part geode with correct
// orientation - rectangle.
TEST_F(GeometryCheckerTest, CheckAndFixCycleOrientationTest1) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.100, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));

  {
    geode->Clear();

    geode->AddPart(5);
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));

    CheckAndFixCycleOrientation(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: single-part geode with correct"
        " orientation - rectangle.");
  }
}

// CheckAndFixCycleOrientationTest2 - single-part geode with correct
// orientation - obtuse angle is incident to most left-lower vertex.
TEST_F(GeometryCheckerTest, CheckAndFixCycleOrientationTest2) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.050, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));

  {
    geode->Clear();

    geode->AddPart(5);
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.050, .0));
    geode->AddVertex(gstVertex(0.220, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));

    CheckAndFixCycleOrientation(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: single-part geode with correct"
        " orientation and obtuse angle 1.");
  }
}


// CheckAndFixCycleOrientationTest3 - single-part geode with correct
// orientation - obtuse angle  is incident to most left-lower vertex.
TEST_F(GeometryCheckerTest, CheckAndFixCycleOrientationTest3) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.050, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.050, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));

  {
    geode->Clear();

    geode->AddPart(6);
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.050, .0));
    geode->AddVertex(gstVertex(0.220, 0.200, .0));
    geode->AddVertex(gstVertex(0.050, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));


    CheckAndFixCycleOrientation(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: single-part geode with correct"
        " orientation and obtuse angle 2.");
  }
}


// CheckAndFixCycleOrientationTest4 - single-part geode with incorrect
// orientation - rectangle.
TEST_F(GeometryCheckerTest, CheckAndFixCycleOrientationTest4) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  exp_geode->AddPart(5);

  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));

  {
    geode->Clear();

    geode->AddPart(5);
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));

    CheckAndFixCycleOrientation(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: single-part geode with incorrect"
        " orientation - rectangle.");
  }
}

// CheckAndFixCycleOrientationTest5 - single-part geode with incorrect
// orientation - obtuse angle is incident to most left-lower vertex.
TEST_F(GeometryCheckerTest, CheckAndFixCycleOrientationTest5) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  exp_geode->AddPart(5);

  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.60, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));

  {
    geode->Clear();

    geode->AddPart(5);
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.60, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));

    CheckAndFixCycleOrientation(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: single-part geode with incorrect"
        " orientation - obtuse angle.");
  }
}

// CheckAndFixCycleOrientationTest6 - single-part geode with incorrect
// orientation - acute angle is incident to most left lower vertex.
TEST_F(GeometryCheckerTest, CheckAndFixCycleOrientationTest6) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  exp_geode->AddPart(5);

  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.180, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));

  {
    geode->Clear();

    geode->AddPart(5);
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.180, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));

    CheckAndFixCycleOrientation(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: single-part geode with incorrect"
        " orientation - acute angle 1.");
  }
}

// CheckAndFixCycleOrientationTest7 - single-part geode with incorrect
// orientation - acute angle is incident to most left lower vertex.
TEST_F(GeometryCheckerTest, CheckAndFixCycleOrientationTest7) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  exp_geode->AddPart(5);

  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.160, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.180, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));

  {
    geode->Clear();

    geode->AddPart(5);
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.180, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.160, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));

    CheckAndFixCycleOrientation(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: single-part geode with incorrect"
        " orientation - acute angle 2.");
  }
}

// CheckAndFixCycleOrientationTest8 - single-part geode with incorrect
// orientation - right angle is incident to most left lower vertex.
TEST_F(GeometryCheckerTest, CheckAndFixCycleOrientationTest8) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  exp_geode->AddPart(5);

  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.200, .0, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.180, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));

  {
    geode->Clear();

    geode->AddPart(5);
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.180, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, .0, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));

    CheckAndFixCycleOrientation(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: single-part geode with incorrect"
        " orientation - right angle.");
  }
}

// CheckAndFixCycleOrientationMultiPartGeodeTest1 - multi-part geode with
// incorrect orientation - rectangles.
TEST_F(GeometryCheckerTest, CheckAndFixCycleOrientationMultiPartGeodeTest1) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.020, 0.050, .0));
  exp_geode->AddVertex(gstVertex(0.400, 0.050, .0));
  exp_geode->AddVertex(gstVertex(0.400, 0.400, .0));
  exp_geode->AddVertex(gstVertex(0.020, 0.400, .0));
  exp_geode->AddVertex(gstVertex(0.020, 0.050, .0));

  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));

  {
    geode->Clear();
    geode->AddPart(5);
    geode->AddVertex(gstVertex(0.020, 0.050, .0));
    geode->AddVertex(gstVertex(0.020, 0.400, .0));
    geode->AddVertex(gstVertex(0.400, 0.400, .0));
    geode->AddVertex(gstVertex(0.400, 0.050, .0));
    geode->AddVertex(gstVertex(0.020, 0.050, .0));

    geode->AddPart(5);
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));

    CheckAndFixCycleOrientation(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: multi-part geode with incorrect"
        " orientation - rectangles.");
  }
}

// CheckAndFixCycleOrientationMultiPartGeodeTest2 - multi-part geode with
// incorrect orientation - acute/obtuse angles are incident to most left
// lower vertex.
TEST_F(GeometryCheckerTest, CheckAndFixCycleOrientationMultiPartGeodeTest2) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.010, 0.020, .0));
  exp_geode->AddVertex(gstVertex(0.400, 0.050, .0));
  exp_geode->AddVertex(gstVertex(0.400, 0.400, .0));
  exp_geode->AddVertex(gstVertex(0.020, 0.400, .0));
  exp_geode->AddVertex(gstVertex(0.010, 0.020, .0));

  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.050, 0.150, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));

  {
    geode->Clear();
    geode->AddPart(5);
    geode->AddVertex(gstVertex(0.010, 0.020, .0));
    geode->AddVertex(gstVertex(0.020, 0.400, .0));
    geode->AddVertex(gstVertex(0.400, 0.400, .0));
    geode->AddVertex(gstVertex(0.400, 0.050, .0));
    geode->AddVertex(gstVertex(0.010, 0.020, .0));

    geode->AddPart(5);
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.050, 0.150, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));

    CheckAndFixCycleOrientation(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: multi-part geode with incorrect"
        " orientation - acute/obtuse angles.");
  }
}

}  // namespace fusion_gst


int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

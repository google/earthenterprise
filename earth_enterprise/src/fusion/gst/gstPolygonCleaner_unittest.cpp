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


#include "fusion/gst/gstPolygonCleaner.h"

#include <gtest/gtest.h>

#include "fusion/gst/gstUnitTestUtils.h"
#include "fusion/gst/gstGeode.h"
#include "fusion/gst/gstVertex.h"


namespace fusion_gst {

// GeometryChecker test class.
class PolygonCleanerTest : public testing::Test {
 protected:
  void CheckCleanPolygon(const gstGeodeHandle &geodeh,
                    const gstGeodeHandle &expected_geodeh,
                    const std::string &message);

 private:
  PolygonCleaner cleaner_;
};

void PolygonCleanerTest::CheckCleanPolygon(
    const gstGeodeHandle &geodeh,
    const gstGeodeHandle &expected_geodeh,
    const std::string &message) {
  gstGeodeHandle geodeh_d = geodeh->Duplicate();

  cleaner_.Run(&geodeh_d);

  TestUtils::CompareGeodes(geodeh_d, expected_geodeh, message);
}

// CheckAndFixCycleOrientationTest1 - single-part geode with correct
// orientation - rectangle.
TEST_F(PolygonCleanerTest, CheckAndFixCycleOrientationTest1) {
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

    CheckCleanPolygon(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: single-part geode with correct"
        " orientation - rectangle.");
  }
}

// CheckAndFixCycleOrientationTest2 - single-part geode with correct
// orientation - obtuse angle is incident to lower-leftmost vertex.
TEST_F(PolygonCleanerTest, CheckAndFixCycleOrientationTest2) {
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

    CheckCleanPolygon(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: single-part geode with correct"
        " orientation and obtuse angle 1.");
  }
}


// CheckAndFixCycleOrientationTest3 - single-part geode with correct
// orientation - obtuse angle  is incident to lower-leftmost vertex.
TEST_F(PolygonCleanerTest, CheckAndFixCycleOrientationTest3) {
  gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  gstGeodeHandle exp_geodeh = gstGeodeImpl::Create(gstPolygon);
  gstGeode *exp_geode = static_cast<gstGeode*>(&(*exp_geodeh));

  exp_geode->AddPart(5);
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.050, .0));
  exp_geode->AddVertex(gstVertex(0.220, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.105, 0.200, .0));
  exp_geode->AddVertex(gstVertex(0.100, 0.100, .0));

  {
    geode->Clear();

    geode->AddPart(6);
    geode->AddVertex(gstVertex(0.105, 0.200, .0));
    geode->AddVertex(gstVertex(0.100, 0.100, .0));
    geode->AddVertex(gstVertex(0.220, 0.050, .0));
    geode->AddVertex(gstVertex(0.220, 0.200, .0));
    geode->AddVertex(gstVertex(0.105, 0.200, .0));

    CheckCleanPolygon(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: single-part geode with correct"
        " orientation and obtuse angle 2.");
  }
}


// CheckAndFixCycleOrientationTest4 - single-part geode with incorrect
// orientation - rectangle.
TEST_F(PolygonCleanerTest, CheckAndFixCycleOrientationTest4) {
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

    CheckCleanPolygon(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: single-part geode with incorrect"
        " orientation - rectangle.");
  }
}

// CheckAndFixCycleOrientationTest5 - single-part geode with incorrect
// orientation - obtuse angle is incident to lower-leftmost vertex.
TEST_F(PolygonCleanerTest, CheckAndFixCycleOrientationTest5) {
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

    CheckCleanPolygon(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: single-part geode with incorrect"
        " orientation - obtuse angle.");
  }
}

// CheckAndFixCycleOrientationTest6 - single-part geode with incorrect
// orientation - acute angle is incident to lower-leftmost vertex.
TEST_F(PolygonCleanerTest, CheckAndFixCycleOrientationTest6) {
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

    CheckCleanPolygon(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: single-part geode with incorrect"
        " orientation - acute angle 1.");
  }
}

// CheckAndFixCycleOrientationTest7 - single-part geode with incorrect
// orientation - acute angle is incident to lower-leftmost vertex.
TEST_F(PolygonCleanerTest, CheckAndFixCycleOrientationTest7) {
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

    CheckCleanPolygon(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: single-part geode with incorrect"
        " orientation - acute angle 2.");
  }
}

// CheckAndFixCycleOrientationTest8 - single-part geode with incorrect
// orientation - right angle is incident to lower-leftmost vertex.
TEST_F(PolygonCleanerTest, CheckAndFixCycleOrientationTest8) {
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

    CheckCleanPolygon(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: single-part geode with incorrect"
        " orientation - right angle.");
  }
}

// CheckAndFixCycleOrientationMultiPartGeodeTest1 - multi-part geode with
// incorrect orientation - rectangles.
TEST_F(PolygonCleanerTest, CheckAndFixCycleOrientationMultiPartGeodeTest1) {
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

    CheckCleanPolygon(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: multi-part geode with incorrect"
        " orientation - rectangles.");
  }
}

// CheckAndFixCycleOrientationMultiPartGeodeTest2 - multi-part geode with
// incorrect orientation - acute/obtuse angles are incident to lower-leftmost
// vertex.
TEST_F(PolygonCleanerTest, CheckAndFixCycleOrientationMultiPartGeodeTest2) {
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
  exp_geode->AddVertex(gstVertex(0.050, 0.150, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.050, 0.150, .0));

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

    CheckCleanPolygon(
        geodeh, exp_geodeh,
        "CheckAndFix cycle orientation: multi-part geode with incorrect"
        " orientation - acute/obtuse angles.");
  }
}


// CleanOverlappedEdgesTest1 - multi-parts geode with incorrect
// orientation(rectangle) and hole-cutting edge.
TEST_F(PolygonCleanerTest, CleanOverlappedEdgesTest1) {
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
    geode->AddPart(11);
    // outer cycle
    geode->AddVertex(gstVertex(0.020, 0.050, .0));
    // hole
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    // outer cycle
    geode->AddVertex(gstVertex(0.020, 0.050, .0));
    geode->AddVertex(gstVertex(0.020, 0.400, .0));
    geode->AddVertex(gstVertex(0.400, 0.400, .0));
    geode->AddVertex(gstVertex(0.400, 0.050, .0));
    geode->AddVertex(gstVertex(0.020, 0.050, .0));

    CheckCleanPolygon(
        geodeh, exp_geodeh,
        "Check overlapped edges cleaning: two-parts geode with incorrect"
        " orientation and hole-cutting edge.");
  }
}

// CleanOverlappedEdgesTest1 - multi-parts geode with incorrect
// orientation(rectangle), hole-cutting edges and overlapped edges in outer
// cycle.
TEST_F(PolygonCleanerTest, CleanOverlappedEdgesTest2) {
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
    geode->AddPart(13);
    // outer cycle
    geode->AddVertex(gstVertex(0.020, 0.050, .0));  // spike
    geode->AddVertex(gstVertex(0.020, 0.400, .0));
    geode->AddVertex(gstVertex(0.020, 0.050, .0));
    // hole
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    // outer cycle
    geode->AddVertex(gstVertex(0.020, 0.050, .0));
    geode->AddVertex(gstVertex(0.020, 0.400, .0));
    geode->AddVertex(gstVertex(0.400, 0.400, .0));
    geode->AddVertex(gstVertex(0.400, 0.050, .0));
    geode->AddVertex(gstVertex(0.020, 0.050, .0));

    CheckCleanPolygon(
        geodeh, exp_geodeh,
        "Check overlapped edges cleaning: multi-parts geode with incorrect"
        " orientation, hole-cutting edge and overlapped edges in outer cycle.");
  }
}

// CleanOverlappedEdgesTest3 - multi-parts geode with incorrect
// orientation(rectangle), hole-cutting edges and overlapped edges in outer/
// inner cycles(spike).
TEST_F(PolygonCleanerTest, CleanOverlappedEdgesTest3) {
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

  exp_geode->AddPart(4);
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));

  {
    geode->Clear();
    geode->AddPart(14);
    // outer cycle
    geode->AddVertex(gstVertex(0.020, 0.050, .0));  // spike
    geode->AddVertex(gstVertex(0.020, 0.400, .0));
    geode->AddVertex(gstVertex(0.020, 0.050, .0));
    // hole
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));  // spike
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    // outer cycle
    geode->AddVertex(gstVertex(0.020, 0.050, .0));
    geode->AddVertex(gstVertex(0.020, 0.400, .0));
    geode->AddVertex(gstVertex(0.400, 0.400, .0));
    geode->AddVertex(gstVertex(0.400, 0.050, .0));
    geode->AddVertex(gstVertex(0.020, 0.050, .0));

    CheckCleanPolygon(
        geodeh, exp_geodeh,
        "Check overlapped edges cleaning: multi-parts geode with incorrect"
        " orientation, hole-cutting edge and overlapped edges"
        " in outer/inner cycles.");
  }
}

// CleanOverlappedEdgesTest4 - multi-parts geode with incorrect
// orientation(rectangle), hole-cutting edges and overlapped edges
// in outer/inner cycles(spike).
TEST_F(PolygonCleanerTest, CleanOverlappedEdgesTest4) {
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

  exp_geode->AddPart(4);
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));

  {
    geode->Clear();
    geode->AddPart(14);
    // outer cycle
    geode->AddVertex(gstVertex(0.020, 0.050, .0));  // spike
    geode->AddVertex(gstVertex(0.020, 0.400, .0));
    geode->AddVertex(gstVertex(0.020, 0.050, .0));
    // hole
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.200, 0.120, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));  // spike
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    // outer cycle
    geode->AddVertex(gstVertex(0.020, 0.050, .0));
    geode->AddVertex(gstVertex(0.020, 0.400, .0));
    geode->AddVertex(gstVertex(0.400, 0.400, .0));  // spike
    geode->AddVertex(gstVertex(0.450, 0.450, .0));
    geode->AddVertex(gstVertex(0.400, 0.400, .0));
    geode->AddVertex(gstVertex(0.400, 0.050, .0));
    geode->AddVertex(gstVertex(0.020, 0.050, .0));

    CheckCleanPolygon(
        geodeh, exp_geodeh,
        "Check overlapped edges cleaning: multi-parts geode with incorrect"
        " orientation, hole-cutting edge and overlapped edges"
        " in outer/inner cycles.");
  }
}

// CleanOverlappedEdgesTest5 - multi-parts geode with incorrect
// orientation(rectangle), hole-cutting edges and overlapped edges
// in outer/inner cycles(spike).
TEST_F(PolygonCleanerTest, CleanOverlappedEdgesTest5) {
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
  exp_geode->AddVertex(gstVertex(0.040, 0.070, .0));
  exp_geode->AddVertex(gstVertex(0.120, 0.120, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.180, .0));
  exp_geode->AddVertex(gstVertex(0.200, 0.070, .0));
  exp_geode->AddVertex(gstVertex(0.040, 0.070, .0));

  {
    geode->Clear();
    geode->AddPart(17);
    // outer cycle
    geode->AddVertex(gstVertex(0.020, 0.050, .0));  // spike
    geode->AddVertex(gstVertex(0.020, 0.400, .0));
    geode->AddVertex(gstVertex(0.020, 0.050, .0));
    geode->AddVertex(gstVertex(0.400, 0.050, .0));
    geode->AddVertex(gstVertex(0.400, 0.400, .0));  // spike
    geode->AddVertex(gstVertex(0.450, 0.450, .0));
    geode->AddVertex(gstVertex(0.400, 0.400, .0));
    // hole
    geode->AddVertex(gstVertex(0.200, 0.180, .0));  // spike
    geode->AddVertex(gstVertex(0.120, 0.180, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    geode->AddVertex(gstVertex(0.120, 0.120, .0));
    geode->AddVertex(gstVertex(0.040, 0.070, .0));
    geode->AddVertex(gstVertex(0.200, 0.070, .0));
    geode->AddVertex(gstVertex(0.200, 0.180, .0));
    // outer cycle
    geode->AddVertex(gstVertex(0.400, 0.400, .0));
    geode->AddVertex(gstVertex(0.020, 0.400, .0));
    geode->AddVertex(gstVertex(0.020, 0.050, .0));

    CheckCleanPolygon(
        geodeh, exp_geodeh,
        "Check overlapped edges cleaning: multi-parts geode with incorrect"
        " orientation, hole-cutting edge and overlapped edges"
        " in outer/inner cycles.");
  }
}

// TODO: add more tests for cleaner.

}  // namespace fusion_gst


int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

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

#include "fusion/gst/gstUnitTestUtils.h"

#include <gtest/gtest.h>

#include <assert.h>
#include <iostream>
#include <string>

#include "common/notify.h"
#include "fusion/gst/gstTypes.h"

std::ostream& operator<<(std::ostream& os, const gstVertex& v) {
  return os << "x= " << v.x << ", y= " << v.y << ", z= " << v.z << ";"
            << std::endl;
}

std::ostream& operator<<(std::ostream& os, const gstGeodeHandle &geodeh) {
  assert(!IsMultiGeodeType((*geodeh).PrimType()));
  const gstGeode *geode = static_cast<const gstGeode*>(&(*geodeh));
  os << "Geode:" << std::endl;
  for (unsigned int p = 0; p < geode->NumParts(); ++p) {
    os << "part:" << p << std::endl;
    for (unsigned int v = 0; v < geode->VertexCount(p); ++v) {
      os << geode->GetVertex(p, v);
    }
  }
  return os;
}

namespace {

void CompareSingleGeodes(const gstGeodeHandle &geodeh,
                         const gstGeodeHandle &expected_geodeh,
                         const std::string &message) {
  if (IsMultiGeodeType((*geodeh).PrimType())) {
    notify(NFY_FATAL, "Not valid geode type: %d", (*geodeh).PrimType());
  }

  if (IsMultiGeodeType((*expected_geodeh).PrimType())) {
    notify(NFY_FATAL, "Not valid expected geode type: %d",
           (*expected_geodeh).PrimType());
  }

  const gstGeode *geode = static_cast<const gstGeode*>(&(*geodeh));

  const gstGeode *expected_geode =
      static_cast<const gstGeode*>(&(*expected_geodeh));

  EXPECT_EQ(expected_geode->NumParts(), geode->NumParts()) << message;
  EXPECT_EQ(expected_geode->TotalVertexCount(), geode->TotalVertexCount()) <<
      message;

  for (unsigned int part = 0; part < expected_geode->NumParts() &&
           part < geode->NumParts(); ++part) {
    for (unsigned int v = 0; v < expected_geode->VertexCount(part) &&
             v < geode->VertexCount(part) ; ++v) {
      const gstVertex &expected_pt = expected_geode->GetVertex(part, v);
      const gstVertex &pt = geode->GetVertex(part, v);
      ASSERT_PRED2(
          fusion_gst::TestUtils::gstVertexEqualXY, expected_pt, pt) << message;
    }
  }
}

void CompareMultiGeodes(const gstGeodeHandle &geodeh,
                        const gstGeodeHandle &expected_geodeh,
                        const std::string &message) {
  if (!IsMultiGeodeType((*geodeh).PrimType())) {
    notify(NFY_FATAL, "Not valid geode type: %d", (*geodeh).PrimType());
  }

  if (!IsMultiGeodeType((*expected_geodeh).PrimType())) {
    notify(NFY_FATAL, "Not valid expected geode type: %d",
           (*expected_geodeh).PrimType());
  }

  const gstMultiPoly *multi_geode =
      static_cast<const gstMultiPoly*>(&(*geodeh));

  const gstMultiPoly *expected_multi_geode =
      static_cast<const gstMultiPoly*>(&(*expected_geodeh));

  for (unsigned int part = 0; part < expected_multi_geode->NumParts() &&
           part < multi_geode->NumParts(); ++part) {
    const gstGeodeHandle &geodeh_p = multi_geode->GetGeode(part);
    const gstGeodeHandle &expected_geodeh_p =
        expected_multi_geode->GetGeode(part);

    CompareSingleGeodes(geodeh_p, expected_geodeh_p, message);
  }
}
}  // namespace


namespace fusion_gst {

void TestUtils::PrintGeode(const gstGeodeHandle &geodeh) {
  if (IsMultiGeodeType((*geodeh).PrimType())) {
    const gstMultiPoly *multi_geode =
        static_cast<const gstMultiPoly*>(&(*geodeh));
    for (unsigned int part = 0; part < multi_geode->NumParts(); ++part) {
      const gstGeodeHandle &geodeh_p = multi_geode->GetGeode(part);
      std::cout << geodeh_p << std::endl;
    }
  } else {
    std::cout << geodeh << std::endl;
  }
}

void TestUtils::CompareGeodes(const gstGeodeHandle &geodeh,
                              const gstGeodeHandle &expected_geodeh,
                              const std::string &message) {
  if ((*geodeh).PrimType() != (*expected_geodeh).PrimType()) {
    notify(NFY_FATAL,
           "Geodes have different type: geode: %d, expected_geode: %d.",
           (*geodeh).PrimType(),
           (*expected_geodeh).PrimType());
  }

  if (IsMultiGeodeType((*geodeh).PrimType())) {
    CompareMultiGeodes(geodeh, expected_geodeh, message);
  } else {
    CompareSingleGeodes(geodeh, expected_geodeh, message);
  }
}

}  // namespace fusion_gst

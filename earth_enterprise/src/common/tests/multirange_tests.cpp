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

//
// Unit tests for geSegmentedArray

#include <cstdint>
#include <geMultiRange.h>
#include <UnitTest.h>


class geMultiRangeTest : public UnitTest<geMultiRangeTest> {
 public:

  bool EmptyConstruction(void) {
    geMultiRange<std::uint32_t> empty_range;
    return (empty_range.AsString() == "");
  }
  bool SimpleConstruction(void) {
    geMultiRange<std::uint32_t> range(4,8);
    return
      ((range.AsString() == "[4,8)") &&
       (geMultiRange<std::uint32_t>(geRange<std::uint32_t>(5,10)).AsString() == "[5,11)") &&
       (geMultiRange<std::uint32_t>(0, 24).AsString() == "[0,24)"));
  }
  bool Union(void) {
    geMultiRange<std::uint32_t> empty_range;
    geMultiRange<std::uint32_t> range_4_8(4,8);
    geMultiRange<std::uint32_t> range_6_12(6,12);
    geMultiRange<std::uint32_t> range_8_10(8,10);
    geMultiRange<std::uint32_t> range_9_14(9,14);

    return
      ((geMultiRange<std::uint32_t>::Union(empty_range, range_4_8).AsString() ==
        "[4,8)") &&
       (geMultiRange<std::uint32_t>::Union(range_4_8, range_6_12).AsString() ==
        "[4,12)") &&
       (geMultiRange<std::uint32_t>::Union(range_4_8, range_8_10).AsString() ==
        "[4,10)") &&
       (geMultiRange<std::uint32_t>::Union(range_4_8, range_9_14).AsString() ==
        "[4,8), [9,14)"));
  }
  bool Contains(void) {
    geMultiRange<std::uint32_t> empty_range;
    geMultiRange<std::uint32_t> range_4_8(4,8);
    geMultiRange<std::uint32_t> range_9_14(9,14);
    geMultiRange<std::uint32_t> multi_range = geMultiRange<std::uint32_t>::Union(range_4_8,
                                                                   range_9_14);

    return (!empty_range.Contains(0) &&
            !empty_range.Contains(1) &&
            !empty_range.Contains(14) &&

            !range_4_8.Contains(0) &&
            !range_4_8.Contains(1) &&
            !range_4_8.Contains(3) &&
            range_4_8.Contains(4) &&
            range_4_8.Contains(7) &&
            !range_4_8.Contains(8) &&
            !range_4_8.Contains(24) &&

            !multi_range.Contains(0) &&
            !multi_range.Contains(1) &&
            !multi_range.Contains(3) &&
            multi_range.Contains(4) &&
            multi_range.Contains(7) &&
            !multi_range.Contains(8) &&
            multi_range.Contains(9) &&
            multi_range.Contains(13) &&
            !multi_range.Contains(14) &&
            !multi_range.Contains(24));
  }



  geMultiRangeTest(void) : BaseClass("geMultiRangeTest") {
    REGISTER(EmptyConstruction);
    REGISTER(SimpleConstruction);
    REGISTER(Union);
    REGISTER(Contains);
  }
};

int main(int argc, char *argv[]) {
  geMultiRangeTest tests;

  return tests.Run();
}

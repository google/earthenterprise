// Copyright 2018 The Open GEE Contributors
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

#include <gtest/gtest.h>
#include <time.h>

#include "timeutils.h"

using namespace getime;

timespec makeTimespec(time_t sec, time_t nsec) {
  timespec result;
  result.tv_sec = sec;
  result.tv_nsec = nsec;
  return result;
}

void testTimespecDiff(time_t x_sec, time_t x_nsec,
                      time_t y_sec, time_t y_nsec,
                      time_t exp_sec, time_t exp_nsec) {
  timespec x = makeTimespec(x_sec, x_nsec);
  timespec y = makeTimespec(y_sec, y_nsec);
  timespec result = timespecDiff(x, y);
  EXPECT_EQ(exp_sec, result.tv_sec);
  EXPECT_EQ(exp_nsec, result.tv_nsec);
}

TEST(TimeUtilsTest, TimespecDiff_Same) {
  testTimespecDiff(15, 320, 15, 320, 0, 0);
}
TEST(TimeUtilsTest, TimespecDiff_NSec) {
  testTimespecDiff(10, 123, 10, 122, 0, 1);
}
TEST(TimeUtilsTest, TimespecDiff_Sec) {
  testTimespecDiff(16, 555, 12, 555, 4, 0);
}
TEST(TimeUtilsTest, TimespecDiff_Both) {
  testTimespecDiff(2136, 87957, 1917, 14809, 219, 73148);
}
TEST(TimeUtilsTest, TimespecDiff_Borrow) {
  testTimespecDiff(10, 1, 9, 999999999, 0, 2);
}
TEST(TimeUtilsTest, TimespecDiff_Borrow2){
  testTimespecDiff(12, 3, 8, 567, 3, 999999436);
}
TEST(TimeUtilsTest, TimespecDiff_Large) {
  testTimespecDiff(987654321, 999999999, 987654320, 999999998, 1, 1);
}
TEST(TimeUtilsTest, TimespecDiff_SmallMinusLarge) {
  EXPECT_DEATH_IF_SUPPORTED(timespecDiff(makeTimespec(10, 0), makeTimespec(11, 0)), ".*");
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

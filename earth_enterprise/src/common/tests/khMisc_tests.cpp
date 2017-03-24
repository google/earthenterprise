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

// Unit tests for khMisc.

#include <iostream>
#include <string>
#include <gtest/gtest.h>
#include "common/khMisc.h"

class khMiscTest : public testing::Test {
 protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST_F(khMiscTest, MillisecondsFromMidnight) {
  struct tm time;
  strptime("2016-03-10T16:50:36", "%Y-%m-%dT%H:%M:%S", &time);
  EXPECT_EQ(60636000, MillisecondsFromMidnight(time));
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

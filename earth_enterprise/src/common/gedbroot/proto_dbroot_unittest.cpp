// Copyright 2020 Google Inc.
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

#include "proto_dbroot.h"

// Test various error conditions when loading proto dbroots

TEST(ProtoDbroot, ReadEncoded) {
  geProtoDbroot dbroot("fusion/testdata/dbroot/proto/dbroot.v5p.DEFAULT", geProtoDbroot::kEncodedFormat);
  ASSERT_TRUE(dbroot.IsValid());
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

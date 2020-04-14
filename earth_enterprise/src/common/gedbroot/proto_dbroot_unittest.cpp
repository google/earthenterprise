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
#include "common/khSimpleException.h"

// Test various error and non-error conditions when loading proto dbroots.
// Since we're validating the logic for loading the dbroots, we don't worry too
// much about the contents.

TEST(ProtoDbroot, InvalidFile) {
  ASSERT_THROW(geProtoDbroot dbroot("notafile", geProtoDbroot::kProtoFormat), khSimpleErrnoException);
}

TEST(ProtoDbroot, ReadEncoded) {
  geProtoDbroot dbroot("fusion/testdata/dbroot/proto/dbroot.v5p.encoded.DEFAULT", geProtoDbroot::kEncodedFormat);
  ASSERT_TRUE(dbroot.IsValid());
}

TEST(ProtoDbroot, ReadDecoded) {
  geProtoDbroot dbroot("fusion/testdata/dbroot/proto/dbroot.v5p.decoded.DEFAULT", geProtoDbroot::kProtoFormat);
  ASSERT_TRUE(dbroot.IsValid());
}

TEST(ProtoDbroot, ReadText) {
  geProtoDbroot dbroot("fusion/testdata/dbroot/proto/dbroot.v5p.text.DEFAULT", geProtoDbroot::kTextFormat);
  ASSERT_TRUE(dbroot.IsValid());
}

TEST(ProtoDbroot, ReadTextInvalid) {
  // Try to read an encoded dbroot as a text dbroot to trigger an error
  ASSERT_THROW(geProtoDbroot dbroot("fusion/testdata/dbroot/proto/dbroot.v5p.encoded.DEFAULT", geProtoDbroot::kTextFormat),
      khSimpleException);
}

TEST(ProtoDbroot, ReadDecodedInvalid) {
  // Try to read an encoded dbroot as a decoded dbroot to trigger an error
  ASSERT_THROW(geProtoDbroot dbroot("fusion/testdata/dbroot/proto/dbroot.v5p.encoded.DEFAULT", geProtoDbroot::kProtoFormat),
      khSimpleException);
}

TEST(ProtoDbroot, ReadEncodedInvalid) {
  // Try to read a decoded dbroot as an encoded dbroot to trigger an error
  ASSERT_THROW(geProtoDbroot dbroot("fusion/testdata/dbroot/proto/dbroot.v5p.decoded.DEFAULT", geProtoDbroot::kEncodedFormat),
      khSimpleException);
}

TEST(ProtoDbroot, DiffKey) {
  // This dbroot was encoded with a non-default key
  geProtoDbroot dbroot("fusion/testdata/dbroot/proto/dbroot.v5p.diffkey.DEFAULT", geProtoDbroot::kEncodedFormat);
  ASSERT_TRUE(dbroot.IsValid());
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

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


// Unit tests for QuadtreePath class

#include <stdio.h>
#include <string>
#include <protobuf/quadtreeset.pb.h>
#include <UnitTest.h>

class ProtoBufUnitTest : public UnitTest<ProtoBufUnitTest> {
 public:

  ProtoBufUnitTest(void) : BaseClass("ProtoBufUnitTest") {
    REGISTER(TestProtocolBufferVersion);
    REGISTER(TestQuadTreeSet);
  }

  bool TestProtocolBufferVersion() {
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    return true;
  }
  // Test the quadtreeset protobuf
  bool TestQuadTreeSet() {

    ::google::protobuf::int32 channel_type = 1010;
    ::google::protobuf::int32 channel_epoch = 1010;
    keyhole::QuadtreeChannel channel;
    channel.set_type(channel_type);
    channel.set_channel_epoch(channel_epoch);

    std::string serialized;
    channel.SerializeToString(&serialized);

    keyhole::QuadtreeChannel channel2;
    TestAssert(channel2.type() != channel_type);
    TestAssert(channel2.channel_epoch() != channel_epoch);

    channel2.ParseFromString(serialized);
    TestEquals(channel2.type(), channel_type);
    TestEquals(channel2.channel_epoch(), channel_epoch);
    return true;
  }

};


int main(int argc, char *argv[]) {
  ProtoBufUnitTest tests;
  return tests.Run();
}

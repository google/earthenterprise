/*
 * Copyright 2021 The Open GEE Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "portableservice.h"

#include <gtest/gtest.h>

#include "httpd.h"
#include <sstream>

// Define some functions that Apache normally provides
void ap_internal_redirect(const char * new_uri, request_rec * r) {}
void ap_log_rerror(const char *file, int line, int module_index, int level, apr_status_t status, const request_rec *r, const char *fmt,...) {}
void ap_set_last_modified(request_rec *r) {}
void ap_hook_handler(ap_HOOK_handler_t *pf, const char * const *aszPre, const char * const *aszSucc, int inorder) {}
apr_time_t apr_date_parse_http(const char *date) { return 0; }

// Intercept data that is written out
std::stringstream write_data;
int ap_rwrite (const void *buf, int nbyte, request_rec *r) {
  write_data.write(static_cast<const char *>(buf), nbyte);
  return nbyte;
}

const std::string TARGET = "target";
const std::string GLOBE = "portable/test_data/test.glb";

class PortableServiceTest : public testing::Test {
  protected:
    PortableService service;
    request_rec rec;
    request_rec *r = &rec;
    ArgMap args;
    PortableServiceTest() {
      service.RegisterPortable(r, TARGET, GLOBE);
      write_data.clear();
    }
    void TestBadRequest(
      request_rec* r,
      const std::string& source_cmd_or_path,
      const std::string& target_path,
      const std::string& suffix,
      int layer_id,
      bool is_balloon_request,
      ArgMap& arg_map,
      const std::string& no_value_arg,
      fusion_portableglobe::CutSpec* cutspec) {
        TestRequest(
          r,
          source_cmd_or_path,
          target_path, suffix,
          layer_id,
          is_balloon_request,
          arg_map,
          no_value_arg,
          cutspec,
          HTTP_NOT_FOUND);
      }
    void TestGoodRequest(
      request_rec* r,
      const std::string& source_cmd_or_path,
      const std::string& target_path,
      const std::string& suffix,
      int layer_id,
      bool is_balloon_request,
      ArgMap& arg_map,
      const std::string& no_value_arg,
      fusion_portableglobe::CutSpec* cutspec,
      uint64_t expected_data) {
        TestRequest(
          r,
          source_cmd_or_path,
          target_path, suffix,
          layer_id,
          is_balloon_request,
          arg_map,
          no_value_arg,
          cutspec,
          OK);

        // Check the first few bytes of data written to make sure we're writing
        // out the correct data.
        uint64_t written_data;
        write_data.read(reinterpret_cast<char *>(&written_data), sizeof(written_data));
        ASSERT_EQ(expected_data, written_data);
      }
  private:
    void TestRequest(
      request_rec* r,
      const std::string& source_cmd_or_path,
      const std::string& target_path,
      const std::string& suffix,
      int layer_id,
      bool is_balloon_request,
      ArgMap& arg_map,
      const std::string& no_value_arg,
      fusion_portableglobe::CutSpec* cutspec,
      int expected_status) {
        auto status = service.ProcessPortableRequest(
          r,
          source_cmd_or_path,
          target_path, suffix,
          layer_id,
          is_balloon_request,
          arg_map,
          no_value_arg,
          cutspec);
        ASSERT_EQ(expected_status, status);
      }
};

TEST_F(PortableServiceTest, NoUnpacker) {
  TestBadRequest(r, "command", "wrongtarget", GLOBE, 0, false, args, "", nullptr);
}

TEST_F(PortableServiceTest, FlatfileQueryNoArgs) {
  TestBadRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "query", nullptr);
}

TEST_F(PortableServiceTest, FlatfileQuery) {
  args.emplace("blist", "0301324");
  args.emplace("request", "QTPacket");
  args.emplace("version", "1");
  TestGoodRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "query", nullptr, 0x656c626174726f50);
}

TEST_F(PortableServiceTest, FlatfileQueryMulti) {
  args.emplace("blist", "03013240301334");
  args.emplace("request", "QTPacket");
  args.emplace("version", "1");
  TestGoodRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "query", nullptr, 0x3a65636976726553);
}

TEST_F(PortableServiceTest, FlatfileQueryNoPath) {
  args.emplace("request", "QTPacket");
  args.emplace("version", "1");
  TestGoodRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "query", nullptr, 0x3a74656772617420);
}

TEST_F(PortableServiceTest, FlatfileQueryRawPath) {
  args.emplace("blist", "");
  args.emplace("request", "ImageryGE");
  args.emplace("version", "3");
  args.emplace("channel", "1000");
  TestGoodRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "query", nullptr, 0x6573616261746164);
}

TEST_F(PortableServiceTest, FlatfileQueryTerrain) {
  args.emplace("blist", "0301324");
  args.emplace("request", "Terrain");
  args.emplace("version", "3");
  args.emplace("channel", "1001");
  TestGoodRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "query", nullptr, 0x7465677261742720);
}

TEST_F(PortableServiceTest, FlatfileQueryVector) {
  args.emplace("blist", "0301324");
  args.emplace("request", "VectorGE");
  args.emplace("version", "3");
  args.emplace("channel", "1002");
  TestGoodRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "query", nullptr, 0x6c626174726f703a);
}

TEST_F(PortableServiceTest, FlatfileNoRequest) {
  TestBadRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "", nullptr);
}

TEST_F(PortableServiceTest, FlatfileEncodedNoDot) {
  TestBadRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "f1c-03012-i", nullptr);
}

TEST_F(PortableServiceTest, FlatfileEncodedInvalidType) {
  TestBadRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "f1c-030132-x.3", nullptr);
}

TEST_F(PortableServiceTest, FlatfileEncodedImagery) {
  TestGoodRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "f1c-030132-i.3", nullptr, 0x645f747365742f65);
}

TEST_F(PortableServiceTest, FlatfileEncodedTerrain) {
  TestGoodRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "f1c-030132030-t.3", nullptr, 0x747365742f617461);
}

TEST_F(PortableServiceTest, FlatfileEncodedVector) {
  TestGoodRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "f1c-03013203-d.5.2", nullptr, 0x65722027626c672e);
}

TEST_F(PortableServiceTest, FlatfileEncodedQuery) {
  TestGoodRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "q2-03013203-q.3", nullptr, 0x6465726574736967);
}

TEST_F(PortableServiceTest, FlatfileEncodedBadTerrain) {
  TestBadRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "f1c-030132-t.3", nullptr);
}

TEST_F(PortableServiceTest, FlatfileEncodedBadVector) {
  TestBadRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "f1c-030132-d.5", nullptr);
}

TEST_F(PortableServiceTest, FlatfileEncodedBadPrefix) {
  TestBadRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "abc-03013203-q.3", nullptr);
}

TEST_F(PortableServiceTest, FlatfileIconRequest) {
  TestGoodRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "lf-0-icons/773_l.png", nullptr, 0x72657320726f6620);
}

TEST_F(PortableServiceTest, FlatfileBadIconRequest) {
  TestBadRequest(r, "flatfile", TARGET, GLOBE, 2, false, args, "lf-0-icons/773_l.png", nullptr);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

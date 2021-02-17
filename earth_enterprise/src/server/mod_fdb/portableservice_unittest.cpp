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

// Define some functions that Apache normally provides
int ap_rwrite (const void *buf, int nbyte, request_rec *r) { return nbyte; }
void ap_internal_redirect(const char * new_uri, request_rec * r) {}
void ap_log_rerror(const char *file, int line, int module_index, int level, apr_status_t status, const request_rec *r, const char *fmt,...) {}
void ap_set_last_modified(request_rec *r) {}
void ap_hook_handler(ap_HOOK_handler_t *pf, const char * const *aszPre, const char * const *aszSucc, int inorder) {}
apr_time_t apr_date_parse_http(const char *date) { return 0; }

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
        auto status = service.ProcessPortableRequest(
          r,
          source_cmd_or_path,
          target_path, suffix,
          layer_id,
          is_balloon_request,
          arg_map,
          no_value_arg,
          cutspec);
        ASSERT_EQ(status, HTTP_NOT_FOUND);
      }
};

TEST_F(PortableServiceTest, NoUnpacker) {
  TestBadRequest(r, "command", "wrongtarget", GLOBE, 0, false, args, "", nullptr);
}

TEST_F(PortableServiceTest, FlatfileQueryNoArgs) {
  TestBadRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "query", nullptr);
}

TEST_F(PortableServiceTest, FlatfileNoRequest) {
  TestBadRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "", nullptr);
}

TEST_F(PortableServiceTest, FlatfileRequestNoDot) {
  TestBadRequest(r, "flatfile", TARGET, GLOBE, 0, false, args, "f1c-03012-i", nullptr);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

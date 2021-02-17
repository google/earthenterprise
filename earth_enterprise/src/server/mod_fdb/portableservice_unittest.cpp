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

// Define some functions that Apache normally provides
int ap_rwrite (const void *buf, int nbyte, request_rec *r) { return nbyte; }
void ap_internal_redirect(const char * new_uri, request_rec * r) {}
void ap_log_rerror(const char *file, int line, int module_index, int level, apr_status_t status, const request_rec *r, const char *fmt,...) {}
void ap_set_last_modified(request_rec *r) {}
void ap_hook_handler(ap_HOOK_handler_t *pf, const char * const *aszPre, const char * const *aszSucc, int inorder) {}
apr_time_t apr_date_parse_http (const char *date) { return 0; }

class PortableServiceTest : public testing::Test {
  protected:
    PortableService service;
};

TEST_F(PortableServiceTest, DoFlatfile) {
  ArgMap map;
  service.ProcessPortableRequest(nullptr, "", "", "", 0, false, map, "", nullptr);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

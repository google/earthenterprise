/*
 * Copyright 2017 Google Inc.
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


#ifndef GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_MOD_FDB_IMPL_H_
#define GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_MOD_FDB_IMPL_H_

#include <httpd.h>
#include <http_config.h>

#ifdef __cplusplus
extern "C" {
#endif

int HandleFdbRequest(request_rec* r);
const char* SetFdbParams(
    cmd_parms* cmd, void *mconfig, const char* globes_path);

#ifdef __cplusplus
}
#endif


#endif  // GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_MOD_FDB_IMPL_H_

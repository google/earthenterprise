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

// TODO: Remove gedb and glc modules when this is stable.

#include "httpd.h"
#include "http_core.h"
#include "server/mod_fdb/mod_fdb_impl.h"

// Specify the Request Handler for the Fdb Apache module.
static void register_hooks (apr_pool_t* p) {
  ap_hook_handler(HandleFdbRequest, NULL, NULL, APR_HOOK_MIDDLE);
}

// Fdb is the Apache command for configuring a fdb server.
// It has a single argument that specifies the absolute path to the
// glx directory.
// TODO: add an argument for Fusion publish root. 
static const command_rec fdb_cmds[] =
{
  // "1" at end refers to number of parameters.
  AP_INIT_TAKE1("Fdb", SetFdbParams, NULL, ACCESS_CONF|RSRC_CONF,
                "Globe directory"),
  { NULL }
};

// Declare the Fdb Apache Module
module AP_MODULE_DECLARE_DATA fdb_module = {
  STANDARD20_MODULE_STUFF,
  NULL,
  NULL,
  NULL,
  NULL,
  fdb_cmds,
  register_hooks,
};

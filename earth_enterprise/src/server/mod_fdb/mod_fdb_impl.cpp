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


#include "server/mod_fdb/mod_fdb_impl.h"
#include <http_config.h>
#include <http_log.h>
#include <http_protocol.h>
#include <http_request.h>
#include <string>
#include <map>
#include "common/khEndian.h"
#include "common/khFileUtils.h"
#include "server/mod_fdb/fdbservice.h"

// Declare the fdb_module here for the command handler to retrieve correct
// per dir config in the handler.
#ifdef APLOG_USE_MODULE
APLOG_USE_MODULE(fdb);
#else
extern module AP_MODULE_DECLARE_DATA fdb_module;
#endif

//////////////////////////////////////////////////////////////////////////////
// These functions are called within the apache module which is why they are
// extern "C".

extern "C" {
/**
 * Handles all apache requests on the fdb directory.
 */
int HandleFdbRequest(request_rec* r) {
  // ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r, "HandleFdbRequest");
  try {
    const std::string kFdbHandler = "fdb-handler";
    if (!r) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "No request.");
      return DECLINED;
    }

    if (kFdbHandler != r->handler) {
      return DECLINED;
    }

    if (!r->path_info) {
      ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r, "No path info.");
      return DECLINED;
    }

    return fdb_service.ProcessRequest(r);
  } catch(const khSimpleNotFoundException &e) {
    // We don't want to log the tile not found errors coz the access logs
    // already report that.
    // ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "Tile not found");
    return HTTP_NOT_FOUND;
  } catch(const std::exception &e) {
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                  "Caught exception in handle request: Error:%s", e.what());
    return DECLINED;
  } catch(...) {
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                  "Caught unknown exception in handle request");
  }
  return HTTP_INTERNAL_SERVER_ERROR;
}

/**
 * Handles input from
 * /opt/google/gehttpd/conf.d/virtual_servers/runtime/default_fdb_runtime
 * to set the base path for the globes.
 */
const char* SetFdbParams(cmd_parms* cmd,
                         void* dir_config,
                         const char* globes_path) {
  std::string path(globes_path);
  fdb_service.SetGlobeDirectory(path);
  return NULL;
}
} /* extern "C" */

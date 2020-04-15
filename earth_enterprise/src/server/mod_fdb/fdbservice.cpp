// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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


#include "server/mod_fdb/fdbservice.h"

#include <string.h>
#include <http_config.h>
#include <http_request.h>
#include <http_log.h>
#include <http_protocol.h>
#include <ap_compat.h>
#include <apr_date.h>
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <map>
#include <string>
#include "common/geFilePool.h"
#include "common/khConstants.h"
#include "common/khFileUtils.h"
#include "common/khStringUtils.h"
#include "common/serverdb/serverdbReader.h"
#include "server/mod_fdb/fusiondbservice.h"
#include "server/mod_fdb/portableservice.h"

#ifdef APLOG_USE_MODULE
APLOG_USE_MODULE(fdb);
#endif


// The service for processing a fdb Apache request.
FdbService fdb_service;

// FdbService constructor.
FdbService::FdbService() {
  // Set whether cutting enabled into Reader/Unpacker managers.
  // Note: initializing, it is set based on executable status of
  // geportableglobebuilder.
  DoSetCuttingEnabled(
      khIsExecutable("/opt/google/bin/geportableglobebuilder"));
}

// Destructor. Deallocate cut specs.
FdbService::~FdbService() {
  std::map<std::string, fusion_portableglobe::CutSpec*>::iterator it;
  for (it = cut_specs_.begin(); it != cut_specs_.end(); ++it) {
    if (it->second) {
      delete it->second;
      it->second = NULL;
    }
  }
}

void FdbService::SetGlobeDirectory(const std::string& globe_directory) {
  portable_service_.SetGlobeDirectory(globe_directory);
}

int FdbService::DoReset(request_rec* r) {
  fusion_db_service_.Reset();
  portable_service_.Reset();
  std::string data;
  data.append("Reset FdbService done.");
  ap_rwrite(&data[0], data.size(), r);
  return OK;
}

// Handles publish Fusion DB request.
int FdbService::DoPublishFusionDb(request_rec* r,
                                  const std::string& target_path,
                                  const std::string& db_path) {
  return fusion_db_service_.RegisterFusionDb(r, target_path, db_path);
}

// Handles publish portable request.
int FdbService::DoPublishPortable(request_rec* r,
                                  const std::string& target_path,
                                  const std::string& db_path) {
  return portable_service_.RegisterPortable(r, target_path, db_path);
}

// Handles unpublish target request.
// Note: If the reader/unpacker is preloaded, it is removed and deleted.
int FdbService::DoUnpublish(request_rec* r,
                            const std::string& target_path) {
  if (fusion_db_service_.UnregisterFusionDb(r, target_path)) {
    return OK;
  } else if (portable_service_.UnregisterPortable(r, target_path)) {
    return OK;
  }

  ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                "Target path %s not registered for serving.",
                target_path.c_str());
  return OK;
}

/**
 * Breaks up args portion of url into key/value pairs. Also returns
 * single key with no associated value (e.g. f1c-03012012-i.18).
 */
void FdbService::ParseUrlArgs(const std::string& arg_str,
                              ArgMap* arg_map,
                              std::string* no_value_arg) {
  std::vector<std::string> tokens;
  TokenizeString(arg_str, tokens, "&");
  std::string name;
  std::string value;
  for (unsigned int i = 0; i < tokens.size(); ++i) {
    const std::string& token = tokens[i];
    size_t pos = token.find("=");
    if (pos == std::string::npos) {
      *no_value_arg = token;
    } else {
      name = token.substr(0, pos);
      value = token.substr(pos+1);
      (*arg_map)[name] = value;
    }
  }
}

/**
 * Parses path of url into target path and residual component
 * after the target path.
 *
 * Examples:
 *  General request
 *   path: "/statusz"
 *   target_path: ""
 *   command: "statusz"
 *   residual: ""

 *  General request
 *   path: "/add_cut_spec"
 *   target_path: ""
 *   command: "add_cut_spec"
 *   residual: ""
 *
 *  General request
 *   path: "/publish"
 *   target_path: ""
 *   command: "publish"
 *   residual: ""
 *
 *  General request
 *   path: "/unpublish"
 *   target_path: ""
 *   command: "publish"
 *   residual: ""
 *
 *  Glb status request
 *   path: "/my_portable_globe/db/statusz"
 *   target_path: "/my_portable_globe"
 *   command: ""
 *   residual: "statusz"
 *
 *  Glb packet request
 *   path: "/my_portable_globe/db/flatfile"
 *   target_path: "/my_portable_globe"
 *   command: ""
 *   residual: "flatfile"
 *
 *  Glm packet request
 *   path: "/my_portable_map/db/query"
 *   target_path: "/my_portable_map"
 *   command: ""
 *   residual: "query"
 *
 *  Glc base globe packet request
 *   path: "/my_portable_globe/db/flatfile"
 *   target_path: "/my_portable_globe"
 *   command: ""
 *   residual: "flatfile"
 *
 *  Glc layer packet request
 *   path: "/my_globe_glc/3/flatfile"
 *   target_path: "/my_globe_glc"
 *   command: ""
 *   residual: "3/flatfile"
 *
 *  Fusion DB packet request
 *   path: "/my_fusion_db/db/flatfile"
 *   target_path: "/my_fusion_db"
 *   command: ""
 *   residual: "flatfile"
 */
void FdbService::ParsePath(const std::string& path,
                           std::string* target_path,
                           std::string* command,
                           std::string* residual) {
  if (path == "") {
    return;
  }

  // If no "/", put the command in target_path and return.
  size_t index = path.find("/", 1);
  if (index == std::string::npos) {
    *command = path.substr(1);  // command
  // Otherwise, first item is the target path.
  } else {
    size_t target_path_index = path.find(kTargetPathSeparator);
    if (target_path_index == std::string::npos) {
      return;
    }

    *target_path = path.substr(0, target_path_index);
    index = target_path_index + ::strlen(kTargetPathSeparator);
    *residual = path.substr(index);
  }
}

/**
 * Returns whether all args are in the arg map.
 */
bool FdbService::CheckArgs(const ArgMap& arg_map,
                          int num_args,
                          const char* args[],
                          std::string* msg) const {
  for (int i = 0; i < num_args; ++i) {
    ArgMap::const_iterator it = arg_map.find(args[i]);
    if (it == arg_map.end() || it->second.empty()) {
      *msg = "Missing or empty parameter: ";
      *msg += args[i];
      return false;
    }
  }
  return true;
}

/**
 * Builds temporary qtnode file to be used for defining a cut spec.
 */
void FdbService::BuildQtnodesFile(const std::string& qtnodes_file,
                                  const std::string& qtnodes_string) {
  std::ofstream fp(qtnodes_file.c_str());
  std::string qtnodes = qtnodes_string;
  for (size_t i = 0; i < qtnodes.size(); ++i) {
    if ((qtnodes[i] == ' ') || (qtnodes[i] == '+')) {
      qtnodes[i] = '\n';
    }
  }
  fp.write(&qtnodes[0], qtnodes.size());
  fp.close();
}

/**
 * Adds given cut spec to map of available cut specs.
 */
int FdbService::DoAddCutSpec(request_rec* r,
                             ArgMap& arg_map) {
  const char* required_args[] =
      {"name", "qtnodes", "min_level", "default_level", "max_level"};
  std::string response;
  // Make sure we got all of the needed arguments for defining a CutSpec.
  if (CheckArgs(arg_map, 5, required_args, &response)) {
    // Move the qtnodes argument to a temporary file.
    std::string qtnodes_file = khTmpFilename(khTmpDirPath() + "/qtnodes");
    BuildQtnodesFile(qtnodes_file, arg_map["qtnodes"]);

    // Get the integer arguments.
    std::uint16_t min_level;
    std::uint16_t default_level;
    std::uint16_t max_level;
    StringToNumber(arg_map["min_level"], &min_level);
    StringToNumber(arg_map["default_level"], &default_level);
    StringToNumber(arg_map["max_level"], &max_level);

    std::string cut_spec_name = arg_map["name"];

    // If CutSpec with this name exists, delete it.
    if (cut_specs_.find(cut_spec_name) != cut_specs_.end()) {
      if (cut_specs_[cut_spec_name]) {
        delete cut_specs_[cut_spec_name];
        cut_specs_[cut_spec_name] = NULL;
      }
    }

    // Create the CutSpec.
    if (arg_map.find("exclusion_qtnodes") == arg_map.end()) {
      cut_specs_[cut_spec_name] = new fusion_portableglobe::CutSpec(
          qtnodes_file, min_level, default_level, max_level);
    } else {
      std::string exclusion_qtnodes_file =
          khTmpFilename(khTmpDirPath() + "/exclusion_qtnodes");
      BuildQtnodesFile(exclusion_qtnodes_file, arg_map["exclusion_qtnodes"]);
      cut_specs_[cut_spec_name] = new fusion_portableglobe::CutSpec(
          qtnodes_file, exclusion_qtnodes_file,
          min_level, default_level, max_level);
      khPruneFileOrDir(exclusion_qtnodes_file);
    }

    khPruneFileOrDir(qtnodes_file);
    response = "Ok";
  }
  ap_rwrite(&response[0], response.size(), r);
  return OK;
}



/**
 * Processes a general command, presumably not aimed at getting data
 * from a specific globe.
 */
int FdbService::ProcessGeneralCommand(request_rec* r,
                                      const std::string& command) {
ap_log_rerror(
    APLOG_MARK, APLOG_NOTICE, 0, r, "General command: >%s<", command.c_str());
  // List of commands w/out arguments.
  if (command == "statusz") {
    r->content_type = "text/html";
    return DoGeneralStatus(r);
  } else if (command == "reset") {
    r->content_type = "text/plain";
    return DoReset(r);
  }

  // List of commands with arguments.
  ArgMap arg_map;
  std::string no_value_arg;
  if (r->args) {
    ParseUrlArgs(r->args, &arg_map, &no_value_arg);
  }

  if (command == "add_cut_spec") {
    r->content_type = "text/html";
    return DoAddCutSpec(r, arg_map);
  }

  if (command == "publish") {
    const char* required_args[] = {"target_path", "db_type", "db_path"};
    std::string response;
    // Make sure we got all of the needed arguments.
    if (!CheckArgs(arg_map, 3, required_args, &response)) {
      r->content_type = "text/plain";
      ap_rwrite(&response[0], response.size(), r);
      return HTTP_BAD_REQUEST;
    }

    std::string target_path = arg_map["target_path"];
    std::string db_type = arg_map["db_type"];
    std::string db_path = arg_map["db_path"];
    r->content_type = "text/plain";

    if (db_type == "ge" || db_type == "map") {
      return DoPublishFusionDb(r, target_path, db_path);
    } else if (db_type == "glb" || db_type == "glm" || db_type == "glc") {
      return DoPublishPortable(r, target_path, db_path);
    } else {
      ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                    "Invalid database type: %s", db_type.c_str());
      return OK;
    }
  }

  if (command == "unpublish") {
    // Make sure we got all of the needed arguments.
    const char* required_args[] = {"target_path"};
    std::string response;
    if (!CheckArgs(arg_map, 1, required_args, &response)) {
      r->content_type = "text/plain";
      ap_rwrite(&response[0], response.size(), r);
      return HTTP_BAD_REQUEST;
    }

    std::string target_path = arg_map["target_path"];
    r->content_type = "text/plain";
    return DoUnpublish(r, target_path);
  }

  // Set cutting enabled.
  if (command == "enable_cut") {
    DoSetCuttingEnabled(true);
    r->content_type = "text/plain";
    std::string data;
    data.append("cutting enabled");
    ap_rwrite(&data[0], data.size(), r);
    return OK;
  }

  // Set cutting disabled.
  if (command == "disable_cut") {
    DoSetCuttingEnabled(false);
    r->content_type = "text/plain";
    std::string data;
    data.append("cutting disabled");
    ap_rwrite(&data[0], data.size(), r);
    return OK;
  }

  r->content_type = "text/plain";
  return TileService::ShowFailedRequest(r, arg_map, command, "NO_GLOBE");
}

/**
 * Delegates request to correct handler.
 * TODO: Fix mime types.
 */
int FdbService::ProcessRequest(request_rec* r) {
  // Default mime type.
  // ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r, "ProcessRequest");

  r->content_type = "application/octet-stream";

  const std::string kPastDate("Sat, 1 Jan 2000 12:00:00 GMT");
  // Set the last modified time.
  r->mtime = apr_date_parse_http(kPastDate.c_str());

  // Create an apr_time_t 10yrs in the future. apr (Apache) time is microsecs.
  const int expires_secs = 10 * 365 * 24 * 3600;  // 10 years in the future.
  apr_time_t future_time = apr_time_make(expires_secs, apr_time_now());

  char buffer[1000];
  apr_rfc822_date(buffer, future_time);
  std::string future_date(buffer);
  std::ostringstream max_age;
  max_age << "max-age=" << expires_secs;

  bool is_cacheable = false;
  if (is_cacheable) {
    apr_table_set(r->headers_out, "Expires", future_date.c_str());
    apr_table_set(r->headers_out, "Cache-Control", max_age.str().c_str());
  } else {
    apr_table_set(r->headers_out, "Expires", kPastDate.c_str());
    apr_table_set(r->headers_out, "Cache-Control", "no-cache");
  }

  std::string target_path;
  std::string command;
  std::string path_residual;

  ParsePath(r->path_info, &target_path, &command, &path_residual);

  if (target_path.empty() && command.empty()) {
    ap_log_rerror(
        APLOG_MARK, APLOG_WARNING, 0, r, "No target/command specified.");
    return HTTP_NOT_FOUND;
  }

  if (!command.empty()) {
    return ProcessGeneralCommand(r, command);
  }

  // Process Globe/Fusion Db requests.
  assert(!target_path.empty());

  ArgMap arg_map;
  std::string no_value_arg;
  if (r->args) {
    ParseUrlArgs(r->args, &arg_map, &no_value_arg);
  }

  ArgMap::const_iterator it = arg_map.find("db_type");
  if (it == arg_map.end() || it->second.empty()) {
     ap_log_rerror(
        APLOG_MARK, APLOG_WARNING, 0, r, "No database type specified.");
    return HTTP_NOT_FOUND;
  }

  fusion_portableglobe::CutSpec* cutspec = NULL;
  // If a cutspec has been defined, use it to determine if we should
  // return the requested data. We do not filter quadset requests at
  // the moment.
  // TODO: Consider using KeepNode to filter quadsets.
  // TODO: or rewriting quadsets based on ExcludeNode and KeepNode.
  if (arg_map.find("cutspec") != arg_map.end()) {
    std::string cutspec_name = arg_map["cutspec"];
    if (cut_specs_.find(cutspec_name.c_str()) != cut_specs_.end()) {
      cutspec = cut_specs_[cutspec_name];
    } else {
      ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                    "Unknown cutspec: %s", cutspec_name.c_str());
    }
  }

  std::string db_type = it->second;
  if (db_type == "ge" || db_type == "map") {
    // Process Fusion DB request.
    return fusion_db_service_.ProcessFusionDbRequest(r,
                                                     path_residual,
                                                     target_path,
                                                     &arg_map,
                                                     no_value_arg,
                                                     cutspec);
  } else if (db_type == "glb" || db_type == "glm" || db_type == "glc") {
    // Process portable globe request.
    std::string residual;
    int layer_id;
    bool is_balloon_request;

    portable_service_.ParsePortablePath(path_residual,
                                        db_type,
                                        &layer_id,
                                        &residual,
                                        &is_balloon_request);

    return portable_service_.ProcessPortableRequest(r,
                                                    residual,
                                                    target_path,
                                                    db_type,
                                                    layer_id,
                                                    is_balloon_request,
                                                    arg_map,
                                                    no_value_arg,
                                                    cutspec);
  } else {
    ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                  "Invalid database type: %s", db_type.c_str());
  }

  return HTTP_NOT_FOUND;
}

/**
 * Gives some basic status on the plugin including how many unpackers
 * have been created for different globes.
 * TODO: Use a template for the output.
 * TODO: Template may be JSON for new web-based server admin.
 */
int FdbService::DoGeneralStatus(request_rec* r) {
  std::string data;

  data.append("<html><body>");
  data.append("<font face='arial' size='1'>");
  data.append("<h3>Fdb Apache Module Status</h3>");
  if (is_cutting_enabled_) {
    data.append("<p><em>cutting enabled</em>");
  } else {
    data.append("<p><em>cutting disabled</em>");
  }

  portable_service_.DoStatus(&data);
  fusion_db_service_.DoStatus(&data);

  data.append("</font>");
  data.append("</body></html>");
  ap_rwrite(&data[0], data.size(), r);

  return OK;
}

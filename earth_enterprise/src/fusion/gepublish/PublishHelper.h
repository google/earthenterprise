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


// Classes for creating and issuing HTTP GET requests.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GEPUBLISH_PUBLISHHELPER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GEPUBLISH_PUBLISHHELPER_H_

#include <string>
#include <vector>
#include <map>

#include "autoingest/.idl/ServerCombination.h"
#include "common/LoggingManager.h"

class geAuth;

typedef std::map<std::string, std::vector<std::string> > HeaderMap;

// Publish Helper processor creates and issuing HTTP GET requests.
class PublishHelper {
 public:
  enum ServerType {
    UNKNOWN_SERVER = 0,
    STREAM_SERVER = 1,
    SEARCH_SERVER = 2
  };

  enum VsType {
    TYPE_GE = 0,
    TYPE_MAP = 1,
    TYPE_BOTH = 2
  };

  PublishHelper(const std::string& fusion_host,
                const ServerConfig& stream_server,
                const ServerConfig& search_server,
                geAuth* auth);

  virtual ~PublishHelper();

  void SetFusionHost(const std::string& fusion_host) {
    fusion_host_ = fusion_host;
  }

  // Returns collected error messages.
  const std::string& ErrMsg();

  bool ProcessGetRequest(ServerType server_type,
                         std::string* args,
                         const std::string& lookfor_header,
                         std::vector<std::string>* header_values);
  bool ProcessGetRequest(ServerType server_type,
                         std::string* args,
                         HeaderMap* header_map);

  bool ProcessPublishGetRequest(ServerType server_type,
                                std::string* args,
                                const std::string& lookfor_header,
                                std::vector<std::string>* header_values);
  bool ProcessPublishGetRequest(ServerType server_type,
                                std::string* args,
                                HeaderMap* header_map);

  bool ProcessGetRequest(const std::string& url,
                         const std::string& cacert,
                         const bool insecure,
                         std::string* args,
                         HeaderMap* header_map,
                         const std::string& host_name = std::string());

 protected:
  // Returns server URL for specified server type.
  std::string ServerUrl(ServerType server_type) const;
  const ServerConfig& Server(ServerType server_type) const;

  // Returns servlet location.
  std::string ServletLocation(ServerType server_type);
  std::string PublishServletLocation();

  // Sends error message to the logger.
  inline void AppendErrMsg(const std::string& err_msg) {
    logger_.AppendErrMsg(err_msg);
  }

  inline const std::string& LogFilename() const {
    return logger_.LogFilename();
  }

  static const int kOptionalPingTimeoutSecs;
  std::string fusion_host_;
  ServerConfig stream_server_;
  ServerConfig search_server_;
  std::string username_;
  std::string password_;
  int timeout_secs_;  // Timeout (in seconds) to use for curl requests.

 private:
  static const int kLogsToKeepDefault;
  static const std::string kErrAuthReqd;
  geAuth* auth_;
  LoggingManager logger_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GEPUBLISH_PUBLISHHELPER_H_

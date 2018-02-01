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


#include "fusion/gepublish/PublishHelper.h"

#include <assert.h>

#include "fusion/gepublish/CurlRequest.h"
#include "common/geAuth.h"
#include "common/khStringUtils.h"

const std::string PublishHelper::kErrAuthReqd = "Authentication required.";

// 3 seconds timeout for Ping should be sufficient.
const int PublishHelper::kOptionalPingTimeoutSecs = 3;
// Number of logs to keep by default. A new log-file is created each request,
// and we keep only specified number of most recent logs. The logs are not
// cleaned up while restarting server.
const int PublishHelper::kLogsToKeepDefault = 5;


// TODO:
// Clean up pushing/publishing functionality: drop DecrementCount().
// ProcessPublishGetRequest() - refactor scheme for selecting servlet locations?


PublishHelper::PublishHelper(const std::string& fusion_host,
                             const ServerConfig& stream_server,
                             const ServerConfig& search_server,
                             geAuth* auth)
    : fusion_host_(fusion_host),
      stream_server_(stream_server),
      search_server_(search_server),
      timeout_secs_(-1),
      auth_(auth),
      logger_(std::string(""),                   // directory name
              std::string("gepublishdatabase"),  // file prefix
              PublishHelper::kLogsToKeepDefault,
              std::string("KH_PUBLISHER_LOGS")) {
  if (stream_server_.url.empty())
    stream_server_.url = "http://" + fusion_host_;
  if (search_server_.url.empty())
    search_server_.url = stream_server_.url;
  curl_global_init(CURL_GLOBAL_ALL);
}

PublishHelper::~PublishHelper() {
}

std::string PublishHelper::ServerUrl(ServerType server_type) const {
  std::string url = (server_type == STREAM_SERVER)
      ? stream_server_.url : search_server_.url;
  // Remove the trailing slash if any.
  if (url[url.size()-1] == '/')
    url = url.substr(0, url.size()-1);
  return url;
}

const ServerConfig& PublishHelper::Server(ServerType server_type) const {
  return (server_type == STREAM_SERVER) ? stream_server_ : search_server_;
}

const std::string& PublishHelper::ErrMsg() {
  return logger_.ErrMsg();
}

bool PublishHelper::ProcessGetRequest(ServerType server_type,
                                      std::string* args,
                                      const std::string& lookfor_header,
                                      std::vector<std::string>* header_values) {
  assert(header_values);
  HeaderMap header_map;
  if (!lookfor_header.empty()) {
    header_map[lookfor_header] = std::vector<std::string>();
  }
  if (!ProcessGetRequest(server_type, args, &header_map)) {
    return false;
  }

  if (!lookfor_header.empty()) {
    *header_values = header_map[lookfor_header];
  }

  return true;
}

bool PublishHelper::ProcessGetRequest(ServerType server_type,
                                      std::string* args,
                                      HeaderMap* header_map) {
  std::string url = ServerUrl(server_type) + ServletLocation(server_type);
  const ServerConfig& server = Server(server_type);
  return ProcessGetRequest(url, server.cacert_ssl, server.insecure_ssl,
                           args, header_map);
}

bool PublishHelper::ProcessPublishGetRequest(
    ServerType server_type,
    std::string* args,
    const std::string& lookfor_header,
    std::vector<std::string>* header_values) {
  assert(header_values);
  HeaderMap header_map;
  if (!lookfor_header.empty()) {
    header_map[lookfor_header] = std::vector<std::string>();
  }
  if (!ProcessPublishGetRequest(server_type, args, &header_map))
    return false;
  if (!lookfor_header.empty()) {
    *header_values = header_map[lookfor_header];
  }

  return true;
}

bool PublishHelper::ProcessPublishGetRequest(
    ServerType server_type,
    std::string* args,
    HeaderMap* header_map) {
  assert(header_map);
  std::string url = ServerUrl(server_type) + PublishServletLocation();
  const ServerConfig& server = Server(server_type);
  return ProcessGetRequest(url, server.cacert_ssl, server.insecure_ssl,
                           args, header_map);
}

bool PublishHelper::ProcessGetRequest(const std::string& url,
                                      const std::string& cacert,
                                      const bool insecure,
                                      std::string* args,
                                      HeaderMap* header_map,
                                      const std::string& host_name) {
  assert(args);
  assert(header_map);
  std::vector<std::string> header_names;
  HeaderMap::const_iterator it = header_map->begin();
  for (; it != header_map->end(); ++it)
    header_names.push_back(it->first);
  GetRequest request(username_, password_, url, cacert, insecure, header_names,
                     timeout_secs_);
  *args += "&Host=";
  // Pretend to be someone else. This is only true for DecrementCount requests.
  if (host_name.empty())
    *args += fusion_host_;
  else
    *args += host_name;

  notify(NFY_DEBUG, "Request %s args: %s", url.c_str(),
         (username_.empty() ? "nouser" : "user"));

  int status = request.Start(*args);
  int max_retries = 3;
  int allow_single_auth_retry = true;
  while (status == CurlRequest::AUTH_REQD) {
    if (auth_ == NULL) {
      AppendErrMsg(kErrAuthReqd);
      return false;
    }
    // Workaround for intermittent issue in which a properly authenticated
    // request fails the first time and succeeds the 2nd.
    if (allow_single_auth_retry && (!username_.empty() && !password_.empty())) {
      allow_single_auth_retry = false;
      notify(NFY_DEBUG, "Authenticated Retry");
      status = request.Start(*args);
      continue;
    }
    allow_single_auth_retry = false;
    notify(NFY_DEBUG, "Retry with Queried Username and Password");
    if ((max_retries-- == 0) || !auth_->GetUserPwd(username_, password_)) {
      AppendErrMsg(kErrAuthReqd);
      return false;
    }
    request.SetUserPwd(username_, password_);
    status = request.Start(*args);
  }

  if (status == -1) {
    std::string message = request.GetStatusMessage();
    if (message.empty()) {
      message = "No status message returned from \n  request: " +
          url + "?" + *args;
    }
    AppendErrMsg(message);

    // Log the failed request and args.
    std::string request_failure = "  Failed request: " + url + "?" + *args;
    notify(NFY_DEBUG, "%s", request_failure.c_str());
    return false;
  }

  if (header_map->size() > 0) {
    request.GetHeaders(header_map);
  }
  notify(NFY_DEBUG, "Request completed successfully");

  return true;
}

// TODO: This method may read the corresponding values from some
// conf files.
std::string PublishHelper::ServletLocation(ServerType server_type) {
  const std::string servlet_location = (server_type == STREAM_SERVER) ?
      "/geserve/StreamPush" : "/geserve/SearchPush";

  return servlet_location;
}

// TODO: This method may read the corresponding values from some
// conf files.
std::string PublishHelper::PublishServletLocation() {
  const std::string servlet_location = "/geserve/Publish";
  return servlet_location;
}

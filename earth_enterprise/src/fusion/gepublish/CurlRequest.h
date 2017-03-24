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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GEPUBLISH_CURLREQUEST_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GEPUBLISH_CURLREQUEST_H_

#include <stdio.h>
#include <curl/curl.h>
#include <string>
#include <vector>
#include <map>

#include "common/ManifestEntry.h"

class geProgress;

class CurlRequest {
 public:
  enum HttpErrCode {
    AUTH_REQD = 401,
    METHOD_NOT_ALLOWED = 405
  };
  CurlRequest(const std::string& username,
              const std::string& password,
              const std::string& url,
              const std::string& cacert,
              const bool insecure);
  virtual ~CurlRequest();
  void SetUserPwd(const std::string& username, const std::string& password);

 protected:
  static size_t CurlWriteFunc(
      void* buffer, size_t size, size_t nmemb, void* userp);

  std::string userpwd_;
  std::string url_;
  CURL* curl_easy_handle_;
  curl_slist* curl_slist_;
  char error_buffer_[CURL_ERROR_SIZE];
};



class UploadRequest : public CurlRequest {
 public:
  UploadRequest(const std::string& username,
                const std::string& password,
                const std::string& url,
                const std::string& cacert,
                const bool insecure,
                const std::string& root_dir,
                const std::vector<ManifestEntry>* const entries);

  virtual ~UploadRequest();
  bool Start(bool report_progress, geProgress* progress);

 private:
  bool Init(std::string src_file_path);
  void Close();
  bool CreateDir(const std::string& dir);
  bool EnsureDestPath(const std::string& escaped_file_path);
  static size_t CurlReadFunc(void* buffer, size_t size, size_t nmemb,
                             UploadRequest* userp);
  size_t ReadData(void* buffer, size_t size);
  std::string GetDestRoot();

  std::string root_dir_;
  const std::vector<ManifestEntry> *const entries_;
  FILE* src_fp_;
};


// GetRequest is actually implemented as a post request coz we can have large
// number of arguments in our request.
class GetRequest : public CurlRequest {
 public:
  typedef std::map<std::string, std::vector<std::string> > HeaderMap;

  // Requests take an optional timeout, by default the timeout is ignored (<=0)
  GetRequest(const std::string& username,
             const std::string& password,
             const std::string& url,
             const std::string& cacert,
             const bool insecure,
             const std::vector<std::string> &header_names,
             int timeout_secs = -1);

  virtual ~GetRequest();
  bool Init();
  int Start(const std::string& args);
  const std::string& GetStatusMessage() const { return status_message_; }
  void GetHeaders(HeaderMap* headers) { *headers = headers_; }

 private:
  static size_t CurlWriteFunc(void* buffer, size_t size, size_t nmemb,
                               GetRequest* userp);
  void ParseResponse();

  int status_code_;
  std::string status_message_;
  HeaderMap headers_;
  std::string response_str_;
  // The timeout for the curl request. if <= 0, it is ignored.
  int timeout_secs_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GEPUBLISH_CURLREQUEST_H_

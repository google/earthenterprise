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


#include "fusion/gepublish/CurlRequest.h"

#include <stdlib.h>
#include "common/geProgress.h"
#include "common/khStringUtils.h"
#include "common/notify.h"


CurlRequest::CurlRequest(const std::string& username,
                         const std::string& password,
                         const std::string& url,
                         const std::string& cacert,
                         const bool insecure)
    : userpwd_(),
      url_(url),
      curl_easy_handle_(NULL),
      curl_slist_(NULL) {
  curl_easy_handle_ = curl_easy_init();
  if (curl_easy_handle_ == NULL) {
    notify(NFY_FATAL, "Could not start libcurl easy session.");
  }
  curl_easy_setopt(curl_easy_handle_, CURLOPT_NOSIGNAL, true);
  curl_easy_setopt(curl_easy_handle_, CURLOPT_FAILONERROR, true);
  curl_easy_setopt(curl_easy_handle_, CURLOPT_FOLLOWLOCATION, true);
  curl_easy_setopt(curl_easy_handle_, CURLOPT_MAXREDIRS, 16);
  curl_easy_setopt(curl_easy_handle_, CURLOPT_ERRORBUFFER, error_buffer_);
  if (insecure) {
    curl_easy_setopt(curl_easy_handle_, CURLOPT_SSL_VERIFYPEER, 0L);
  } else if (!cacert.empty()) {
    curl_easy_setopt(curl_easy_handle_, CURLOPT_CAINFO, cacert.c_str());
  }

  // Ask for a persistent connection
  curl_slist_ = curl_slist_append(curl_slist_, "Connection: Keep-Alive");
  if (curl_slist_ != NULL)
    curl_easy_setopt(curl_easy_handle_, CURLOPT_HTTPHEADER, curl_slist_);

  SetUserPwd(username, password);
}

CurlRequest::~CurlRequest() {
  if (curl_easy_handle_) {
    curl_slist_free_all(curl_slist_);
    curl_easy_cleanup(curl_easy_handle_);
  }
}

void CurlRequest::SetUserPwd(const std::string& username,
                             const std::string& password) {
  if (!username.empty() && !password.empty()) {
    userpwd_ = username + ":" + password;
    curl_easy_setopt(curl_easy_handle_, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    curl_easy_setopt(curl_easy_handle_, CURLOPT_USERPWD, userpwd_.c_str());
  }
}


size_t CurlRequest::CurlWriteFunc(void* buffer,
                                  size_t size,
                                  size_t nmemb,
                                  void* userp) {
  return size * nmemb;
}


UploadRequest::UploadRequest(const std::string& username,
                             const std::string& password,
                             const std::string& url,
                             const std::string& cacert,
                             const bool insecure,
                             const std::string& root_dir,
                             const std::vector<ManifestEntry>* const entries)
    : CurlRequest(username, password, url, cacert, insecure),
      root_dir_(root_dir),
      entries_(entries),
      src_fp_(NULL) {
  assert(entries_ != NULL);
}


UploadRequest::~UploadRequest() {
}


bool UploadRequest::Init(std::string src_file_path) {
  curl_easy_setopt(curl_easy_handle_,
                   CURLOPT_READFUNCTION, UploadRequest::CurlReadFunc);
  curl_easy_setopt(curl_easy_handle_,
                   CURLOPT_WRITEFUNCTION, CurlRequest::CurlWriteFunc);
  curl_easy_setopt(
      curl_easy_handle_, CURLOPT_INFILE, reinterpret_cast<void*>(this));
  src_fp_ = fopen(src_file_path.c_str(), "r");
  if (src_fp_ == NULL) {
    notify(NFY_DEBUG, "src file open failed");
    return false;
  }

  return true;
}


void UploadRequest::Close() {
  if (src_fp_ != NULL) {
    fclose(src_fp_);
    src_fp_ = NULL;
  }
}


bool UploadRequest::CreateDir(const std::string& dir) {
  std::string dest_root = url_;
  std::string complete_dir = url_ + "/" + dir;
  curl_easy_setopt(curl_easy_handle_, CURLOPT_URL, complete_dir.c_str());
  // Create MKCOL request to create directory. We don't need to check if the
  // directory already exists coz if it does then we'll get a
  // METHOD_NOT_ALLOWED. This way is more efficient.
  curl_easy_setopt(curl_easy_handle_, CURLOPT_CUSTOMREQUEST, "MKCOL");
  curl_easy_perform(curl_easy_handle_);
  // Set the custom request back to null so that the following requests can
  // be set correctly.
  curl_easy_setopt(curl_easy_handle_, CURLOPT_CUSTOMREQUEST, NULL);
  long result_code = 0;
  curl_easy_getinfo(curl_easy_handle_, CURLINFO_RESPONSE_CODE, &result_code);
  // We're OK with METHOD_NOT_ALLOWED coz that just means that the directory
  // already exists.
  if (result_code > 400 && result_code != METHOD_NOT_ALLOWED) {
    notify(NFY_DEBUG, "Could not create directory: %s", dir.c_str());
    notify(NFY_DEBUG, "MKCOL resulted in: %ld", result_code);
    return false;
  }
  return true;
}


bool UploadRequest::EnsureDestPath(const std::string& escaped_file_path) {
  std::vector<std::string> tokens;
  // Add the dest path to the list of dirs.
  TokenizeString(escaped_file_path, tokens, "/");
  std::string dir = root_dir_;
  for (int i = 0; i < static_cast<int>(tokens.size()) - 1; ++i) {
    dir += "/" + tokens[i];
    if (!CreateDir(dir))
      return false;
  }
  return true;
}


bool UploadRequest::Start(bool report_progress, geProgress* progress) {
  int64 processed_size = 0;
  size_t num_entries = entries_->size();
  for (size_t i = 0; i < num_entries; ++i) {
    const ManifestEntry& entry = (*entries_)[i];
    notify(NFY_DEBUG, "%s", entry.orig_path.c_str());
    if (!Init(entry.current_path)) {
      return false;
    }

    if (!CreateDir(root_dir_))
      return false;

    // We cannot escape the entire URL because that results in an invalid
    // PUT request (all the '/' and '.' chars are also replaced)
    std::string escaped_orig_path = ReplaceString(entry.orig_path, " ", "%20");

    if (!EnsureDestPath(escaped_orig_path))
      return false;

    std::string dest_url = url_ + "/" + root_dir_ + escaped_orig_path;
    curl_easy_setopt(curl_easy_handle_, CURLOPT_URL, dest_url.c_str());
    curl_easy_setopt(curl_easy_handle_, CURLOPT_UPLOAD, true);
    CURLcode code = curl_easy_perform(curl_easy_handle_);
    curl_easy_setopt(curl_easy_handle_, CURLOPT_UPLOAD, false);
    if (code != CURLE_OK) {
      notify(NFY_DEBUG, "Could not copy: Curl error code: %d", code);
      notify(NFY_DEBUG, "HTTP Error: %s", error_buffer_);
      notify(NFY_DEBUG, "  for: %s", url_.c_str());
      return false;
    }
    Close();
    if (report_progress && progress != NULL) {
      if (!progress->incrementDone(entry.data_size)) {
        notify(NFY_WARN, "Push interrupted");
        return false;
      }
      processed_size += entry.data_size;
      notify(NFY_DEBUG, "Done files: %zd/%zd, bytes: %zd",
             i + 1, num_entries, processed_size);
    }
  }
  return true;
}



size_t UploadRequest::CurlReadFunc(void* buffer,
                             size_t size,
                             size_t nmemb,
                             UploadRequest* request) {
  return request->ReadData(buffer, size * nmemb);
}


size_t UploadRequest::ReadData(void* buffer, size_t requested_size) {
  size_t bytes_read = fread(buffer, 1, requested_size, src_fp_);
  return bytes_read;
}


GetRequest::GetRequest(const std::string& username,
                       const std::string& password,
                       const std::string& url,
                       const std::string& cacert,
                       const bool insecure,
                       const std::vector<std::string> &header_names,
                       int timeout_secs)
    : CurlRequest(username, password, url, cacert, insecure),
      status_code_(-1),
      timeout_secs_(timeout_secs) {
  for (uint i = 0; i < header_names.size(); ++i) {
    if (!header_names[i].empty())
      headers_[header_names[i]] = std::vector<std::string>();
  }
}


GetRequest::~GetRequest() {
}


bool GetRequest::Init() {
  curl_easy_setopt(curl_easy_handle_,
                   CURLOPT_WRITEFUNCTION, GetRequest::CurlWriteFunc);
  curl_easy_setopt(
      curl_easy_handle_, CURLOPT_WRITEDATA, reinterpret_cast<void*>(this));
  return true;
}


int GetRequest::Start(const std::string& args) {
  Init();
  curl_easy_setopt(curl_easy_handle_, CURLOPT_POSTFIELDS, args.c_str());
  curl_easy_setopt(curl_easy_handle_, CURLOPT_URL, url_.c_str());

  notify(NFY_DEBUG, "GetRequest: url: %s", url_.c_str());
  notify(NFY_DEBUG, "GetRequest: args: %s", args.c_str());

  if (timeout_secs_ > 0) {
    curl_easy_setopt(curl_easy_handle_, CURLOPT_TIMEOUT, timeout_secs_);
  }
  CURLcode code = curl_easy_perform(curl_easy_handle_);

  if (code != CURLE_OK) {
    long result_code = 0;
    CURLcode getinfoCode =
        curl_easy_getinfo(curl_easy_handle_, CURLINFO_RESPONSE_CODE, &result_code);
    // Authentication required.
    if (getinfoCode == CURLE_OK && result_code == AUTH_REQD)
      return result_code;
    // Ignore the GOT_NOTHING error code. This happens sometimes when
    // tomcat restarts apache and for some reason apache sends out an
    // empty response. The confusing part is that this does not happen
    // every time. We need to revisit this problem and see if this
    // can be fixed in a different way.
    if (code == CURLE_GOT_NOTHING)
      return 0;
    // Else error out.
    notify(NFY_DEBUG, "Curl error code: %d", code);
    notify(NFY_DEBUG, "HTTP Error: %s", error_buffer_);
    notify(NFY_DEBUG, "  for: %s", url_.c_str());
    notify(NFY_DEBUG, "  args: %s", args.c_str());
  }
  ParseResponse();
  return status_code_;
}


size_t GetRequest::CurlWriteFunc(void* buffer, size_t size, size_t nmemb,
                                  GetRequest* request) {
  request->response_str_ +=
      std::string(reinterpret_cast<char*>(buffer), size*nmemb);
  return size * nmemb;
}


void GetRequest::ParseResponse() {
  // Split the response into lines.
  std::vector<std::string> lines;

  notify(NFY_DEBUG, "Response: %s",
         response_str_.empty() ? "empty" : response_str_.c_str());

  TokenizeString(response_str_, lines, "\n");
  for (uint i = 0; i < lines.size(); ++i) {
    std::string kDelim = ":";
    std::string kStatusCode = "Gepublish-StatusCode";
    std::string kStatusMessage = "Gepublish-StatusMessage";
    std::vector<std::string> tokens;
    CleanString(&lines[i], "\r\n");
    TokenizeString(lines[i], tokens, kDelim, 2);
    // We don't want to report an error for an invalid header coz we don't care!
    if (tokens.size() < 2)
      continue;
    // Skip this header if it is not the one we are looking for.
    HeaderMap::iterator it = headers_.find(tokens[0]);
    if (it != headers_.end()) {
      it->second.push_back(tokens[1]);
    } else if (tokens[0] == kStatusCode) {
      status_code_ = atoi(tokens[1].c_str());
    } else if (tokens[0] == kStatusMessage) {
      status_message_ = tokens[1];
    }
  }
}

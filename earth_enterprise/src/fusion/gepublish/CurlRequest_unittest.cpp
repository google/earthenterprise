// Copyright 2018 the Open GEE Contributors
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


// Unit tests for CurlRequest

#include <gtest/gtest.h>
#include "fusion/gepublish/CurlRequest.h"
#include <list>


namespace {

class CurlRequestUnitTest : public testing::Test { 

};

struct curl_api_fake : public curl_api {
  CURL* easy_init() override
  {
    return reinterpret_cast<CURL*>(this);
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, bool param) override
  {
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, int param) override
  {
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, long param) override
  {
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, unsigned long param) override
  {
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, const char* param) override
  {
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, curl_slist* param) override
  {
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, CurlRequestWriteFunc param) override
  {
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, GetRequestWriteFunc param) override
  {
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, UploadRequestReadFunc param) override
  {
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, void* param) override
  {
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, std::nullptr_t) override
  {
    return CURLE_OK;
  }
  curl_slist*slist_append(curl_slist* lst, const char * item) override
  {
    std::list<std::string>* lst_impl;
    if (!lst) {
        lst_impl = new std::list<std::string>();
        lst = reinterpret_cast<curl_slist*>(lst_impl);
    }
    else {
        lst_impl = reinterpret_cast<std::list<std::string>*>(lst);
    }
    lst_impl->push_back(item);

    return lst;
  }
  void slist_free_all(curl_slist* lst) override
  {
     delete (reinterpret_cast<std::list<std::string>*>(lst));
  }
  void easy_cleanup(CURL *handle) override
  {
  }
  CURLcode easy_perform(CURL * handle) override
  {
    return CURLE_OK;
  }
  CURLcode easy_getinfo(CURL *handle, CURLINFO info, long* result_code) override
  {
    return CURLE_OK;
  }
};

// dummy test
TEST_F(CurlRequestUnitTest, dummy) {
    curl_api_fake fake_api;
    CurlRequest req("user", "psw", "http://myserver.local/req", "", true, &fake_api);
}

}  // namespace

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

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
#include <stdexcept>
#include "common/ManifestEntry.h"
#include <vector>



namespace {

class CurlRequestUnitTest : public testing::Test { 

};

//use this string to track flow of calls
std::string codePathString = "";
int codePathCounter = 0;
curl_slist* postAppendKeepAlive;
curl_slist* easySetOptParm;
CURL* originalHandle;
CURL* terminalHandle;



struct curl_api_fake : public curl_api {
  CURL* easy_init() override
  {
    return reinterpret_cast<CURL*>(this);
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, bool param) override
  {
    codePathCounter++;
    codePathString = codePathString + std::to_string(codePathCounter) + "bool:"+std::to_string(param)+";option:"+std::to_string(option)+",\n"; 
    originalHandle = handle;
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, int param) override
  {
    codePathCounter++;
    codePathString = codePathString + std::to_string(codePathCounter) + "int:"+std::to_string(param)+";option:"+std::to_string(option)+",\n"; 
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, long param) override
  {
    codePathCounter++;
    codePathString = codePathString + std::to_string(codePathCounter) + "long:"+std::to_string(param)+";option:"+std::to_string(option)+",\n"; 
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, unsigned long param) override
  {
    codePathCounter++;
    codePathString = codePathString + std::to_string(codePathCounter) + "unsignedLong:"+std::to_string(param)+";option:"+std::to_string(option)+",\n"; 
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, const char* param) override
  {
    codePathCounter++;
    if ("10010" == std::to_string(option))
    {
      size_t len = strlen(param);
      codePathString = codePathString + std::to_string(codePathCounter) + "constChar*errsize:"+std::to_string(len)+";option:"+std::to_string(option)+",\n"; 

    }
    else if ("10005" == std::to_string(option) || "10065" == std::to_string(option))
    {
      std::string aCharToString(param);
      codePathString = codePathString + std::to_string(codePathCounter) + "constChar*:"+aCharToString+";option:"+std::to_string(option)+",\n"; 

    }

    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, curl_slist* param) override
  {
    codePathCounter++;
    codePathString = codePathString + std::to_string(codePathCounter) + "curl_slist*:<addContentHere>;option:"+std::to_string(option)+",\n";
    easySetOptParm = param;
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, CurlRequestWriteFunc param) override
  {
    codePathCounter++;
    //std::cout <<"Inside_CurlRequestWriteFunc\n";
    //codePathString = codePathString + std::to_string(codePathCounter) + "curlRequestWriteFunc:"+std::to_string(param)+";option:"+std::to_string(option)+",\n";
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, GetRequestWriteFunc param) override
  {
    codePathCounter++;
    //std::cout <<"Inside_GetRequestWriteFunc\n";
    //codePathString = codePathString + std::to_string(codePathCounter) + "GetRequestWriteFunc:"+std::to_string(param)+";option:"+std::to_string(option)+",\n";
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, UploadRequestReadFunc param) override
  {
    codePathCounter++;
    //std::cout <<"Inside_UploadRequestReadFunc\n";
    //codePathString = codePathString + std::to_string(codePathCounter) + "UploadRequestReadFunc:"+std::to_string(param)+";option:"+std::to_string(option)+",\n";
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, void* param) override
  {
    codePathCounter++;
    codePathString = codePathString + std::to_string(codePathCounter) + "void*:<getVoidVal>;option:"+std::to_string(option)+",\n";
    return CURLE_OK;
  }
  CURLcode easy_setopt(CURL *handle, CURLoption option, std::nullptr_t) override
  {
    codePathCounter++;
    codePathString = codePathString + std::to_string(codePathCounter) + "nullptr_t:nullptr_t;option:"+std::to_string(option)+",\n";
    return CURLE_OK;
  }
  curl_slist*slist_append(curl_slist* lst, const char * item) override
  {
    codePathCounter++;
    std::list<std::string>* lst_impl;
    if (!lst) {
        lst_impl = new std::list<std::string>();
        lst = reinterpret_cast<curl_slist*>(lst_impl);
    }
    else {
        lst_impl = reinterpret_cast<std::list<std::string>*>(lst);
    }
    lst_impl->push_back(item);
    
    std::string charToString(item);
    codePathString = codePathString+ std::to_string(codePathCounter)+"curl_slist_append:"+charToString+",\n";
    postAppendKeepAlive = lst;
    //codePathString = codePathString + std::to_string(codePathCounter) + "slist_append-constChar*:"+std::to_string(item)+";option:"+std::to_string(lst)+",\n";
    return lst;
  }
  void slist_free_all(curl_slist* lst) override
  {
     codePathCounter++;
     delete (reinterpret_cast<std::list<std::string>*>(lst));
     //std::cout <<"Inside_free_all\n";
     codePathString = codePathString + std::to_string(codePathCounter) + "slist_free_all,\n";
  }
  void easy_cleanup(CURL *handle) override
  {
    codePathCounter++;
    //std::cout <<"Inside_cleanup\n";
    codePathString = codePathString + std::to_string(codePathCounter) + "easy_cleanup,\n";
    terminalHandle = handle;
  }
  CURLcode easy_perform(CURL * handle) override
  {
    codePathCounter++;
    //std::cout <<"Inside_easy_perform\n";
    //codePathString = codePathString + std::to_string(codePathCounter) + "easy_perform:"+std::to_string(handle)+",\n";
    return CURLE_OK;
  }
  CURLcode easy_getinfo(CURL *handle, CURLINFO info, long* result_code) override
  {
    codePathCounter++;
    //std::cout <<"Inside_easy_getinfo\n";
    //codePathString = codePathString + std::to_string(codePathCounter) + "easy_getinfo:"+std::to_string(result_code)+";option:"+std::to_string(info)+",\n";
    return CURLE_OK;
  }
};

//helper, return array of strings showing order of API invocation, and parameters passed
std::string *apiTracker() 
{ 
  std::string s = codePathString;
  std::string delimiter = ",";
  size_t pos = 0;
  std::string token;
  int arrayIndex = 0;
  std::string *actualvalues = new std::string[codePathCounter];
      while ((pos = s.find(delimiter))!= std::string::npos) {
          token = s.substr(0, pos);
          //add each token to array:
          actualvalues[arrayIndex] = token; //each token holds unique Id and params for each API call
          s.erase(0, pos + delimiter.length());
          arrayIndex++;
      }    
   return actualvalues; 
} 

//1. test insecure true, and cacert is emtpy; verify the 'insecure' path is taken (0L is invoked ) and not cacert.c_str at the specified index
 TEST_F(CurlRequestUnitTest, isInsecureAndEmptyCaCert) {
    codePathCounter = 0;
    codePathString = "";
    curl_api_fake fake_api;
    CurlRequest req("user", "psw", "http://myserver.local/req", "", true, &fake_api);
    std::string *ptr = apiTracker(); 
    EXPECT_EQ("\n6long:0;option:64", ptr[5]);
}

//2. test insecure = true and cacert is not empty-- verify the insecure path is still taken since it is first evaluated
 TEST_F(CurlRequestUnitTest, isInsecureAndNonEmptyCaCert) {
    codePathCounter = 0;
    codePathString = "";
    curl_api_fake fake_api;
    CurlRequest req("user", "psw", "http://myserver.local/req", "caCert", true, &fake_api);
    std::string *ptr = apiTracker(); 
    EXPECT_EQ("\n6long:0;option:64", ptr[5]);
}

//3. test insecure = false and cacert is empty; verify neither of them are satisfied, and that flow of execution
//continues to slist_append to ask for persistent connection.
 TEST_F(CurlRequestUnitTest, isSecureAndEmptyCaCert) {
    codePathCounter = 0;
    codePathString = "";
    curl_api_fake fake_api;
    CurlRequest req("user", "psw", "http://myserver.local/req", "", false, &fake_api);
    std::string *ptr = apiTracker(); 
    EXPECT_EQ("\n6curl_slist_append:Connection: Keep-Alive", ptr[5]);
}

//4. test insecure = false and cacert is not empty; verify cacert path is taken
TEST_F(CurlRequestUnitTest, isSecureAndNonEmptyCaCert) {
    codePathCounter = 0;
    codePathString = "";
    curl_api_fake fake_api;
    CurlRequest req("user", "psw", "http://myserver.local/req", "caCert", false, &fake_api);
    std::string *ptr = apiTracker(); 
    EXPECT_EQ("\n6constChar*:caCert;option:10065", ptr[5]);
}

//5. verify that if curl_slist_! = NULL, then it's block is invoked
TEST_F(CurlRequestUnitTest, CurlsListIsNonNull) {
    postAppendKeepAlive = NULL;
    easySetOptParm = NULL;
    codePathCounter = 0;
    codePathString = "";
    curl_api_fake fake_api;
    CurlRequest req("user", "psw", "http://myserver.local/req", "caCert", false, &fake_api);
    std::string *ptr = apiTracker(); 
    EXPECT_TRUE(postAppendKeepAlive!=NULL);
    EXPECT_TRUE(easySetOptParm!=NULL);
    EXPECT_EQ(postAppendKeepAlive, easySetOptParm);
    EXPECT_EQ("\n8curl_slist*:<addContentHere>;option:10023", ptr[7]);
}

//6. verify that if username and password are both not empty, then the following calls are invoked and the userpsw is concatenation of the username+password
TEST_F(CurlRequestUnitTest, SetUsrPwdWhenBothNonEmpty) {
    codePathCounter = 0;
    codePathString = "";
    curl_api_fake fake_api;
    CurlRequest req("user", "psw", "http://myserver.local/req", "caCert", false, &fake_api);
    std::string *ptr = apiTracker(); 
    EXPECT_EQ("\n9unsignedLong:18446744073709551599;option:107", ptr[8]);
    EXPECT_EQ("\n10constChar*:user:psw;option:10005", ptr[9]);
}

//7. Verify that the first 5 calls follow in the prescribed manner
TEST_F(CurlRequestUnitTest, executionFlow_NoSignalToErrBufferOption) {
    codePathCounter = 0;
    codePathString = "";
    curl_api_fake fake_api;
    CurlRequest req("user", "psw", "http://myserver.local/req", "demo", false, &fake_api);
    std::cout << codePathString;
    std::string *ptr = apiTracker(); 
    EXPECT_EQ("1bool:1;option:99", ptr[0]);
    EXPECT_EQ("\n2bool:1;option:45", ptr[1]);
    EXPECT_EQ("\n3bool:1;option:52", ptr[2]);
    EXPECT_EQ("\n4int:16;option:68", ptr[3]);
    EXPECT_EQ("\n5constChar*errsize:6;option:10010", ptr[4]);
 }

//8. TODO: attempt a request with empty or malformed URL.

//9. Test UploadRequest::UploadRequest null Entries
 TEST_F(CurlRequestUnitTest, UploadRequest_nullEntries) {
    codePathCounter = 0;
    codePathString = "";
    curl_api_fake fake_api;
    const std::vector<ManifestEntry> *const entries_ = NULL;
    ASSERT_DEATH(UploadRequest req("user", "pwd", "http://myserver.com/req", "caCert", false, "/root_dir", entries_, &fake_api),"Assertion `entries_ != __null' failed.");
 }

//10. verify that if username is empty, then the following 2 calls in SetUserPwd are not invoked, and that slist_free_all and easy_cleanup are
//***********NOTE, it appears that the method throws Segmentation fault when username is empty.
TEST_F(CurlRequestUnitTest, SkipSetUserPwdWhenUserEmpty) {
    codePathCounter = 0;
    codePathString = "";
    curl_api_fake fake_api;
    CurlRequest req("", "psw", "http://myserver.local/req", "caCert", false, &fake_api);
    std::string *ptr = apiTracker(); 
    EXPECT_EQ("\n9slist_free_all", ptr[8]);
    EXPECT_TRUE(easySetOptParm == NULL);
    EXPECT_EQ("\n10ceasy_cleanup", ptr[9]);
    EXPECT_EQ(originalHandle, terminalHandle);
}

//11. verify that if Pwd is empty, then the following 2 calls are not invoked
//***********NOTE, it appears that the method throws Segmentation fault when password is empty.
TEST_F(CurlRequestUnitTest, SkipSetUserPwdWhenPwdEmpty) {
    codePathCounter = 0;
    codePathString = "";
    curl_api_fake fake_api;
    CurlRequest req("user", "", "http://myserver.local/req", "caCert", false, &fake_api);
    std::string *ptr = apiTracker(); 
    EXPECT_EQ("\n9slist_free_all", ptr[8]);
    EXPECT_TRUE(easySetOptParm == NULL);
    EXPECT_EQ("\n10ceasy_cleanup", ptr[9]);
    EXPECT_EQ(originalHandle, terminalHandle);
}

//12. verify that if username and password are empty, then the following 2 calls are not invoked.
//***********NOTE, it appears that the method throws Segmentation fault when usrname and password are empty.
TEST_F(CurlRequestUnitTest, SkipSetUserPwdWhenBothEmpty) {
    codePathCounter = 0;
    codePathString = "";
    curl_api_fake fake_api;
    CurlRequest req("", "", "http://myserver.local/req", "caCert", false, &fake_api);
    std::string *ptr = apiTracker(); 
    EXPECT_EQ("\n9slist_free_all", ptr[8]);
    EXPECT_TRUE(easySetOptParm == NULL);
    EXPECT_EQ("\n10ceasy_cleanup", ptr[9]);
    EXPECT_EQ(originalHandle, terminalHandle);
}

//13. test Null curl_api --yields a rather uncatchable segmentation fault.
 TEST_F(CurlRequestUnitTest, nullAPI) {
    codePathCounter = 0;
    codePathString = "";
    try
    {
      CurlRequest req("user", "psw", "http://myserver.local/req", "demo", false, NULL);
    }
    catch(std::exception& e)
    {
      std::string s2 = "Segmentation fault";
      bool found = false;
      std::string eString = strdup(e.what());
      if (eString.find(s2) != std::string::npos) {
          found = true;
        }
      std::cerr << "my error: " << eString <<std::endl;
      EXPECT_FALSE(found); //ideally, fail gracefully on 'found' being true; segFaults are difficult to catch
    }
 }

} // namespace

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

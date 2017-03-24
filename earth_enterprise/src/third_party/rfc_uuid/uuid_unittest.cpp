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

// Merge_unittest.cpp - unit tests for Merge classes

#include <stdlib.h>
#include <iostream>
#include <string>
#include <list>
#include <algorithm>
#include <UnitTest.h>
#include "uuid.h"


class UuidUnitTest : public UnitTest<UuidUnitTest> {
 public:
  UuidUnitTest() : BaseClass("UuidUnitTest") {
    REGISTER(UniquenessTest);
    REGISTER(FormatTest);
  }

  bool UniquenessTest() {
    std::list<std::string> results;
    int n = 1000;
    for(int i = 0; i < n; ++i) {
      std::string result = create_uuid_string();
      if (find(results.begin(), results.end(), result) != results.end()) {
        std::cerr << "Error encountered: " << result.c_str() << " twice!" << std::endl;
        return false;
      }
      results.push_back(result);
    }
    std::cerr << n << " UUIDs generated uniquely." << std::endl;
    return true;
  }
  bool FormatTest() {
    std::string result = create_uuid_string();
    std::cerr << result.c_str() << " size=" << result.size() << std::endl;


    if (result.size() != 36) {
      std::cerr << "Error encountered: " << result.c_str() <<
        " is wrong format, too few characters!" << std::endl;
      return false;
    }
    int hyphen_locations[] = {8, 13, 18, 23};
    for(int i = 0; i < 4; ++i) {
      if (result[hyphen_locations[i]] != '-') {
        std::cerr << "Error encountered: " << result.c_str() <<
          " has a non-hyphen in position " << i << "." << std::endl;
        return false;
      }
    }
    int current_index = 0;
    for(int i = 0; i < 36; ++i) {
      if (i == hyphen_locations[current_index]) {
        ++current_index;
        continue;
      }
      char value = result[i];
      if ((value < '0' && value > '9') && (value < 'a' && value > 'f')) {
        std::cerr << "Error encountered: " << result.c_str() <<
          " has a hex value, " << value << ", in position " << i << "." << std::endl;
        return false;
      }
    }
    return true;
  }
};

int main(int argc, char *argv[]) {

  UuidUnitTest tests;
  return tests.Run();
}

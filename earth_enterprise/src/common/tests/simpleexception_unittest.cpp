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

//
// Unit tests for khSimpleException

#include <string.h>
#include <UnitTest.h>
#include <khSimpleException.h>

class khSimpleExceptionUnitTest : public UnitTest<khSimpleExceptionUnitTest> {
 public:
  bool TestEmpty() {
    bool result = false;

    try {
      throw khSimpleException();
    } catch (const std::exception &e) {
      result = (strcmp(e.what(), "Unknown error")==0);
    }

    return result;
  }

  bool TestStatic() {
    bool result = false;
    const char * staticError = "Static error string";

    try {
      throw khSimpleException(staticError);
    } catch (const std::exception &e) {
      result = (strcmp(e.what(), staticError)==0);
    }

    return result;
  }

  bool TestStream() {
    bool result = false;
    std::string fname("foo.tif");

    try {
      throw khSimpleException() << "Unable to open " << fname;
    } catch (const std::exception &e) {
      result = (strcmp(e.what(), "Unable to open foo.tif")==0);
    }

    return result;
  }

  bool TestErrno() {
    bool result = false;
    std::string fname("foo.tif");

    try {
      errno = EPERM;
      throw khSimpleErrnoException() << "Unable to open " << fname;
    } catch (const std::exception &e) {
      result = (strcmp(e.what(),
                       "Unable to open foo.tif: Operation not permitted")==0);
    }

    return result;
  }



  khSimpleExceptionUnitTest(void) : BaseClass("khSimpleException") {
    REGISTER(TestEmpty);
    REGISTER(TestStatic);
    REGISTER(TestStream);
    REGISTER(TestErrno);
  }
};

int main(int argc, char *argv[]) {
  khSimpleExceptionUnitTest tests;

  return tests.Run();
}

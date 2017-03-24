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

#include <UnitTest.h>
#include <geLockfileGuard.h>
#include <khFileUtils.h>

class geLockfileGuardUnitTest : public UnitTest<geLockfileGuardUnitTest> {
 public:
  bool TestBasic() {
    // make a temporary filename that we can play with
    std::string tmp_filename = khTmpFilename("lockguardtest");
    khUnlink(tmp_filename);

    {
      std::string test_contents = "Testing...";
      geLockfileGuard guard(tmp_filename, test_contents);


      if (!khExists(tmp_filename)) {
        fprintf(stderr, "File doesn't exist\n");
        return false;
      }

      std::string contents;
      if (!khReadStringFromFile(tmp_filename, contents)) {
        fprintf(stderr, "Cannot read file\n");
        return false;
      }

      if (contents != test_contents) {
        fprintf(stderr, "File contents don't match\n");
        return false;
      }
    }

    if (khExists(tmp_filename)) {
      fprintf(stderr, "File still exists\n");
      return false;
    }

    return true;
  }

  geLockfileGuardUnitTest(void) : BaseClass("geLockfileGuard") {
    REGISTER(TestBasic);
  }
};

int main(int argc, char *argv[]) {
  geLockfileGuardUnitTest tests;

  return tests.Run();
}


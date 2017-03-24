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
#include <DottedVersion.h>

class khDottedVersionUnitTest : public UnitTest<khDottedVersionUnitTest> {
 public:
  bool TestConstructor() {
    return ((DottedVersion("").ToString() == "") &&
            (DottedVersion("2").ToString() == "2") &&
            (DottedVersion("3.1").ToString() == "3.1") &&
            (DottedVersion("4.2.152").ToString() == "4.2.152") &&
            (DottedVersion("194.2.152").ToString() == "194.2.152"));
  }

  bool Different(const DottedVersion &less, const DottedVersion &more) {
    return (( (less <  more) &&
              (less <= more) &&
             !(less >  more) &&
             !(less >= more) &&
             !(less == more) &&
              (less != more))
            &&
            (!(more <  less) &&
             !(more <= less) &&
              (more >  less) &&
              (more >= less) &&
             !(more == less) &&
              (more != less)));
  }
  bool Same(const DottedVersion &ver) {
    return (!(ver <  ver) &&
             (ver <= ver) &&
            !(ver >  ver) &&
             (ver >= ver) &&
             (ver == ver) &&
             !(ver != ver));
  }

  bool SingleComparisons() {
    return (Different(DottedVersion("1"), DottedVersion("2")) &&
            Same(DottedVersion("3")) &&
            Same(DottedVersion("")));
  }
  bool DoubleComparisons() {
    return (Different(DottedVersion("2.1"), DottedVersion("2.2")) &&
            Same(DottedVersion("2.3")));
  }

  bool AlphaComparison() {
    DottedVersion v3_0_alpha_0 ("3.0.alpha.0");
    DottedVersion v3_0_0 ("3.0.0");

    return v3_0_alpha_0 < v3_0_0;
  }


  bool ComplexComparisons() {
    DottedVersion vNone  ("");
    DottedVersion v2     ("2");
    DottedVersion v2_1   ("2.1");
    DottedVersion v2_1_0 ("2.1.0");
    DottedVersion v2_1_1 ("2.1.1");
    DottedVersion v3     ("3");
    DottedVersion v4_9   ("4.9");
    DottedVersion v4_10  ("4.10");
    DottedVersion v3_0_alpha_0 ("3.0.alpha.0");
    DottedVersion v3_0_0 ("3.0.0");

    return (Different(vNone,  v2) &&
            Different(vNone,  v2_1) &&
            Different(v2,     v2_1) &&
            Different(v2,     v2_1_1) &&
            Different(v2_1,   v2_1_0) &&
            Different(v2_1,   v2_1_1) &&
            Different(v2_1,   v3) &&
            Different(v2_1_1, v3) &&
            Different(v4_9,   v4_10) &&
            Different(v3_0_alpha_0, v3_0_0)
            );
  }

  khDottedVersionUnitTest(void) : BaseClass("khDottedVersion") {
    REGISTER(TestConstructor);
    REGISTER(SingleComparisons);
    REGISTER(DoubleComparisons);
    REGISTER(AlphaComparison);
    REGISTER(ComplexComparisons);
  }
};

int main(int argc, char *argv[]) {
  khDottedVersionUnitTest tests;

  return tests.Run();
}


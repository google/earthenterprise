// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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
// Unit tests for geSegmentedArray

#include <geSegmentedArray.h>
#include <UnitTest.h>


class CountingClass {
 public:
  static unsigned int count;

  CountingClass(void) { ++count; }
  CountingClass(const CountingClass &) { ++count; }
  ~CountingClass(void) { --count; }
};
unsigned int CountingClass::count = 0;


class geSegmentedArrayTest : public UnitTest<geSegmentedArrayTest> {
 public:

  bool Harness(bool (geSegmentedArrayTest::*testFunc)(unsigned int, unsigned int)) {
    return 
      // segment size 1
      (this->*testFunc)(0, 0) &&
      (this->*testFunc)(0, 1) &&
      (this->*testFunc)(0, 2) &&
      (this->*testFunc)(0, 3) &&
      (this->*testFunc)(0, 50) &&

      // segment size 2
      (this->*testFunc)(1, 0) &&
      (this->*testFunc)(1, 1) &&
      (this->*testFunc)(1, 2) &&
      (this->*testFunc)(1, 3) &&
      (this->*testFunc)(1, 4) &&
      (this->*testFunc)(1, 5) &&
      (this->*testFunc)(1, 50) &&

      // segment size 4
      (this->*testFunc)(2, 0) &&
      (this->*testFunc)(2, 1) &&
      (this->*testFunc)(2, 2) &&
      (this->*testFunc)(2, 3) &&
      (this->*testFunc)(2, 4) &&
      (this->*testFunc)(2, 5) &&
      (this->*testFunc)(2, 6) &&
      (this->*testFunc)(2, 7) &&
      (this->*testFunc)(2, 8) &&
      (this->*testFunc)(2, 9) &&
      (this->*testFunc)(2, 50) &&

      // segment size 1024
      (this->*testFunc)(10, 0) &&
      (this->*testFunc)(10, 1) &&
      (this->*testFunc)(10, 1023) &&
      (this->*testFunc)(10, 1024) &&
      (this->*testFunc)(10, 1025) &&
      (this->*testFunc)(10, 2047) &&
      (this->*testFunc)(10, 2048) &&
      (this->*testFunc)(10, 2049) &&
      (this->*testFunc)(10, 10000);
  }

  bool BasicSegmentationSub(unsigned int segSizeLog2, unsigned int iterations) {
    geSegmentedArray< unsigned int>  array(segSizeLog2);

    // push_back test
    for (unsigned int i = 0; i < iterations; ++i) {
      array.push_back(i);
    }

    
    // size test
    if (array.size() != iterations) {
      fprintf(stderr,
              "BasicSegmentation(%u, %u): Bad size %u != %u\n",
              segSizeLog2, iterations,
              (unsigned int)array.size(), iterations);
      return false;
    }

    // num segments test
    unsigned int expectedSegmentCount =
      (iterations + array.kSegmentSize - 1)/array.kSegmentSize;
    if (array.segments.size() != expectedSegmentCount) {
      fprintf(stderr,
              "BasicSegmentation(%u, %u): Bad number of segments %u != %u\n",
              segSizeLog2, iterations,
              (unsigned int)array.segments.size(), expectedSegmentCount);
      return false;
    }

    // value fetch test
    for (unsigned int i = 0; i < iterations; ++i) {
      if (array[i] != i) {
        fprintf(stderr,
                "BasicSegmentation(%u, %u): Bad push_back value [%u] == %u\n",
                segSizeLog2, iterations,
                i, array[i]);
        return false;
      }
    }

    // value set test
    for (unsigned int i = 0; i < iterations; ++i) {
      array[i] = iterations - i;
    }
    for (unsigned int i = 0; i < iterations; ++i) {
      if (array[i] != iterations - i) {
        fprintf(stderr,
                "BasicSegmentation(%u, %u): Bad set value [%u] == %u\n",
                segSizeLog2, iterations,
                i,  array[i]);
        return false;
      }
    }

    return true;
  }
  bool BasicSegmentation(void) {
    return Harness(&geSegmentedArrayTest::BasicSegmentationSub);
  }



  bool ConstructDestroySub(unsigned int segSizeLog2, unsigned int iterations) {
    CountingClass::count = 0;

    {
      geSegmentedArray<CountingClass> array(segSizeLog2);

      for (unsigned int i = 0; i < iterations; ++i) {
        array.push_back(CountingClass());
      }

      // make sure we have the right number of constructed objects
      if (CountingClass::count != iterations) {
        fprintf(stderr,
                "ConstructDestroy(%u, %u): Bad count %u != %u\n",
                segSizeLog2, iterations,
                CountingClass::count, iterations);
        return false;
      }
    }

    // make sure all those objects have been destroyed
    if (CountingClass::count != 0) {
      fprintf(stderr,
              "ConstructDestroy(%u, %u): Bad count %u != 0\n",
              segSizeLog2, iterations,
              CountingClass::count);
      return false;
    }

    return true;
  }
  bool ConstructDestroy(void) {
    return Harness(&geSegmentedArrayTest::ConstructDestroySub);
  }


  geSegmentedArrayTest(void) : BaseClass("geSegmentedArrayTest") {
    REGISTER(BasicSegmentation);
    REGISTER(ConstructDestroy);
  }
};

int main(int argc, char *argv[]) {
  geSegmentedArrayTest tests;

  return tests.Run();
}

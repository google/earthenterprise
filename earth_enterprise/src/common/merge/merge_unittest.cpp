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


// Merge_unittest.cpp - unit tests for Merge classes

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <khSimpleException.h>
#include <quadtreepath.h>
#include <UnitTest.h>
#include "merge.h"


class MergeUnitTest : public UnitTest<MergeUnitTest> {
 public:
  MergeUnitTest(bool verbose) : BaseClass("MergeUnitTest"), verbose_(verbose) {
    REGISTER(SimpleNumbersTest);
    REGISTER(CascadeTest);
  }

  bool verbose_;

  //---------------------------------------------------------------------------

  // Define numbers merge class

  class MergeSourceNumbers : public MergeSource<int> {
   public:
    MergeSourceNumbers(std::string name, int init, int increment, int limit)
        : MergeSource<int>(name),
          init_(init),
          increment_(increment),
          limit_(limit),
          current_(init) {}
    ~MergeSourceNumbers() {}
    virtual const int &Current() const {
      if (current_ < limit_)
        return current_;
      else
        throw khSimpleException("MergeSourceNumbers: Current called after end");
    }
    virtual void Close() {}
    virtual bool Advance() {
      current_ += increment_;
      return current_ < limit_;
    }
   private:
    int init_, increment_, limit_, current_;
  };

  bool SimpleNumbersTest() {
    const int num_sources = 10;
    std::uint32_t input_value_count = 0;

    Merge<int> merge("NumbersMerge");
    int limit = num_sources*3;

    MergeSourceNumbers *source;
    for (int i = 0; i < num_sources; ++i) {
      char name[16];
      snprintf(name, 16, "Src%d", i);
      int increment = 1 + i*2;
      source = new MergeSourceNumbers(name, i, increment, limit);
      if (i < num_sources-1) {          // don't add last source yet
        merge.AddSource(TransferOwnership(source));
      }
      input_value_count += (limit - i + increment - 1)/increment;
    }

    std::uint32_t output_value_count = 0;
    merge.Start();
    int prev = -1;

    while (merge.Active()) {
      const int &cur = merge.Current();
      if (prev > cur) {
        throw khSimpleException("SimpleNumbersTest: Current() out of order, ")
          << prev << " > " << cur << ", source " << merge.CurrentName();
      } else {
        prev = cur;
      }
      if (verbose_) {
        std::cout << "<" << cur << std::endl;
      }

      merge.Advance();
      ++output_value_count;

      // Test add to active merge
      if (source) {
        merge.AddSource(TransferOwnership(source));
        source = NULL;
      }
    }
       
    merge.Close();

    if (input_value_count != output_value_count) {
      std::cerr << "Error: Input value count: " << input_value_count
                << " != Output value count: " << output_value_count << std::endl;
      return false;
    }
    return true;
  }

  //---------------------------------------------------------------------------

  // Test cascaded merge using random preorder tree walks

  // Generate random numbers in range [0..max_val]
  static int Random(int max_val) {
    return static_cast<int>
      (static_cast<double>(random())
       / (1.000001 * static_cast<double>(RAND_MAX))
       * (1.0 + static_cast<double>(max_val)));
  }

  class MergeSourceRandomWalk : public MergeSource<QuadtreePath> {
   public:
    MergeSourceRandomWalk(std::string name,
                          std::string start_path,
                          int max_walk,
                          int skip_range)
        : MergeSource<QuadtreePath>(name),
          current_(start_path),
          walk_length_(0),
          max_walk_(max_walk),
          skip_range_(skip_range) {
    }
    virtual const QuadtreePath &Current() const {
      if (walk_length_ <= max_walk_)
        return current_;
      else
        throw khSimpleException("MergeSourceRandomWalk: Current called after end");
    }
    virtual void Close() {}
    virtual bool Advance() {
      if (++walk_length_ < max_walk_) {
        int skip = 1 + Random(skip_range_ - 1);
        do {
          TestAssert(current_.Advance(QuadtreePath::kMaxLevel));
        } while (--skip > 0);
        return true;
      } else {
        return false;
      }
    }
   private:
    QuadtreePath current_;
    int walk_length_;                   // length of path so far
    int max_walk_;
    int skip_range_;
  };

  bool CascadeTest() {
    int input_value_count = 0;
    const int kValuesPerTest = 1000;

    // stage1 and stage2 will feed into stage3
    Merge<QuadtreePath> *stage1 = new Merge<QuadtreePath>("Stage1");
    stage1->AddSource(
        TransferOwnership(
            new MergeSourceRandomWalk("Stage1A",
                                      "1031",
                                      kValuesPerTest,
                                      4)));
    input_value_count += kValuesPerTest;
    stage1->AddSource(
        TransferOwnership(
            new MergeSourceRandomWalk("Stage1B",
                                      "10313",
                                      kValuesPerTest,
                                      4)));
    input_value_count += kValuesPerTest;
    stage1->AddSource(
        TransferOwnership(
            new MergeSourceRandomWalk("Stage1C",
                                      "1031",
                                      kValuesPerTest,
                                      4)));
    input_value_count += kValuesPerTest;
    stage1->Start();

    Merge<QuadtreePath> *stage2 = new Merge<QuadtreePath>("Stage2");
    stage2->AddSource(
        TransferOwnership(
            new MergeSourceRandomWalk("Stage2A",
                                      "10313",
                                      kValuesPerTest,
                                      4)));
    input_value_count += kValuesPerTest;
    stage2->AddSource(
        TransferOwnership(
            new MergeSourceRandomWalk("Stage2B",
                                      "1031",
                                      kValuesPerTest,
                                      4)));
    input_value_count += kValuesPerTest;
    stage2->AddSource(
        TransferOwnership(
            new MergeSourceRandomWalk("Stage2C",
                                      "1031",
                                      kValuesPerTest,
                                      4)));
    input_value_count += kValuesPerTest;
    stage2->AddSource(
        TransferOwnership(
            new MergeSourceRandomWalk("Stage2D",
                                      "10313",
                                      kValuesPerTest,
                                      4)));
    input_value_count += kValuesPerTest;
    stage2->Start();

    // stage3 has two native sources, plus stage1 and stage2
    Merge<QuadtreePath> stage3("Stage3");
    stage3.AddSource(
        TransferOwnership(
            new MergeSourceRandomWalk("Stage3A",
                                      "1031",
                                      kValuesPerTest,
                                      4)));
    input_value_count += kValuesPerTest;
    stage3.AddSource(
        TransferOwnership(
            new MergeSourceRandomWalk("Stage3B",
                                      "1031",
                                      kValuesPerTest,
                                      4)));
    input_value_count += kValuesPerTest;

    stage3.AddSource(TransferOwnership(stage1));
    stage1 = NULL;                      // owned by stage3 now
    stage3.AddSource(TransferOwnership(stage2));
    stage2 = NULL;                      // owned by stage3 now

    stage3.Start();
    QuadtreePath prev;
    int output_value_count = 0;

    while (stage3.Active()) {
      const QuadtreePath &cur = stage3.Current();
      if (prev > cur) {
        throw khSimpleException("CascadeTest: Current() out of order, ")
          << prev.AsString() << " > " << cur.AsString()
          << ", source " << stage3.CurrentName();
      } else {
        prev = cur;
      }
      if (verbose_) {
        std::cout << "< " << cur.AsString() << std::endl;
      }

      stage3.Advance();
      ++output_value_count;
    }

    stage3.Close();

    if (input_value_count != output_value_count) {
      std::cerr << "Error: Input value count: " << input_value_count
                << " != Output value count: " << output_value_count << std::endl;
      return false;
    }
    return true;
  }
};

int main(int argc, char *argv[]) {
  bool debug = false;
  
  if (argc > 1) {
    if (argc == 2  &&  strcmp(argv[1], "debug") == 0) {
      debug = true;
    } else {
      std::cerr << "usage: merge_unittest [debug]" << std::endl;
      return 1;
    }
  }

  MergeUnitTest tests(debug);
  return tests.Run();
}

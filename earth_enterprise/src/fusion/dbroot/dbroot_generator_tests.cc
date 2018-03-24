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
// tests for dbroot generation
// These tests use real-life config files for the external dbroot tools
// and compare the output against golden output files


#include <algorithm>
#include <gtest/gtest.h>
#include "common/khFileUtils.h"
#include "fusion/dbroot/vector_dbroot_context.h"
#include "fusion/dbroot/raster_dbroot_context.h"
#include "fusion/dbroot/proto_dbroot_combiner.h"


class Test_dbrootgen : public testing::Test {
 protected:
  std::string work_dir_;

  virtual void SetUp() {
    const ::testing::TestInfo * test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();

    // create a temp dir for the test to work in
    work_dir_ = khCreateTmpDir(std::string(test_info->test_case_name()) +
                               std::string(test_info->name()));
    // khCreateTmpDir reports failure by returning an empty string
    ASSERT_FALSE(work_dir_.empty());
  }

  virtual void TearDown() {
    khPruneFileOrDir(work_dir_);
  }

  void OverrideProvidersAndLocales(ProtoDbrootContext *context) {
    // setup a consistent set of providers for testcases to use
    // otherwise tests would fail on machines with assetroots that
    // have different providers (99% of them)
    context->provider_map_.clear();
    context->provider_map_[1] =
        gstProvider("NASA",            // name
                    "NASA",            // key
                    "\xA9 NASA 1900",  // copyright
                    -1,                // copyrightVerticalPos
                    false,             // report
                    256,               // copyrightPriority
                    1);                // id
    context->provider_map_[2] =
        gstProvider("John",            // name
                    "John",            // key
                    "\xA9 John 2000",  // copyright
                    -1,                // copyrightVerticalPos
                    false,             // report
                    256,               // copyrightPriority
                    2);                // id

    context->locale_set_.AddLocaleForTesting("it");
  }

  void RunVectorGolden(const std::string &golden_dir) {
    std::string input_config   = khComposePath(golden_dir, "config.xml");

    // Create my context object
    VectorDbrootContext context(input_config, ProtoDbrootContext::kForTesting);
    OverrideProvidersAndLocales(&context);  // use testcase versions

    // Generate all the dbroots. icons are skipped in test mode
    context.EmitAll(work_dir_, geProtoDbroot::kTextFormat);

    CheckGoldenResults(golden_dir, &context);
  }

  void RunRasterGolden(const std::string &golden_dir,
                       bool is_imagery) {
    std::string input_config   = khComposePath(golden_dir, "config.xml");

    // Create my context object
    RasterDbrootContext context(input_config, is_imagery,
                                ProtoDbrootContext::kForTesting);
    OverrideProvidersAndLocales(&context);  // use testcase versions

    // Generate all the dbroots. icons are skipped in test mode
    context.EmitAll(work_dir_, geProtoDbroot::kTextFormat);

    CheckGoldenResults(golden_dir, &context);
  }

  void RunFormat(const std::string &golden_dir,
                 geProtoDbroot::FileFormat output_format) {
    std::string input_config   = khComposePath(golden_dir, "config.xml");

    // Create my context object
    VectorDbrootContext context(input_config, ProtoDbrootContext::kForTesting);
    OverrideProvidersAndLocales(&context);  // use testcase versions

    // Generate all the dbroots. icons are skipped in test mode
    context.EmitAll(work_dir_, output_format);

    geProtoDbroot emitted(khComposePath(work_dir_,
                                        kPostamblePrefix + ".DEFAULT"),
                          output_format);
    emitted.Write(khComposePath(work_dir_, "dbroot.text"),
                  geProtoDbroot::kTextFormat);
    EXPECT_TRUE(khFilesEqual(
        khComposePath(work_dir_, "dbroot.text"),
        khComposePath(golden_dir, kPostamblePrefix + ".DEFAULT")));
  }

  void CheckGoldenResults(const std::string &golden_dir,
                          ProtoDbrootContext *context) {
    // Make sure we generated exactly the locales we were expecting
    std::vector<std::string> generated_dbroots;
    khGetBasenamesMatchingPattern(work_dir_, kPostamblePrefix, "",
                                  &generated_dbroots);
    std::sort(generated_dbroots.begin(), generated_dbroots.end());
    std::vector<std::string> golden_dbroots;
    khGetBasenamesMatchingPattern(golden_dir, kPostamblePrefix, "",
                                  &golden_dbroots);
    std::sort(golden_dbroots.begin(), golden_dbroots.end());
    ASSERT_TRUE(golden_dbroots == generated_dbroots);

    // Now make sure each of the output dbroots matches it's golden counterpart
    uint size = generated_dbroots.size();
    for (uint i = 0; i < size; ++i) {
      EXPECT_TRUE(khFilesEqual(
          khComposePath(work_dir_, generated_dbroots[i]),
          khComposePath(golden_dir, golden_dbroots[i])));
    }

    // dump list of icon paths used
    std::string icon_list_file = "icons.list";
    std::vector<std::string> icon_paths;
    typedef ProtoDbrootContext::UsedIconSet::const_iterator IconIterator;
    for (IconIterator i = context->used_legend_icons_.begin();
         i != context->used_legend_icons_.end(); ++i) {
      icon_paths.push_back(i->LegendHref());
    }
    for (IconIterator i = context->used_normal_icons_.begin();
         i != context->used_normal_icons_.end(); ++i) {
      icon_paths.push_back(i->NormalHref());
    }
    for (IconIterator i = context->used_highlight_icons_.begin();
         i != context->used_highlight_icons_.end(); ++i) {
      icon_paths.push_back(i->HighlightHref());
    }
    for (IconIterator i = context->used_normalhighlight_icons_.begin();
         i != context->used_normalhighlight_icons_.end(); ++i) {
      icon_paths.push_back(i->NormalHighlightHref());
    }
    std::sort(icon_paths.begin(), icon_paths.end());
    khWriteStringVectorToFile(khComposePath(work_dir_, icon_list_file),
                              icon_paths);

    // check icon list against golden icon list
    EXPECT_TRUE(khFilesEqual(khComposePath(work_dir_, icon_list_file),
                             khComposePath(golden_dir, icon_list_file)));
  }
};

// Test vector
TEST_F(Test_dbrootgen, Vector1) {
  RunVectorGolden("fusion/testdata/dbroot/vector1");
}

// Test imagery
TEST_F(Test_dbrootgen, ImageryTest1) {
  RunRasterGolden("fusion/testdata/dbroot/imagery1",
                  true /* is_imagery */);
}

// Test terrain
TEST_F(Test_dbrootgen, TerrainTest1) {
  RunRasterGolden("fusion/testdata/dbroot/terrain1",
                  false /* is_imagery */);
}

// Test binary output
TEST_F(Test_dbrootgen, ProtoFormatTest1) {
  RunFormat("fusion/testdata/dbroot/vector1", geProtoDbroot::kProtoFormat);
}

// Test binary output
TEST_F(Test_dbrootgen, EncodedFormatTest1) {
  RunFormat("fusion/testdata/dbroot/vector1", geProtoDbroot::kEncodedFormat);
}

class Test_dbroot_combiner : public testing::Test {
 protected:
  std::string work_dir_;

  virtual void SetUp() {
    const ::testing::TestInfo * test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();

    // create a temp dir for the test to work in
    work_dir_ = khCreateTmpDir(std::string(test_info->test_case_name()) +
                               std::string(test_info->name()));
    // khCreateTmpDir reports failure by returning an empty string
    ASSERT_FALSE(work_dir_.empty());
  }

  virtual void TearDown() {
    khPruneFileOrDir(work_dir_);
  }

  void RunGolden(const std::string &golden_dir) {
    std::vector<std::string> input_dbroots;
    khGetNumericFilenames(khComposePath(golden_dir, "input_dbroot"),
                          input_dbroots);

    ProtoDbrootCombiner combiner;
    for (std::vector<std::string>::const_iterator input = input_dbroots.begin();
         input != input_dbroots.end(); ++input) {
      combiner.AddDbroot(*input, geProtoDbroot::kTextFormat);
    }

    std::string output_file = khComposePath(work_dir_, "combined");
    std::string golden_file = khComposePath(golden_dir, "combined");
    combiner.Write(output_file, geProtoDbroot::kTextFormat);

    EXPECT_TRUE(khFilesEqual(output_file, golden_file));
  }
};


// Test basic
TEST_F(Test_dbroot_combiner, CombinerTest1) {
  RunGolden("fusion/testdata/dbroot/combined1");
}


int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

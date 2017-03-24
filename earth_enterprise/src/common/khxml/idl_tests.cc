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
// tests for IDL parser
// also uses classes from test_classes.idl

#include <gtest/gtest.h>
#include "common/khFileUtils.h"
#include "common/khxml/.idl/test_classes.h"


namespace {

class IdlGtestEnv : public ::testing::Environment {
 public:
  std::string work_dir_;
  std::string source_dir_;

  virtual void SetUp() {
    // create a temp dir for all tests to work in
    work_dir_ = khCreateTmpDir("IdlTests");
    // khCreateTmpDir reports failure by returning an empty string
    ASSERT_FALSE(work_dir_.empty());

    // where our test data lives
    source_dir_ = "fusion/testdata/khxml";
    ASSERT_TRUE(khDirExists(source_dir_));
  }

  virtual void TearDown() {
    khPruneFileOrDir(work_dir_);
  }
};

IdlGtestEnv *global_env;


class IdlGtest : public testing::Test { };

TEST_F(IdlGtest, VersionNew) {
  IdlTestVersion obj;
  EXPECT_EQ(3, obj.idl_version);
}

TEST_F(IdlGtest, VersionSave) {
  std::string save_path = khComposePath(global_env->work_dir_, "save.xml");
  IdlTestVersion obj;
  obj.idl_version = 2;
  ASSERT_TRUE(obj.Save(save_path));

  IdlTestVersion obj2;
  ASSERT_TRUE(obj2.Load(save_path));
  EXPECT_EQ(3, obj2.idl_version);
}

TEST_F(IdlGtest, VersionLoad) {
  std::string load_path = khComposePath(global_env->source_dir_,
                                        "version_notset.xml");
  IdlTestVersion obj;
  ASSERT_TRUE(obj.Load(load_path));
  EXPECT_EQ(0, obj.idl_version);

  load_path = khComposePath(global_env->source_dir_, "version_2.xml");
  IdlTestVersion obj2;
  ASSERT_TRUE(obj2.Load(load_path));
  EXPECT_EQ(2, obj2.idl_version);
}

TEST_F(IdlGtest, VersionIsCurrent) {
  std::string load_path = khComposePath(global_env->source_dir_,
                                        "version_notset.xml");
  IdlTestVersion obj;
  ASSERT_TRUE(obj.Load(load_path));
  EXPECT_FALSE(obj._IsCurrentIdlVersion());

  load_path = khComposePath(global_env->source_dir_, "version_2.xml");
  IdlTestVersion obj2;
  ASSERT_TRUE(obj2.Load(load_path));
  EXPECT_FALSE(obj2._IsCurrentIdlVersion());

  load_path = khComposePath(global_env->source_dir_, "version_3.xml");
  IdlTestVersion obj3;
  ASSERT_TRUE(obj3.Load(load_path));
  EXPECT_TRUE(obj3._IsCurrentIdlVersion());

  IdlTestVersion obj4;
  EXPECT_TRUE(obj4._IsCurrentIdlVersion());
}

TEST_F(IdlGtest, VersionUpdateIdlVersion) {
  std::string load_path = khComposePath(global_env->source_dir_,
                                        "version_notset.xml");
  IdlTestVersion obj;
  ASSERT_TRUE(obj.Load(load_path));
  EXPECT_FALSE(obj._IsCurrentIdlVersion());
  obj._UpdateIdlVersion();
  EXPECT_TRUE(obj._IsCurrentIdlVersion());

  IdlTestVersion obj2;
  EXPECT_TRUE(obj2._IsCurrentIdlVersion());
  obj2._UpdateIdlVersion();
  EXPECT_TRUE(obj2._IsCurrentIdlVersion());
}

TEST_F(IdlGtest, VersionIsUpToDate1) {
  std::string load_path = khComposePath(global_env->source_dir_,
                                        "version_notset.xml");
  IdlTestVersion obj;
  ASSERT_TRUE(obj.Load(load_path));

  // load same file again
  IdlTestVersion obj2;
  ASSERT_TRUE(obj2.Load(load_path));

  EXPECT_TRUE(::IsUpToDate(obj, obj2));
}

TEST_F(IdlGtest, VersionIsUpToDate2) {
  std::string load_path = khComposePath(global_env->source_dir_,
                                        "version_notset.xml");
  IdlTestVersion obj;
  ASSERT_TRUE(obj.Load(load_path));

  // load same file again
  IdlTestVersion obj2;
  ASSERT_TRUE(obj2.Load(load_path));

  obj2._UpdateIdlVersion();
  EXPECT_FALSE(::IsUpToDate(obj, obj2));
}

TEST_F(IdlGtest, VersionIsUpToDate3) {
  std::string load_path = khComposePath(global_env->source_dir_,
                                        "version_notset.xml");
  IdlTestVersion obj;
  ASSERT_TRUE(obj.Load(load_path));

  // load same file again
  IdlTestVersion obj2;
  ASSERT_TRUE(obj2.Load(load_path));

  obj2._UpdateIdlVersion();
  obj._UpdateIdlVersion();
  EXPECT_TRUE(::IsUpToDate(obj, obj2));
}

TEST_F(IdlGtest, NoVersionIsCurrent) {
  IdlTestNoVersion obj;
  EXPECT_TRUE(obj._IsCurrentIdlVersion());
}

TEST_F(IdlGtest, NoVersionUpdateIdlVersion) {
  IdlTestNoVersion obj;
  // just have to make sure it compiles
  obj._UpdateIdlVersion();
}



}  // namespace


int main(int argc, char **argv) {
  global_env = new IdlGtestEnv();
  ::testing::AddGlobalTestEnvironment(global_env);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

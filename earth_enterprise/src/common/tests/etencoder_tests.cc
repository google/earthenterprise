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
// tests for etEncoder


#include <string>
#include <gtest/gtest.h>
#include "common/etencoder.h"
#include "common/khSimpleException.h"

class Test_crypt : public testing::Test {
 protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

  void RunBasic(std::string &data, const std::string &key,
                const std::string &golden) {
    etEncoder::Encode(&data[0], data.size(), &key[0], key.size());

    EXPECT_EQ(golden, data);
  }
};


// The etEncoder::Encode routine is pretty touchy.
// The key needs a length that's a multiple of 8 and at least 24
std::string OffKey    = std::string(32, '\x00');
std::string OnKey     = std::string(32, '\xff');
std::string MixedKey  = std::string(32, '\x36');
std::string RandomKey =
    std::string("qsdfjhsdfjhskjfhkjhsdf kljhsdfjkhsdfjkhs");
std::string InvalidLengthKey = std::string(5, '\x00');

TEST_F(Test_crypt, EmptyOffKey) {
  std::string data;
  RunBasic(data, OffKey, std::string());
}
TEST_F(Test_crypt, EmptyOnKey) {
  std::string data;
  RunBasic(data, OnKey, std::string());
}
TEST_F(Test_crypt, OffKey) {
  std::string data(128, '\x59');
  RunBasic(data, OffKey, std::string(128, '\x59'));
}
TEST_F(Test_crypt, OnKey) {
  std::string data(128, '\x4d');
  RunBasic(data, OnKey, std::string(128, '\xb2'));
}
TEST_F(Test_crypt, MixedKey) {
  std::string data(128, '\xa6');
  RunBasic(data, MixedKey, std::string(128, '\x90'));
}
TEST_F(Test_crypt, Random) {
  std::string data(50, '\xa6');
  RunBasic(data, RandomKey,
           std::string("\xCD\xCC\xCE\xD5\xC2\xC0\x86\xCD\xD7\xD5"
                       "\xC2\xC0\xCC\xCE\xD5\xC2\xCA\xCC\xCE\xD5"
                       "\xC2\xC0\xCC\xCD\xC0\xCC\xCE\xD5\xCD\xCC"
                       "\xC0\xCE\xCE\xD5\xC2\xC0\xCC\xCD\xCE\xD5"
                       "\xCD\xCC\xCE\xD5\xC2\xC0\x86\xCD\xD7\xD5"));
}
TEST_F(Test_crypt, LongRandom) {
  std::string data(253, '\xa8');
  RunBasic(data, RandomKey,
           std::string("\xC3\xC2\xC0\xDB\xCC\xCE\x88\xC3\xD9\xDB"
                       "\xCC\xCE\xC2\xC0\xDB\xCC\xC4\xC2\xC0\xDB"
                       "\xCC\xCE\xC2\xC3\xCE\xC2\xC0\xDB\xC3\xC2"
                       "\xCE\xC0\xC0\xDB\xCC\xCE\xC2\xC3\xC0\xDB"
                       "\xC3\xC2\xC0\xDB\xCC\xCE\x88\xC3\xD9\xDB"
                       "\xCC\xCE\xC2\xC0\xDB\xCC\xC4\xC2\xC0\xDB"
                       "\xCC\xCE\xC2\xC3\xCE\xC2\xC0\xDB\xC3\xC2"
                       "\xCE\xC0\xC0\xDB\xCC\xCE\xC2\xC3\xC0\xDB"
                       "\xC3\xC2\xC0\xDB\xCC\xCE\x88\xC3\xD9\xDB"
                       "\xCC\xCE\xC2\xC0\xDB\xCC\xC4\xC2\xC0\xDB"
                       "\xCC\xCE\xC2\xC3\xCE\xC2\xC0\xDB\xC3\xC2"
                       "\xCE\xC0\xC0\xDB\xCC\xCE\xC2\xC3\xC0\xDB"
                       "\xC3\xC2\xC0\xDB\xCC\xCE\x88\xC3\xD9\xDB"
                       "\xCC\xCE\xC2\xC0\xDB\xCC\xC4\xC2\xC0\xDB"
                       "\xCC\xCE\xC2\xC3\xCE\xC2\xC0\xDB\xC3\xC2"
                       "\xCE\xC0\xC0\xDB\xCC\xCE\xC2\xC3\xC0\xDB"
                       "\xC3\xC2\xC0\xDB\xCC\xCE\x88\xC3\xD9\xDB"
                       "\xCC\xCE\xC2\xC0\xDB\xCC\xC4\xC2\xC0\xDB"
                       "\xCC\xCE\xC2\xC3\xCE\xC2\xC0\xDB\xC3\xC2"
                       "\xCE\xC0\xC0\xDB\xCC\xCE\xC2\xC3\xC0\xDB"
                       "\xC3\xC2\xC0\xDB\xCC\xCE\x88\xC3\xD9\xDB"
                       "\xCC\xCE\xC2\xC0\xDB\xCC\xC4\xC2\xC0\xDB"
                       "\xCC\xCE\xC2\xC3\xCE\xC2\xC0\xDB\xC3\xC2"
                       "\xCE\xC0\xC0\xDB\xCC\xCE\xC2\xC3\xC0\xDB"
                       "\xC3\xC2\xC0\xDB\xCC\xCE\x88\xC3\xD9\xDB"
                       "\xCC\xCE\xC2"));
}
TEST_F(Test_crypt, ShortData) {
  std::string data(1, '\xa6');
  RunBasic(data, MixedKey, std::string(1, '\x90'));
}
TEST_F(Test_crypt, OneShyData) {
  std::string data(MixedKey.size()-1, '\xa6');
  RunBasic(data, MixedKey, std::string(MixedKey.size()-1, '\x90'));
}
TEST_F(Test_crypt, OneMoreData) {
  std::string data(MixedKey.size()+1, '\xa6');
  RunBasic(data, MixedKey, std::string(MixedKey.size()+1, '\x90'));
}
TEST_F(Test_crypt, SameLenData) {
  std::string data(MixedKey.size(), '\xa6');
  RunBasic(data, MixedKey, std::string(MixedKey.size(), '\x90'));
}
TEST_F(Test_crypt, TenXData) {
  std::string data(MixedKey.size()*10, '\xa6');
  RunBasic(data, MixedKey, std::string(MixedKey.size()*10, '\x90'));
}
TEST_F(Test_crypt, FifteenXPlus3Data) {
  std::string data(MixedKey.size()*15+3, '\xa6');
  RunBasic(data, MixedKey, std::string(MixedKey.size()*15+3, '\x90'));
}
TEST_F(Test_crypt, InvalidLengthKey) {
  std::string data(100, '\x00');
  ASSERT_THROW(RunBasic(data, InvalidLengthKey, ""), khSimpleException);
}
TEST_F(Test_crypt, ShortKey) {
  std::string ShortKey(MixedKey, 0, 8);
  std::string data(ShortKey.size() * 2.5, '\xa6');
  RunBasic(data, ShortKey, std::string(ShortKey.size() * 2.5, '\x90'));
}


int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

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
// Unit tests for khStringUtils.

#include <string>
#include <gtest/gtest.h>
#include "common/khStringUtils.h"

class khStringUtilsTest : public testing::Test {
 protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST_F(khStringUtilsTest, IdenticalArrayChar) {
  char char_array[17] = {-7, -7, -7, -7, -7, -7, -7, -7,
                         -7, -7, -7, -7, -7, -7, -7, -7, -7};
  EXPECT_TRUE(IsArrayOfIdenticalElements(char_array, 17));
  for (int i = 0; i < 17; ++i) {
    EXPECT_EQ(-7, char_array[i]);
  }
}

TEST_F(khStringUtilsTest, IdenticalArrayInt) {
  int int_array[13] = {655366, 655366, 655366, 655366, 655366, 655366,
                       655366, 655366, 655366, 655366, 655366, 655366, 655366};
  EXPECT_TRUE(IsArrayOfIdenticalElements(int_array, 13));
  for (int i = 0; i < 13; ++i) {
    EXPECT_EQ(655366, int_array[i]);
  }
}

TEST_F(khStringUtilsTest, NonIdenticalArrayCharLast) {
  char char_array[17] = {-7, -7, -7, -7, -7, -7, -7, -7,
                         -7, -7, -7, -7, -7, -7, -7, -7, -3};
  EXPECT_FALSE(IsArrayOfIdenticalElements(char_array, 17));
  for (int i = 0; i < 17 - 1; ++i) {
    EXPECT_EQ(-7, char_array[i]);
  }
  EXPECT_EQ(-3, char_array[17-1]);
}

TEST_F(khStringUtilsTest, NonIdenticalArrayCharFirst) {
  char char_array[17] = {-3, -7, -7, -7, -7, -7, -7, -7,
                         -7, -7, -7, -7, -7, -7, -7, -7, -7};
  EXPECT_FALSE(IsArrayOfIdenticalElements(char_array, 17));
  EXPECT_EQ(-3, char_array[0]);
  for (int i = 1; i < 17; ++i) {
    EXPECT_EQ(-7, char_array[i]);
  }
}

TEST_F(khStringUtilsTest, NonIdenticalArrayIntMiddle) {
  int int_array[17] = {-7, -7, -7, -7, -7, -7, -7, -7,
                       -7, -7, -7, -7, -7, -7, -7, -7, -7};
  int_array[8] = -3;
  EXPECT_FALSE(IsArrayOfIdenticalElements(int_array, 17));
  for (int i = 0; i < 17; ++i) {
    if ( i == 8 ) {
      EXPECT_EQ(-3, int_array[i]);
    } else {
      EXPECT_EQ(-7, int_array[i]);
    }
  }
}

TEST_F(khStringUtilsTest,  TestParseDec32Value) {
  std::string x = "123";
  EXPECT_EQ(123, ParseDec32Value(x, 0U, 3U, -100));
  EXPECT_EQ(23, ParseDec32Value(x, 1U, 3U, -100));
  EXPECT_EQ(23, ParseDec32Value(x, 1U, 2U, -100));
  EXPECT_EQ(2, ParseDec32Value(x, 1U, 1U, -100));
  x = "x";
  EXPECT_EQ(-100, ParseDec32Value(x, 1U, 1U, -100));
  x = "-1";
  EXPECT_EQ(-1, ParseDec32Value(x, 0U, 2U, -100));
  EXPECT_EQ(-100, ParseDec32Value(x, 0U, 1U, -100));
  x = "1x";
  EXPECT_EQ(1, ParseDec32Value(x, 0U, 2U, -100));
}

TEST_F(khStringUtilsTest, TestUrlSplitter) {
  { // Basic test
    std::string url = "http://www.test.com/whyme/no?";
    std::string protocol;
    std::string host;
    std::string path;
    EXPECT_TRUE(UrlSplitter(url, &protocol, &host, &path));
    EXPECT_EQ(protocol, "http");
    EXPECT_EQ(host, "www.test.com");
    EXPECT_EQ(path, "whyme/no?");

    // Add several trailing slashes...shouldn't change result.
    url += "////";
    EXPECT_TRUE(UrlSplitter(url, &protocol, &host, &path));
    EXPECT_EQ(protocol, "http");
    EXPECT_EQ(host, "www.test.com");
    EXPECT_EQ(path, "whyme/no?");
  }
  { // No path
    std::string url = "https://www.test.com";
    std::string protocol;
    std::string host;
    std::string path;
    EXPECT_TRUE(UrlSplitter(url, &protocol, &host, &path));
    EXPECT_EQ(protocol, "https");
    EXPECT_EQ(host, "www.test.com");
    EXPECT_EQ(path, "");
    // Add several trailing slashes...shouldn't change result.
    url += "////";
    EXPECT_TRUE(UrlSplitter(url, &protocol, &host, &path));
    EXPECT_EQ(protocol, "https");
    EXPECT_EQ(host, "www.test.com");
    EXPECT_EQ(path, "");
  }
  { // Malformed urls
    std::string url = "https:/www.test.com/";
    std::string protocol;
    std::string host;
    std::string path;
    EXPECT_FALSE(UrlSplitter(url, &protocol, &host, &path));
    url = "www.test.com/";
    EXPECT_FALSE(UrlSplitter(url, &protocol, &host, &path));
    url = "htp://www.test.com/";
    EXPECT_FALSE(UrlSplitter(url, &protocol, &host, &path));
    url = "htp://";
    EXPECT_FALSE(UrlSplitter(url, &protocol, &host, &path));
  }
}

static
void AssertRelaxedUrlSplitterProduces(
    const std::string& url,
    bool expected_success,
    const std::string& expected_protocol,
    const std::string& expected_host,
    const std::string& expected_path_query) {
  std::string protocol;
  std::string host;
  std::string path_query;
  bool success = RelaxedUrlSplitter(url, &protocol, &host, &path_query);
  ASSERT_EQ(success, expected_success);
  ASSERT_EQ(protocol, expected_protocol);
  ASSERT_EQ(host, expected_host);
  ASSERT_EQ(path_query, expected_path_query);
}

TEST_F(khStringUtilsTest, TestRelaxedUrlSplitter) {
  // basic
  AssertRelaxedUrlSplitterProduces(
      "http://www.test.com/whyme/no?", true,
      "http", "www.test.com", "whyme/no?");

  AssertRelaxedUrlSplitterProduces(
      "www.test.com/whyme/no?", true,
      "", "www.test.com", "whyme/no?");

  // Add several trailing slashes...shouldn't change result.
  AssertRelaxedUrlSplitterProduces(
      "www.test.com/whyme/no?////", true,
      "", "www.test.com", "whyme/no?");

  // no path
  AssertRelaxedUrlSplitterProduces(
      "https://www.test.com", true,
      "https", "www.test.com", "");
  AssertRelaxedUrlSplitterProduces(
      "www.test.com", true,
      "", "www.test.com", "");
  // don't know how this is 'no path' but...
  AssertRelaxedUrlSplitterProduces(
      "www.test.com/////", true,
      "", "www.test.com", "");

  AssertRelaxedUrlSplitterProduces(
      "www.test.com/?summat=5", true,
      "", "www.test.com", "?summat=5");

  // query instead of path.
  AssertRelaxedUrlSplitterProduces(
      "www.test.com?summat=5", true,
      "", "www.test.com", "summat=5");

  // unusual
  AssertRelaxedUrlSplitterProduces(
      "goto/something", true,
      "", "goto", "something");

  { // Malformed urls
    std::string protocol;
    std::string host;
    std::string path;
    EXPECT_FALSE(RelaxedUrlSplitter("htp://www.test.com/",
                                    &protocol, &host, &path));
    const char* detectably_bad_urls[] = {
      // bad protocols
      "htp://", "htp://www.test.com",
      // empty hostname
      "http://", "",
      // Without a '://' the splitter fails, it is difficult to
      // distinguish a failed protocol from some other part of a url.
      // "http:/", "http:://", "http:", "http/", "http//:", "http://:", "http",
      NULL
    };
    for (const char** bad = detectably_bad_urls; *bad; ++bad) {
      EXPECT_FALSE(RelaxedUrlSplitter(*bad, &protocol, &host, &path));
    }
  }
}

TEST_F(khStringUtilsTest, TestReplaceString) {
  { // Test replace with bigger.
    std::string input = "12341234123412";
    std::string replace = "12";
    std::string replace_with = "abcc";
    std::string result = ReplaceString(input, replace, replace_with);
    std::string expected_result = "abcc34abcc34abcc34abcc";
    EXPECT_EQ(expected_result, result);
  }
  { // Test replace with smaller.
    std::string input = "12341234123412";
    std::string replace = "1234";
    std::string replace_with = "ab";
    std::string result = ReplaceString(input, replace, replace_with);
    std::string expected_result = "ababab12";
    EXPECT_EQ(expected_result, result);
  }
  { // Test replace with nothing.
    std::string input = "12341234123412";
    std::string replace = "123";
    std::string replace_with = "";
    std::string result = ReplaceString(input, replace, replace_with);
    std::string expected_result = "44412";
    EXPECT_EQ(expected_result, result);
  }
  { // Test replace with no match.
    std::string input = "12341234123412";
    std::string replace = "abc";
    std::string replace_with = "bbb";
    std::string result = ReplaceString(input, replace, replace_with);
    std::string expected_result = input;
    EXPECT_EQ(expected_result, result);
  }
}

TEST_F(khStringUtilsTest, TestReplaceSuffix) {
  { // Suffix exists.
    std::string input = "12341234123412";
    std::string suffix_out = "12";
    std::string suffix_in = "abcc";
    std::string result = ReplaceSuffix(input, suffix_out, suffix_in);
    std::string expected_result = "123412341234abcc";
    EXPECT_EQ(expected_result, result);
  }
  { // Suffix out is empty exists.
    std::string input = "12341234123412";
    std::string suffix_out = "";
    std::string suffix_in = "abcc";
    std::string result = ReplaceSuffix(input, suffix_out, suffix_in);
    std::string expected_result = "12341234123412abcc";
    EXPECT_EQ(expected_result, result);
  }
  { // Suffix doesn't exist.
    std::string input = "12341234123412";
    std::string suffix_out = "1234";
    std::string suffix_in = "ab";
    std::string result = ReplaceSuffix(input, suffix_out, suffix_in);
    std::string expected_result = input + suffix_in;
    EXPECT_EQ(expected_result, result);
  }
}

TEST_F(khStringUtilsTest, TestParseUTCTime) {
  // Valid strings.
  EXPECT_EQ(ParseUTCTime("1970-01-01T03:12:36.329Z"), true);
  EXPECT_EQ(ParseUTCTime("1970-01-01T03:12:36Z"), true);
  EXPECT_EQ(ParseDate("1970-01-01"), true);
  EXPECT_EQ(ParseDate("0001:01:01"), true);

  // Invalid strings.
  EXPECT_EQ(ParseUTCTime("1970-01-01T03:12:36.329+03.00"), false);
  EXPECT_EQ(ParseUTCTime("1970-01-01T03:12:36.329-03.00"), false);
  EXPECT_EQ(ParseUTCTime("1970-01-01T03:12:36.329"), false);
  EXPECT_EQ(ParseUTCTime("1970-01-01T03:12:36"), false);

  // Itemized parse results.
  std::string input = "1970-02-01T03:12:36Z";
  struct tm ts;
  EXPECT_EQ(true, ParseUTCTime(input));
  EXPECT_EQ(true, ParseUTCTime(input, &ts));
  EXPECT_EQ(true, ParseUTCTime(input, &ts));

  EXPECT_EQ(1970, ts.tm_year + 1900);   // tm_year starts from 1900
  EXPECT_EQ(2, ts.tm_mon + 1);          // tm_mon starts from 0
  EXPECT_EQ(1, ts.tm_mday);
  EXPECT_EQ(3, ts.tm_hour);
  EXPECT_EQ(12, ts.tm_min);
  EXPECT_EQ(36, ts.tm_sec);

  input = "0001-01-01";
  ParseUTCTime(input, &ts);
  EXPECT_EQ(-1899, ts.tm_year);     // tm_year starts from 1900
  EXPECT_EQ(0, ts.tm_mon);          // tm_mon starts from 0
  EXPECT_EQ(1, ts.tm_mday);
  EXPECT_EQ(0, ts.tm_hour);
  EXPECT_EQ("0001-01-01T00:00:00Z", GetUTCTimeString(ts));

  input = "0001:01:01";
  ParseUTCTime(input, &ts);
  EXPECT_EQ(-1899, ts.tm_year);     // tm_year starts from 1900
  EXPECT_EQ(0, ts.tm_mon);          // tm_mon starts from 0
  EXPECT_EQ(1, ts.tm_mday);
  EXPECT_EQ(0, ts.tm_hour);
  EXPECT_EQ("0001-01-01T00:00:00Z", GetUTCTimeString(ts));
}

TEST_F(khStringUtilsTest, GetUTCTimeString) {
  std::string input = "1970-02-01T03:12:36Z";
  struct tm ts;
  EXPECT_EQ(true, ParseUTCTime(input, &ts));
  EXPECT_EQ(input, GetUTCTimeString(ts));
}

TEST_F(khStringUtilsTest, TimeToHexString) {
  std::string input = "1000-10-13T16:13:02Z";
  std::string date_time;
  std::string date;
  std::string time;
  EXPECT_EQ(true, TimeToHexString(input, &date_time, &date, &time));

  EXPECT_EQ("7d14d.37ad6b0", date_time);
  EXPECT_EQ("7d14d", date);
  EXPECT_EQ("37ad6b0", time);
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

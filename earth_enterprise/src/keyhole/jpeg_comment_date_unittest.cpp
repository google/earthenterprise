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


#include <string>
#include <gtest/gtest.h>
#include "keyhole/jpeg_comment_date.h"

using keyhole::JpegCommentDate;

namespace {

struct TestDateRecord {
  std::string in_protocol_format;
  int year, month, day;
  // in_protocol_format == Encode(Decode(in_protocol_format))?
  bool is_reversible;
  std::string in_hex_format;
  bool uses_dashes;
};
static const TestDateRecord kTestDates[] = {
  { "2006:12:27", 2006, 12, 27, true, "fad9b"},
  { "2006-12-27", 2006, 12, 27, true, "fad9b", true},
  { "4095:12:31", 4095, 12, 31, true, "1fff9f"},
  { "1900:01:01", 1900, 1, 1, true, },
  { "1900:00:00", 1900, 0, 0, true, },
  { "0000:07:04", 0, 7, 4, false, },
  { "4096:00:00", 4096, 0, 0, false, },
  { "0000:12:31", 0, 12, 31, false, },
  { "2006:13:01", 2006, 13, 1, false, },
  { "2006:01:32", 2006, 1, 32, false, },
  { "2006:00:32", 2006, 0, 32, false, },
  { "2006:12:-2", 2006, 12, -2, false, },
  { "2006:12:xx", 2006, 12, -1, false, },
  { "2006:12;27", -1, -1, -1, false, },
  { "2006:12:2", -1, -1, -1, false, },
};

class JpegCommentDateTest : public testing::Test { };

TEST_F(JpegCommentDateTest, DateDecodingAndEncoding) {
  JpegCommentDate default_date;
  EXPECT_EQ(JpegCommentDate::kYearUnknown, default_date.year());
  EXPECT_EQ(JpegCommentDate::kMonthUnknown, default_date.month());
  EXPECT_EQ(JpegCommentDate::kDayUnknown, default_date.day());

  // Decode: string => JpegCommentDate => YearMonthDayKey => year,month,day.
  for (unsigned int i = 0; i < arraysize(kTestDates); ++i) {
    const JpegCommentDate date(kTestDates[i].in_protocol_format);
    if (kTestDates[i].is_reversible) {
      EXPECT_EQ(kTestDates[i].year, date.year());
      EXPECT_EQ(kTestDates[i].month, date.month());
      EXPECT_EQ(kTestDates[i].day, date.day());
    } else {
      EXPECT_EQ(JpegCommentDate::kYearUnknown, date.year());
      EXPECT_EQ(JpegCommentDate::kMonthUnknown, date.month());
      EXPECT_EQ(JpegCommentDate::kDayUnknown, date.day());
    }
    const JpegCommentDate::YearMonthDayKey key(date.AsYearMonthDayKey());
    int year, month, day;
    JpegCommentDate::YearMonthDayKeyAsInts(key, &year, &month, &day);
    EXPECT_EQ(date.year(), year);
    EXPECT_EQ(date.month(), month);
    EXPECT_EQ(date.day(), day);
    if (!kTestDates[i].in_hex_format.empty())
      EXPECT_EQ(date.GetHexString(), kTestDates[i].in_hex_format);
  }

  // Encode: year,month,day => YearMonthDayKey => JpegCommentDate => string.
  for (unsigned int i = 0; i < arraysize(kTestDates); ++i) {
    JpegCommentDate::YearMonthDayKey key;
    const bool is_date_valid(
        kTestDates[i].is_reversible || JpegCommentDate::AreYearMonthDayValid(
            kTestDates[i].year, kTestDates[i].month, kTestDates[i].day));
    EXPECT_EQ(is_date_valid, JpegCommentDate::YearMonthDayKeyFromInts(
        kTestDates[i].year, kTestDates[i].month, kTestDates[i].day, &key));
    int year, month, day;
    JpegCommentDate::YearMonthDayKeyAsInts(key, &year, &month, &day);
    if (kTestDates[i].is_reversible) {
      EXPECT_EQ(kTestDates[i].year, year);
      EXPECT_EQ(kTestDates[i].month, month);
      EXPECT_EQ(kTestDates[i].day, day);
    } else {
      EXPECT_EQ(JpegCommentDate::kYearUnknown, year);
      EXPECT_EQ(JpegCommentDate::kMonthUnknown, month);
      EXPECT_EQ(JpegCommentDate::kDayUnknown, day);
    }
    const JpegCommentDate date(key);
    EXPECT_EQ(year, date.year());
    EXPECT_EQ(month, date.month());
    EXPECT_EQ(day, date.day());
    if (kTestDates[i].is_reversible  && !kTestDates[i].uses_dashes) {
      std::string date_in_protocol_format;
      date.AppendToString(&date_in_protocol_format);
      EXPECT_EQ(kTestDates[i].in_protocol_format, date_in_protocol_format);
    }
  }
}
// Utility for creating a date (and verifying it was valid).
JpegCommentDate CreateValidDate(int year, int month, int day) {
  JpegCommentDate::YearMonthDayKey date_int;
  EXPECT_TRUE(JpegCommentDate::YearMonthDayKeyFromInts(year, month, day,
                                                       &date_int));
  return JpegCommentDate(date_int);
}

TEST_F(JpegCommentDateTest, DateCompare) {
  int year = 2000;
  int month = 10;
  int day = 15;
  { // Same dates.
    JpegCommentDate date1 = CreateValidDate(year, month, day);
    JpegCommentDate date2 = CreateValidDate(year, month, day);
    EXPECT_FALSE(date1 < date2);
    EXPECT_FALSE(date2 < date1);
  }

  { // Differ only by year
    JpegCommentDate date1 = CreateValidDate(year, month, day);
    JpegCommentDate date2 = CreateValidDate(year+1, month, day);
    EXPECT_TRUE(date1 < date2);
    EXPECT_FALSE(date2 < date1);
  }

  { // Differ only by month
    JpegCommentDate date1 = CreateValidDate(year, month, day);
    JpegCommentDate date2 = CreateValidDate(year, month+1, day);
    EXPECT_TRUE(date1 < date2);
    EXPECT_FALSE(date2 < date1);
  }

  { // Differ only by day
    JpegCommentDate date1 = CreateValidDate(year, month, day);
    JpegCommentDate date2 = CreateValidDate(year, month, day+1);
    EXPECT_TRUE(date1 < date2);
    EXPECT_FALSE(date2 < date1);
  }

  { // Check Ancient date and Oldest date constants.
    JpegCommentDate date1 = CreateValidDate(1, 1, 1);
    EXPECT_TRUE(keyhole::kOldestJpegCommentDate == date1);
  }
}

}  // namespace

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

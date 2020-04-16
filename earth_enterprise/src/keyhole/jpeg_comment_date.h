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

// These classes extend the JPEG Comment Protocol (see jpeg_comments.h), by
// defining messages that are specifically used to embed acquisition dates of
// images assembled from multiple assets.

// Design Patterns: Iterator.
// ============================================================================
#ifndef KEYHOLE_COMMON_INTERNAL_JPEG_COMMENTS_DATES_H__
#define KEYHOLE_COMMON_INTERNAL_JPEG_COMMENTS_DATES_H__

#include <string>
#include "common/base/macros.h"
#include <cstdint>

namespace keyhole {

class JpegCommentDate;

#undef CHECK
#undef DCHECK
// CHECK dies with a fatal error if condition is not true.  It is *not*
// controlled by NDEBUG, so the check will be executed regardless of
// compilation mode.  Therefore, it is safe to do things like:
//    CHECK(fp->Write(x) == 4)
#define CHECK(condition)  \
      if (!(condition)) fprintf(stderr, "Check failed: " #condition " ");
#define DCHECK(a) assert(a)
// Common methods/globals used within Google3 code.

// ============================================================================
// This class defines how dates are represented in Jpeg header messages.
// Basically they follow the Exif conventions, except that: anything smaller
// than a day is not represented, and "00"s are used as unknowns.
// ============================================================================
class JpegCommentDate {
 public:
  typedef std::uint32_t YearMonthDayKey;  // year,month,day = leadingbits,4bits,5bits.

  static const int kMinimumYear;
  static const int kMaximumYear;
  static const int kYearUnknown;
  static const int kMonthUnknown;
  static const int kDayUnknown;
  // Oldest date for published imagery.
  static const YearMonthDayKey kAncientDate;

  // Default date is "completely unknown".
  JpegCommentDate() : year_(kYearUnknown),
                      month_(kMonthUnknown),
                      day_(kDayUnknown),
                      match_all_dates_(false) { }
  // Construct from YearMonthDayKey.
  explicit JpegCommentDate(YearMonthDayKey date) : match_all_dates_(false) {
    YearMonthDayKeyAsInts(date, &year_, &month_, &day_);
  }
  // Construct from Protocol format (Exif format with "Unknowns"). If parsing
  // fails, object will be initialized with "completely unknown" date.
  // Format expected is "YYYY-MM-DD" or "YYYY:MM:DD"
  explicit JpegCommentDate(const std::string &date);
  // Default destructor enabled.

  // Convert to YearMonthDayKey.
  YearMonthDayKey AsYearMonthDayKey() const {
    YearMonthDayKey date;
    YearMonthDayKeyFromInts(year_, month_, day_, &date);
    return date;
  }
  // Convert to Protocol format (Exif format with "Unknowns"). A "completely
  // unknown" date will be printed as 0000:00:00.
  void AppendToString(std::string *buffer) const;

  // Get a 5+ digit hexadecimal version of the jpeg date, e.g., "fc353"
  std::string GetHexString() const;

  // Accessors for date components:
  int year() const { return year_; }  // Absolute, not relative to anything.
  int month() const { return month_; }
  int day() const { return day_; }

  bool IsCompletelyUnknown() const {
    return kYearUnknown == year();
  }

  static bool YearMonthDayKeyFromInts(int year, int month, int day,
                                      YearMonthDayKey *date);
  static void YearMonthDayKeyAsInts(YearMonthDayKey date,
                                    int *year, int *month, int *day);

  // Do inputs correspond to a valid JpegCommentDate?
  static bool AreYearMonthDayValid(int year, int month, int day);
  // This method modifes month and day (if necessary) so as to enforce the fact
  // that kYearUnknown -> kMonthUnknown and so on.
  static void PropagateUnknowns(int year, int *month, int *day);

  // Return true if this date is equal to the other.
  bool operator==(const JpegCommentDate& other) const;

  // Return true if this date is less than other.
  bool operator<(const JpegCommentDate& other) const;

  // Flag this date as matching all dates.
  // Note: this setting has nothing to do with operator== or operator<.
  void SetMatchAllDates(bool value) { match_all_dates_ = value; }

  // Return true if this date object is specified to match all dates.
  bool MatchAllDates() const { return match_all_dates_; };

 private:
  int year_;
  int month_;
  int day_;
  bool match_all_dates_;
};  // class JpegCommentDate

// The uninitialized/unknown jpeg comment date.
extern const JpegCommentDate kUnknownJpegCommentDate;
// The oldest jpeg comment date.
extern const JpegCommentDate kOldestJpegCommentDate;

}  // namespace keyhole

#endif  // KEYHOLE_COMMON_INTERNAL_JPEG_COMMENTS_DATES_H__

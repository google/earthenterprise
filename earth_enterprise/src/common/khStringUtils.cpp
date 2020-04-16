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


#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <iomanip>
#include <limits>
#include <fstream>
#include <sstream>
#include "keyhole/jpeg_comment_date.h"
#include "common/khConstants.h"
#include "common/khMisc.h"
#include "common/khstl.h"
#include "common/khStringUtils.h"
#include "common/notify.h"

const char* const kISO_8601_DateTimeFormatString = "%Y-%m-%dT%H:%M:%SZ";

void TokenizeString(const std::string& str,
                    std::vector<std::string>& tokens,
                    const std::string& delimiters,
                    int max_tokens) {
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

  --max_tokens;
  while (std::string::npos != pos || std::string::npos != lastPos) {
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    if (--max_tokens == 0) {
      if (std::string::npos != pos)
        tokens.push_back(str.substr(pos+1, std::string::npos - pos));
      break;
    }
    lastPos = str.find_first_not_of(delimiters, pos);
    pos = str.find_first_of(delimiters, lastPos);
  }
}

bool UrlSplitter(const std::string& url,
                 std::string* protocol,
                 std::string* host,
                 std::string* path) {
  const std::string kProtocolDelimiter = "://";
  std::string::size_type pos = url.find(kProtocolDelimiter);
  if (pos == std::string::npos) {
    return false;  // Invalid delimiter.
  }
  std::string temp_protocol = url.substr(0, pos);
  if (temp_protocol != "http" && temp_protocol != "https") {
    return false;  // Invalid protocol.
  }
  std::string temp_host = url.substr(pos + kProtocolDelimiter.size());
  if (temp_host.empty()) {
    return false;  // No host name.
  }
  // Ok, we've got a "valid" url (not checking for invalid characters).
  *protocol = temp_protocol;
  // Need to split the path from the host...find the first "/".
  pos = temp_host.find("/");
  if (pos != std::string::npos) {
    // Pull the path part from the temp_host string.
    *path = temp_host.substr(pos + 1);
    // Strip trailing "/" characters.
    while ((!path->empty()) && (*path)[path->size()-1] == '/') {
      path->resize(path->size() - 1);
    }
    // Strip the path from the host result.
    *host = temp_host.substr(0, pos);
  } else {
    *host = temp_host;
  }
  return true;
}

bool RelaxedUrlSplitter(const std::string& url,
                        std::string* protocol,
                        std::string* host,
                        std::string* path_and_or_query) {
  const std::string kProtocolDelimiter = "://";
  std::string::size_type pos = url.find(kProtocolDelimiter);
  std::string::size_type pos_hostname;
  bool have_protocol = pos != std::string::npos;
  std::string temp_protocol;
  if (!have_protocol) {
    pos = 0;
    pos_hostname = 0;
  } else {
    temp_protocol = url.substr(0, pos);
    if (temp_protocol != "http" && temp_protocol != "https") {
      return false;  // Invalid protocol.
    }
    pos_hostname = pos + kProtocolDelimiter.size();
  }

  std::string temp_host = url.substr(pos_hostname);
  if (temp_host.empty()) {
    return false;  // No host name.
  }
  // Ok, we've got a "valid" url (not checking for invalid characters).
  *protocol = temp_protocol;
  // Need to split the path from the host...find the first '/'
  // (or '?' if it comes first)
  pos = temp_host.find_first_of("/?");
  if (pos != std::string::npos) {
    // Pull the path part from the temp_host string.
    *path_and_or_query = temp_host.substr(pos + 1);
    // Strip trailing "/" characters.
    while ((!path_and_or_query->empty()) &&
           (*path_and_or_query)[path_and_or_query->size() - 1] == '/') {
      path_and_or_query->resize(path_and_or_query->size() - 1);
    }
    // Strip the path from the host result.
    *host = temp_host.substr(0, pos);
  } else {
    *host = temp_host;
  }
  return true;
}

std::string ComposeUrl(bool use_ssl_flag,
                       const std::string& host,
                       const std::string& port,
                       const std::string& path) {
  std::string url;
  if (use_ssl_flag) {
    url = "https://";
  } else {
    url = "http://";
  }

  url += host;

  if (!port.empty()) {
    url += ":" + port;
  }

  if (!path.empty()) {
    url += "/" + path;
  }

  return url;
}

void CleanString(std::string* str, const std::string& remove_chars) {
  std::string::size_type pos = str->find_first_of(remove_chars, 0);
  while (std::string::npos != pos) {
    str->erase(pos, 1);
    pos = str->find_first_of(remove_chars, pos);
  }
}

std::string ReplaceString(const std::string& input,
                          const std::string& replace_string,
                          const std::string& replace_with_string) {
  // Search for the first occurrence of replace_string in input.
  std::string::size_type pos_prev = 0;
  std::string::size_type pos = input.find(replace_string, pos_prev);
  if (pos == std::string::npos) {
    return input;  // No occurrences, return the input string.
  }
  std::string result;
  unsigned int replace_string_length = replace_string.size();
  while (std::string::npos != pos) {
    // Copy the characters from the previous search to the beginning
    // of the replace_string occurrence.
    result += input.substr(pos_prev, pos - pos_prev);
    // Copy in the replace_with_string.
    result += replace_with_string;
    // Move the cursor past the previous instance of replace_string.
    pos_prev = pos + replace_string_length;
    // Find the next occurrence.
    pos = input.find(replace_string, pos_prev);
  }
  result += input.substr(pos_prev);
  return result;
}

void ReplaceCharsInString(std::string& str,
                          const std::string& rep,
                          const std::string& rep_with) {
  std::string::size_type rep_len = rep_with.size();
  std::string::size_type pos = str.find_first_of(rep, 0);
  while (std::string::npos != pos) {
    str.replace(pos, 1, rep_with, 0, rep_len);
    pos = str.find_first_of(rep, pos+rep_len);
  }
}

std::string ReplaceSuffix(const std::string& input_string,
                          const std::string& suffix_remove,
                          const std::string& suffix_add) {
  int expected_start_of_suffix = static_cast<int>(input_string.size()) -
    static_cast<int>(suffix_remove.size());
  if (expected_start_of_suffix < 0 ||
      (input_string.compare(expected_start_of_suffix, suffix_remove.size(),
                            suffix_remove) != 0)) {
    return input_string + suffix_add;
  }
  return input_string.substr(0, expected_start_of_suffix) + suffix_add;
}

template <typename T>
std::string Itoa(T x) {
  std::ostringstream stream;
  if (!(stream << x))
    return "";
  return stream.str();
}

// explicit instantiation
template std::string Itoa(std::int32_t x);
template std::string Itoa(std::uint32_t x);
template std::string Itoa(std::int64_t x);
template std::string Itoa(std::uint64_t x);

std::string DoubleToString(double x) {
  std::ostringstream stream;
  if (!(stream << x))
    return "";
  return stream.str();
}

// convert integer to base 62
// digits=10 + lowercase=26 + uppercase=26 = 62 chars
std::string IntToBase62(int value) {
  static const char StyleNameChars[] =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  static const int StyleNameLen = strlen(StyleNameChars);

  // find the number of output digits needed
  int sz = 1;
  for (int tmp = value; (tmp /= StyleNameLen) > 0; ++sz) { }

  // pre-size the output string
  std::string out_str;
  out_str.resize(sz);

  // put the base62 chars into the string (working from back to front)
  do {
    out_str[sz-1] = StyleNameChars[value % StyleNameLen];
    value /= StyleNameLen;
    --sz;
  } while (sz);

  return out_str;
}

// Convert time interval to "tm" structure.
bool time2tm(time_t timeval, struct tm *ts, const bool localtime = false) {
  if (!timeval)
    time(&timeval);

  return (timeval != (time_t)-1) &&
         (localtime ? (localtime_r(&timeval, ts) != 0) :
                      (gmtime_r(&timeval, ts) != 0));
}

// Gets UTC time string with specified format.
std::string GetUtcTimeStringWithFormat(
    time_t timeval, const std::string& format) {
  struct tm ts;
  char buf[256];

  if (time2tm(timeval, &ts) &&
      (strftime(buf, sizeof(buf), format.c_str(), &ts) > 0)) {
    return buf;
  } else {
    return std::string();
  }
}

bool TimeToHexString(const std::string& input,
                     std::string* date_time_hex,
                     std::string* date_hex,
                     std::string* time_hex) {
  // Assume input is in UTC format.
  struct tm time = {0};
  if (!ParseUTCTime(input, &time)) {
    return false;
  }

  // Build date hex code
  keyhole::JpegCommentDate::YearMonthDayKey jpeg_date_key;
  keyhole::JpegCommentDate::YearMonthDayKeyFromInts(
    time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, &jpeg_date_key);
  keyhole::JpegCommentDate jpeg_date(jpeg_date_key);
  std::string date_hex_ = jpeg_date.GetHexString();

  // Build time hex code
  std::int32_t milliSeconds = MillisecondsFromMidnight(time);
  char buf[8];
  snprintf(buf, sizeof(buf), "%x", milliSeconds);
  std::string time_hex_ = std::string(buf);

  if (date_time_hex) {
    *date_time_hex = date_hex_ + '.' + time_hex_;
  }
  if (date_hex) {
    *date_hex = date_hex_;
  }
  if (time_hex) {
    *time_hex = time_hex_;
  }
  return true;
}

// Gets local time string with specified format.
std::string GetTimeStringWithFormat(
      time_t timeval, const std::string& format) {
  struct tm ts;
  char buf[256];

  if (!timeval)
    time(&timeval);

  if (time2tm(timeval, &ts, true) &&
      (strftime(buf, sizeof(buf), format.c_str(), &ts) > 0)) {
    return buf;
  } else {
    return std::string();
  }
}

std::string GetCurrentTimeStringWithFormat(const std::string& format) {
  time_t now = time(0);
  return GetTimeStringWithFormat(now, format);
}

std::string GetUTCTimeString(const struct tm& time) {
  std::string timestr;
  const std::string format = "%04Y-%m-%dT%H:%M:%S";
  char buf[256];
  if (strftime(buf, sizeof(buf), format.c_str(), &time) > 0) {
    timestr = timestr + buf + "Z";
  }
  return timestr;
}

bool ParseUTCTime(const std::string& input, struct tm *time) {
  const std::string format = "%Y-%m-%dT%H:%M:%S%Z";
  struct tm time_ = {0};
  try {
    char* next = strptime(input.data(), format.c_str(), &time_);
    if (next) {
      // Parse as ISO 8601 time string
      if (input[input.length()-1] != 'Z') {
        // Only supports UTC time such as "1970-01-01T03:12:36Z",
        // Do not support string with time zone for now, e.g.
        // "1970-01-01T03:12:36+03.00".
        notify(NFY_WARN, "Not a standard ISO 8601 UTC string: %s\n",
               input.c_str());
        return false;
      }
    } else {
      // Parse as date only.
      if (!ParseDate(input, &time_)) {
        notify(NFY_WARN, "Can't parse date/time: %s\n", input.c_str());
        return false;
      }
    }
  } catch (...) {
    notify(NFY_WARN, "Error parsing: %s\n", input.c_str());
    return false;
  }
  if (time) {
    *time = time_;
  }

  return true;
}

bool ParseDate(const std::string& input, struct tm *time) {
  struct tm time_ = {0};
  if (strptime(input.c_str(), "%Y-%m-%e", &time_) != NULL ||
      strptime(input.c_str(), "%Y:%m:%e", &time_) != NULL) {
    if (time) {
      *time  = time_;
    }
    return true;
  } else {
    return false;
  }
}

std::string FindValueInVariableFile(const std::string& filename,
                                    const std::string& key) throw() {
  try {
    std::ifstream in(filename.c_str());
    if (in) {
      std::string line;
      while (std::getline(in, line)) {
        if (StartsWith(line, key)) {
          unsigned int position = line.find_first_of('=');
          if (position < 0) {
            continue;  // skip, not a valid entry.
          }
          std::string result = line.substr(position+1);
          return TrimWhite(result);
        }
      }
    }
  } catch (...) {
    // ignore, return empty string
  }
  return std::string("");
}


std::int32_t ParseDec32Value(const std::string& string_value, std::uint32_t start_index,
                             size_t size, std::int32_t default_result) {
  std::string substring = string_value.substr(start_index, size);
  if (substring.empty()) {
  }
  char *error = NULL;
  std::int64_t value = strtoll(substring.c_str(), &error, 10);
  // Limit long values to std::int32_t min/max.  Needed for lp64; no-op on 32 bits.
  if (value > std::numeric_limits<std::int32_t>::max()) {
    value = std::numeric_limits<std::int32_t>::max();
  } else if (value < std::numeric_limits<std::int32_t>::min()) {
    value = std::numeric_limits<std::int32_t>::min();
  }
  // Check if there was no number to convert.
  return (error == substring.c_str()) ? default_result : value;
}

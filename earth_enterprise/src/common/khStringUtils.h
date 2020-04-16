/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_KHSTRINGUTILS_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_KHSTRINGUTILS_H_

#include <assert.h>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "common/khTypes.h"
#include <cstdint>

extern const char* const kISO_8601_DateTimeFormatString;


// Tokenizes the given string and appends the tokens to the given vector.
// If max_tokens is specified and is less than the number of potential tokens
// then the last token will contain the remaining portion of the string
// (including the delimiters).
void TokenizeString(const std::string& str,
                    std::vector<std::string>& tokens,
                    const std::string& delimiters,
                    int max_tokens = -1);

// Split a url into 3 components corresponding to:
// the protocol, the server name and the path within the server.
// url: the url string (e.g., http://www.domain.com/test/this/)
// protocol: on output, the protocol string (either "http" or "https")
// host: on output, the host name (e.g., www.mycompany.com")
// path: the path within the url (if url is http://x.com/this/path/,
//       returns "this/path" without the trailing slash.
// Returns false if the url is malformed (missing http or https, "://" or a
// host name).
bool UrlSplitter(const std::string& url,
                 std::string* protocol,
                 std::string* host,
                 std::string* path);

// Like UrlSplitter, but is more tolerant. The idea is, if a url will
// work when sent to a server, we'll parse it. Its error checking is
// correspondingly much weaker, though.
// Specific differences are, it does not require, though of course will accept:
//   * a leading protocol (https:// or http://), eg "www.google.com" is fine.
//   * '/' separating the host from the query string,
//     eg, http://khtestserver.google.com?db=sky is ok.
// Returns success of parsing.
//   A caveat - its well-formedness checking is weak, it may wrongly
// return success, and additionally in that case its behavior is undefined.
bool RelaxedUrlSplitter(const std::string& url,
                        std::string* protocol,
                        std::string* host,
                        std::string* path_and_or_query);

// Build url from given parameters.
// use_ssl_flag: True if using ssl ("https" instead of "http").
// host: Server name (e.g., www.mycompany.com").
// port: Server port (e.g. 8080). If empty, assumes port 80.
// Returns composed url.
std::string ComposeUrl(bool use_ssl_flag,
                       const std::string& host,
                       const std::string& port,
                       const std::string& path);

// Removes occurences of given chars from the string.
void CleanString(std::string* str, const std::string& remove_chars);

// Replaces ALL occurrences of one string with another in an input string.
// input: the input string
// replace_string: the string to be replaced
// replace_with_string: the string which will replace the replace_string
// returns: the input string contents after replacements
std::string ReplaceString(const std::string& input,
                          const std::string& replace_string,
                          const std::string& replace_with_string);


// Replaces occurences of 'rep' chars with 'rep_with' chars
void ReplaceCharsInString(std::string& str,
                          const std::string& rep,
                          const std::string& rep_with);
// Return a copy of the input string with the specified "suffix_remove" replaced
// by the specified "suffix_add"
// if "suffix_remove" does not occur at the end of input_string, then
// we return input_string with suffix_add appended.
std::string ReplaceSuffix(const std::string& input_string,
                          const std::string& suffix_remove,
                          const std::string& suffix_add);

template< typename T >
std::string NumberToString(T number) {
  std::ostringstream stream;
  return (stream << number) ? stream.str() : std::string("");
}

template< typename T >
void StringToNumber(const std::string& str, T* ptr_to_num) {
  *ptr_to_num = 0;
  std::istringstream stream(str);
  stream >> *ptr_to_num;
}

template <typename T>
std::string Itoa(T x);

std::string DoubleToString(double x);

std::string IntToBase62(int val);

// Gets UTC time string with specified format.
// Note: if timeval equals 0, function gets current time.
std::string GetUtcTimeStringWithFormat(time_t timeval,
                                       const std::string& format);

// Convert time to UTC time string such as "1970-02-01T03:12:36Z"
std::string GetUTCTimeString(const struct tm& time);

// Convert time to Hex string as "DateHex.TimeHex".
bool TimeToHexString(const std::string& input,
                     std::string* date_time_hex = NULL,
                     std::string* date_hex = NULL,
                     std::string* time_hex = NULL);

// Gets local time string with specified format.
// Note: if timeval equals 0, function gets current time.
std::string GetTimeStringWithFormat(time_t timeval, const std::string& format);

std::string GetCurrentTimeStringWithFormat(const std::string& format);

// Parse UTC ISO 8601 time string into tm (year, month, day, hour, min, sec).
bool ParseUTCTime(const std::string& time, struct tm *ts = NULL);

// Test and parse date as "YYYY:MM:DD" or "YYYY-MM-DD".
bool ParseDate(const std::string& time, struct tm *ts = NULL);

// Will pull the string value associated with a key from a
// key=value file.
// will return an empty string if anything goes wrong.
// Whitespace is trimmed from the result.
// e.g.,
// key=  value
// will return "value"
//  key=value
// will return "" because the leading whitespace will not allow the key
// to be matched (i.e. no spaces between beginning of line and key are allowed).
std::string FindValueInVariableFile(const std::string& filename,
                                    const std::string& key) throw();

// Take the substring of string_value defined by the start_index and size and
// convert the value to an integer.
// If it can't be converted, return the default_result
std::int32_t ParseDec32Value(const std::string& value, std::uint32_t start_index,
                             size_t size, std::int32_t default_result);

// Returns whether all in the array ptr have same value. Assumes that T has a
// '==', '=' and ~ operator (which gives different but reversible data).
// It iterates first element to last.
// Note: Assumes length is greater than 1.
template< typename T>
bool IsArrayOfIdenticalElements(const T *ptr, size_t length) {
  assert(length > 1);
  T * const sentinel = const_cast<T*>(ptr + (length - 1));
  const T first_data = *ptr;
  if (first_data != *sentinel) {
    return false;
  }
  // change sentinel to be the stop point and decrease end point comparison
  *sentinel = ~first_data;
  for ( ; *(++ptr) == first_data ; ) {
  }
  *sentinel = first_data;
  return (ptr == sentinel);
}


// Packs a set of strings say (s1, s2 s3) by str(s1.length()) + "\n" + s1 +
// str(s2.length()) + "\n" + s2 + str(s3.length()) + "\n" + s3.
// Note that while unpacking we will read length followed by \n, and then read
// length long string. So even if the string has \n in it, unpacking will be OK.
class PackedString : public std::string {
 public:
  void Append(const std::string& str) {
    append(NumberToString(str.length()));
    append("\n");
    append(str);
  }

  // This does deep copying. Safe to modify the PackedString after this call.
  void Unpack(std::string* str_ary) const {
    for (size_t start = 0; start < length(); ++str_ary) {
      size_t last = find_first_of('\n', start);
      size_t size = 0;
      StringToNumber(substr(start, (last - start)), &size);
      start = last + 1;     // Actual string is after \n
      *str_ary = (size == 0) ? "" : substr(start, size);
      start += size;
    }
  }

  // This does deep copying. Safe to modify the PackedString after this call.
  void Unpack(std::vector<std::string*>* str_ary) const {
    std::vector<std::string*>::iterator iter = str_ary->begin();
    for (size_t start = 0; start < length(); ++iter) {
      size_t last = find_first_of('\n', start);
      size_t size = 0;
      StringToNumber(substr(start, (last - start)), &size);
      start = last + 1;     // Actual string is after \n
      (**iter) = (size == 0) ? "" : substr(start, size);
      start += size;
    }
  }

  // This does shallow copying for output char* pointers. Need to take care not
  // to delete/modify the PackedString till the output pointers are out of use.
  // If unpacking vails, the entire string array is cleared.
  // Return whether unpacked successfully.
  bool Unpack(std::vector< std::pair< const char*, size_t > >* str_ary) const {
    try {
      std::vector< std::pair< const char*, size_t > >::iterator iter =
          str_ary->begin();
      size_t len = length();
      for (size_t start = 0; start < len; ++iter) {
        size_t last = find_first_of('\n', start);
        // If we ran out, at least one was missing. Since we don't know which
        // one, clear them all.
        if (last == len) {
          str_ary->clear();
          return false;
        }
        size_t size = 0;
        StringToNumber(substr(start, (last - start)), &size);
        start = last + 1;     // Actual string is after \n
        (*iter) = std::make_pair(&at(start), size);
        start += size;
      }
     } catch(const std::exception &e) {
      str_ary->clear();
      std::cout << "Exception in Unpack:" << e.what() << std::endl;
      return false;
     }

    return true;
  }
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_KHSTRINGUTILS_H_

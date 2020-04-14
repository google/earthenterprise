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

//
// simple string conversions that DO NOT US QSTRING!
// #include khstrconv.h if you need QString conversions too

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_KHSIMPLE_STRCONV_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_KHSIMPLE_STRCONV_H_

#include <sstream>
#include <cctype>
#include <algorithm>
#include "common/khTypes.h"
#include <cstdint>
#include "common/SharedString.h"

template <class T>
std::string
ToString(const T &value)
{
  std::ostringstream out;
  // Converting binary double precision to decimal digits requires 17 digits
  // to recover the double precision number.
  // For extensive proof, see pages 236-237 of this doc:
  //   http://www.physics.ohio-state.edu/~dws/grouplinks/floating_point_math.pdf
  // Using less than 17 will introduce rounding errors when converting doubles
  // to strings in XML files.  For example, the header.xml file of an image pyramid.
  // If the geographic coordinates are precisely aligned with our quad boundaries,
  // this loss of precision will produce incorrect tile extents.
  out.precision(17);
  out << value;
  return out.str();
}

inline
std::string
ToString(const std::string &str)
{
  return str;
}

inline
std::string
ToString(khTypes::StorageEnum s)
{
  // emit the enum as a string - this gives greater readiblity
  // and higher tolerance to changes in the enum
  return khTypes::StorageName(s);
}

inline
std::string
ToString(unsigned char c)
{
  std::ostringstream out;
  out << (int)c;
  return out.str();
}



template <class T>
void
FromString(const std::string &str, T &val)
{
  std::istringstream in(str);
  in >> val;
}

inline
void
FromString(const std::string &str, SharedString &val)
{
  val = str;
}

inline
void
FromString(const std::string &str, std::string &val)
{
  val = str;
}

inline
void
FromString(const std::string &str, khTypes::StorageEnum &s)
{
  s = khTypes::StorageNameToEnum(str.c_str());
}

inline
void
FromString(const std::string &str, unsigned char &c)
{
  std::istringstream in(str);
  int val;
  in >> val;
  c = val;
}

inline
std::string
UpperCaseString(const std::string &str)
{
  std::string ret = str;
  std::transform(ret.begin(), ret.end(), ret.begin(), toupper);
  return ret;
}

inline
std::string
LowerCaseString(const std::string &str)
{
  std::string ret = str;
  std::transform(ret.begin(), ret.end(), ret.begin(), tolower);
  return ret;
}

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_KHSIMPLE_STRCONV_H_

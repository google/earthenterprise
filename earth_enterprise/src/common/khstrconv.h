/*
 * Copyright 2017 Google Inc.
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



#ifndef __khstrconv_h
#define __khstrconv_h

#include <qstring.h>

// Bring in the bulk of the conversions from the "simple" variant of myself
#include "common/khsimple_strconv.h"

inline
void
FromString(const std::string &str, QString &val)
{
  val = str.c_str();
}

template <class T>
QString
ToQString(const T &value)
{
  return QString::fromUtf8(ToString(value).c_str());
}

inline
QString
ToQString(const QString &str)
{
  return str;
}


// Parse a string into a float.  Returns true on success, false on error.
// Strict: allows no extra characters.
bool FromStringStrict(const std::string str, float *f);


template <class T>
void
FromQString(const QString &str, T &val)
{
  FromString((const char *)str.utf8(), val);
}

inline
void
FromQString(const QString &str, QString &val)
{
  val = str;
}

template <class Iter>
QString
qstring_join(Iter begin, Iter end, const QString &sep)
{
  QString result;
  for (Iter i = begin; i != end; ++i) {
    if (i != begin) {
      result += sep;
    }
    result += ToQString(*i);
  }
  return result;
}




#endif /* __khstrconv_h */

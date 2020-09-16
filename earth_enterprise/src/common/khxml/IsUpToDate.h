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


#ifndef COMMON_KHXML_ISUPTODATE_H__
#define COMMON_KHXML_ISUPTODATE_H__

#include <string>
#include <vector>
#include <list>
#include <set>
#include <cstdint>
#include <qstring.h>
#include <qcolor.h>
#include <Defaultable.h>
#include <khMetaData.h>
#include <khExtents.h>
#include <geRange.h>
#include <khInsetCoverage.h>
#include <khTileAddr.h>

#include "common/SharedString.h"

#define DEFINE_ISUPTODATE(T)                     \
inline bool IsUpToDate(const T &a, const T &b) { \
  return a == b;                                 \
}

#define DEFINE_TEMPLATED_ISUPTODATE(T)                  \
template <class U>                                      \
inline bool IsUpToDate(const T<U> &a, const T<U> &b) {  \
  return a == b;                                        \
}

#define DEFINE_CONTAINER_ISUPTODATE(Cont)                       \
template <class T>                                              \
inline bool IsUpToDate(const Cont<T> &a, const Cont<T> &b) {    \
  if (a.size() != b.size()) {                                   \
    return false;                                               \
  }                                                             \
  typename Cont<T>::const_iterator ai = a.begin();              \
  typename Cont<T>::const_iterator bi = b.begin();              \
  for (; ai != a.end(); ++ai, ++bi) {                           \
    if (!IsUpToDate(*ai, *bi)) {                                \
      return false;                                             \
    }                                                           \
  }                                                             \
  return true;                                                  \
}


DEFINE_ISUPTODATE(std::string);
DEFINE_ISUPTODATE(SharedString);
DEFINE_ISUPTODATE(signed             char);
DEFINE_ISUPTODATE(unsigned           char);
DEFINE_ISUPTODATE(signed   short     int);
DEFINE_ISUPTODATE(unsigned short     int);
DEFINE_ISUPTODATE(signed             int);
DEFINE_ISUPTODATE(unsigned           int);
DEFINE_ISUPTODATE(signed   long      int);
DEFINE_ISUPTODATE(unsigned long      int);
DEFINE_ISUPTODATE(signed   long long int);
DEFINE_ISUPTODATE(unsigned long long int);
DEFINE_ISUPTODATE(float32);
DEFINE_ISUPTODATE(float64);
DEFINE_ISUPTODATE(bool);
DEFINE_ISUPTODATE(khLevelCoverage);
DEFINE_ISUPTODATE(khInsetCoverage);
DEFINE_ISUPTODATE(khMetaData);
DEFINE_ISUPTODATE(QColor);

DEFINE_TEMPLATED_ISUPTODATE(khSize);
DEFINE_TEMPLATED_ISUPTODATE(khOffset);
DEFINE_TEMPLATED_ISUPTODATE(khExtents);
DEFINE_TEMPLATED_ISUPTODATE(geRange);
DEFINE_TEMPLATED_ISUPTODATE(Defaultable);


DEFINE_CONTAINER_ISUPTODATE(std::vector);
DEFINE_CONTAINER_ISUPTODATE(std::list);
DEFINE_CONTAINER_ISUPTODATE(std::set);

template <class K, class V>
inline bool IsUpToDate(const std::map<K,V> &a, const std::map<K,V> &b) {
  if (a.size() != b.size()) {
    return false;
  }
  typename std::map<K,V>::const_iterator ai = a.begin();
  for (; ai != a.end(); ++ai) {
    typename std::map<K,V>::const_iterator bi = b.find(ai->first);
    if (bi == b.end()) {
      return false;
    }
    if (!IsUpToDate(ai->second, bi->second)) {
      return false;
    }
  }
  return true;
}

inline bool IsUpToDate(const QString &a, const QString &b) {
  if (a.isEmpty() && b.isEmpty()) {
    return true;
  }
  return a == b;
}



#endif // COMMON_KHXML_ISUPTODATE_H__

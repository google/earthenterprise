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


#ifndef __khMetaData_h
#define __khMetaData_h

#include <map>
#include "khstrconv.h"
#include "CacheSizeCalculations.h"

class khMetaData
{
 public:
  typedef std::map<QString, QString> MapType;
  typedef std::map<QString, QString>::const_iterator MapTypeConstIterator;
  MapType map;

  khMetaData(void) { }
  khMetaData(const MapType &m) : map(m) { }

  template <class T>
  void SetValue(const QString &key, const T& val) {
    map.erase(key); // last one takes priority
    map.insert(std::make_pair(key, ToQString(val)));
  }

  void Erase(const QString &key) {
    map.erase(key);
  }

  // determine amount of memory used by a khMetaData object and the map it holds
  uint64 GetSize() {
    uint64 total = 0;
    for (const auto &i : map) {
      total += GetObjectSize(i.first) + GetObjectSize(i.second);
    }
    return sizeof(*this) + total;
  }

  bool operator==(const khMetaData &o) const { return map == o.map; }

  QString GetValue(const QString &key,
                   const QString &dflt = QString()) const
  {
    MapTypeConstIterator found = map.find(key);
    if (found != map.end()) {
      return found->second;
    } else {
      return dflt;
    }
  }

  template <class RetType>
  RetType GetAs(const QString &key,
                const RetType &dflt = RetType()) const
  {
    RetType result = dflt;
    MapTypeConstIterator found = map.find(key);
    if (found != map.end()) {
      FromQString(found->second, result);
    }
    return result;
  }
};


#endif /* __khMetaData_h */

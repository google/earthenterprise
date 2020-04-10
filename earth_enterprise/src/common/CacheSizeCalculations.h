/*
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

#ifndef _CACHE_SIZE_CALCULATIONS_H
#define _CACHE_SIZE_CALCULATIONS_H
#include <cstdint>
#include <memory>
#include <vector>
#include <set>
#include <map>
#include <string>
#include "notify.h"
#include <qstring.h>
#include "SharedString.h"

template<class T>
inline std::uint64_t GetSize(const T &obj);

//determine amount of memory used by an object pointed to by a pointer
template<class T>
inline std::uint64_t GetHeapUsage(const T* &ptr) {
    std::uint64_t ptrSize = (ptr ? GetSize(*ptr) : 0);
    return ptrSize;
}
//determine amount of memory used by an object pointed to by a shared_ptr
template<class T>
inline std::uint64_t GetHeapUsage(const std::shared_ptr<T> &shptr) {
    std::uint64_t shptrSize = (shptr ? GetSize(*shptr) : 0);
    return shptrSize;
}
//determine amount of memory used by a string
inline std::uint64_t GetHeapUsage(const std::string &str) {
    std::uint64_t strSize = (str.capacity() * sizeof(char));
    return strSize;
}
// determine amount of memory used by a qstring
inline std::uint64_t GetHeapUsage(const QString &qstr) {
    std::uint64_t qstrSize = (qstr.capacity() * sizeof(char16_t));
    return qstrSize;
}
// determine amount of memory used by a given object
// returns 0 for all non specified objects
template<class T>
inline std::uint64_t GetHeapUsage(const T &obj) {
    return 0;
}
//determine amount of memory used by a map's contents
template<class key, class val>
inline std::uint64_t GetHeapUsage(const std::map<key, val> &map) {
    std::uint64_t total = 0;
    for (const auto &kv : map) {
        total += GetHeapUsage(kv.first) + GetHeapUsage(kv.second);
    }
    return total;
}
// determine amount of memory used by a vector's contents
template<class T>
inline std::uint64_t GetHeapUsage(const std::vector<T> &vec) {
    std::uint64_t total = 0;
    for(const auto &t : vec) {
      total += GetHeapUsage(t);
    }
    return total;
}
//determine amount of memory used by a set's contents
template<class T>
inline std::uint64_t GetHeapUsage(const std::set<T> &set) {
    std::uint64_t total = 0;
    for (const auto &t : set) {
        total += GetHeapUsage(t);
    }
    return total;
}
//determine amount of memory used by a given object
template<class T>
inline std::uint64_t GetSize(const T &obj) {
    std::uint64_t objSize = GetHeapUsage(obj);
    return sizeof(obj) + objSize;
}
#endif

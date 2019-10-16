/*
 * Copyright 2019 The Open GEE Contributors
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
#include "khTypes.h"
#include <memory>
#include <vector>
#include <set>
#include <map>
#include <string>
#include "notify.h"
#include <qstring.h>
#include <typeinfo>
#include <type_traits>
#include "SharedString.h"

//determine amount of memory used by an object pointed to by a pointer
template<class T>
inline uint64 GetHeapUsage(const T* &ptr) {
    uint64 ptrSize = (ptr ? GetSize(*ptr) : 0);
    return ptrSize;
}
//determine amount of memory used by an object pointed to by a shared_ptr
template<class T>
inline uint64 GetHeapUsage(const std::shared_ptr<T> &shptr) {
    uint64 shptrSize = (shptr ? GetSize(*shptr) : 0);
    return shptrSize;
}
//determine amount of memory used by a string
inline uint64 GetHeapUsage(const std::string &str) {
    uint64 strSize = (str.capacity() * sizeof(char));
    return strSize;
}
// determine amount of memory used by a qstring
inline uint64 GetHeapUsage(const QString &qstr) {
    uint64 qstrSize = (qstr.capacity() * sizeof(char16_t));
    return qstrSize;
}
// determine amount of memory used by a given object
// returns 0 for all non specified objects
template<class T>
inline uint64 GetHeapUsage(const T &obj) {
    notify(NFY_WARN, "\tGENERAL of %s", typeid(obj).name());
    return 0;
}
// determine amount of memory used by a vector's contents
template<class T>
inline uint64 GetHeapUsage(const std::vector<T> &vec) {
    uint64 total = 0;
    for(const auto &t : vec) {
      total += GetHeapUsage(t);
    }
    return total;
}
//determine amount of memory used by a set's contents
template<class T>
inline uint64 GetHeapUsage(const std::set<T> &set) {
    uint64 total = 0;
    for (const auto &t : set) {
        total += GetHeapUsage(t);
    }
    return total;
}
//determine amount of memory used by a map's contents
template<class key, class val>
inline uint64 GetHeapUsage(const std::map<key, val> &map) {
    uint64 total = 0;
    for (const auto &kv : map) {
        total += GetHeapUsage(kv.first) + GetHeapUsage(kv.second);
    }
    return total;
}
// determine amount of memory used by a given object
template<class T>
inline uint64 GetSize(const T &obj) {
    notify(NFY_WARN, "GetSize of %s:", typeid(obj).name());
    uint64 objSize = GetHeapUsage(obj);
    notify(NFY_WARN, "sizeof: %lu\tGetHeapUsage: %lu", sizeof(obj), objSize);
    return sizeof(obj) + objSize;
}
#endif

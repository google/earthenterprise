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
#include "khRefCounter.h"
#include "khMetaData.h"

// determine amount of memory used by generic objects
template<class T>
inline uint64 GetObjectSize(T obj) {
    return sizeof(obj);
}
// determine amount of memory used by a vector and all of it's contents
template<class T>
inline uint64 GetObjectSize(std::vector<T> vec) {
    uint64 total = 0;
    for(const auto &t : vec) {
      total += GetObjectSize(t);
    }

    return sizeof(vec) + total;
}
// determine amount of memory used by a string
inline uint64 GetObjectSize(std::string str) {
    return sizeof(str) + (str.capacity() * sizeof(char));
}
// determine amount of memory used by qstring
inline uint64 GetObjectSize(QString qstr) {
    return sizeof(qstr) + (qstr.capacity() * sizeof(char16_t));
}
// determine amount of memory used by a khRefGuard and the object it points to
template<class T>
inline uint64 GetObjectSize(khRefGuard<T> guard) {
    return sizeof(guard) + guard.getPtrSize();
}
// determine amount of memory used by a khMetaData object and the map it holds
inline uint64 GetObjectSize(khMetaData meta) {
    uint64 total = 0;
    for (const auto &i : meta.map) {
      total += GetObjectSize(i.first) + GetObjectSize(i.second);
    }
    return sizeof(meta) + total;
}
#endif
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
#include "notify.h"

// determine amount of memory used by generic objects
template<class T>
inline uint64 GetObjectSize(const T &obj) {
    return sizeof(obj);
}
// determine amount of memory used by a vector and all of it's contents
template<class T>
inline uint64 GetObjectSize(const std::vector<T> &vec) {
    uint64 total = 0;
    for(const auto &t : vec) {
      total += GetObjectSize(t);
    }

    return sizeof(vec) + total;
}
// determine amount of memory used by a string
inline uint64 GetObjectSize(const std::string &str) {
    return sizeof(str) + (str.capacity() * sizeof(char));
}
// determine amount of memory used by a shared_ptr and the object it points to
template<class T>
inline uint64 GetObjectSize(const std::shared_ptr<T> &guard) {
    return sizeof(guard) + (guard ? guard->GetSize() : 0);
}
#endif

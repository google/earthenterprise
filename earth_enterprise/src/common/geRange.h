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

//

#ifndef COMMON_GERANGE_H__
#define COMMON_GERANGE_H__

template <class T>
class geRange {
 public:
  T min;
  T max;

  geRange(T min_, T max_) : min(min_), max(max_) { }

  inline bool operator==(const geRange &o) const {
    return ((min == o.min) && (max == o.max));
  }

  inline bool Contains(T t) const { return ((t >= min) && (t <= max)); }

  template <class X>
  geRange(const geRange<X> &o) :
      min(static_cast<T>(o.min)),
      max(static_cast<T>(o.max))
  { }
};

#endif // COMMON_GERANGE_H__

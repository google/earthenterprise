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

#ifndef __Defaultable_h
#define __Defaultable_h

template <class T>
class Defaultable {
  bool useDefault;
  T val;
 public:
  inline Defaultable(void) : useDefault(true), val(T()) { }
  inline Defaultable(const T &t) : useDefault(false), val(t) { }
  inline Defaultable(bool ud, const T &t) : useDefault(ud), val(t) { }
  inline bool UseDefault(void) const { return useDefault; }
  inline const T& GetValue(void) const { return val; }
  inline operator const T&(void) const { return val; }

  // Warning: this doesn't affect the useDefault flag
  // use this sparingly
  inline T& GetMutableValue(void) { return val; }

  template <class U>
  inline Defaultable& operator=(const U& newVal) {
    val = newVal;
    useDefault = false;
    return *this;
  };
  inline Defaultable& operator=(const Defaultable &other) {
    val = other.val;
    useDefault = other.useDefault;
    return *this;
  }
  inline void BindDefaults(const Defaultable &other) {
    if (useDefault) {
      val = other.val;
    }
  }
  inline void SetUseDefault(void) {
    val = T();
    useDefault = true;
  }

  // don't change the value - if you want to do that just use operator=
  inline void ClearUseDefault(void) {
    useDefault = false;
  }
  inline bool operator==(const Defaultable &other) const {
    return ((useDefault == other.useDefault) &&
            (useDefault || (val == other.val)));
  }
};


#endif /* __Defaultable_h */

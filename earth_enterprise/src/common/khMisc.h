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



#ifndef _khMisc_h_
#define _khMisc_h_

#include <math.h>
#include <time.h>
#include <khTypes.h>
#include <cstdint>

inline int log2(int val)
{
  int l2 = 0;
  int v = val;
  while (v >>= 1)
    l2++;
  if (pow(2, l2) != val)
    return 0;
  else
    return l2;
}

inline unsigned int log2(unsigned int val)
{
  unsigned int l2 = 0;
  unsigned int v = val;
  while (v >>= 1)
    l2++;
  if (pow(2, l2) != val)
    return 0;
  else
    return l2;
}

inline int log2up(int val)
{
  int l2 = 0;
  int v = val;
  while (v >>= 1)
    l2++;
  if (pow(2, l2) != val)
    return l2+1;
  else
    return l2;
}

template<class Type>
Type Clamp(Type val, Type min, Type max) {
  if (val < min)
    return min;
  else if (val > max)
    return max;
  else
    return val;
}

// ****************************************************************************
// ***  Templated versions for compile time constants
// ****************************************************************************
template <unsigned long long num>
struct CompileTimeIsPowerOf2Checker {
  enum { result = (num & 0x1) ? ((num>>1) == 0) :
         CompileTimeIsPowerOf2Checker<(num>>1)>::result };
};
template <>
struct CompileTimeIsPowerOf2Checker<0> {
  enum { result = false };
};

#define CompileTimeIsPowerOf2(x) CompileTimeIsPowerOf2Checker<x>::result




// will generate compile time errors if num is not a power of 2
template <unsigned long long num>
struct CompileTimeLog2Checker {
  // will generate compile time error for numbers that are not power of two
  static_assert(CompileTimeIsPowerOf2(num), "Not A Power Of 2");

  // Simplified algorithm since:
  // 1) The above static_assert guarantees that it's a power of two
  // 2) The specializations below handles the case for '1'
  enum { result = 1 + CompileTimeLog2Checker<(num>>1)>::result };
};
template <> struct CompileTimeLog2Checker<1> {
  enum { result = 0 };
};

// Count milliseconds from midnight.
inline std::int32_t MillisecondsFromMidnight(const struct tm& time) {
  return ((time.tm_hour * 60 +  time.tm_min) * 60 +  time.tm_sec) * 1000;
}

#define CompileTimeLog2(x) CompileTimeLog2Checker<x>::result


#endif // !_khMisc_h_

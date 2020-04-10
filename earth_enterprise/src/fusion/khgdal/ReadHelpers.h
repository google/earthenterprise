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

#ifndef __ReadHelpers_h
#define __ReadHelpers_h

#include <gdal.h>
#include <khCalcHelper.h>


// ****************************************************************************
// ***  PaletteAssigner
// ****************************************************************************
template <class T, unsigned int numcomp>
class PaletteAssigner { };
// specialized for numcomp == 1
template <class T>
class PaletteAssigner<T, 1>
{
 public:
  inline static void Assign(T *const dest[], std::uint32_t pos,
                            const GDALColorEntry &entry) {
    dest[0][pos] = ClampRange<T>(entry.c1);
  }
};
// specialized for numcomp == 3
template <class T>
class PaletteAssigner<T, 3>
{
 public:
  inline static void Assign(T *const dest[], std::uint32_t pos,
                            const GDALColorEntry &entry) {
    dest[0][pos] = ClampRange<T>(entry.c1);
    dest[1][pos] = ClampRange<T>(entry.c2);
    dest[2][pos] = ClampRange<T>(entry.c3);
  }
};


// ****************************************************************************
// ***  ZeroAssigner
// ****************************************************************************
template <class T, unsigned int numcomp>
class ZeroAssigner { };
// specialized for numcomp == 1
template <class T>
class ZeroAssigner<T, 1>
{
 public:
  inline static void Assign(T *const dest[], std::uint32_t pos) {
    dest[0][pos] = 0;
  }
};
// specialized for numcomp == 3
template <class T>
class ZeroAssigner<T, 3>
{
 public:
  inline static void Assign(T *const dest[], std::uint32_t pos) {
    dest[0][pos] = 0;
    dest[1][pos] = 0;
    dest[2][pos] = 0;
  }
};




#endif /* __ReadHelpers_h */

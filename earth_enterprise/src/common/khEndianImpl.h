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



#ifndef __khEndianImpl_h
#define __khEndianImpl_h

/*****************************************************************************
 **
 ** Implementations of internal routines. Don't use these directly. Use the API
 ** in khEndian.h
 **
 ** Note: These must be able to deal with misaligned src & dest
 **
 *****************************************************************************/
#include <algorithm>
#include <cstdint>

/*****************************************************************************
 **
 **   These are specialized sizes 8,4,2 & 1. Trying to use any other sizes
 ** will generate a compile time error.
 **
 *****************************************************************************/
template <int nBytes>
inline void bitcopyswap(void *dest, const void *src);
template <int nBytes>
inline void bitcopy(void *dest, const void *src);


template <>
inline void bitcopy<8>(void *dest, const void *src)
{
  *((std::uint8_t *)(dest)+0) = *((std::uint8_t *)(src)+0);
  *((std::uint8_t *)(dest)+1) = *((std::uint8_t *)(src)+1);
  *((std::uint8_t *)(dest)+2) = *((std::uint8_t *)(src)+2);
  *((std::uint8_t *)(dest)+3) = *((std::uint8_t *)(src)+3);
  *((std::uint8_t *)(dest)+4) = *((std::uint8_t *)(src)+4);
  *((std::uint8_t *)(dest)+5) = *((std::uint8_t *)(src)+5);
  *((std::uint8_t *)(dest)+6) = *((std::uint8_t *)(src)+6);
  *((std::uint8_t *)(dest)+7) = *((std::uint8_t *)(src)+7);
}

template <>
inline void bitcopy<4>(void *dest, const void *src)
{
  *((std::uint8_t *)(dest)+0) = *((std::uint8_t *)(src)+0);
  *((std::uint8_t *)(dest)+1) = *((std::uint8_t *)(src)+1);
  *((std::uint8_t *)(dest)+2) = *((std::uint8_t *)(src)+2);
  *((std::uint8_t *)(dest)+3) = *((std::uint8_t *)(src)+3);
}

template <>
inline void bitcopy<2>(void *dest, const void *src)
{
  *((std::uint8_t *)(dest)+0) = *((std::uint8_t *)(src)+0);
  *((std::uint8_t *)(dest)+1) = *((std::uint8_t *)(src)+1);
}

template <>
inline void bitcopy<1>(void *dest, const void *src)
{
  *(std::uint8_t*)dest = *(std::uint8_t*)src;
}


template <>
inline void bitcopyswap<8>(void *dest, const void *src)
{
  *((std::uint8_t *)(dest)+7) = *((std::uint8_t *)(src)+0);
  *((std::uint8_t *)(dest)+6) = *((std::uint8_t *)(src)+1);
  *((std::uint8_t *)(dest)+5) = *((std::uint8_t *)(src)+2);
  *((std::uint8_t *)(dest)+4) = *((std::uint8_t *)(src)+3);
  *((std::uint8_t *)(dest)+3) = *((std::uint8_t *)(src)+4);
  *((std::uint8_t *)(dest)+2) = *((std::uint8_t *)(src)+5);
  *((std::uint8_t *)(dest)+1) = *((std::uint8_t *)(src)+6);
  *((std::uint8_t *)(dest)+0) = *((std::uint8_t *)(src)+7);
}

template <>
inline void bitcopyswap<4>(void *dest, const void *src)
{
  *((std::uint8_t *)(dest)+3) = *((std::uint8_t *)(src)+0);
  *((std::uint8_t *)(dest)+2) = *((std::uint8_t *)(src)+1);
  *((std::uint8_t *)(dest)+1) = *((std::uint8_t *)(src)+2);
  *((std::uint8_t *)(dest)+0) = *((std::uint8_t *)(src)+3);
}

template <>
inline void bitcopyswap<2>(void *dest, const void *src)
{
  *((std::uint8_t *)(dest)+1) = *((std::uint8_t *)(src)+0);
  *((std::uint8_t *)(dest)+0) = *((std::uint8_t *)(src)+1);
}

template <>
inline void bitcopyswap<1>(void *dest, const void *src)
{
  *(std::uint8_t*)dest = *(std::uint8_t*)src;
}


// ****************************************************************************
// ***  Deprecated - swap-in-place routines
// ****************************************************************************
template <int nBytes>
inline void bitswap(void *srcdest);

template <>
inline void bitswap<8>(void *srcdest)
{
  // 0x1122334455667788 --> 0x8877665544332211
  std::swap(*((std::uint8_t *)(srcdest)+0), *((std::uint8_t *)(srcdest)+7));
  std::swap(*((std::uint8_t *)(srcdest)+1), *((std::uint8_t *)(srcdest)+6));
  std::swap(*((std::uint8_t *)(srcdest)+2), *((std::uint8_t *)(srcdest)+5));
  std::swap(*((std::uint8_t *)(srcdest)+3), *((std::uint8_t *)(srcdest)+4));
}

template <>
inline void bitswap<4>(void *srcdest)
{
  // 0x12 34 56 78 --> 0x78 56 34 12
  std::swap(*((std::uint8_t *)(srcdest)+0), *((std::uint8_t *)(srcdest)+3));
  std::swap(*((std::uint8_t *)(srcdest)+1), *((std::uint8_t *)(srcdest)+2));
}

template <>
inline void bitswap<2>(void *srcdest)
{
  std::swap(*((std::uint8_t *)(srcdest)+0), *((std::uint8_t *)(srcdest)+1));
}

template <>
inline void bitswap<1>(void *srcdest)
{
  // nothing to do
}

#endif /* __khEndianImpl_h */

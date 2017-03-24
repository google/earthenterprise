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
#include <khTypes.h>
#include <algorithm>

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
  *((uint8 *)(dest)+0) = *((uint8 *)(src)+0);
  *((uint8 *)(dest)+1) = *((uint8 *)(src)+1);
  *((uint8 *)(dest)+2) = *((uint8 *)(src)+2);
  *((uint8 *)(dest)+3) = *((uint8 *)(src)+3);
  *((uint8 *)(dest)+4) = *((uint8 *)(src)+4);
  *((uint8 *)(dest)+5) = *((uint8 *)(src)+5);
  *((uint8 *)(dest)+6) = *((uint8 *)(src)+6);
  *((uint8 *)(dest)+7) = *((uint8 *)(src)+7);
}

template <>
inline void bitcopy<4>(void *dest, const void *src)
{
  *((uint8 *)(dest)+0) = *((uint8 *)(src)+0);
  *((uint8 *)(dest)+1) = *((uint8 *)(src)+1);
  *((uint8 *)(dest)+2) = *((uint8 *)(src)+2);
  *((uint8 *)(dest)+3) = *((uint8 *)(src)+3);
}

template <>
inline void bitcopy<2>(void *dest, const void *src)
{
  *((uint8 *)(dest)+0) = *((uint8 *)(src)+0);
  *((uint8 *)(dest)+1) = *((uint8 *)(src)+1);
}

template <>
inline void bitcopy<1>(void *dest, const void *src)
{
  *(uint8*)dest = *(uint8*)src;
}


template <>
inline void bitcopyswap<8>(void *dest, const void *src)
{
  *((uint8 *)(dest)+7) = *((uint8 *)(src)+0);
  *((uint8 *)(dest)+6) = *((uint8 *)(src)+1);
  *((uint8 *)(dest)+5) = *((uint8 *)(src)+2);
  *((uint8 *)(dest)+4) = *((uint8 *)(src)+3);
  *((uint8 *)(dest)+3) = *((uint8 *)(src)+4);
  *((uint8 *)(dest)+2) = *((uint8 *)(src)+5);
  *((uint8 *)(dest)+1) = *((uint8 *)(src)+6);
  *((uint8 *)(dest)+0) = *((uint8 *)(src)+7);
}

template <>
inline void bitcopyswap<4>(void *dest, const void *src)
{
  *((uint8 *)(dest)+3) = *((uint8 *)(src)+0);
  *((uint8 *)(dest)+2) = *((uint8 *)(src)+1);
  *((uint8 *)(dest)+1) = *((uint8 *)(src)+2);
  *((uint8 *)(dest)+0) = *((uint8 *)(src)+3);
}

template <>
inline void bitcopyswap<2>(void *dest, const void *src)
{
  *((uint8 *)(dest)+1) = *((uint8 *)(src)+0);
  *((uint8 *)(dest)+0) = *((uint8 *)(src)+1);
}

template <>
inline void bitcopyswap<1>(void *dest, const void *src)
{
  *(uint8*)dest = *(uint8*)src;
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
  std::swap(*((uint8 *)(srcdest)+0), *((uint8 *)(srcdest)+7));
  std::swap(*((uint8 *)(srcdest)+1), *((uint8 *)(srcdest)+6));
  std::swap(*((uint8 *)(srcdest)+2), *((uint8 *)(srcdest)+5));
  std::swap(*((uint8 *)(srcdest)+3), *((uint8 *)(srcdest)+4));
}

template <>
inline void bitswap<4>(void *srcdest)
{
  // 0x12 34 56 78 --> 0x78 56 34 12
  std::swap(*((uint8 *)(srcdest)+0), *((uint8 *)(srcdest)+3));
  std::swap(*((uint8 *)(srcdest)+1), *((uint8 *)(srcdest)+2));
}

template <>
inline void bitswap<2>(void *srcdest)
{
  std::swap(*((uint8 *)(srcdest)+0), *((uint8 *)(srcdest)+1));
}

template <>
inline void bitswap<1>(void *srcdest)
{
  // nothing to do
}

#endif /* __khEndianImpl_h */

// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Basic integer type definitions for various platforms
//
// This code is compiled directly on many platforms, including client
// platforms like Windows, Mac, and embedded systems.  Before making
// any changes here, make sure that you're not breaking any platforms.

#ifndef BASE_INTEGRAL_TYPES_H_
#define BASE_INTEGRAL_TYPES_H_
#include <cstdint>
// These typedefs are also defined in base/google.swig. In the
// SWIG environment, we use those definitions and avoid duplicate
// definitions here with an ifdef. The definitions should be the
// same in both files, and ideally be only defined in this file.
#ifndef SWIG
// Standard typedefs
// All Google2 code is compiled with -funsigned-char to make "char"
// unsigned.  Google2 code therefore doesn't need a "uchar" type.
typedef signed char         schar;
typedef signed char         int8;

// NOTE: unsigned types are DANGEROUS in loops and other arithmetical
// places.  Use the signed types unless your variable represents a bit
// pattern (eg a hash value) or you really need the extra bit.  Do NOT
// use 'unsigned' to express "this value should always be positive";
// use assertions for this.

// A type to represent a Unicode code-point value. As of Unicode 4.0,
// such values require up to 21 bits.
// (For type-checking on pointers, make this explicitly signed,
// and it should always be the signed version of whatever int32 is.)
typedef signed int         char32;

//  A type to represent a natural machine word (for e.g. efficiently
// scanning through memory for checksums or index searching). Don't use
// this for storing normal integers. Ideally this would be just
// unsigned int, but our 64-bit architectures use the LP64 model
// (http://www.opengroup.org/public/tech/aspen/lp64_wp.htm), hence
// their ints are only 32 bits. We want to use the same fundamental
// type on all archs if possible to preserve *printf() compatability.
typedef unsigned long      uword_t;

#endif /* SWIG */

static const std::uint8_t  kuint8max  = (( std::uint8_t) 0xFF);
static const std::uint16_t kuint16max = ((std::uint16_t) 0xFFFF);
static const std::uint32_t kuint32max = ((std::uint32_t) 0xFFFFFFFF);
static const std::uint64_t kuint64max = ((std::uint64_t) UINT64_C(0xFFFFFFFFFFFFFFFF));
static const std::int8_t   kint8min   = ((  std::int8_t) ~0x7F);
static const std::int8_t   kint8max   = ((  std::int8_t) 0x7F);
static const std::int16_t  kint16min  = (( std::int16_t) ~0x7FFF);
static const std::int16_t  kint16max  = (( std::int16_t) 0x7FFF);
static const std::int32_t  kint32min  = (( std::int32_t) ~0x7FFFFFFF);
static const std::int32_t  kint32max  = (( std::int32_t) 0x7FFFFFFF);
static const std::int64_t  kint64min  = (( std::int64_t) UINT64_C(~0x7FFFFFFFFFFFFFFF));
static const std::int64_t  kint64max  = (( std::int64_t) UINT64_C(0x7FFFFFFFFFFFFFFF));

// TODO: remove this eventually.
// No object has kIllegalFprint as its Fingerprint.
typedef std::uint64_t Fprint;
static const Fprint kIllegalFprint = 0;
static const Fprint kMaxFprint = UINT64_C(0xFFFFFFFFFFFFFFFF);

#endif  // BASE_INTEGRAL_TYPES_H_

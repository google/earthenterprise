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


#ifndef BASE_MACROS_H_
#define BASE_MACROS_H_
#include <stddef.h>         // For size_t
#include "base/port.h"

// DISALLOW_COPY_AND_ASSIGN disallows the copy constructor and copy assignment
// operator. DISALLOW_IMPLICIT_CONSTRUCTORS is like DISALLOW_COPY_AND_ASSIGN,
// but also disallows the default constructor, intended to help make a
// class uninstantiable.
//
// Unless building with a pre-C++11 compiler, prefer the language-supported
// "= delete" syntax in the public: section of the class.
//
// These must be placed in the private: declarations for a class,
// as a technical requirement of the non-C++11 branch.
#if LANG_CXX11
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;      \
  TypeName& operator=(const TypeName&) = delete
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName() = delete;                           \
  DISALLOW_COPY_AND_ASSIGN(TypeName)
#else  // C++98 case follows
// Note that these C++98 implementations cannot completely disallow copying,
// as members and friends can still accidentally make elided copies without
// triggering a linker error.
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  TypeName& operator=(const TypeName&)
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName();                                    \
  DISALLOW_COPY_AND_ASSIGN(TypeName)
#endif  // LANG_CXX11

// The arraysize(arr) macro returns the # of elements in an array arr.
// The expression is a compile-time constant, and therefore can be
// used in defining new arrays, for example.  If you use arraysize on
// a pointer by mistake, you will get a compile-time error.
//
// This template function declaration is used in defining arraysize.
// Note that the function doesn't need an implementation, as we only
// use its type.
template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];

// The gcc wants both of these prototypes.
template <typename T, size_t N>
char (&ArraySizeHelper(const T (&array)[N]))[N];

#define arraysize(array) (sizeof(ArraySizeHelper(array)))

#endif  // BASE_MACROS_H_

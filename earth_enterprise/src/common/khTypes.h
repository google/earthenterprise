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


#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_KHTYPES_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_KHTYPES_H_

#include <assert.h>

#if defined(__sgi)
#include <sys/types.h>
typedef uint16_t                        uint16;
typedef int16_t                         int16;
typedef uint32_t                        uint32;
typedef int32_t                         int32;
typedef uint64_t                        uint64;
typedef int64_t                         int64;
#elif defined(__linux__)
#include <sys/types.h>
typedef u_int16_t                       uint16;
typedef int16_t                         int16;
typedef u_int32_t                       uint32;
typedef int32_t                         int32;
typedef u_int64_t                       uint64;
typedef int64_t                         int64;
#if     !defined(TRUE) || ((TRUE) != 1)
#undef TRUE
#define TRUE    (1)
#endif
#if     !defined(FALSE) || ((FALSE) != 0)
#undef FALSE
#define FALSE   (0)
#endif
#elif defined(_WIN32)
typedef unsigned short                  uint16;
typedef short                           int16;
typedef unsigned long                   uint32;
typedef long                            int32;
typedef unsigned __int64                uint64;
typedef __int64                         int64;
typedef unsigned short                  ushort;
#endif

typedef unsigned char                   uchar;
typedef unsigned char                   uint8;
typedef signed char                     int8;
typedef float                           float32;
typedef double                          float64;

#ifdef __cplusplus

// Class template to extract properties from a generic type.
// It is used to overload a function template based on a type condition.
template <int v>
struct Int2Type {
    enum {value = v};
};

// defined ptr_uint_t to the appropriate type for this platform
template <int size>
class PointerAsUIntImpl;

template <>
class PointerAsUIntImpl<4> {
 public:
  typedef uint32 Type;
};

template <>
class PointerAsUIntImpl<8> {
 public:
  typedef uint64 Type;
};

typedef PointerAsUIntImpl<sizeof(char*)>::Type ptr_uint_t;



template <class T>
class khSize {
 public:
  T width, height;

  khSize(void) : width(), height() { }
  khSize(T width_, T height_) : width(width_), height(height_) { }

  // Returns whether either width or height is less than or equal to 0.
  bool degenerate() {
    return (width <= 0 || height <= 0);
  }

  bool operator==(const khSize &o) const {
    return (width == o.width &&
            height == o.height);
  }
  bool operator!=(const khSize &o) const {
    return !operator==(o);
  }
};

enum CoordOrder {XYOrder, RowColOrder, NSEWOrder};

template <class T>
class khOffset {
  T x_, y_;

 public:
  inline T x(void) const { return x_; }
  inline T y(void) const { return y_; }
  inline T row(void) const { return y_; }
  inline T col(void) const { return x_; }

  inline khOffset(void) : x_(), y_() { }
  inline khOffset(CoordOrder o, T a, T b) {
    switch (o) {
      case XYOrder:
        x_ = a;    // x
        y_ = b;    // y
        break;
      case RowColOrder:
        y_ = a;    // row
        x_ = b;    // col
        break;
      case NSEWOrder:
        assert(0);
        break;
    }
  }

  inline bool operator==(const khOffset &o) const {
    return (x_ == o.x_ &&
            y_ == o.y_);
  }
  inline bool operator!=(const khOffset &o) const {
    return !operator==(o);
  }
};

namespace khTypes {
// DO NOT change the values in this enum, they are stored in various
// binary file formats. Note: these DO NOT map directly to GDALDataType!
// Use the routines in khgdal.h to map back and forth.
enum StorageEnum {
  UInt8   = 0,
  Int8    = 1,
  UInt16  = 2,
  Int16   = 3,
  UInt32  = 4,
  Int32   = 5,
  UInt64  = 6,
  Int64   = 7,
  Float32 = 8,
  Float64 = 9
};

extern const char *StorageName(StorageEnum s);
extern uint StorageSize(StorageEnum s);
extern StorageEnum StorageNameToEnum(const char *n);

// Helper - used to map type to enum
template <class T> class Helper { };
#define DefineHelper(type_, enum_)                    \
    template <>                                       \
    class Helper<type_> {                             \
     public:                                          \
      typedef type_ Type;                             \
      static const StorageEnum Storage = enum_;       \
    };
DefineHelper(uint8,   UInt8);
DefineHelper(int8,    Int8);
DefineHelper(uint16,  UInt16);
DefineHelper(int16,   Int16);
DefineHelper(uint32,  UInt32);
DefineHelper(int32,   Int32);
DefineHelper(uint64,  UInt64);
DefineHelper(int64,   Int64);
DefineHelper(float32, Float32);
DefineHelper(float64, Float64);
};  // namespace khTypes

#endif  //  #ifdef __cplusplus


#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_KHTYPES_H_

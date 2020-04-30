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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTTYPES_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTTYPES_H_

#include <stdarg.h>
#include <vector>
#include <string>
#include <cstdint>

#include "common/khTypes.h"
#include "common/notify.h"

#if     !defined(TRUE) || ((TRUE) != 1)
#undef TRUE
#define TRUE    (1)
#endif
#if     !defined(FALSE) || ((FALSE) != 0)
#undef FALSE
#define FALSE   (0)
#endif
#if     !defined(SUCCESS) || ((SUCCESS) != 0)
#undef SUCCESS
#define SUCCESS (0)
#endif
#if     !defined(FAILURE) || ((FAILURE) != -1)
#undef FAILURE
#define FAILURE (-1)
#endif

typedef int (*gstItemGetFunc)(void *data, int tag, ...);

typedef std::vector<int> SelectList;
typedef std::vector<int>::iterator SelectListIterator;

typedef std::uint32_t gstFormatType;
typedef std::uint32_t gstClassType;

struct gstFormatDesc {
  int tag;
  char *tagStr;
  std::uint32_t flags;
  char *label;
  char *verbose;
  void *spec;
};

struct gstFmtDescSet {
  char *label;
  gstFormatDesc *fmtDesc;
  int fmtCount;
};

enum gstTagFlags {
  gstTagUnknown    = 0x00,
  gstTagInt        = 0x01,
  gstTagUInt       = 0x02,
  gstTagInt64      = 0x03,
  gstTagUInt64     = 0x04,
  gstTagFloat      = 0x05,
  gstTagDouble     = 0x06,
  gstTagString     = 0x07,
  gstTagUnicode    = 0x08,
  gstTagBoolean    = 0x09,

  gstTagTypeMask   = 0x0f,

  gstTagImmutable  = 0x10,
  gstTagStateMask  = 0xf0,

  gstTagInvalid    = 0xff
};

struct gstEnumStringSpec {
  char *name;
  char *desc;
};

struct gstEnumIntSpec {
  int tag;
  char *desc;
};

// DO NOT RE-SORT!  THESE NUMBERS
// ARE USED IN THE KGB FILES.
enum gstPrimType {
  gstUnknown         = 0x00,
  gstPoint           = 0x01,
  gstPolyLine        = 0x02,
  gstStreet          = 0x03,
  gstPolygon         = 0x04,
  gstPolygon25D      = 0x05,
  gstPolygon3D       = 0x06,
  gstMultiPolygon    = 0x07,
  gstMultiPolygon25D = 0x08,
  gstMultiPolygon3D  = 0x09,
  // 2.5D extension types
  gstPoint25D        = 0x80 | gstPoint,
  gstPolyLine25D     = 0x80 | gstPolyLine,
  gstStreet25D       = 0x80 | gstStreet,
};

std::string PrettyPrimType(gstPrimType type);

gstPrimType ToFlatPrimType(gstPrimType type);

bool IsMultiGeodeType(gstPrimType type);

gstPrimType GetMultiGeodeTypeFromSingle(gstPrimType type);

gstPrimType GetSingleGeodeTypeFromMulti(gstPrimType type);

enum {
  GST_PREVIEW = 0x01 << 0,
  GST_VECTOR  = 0x01 << 1,
  GST_IMAGERY = 0x01 << 2,
  GST_TERRAIN = 0x01 << 3
};

#if 0
enum {
  GST_NODE = 0,
  GST_GROUP = 1,
  GST_GEODE = 2,
  GST_GEOSET = 3,
  GST_QUADNODE = 4,
  GST_LOD = 5
};
#endif

enum gstReadMode {
  GST_READONLY,
  GST_READWRITE,  // Note that the read-write mode is not supported!
  GST_WRITEONLY
};

// Error codes
enum gstStatus {
  GST_OKAY = 0,
  GST_WRITE_FAIL,
  GST_OPEN_FAIL,
  GST_READ_FAIL,
  GST_FILE_EXISTS,
  GST_UNINITIALIZED,
  GST_UNSUPPORTED,
  GST_PARSE_FAIL,
  GST_TRANSFORM_FAIL,
  GST_PERMISSION_DENIED,
  GST_UNKNOWN
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTTYPES_H_

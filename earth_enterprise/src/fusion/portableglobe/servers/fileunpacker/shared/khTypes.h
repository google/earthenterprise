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

/* TODO: High-level file comment. */

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_KHTYPES_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_KHTYPES_H_

#include <assert.h>
#include <sys/types.h>

#if     !defined(TRUE) || ((TRUE) != 1)
#undef TRUE
#define TRUE    (1)
#endif
#if     !defined(FALSE) || ((FALSE) != 0)
#undef FALSE
#define FALSE   (0)
#endif

typedef u_int16_t                       uint16;
typedef int16_t                         int16;
typedef u_int32_t                       uint32;
typedef int32_t                         int32;
typedef u_int64_t                       uint64;
typedef int64_t                         int64;

typedef unsigned char                   uchar;
typedef unsigned char                   uint8;
typedef signed char                     int8;
typedef float                           float32;
typedef double                          float64;

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_KHTYPES_H_

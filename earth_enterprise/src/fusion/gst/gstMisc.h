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

#ifndef KHSRC_FUSION_GST_GSTMISC_H__
#define KHSRC_FUSION_GST_GSTMISC_H__

#include <string.h>
#include <stdio.h>
#include <khMisc.h>

class geFilePool;
void gstInit(geFilePool* pool);
inline void gstInit() { gstInit(NULL); }
extern geFilePool* GlobalFilePool;

extern double GlobalGrid[];

inline char* strdupSafe(const char* s) {
  if (s == NULL || *s == 0)
    return NULL;

  size_t lens = strlen(s);
  char* n = new char[lens + 1];
  strncpy(n, s, lens + 1);
  return n;
}

inline char* strcpySafe(char* dst, const char* src) {
  if (src == NULL || *src == 0) {
    *dst = '\0';
  } else {
    strcpy(dst, src);
  }

  return dst;
}

#define strDup strdupSafe

//
// a safe strcmp
// (allows NULL values)
//
inline int strcmpSafe(const char* s1, const char* s2) {
  if (s1 == NULL && s2 == NULL)
    return 0;

  if (s1 == NULL)
    return -1;

  if (s2 == NULL)
    return 1;

  return strcmp(s1, s2);
}

//
// a safe strlen
// (allows NULL values)
//
inline int strlenSafe(const char* s) {
  if (s == NULL || *s == '\0')
    return 0;

  return strlen(s);
}

inline double gstRoundDown(double a, double grid) {
  return a - fmod(a, grid) - (a < 0 ? grid : 0);
}

inline double gstRoundUp(double a, double grid) {
  return a - fmod(a, grid) + (a > 0 ? grid : 0);
}

inline int gstRoundUp(int a, int grid) {
  return a - static_cast<int>(fmod((double)a, (double)grid)) + grid;
}

#endif  // !KHSRC_FUSION_GST_GSTMISC_H__

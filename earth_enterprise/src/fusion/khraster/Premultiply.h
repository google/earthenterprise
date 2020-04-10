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

#ifndef FUSION_KHRASTER_PREMULTIPLY_H__
#define FUSION_KHRASTER_PREMULTIPLY_H__

#include <cstdint>

template <unsigned int a, unsigned int r, unsigned int g, unsigned int b>
inline void UndoAlphaPremultiply(unsigned char *raw_buf, unsigned int numBytes) {
  for (unsigned int i = 0; i < numBytes; i+=4) {
    unsigned char * const pixel = &raw_buf[i];
    if (pixel[a] == 0) {
      pixel[r] = pixel[g] = pixel[b] = 0;
    } else {
      unsigned int s = (uint(255) << 16) / pixel[a];
      pixel[r] = (pixel[r] * s + (1<<15)) >> 16;
      pixel[g] = (pixel[g] * s + (1<<15)) >> 16;
      pixel[b] = (pixel[b] * s + (1<<15)) >> 16;
    }
  }
}

inline void UndoAlphaPremultiplyARGB(unsigned int *raw_buf, unsigned int numPixels) {
#if __BYTE_ORDER == __BIG_ENDIAN
  UndoAlphaPremultiply<0,1,2,3>((unsigned char*)raw_buf, numPixels*4);
#else
  UndoAlphaPremultiply<3,2,1,0>((unsigned char*)raw_buf, numPixels*4);
#endif
}


#endif // FUSION_KHRASTER_PREMULTIPLY_H__

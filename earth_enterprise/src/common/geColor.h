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


#ifndef KHSRC_COMMON_GECOLOR_H__
#define KHSRC_COMMON_GECOLOR_H__

class geColor {
 public:
  enum ColorOrder { RGBA, BGRA, AGBR };

  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;

  geColor() : r(0), g(0), b(0), a(0) {}

  geColor(ColorOrder order, unsigned char w, unsigned char x, unsigned char y, unsigned char z) {
    switch (order) {
      case RGBA:
        r = w; g = x; b = y; a = z;
        break;
      case BGRA:
        b = w; g = x; r = y; a = z;
        break;
      case AGBR:
        a = w; g = x; b = y; r = z;
        break;
    }
  }

  bool operator==(const geColor& that) const {
    return r == that.r && g == that.g && b == that.b && a == that.a;
  }
  bool operator!=(const geColor& that) const {
    return r != that.r || g != that.g || b != that.b || a != that.a;
  }
  bool operator<(const geColor& that) const {
    return PackedRGBA() < that.PackedRGBA();
  }


  bool IsEmpty() const { return r == 0 && g == 0 && b == 0 && a == 0; }
  bool IsTransparent() const { return a == 0; }

  std::uint32_t PackedRGBA() const {
    return (std::uint32_t)r << 24 |
           (std::uint32_t)g << 16 |
           (std::uint32_t)b << 8  |
           (std::uint32_t)a;
  }
};

#endif  // !KHSRC_COMMON_GECOLOR_H__

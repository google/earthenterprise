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


#ifndef __ProcessNeighbors_h
#define __ProcessNeighbors_h

#include <cstdint>

template <unsigned int tilewidth, unsigned int tileheight, class Handler>
void ProcessNeighbors(std::uint32_t pos, Handler &h)
{
  unsigned int row = pos / tilewidth;
  unsigned int col = pos % tilewidth;
  switch (row) {
    case 0:
      switch (col) {
        case 0:
          // lower left - min y on bottom
          h.handle(pos+1);
          h.handle(pos+tilewidth);
          h.handle(pos+tilewidth+1);
          break;
        case tilewidth-1:
          // lower right - min y on bottom
          h.handle(pos-1);
          h.handle(pos+tilewidth-1);
          h.handle(pos+tilewidth);
          break;
        default:
          // lower edge - min y on bottom
          h.handle(pos-1);
          h.handle(pos+1);
          h.handle(pos+tilewidth-1);
          h.handle(pos+tilewidth);
          h.handle(pos+tilewidth+1);
      }
      break;
    case tileheight-1:
      switch (col) {
        case 0:
          // upper left - min y on bottom
          h.handle(pos-tilewidth);
          h.handle(pos-tilewidth+1);
          h.handle(pos+1);
          break;
        case tilewidth-1:
          // upper right - min y on bottom
          h.handle(pos-tilewidth-1);
          h.handle(pos-tilewidth);
          h.handle(pos-1);
          break;
        default:
          // upper edge - min y on bottom
          h.handle(pos-tilewidth-1);
          h.handle(pos-tilewidth);
          h.handle(pos-tilewidth+1);
          h.handle(pos-1);
          h.handle(pos+1);
      }
      break;
    default:
      switch (col) {
        case 0:
          // left edge
          h.handle(pos-tilewidth);
          h.handle(pos-tilewidth+1);
          h.handle(pos+1);
          h.handle(pos+tilewidth);
          h.handle(pos+tilewidth+1);
          break;
        case tilewidth-1:
          // right edge
          h.handle(pos-tilewidth-1);
          h.handle(pos-tilewidth);
          h.handle(pos-1);
          h.handle(pos+tilewidth-1);
          h.handle(pos+tilewidth);
          break;
        default:
          // interior
          h.handle(pos-tilewidth-1);
          h.handle(pos-tilewidth);
          h.handle(pos-tilewidth+1);
          h.handle(pos-1);
          h.handle(pos+1);
          h.handle(pos+tilewidth-1);
          h.handle(pos+tilewidth);
          h.handle(pos+tilewidth+1);
      }
  }
}


#endif /* __ProcessNeighbors_h */

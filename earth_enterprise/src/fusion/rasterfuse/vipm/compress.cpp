// Copyright 2017 Google Inc.
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

#include <stdio.h>
#include <khEndian.h>
#include "compress.h"

#if 0
void dump(int index, float f) {
  unsigned long *blah = (unsigned long *) &f;
  if (f != 0.0f)
    printf("[%i] float %f (%i) -- 0x%lx (%i)\n", index, f, (int)sizeof(float),
           *blah, (int)sizeof(*blah));
}
#endif


//////////////////////////////////////////////////////////////////////////////
static int arrayCopy(etArray<unsigned char> &buffer, unsigned char *ptr,
                     int sz)
{
  if(!sz) return 0;

  int i;
  swapbysize(ptr, sz);
  for(i=0;i<sz;i++) buffer.add(ptr+i);
  swapbysize(ptr, sz);

  return sz;
}

static void arraySet(etArray<unsigned char> &buffer, unsigned char *ptr,
                     int sz, int idx)
{
  if(!sz) return;

  int i;
  swapbysize(ptr, sz);
  for(i=0;i<sz;i++) buffer[idx+i] = *(ptr+i);
  swapbysize(ptr, sz);
}

// So to compress the tile, I use that fact that the points are snapped
// to a predefined grid size.
void compress(int level, double ox, double oy, double dx, double dy,
              etArray<etVec3f> &pts, etArray<etFace3i> &fcs,
              etArray<unsigned char> &buffer,
              const size_t reserve_size)
{
  // Mandatory if we want to encode the triangle indices with 8 bits.
  // assert(pts.length < 256);

  // Mandatory if we want to encode the triangle indices with 16 bits.
  assert(pts.length < 65536);

  // Define the quad size and the tile size.
  double exportOriginX = ox;          // Tile origin
  double exportOriginY = oy;
  double exportStepX   = dx;          // Tile grid interval
  double exportStepY   = dy;
  int    exportNPoints = pts.length;
  int    exportNFaces  = fcs.length;
  int    exportQTLevel = level;

  // insert placeholder for size
  int size = 0;
  int size_idx = buffer.length;
  arrayCopy(buffer, (unsigned char*)&size, sizeof(int));

  // Writing the info to the buffer
  size += arrayCopy(buffer, (unsigned char*)&exportOriginX, sizeof(double));
  size += arrayCopy(buffer, (unsigned char*)&exportOriginY, sizeof(double));
  size += arrayCopy(buffer, (unsigned char*)&exportStepX,   sizeof(double));
  size += arrayCopy(buffer, (unsigned char*)&exportStepY,   sizeof(double));
  size += arrayCopy(buffer, (unsigned char*)&exportNPoints, sizeof(int)   );
  size += arrayCopy(buffer, (unsigned char*)&exportNFaces,  sizeof(int)   );
  size += arrayCopy(buffer, (unsigned char*)&exportQTLevel, sizeof(int)   );

  // Processing all the vertices and compiling the grid indices.
  // Note: I assume that the vertex array already has its coords
  //       converted into grid indices.
  // TODO: 1.) Compress the Z component into a short format.
  //       2.) Store the coordinates per component rather than per vertex.
  //           This will be more aggressively compressed by a LZW.
  {
    int i;
    for(i=0;i<pts.length;i++)
    {
      unsigned char exportX = (unsigned char)(pts[i][0]);
      unsigned char exportY = (unsigned char)(pts[i][1]);
      float exportZ = pts[i][2];
      size += arrayCopy(buffer, (unsigned char*)&exportX,
                        sizeof(unsigned char));
      size += arrayCopy(buffer, (unsigned char*)&exportY,
                        sizeof(unsigned char));
      // XXX - irix and linux of level 0 mismatch here
      //             dump(i, exportZ);
      size += arrayCopy(buffer, (unsigned char*)&exportZ, sizeof(float));
    }
  }

  // Processing all the triangles now.
  // Note: Quick & ugly solution now. Will be more efficient with tristrips.
  // TODO: bind to triangle count per tiles to a value and encode the indices
  //       with unsigned chars.
  {
    int i;
    for(i=0;i<fcs.length;i++)
    {
      unsigned short exportP1 = (unsigned short)(fcs[i][0]);
      unsigned short exportP2 = (unsigned short)(fcs[i][1]);
      unsigned short exportP3 = (unsigned short)(fcs[i][2]);
      size += arrayCopy(buffer, (unsigned char*)&exportP1,
                        sizeof(unsigned short));
      size += arrayCopy(buffer, (unsigned char*)&exportP2,
                        sizeof(unsigned short));
      size += arrayCopy(buffer, (unsigned char*)&exportP3,
                        sizeof(unsigned short));
    }
  }

  // fill in actual size
  arraySet(buffer, (unsigned char*)&size, sizeof(int), size_idx);

  // reserve enough buffer space on the end for our caller to do what it
  // wants to do (typically add a CRC)
  buffer.expand(buffer.length + reserve_size);
}


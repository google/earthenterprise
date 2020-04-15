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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <set>
#include <notify.h>
#include <khEndian.h>
#include <khTileAddr.h>
#include <exception>
#include <notify.h>
#include <packetfile/packetfilereader.h>


 std::uint32_t numMeshes[MaxTmeshLevel+1];


void usage(void) {
  notify(NFY_FATAL, "usage: dumpmesh [--points] [--brief] [--summary] <meshfile>\n");
}

bool beBrief = false;
bool showPoints = false;


struct MeshHeader {
  int size;
  double ox;
  double oy;
  double dx;
  double dy;
  int    numPoints;
  int    numFaces;
  int    level;

  void read(LittleEndianReadBuffer &buf) {
    buf >> size
        >> ox
        >> oy
        >> dx
        >> dy
        >> numPoints
        >> numFaces
        >> level;
  }
};


void HandleMesh(const QuadtreePath &qtpath, const std::string &rawbuf) {
  static unsigned int numRecs = 0;
  std::uint32_t level, row, col;
  qtpath.GetLevelRowCol(&level, &row, &col);
  printf("%05d) node:%u,%u,%u\n",
         numRecs++, level, col, row);
  if (!beBrief) {
    std::uint32_t len = rawbuf.size();
    std::uint32_t count = 0;
    LittleEndianReadBuffer buf(&rawbuf[0], len);
    typedef LittleEndianReadBuffer::size_type size_type;
    while (len) {
      MeshHeader meshHead;
      size_type headStart = buf.CurrPos();
      meshHead.read(buf);
      printf("      size:%d level:%d pts/faces:%d,%d orig:%.12f,%.12f dx/dy:%.12f,%.12f\n",
             meshHead.size,
             meshHead.level,
             meshHead.numPoints,
             meshHead.numFaces,
             meshHead.ox, meshHead.oy,
             meshHead.dx,meshHead.dy);
      size_type headEnd = buf.CurrPos();
      if (meshHead.level <= (int)MaxTmeshLevel) {
        ++numMeshes[meshHead.level];
      }
      if (showPoints) {
        std::set<unsigned short> usedPoints;
        for (int i = 0; i < meshHead.numPoints; ++i) {
          unsigned char x;
          unsigned char y;
          float z;
          buf >> x >> y >> z;
          printf("          pt: %d,%d,%.12f\n", x, y, z);
        }
        for (int i = 0; i < meshHead.numFaces; ++i) {
          unsigned short p1;
          unsigned short p2;


          unsigned short p3;
          buf >> p1 >> p2 >> p3;
          usedPoints.insert(p1);
          usedPoints.insert(p2);
          usedPoints.insert(p3);
          printf("        face: %d,%d,%d\n", p1, p2, p3);
        }
        for (unsigned short i = 0; i < (unsigned short)meshHead.numPoints;
             ++i) {
          std::set<unsigned short>::const_iterator found =
            usedPoints.find(i);
          if (found == usedPoints.end()) {
            fprintf(stderr, "lrcs %d,%d,%d,%d not using point %u\n",
                    level, row, col, count, i);
          }
        }
      } else {
        buf.Skip(meshHead.size + 4 - ((char*)headEnd - (char*)headStart));
      }

      // the mesh's size doesn't include the 4 bytes for the size itself
      len -= meshHead.size + 4;
      ++count;
    }
  }
}




int main(int argc, char *argv[]) {
  try {
    const char *meshfile = 0;
    bool showSummary = false;

    memset(numMeshes, 0, sizeof(numMeshes));


    int argi = 1;                 // skip program name
    while (argi < argc) {
      if (strncmp(argv[argi], "-", 1) != 0)
        break;
      if (strcmp(argv[argi], "--points") == 0) {
        argi++;
        showPoints = true;
      } else if (strcmp(argv[argi], "--summary") == 0) {
        argi++;
        showSummary = true;
      } else if (strcmp(argv[argi], "--brief") == 0) {
        argi++;
        beBrief = true;
      } else {
        notify(NFY_WARN, "Unreecognized commandline option: %s\n", argv[argi]);
        usage();
      }
    }
    if (argi < argc) {
      meshfile = argv[argi++];
    } else {
      usage();
    }


    geFilePool file_pool;
    PacketFileReader reader(file_pool, meshfile);
    std::string buf;
    QuadtreePath qtpath;
    while (reader.ReadNextCRC(&qtpath, buf) > 0) {
      HandleMesh(qtpath, buf);
    }

    if (showSummary) {
      unsigned int MinWithMeshes = MaxTmeshLevel+1;
      unsigned int MaxWithMeshes = 0;
      for (unsigned int i = StartTmeshLevel; i <= MaxTmeshLevel; ++i) {
        if (numMeshes[i]) {
          if (i < MinWithMeshes) MinWithMeshes = i;
          if (i > MaxWithMeshes) MaxWithMeshes = i;
        }
      }

      for (unsigned int i = MinWithMeshes; i <= MaxWithMeshes; ++i) {
        fprintf(stderr, "level %d: %u meshes\n", i, numMeshes[i]);
      }
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  }
  return 0;
}

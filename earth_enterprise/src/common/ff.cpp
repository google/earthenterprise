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
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdarg.h>

#include "ff.h"
#include <khEndian.h>

// for khstrerror
#include <notify.h>

static void
errexit(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  exit( EXIT_FAILURE );
}

void
FFRecHeader::BigEndianToHost(void)
{
  len_   = ::BigEndianToHost(len_);
  level_ = ::BigEndianToHost(level_);
  x_     = ::BigEndianToHost(x_);
  y_     = ::BigEndianToHost(y_);
  vers_  = ::BigEndianToHost(vers_);
  // don't need to do padding[] since 0's are 0's in any endian-ness
}


int ff_read(const char *ffpath, ffReadCallbackFunc callback, void *userData)
{
  // open the flat file
  int fffd = open(ffpath, O_RDONLY | O_LARGEFILE);
  if (fffd == -1)
    errexit("ff_read: could not open %s, %s\n", ffpath,
            khstrerror(errno).c_str());

  struct stat64 sb;
  fstat64(fffd, &sb);

  // use a 100MB window into the file for mapping
  const size_t windowSize = 100*1024*1024;
  const long pageSize = sysconf(_SC_PAGESIZE);
  const long pageOffMask = (pageSize - 1);
  const long pageMask = ~pageOffMask;

  off64_t ffoff = 0;          // current offset into flatfile

  std::uint64_t recnum = 0;
  std::uint64_t winnum = 0;

  void* mapAddr = 0;
  while (ffoff < sb.st_size) {

    assert((ffoff & 31) == 0);

#if __ia64__
    std::uint64_t ffpgoff = ffoff & pageMask;
    size_t ffwin;           // end of the current window
    ffwin = ffpgoff + windowSize;
    if ((int)ffwin > sb.st_size)
#else
      off64_t ffpgoff = ffoff & pageMask;
    off64_t ffwin = ffpgoff + windowSize;   // end of the current window

    if ( ffwin > sb.st_size )
#endif
      ffwin = sb.st_size;

    mapAddr = mmap64(mapAddr, windowSize, PROT_READ,
                     (mapAddr == 0 ? 0 : MAP_FIXED) | MAP_SHARED,
                     fffd, ffpgoff);

    if (mapAddr == MAP_FAILED)
      errexit("ff_read: could not mmap flatfile %s, %s\n", ffpath,
              khstrerror(errno).c_str());

    const char* ffp = (const char*)mapAddr + (ffoff & pageOffMask);

    while (ffoff < (int)ffwin) {
      // get header from bigendian buffer
      // the data is 32bit aligned so casting buffer is safe
      FFRecHeader header = *(const FFRecHeader*)ffp;
      header.BigEndianToHost();

      // Todo: Don't use literal '24' here
      if (header.len() > 1024*1024*3 || header.level() > 24 ||
          header.x() > (1ull << header.level()) ||
          header.y() > (1ull << header.level())) {
        fprintf(stderr, "Suspicious ff file - possibly invalid record in:\n");
        fprintf(stderr, "tile [%lu] len %d - %d %d %d - vers %d\n",
                (unsigned long int)recnum,
                (int)header.len(), (int)header.level(),
                (int)header.x(), (int)header.y(),
                (int)header.vers());
        munmap(mapAddr, windowSize);
        close(fffd);
        return -1;
      }

      off64_t clen = header.paddedLen();

      // check for premature end of file
      if (ffoff + sizeof(FFRecHeader) + clen > (unsigned int)sb.st_size) {
        fprintf(stderr, "Broken ff file - end of file in:\n");
        fprintf(stderr, "tile [%lu] len %d - %d %d %d - vers %d\n",
                (unsigned long int)recnum,
                (int)header.len(), (int)header.level(),
                (int)header.x(), (int)header.y(),
                (int)header.vers());
        munmap(mapAddr, windowSize);
        close(fffd);
        return -1;
      }

      // remap the window if the compressed data spans current
      if (off64_t(ffoff + sizeof(FFRecHeader) + clen) > ffwin)
        break;
            
      const char* jp = ffp + sizeof(FFRecHeader);
      if (!(*callback)(&header, (void *) jp, userData)) {
        // the callback has requested that we stop traversing
        munmap(mapAddr, windowSize);
        close(fffd);
        return 0;
      }

      ffoff += sizeof(FFRecHeader) + clen;
      ffp += sizeof(FFRecHeader) + clen;

      recnum++;
    } // while ffoff < ffwin

    winnum ++;
  } // while ffoff < sb.st_size

  munmap(mapAddr, windowSize);
  close(fffd);

  return 0;
}   // End ff_read()


struct EstimateCallbackData {
  int num;
  std::uint32_t totalSize;
  EstimateCallbackData(void) : num(0), totalSize(0) { }
};


static bool
EstimateCallback(const FFRecHeader *head, void * /* data */, void *userData)
{
  EstimateCallbackData *cbData = (EstimateCallbackData*)userData;

  ++cbData->num;
  cbData->totalSize += head->paddedLen() + sizeof(FFRecHeader);

  return (cbData->num < 11);
}

 std::uint64_t ff_estimate_numentries(const char *ffpath) 
{
  // get the total size of the file
  int fffd = open(ffpath, O_RDONLY | O_LARGEFILE);
  if (fffd == -1)
    errexit("ff_read: could not open %s, %s\n", ffpath,
            khstrerror(errno).c_str());
  struct stat64 sb;
  fstat64(fffd, &sb);
  close(fffd);

  // get the average size of the first 10 records
  EstimateCallbackData estimate;
  if (ff_read(ffpath, EstimateCallback, &estimate) < 0) {
    errexit("ff_read: unable to sample file for entries estimate\n");
  }
  double averageSize = (double)estimate.totalSize / (double)estimate.num;

  // estimate by dividing the total file size by the average record size
  return (std::uint64_t)((double)sb.st_size / averageSize);
}

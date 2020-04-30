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


#include <kbf.h>
#include <khFileUtils.h>
#include <khSimpleException.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


kbfFile::kbfFile()
  : fp(NULL) {
}

kbfFile::~kbfFile() {
  if (fp != NULL)
    close();
}

bool kbfFile::open(const char* filename, int mode) {
  bool retval = false;

  if (mode == KBF_READ) {
    if ((fp = fopen(filename, "r")) != NULL)
      retval = true;
  } else if (mode == KBF_WRITE) {
    if ((fp = fopen(filename, "a")) != NULL)
      retval = true;
  }

  return retval;
}

void kbfFile::resetposition() {
  fseek(fp, 0, SEEK_SET);
}

int kbfFile::eof() {
  return feof(fp);
}

void kbfFile::close() {
  fclose(fp);
  fp = NULL;
}

//
// read a record header, and optionally the record itself
// depending on buffersize.  0 = header only, non-0 = header and record
//
bool kbfFile::read(kbfFileHeader& header, char* buffer, std::uint32_t buffersize,
                   std::int32_t &fileposition) {
  if (fp == NULL)
    return false;

  // read record header
  if (fread(&header, sizeof(kbfFileHeader), 1, fp) != 1)
    return false;

  if (eof())
    return false;

#if defined(__sgi)
  header.byteSwap();
#endif

  // validate header magic
  if (!header.checkMagic())
    return false;

  // upate file position to point to data record
  if ((fileposition = ftell(fp)) == -1)
    return false;

  bool retval = false;

  // skip forward to next record header
  if (buffersize == 0) {
    if (fseek(fp, header.size, SEEK_CUR) == 0)
      retval = true;
    // read record contents if buffer is big enough
  } else if (buffersize >= header.size) {
    if (fread(buffer, header.size, 1, fp) == 1)
      retval = true;
  }

  return retval;
}

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


#ifndef COMMON_PACKETFILE_PACKETFILE_H__
#define COMMON_PACKETFILE_PACKETFILE_H__

// PacketFile - a packet file is a FileBundle with an attached index
// for record access.  A packet file may store vector, terrain, or
// image data.
//
// To write or read a packet file, don't include this header
// directly.  Include packetfilewriter.h or packetfilereader.h.

#include <common/khTypes.h>
#include <string>

// PacketFile - internal namespace to define some constants common to
// reader and writer

namespace PacketFile {
  extern const std::string kIndexBase;
  extern const std::string kSignature;
  extern const std::uint16_t kFormatVersion;
  extern const size_t kIndexHeaderSize;
  extern bool  IsPacketFile(const std::string &path);
  extern std::string IndexFilename(const std::string &path);
};

#endif  // COMMON_PACKETFILE_PACKETFILE_H__

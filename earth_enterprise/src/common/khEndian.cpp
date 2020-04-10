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


#include "khEndian.h"
#include <third_party/rsa_md5/crc32.h>

// ****************************************************************************
// ***  EndianWriteBuffer
// ****************************************************************************
void EndianWriteBuffer::rawwrite(const void *src, unsigned int len) {
  size_type new_position = PrepareToStore(len);
  buf.replace(position_, len, (const char *)src, len);
  position_ = new_position;
}
void EndianWriteBuffer::pad(unsigned int len, char pad) {
  size_type new_position = PrepareToStore(len);
  buf.replace(position_, len, len, pad);
  position_ = new_position;
}
void EndianWriteBuffer::push(const std::string &s) {
  std::string::size_type found = s.find('\0');
  if (found == std::string::npos) {
    // c_str() will add null
    rawwrite(s.c_str(), s.size()+1);
  } else {
    rawwrite(s.data(), found+1);
  }
}
EndianWriteBuffer::EndianWriteBuffer(Endianness bufEndianness, unsigned int reserve) :
    bufferEndianness(bufEndianness),
    position_(0) {
  if (reserve) {
    buf.reserve(reserve);
  }
}




// ****************************************************************************
// ***  EndianReadBuffer
// ****************************************************************************
EndianReadBuffer::EndianReadBuffer(Endianness bufEndianness,
                                   const void* data, size_type size) :
    std::string((const char *)data, size),
    bufferEndianness(bufEndianness),
    next_(0)
{ }
EndianReadBuffer::EndianReadBuffer(Endianness bufEndianness,
                                   unsigned int reserve_size) :
    std::string(), bufferEndianness(bufEndianness),
    next_(0)
{
  if (reserve_size) {
    reserve(reserve_size);
  }
}

void EndianReadBuffer::Skip(size_type count) {
  if (next_ + count <= size()) {
    next_ += count;
  } else {
    throw khSimpleException(
        "EndianReadBuffer: Skip ran off end of buffer");
  }
}

void EndianReadBuffer::rawread(void *dest, size_type count) {
  if (next_ + count <= size()) {
    ::memcpy(dest, data()+next_, count);
    next_ += count;
  } else {
    throw khSimpleException(
        "EndianReadBuffer: rawread ran off end of buffer");
  }
}

const char * EndianReadBuffer::PullRaw(size_type count) {
  if (next_ + count <= size()) {
    const char *result = data() + next_;
    next_ += count;
    return result;
  } else {
    throw khSimpleException(
        "EndianReadBuffer: PullRaw ran off end of buffer");
  }
}

void EndianReadBuffer::Pull(std::string *c_string) {
  size_type found = find('\0', next_);
  if (found == npos) {
    throw khSimpleException(
        "EndianReadBuffer: C string ran off end of buffer");
  } else {
    c_string->assign(*this, next_, found - next_);
    next_ = found + 1;
  }
}

EndianReadBuffer::size_type EndianReadBuffer::Seek(size_type new_pos) {
  if (new_pos >= 0  &&  new_pos <= size()) {
    size_type prev = next_;
    next_ = new_pos;
    return prev;
  } else {
    throw khSimpleException(
        "EndianReadBuffer: invalid Seek position");
  }
}

void EndianReadBuffer::CheckCRC(size_type size, const char *msg) {
  size_type checkSize = size - sizeof(std::uint32_t);
  std::uint32_t calcCRC = Crc32(CurrPtr(), checkSize);
  std::uint32_t storedCRC;
  if (bufferEndianness == LittleEndian) {
    FromLittleEndianBuffer(&storedCRC, CurrPtr() + checkSize);
  } else {
    FromBigEndianBuffer(&storedCRC, CurrPtr() + checkSize);
  }
  if (calcCRC != storedCRC) {
    throw khSimpleException("CRC mismatch in ") << msg;
  }
}


void EndianReadBuffer::SetValue(const std::string& src_str) {
  // We do not want to use std::string = operator coz it will de-allocate
  // the current mem and point to the src memory and allocate memory at a later
  // stage (John Johnson for more details)
  resize(src_str.size());
  ::memcpy((void*) data(), src_str.data(), size());
  next_ = 0;
}


void EndianReadBuffer::SetValueFromBuffer(const char* src, int count) {
  // We do not want to use std::string = operator coz it will de-allocate
  // the current mem and point to the src memory and allocate memory at a later
  // stage (John Johnson for more details)
  resize(count);
  ::memcpy((void*) data(), src, size());
  next_ = 0;
}

// ****************************************************************************
// ***  FixedLengthString
// ****************************************************************************
void FixedLengthWriteString::Push(EndianWriteBuffer& buf) const {
  if (str.size() >= fixedLength) {
    buf.rawwrite(str.data(), fixedLength);
  } else {
    buf.rawwrite(str.data(), str.size());
    buf.pad(fixedLength - str.size(), padding);
  }
}

void FixedLengthWriteStringRaw::Push(EndianWriteBuffer& buf) const {
  if (strLength >= fixedLength) {
    buf.rawwrite(str, fixedLength);
  } else {
    buf.rawwrite(str, strLength);
    buf.pad(fixedLength - strLength, padding);
  }
}

void FixedLengthReadString::Pull(EndianReadBuffer& buf) const {
  str.resize(fixedLength);
  buf.rawread(&str[0], fixedLength);
}

void FixedLengthReadStringRaw::Pull(EndianReadBuffer& buf) const {
  buf.rawread(str, fixedLength);
}

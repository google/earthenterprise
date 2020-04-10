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


#ifndef _khEndian_h_
#define _khEndian_h_

#include <string.h>
#include <string>
#include <vector>
#include <endian.h>
#include <common/base/macros.h>
#include <khTypes.h>
#include <cstdint>
#include <khGuard.h>
#include <khSimpleException.h>
#include <khEndianImpl.h>

// ****************************************************************************
// ***  EndianBuffer usage
// ***
// ***  Write serializer/deserializer classes using base classes
// ***  (EndianWriteBuffer and EndianReadBuffer), they will then work for
// ***  both big and little endian buffers.
// ***

// ***  Instantiate buffers as {Big,Little}Endian{Read,Write}Buffer and
// ***  treat them as a stream for pushing/pulling things to be serialized.

// ***  Things that support streaming to/from EndianBuffers
// ***  - numeric types (e.g std::uint8_t, std::int16_t, float, etc.)
// ***  - std::string
// ***        -> serialized as zero-terminated string
// ***  - FixedLengthString(str, len, pad=0)
// ***        -> stored w/o termination, will truncate or pad as necessary
// ***  - EncodeAs<type>(val)/DecodeAs<type>(val)
// ***        -> does static_cast while pushing/pulling
// ***        -> useful for Enums
// ***  - Custom classes that have defined their own serializers
// ***
// ***  ----- example usage -----
// ***  enum Color {Oracle, Red, Green, Blue};
// ***  ...
// ***  LittleEndianWriteBuffer buf;
// ***  std::int32_t myi = 123;
// ***  std::string mystr = "Hello World";
// ***  float64 myf = 456.789;
// ***  Color myc = Red;
// ***  buf << myi << mystr << myf
// ***      << EncodeAs<std::uint8_t>(myc)
// ***      << FixedLengthString("Google Magic", 16, ' ');
// ***  std::uint32_t RecordSize = buf.size();
// ***  writer.Write(buf.data(), RecordSize);
// ***  ...
// ***  LittleEndianReadBuffer rbuf;
// ***  reader.Read(buf, RecordSize);
// ***  std::int32_t ri;
// ***  std::string rstr;
// ***  float64 rf;
// ***  Color rc;
// ***  std::string rmagic;
// ***  rbuf >> ri >> rstr >> rf
// ***       >> DecodeAs<std::uint8_t>(rc)
// ***       >> FixedLengthString(rmagic, 16);
// ***
// ****************************************************************************








// ****************************************************************************
// ***  EndianWriteBuffer
// ***
// ***  Base class for LittleEndianWriteBuffer and BigEndianWriteBuffer
// ***
// ***  When writing serializer functions always do it with this base class.
// ***  When instantiating a buffer, instantiate on of the derived classes.
// ***
// ***  Normally you will just use the stream operators. But occasionally you
// ***  may need to call some of the methods directly
// ***
// ***  -- example --
// ***  LittleEndianWriteBuffer buf;
// ***  std::int32_t myi = 123;
// ***  float64 myf = 456.789;
// ***  buf << myi << myf;
// ***  writer.Read(buf.data(), buf.size());
// ****************************************************************************
class EndianWriteBuffer {
 protected:
  enum Endianness {LittleEndian, BigEndian};
 public:
  typedef std::string::size_type size_type;

  size_type CurrPos(void) const { return position_; }
  void rawwrite(const void *src, unsigned int len);
  void pad(unsigned int len, char pad);

  // push object to buffer - instantiated only for numeric types
  template <class T> inline void push(const T &t);

  // write the string with null terminator
  void push(const std::string &s);

  inline const char&  operator[](size_type i) const { return buf[i]; }
  inline const void * data(void) const     { return buf.data(); }
  inline size_type    size(void) const     { return buf.size(); }
  inline size_type    capacity(void) const { return buf.capacity(); }
  inline void         reset(void)          { Seek(0); buf.resize(0); }

  // Allow random access to buffer (note: a Seek beyond the end of the
  // buffer will cause the buffer to be extended at the next output to
  // the new position)
  inline void Seek(size_type new_position) {
    position_ = new_position;
  }
 protected:
  EndianWriteBuffer(Endianness bufEndianness, unsigned int reserve);

 private:

  Endianness bufferEndianness;
  std::string buf;
  size_type position_;

  inline size_type PrepareToStore(unsigned int len) {
    size_type new_position = position_ + len;
    if (new_position > buf.size()) {
      buf.resize(new_position);
    }
    return new_position;
  }

  DISALLOW_COPY_AND_ASSIGN(EndianWriteBuffer);
};


class LittleEndianWriteBuffer : public EndianWriteBuffer {
 public:
  inline LittleEndianWriteBuffer(unsigned int reserve = 2048) :
      EndianWriteBuffer(LittleEndian, reserve) { }
 private:
  DISALLOW_COPY_AND_ASSIGN(LittleEndianWriteBuffer);
};
class BigEndianWriteBuffer : public EndianWriteBuffer {
 public:
  inline BigEndianWriteBuffer(unsigned int reserve = 2048) :
      EndianWriteBuffer(BigEndian, reserve) { }
 private:
  DISALLOW_COPY_AND_ASSIGN(BigEndianWriteBuffer);
};


// ****************************************************************************
// ***  EndianReadBuffer
// ***
// ***  Base class for LittleEndianReadBuffer and BigEndianReadBuffer
// ***
// ***  When writing serializer functions always do it with this base class.
// ***  When instantiating a buffer, instantiate on of the derived classes.
// ***
// ***  Normally you will just use the stream operators. But occasionally you
// ***  may need to call some of the methods directly
// ***
// ***  -- example --
// ***  LittleEndianReadBuffer buf;
// ***  buf.resize(toread);
// ***  reader.Read(&buf[0], toread);
// ***  std::int32_t myi;
// ***  float64 myf;
// ***  buf >> myi >> myf;
// ****************************************************************************
class EndianReadBuffer : public std::string {
 protected:
  enum Endianness {LittleEndian, BigEndian};
 public:
  static const size_type kDefaultReserve = 2048;

  void Skip(size_type count);
  void rawread(void *dest, size_type count);

  // pull object from buffer - instantiated only for numeric types
  template <class T> inline void Pull(T *t);

  // Pull zero terminated string into s
  void Pull(std::string *s);

  // Pull fixed number of characters (return pointer into buffer)
  const char *PullRaw(size_type count);

  // Change position in buffer (returns previous position)
  size_type Seek(size_type new_pos);

  size_type CurrPos(void) const { return next_; }
  const char * CurrPtr(void) const { return &operator[](next_); }
  bool AtEnd() const { return next_ == size(); }

  // compute a Crc32 on size-sizeof(Crc32) bytes starting at CurrPos
  // check it against a Crc32 stored at CurrPos + size - sizeof(Crc32)
  void CheckCRC(size_type size, const char *msg);

  // Copy from a std::string.
  void SetValue(const std::string& src_str);

  // Copy "count" bytes from a char buffer.
  void SetValueFromBuffer(const char* src, int count);

  // Define Factory methods to create a new read buffer of the same type
  khTransferGuard<EndianReadBuffer>
  NewBuffer(const void* data, size_type size) {
    return TransferOwnership(
        new EndianReadBuffer(bufferEndianness, data, size));
  }
  khTransferGuard<EndianReadBuffer>
  NewBuffer(size_type reserve = kDefaultReserve) {
    return TransferOwnership(
        new EndianReadBuffer(bufferEndianness, reserve));
  }
 protected:
  EndianReadBuffer(Endianness bufEndianness, const void* data, size_type size);
  EndianReadBuffer(Endianness bufEndianness, unsigned int reserve = kDefaultReserve);

 private:

  Endianness bufferEndianness;
  size_type next_;

  DISALLOW_COPY_AND_ASSIGN(EndianReadBuffer);
};

class LittleEndianReadBuffer : public EndianReadBuffer {
 public:
  inline LittleEndianReadBuffer(const void* data, size_type size) :
      EndianReadBuffer(LittleEndian, data, size) { }
  inline LittleEndianReadBuffer(const std::string &data) :
      EndianReadBuffer(LittleEndian, &data[0], data.size()) { }
  inline LittleEndianReadBuffer(const std::vector<char> &data) :
      EndianReadBuffer(LittleEndian, &data[0], data.size()) { }
  inline LittleEndianReadBuffer(const std::vector<unsigned char> &data) :
      EndianReadBuffer(LittleEndian, &data[0], data.size()) { }
  inline LittleEndianReadBuffer(size_type reserve = kDefaultReserve) :
      EndianReadBuffer(LittleEndian, reserve) { }
 private:
  DISALLOW_COPY_AND_ASSIGN(LittleEndianReadBuffer);
};
class BigEndianReadBuffer : public EndianReadBuffer {
 public:
  inline BigEndianReadBuffer(const void* data, size_type size) :
      EndianReadBuffer(BigEndian, data, size) { }
  inline BigEndianReadBuffer(const std::string &data) :
      EndianReadBuffer(BigEndian, &data[0], data.size()) { }
  inline BigEndianReadBuffer(const std::vector<char> &data) :
      EndianReadBuffer(BigEndian, &data[0], data.size()) { }
  inline BigEndianReadBuffer(const std::vector<unsigned char> &data) :
      EndianReadBuffer(BigEndian, &data[0], data.size()) { }
  inline BigEndianReadBuffer(size_type reserve = kDefaultReserve) :
      EndianReadBuffer(BigEndian, reserve) { }
 private:
  DISALLOW_COPY_AND_ASSIGN(BigEndianReadBuffer);
};


/*****************************************************************************
 **
 ** Low-level Public API for endian swapping of numeric types to/from a
 ** pentiallially misaligned buffer. These are used to implement the higher
 ** level APIs.
 **
 ** Parameters are (dest, src) just like memcpy.
 **
 ** --- example ---
 ** std::vector<char> buf(50);
 ** ToLittleEndianBuffer(&buf[27], 12345);
 ** std::int32_t i;
 ** FromLittleEndianBuffer(&i, &buf[27]);
 ** assert(i==12345);
 ** ---------------
 **
 ** NOTE: You should probably be using a higher level API like EndianReadBuffer
 ** or EndianWriteBuffer
 *****************************************************************************/
#if __BYTE_ORDER == __BIG_ENDIAN
#define DefineToFromEndianBuffer(T)                             \
inline void ToBigEndianBuffer(void *buf, const T &t) {          \
  bitcopy<sizeof(T)>(buf, &t);                                  \
}                                                               \
inline void FromBigEndianBuffer(T *t, const void *buf) {        \
  bitcopy<sizeof(T)>(t, buf);                                   \
}                                                               \
inline void ToLittleEndianBuffer(void *buf, const T &t) {       \
  bitcopyswap<sizeof(T)>(buf, &t);                              \
}                                                               \
inline void FromLittleEndianBuffer(T *t, const void *buf) {     \
  bitcopyswap<sizeof(T)>(t, buf);                               \
}
#else
#define DefineToFromEndianBuffer(T)                             \
inline void ToBigEndianBuffer(void *buf, const T &t) {          \
  bitcopyswap<sizeof(T)>(buf, &t);                              \
}                                                               \
inline void FromBigEndianBuffer(T *t, const void *buf) {        \
  bitcopyswap<sizeof(T)>(t, buf);                               \
}                                                               \
inline void ToLittleEndianBuffer(void *buf, const T &t) {       \
  bitcopy<sizeof(T)>(buf, &t);                                  \
}                                                               \
inline void FromLittleEndianBuffer(T *t, const void *buf) {     \
  bitcopy<sizeof(T)>(t, buf);                                   \
}
#endif

DefineToFromEndianBuffer(std::int8_t);
DefineToFromEndianBuffer(std::uint8_t);
DefineToFromEndianBuffer(std::int16_t);
DefineToFromEndianBuffer(std::uint16_t);
DefineToFromEndianBuffer(std::int32_t);
DefineToFromEndianBuffer(std::uint32_t);
DefineToFromEndianBuffer(std::int64_t);
DefineToFromEndianBuffer(std::uint64_t);
DefineToFromEndianBuffer(float32);
DefineToFromEndianBuffer(float64);


// ****************************************************************************
// ***  EndianWriteBuffer template method definitions
// ***
// ***  Declared here so they can be after the definition of
// **   ToLittleEndianBuffer and ToBigEndianBuffer
// ****************************************************************************
template <class T>
inline void EndianWriteBuffer::push(const T &t) {
  size_type new_position = PrepareToStore(sizeof(T));
  if (bufferEndianness == LittleEndian) {
    ToLittleEndianBuffer(&buf[position_], t);
  } else {
    ToBigEndianBuffer(&buf[position_], t);
  }
  position_ = new_position;
}


// ****************************************************************************
// ***  EndianReadBuffer template method definitions
// ***
// ***  Declared here so they can be after the definition of
// **   FromLittleEndianBuffer and FromBigEndianBuffer
// ****************************************************************************
template <class T>
inline void EndianReadBuffer::Pull(T *t) {
  if (next_ + sizeof(T) <= size()) {
    if (bufferEndianness == LittleEndian) {
      FromLittleEndianBuffer(t, data() + next_);
    } else {
      FromBigEndianBuffer(t, data() + next_);
    }
    next_ += sizeof(T);
  } else {
    throw khSimpleException(
        "EndianReadBuffer: pull past end of buffer");
  }
}

// ****************************************************************************
// ***  Stream Operators
// ****************************************************************************
#define DefineStreamOperators(T)                                        \
inline EndianWriteBuffer& operator<<(EndianWriteBuffer& buf, T t) {     \
  buf.push(t);                                                          \
  return buf;                                                           \
}                                                                       \
inline EndianReadBuffer& operator>>(EndianReadBuffer& buf, T &t) {      \
  buf.Pull(&t);                                                         \
  return buf;                                                           \
}
DefineStreamOperators(std::int8_t);
DefineStreamOperators(std::uint8_t);
DefineStreamOperators(std::int16_t);
DefineStreamOperators(std::uint16_t);
DefineStreamOperators(std::int32_t);
DefineStreamOperators(std::uint32_t);
DefineStreamOperators(std::int64_t);
DefineStreamOperators(std::uint64_t);
DefineStreamOperators(float32);
DefineStreamOperators(float64);
DefineStreamOperators(std::string);

// stream wrapper around T::Push()
template <class T>
inline EndianWriteBuffer& operator<<(EndianWriteBuffer& buf, const T &t) {
  t.Push(buf);
  return buf;
}

// stream wrapper around T::Pull()
template <class T>
inline EndianReadBuffer& operator>>(EndianReadBuffer& buf, T &t) {
  t.Pull(buf);
  return buf;
}



// ****************************************************************************
// ***  EncodeAs/DecodeAs
// ****************************************************************************
template <class T, class U>
class EndianDecoder {
  U &u;
 public:
  inline EndianDecoder(U &u_) : u(u_) { }
  inline void Pull(EndianReadBuffer &buf) const {
    T t;
    buf >> t;
    u = static_cast<U>(t);
  }
};
template <class T, class U>
inline EndianReadBuffer& operator>>(EndianReadBuffer& buf,
                                    const EndianDecoder<T, U> &decoder) {
  decoder.Pull(buf);
  return buf;
}
template <class T, class U>
inline T EncodeAs(const U &u) {
  return static_cast<T>(u);
}
template <class T, class U>
inline EndianDecoder<T,U> DecodeAs(U &u) {
  return EndianDecoder<T,U>(u);
}


// ****************************************************************************
// ***  Vector stream operators
// ****************************************************************************
template <class T>
inline EndianWriteBuffer& operator<<(EndianWriteBuffer& buf,
                                     const std::vector<T> &t) {
  buf << EncodeAs<std::uint32_t>(t.size());
  for (unsigned int i = 0; i < t.size(); ++i) {
    buf << t[i];
  }
  return buf;
}

template <class T>
inline EndianReadBuffer& operator>>(EndianReadBuffer& buf,
                                    std::vector<T> &t) {
  std::uint32_t count;
  buf >> count;
  t.resize(count);
  for (unsigned int i = 0; i < t.size(); ++i) {
    buf >> t[i];
  }
  return buf;
}

// ****************************************************************************
// ***  bool stream operators
// ****************************************************************************
inline EndianWriteBuffer& operator<<(EndianWriteBuffer& buf, const bool &t) {
  buf << EncodeAs<std::uint8_t>(t);
  return buf;
}

inline EndianReadBuffer& operator>>(EndianReadBuffer& buf, bool &t) {
  buf >> DecodeAs<std::uint8_t>(t);
  return buf;
}



// ****************************************************************************
// ***  FixedLengthString
// ***
// ***  Stream modifier for emitting a string as fixed length rather than null
// ***  terminated.
// ***
// ***  --- example usage ---
// *** const char magic[16] = "GEIndexFile     ";
// *** LittleEndianWriteBuffer wbuf;
// *** wbuf << FixedLengthString(magic, 16);
// *** ...;
// *** LittleEndianReadBuffer rbuf;
// *** ...;
// *** std::string magicstr;
// *** buf >> FixedLengthString(magicstr, 16);
// ****************************************************************************

class FixedLengthWriteString {
  const std::string &str;
  unsigned int fixedLength;
  char padding;

 public:
  void Push(EndianWriteBuffer& buf) const;

  FixedLengthWriteString(const std::string &str_, unsigned int len, char pad) :
      str(str_), fixedLength(len), padding(pad) { }
};
class FixedLengthWriteStringRaw {
  const char *str;
  unsigned int strLength;
  unsigned int fixedLength;
  char padding;

 public:
  void Push(EndianWriteBuffer& buf) const;

  FixedLengthWriteStringRaw(const char *str_, unsigned int len, char pad) :
      str(str_), strLength(strlen(str_)), fixedLength(len), padding(pad) { }
};
class FixedLengthReadString {
  std::string &str;
  unsigned int fixedLength;

 public:
  void Pull(EndianReadBuffer& buf) const;

  FixedLengthReadString(std::string &str_, unsigned int len) :
      str(str_), fixedLength(len) { }
};
class FixedLengthReadStringRaw {
  char *str;
  unsigned int fixedLength;

 public:
  void Pull(EndianReadBuffer& buf) const;

  FixedLengthReadStringRaw(char *str_, unsigned int len) :
      str(str_), fixedLength(len) { }
};
inline FixedLengthWriteString FixedLengthString(const std::string &str_,
                                                unsigned int len, char pad = 0) {
  return FixedLengthWriteString(str_, len, pad);
}
inline FixedLengthWriteStringRaw FixedLengthString(const char *str_,
                                                   unsigned int len, char pad = 0) {
  return FixedLengthWriteStringRaw(str_, len, pad);
}
inline FixedLengthReadString FixedLengthString(std::string &str_, unsigned int len) {
  return FixedLengthReadString(str_, len);
}
inline FixedLengthReadStringRaw FixedLengthString(char *str_, unsigned int len) {
  return FixedLengthReadStringRaw(str_, len);
}

// specialize these because we read into temporary instances so they
// must have const ref arguments
inline EndianReadBuffer& operator>>(EndianReadBuffer& buf,
                                    const FixedLengthReadString &fls) {
  fls.Pull(buf);
  return buf;
}
// specialize these because we read into temporary instances so they
// must have const ref arguments
inline EndianReadBuffer& operator>>(EndianReadBuffer& buf,
                                    const FixedLengthReadStringRaw &fls) {
  fls.Pull(buf);
  return buf;
}



/*****************************************************************************
 **
 ** LittleEndianToHost()
 ** BigEndianToHost()
 ** HostToLittleEndian()
 ** HostToBigEndian()
 **
 ** Older Low-level Public API for endian swapping of numeric types
 **
 ** Use if you already have a variable (not storage in a buffer) that you
 ** know is of a particular endianness and you want to convert it to the
 ** host endianness.
 **
 ** --- example ---
 ** std::int32_t litleEndianI = ...;
 ** std::int32_t i = LittleEndianToHost(littleEndianI);
 ** ---------------
 **
 ** NOTE: You should probably be using a higher level API like EndianBuffer
 *****************************************************************************/
#if __BYTE_ORDER == __BIG_ENDIAN
#define DefineEndianToHost(T)                   \
inline T BigEndianToHost(T t) {                 \
  return t;                                     \
}                                               \
inline T LittleEndianToHost(T t) {              \
  T tmp;                                        \
  bitcopyswap<sizeof(T)>(&tmp, &t);             \
  return tmp;                                   \
}
#else
#define DefineEndianToHost(T)                   \
inline T BigEndianToHost(T t) {                 \
  T tmp;                                        \
  bitcopyswap<sizeof(T)>(&tmp, &t);             \
  return tmp;                                   \
}                                               \
inline T LittleEndianToHost(T t) {              \
  return t;                                     \
}
#endif

DefineEndianToHost(std::int8_t);
DefineEndianToHost(std::uint8_t);
DefineEndianToHost(std::int16_t);
DefineEndianToHost(std::uint16_t);
DefineEndianToHost(std::int32_t);
DefineEndianToHost(std::uint32_t);
DefineEndianToHost(long int);
DefineEndianToHost(long long int);
DefineEndianToHost(std::uint64_t);
DefineEndianToHost(float32);
DefineEndianToHost(float64);


// just another name for code clarity
template <class T>
inline T HostToBigEndian(T t) { return BigEndianToHost(t); }
template <class T>
inline T HostToLittleEndian(T t) { return LittleEndianToHost(t); }


/*****************************************************************************
 **
 **   Deprecated API - inplace endian swapping
 **
 ** swapb & swapbysize do nothing on little endian machines and do endian
 ** swapping on big endian machines.  The problem with this API is that the
 ** name implies that the swap is always going to happen but it may not.
 **
 *****************************************************************************/
#if __BYTE_ORDER == __BIG_ENDIAN
#define khEndianDoSwap
#endif

#ifdef khEndianDoSwap
#define swapb( value )           bitswap<sizeof(T)>(&value);
#define swapbysize( ptr, size )  bitswap<size>( ptr )
#else
#define swapb( value )
#define swapbysize( pt, size )
#endif





#endif // ! _khEndian_h_

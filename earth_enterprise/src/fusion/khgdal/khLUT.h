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

#ifndef __khLUT_h
#define __khLUT_h

#include <string>
#include <sstream>
#include <khException.h>
#include <khstl.h>
#include <gdal.h>

// size helper classes, these will completely dissapper into inlines
// at compile time
template <class T>
class khLUTHelper
{
 public:
  enum {CanLUT = 0};
};
template <>
class khLUTHelper<unsigned char>
{
 public:
  enum {CanLUT = 1};
  static const unsigned char min = 0;
  static const unsigned char max = UCHAR_MAX;
  static const unsigned int  range = UCHAR_MAX+1;
  static unsigned int index(unsigned char c) { return c; }
};
template <>
class khLUTHelper<unsigned short>
{
 public:
  enum {CanLUT = 1};
  static const unsigned short min = 0;
  static const unsigned short max = USHRT_MAX;
  static const unsigned int range = USHRT_MAX+1;
  static unsigned int index(unsigned short s) { return s; }
};
template <>
class khLUTHelper<short>
{
 public:
  enum {CanLUT = 1};
  static const short min = SHRT_MIN;
  static const short max = SHRT_MAX;
  static const unsigned int range = USHRT_MAX+1;
  static unsigned int index(short s) {
    return (unsigned short)(s);
  }
};

template <class T>
class NumericStreamType
{
 public:
  typedef T Type;
};
template <>
class NumericStreamType<unsigned char>
{
 public:
  typedef unsigned int Type;
};


template <class In, class Out>
class khLUT
{
  // private and unimplemented. You can't copy a khLUT.
  khLUT(const khLUT&);
  khLUT& operator=(const khLUT&);

  Out lut[khLUTHelper<In>::range];
 public:
  typedef In  InPixelType;
  typedef Out OutPixelType;
  static const bool IsIdentity = false;

  void Parse(std::istream &in) {
    unsigned int numElem, minElem, maxElem;
    in >> numElem >> minElem >> maxElem;
    if (!in) {
      throw khException(kh::tr("Unable to get num/min/max from LUT table"));
    }
    if ((minElem > maxElem) || (maxElem >= numElem)) {
      throw khException(kh::tr("Invalid num/min/max in LUT table %1/%2/%3")
                        .arg(numElem).arg(minElem).arg(maxElem));
    }
    if (numElem != khLUTHelper<In>::range) {
      throw khException(kh::tr
                        ("Invalid LUT table size %1. Expected %2.")
                        .arg(numElem).arg(khLUTHelper<In>::range));
    }
    for (unsigned int i = 0; i < minElem; ++i) {
      lut[i] = Out();   // Out() provides the '0' for the type
    }
    for (unsigned int i = maxElem + 1; i < numElem; ++i) {
      lut[i] = Out();
    }
    while (minElem <= maxElem) {
      int data;
      if (!(in >> data)) {
        throw khException(kh::tr("Unable to read LUT table entry"));
      }
      if (data < 0) {
        typename NumericStreamType<Out>::Type data2;
        if (!(in >> data2)) {
          throw khException(kh::tr("Unable to read LUT table entry"));
        }
        for (; data && (minElem < numElem); ++data) {
          lut[minElem++] = data2;
        }
      } else {
        lut[minElem++] = Out(data);
      }
    }
  }
  void Store(std::string &storage) {
    int minIndex = 0;
    while (lut[minIndex] == Out() && minIndex < (int)khLUTHelper<In>::range-1)
      minIndex++;

    int maxIndex = khLUTHelper<In>::range - 1;
    while (lut[maxIndex] == Out() && maxIndex > 0)
      maxIndex--;

    // is this an all zero table? if so, still write one zero
    if (maxIndex < minIndex) {
      minIndex = 0;
      maxIndex = 0;
    }

    std::ostringstream out;
    out << "Table:"
        << khLUTHelper<In>::range << ' '
        << minIndex << ' '
        << maxIndex << ' ';

    typedef typename NumericStreamType<Out>::Type StreamType;

    // describe counts
    for (int leftIndex = minIndex; leftIndex <= maxIndex; ) {
      // determine run length
      int rightIndex = leftIndex;
      while (rightIndex <= maxIndex && lut[leftIndex] == lut[rightIndex])
        rightIndex++; // advances PAST end of run

      // describe counts
      if (rightIndex - leftIndex < 2)
        out << (StreamType)lut[leftIndex] << ' ';
      else
        out << -(rightIndex - leftIndex) << ' '
            << (StreamType)lut[leftIndex] << ' ';
      leftIndex = rightIndex;
    }
    storage = out.str();
  }

  khLUT(void) {
    for (unsigned int i = 0; i < khLUTHelper<In>::range; ++i) {
      lut[i] = Out();
    }
  }

  khLUT(const std::string &lutdef) {
    if (StartsWith(lutdef, "Table:")) {
      std::istringstream in(lutdef.substr(6, std::string::npos));
      Parse(in);
    } else {
      throw khException(kh::tr("Unrecognized LUT definition \"%1\"")
                        .arg(lutdef.c_str()));
    }
  }

  inline Out operator[](In i) const { return lut[khLUTHelper<In>::index(i)];}
};


template <class InOut>
class khIdentityLUT
{
 public:
  typedef InOut  InPixelType;
  typedef InOut OutPixelType;
  static const bool IsIdentity = true;


  khIdentityLUT(const std::string &lutdef) {
    if (!lutdef.empty()) {
      throw khException
        (kh::tr("Unexpected LUT definition for identity LUT"));
    }
  }

  inline InOut operator[](InOut i) const { return i; }
};


template <class In>
void
ParseAndStoreLUT(std::istream &in, GDALDataType outtype, std::string &storage)
{
  switch (outtype) {
    case GDT_Byte: {
      khLUT<In, unsigned char> lut;
      lut.Parse(in);
      lut.Store(storage);
      break;
    }
    case GDT_UInt16: {
      khLUT<In, std::uint16_t> lut;
      lut.Parse(in);
      lut.Store(storage);
      break;
    }
    case GDT_UInt32: {
      khLUT<In, std::uint32_t> lut;
      lut.Parse(in);
      lut.Store(storage);
      break;
    }
#if 0
      // the LUT string representation can't deal with signed output values
      // because use negative values to represent runlength compression
    case GDT_Int16: {
      khLUT<In, std::int16_t> lut;
      lut.Parse(in);
      lut.Store(storage);
      break;
    }
    case GDT_Int32: {
      khLUT<In, std::int32_t> lut;
      lut.Parse(in);
      lut.Store(storage);
      break;
    }
    case GDT_Float32: {
      khLUT<In, float> lut;
      lut.Parse(in);
      lut.Store(storage);
      break;
    }
    case GDT_Float64: {
      khLUT<In, double> lut;
      lut.Parse(in);
      lut.Store(storage);
      break;
    }
#endif
    default:
      throw khException(kh::tr("Unsupported LUT output type %1")
                        .arg(GDALGetDataTypeName(outtype)));
  }
}

#endif /* __khLUT_h */

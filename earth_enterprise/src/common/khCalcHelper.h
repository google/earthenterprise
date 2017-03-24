/*
 * Copyright 2017 Google Inc.
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


#ifndef __khCalcHelper_h
#define __khCalcHelper_h

#include <khTypes.h>
#include <limits>

// ****************************************************************************
// ***  Helper classes - Don't use directly
// ***
// ***  Use khCalcHelper<> defined below.
// ****************************************************************************
template <class T, class AccumT, class DiffT, class SignedT>
class IntegralCalcHelper {
 public:
  typedef T CalcType;
  typedef AccumT AccumType;
  typedef DiffT DiffType;
  typedef SignedT SignedPromote;
  static inline CalcType AverageOf4(CalcType a, CalcType b,
                                    CalcType c, CalcType d) {
    return (CalcType) ((2 +
                        (AccumType)(a) +
                        (AccumType)(b) +
                        (AccumType)(c) +
                        (AccumType)(d) ) / 4);
  }
  static inline CalcType AverageOf2(CalcType a, CalcType b) {
    return (CalcType) ((1 + (AccumType)(a) + (AccumType)(b)) / 2);
  }
  static inline CalcType ZeroOrAverage(CalcType a, CalcType b,
                                       CalcType c, CalcType d) {
    if ((a == 0) ||
        (b == 0) ||
        (c == 0) ||
        (d == 0)) {
      return 0;
    } else {
      return AverageOf4(a, b, c, d);
    }
  }
  static inline CalcType TakeFirst(CalcType a, CalcType,
                                   CalcType, CalcType) {
    return a;
  }
  static inline CalcType Average(AccumType accum, uint count) {
    // since we have to check for count == 0 anyway, we may as well
    // optimize the average for the cases we can
    switch (count) {
      case 0:
        return 0;
      case 1:
        return accum;
      case 2:
        return (accum + 1) / 2;
      case 4:
        return (accum + 2) / 4;
      case 6:
        return (accum + 3) / 6;
      case 8:
        return (accum + 4) / 8;
      case 3:
      case 5:
      case 7:
      default:
        return (CalcType)(0.5 + ((double)accum / (double)count));
    }
    return 0;  // unreached but silences warnings
  }
  static inline T Scale(T t, double factor) {
    return (T) (0.5 + (t * factor));
  }
  static inline bool WithinTolerance(T a, T b, T tolerance) {
    DiffType diff = DiffType(a) - DiffType(b);
    if (diff < 0)
      diff = -diff;
    return (diff <= DiffType(tolerance));
  }
};


template <class T, class AccumT, class DiffT>
class FloatingPointCalcHelper {
 public:
  typedef T CalcType;
  typedef AccumT AccumType;
  typedef DiffT DiffType;
  static inline CalcType AverageOf4(CalcType a, CalcType b,
                                    CalcType c, CalcType d) {
    return (CalcType) (((AccumType)(a) +
                        (AccumType)(b) +
                        (AccumType)(c) +
                        (AccumType)(d)) / (AccumType)4.0);
  }
  static inline CalcType AverageOf2(CalcType a, CalcType b) {
    return (CalcType) (((AccumType)(a) + (AccumType)(b)) / (AccumType)2.0);
  }
  static inline CalcType ZeroOrAverage(CalcType a, CalcType b,
                                       CalcType c, CalcType d) {
    if ((a == 0) ||
        (b == 0) ||
        (c == 0) ||
        (d == 0)) {
      return 0;
    } else {
      return AverageOf4(a, b, c, d);
    }
  }
  static inline CalcType TakeFirst(CalcType a, CalcType,
                                   CalcType, CalcType) {
    return a;
  }
  static inline CalcType Average(AccumType accum, uint count) {
    if (!count)
      return 0.0;
    else
      return (CalcType) (accum / (AccumType)count);
  }
  static inline T Scale(T t, double factor) {
    return (T) (t * factor);
  }
  static inline bool WithinTolerance(T a, T b, T tolerance) {
    DiffType diff = DiffType(a) - DiffType(b);
    if (diff < 0)
      diff = -diff;
    return (diff <= DiffType(tolerance));
  }
};


// ****************************************************************************
// ***  Simple specialized templates to be used by other templates to
// ***  do type-specific calculations
// ****************************************************************************
template <class T>
class khCalcHelper { };

template <>
class khCalcHelper<uint8> :
    public IntegralCalcHelper<uint8, uint16, int16, int16>{ };

template <>
class khCalcHelper<int8> :
    public IntegralCalcHelper<int8, int16, int16, int8> { };

template <>
class khCalcHelper<uint16> :
    public IntegralCalcHelper<uint16, uint32, int32, int32>{ };

template <>
class khCalcHelper<int16> :
    public IntegralCalcHelper<int16, int32, int32, int16> { };

template <>
class khCalcHelper<uint32> :
    public IntegralCalcHelper<uint32, uint64, int64, int64>{ };

template <>
class khCalcHelper<int32> :
    public IntegralCalcHelper<int32, int64, int64, int32> { };

template <>
class khCalcHelper<float> :
    public FloatingPointCalcHelper<float, float, float> { };

template <>
class khCalcHelper<double> :
    public FloatingPointCalcHelper<double, double, double> { };




// ****************************************************************************
// ***  IntegralDataClamper - specializations for the various signed/unsigned
// ***  permutations
// ****************************************************************************
template <class In, class Out, bool inSigned, bool outSigned>
class IntegralDataClamper { };

// Signed -> Signed
template <class In, class Out>
class IntegralDataClamper<In, Out, true, true> {
 public:
  inline static Out Clamp(In val) {
    if (val < std::numeric_limits<Out>::min())
      return std::numeric_limits<Out>::min();
    else if (val > std::numeric_limits<Out>::max())
      return std::numeric_limits<Out>::max();
    else
      return (Out)val;
  }
};
// Signed -> Unsigned
template <class In, class Out>
class IntegralDataClamper<In, Out, true, false> {
  typedef typename khCalcHelper<Out>::SignedPromote SOut;
 public:
  inline static Out Clamp(In val) {
    if (val < 0)
      return 0;
    else if (val > (SOut)std::numeric_limits<Out>::max())
      return std::numeric_limits<Out>::max();
    else
      return (Out)val;
  }
};
// Unsigned -> Signed
template <class In, class Out>
class IntegralDataClamper<In, Out, false, true> {
  typedef typename khCalcHelper<In>::SignedPromote SIn;
 public:
  inline static Out Clamp(In val) {
    if ((SIn)val > std::numeric_limits<Out>::max())
      return std::numeric_limits<Out>::max();
    else
      return (Out)val;
  }
};
// uint64 -> Signed
template <class Out>
class IntegralDataClamper<uint64, Out, false, true> {
 public:
  inline static Out Clamp(uint64 val) {
    if (val > (uint64)std::numeric_limits<Out>::max())
      return std::numeric_limits<Out>::max();
    else
      return (Out)val;
  }
};
// Signed -> uint64
template <class In>
class IntegralDataClamper<In, uint64, true, false> {
 public:
  inline static uint64 Clamp(In val) {
    if (val < 0)
      return 0;
    else
      return (uint64)val;
  }
};



// Unsigned -> Unsigned
template <class In, class Out>
class IntegralDataClamper<In, Out, false, false> {
 public:
  inline static Out Clamp(In val) {
    if (val > std::numeric_limits<Out>::max())
      return std::numeric_limits<Out>::max();
    else
      return (Out)val;
  }
};



// ****************************************************************************
// ***  MixedDataClamper - specialization for the various float/integral
// ***  permutations
// ****************************************************************************
template <class In, class Out, bool inIntegral, bool outIntegral>
class MixedDataClamper { };

// Integral -> Integral
template <class In, class Out>
class MixedDataClamper<In, Out, true, true> :
    public IntegralDataClamper<In, Out,
                               std::numeric_limits<In>::is_signed,
                               std::numeric_limits<Out>::is_signed> {
};
// Integral -> Float
template <class In, class Out>
class MixedDataClamper<In, Out, true, false> {
 public:
  inline static Out Clamp(In val) {
    return (Out)val;
  }
};
// Float -> Integral
template <class In, class Out>
class MixedDataClamper<In, Out, false, true> {
 public:
  inline static Out Clamp(In val) {
    if (val < (In)std::numeric_limits<Out>::min())
      return std::numeric_limits<Out>::min();
    else if (val > (In)std::numeric_limits<Out>::max())
      return std::numeric_limits<Out>::max();
    else
      return (Out)val;
  }
};
// Float -> Float
template <class In, class Out>
class MixedDataClamper<In, Out, false, false> {
 public:
  inline static Out Clamp(In val) { return (Out)val; }
};



// ****************************************************************************
// ***  DataClamper - most generic of *Clamper templates
// ***  Just defers to greater specialized versions
// ****************************************************************************
template <class In, class Out>
class DataClamper :
    public MixedDataClamper<In, Out,
                            std::numeric_limits<In>::is_integer,
                            std::numeric_limits<Out>::is_integer> {
};

// same type for both in and out, just pass it through
template <class InOut>
class DataClamper<InOut, InOut> {
 public:
  inline static InOut Clamp(InOut val) { return val; }
};



// ****************************************************************************
// ***  ClampRange
// ***
// ***  Clamp the input to the specified range using as few "if's" as possible
// ***
// ***  usage:
// ***  short  s = -1233;
// ***  uint32 i = 34000;
// ***  uint8  b = 145;
// ***  float  f = -134.567;
// ***  uint8  r1 = ClampRange<uint8>(s);
// ***  uint16 r2 = ClampRange<uint16>(i);
// ***  uint8  r3 = ClampRange<uint8>(b);
// ***  int16  r4 = ClampRange<int16>(f);
// ***  etc...
// ****************************************************************************
template <class Out, class In>
inline Out ClampRange(In v) {
  return DataClamper<In, Out>::Clamp(v);
}


#endif /* __khCalcHelper_h */

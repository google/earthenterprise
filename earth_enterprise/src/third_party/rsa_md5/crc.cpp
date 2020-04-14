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


// Implementation of CRCs (aka Rabin Fingerprints).
// Treats the input as a polynomial with coefficients in Z(2),
// and finds the remainder when divided by an irreducible polynomial
// of the appropriate length.
// It handles all CRC sizes from 8 to 128 bits.
// It's somewhat complicated by having separate implementations optimized for
// CRC's <=32 bits, <= 64 bits, and <= 128 bits.
// The input string is prefixed with a "1" bit, and has "degree" "0" bits
// appended to it before the remainder is found.   This ensures that
// short strings are scrambled somewhat and that strings consisting
// of all nulls have a non-zero CRC.

#include <stddef.h>
#include <notify.h>
#include <khThread.h>                   // for khMutex
#include "crc.h"

#define CHECK(a) \
  { if (!(a)) { \
      notify(NFY_FATAL, "FAILED at %s:%d" \
      " CHECK(" #a ")\n", __FILE__, __LINE__); \
      exit(2);\
    } \
  }

#if defined(__i386__) || defined(__x86_64__)
static const bool kNeedAlignedLoads = false;
#else
static const bool kNeedAlignedLoads = true;
#endif

static const int SMALL_BITS = 8;
                    // When extending an input with a string of zeroes,
                    // if the number of zeroes is less than 2**SMALL_BITS,
                    // a normal Extend is done, rather than a polynomial
                    // multiplication.
static const char zeroes[1 << SMALL_BITS] = { 0 };  // an array of zeroes

static const std::uint8_t *zero_ptr = 0;   // The 0 pointer---used for alignment

// Used to fetch a correctly-aligned 32-bit word in correct byte-order
static inline std::uint32_t WORD(const std::uint8_t *p) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  return *reinterpret_cast<const std::uint32_t *>(p);
#else
  std::uint32_t x = *reinterpret_cast<const std::uint32_t *>(p);
  return (x >> 24) | (x << 24) | ((x >> 8) & 0xff00) | ((x & 0xff00) << 8);
#endif
}
// These are used to index a 2-entry array of words that together
// for a longer integer.  LO indexes the low-order half.
static const int LO = (__BYTE_ORDER != __LITTLE_ENDIAN);
static const int HI = 1-LO;

// Constructor and destructor for baseclase CRC.
CRC::~CRC() {}
CRC::CRC() {}

struct CRC_pair {             // Used to represent a 128-bit value
  std::uint64_t lo;
  std::uint64_t hi;
};

class CRCImpl : public CRC {    // Implemention of the abstract class CRC
 public:
  CRCImpl() {};
  virtual ~CRCImpl() { CHECK(!this->is_default_); };

  // The internal version of CRC::New().
  static CRCImpl *NewInternal(std::uint64_t lo, std::uint64_t hi,
                              int degree, size_t roll_length);

  virtual void Empty(std::uint64_t *lo, std::uint64_t *hi) const;

  bool is_default_;       // This CRC is one of the default CRCs
  CRCImpl *next_;          // next entry in cache
  size_t roll_length_;    // length of window in rolling CRC
  int degree_;            // bits in the CRC
  std::uint64_t poly_lo_;        // The CRC of the empty string, low part
  std::uint64_t poly_hi_;        // The CRC of the empty string, high part

 private:
  DISALLOW_COPY_AND_ASSIGN(CRCImpl);
};

// This is the 32-bit implementation.  It handles all sizes from 8 to 32.
class CRC32 : public CRCImpl {
 public:
  CRC32() {}
  virtual ~CRC32() {}

  virtual void Extend(std::uint64_t *lo, std::uint64_t *hi,
                      const void *bytes, size_t length) const;
  virtual void ExtendByZeroes(std::uint64_t *lo, std::uint64_t *hi, size_t length) const;
  virtual void Roll(std::uint64_t *lo, std::uint64_t *hi, std::uint8_t o_byte, std::uint8_t i_byte) const;

  std::uint32_t table0_[256];  // table of byte extensions
  std::uint32_t table1_[256];  // table of byte extensions, shifted by 1 byte
  std::uint32_t table2_[256];  // table of byte extensions, shifted by 2 bytes
  std::uint32_t table3_[256];  // table of byte extensions, shifted by 3 bytes
  std::uint32_t roll_[256];    // table of byte roll values
  std::uint32_t zeroes_[256];  // table of zero extensions

 private:
  DISALLOW_COPY_AND_ASSIGN(CRC32);
};

// This is the 64-bit implementation.  It handles all sizes from 33 to 64.
class CRC64 : public CRCImpl {
 public:
  CRC64() {};
  virtual ~CRC64() {};

  virtual void Extend(std::uint64_t *lo, std::uint64_t *hi,
                      const void *bytes, size_t length) const;
  virtual void ExtendByZeroes(std::uint64_t *lo, std::uint64_t *hi, size_t length) const;
  virtual void Roll(std::uint64_t *lo, std::uint64_t *hi, std::uint8_t o_byte, std::uint8_t i_byte) const;

  std::uint64_t table0_[256];  // table of byte extensions
  std::uint64_t table1_[256];  // table of byte extensions, shifted by 1 byte
  std::uint64_t table2_[256];  // table of byte extensions, shifted by 2 bytes
  std::uint64_t table3_[256];  // table of byte extensions, shifted by 3 bytes
  std::uint64_t roll_[256];    // table of byte roll values
  std::uint64_t zeroes_[256];  // table of zero extensions

 private:
  DISALLOW_COPY_AND_ASSIGN(CRC64);
};

// This is the 128-bit implementation.  It handles all sizes from 65 to 128.
class CRC128 : public CRCImpl {
 public:
  CRC128() {};
  virtual ~CRC128() {};

  virtual void Extend(std::uint64_t *lo, std::uint64_t *hi,
                      const void *bytes, size_t length) const;
  virtual void ExtendByZeroes(std::uint64_t *lo, std::uint64_t *hi, size_t length) const;
  virtual void Roll(std::uint64_t *lo, std::uint64_t *hi, std::uint8_t o_byte, std::uint8_t i_byte) const;

  struct CRC_pair table0_[256]; // table of byte extensions
  struct CRC_pair table1_[256]; // table of byte extensions, shifted by 1 byte
  struct CRC_pair table2_[256]; // table of byte extensions, shifted by 2 bytes
  struct CRC_pair table3_[256]; // table of byte extensions, shifted by 3 bytes
  struct CRC_pair roll_[256];   // table of byte roll values
  struct CRC_pair zeroes_[256]; // table of zero extensions

 private:
  DISALLOW_COPY_AND_ASSIGN(CRC128);
};

static const std::uint64_t UINT64_ZERO = 0;    // a 64-bit zero
static const std::uint64_t UINT64_ONE = 1;     // a 64-bit 1

// The B() macro sets the bit corresponding to X**(_x) in the polynomial
#define B(_x) (UINT64_ONE << ((_x) < 64?(63-(_x)):(127-(_x))))

// Used to initialize polynomials.
// The redundant tests on _len are to avoid warnings from the
// compiler about inappropriate shift lengths.   These shifts
// occur on not-taken branch of the ?: in some cases.
#define DEF_POLY(_h,_l,_len) \
  { ((_len) <= 64  ? (_l) >> ((_len) <= 64? 64 - (_len): 0) : \
     (_len) == 128 ? (_h) : \
                      ((_h) >> ((_len) > 64? 128 - (_len): 0)) | \
                      ((_l) << ((_len) > 64 && (_len) < 128? (_len)-64: 0))), \
    ((_len) <= 64  ? 0 : \
     (_len) == 128 ? (_l) :  \
                     (_l) >> ((_len) > 64? 128 - (_len): 0)), \
    (_len) }

// A table of irreducible polynomials suitable for use with the implementation.
// Indexes 0...1 have degree 32 polynomials.
// Indexes 2...3 have degree 64 polynomials.
// Indexes 4...5 have degree 96 polynomials.
// Indexes 6...7 have degree 128 polynomials.
// Index i=8...128 has a degree i polynomial.
// All polynomials in the table are guaranteed distinct.
static const struct CRC::Poly poly_list[] = {
  DEF_POLY(UINT64_ZERO, B(30)+B(27)+B(26)+B(25)+B(23)+B(20)+B(17)+B(15)+B(14)+
           B(12)+B(6)+B(5)+B(2)+B(0), 32),
  DEF_POLY(UINT64_ZERO, B(31)+B(28)+B(27)+B(26)+B(24)+B(22)+B(19)+B(18)+B(16)+
           B(13)+B(11)+B(10)+B(9)+B(4)+B(2)+B(0), 32),
  DEF_POLY(UINT64_ZERO, B(60)+B(59)+B(58)+B(56)+B(55)+B(54)+B(51)+B(50)+B(49)+
           B(48)+B(47)+B(45)+B(44)+B(42)+B(40)+B(39)+B(38)+B(36)+B(34)+B(33)+
           B(32)+B(31)+B(30)+B(27)+B(25)+B(23)+B(22)+B(21)+B(20)+B(19)+
           B(17)+B(16)+B(15)+B(8)+B(7)+B(6)+B(5)+B(0), 64),
  DEF_POLY(UINT64_ZERO, B(63)+B(62)+B(60)+B(58)+B(57)+B(56)+B(54)+B(52)+B(46)+
           B(45)+B(43)+B(40)+B(37)+B(36)+B(34)+B(33)+B(32)+B(31)+B(30)+B(29)+
           B(28)+B(27)+B(26)+B(23)+B(19)+B(18)+B(15)+B(14)+B(13)+B(9)+B(8)+
           B(0), 64),
  DEF_POLY(B(95)+B(94)+B(91)+B(90)+B(89)+B(88)+B(87)+B(86)+B(79)+B(78)+
           B(77)+B(76)+B(75)+B(74)+B(73)+B(69)+B(68)+B(66), B(63)+B(61)+
           B(59)+B(57)+B(53)+B(51)+B(50)+B(47)+B(40)+B(39)+B(38)+B(36)+
           B(35)+B(33)+B(29)+B(28)+B(27)+B(25)+B(24)+B(23)+B(21)+B(19)+
           B(18)+B(17)+B(16)+B(13)+B(12)+B(10)+B(9)+B(7)+B(4)+B(2)+B(1)+
           B(0), 96),
  DEF_POLY(B(95)+B(92)+B(89)+B(88)+B(87)+B(85)+B(84)+B(82)+B(81)+B(80)+
           B(79)+B(78)+B(76)+B(75)+B(70)+B(69)+B(66)+B(65), B(60)+B(56)+
           B(55)+B(52)+B(51)+B(49)+B(48)+B(46)+B(44)+B(42)+B(41)+B(39)+
           B(38)+B(37)+B(35)+B(33)+B(32)+B(30)+B(28)+B(27)+B(25)+B(22)+
           B(19)+B(17)+B(14)+B(12)+B(10)+B(0), 96),
  DEF_POLY(B(122)+B(121)+B(120)+B(119)+B(117)+B(116)+B(114)+B(113)+B(112)+
           B(111)+B(109)+B(107)+B(104)+B(102)+B(100)+B(98)+B(96)+B(94)+
           B(93)+B(92)+B(91)+B(90)+B(88)+B(87)+B(86)+B(84)+B(82)+B(80)+
           B(75)+B(74)+B(73)+B(69), B(62)+B(61)+B(58)+B(52)+B(48)+B(47)+
           B(46)+B(45)+B(42)+B(41)+B(38)+B(37)+B(35)+B(33)+B(32)+B(31)+
           B(30)+B(28)+B(26)+B(24)+B(22)+B(21)+B(20)+B(19)+B(18)+B(17)+
           B(10)+B(9)+B(8)+B(7)+B(5)+B(2)+B(1)+B(0), 128),
  DEF_POLY(B(127)+B(126)+B(124)+B(121)+B(117)+B(116)+B(115)+B(113)+B(112)+
           B(111)+B(108)+B(105)+B(104)+B(103)+B(100)+B(98)+B(96)+B(93)+
           B(92)+B(90)+B(89)+B(88)+B(86)+B(85)+B(80)+B(77)+B(76)+B(72)+
           B(70)+B(69)+B(68)+B(65)+B(64), B(62)+B(61)+B(59)+B(58)+B(56)+
           B(53)+B(52)+B(51)+B(50)+B(48)+B(46)+B(39)+B(35)+B(34)+B(33)+
           B(32)+B(30)+B(29)+B(28)+B(22)+B(21)+B(19)+B(18)+B(17)+B(14)+
           B(10)+B(9)+B(7)+B(5)+B(4)+B(3)+B(2)+B(0), 128),
  DEF_POLY(UINT64_ZERO, B(7)+B(6)+B(5)+B(4)+B(2)+B(0), 8),
  DEF_POLY(UINT64_ZERO, B(8)+B(4)+B(3)+B(2)+B(1)+B(0), 9),
  DEF_POLY(UINT64_ZERO, B(8)+B(6)+B(5)+B(3)+B(1)+B(0), 10),
  DEF_POLY(UINT64_ZERO, B(10)+B(9)+B(7)+B(5)+B(1)+B(0), 11),
  DEF_POLY(UINT64_ZERO, B(11)+B(10)+B(5)+B(2)+B(1)+B(0), 12),
  DEF_POLY(UINT64_ZERO, B(12)+B(11)+B(10)+B(8)+B(5)+B(3)+B(2)+B(0), 13),
  DEF_POLY(UINT64_ZERO, B(11)+B(10)+B(9)+B(8)+B(7)+B(5)+B(4)+B(2)+B(1)+
           B(0), 14),
  DEF_POLY(UINT64_ZERO, B(14)+B(12)+B(11)+B(10)+B(9)+B(5)+B(3)+B(2)+B(1)+
           B(0), 15),
  DEF_POLY(UINT64_ZERO, B(12)+B(11)+B(7)+B(6)+B(5)+B(4)+B(3)+B(0), 16),
  DEF_POLY(UINT64_ZERO, B(16)+B(14)+B(11)+B(10)+B(8)+B(3)+B(1)+B(0), 17),
  DEF_POLY(UINT64_ZERO, B(12)+B(11)+B(9)+B(8)+B(7)+B(4)+B(3)+B(0), 18),
  DEF_POLY(UINT64_ZERO, B(12)+B(11)+B(8)+B(7)+B(6)+B(5)+B(2)+B(0), 19),
  DEF_POLY(UINT64_ZERO, B(18)+B(15)+B(14)+B(12)+B(9)+B(6)+B(3)+B(0), 20),
  DEF_POLY(UINT64_ZERO, B(20)+B(19)+B(14)+B(13)+B(12)+B(11)+B(8)+B(7)+B(6)+
           B(5)+B(2)+B(0), 21),
  DEF_POLY(UINT64_ZERO, B(21)+B(20)+B(18)+B(16)+B(15)+B(14)+B(12)+B(9)+B(7)+
           B(2)+B(1)+B(0), 22),
  DEF_POLY(UINT64_ZERO, B(22)+B(21)+B(17)+B(16)+B(15)+B(14)+B(12)+B(10)+B(7)+
           B(4)+B(1)+B(0), 23),
  DEF_POLY(UINT64_ZERO, B(23)+B(22)+B(21)+B(18)+B(17)+B(15)+B(14)+B(12)+B(4)+
           B(0), 24),
  DEF_POLY(UINT64_ZERO, B(24)+B(23)+B(22)+B(20)+B(18)+B(17)+B(14)+B(13)+B(9)+
           B(0), 25),
  DEF_POLY(UINT64_ZERO, B(25)+B(22)+B(21)+B(19)+B(17)+B(15)+B(14)+B(12)+B(11)+
           B(10)+B(6)+B(4)+B(3)+B(0), 26),
  DEF_POLY(UINT64_ZERO, B(26)+B(25)+B(19)+B(17)+B(16)+B(13)+B(5)+B(4)+B(1)+
           B(0), 27),
  DEF_POLY(UINT64_ZERO, B(23)+B(22)+B(21)+B(20)+B(19)+B(18)+B(13)+B(12)+B(10)+
           B(9)+B(8)+B(6)+B(5)+B(3)+B(1)+B(0), 28),
  DEF_POLY(UINT64_ZERO, B(27)+B(26)+B(25)+B(23)+B(22)+B(20)+B(19)+B(15)+B(14)+
           B(11)+B(10)+B(8)+B(7)+B(6)+B(4)+B(0), 29),
  DEF_POLY(UINT64_ZERO, B(29)+B(27)+B(25)+B(23)+B(20)+B(19)+B(18)+B(17)+B(16)+
           B(14)+B(11)+B(10)+B(9)+B(7)+B(6)+B(5)+B(4)+B(0), 30),
  DEF_POLY(UINT64_ZERO, B(30)+B(29)+B(28)+B(27)+B(25)+B(23)+B(22)+B(21)+B(20)+
           B(19)+B(18)+B(16)+B(15)+B(10)+B(9)+B(8)+B(4)+B(3)+B(1)+B(0), 31),
  DEF_POLY(UINT64_ZERO, B(31)+B(29)+B(28)+B(27)+B(21)+B(20)+B(15)+B(13)+B(10)+
           B(9)+B(8)+B(7)+B(4)+B(3)+B(2)+B(0), 32),
  DEF_POLY(UINT64_ZERO, B(32)+B(31)+B(30)+B(29)+B(27)+B(25)+B(24)+B(22)+B(21)+
           B(19)+B(15)+B(10)+B(4)+B(3)+B(2)+B(0), 33),
  DEF_POLY(UINT64_ZERO, B(30)+B(27)+B(26)+B(25)+B(24)+B(20)+B(19)+B(18)+B(16)+
           B(15)+B(14)+B(12)+B(9)+B(8)+B(7)+B(5)+B(1)+B(0), 34),
  DEF_POLY(UINT64_ZERO, B(34)+B(32)+B(28)+B(27)+B(26)+B(22)+B(21)+B(20)+B(19)+
           B(14)+B(13)+B(12)+B(10)+B(6)+B(5)+B(4)+B(3)+B(0), 35),
  DEF_POLY(UINT64_ZERO, B(35)+B(34)+B(33)+B(32)+B(31)+B(28)+B(26)+B(24)+B(22)+
           B(21)+B(20)+B(19)+B(18)+B(14)+B(13)+B(12)+B(10)+B(9)+B(8)+B(6)+B(5)+
           B(4)+B(3)+B(0), 36),
  DEF_POLY(UINT64_ZERO, B(36)+B(35)+B(31)+B(30)+B(28)+B(26)+B(25)+B(23)+B(22)+
           B(20)+B(19)+B(18)+B(16)+B(13)+B(12)+B(7)+B(6)+B(4)+B(3)+B(0), 37),
  DEF_POLY(UINT64_ZERO, B(37)+B(34)+B(33)+B(32)+B(31)+B(30)+B(29)+B(28)+B(27)+
           B(26)+B(25)+B(23)+B(18)+B(16)+B(15)+B(13)+B(12)+B(11)+B(10)+B(9)+
           B(8)+B(7)+B(6)+B(2)+B(1)+B(0), 38),
  DEF_POLY(UINT64_ZERO, B(38)+B(37)+B(33)+B(32)+B(31)+B(27)+B(25)+B(24)+B(21)+
           B(20)+B(19)+B(18)+B(17)+B(15)+B(14)+B(8)+B(7)+B(6)+B(3)+B(2)+B(1)+
           B(0), 39),
  DEF_POLY(UINT64_ZERO, B(38)+B(37)+B(35)+B(34)+B(32)+B(31)+B(30)+B(27)+B(24)+
           B(21)+B(20)+B(14)+B(13)+B(11)+B(8)+B(4)+B(2)+B(0), 40),
  DEF_POLY(UINT64_ZERO, B(38)+B(36)+B(35)+B(34)+B(33)+B(31)+B(30)+B(29)+B(28)+
           B(27)+B(23)+B(22)+B(20)+B(19)+B(18)+B(17)+B(15)+B(14)+B(11)+B(5)+
           B(4)+B(0), 41),
  DEF_POLY(UINT64_ZERO, B(41)+B(37)+B(36)+B(35)+B(32)+B(31)+B(30)+B(29)+B(28)+
           B(25)+B(19)+B(18)+B(14)+B(13)+B(12)+B(7)+B(6)+B(4)+B(2)+B(0), 42),
  DEF_POLY(UINT64_ZERO, B(42)+B(40)+B(38)+B(37)+B(36)+B(35)+B(34)+B(33)+B(31)+
           B(29)+B(27)+B(26)+B(25)+B(23)+B(21)+B(20)+B(19)+B(15)+B(11)+B(10)+
           B(9)+B(8)+B(6)+B(5)+B(3)+B(0), 43),
  DEF_POLY(UINT64_ZERO, B(43)+B(42)+B(40)+B(39)+B(37)+B(35)+B(32)+B(30)+B(26)+
           B(25)+B(24)+B(20)+B(16)+B(13)+B(12)+B(11)+B(8)+B(6)+B(5)+B(4)+B(1)+
           B(0), 44),
  DEF_POLY(UINT64_ZERO, B(43)+B(42)+B(41)+B(40)+B(39)+B(38)+B(33)+B(32)+B(27)+
           B(26)+B(25)+B(23)+B(20)+B(18)+B(17)+B(16)+B(14)+B(11)+B(10)+B(9)+
           B(6)+B(5)+B(1)+B(0), 45),
  DEF_POLY(UINT64_ZERO, B(45)+B(43)+B(42)+B(41)+B(40)+B(39)+B(32)+B(31)+B(30)+
           B(29)+B(27)+B(25)+B(23)+B(18)+B(17)+B(16)+B(10)+B(9)+B(7)+B(6)+B(4)+
           B(3)+B(2)+B(0), 46),
  DEF_POLY(UINT64_ZERO, B(45)+B(44)+B(43)+B(41)+B(40)+B(39)+B(38)+B(37)+B(32)+
           B(30)+B(23)+B(21)+B(20)+B(17)+B(15)+B(13)+B(11)+B(10)+B(7)+B(5)+
           B(3)+B(0), 47),
  DEF_POLY(UINT64_ZERO, B(46)+B(42)+B(41)+B(39)+B(37)+B(36)+B(35)+B(29)+B(28)+
           B(25)+B(24)+B(21)+B(20)+B(18)+B(17)+B(13)+B(12)+B(11)+B(10)+B(9)+
           B(8)+B(5)+B(1)+B(0), 48),
  DEF_POLY(UINT64_ZERO, B(48)+B(44)+B(41)+B(40)+B(39)+B(38)+B(37)+B(36)+B(35)+
           B(34)+B(30)+B(28)+B(27)+B(24)+B(21)+B(18)+B(17)+B(8)+B(3)+B(0), 49),
  DEF_POLY(UINT64_ZERO, B(48)+B(47)+B(46)+B(45)+B(44)+B(43)+B(42)+B(35)+B(33)+
           B(29)+B(26)+B(24)+B(23)+B(21)+B(18)+B(16)+B(14)+B(13)+B(12)+B(9)+
           B(7)+B(6)+B(5)+B(4)+B(3)+B(0), 50),
  DEF_POLY(UINT64_ZERO, B(47)+B(46)+B(45)+B(44)+B(43)+B(40)+B(39)+B(38)+B(36)+
           B(35)+B(30)+B(29)+B(28)+B(26)+B(25)+B(24)+B(23)+B(22)+B(20)+B(19)+
           B(18)+B(17)+B(15)+B(11)+B(7)+B(4)+B(3)+B(0), 51),
  DEF_POLY(UINT64_ZERO, B(51)+B(46)+B(43)+B(38)+B(37)+B(36)+B(34)+B(31)+B(27)+
           B(26)+B(20)+B(17)+B(16)+B(15)+B(13)+B(12)+B(11)+B(9)+B(7)+B(5)+B(1)+
           B(0), 52),
  DEF_POLY(UINT64_ZERO, B(50)+B(49)+B(47)+B(46)+B(44)+B(42)+B(41)+B(37)+B(36)+
           B(35)+B(33)+B(29)+B(28)+B(26)+B(24)+B(23)+B(21)+B(20)+B(14)+B(13)+
           B(12)+B(11)+B(10)+B(9)+B(8)+B(6)+B(3)+B(2)+B(1)+B(0), 53),
  DEF_POLY(UINT64_ZERO, B(52)+B(47)+B(46)+B(44)+B(43)+B(42)+B(40)+B(36)+B(32)+
           B(31)+B(30)+B(29)+B(28)+B(26)+B(25)+B(24)+B(23)+B(22)+B(20)+B(19)+
           B(17)+B(16)+B(15)+B(14)+B(13)+B(12)+B(11)+B(10)+B(7)+B(4)+B(2)+
           B(0), 54),
  DEF_POLY(UINT64_ZERO, B(53)+B(50)+B(48)+B(47)+B(37)+B(35)+B(31)+B(30)+B(25)+
           B(22)+B(21)+B(20)+B(19)+B(18)+B(15)+B(10)+B(8)+B(6)+B(3)+B(2)+B(1)+
           B(0), 55),
  DEF_POLY(UINT64_ZERO, B(54)+B(52)+B(51)+B(49)+B(48)+B(42)+B(38)+B(37)+B(31)+
           B(30)+B(27)+B(26)+B(24)+B(23)+B(22)+B(19)+B(16)+B(12)+B(11)+B(8)+
           B(6)+B(4)+B(3)+B(0), 56),
  DEF_POLY(UINT64_ZERO, B(55)+B(54)+B(51)+B(49)+B(48)+B(47)+B(46)+B(44)+B(43)+
           B(42)+B(41)+B(40)+B(39)+B(38)+B(32)+B(29)+B(27)+B(26)+B(23)+B(21)+
           B(20)+B(15)+B(12)+B(7)+B(6)+B(5)+B(3)+B(0), 57),
  DEF_POLY(UINT64_ZERO, B(57)+B(54)+B(52)+B(47)+B(45)+B(42)+B(41)+B(40)+B(39)+
           B(36)+B(34)+B(33)+B(31)+B(28)+B(26)+B(21)+B(20)+B(18)+B(17)+B(16)+
           B(13)+B(11)+B(8)+B(7)+B(4)+B(2)+B(1)+B(0), 58),
  DEF_POLY(UINT64_ZERO, B(58)+B(56)+B(54)+B(49)+B(47)+B(46)+B(43)+B(40)+B(38)+
           B(36)+B(35)+B(33)+B(32)+B(31)+B(30)+B(27)+B(24)+B(22)+B(21)+B(19)+
           B(17)+B(16)+B(11)+B(10)+B(9)+B(8)+B(7)+B(4)+B(3)+B(2)+B(1)+B(0),
           59),
  DEF_POLY(UINT64_ZERO, B(56)+B(54)+B(51)+B(46)+B(43)+B(42)+B(40)+B(39)+B(37)+
           B(35)+B(34)+B(33)+B(32)+B(31)+B(30)+B(29)+B(27)+B(25)+B(22)+B(21)+
           B(20)+B(19)+B(17)+B(16)+B(15)+B(14)+B(13)+B(12)+B(9)+B(7)+B(4)+
           B(3)+B(1)+B(0), 60),
  DEF_POLY(UINT64_ZERO, B(59)+B(58)+B(57)+B(56)+B(54)+B(53)+B(50)+B(49)+B(47)+
           B(44)+B(42)+B(41)+B(40)+B(37)+B(35)+B(34)+B(32)+B(30)+B(29)+B(27)+
           B(26)+B(22)+B(21)+B(20)+B(17)+B(14)+B(13)+B(12)+B(8)+B(5)+B(4)+
           B(0), 61),
  DEF_POLY(UINT64_ZERO, B(61)+B(59)+B(57)+B(55)+B(54)+B(53)+B(52)+B(51)+B(50)+
           B(49)+B(48)+B(45)+B(44)+B(40)+B(37)+B(35)+B(32)+B(31)+B(29)+B(25)+
           B(24)+B(23)+B(20)+B(17)+B(16)+B(15)+B(13)+B(12)+B(11)+B(10)+B(6)+
           B(5)+B(2)+B(0), 62),
  DEF_POLY(UINT64_ZERO, B(62)+B(57)+B(56)+B(53)+B(52)+B(51)+B(50)+B(46)+B(41)+
           B(38)+B(35)+B(34)+B(33)+B(31)+B(27)+B(25)+B(23)+B(21)+B(19)+B(18)+
           B(17)+B(16)+B(13)+B(11)+B(7)+B(5)+B(1)+B(0), 63),
  DEF_POLY(UINT64_ZERO, B(62)+B(61)+B(60)+B(57)+B(55)+B(54)+B(53)+B(49)+B(48)+
           B(46)+B(44)+B(42)+B(40)+B(39)+B(37)+B(36)+B(28)+B(27)+B(25)+B(23)+
           B(22)+B(21)+B(17)+B(15)+B(13)+B(7)+B(6)+B(4)+B(2)+B(0), 64),
  DEF_POLY(UINT64_ZERO, B(63)+B(62)+B(59)+B(57)+B(54)+B(53)+B(51)+B(48)+
           B(47)+B(46)+B(45)+B(44)+B(41)+B(40)+B(38)+B(36)+B(35)+B(28)+
           B(25)+B(24)+B(21)+B(20)+B(18)+B(16)+B(15)+B(13)+B(11)+B(8)+B(7)+
           B(3)+B(1)+B(0), 65),
  DEF_POLY(UINT64_ZERO, B(63)+B(58)+B(57)+B(56)+B(52)+B(51)+B(50)+B(44)+
           B(41)+B(40)+B(36)+B(34)+B(32)+B(31)+B(27)+B(25)+B(23)+B(21)+
           B(20)+B(19)+B(18)+B(17)+B(15)+B(14)+B(12)+B(11)+B(10)+B(8)+B(5)+
           B(4)+B(3)+B(0), 66),
  DEF_POLY(B(66), B(62)+B(60)+B(59)+B(58)+B(57)+B(56)+B(55)+B(54)+B(52)+
           B(50)+B(47)+B(46)+B(45)+B(43)+B(42)+B(41)+B(38)+B(37)+B(36)+
           B(33)+B(32)+B(31)+B(30)+B(28)+B(27)+B(26)+B(24)+B(21)+B(18)+
           B(17)+B(14)+B(13)+B(12)+B(11)+B(10)+B(7)+B(4)+B(3)+B(0), 67),
  DEF_POLY(B(67)+B(66), B(63)+B(61)+B(57)+B(55)+B(51)+B(47)+B(45)+B(43)+
           B(42)+B(41)+B(40)+B(39)+B(32)+B(31)+B(30)+B(28)+B(27)+B(25)+
           B(19)+B(18)+B(17)+B(15)+B(11)+B(9)+B(8)+B(7)+B(6)+B(5)+B(4)+B(3)+
           B(1)+B(0), 68),
  DEF_POLY(B(68), B(60)+B(57)+B(55)+B(54)+B(52)+B(50)+B(49)+B(48)+B(44)+
           B(40)+B(38)+B(37)+B(33)+B(31)+B(28)+B(25)+B(22)+B(21)+B(20)+
           B(19)+B(18)+B(17)+B(13)+B(12)+B(9)+B(8)+B(6)+B(5)+B(4)+B(1)+
           B(0), 69),
  DEF_POLY(B(69)+B(68)+B(67)+B(66), B(63)+B(62)+B(61)+B(59)+B(51)+B(49)+
           B(48)+B(46)+B(45)+B(42)+B(40)+B(38)+B(36)+B(35)+B(33)+B(32)+
           B(30)+B(29)+B(27)+B(23)+B(22)+B(21)+B(16)+B(12)+B(5)+B(4)+B(1)+
           B(0), 70),
  DEF_POLY(B(70)+B(69)+B(68)+B(64), B(63)+B(62)+B(61)+B(60)+B(59)+B(57)+
           B(56)+B(55)+B(54)+B(53)+B(51)+B(50)+B(47)+B(44)+B(43)+B(41)+
           B(39)+B(37)+B(36)+B(33)+B(32)+B(26)+B(25)+B(24)+B(23)+B(21)+
           B(20)+B(19)+B(17)+B(12)+B(11)+B(10)+B(8)+B(6)+B(5)+B(4)+B(2)+
           B(0), 71),
  DEF_POLY(B(71)+B(69)+B(68)+B(65)+B(64), B(62)+B(61)+B(59)+B(58)+B(55)+
           B(53)+B(51)+B(49)+B(48)+B(47)+B(43)+B(40)+B(38)+B(37)+B(36)+
           B(35)+B(33)+B(32)+B(31)+B(30)+B(29)+B(26)+B(24)+B(19)+B(18)+
           B(15)+B(13)+B(9)+B(7)+B(6)+B(3)+B(1)+B(0), 72),
  DEF_POLY(B(71)+B(70)+B(69)+B(67)+B(65), B(63)+B(62)+B(61)+B(58)+B(57)+
           B(56)+B(55)+B(52)+B(51)+B(50)+B(49)+B(46)+B(45)+B(44)+B(43)+
           B(41)+B(37)+B(36)+B(34)+B(33)+B(27)+B(26)+B(25)+B(21)+B(19)+
           B(18)+B(16)+B(15)+B(14)+B(13)+B(9)+B(8)+B(6)+B(5)+B(2)+B(1)+
           B(0), 73),
  DEF_POLY(B(73)+B(71)+B(70)+B(65)+B(64), B(62)+B(60)+B(55)+B(54)+B(52)+
           B(50)+B(48)+B(47)+B(46)+B(44)+B(41)+B(40)+B(31)+B(29)+B(28)+
           B(27)+B(26)+B(24)+B(23)+B(22)+B(20)+B(16)+B(12)+B(9)+B(6)+B(5)+
           B(4)+B(2)+B(0), 74),
  DEF_POLY(B(74)+B(73)+B(72)+B(67)+B(64), B(63)+B(61)+B(60)+B(58)+B(57)+
           B(56)+B(54)+B(52)+B(51)+B(50)+B(44)+B(43)+B(42)+B(41)+B(40)+
           B(39)+B(38)+B(36)+B(35)+B(33)+B(32)+B(31)+B(29)+B(28)+B(26)+
           B(23)+B(21)+B(19)+B(18)+B(16)+B(15)+B(13)+B(12)+B(11)+B(7)+B(6)+
           B(5)+B(4)+B(3)+B(2)+B(0), 75),
  DEF_POLY(B(75)+B(74)+B(71)+B(70)+B(66), B(63)+B(61)+B(59)+B(57)+B(53)+
           B(50)+B(49)+B(48)+B(44)+B(43)+B(42)+B(37)+B(33)+B(30)+B(27)+
           B(24)+B(23)+B(20)+B(18)+B(15)+B(12)+B(11)+B(9)+B(7)+B(6)+B(4)+
           B(3)+B(2)+B(0), 76),
  DEF_POLY(B(73)+B(71)+B(70)+B(68)+B(67)+B(66)+B(65), B(63)+B(60)+B(59)+
           B(58)+B(57)+B(54)+B(49)+B(47)+B(46)+B(45)+B(43)+B(41)+B(38)+
           B(34)+B(33)+B(31)+B(30)+B(29)+B(27)+B(25)+B(24)+B(21)+B(20)+
           B(19)+B(16)+B(15)+B(14)+B(13)+B(10)+B(8)+B(6)+B(5)+B(4)+B(2)+
           B(0), 77),
  DEF_POLY(B(77)+B(76)+B(75)+B(74)+B(70)+B(66)+B(65)+B(64), B(63)+B(62)+
           B(60)+B(58)+B(57)+B(55)+B(52)+B(51)+B(44)+B(41)+B(39)+B(38)+
           B(35)+B(31)+B(30)+B(29)+B(26)+B(22)+B(21)+B(20)+B(19)+B(15)+
           B(13)+B(11)+B(6)+B(4)+B(1)+B(0), 78),
  DEF_POLY(B(78)+B(76)+B(75)+B(71)+B(68)+B(67)+B(65), B(63)+B(61)+B(60)+
           B(55)+B(54)+B(51)+B(50)+B(48)+B(44)+B(42)+B(41)+B(40)+B(38)+
           B(35)+B(34)+B(32)+B(28)+B(26)+B(23)+B(22)+B(19)+B(15)+B(13)+
           B(12)+B(8)+B(7)+B(5)+B(2)+B(0), 79),
  DEF_POLY(B(77)+B(76)+B(75)+B(73)+B(70)+B(66), B(63)+B(61)+B(60)+B(59)+
           B(56)+B(54)+B(53)+B(52)+B(50)+B(44)+B(43)+B(40)+B(39)+B(38)+
           B(35)+B(34)+B(33)+B(29)+B(28)+B(27)+B(26)+B(25)+B(24)+B(23)+
           B(22)+B(21)+B(20)+B(18)+B(16)+B(13)+B(12)+B(11)+B(10)+B(8)+B(7)+
           B(6)+B(3)+B(2)+B(1)+B(0), 80),
  DEF_POLY(B(78)+B(77)+B(76)+B(75)+B(73)+B(71)+B(67)+B(66)+B(65)+
           B(64), B(61)+B(54)+B(53)+B(52)+B(49)+B(47)+B(44)+B(41)+B(40)+
           B(35)+B(33)+B(31)+B(30)+B(28)+B(27)+B(26)+B(25)+B(22)+B(21)+
           B(20)+B(16)+B(15)+B(13)+B(12)+B(11)+B(0), 81),
  DEF_POLY(B(81)+B(80)+B(79)+B(77)+B(76)+B(74)+B(73)+B(72)+B(68)+B(67)+
           B(66)+B(64), B(62)+B(51)+B(50)+B(49)+B(47)+B(46)+B(45)+B(43)+
           B(41)+B(38)+B(37)+B(34)+B(32)+B(30)+B(27)+B(26)+B(25)+B(24)+
           B(23)+B(22)+B(20)+B(19)+B(16)+B(15)+B(13)+B(12)+B(9)+B(7)+B(5)+
           B(4)+B(1)+B(0), 82),
  DEF_POLY(B(82)+B(81)+B(79)+B(78)+B(77)+B(75)+B(72)+B(71)+B(69)+B(68)+
           B(67)+B(66)+B(65)+B(64), B(60)+B(58)+B(57)+B(56)+B(53)+B(52)+
           B(51)+B(49)+B(48)+B(45)+B(43)+B(41)+B(40)+B(39)+B(38)+B(37)+
           B(36)+B(35)+B(33)+B(26)+B(24)+B(21)+B(19)+B(16)+B(13)+B(12)+
           B(11)+B(9)+B(7)+B(5)+B(4)+B(3)+B(1)+B(0), 83),
  DEF_POLY(B(79)+B(77)+B(73)+B(72)+B(71)+B(66)+B(64), B(62)+B(61)+B(59)+
           B(58)+B(57)+B(56)+B(53)+B(52)+B(51)+B(48)+B(47)+B(46)+B(45)+
           B(43)+B(42)+B(41)+B(38)+B(37)+B(35)+B(33)+B(32)+B(29)+B(24)+
           B(22)+B(17)+B(16)+B(15)+B(13)+B(11)+B(10)+B(9)+B(7)+B(6)+B(5)+
           B(0), 84),
  DEF_POLY(B(83)+B(78)+B(76)+B(73)+B(70)+B(69)+B(68)+B(67)+B(66)+
           B(64), B(62)+B(61)+B(60)+B(59)+B(54)+B(51)+B(50)+B(48)+B(47)+
           B(42)+B(41)+B(40)+B(38)+B(37)+B(36)+B(34)+B(31)+B(30)+B(28)+
           B(27)+B(26)+B(24)+B(22)+B(21)+B(20)+B(19)+B(18)+B(16)+B(15)+
           B(14)+B(13)+B(12)+B(10)+B(6)+B(4)+B(0), 85),
  DEF_POLY(B(84)+B(77)+B(76)+B(75)+B(71)+B(70)+B(69)+B(67)+B(65), B(63)+
           B(62)+B(59)+B(58)+B(57)+B(55)+B(53)+B(52)+B(51)+B(48)+B(47)+
           B(45)+B(43)+B(40)+B(38)+B(36)+B(34)+B(33)+B(31)+B(27)+B(25)+
           B(24)+B(23)+B(22)+B(19)+B(15)+B(13)+B(12)+B(11)+B(8)+B(6)+B(4)+
           B(0), 86),
  DEF_POLY(B(85)+B(84)+B(83)+B(81)+B(80)+B(78)+B(73)+B(72)+B(70)+B(68)+
           B(67)+B(64), B(61)+B(60)+B(58)+B(57)+B(55)+B(52)+B(50)+B(49)+
           B(47)+B(44)+B(37)+B(36)+B(35)+B(34)+B(32)+B(31)+B(30)+B(25)+
           B(24)+B(23)+B(20)+B(13)+B(12)+B(11)+B(10)+B(9)+B(7)+B(6)+B(4)+
           B(3)+B(2)+B(0), 87),
  DEF_POLY(B(86)+B(85)+B(84)+B(83)+B(82)+B(80)+B(77)+B(74)+B(70)+B(69)+
           B(65), B(63)+B(60)+B(59)+B(57)+B(56)+B(55)+B(53)+B(50)+B(49)+
           B(48)+B(45)+B(42)+B(41)+B(40)+B(39)+B(38)+B(37)+B(36)+B(25)+
           B(21)+B(19)+B(13)+B(11)+B(8)+B(5)+B(4)+B(2)+B(1)+B(0), 88),
  DEF_POLY(B(86)+B(85)+B(83)+B(82)+B(81)+B(78)+B(77)+B(74)+B(73)+B(72)+
           B(70)+B(69)+B(68)+B(65)+B(64), B(59)+B(57)+B(55)+B(54)+B(51)+
           B(50)+B(46)+B(45)+B(44)+B(43)+B(42)+B(40)+B(38)+B(37)+B(33)+
           B(31)+B(30)+B(29)+B(28)+B(27)+B(23)+B(22)+B(21)+B(20)+B(18)+
           B(17)+B(16)+B(15)+B(10)+B(9)+B(3)+B(1)+B(0), 89),
  DEF_POLY(B(86)+B(83)+B(82)+B(80)+B(79)+B(73)+B(70)+B(69)+B(67)+
           B(64), B(63)+B(62)+B(61)+B(57)+B(56)+B(54)+B(51)+B(49)+B(47)+
           B(46)+B(45)+B(40)+B(39)+B(37)+B(35)+B(33)+B(32)+B(29)+B(28)+
           B(27)+B(25)+B(24)+B(23)+B(22)+B(21)+B(20)+B(19)+B(18)+B(17)+
           B(15)+B(9)+B(8)+B(7)+B(3)+B(2)+B(0), 90),
  DEF_POLY(B(90)+B(89)+B(84)+B(81)+B(80)+B(78)+B(74)+B(73)+B(71)+B(68)+
           B(64), B(60)+B(59)+B(58)+B(57)+B(55)+B(54)+B(52)+B(50)+B(49)+
           B(47)+B(45)+B(42)+B(41)+B(39)+B(38)+B(36)+B(32)+B(28)+B(25)+
           B(21)+B(20)+B(19)+B(15)+B(12)+B(11)+B(9)+B(8)+B(3)+
           B(0), 91),
  DEF_POLY(B(91)+B(89)+B(88)+B(87)+B(86)+B(85)+B(84)+B(83)+B(80)+B(78)+
           B(76)+B(72)+B(70)+B(68), B(63)+B(62)+B(61)+B(59)+B(57)+B(56)+
           B(52)+B(51)+B(50)+B(49)+B(43)+B(40)+B(39)+B(37)+B(36)+B(35)+
           B(34)+B(33)+B(32)+B(26)+B(25)+B(24)+B(23)+B(22)+B(18)+B(15)+
           B(12)+B(11)+B(9)+B(7)+B(6)+B(3)+B(1)+B(0), 92),
  DEF_POLY(B(86)+B(85)+B(83)+B(82)+B(79)+B(78)+B(77)+B(75)+B(74)+B(73)+
           B(66)+B(64), B(59)+B(57)+B(56)+B(55)+B(54)+B(52)+B(51)+B(40)+
           B(38)+B(36)+B(34)+B(33)+B(28)+B(27)+B(26)+B(25)+B(23)+B(22)+
           B(21)+B(20)+B(19)+B(18)+B(16)+B(15)+B(14)+B(13)+B(12)+B(11)+B(8)+
           B(7)+B(6)+B(5)+B(4)+B(0), 93),
  DEF_POLY(B(93)+B(92)+B(91)+B(89)+B(88)+B(87)+B(86)+B(81)+B(80)+B(75)+
           B(66)+B(64), B(62)+B(61)+B(60)+B(59)+B(58)+B(57)+B(56)+B(54)+
           B(48)+B(47)+B(46)+B(45)+B(44)+B(42)+B(41)+B(38)+B(37)+B(36)+
           B(34)+B(33)+B(31)+B(30)+B(27)+B(26)+B(25)+B(22)+B(13)+B(12)+
           B(11)+B(10)+B(8)+B(7)+B(4)+B(0), 94),
  DEF_POLY(B(94)+B(88)+B(87)+B(82)+B(79)+B(78)+B(76)+B(73)+B(65)+
           B(64), B(62)+B(61)+B(60)+B(59)+B(58)+B(57)+B(53)+B(51)+B(50)+
           B(49)+B(48)+B(47)+B(46)+B(44)+B(40)+B(36)+B(34)+B(33)+B(30)+
           B(28)+B(27)+B(25)+B(22)+B(19)+B(18)+B(17)+B(16)+B(14)+B(7)+B(5)+
           B(3)+B(2)+B(1)+B(0), 95),
  DEF_POLY(B(92)+B(89)+B(88)+B(86)+B(83)+B(79)+B(78)+B(76)+B(75)+B(74)+
           B(72)+B(70)+B(67)+B(66), B(63)+B(60)+B(57)+B(55)+B(53)+B(51)+
           B(47)+B(46)+B(44)+B(43)+B(42)+B(39)+B(38)+B(36)+B(34)+B(32)+
           B(31)+B(30)+B(27)+B(26)+B(25)+B(22)+B(21)+B(19)+B(17)+B(13)+
           B(11)+B(10)+B(9)+B(8)+B(7)+B(4)+B(1)+B(0), 96),
  DEF_POLY(B(96)+B(94)+B(93)+B(91)+B(89)+B(87)+B(85)+B(83)+B(81)+B(78)+
           B(76)+B(74)+B(73)+B(68)+B(67)+B(64), B(62)+B(61)+B(57)+B(55)+
           B(54)+B(53)+B(49)+B(47)+B(41)+B(38)+B(35)+B(33)+B(28)+B(27)+
           B(24)+B(23)+B(21)+B(19)+B(18)+B(17)+B(15)+B(13)+B(12)+B(11)+B(8)+
           B(6)+B(4)+B(3)+B(1)+B(0), 97),
  DEF_POLY(B(97)+B(93)+B(92)+B(91)+B(90)+B(87)+B(83)+B(82)+B(80)+B(77)+
           B(76)+B(75)+B(74)+B(73)+B(72)+B(70)+B(69)+B(68)+B(66)+B(65)+
           B(64), B(63)+B(62)+B(61)+B(60)+B(59)+B(57)+B(55)+B(53)+B(50)+
           B(49)+B(48)+B(45)+B(44)+B(43)+B(42)+B(40)+B(38)+B(36)+B(35)+
           B(34)+B(28)+B(27)+B(24)+B(22)+B(21)+B(18)+B(17)+B(16)+B(15)+
           B(14)+B(12)+B(11)+B(9)+B(8)+B(2)+B(1)+B(0), 98),
  DEF_POLY(B(96)+B(94)+B(92)+B(86)+B(85)+B(84)+B(78)+B(77)+B(76)+B(75)+
           B(73)+B(71)+B(69)+B(68)+B(65), B(61)+B(59)+B(57)+B(56)+B(54)+
           B(50)+B(47)+B(46)+B(44)+B(41)+B(38)+B(36)+B(35)+B(34)+B(33)+
           B(32)+B(29)+B(27)+B(26)+B(25)+B(23)+B(22)+B(21)+B(19)+B(17)+
           B(16)+B(11)+B(9)+B(7)+B(6)+B(3)+B(2)+B(0), 99),
  DEF_POLY(B(99)+B(96)+B(95)+B(93)+B(92)+B(88)+B(87)+B(83)+B(78)+B(77)+
           B(76)+B(75)+B(74)+B(73)+B(70)+B(66)+B(64), B(63)+B(62)+B(60)+
           B(59)+B(57)+B(56)+B(53)+B(50)+B(47)+B(41)+B(39)+B(38)+B(37)+
           B(34)+B(25)+B(23)+B(21)+B(20)+B(19)+B(18)+B(17)+B(16)+B(13)+B(9)+
           B(8)+B(6)+B(5)+B(1)+B(0), 100),
  DEF_POLY(B(100)+B(98)+B(97)+B(95)+B(93)+B(92)+B(91)+B(89)+B(87)+B(85)+
           B(84)+B(82)+B(81)+B(80)+B(79)+B(76)+B(68)+B(66)+B(65), B(63)+
           B(62)+B(59)+B(57)+B(52)+B(51)+B(50)+B(47)+B(46)+B(45)+B(42)+
           B(41)+B(40)+B(39)+B(38)+B(37)+B(36)+B(34)+B(32)+B(31)+B(30)+
           B(24)+B(22)+B(21)+B(20)+B(18)+B(17)+B(16)+B(14)+B(12)+B(11)+
           B(10)+B(8)+B(7)+B(5)+B(4)+B(2)+B(1)+B(0), 101),
  DEF_POLY(B(101)+B(99)+B(97)+B(96)+B(92)+B(89)+B(88)+B(87)+B(86)+B(84)+
           B(82)+B(81)+B(80)+B(78)+B(77)+B(76)+B(75)+B(74)+B(73)+
           B(69), B(60)+B(59)+B(57)+B(56)+B(55)+B(54)+B(53)+B(51)+B(50)+
           B(49)+B(47)+B(45)+B(43)+B(41)+B(35)+B(34)+B(32)+B(31)+B(29)+
           B(27)+B(26)+B(25)+B(24)+B(21)+B(13)+B(12)+B(9)+B(8)+B(6)+B(5)+
           B(3)+B(0), 102),
  DEF_POLY(B(101)+B(98)+B(97)+B(96)+B(94)+B(93)+B(92)+B(90)+B(89)+B(88)+
           B(87)+B(85)+B(83)+B(81)+B(80)+B(79)+B(76)+B(75)+B(71)+B(70)+
           B(69)+B(66), B(63)+B(62)+B(60)+B(59)+B(58)+B(56)+B(54)+B(53)+
           B(48)+B(45)+B(43)+B(42)+B(41)+B(37)+B(36)+B(32)+B(31)+B(30)+
           B(27)+B(25)+B(23)+B(22)+B(19)+B(16)+B(15)+B(11)+B(9)+B(5)+B(3)+
           B(0), 103),
  DEF_POLY(B(98)+B(97)+B(95)+B(94)+B(91)+B(89)+B(88)+B(86)+B(85)+B(84)+
           B(81)+B(79)+B(78)+B(76)+B(74)+B(73)+B(70)+B(69)+B(68)+B(67)+
           B(66)+B(64), B(59)+B(53)+B(52)+B(51)+B(48)+B(46)+B(45)+B(43)+
           B(37)+B(34)+B(33)+B(31)+B(30)+B(28)+B(25)+B(22)+B(21)+B(20)+
           B(19)+B(14)+B(10)+B(8)+B(4)+B(2)+B(1)+B(0), 104),
  DEF_POLY(B(103)+B(100)+B(99)+B(98)+B(94)+B(90)+B(89)+B(86)+B(84)+B(82)+
           B(79)+B(76)+B(74)+B(73)+B(72)+B(71)+B(70)+B(69)+B(67)+
           B(66), B(63)+B(62)+B(59)+B(58)+B(57)+B(55)+B(51)+B(49)+B(48)+
           B(47)+B(46)+B(43)+B(42)+B(38)+B(36)+B(34)+B(33)+B(31)+B(30)+
           B(29)+B(28)+B(27)+B(24)+B(21)+B(20)+B(18)+B(17)+B(16)+B(14)+
           B(13)+B(11)+B(9)+B(7)+B(6)+B(5)+B(0), 105),
  DEF_POLY(B(105)+B(104)+B(103)+B(102)+B(100)+B(98)+B(94)+B(93)+B(92)+B(91)+
           B(90)+B(89)+B(87)+B(86)+B(85)+B(83)+B(82)+B(81)+B(79)+B(77)+
           B(69)+B(68)+B(67)+B(64), B(61)+B(60)+B(59)+B(58)+B(56)+B(55)+
           B(53)+B(50)+B(48)+B(44)+B(40)+B(38)+B(37)+B(36)+B(35)+B(34)+
           B(33)+B(30)+B(29)+B(26)+B(22)+B(20)+B(13)+B(10)+B(8)+B(7)+B(5)+
           B(0), 106),
  DEF_POLY(B(105)+B(101)+B(100)+B(98)+B(97)+B(96)+B(93)+B(92)+B(91)+B(90)+
           B(87)+B(86)+B(81)+B(79)+B(77)+B(75)+B(74)+B(72)+B(68)+B(67)+
           B(64), B(63)+B(62)+B(61)+B(60)+B(59)+B(58)+B(54)+B(53)+B(52)+
           B(50)+B(48)+B(47)+B(45)+B(42)+B(41)+B(38)+B(32)+B(29)+B(27)+
           B(26)+B(24)+B(21)+B(19)+B(18)+B(16)+B(15)+B(14)+B(13)+B(12)+
           B(10)+B(7)+B(6)+B(4)+B(1)+B(0), 107),
  DEF_POLY(B(106)+B(105)+B(102)+B(100)+B(97)+B(95)+B(90)+B(89)+B(88)+B(86)+
           B(83)+B(82)+B(81)+B(79)+B(78)+B(75)+B(72)+B(66)+B(64), B(63)+
           B(62)+B(59)+B(58)+B(56)+B(54)+B(52)+B(51)+B(50)+B(48)+B(46)+
           B(45)+B(44)+B(42)+B(40)+B(37)+B(36)+B(35)+B(33)+B(29)+B(27)+
           B(22)+B(19)+B(17)+B(14)+B(12)+B(11)+B(10)+B(9)+B(8)+B(7)+B(6)+
           B(5)+B(3)+B(0), 108),
  DEF_POLY(B(108)+B(102)+B(101)+B(100)+B(99)+B(98)+B(96)+B(95)+B(94)+B(90)+
           B(89)+B(88)+B(87)+B(84)+B(83)+B(81)+B(80)+B(77)+B(76)+B(75)+
           B(71)+B(67)+B(65), B(63)+B(61)+B(60)+B(54)+B(50)+B(49)+B(48)+
           B(43)+B(40)+B(39)+B(38)+B(36)+B(34)+B(29)+B(28)+B(27)+B(22)+
           B(21)+B(19)+B(16)+B(14)+B(13)+B(12)+B(10)+B(9)+B(7)+B(6)+B(5)+
           B(3)+B(2)+B(0), 109),
  DEF_POLY(B(109)+B(108)+B(107)+B(102)+B(101)+B(98)+B(97)+B(96)+B(94)+B(92)+
           B(91)+B(90)+B(88)+B(87)+B(85)+B(84)+B(83)+B(82)+B(81)+B(80)+
           B(79)+B(78)+B(74)+B(73)+B(71)+B(70)+B(69)+B(66)+B(64), B(61)+
           B(58)+B(57)+B(56)+B(50)+B(49)+B(46)+B(44)+B(43)+B(41)+B(36)+
           B(35)+B(34)+B(30)+B(29)+B(26)+B(25)+B(24)+B(22)+B(21)+B(17)+
           B(13)+B(11)+B(9)+B(4)+B(1)+B(0), 110),
  DEF_POLY(B(110)+B(109)+B(105)+B(98)+B(97)+B(95)+B(94)+B(93)+B(92)+B(90)+
           B(88)+B(84)+B(83)+B(82)+B(80)+B(77)+B(75)+B(72)+B(71)+B(70)+
           B(69)+B(66), B(63)+B(61)+B(60)+B(59)+B(57)+B(56)+B(55)+B(52)+
           B(51)+B(50)+B(49)+B(47)+B(43)+B(40)+B(36)+B(35)+B(34)+B(33)+
           B(31)+B(27)+B(26)+B(21)+B(20)+B(19)+B(17)+B(16)+B(12)+B(8)+B(6)+
           B(4)+B(3)+B(2)+B(1)+B(0), 111),
  DEF_POLY(B(109)+B(107)+B(106)+B(104)+B(100)+B(98)+B(96)+B(95)+B(94)+B(92)+
           B(91)+B(90)+B(89)+B(88)+B(86)+B(84)+B(81)+B(79)+B(78)+B(77)+
           B(75)+B(73)+B(71)+B(70)+B(69)+B(67)+B(64), B(63)+B(62)+B(61)+
           B(60)+B(58)+B(56)+B(54)+B(52)+B(51)+B(49)+B(48)+B(45)+B(44)+
           B(39)+B(38)+B(37)+B(36)+B(35)+B(34)+B(32)+B(30)+B(26)+B(25)+
           B(24)+B(23)+B(22)+B(21)+B(19)+B(16)+B(15)+B(11)+B(10)+B(9)+B(8)+
           B(3)+B(1)+B(0), 112),
  DEF_POLY(B(111)+B(107)+B(102)+B(100)+B(99)+B(98)+B(97)+B(96)+B(95)+B(94)+
           B(93)+B(92)+B(87)+B(86)+B(82)+B(81)+B(80)+B(79)+B(77)+B(76)+
           B(75)+B(72)+B(69)+B(64), B(61)+B(58)+B(56)+B(54)+B(53)+B(52)+
           B(51)+B(49)+B(46)+B(43)+B(40)+B(39)+B(37)+B(36)+B(35)+B(34)+
           B(33)+B(31)+B(29)+B(24)+B(22)+B(21)+B(20)+B(15)+B(14)+B(12)+
           B(10)+B(6)+B(1)+B(0), 113),
  DEF_POLY(B(112)+B(111)+B(110)+B(104)+B(102)+B(101)+B(100)+B(92)+B(89)+
           B(87)+B(83)+B(82)+B(80)+B(79)+B(75)+B(74)+B(73)+B(72)+B(71)+
           B(70)+B(68)+B(67)+B(65), B(60)+B(59)+B(57)+B(56)+B(55)+B(52)+
           B(50)+B(47)+B(44)+B(41)+B(36)+B(35)+B(30)+B(29)+B(26)+B(25)+
           B(24)+B(21)+B(18)+B(17)+B(16)+B(14)+B(12)+B(10)+B(7)+B(6)+
           B(0), 114),
  DEF_POLY(B(114)+B(112)+B(111)+B(110)+B(108)+B(107)+B(103)+B(102)+B(98)+
           B(97)+B(96)+B(90)+B(88)+B(87)+B(86)+B(83)+B(82)+B(80)+B(79)+
           B(77)+B(75)+B(70)+B(66)+B(65)+B(64), B(61)+B(60)+B(59)+B(58)+
           B(57)+B(53)+B(52)+B(51)+B(50)+B(47)+B(45)+B(43)+B(39)+B(38)+
           B(33)+B(32)+B(31)+B(29)+B(27)+B(21)+B(17)+B(14)+B(12)+B(10)+B(7)+
           B(4)+B(2)+B(1)+B(0), 115),
  DEF_POLY(B(113)+B(110)+B(108)+B(106)+B(105)+B(102)+B(101)+B(100)+B(98)+
           B(96)+B(92)+B(89)+B(87)+B(86)+B(84)+B(81)+B(79)+B(78)+B(76)+
           B(75)+B(73)+B(72)+B(71)+B(70)+B(67)+B(64), B(63)+B(62)+B(61)+
           B(52)+B(47)+B(45)+B(44)+B(42)+B(40)+B(39)+B(35)+B(34)+B(33)+
           B(31)+B(29)+B(25)+B(18)+B(15)+B(14)+B(10)+B(8)+B(6)+B(1)+
           B(0), 116),
  DEF_POLY(B(113)+B(111)+B(110)+B(109)+B(107)+B(106)+B(103)+B(102)+B(100)+
           B(96)+B(95)+B(94)+B(91)+B(90)+B(89)+B(86)+B(82)+B(81)+B(78)+
           B(77)+B(76)+B(75)+B(74)+B(73)+B(70)+B(67)+B(66), B(63)+B(61)+
           B(59)+B(57)+B(56)+B(55)+B(53)+B(52)+B(51)+B(50)+B(47)+B(45)+
           B(42)+B(40)+B(37)+B(35)+B(32)+B(30)+B(29)+B(25)+B(22)+B(21)+
           B(20)+B(19)+B(16)+B(15)+B(14)+B(12)+B(8)+B(5)+B(0), 117),
  DEF_POLY(B(117)+B(113)+B(110)+B(108)+B(105)+B(104)+B(103)+B(102)+B(99)+
           B(98)+B(97)+B(94)+B(93)+B(91)+B(90)+B(89)+B(85)+B(84)+B(82)+
           B(81)+B(79)+B(78)+B(77)+B(74)+B(73)+B(69)+B(67)+B(64), B(63)+
           B(62)+B(61)+B(57)+B(55)+B(51)+B(50)+B(46)+B(45)+B(43)+B(42)+
           B(41)+B(37)+B(33)+B(32)+B(30)+B(27)+B(26)+B(21)+B(19)+B(18)+
           B(17)+B(15)+B(14)+B(12)+B(10)+B(8)+B(7)+B(3)+B(2)+B(1)+
           B(0), 118),
  DEF_POLY(B(118)+B(111)+B(109)+B(107)+B(106)+B(105)+B(104)+B(101)+B(99)+
           B(98)+B(97)+B(94)+B(92)+B(91)+B(89)+B(83)+B(82)+B(80)+B(79)+
           B(67)+B(66), B(62)+B(61)+B(60)+B(58)+B(57)+B(52)+B(48)+B(46)+
           B(44)+B(42)+B(40)+B(39)+B(38)+B(36)+B(34)+B(33)+B(32)+B(29)+
           B(23)+B(22)+B(20)+B(19)+B(18)+B(15)+B(13)+B(12)+B(11)+B(6)+B(5)+
           B(4)+B(3)+B(1)+B(0), 119),
  DEF_POLY(B(116)+B(115)+B(113)+B(112)+B(110)+B(107)+B(106)+B(104)+B(103)+
           B(101)+B(100)+B(99)+B(98)+B(90)+B(89)+B(88)+B(87)+B(82)+B(80)+
           B(79)+B(77)+B(76)+B(75)+B(74)+B(73)+B(71)+B(70)+B(68)+B(65)+
           B(64), B(63)+B(62)+B(59)+B(55)+B(54)+B(48)+B(47)+B(45)+B(44)+
           B(40)+B(39)+B(38)+B(35)+B(33)+B(29)+B(27)+B(26)+B(25)+B(24)+
           B(23)+B(22)+B(21)+B(18)+B(17)+B(15)+B(13)+B(12)+B(10)+B(8)+B(3)+
           B(2)+B(0), 120),
  DEF_POLY(B(118)+B(117)+B(114)+B(113)+B(112)+B(110)+B(109)+B(104)+B(103)+
           B(101)+B(99)+B(97)+B(96)+B(95)+B(93)+B(92)+B(91)+B(90)+B(89)+
           B(87)+B(85)+B(84)+B(82)+B(81)+B(79)+B(73)+B(72)+B(68)+B(67)+
           B(66)+B(64), B(60)+B(58)+B(57)+B(56)+B(54)+B(53)+B(52)+B(51)+
           B(49)+B(48)+B(47)+B(45)+B(44)+B(38)+B(37)+B(36)+B(35)+B(33)+
           B(32)+B(31)+B(30)+B(27)+B(26)+B(24)+B(23)+B(22)+B(20)+B(19)+
           B(18)+B(16)+B(15)+B(12)+B(6)+B(5)+B(4)+B(2)+B(0), 121),
  DEF_POLY(B(121)+B(118)+B(114)+B(112)+B(109)+B(106)+B(103)+B(102)+B(101)+
           B(100)+B(97)+B(95)+B(90)+B(89)+B(87)+B(83)+B(81)+B(80)+B(79)+
           B(78)+B(77)+B(76)+B(75)+B(74)+B(72)+B(71)+B(70)+B(69)+B(68)+
           B(66)+B(64), B(61)+B(57)+B(51)+B(50)+B(47)+B(46)+B(43)+B(39)+
           B(38)+B(37)+B(36)+B(34)+B(33)+B(32)+B(30)+B(28)+B(27)+B(24)+
           B(22)+B(20)+B(18)+B(17)+B(14)+B(12)+B(11)+B(9)+B(7)+B(2)+
           B(0), 122),
  DEF_POLY(B(122)+B(121)+B(120)+B(119)+B(118)+B(117)+B(116)+B(113)+B(112)+
           B(111)+B(109)+B(106)+B(105)+B(103)+B(100)+B(98)+B(97)+B(95)+
           B(93)+B(92)+B(90)+B(87)+B(86)+B(85)+B(83)+B(81)+B(78)+B(77)+
           B(75)+B(74)+B(73)+B(72)+B(71)+B(70)+B(69)+B(68)+B(67)+B(65)+
           B(64), B(63)+B(62)+B(60)+B(55)+B(52)+B(51)+B(49)+B(47)+B(45)+
           B(43)+B(42)+B(41)+B(37)+B(36)+B(35)+B(34)+B(32)+B(28)+B(27)+
           B(26)+B(24)+B(23)+B(21)+B(20)+B(16)+B(13)+B(10)+B(9)+B(8)+B(7)+
           B(5)+B(2)+B(0), 123),
  DEF_POLY(B(123)+B(121)+B(120)+B(118)+B(117)+B(116)+B(115)+B(112)+B(111)+
           B(110)+B(109)+B(107)+B(104)+B(102)+B(101)+B(100)+B(99)+B(98)+
           B(97)+B(94)+B(90)+B(87)+B(86)+B(84)+B(83)+B(82)+B(79)+B(75)+
           B(72)+B(71)+B(70)+B(64), B(63)+B(56)+B(54)+B(51)+B(50)+B(47)+
           B(45)+B(44)+B(42)+B(39)+B(38)+B(36)+B(34)+B(33)+B(29)+B(26)+
           B(24)+B(20)+B(16)+B(14)+B(11)+B(10)+B(8)+B(7)+B(6)+B(4)+B(2)+
           B(0), 124),
  DEF_POLY(B(124)+B(123)+B(121)+B(119)+B(118)+B(116)+B(115)+B(114)+B(107)+
           B(105)+B(104)+B(103)+B(102)+B(99)+B(98)+B(96)+B(94)+B(93)+B(89)+
           B(83)+B(82)+B(81)+B(80)+B(79)+B(78)+B(75)+B(74)+B(73)+B(72)+
           B(70)+B(69)+B(68)+B(64), B(63)+B(59)+B(56)+B(55)+B(52)+B(51)+
           B(50)+B(49)+B(48)+B(44)+B(42)+B(38)+B(37)+B(36)+B(33)+B(31)+
           B(29)+B(27)+B(26)+B(25)+B(23)+B(21)+B(19)+B(18)+B(16)+B(14)+
           B(11)+B(8)+B(7)+B(6)+B(4)+B(1)+B(0), 125),
  DEF_POLY(B(124)+B(122)+B(121)+B(120)+B(119)+B(117)+B(113)+B(110)+B(108)+
           B(105)+B(103)+B(102)+B(101)+B(97)+B(93)+B(91)+B(90)+B(88)+B(86)+
           B(84)+B(82)+B(81)+B(79)+B(77)+B(76)+B(75)+B(73)+B(72)+B(71)+
           B(69)+B(67)+B(64), B(63)+B(62)+B(61)+B(60)+B(58)+B(56)+B(55)+
           B(52)+B(51)+B(48)+B(47)+B(45)+B(44)+B(42)+B(41)+B(40)+B(39)+
           B(37)+B(33)+B(32)+B(30)+B(29)+B(28)+B(27)+B(26)+B(25)+B(24)+
           B(23)+B(19)+B(18)+B(17)+B(16)+B(14)+B(13)+B(11)+B(9)+B(8)+B(7)+
           B(4)+B(2)+B(1)+B(0), 126),
  DEF_POLY(B(125)+B(124)+B(121)+B(116)+B(115)+B(105)+B(103)+B(101)+B(94)+
           B(93)+B(91)+B(90)+B(88)+B(87)+B(86)+B(85)+B(77)+B(73)+B(72)+
           B(70)+B(68)+B(67), B(63)+B(62)+B(61)+B(59)+B(57)+B(53)+B(52)+
           B(51)+B(49)+B(48)+B(46)+B(44)+B(41)+B(39)+B(38)+B(36)+B(35)+
           B(30)+B(27)+B(25)+B(23)+B(20)+B(19)+B(13)+B(12)+B(11)+B(10)+B(8)+
           B(7)+B(5)+B(4)+B(3)+B(2)+B(0), 127),
  DEF_POLY(B(127)+B(122)+B(121)+B(118)+B(117)+B(116)+B(109)+B(108)+B(107)+
           B(106)+B(104)+B(103)+B(102)+B(101)+B(96)+B(93)+B(92)+B(91)+B(89)+
           B(86)+B(85)+B(80)+B(78)+B(77)+B(76)+B(75)+B(74)+B(73)+B(72)+
           B(71)+B(66), B(60)+B(56)+B(53)+B(52)+B(50)+B(47)+B(45)+B(41)+
           B(39)+B(38)+B(37)+B(35)+B(34)+B(33)+B(30)+B(28)+B(25)+B(24)+
           B(23)+B(21)+B(20)+B(19)+B(14)+B(13)+B(10)+B(8)+B(5)+B(4)+B(2)+
           B(1)+B(0), 128),
};

// The number of polynomials in POLYS[].
const int CRC::N_POLYS = sizeof (poly_list) / sizeof (poly_list[0]);

// The externally visible name of poly_list.
// This guarantees that the size of poly_list is opaque.
const struct CRC::Poly *const CRC::POLYS = poly_list;

// The list of default CRC implementations that have been initialized.
// Protected by mu.
static struct {
  CRCImpl *crc;
} defaults[129];
static khMutex mu;

// The "constructor" for a CRC with an default polynomial.
CRC *CRC::Default(int degree, size_t roll_length) {
  CHECK(8 <= degree && degree <= 128);  // precondition
  CRCImpl *crc = 0;

  mu.Lock();
  // Find correct rolling window length
  crc = defaults[degree].crc;
  while (crc != 0 && roll_length != crc->roll_length_) {
    crc = crc->next_;
  }
  if (crc == 0) {
    crc = CRCImpl::NewInternal(CRC::POLYS[degree].lo, CRC::POLYS[degree].hi,
                               degree, roll_length); // Build the table
    crc->is_default_ = true;              // Mark it so deletes will crash
    crc->next_ = defaults[degree].crc;    // Add the table to the list
    defaults[degree].crc = crc;
  }
  mu.Unlock();

  return crc;
}

// The "constructor" for a CRC with an arbitrary polynomial.
CRC *CRC::New(std::uint64_t lo, std::uint64_t hi, int degree, size_t roll_length) {
  return CRCImpl::NewInternal(lo, hi, degree, roll_length);
}

// Internal version of the "constructor".
CRCImpl *CRCImpl::NewInternal(std::uint64_t lo, std::uint64_t hi,
                              int degree, size_t roll_length) {
  CHECK(8 <= degree && degree <= 128);  // precondition
  CHECK(lo != 0 || hi != 0);            // precondition
  // Generate the tables for extending a CRC by 4 bytes at a time.
  // Why 4 and not 8?  Because Pentium 4 has such small caches.
  typedef struct CRC_pair CRC_pair256[256];
  CRC_pair256 *t = new CRC_pair256[4]; // on heap to avoid a large stack frame
  for (int j = 0; j != 4; j++) {      // for each byte of extension....
    t[j][0].lo = 0;                   // a zero has no effect
    t[j][0].hi = 0;
    for (int i = 128; i != 0;  i >>= 1) {  // fill in entries for powers of 2
      if (j == 0 && i == 128) {
        t[j][i].lo = lo; // top bit in first byte is easy---it's the polynomial
        t[j][i].hi = hi;
      } else {
                  // each successive power of two is derive from the previous
                  // one, either in this table, or the last table
        struct CRC_pair pred;
        if (i == 128) {
          pred = t[j-1][1];
        } else {
          pred = t[j][i << 1];
        }
        // Advance the CRC by one bit (multiply by X, and take remainder
        // through one step of polynomial long division)
        if (pred.lo & 1) {
          t[j][i].lo = (pred.lo >> 1) ^ (pred.hi << 63) ^ lo;
          t[j][i].hi = (pred.hi >> 1) ^ hi;
        } else {
          t[j][i].lo = (pred.lo >> 1) ^ (pred.hi << 63);
          t[j][i].hi = pred.hi >> 1;
        }
      }
    }
    // CRCs have the property that CRC(a xor b) == CRC(a) xor CRC(b)
    // so we can make all the tables for non-powers of two by
    // xoring previously created entries.
    for (int i = 2; i != 256;  i <<= 1) {
      for (int k = i+1; k != (i << 1); k++) {
        t[j][k].lo = t[j][i].lo ^ t[j][k-i].lo;
        t[j][k].hi = t[j][i].hi ^ t[j][k-i].hi;
      }
    }
  }

  // Copy the newly built tables in t[] into an appropriate
  // CRC implenentation object.
  CRCImpl *result = 0;
  CRC32 *crc32 = 0;
  CRC64 *crc64 = 0;
  CRC128 *crc128 = 0;
  if (degree <= 32) {
    crc32 = new CRC32();
    for (int i = 0; i != 256; i++) {
      crc32->table0_[i] = t[0][i].lo;
      crc32->table1_[i] = t[1][i].lo;
      crc32->table2_[i] = t[2][i].lo;
      crc32->table3_[i] = t[3][i].lo;
    }
    result = crc32;
  } else if (degree <= 64) {
    crc64 = new CRC64();
    for (int i = 0; i != 256; i++) {
      crc64->table0_[i] = t[0][i].lo;
      crc64->table1_[i] = t[1][i].lo;
      crc64->table2_[i] = t[2][i].lo;
      crc64->table3_[i] = t[3][i].lo;
    }
    result = crc64;
  } else {
    crc128 = new CRC128();
    for (int i = 0; i != 256; i++) {
      crc128->table0_[i] = t[0][i];
      crc128->table1_[i] = t[1][i];
      crc128->table2_[i] = t[2][i];
      crc128->table3_[i] = t[3][i];
    }
    result = crc128;
  }
  // "result" is now a CRC object of the right type to handle
  // the polynomial of the right degree.

  result->is_default_ = false;        // fill in the fields
  result->next_ = 0;
  result->roll_length_ = roll_length;
  result->degree_ = degree;
  result->poly_lo_ = lo;
  result->poly_hi_ = hi;

  // Build the table for extending by zeroes.
  // Entry i=a-1+3*b (a in {1, 2, 3}, b in {0, 1, 2, 3, ...}
  // contains a polynomial Pi such that multiplying
  // a CRC by Pi mod P, where P is the CRC polynomial, is equivalent to
  // appending a*2**(2*b+SMALL_BITS) zero bytes to the original string.
  // Entry is generated by calling ExtendByZeroes() twice using
  // half the length from the previous entry.
  int j = 0;
  for (std::uint64_t inc_len = (1 << SMALL_BITS); inc_len != 0; inc_len <<= 2) {
    result->Empty(&lo, &hi);
    for (int k = 0; k != 3; k++) {
      result->ExtendByZeroes(&lo, &hi, inc_len >> 1);
      result->ExtendByZeroes(&lo, &hi, inc_len >> 1);
      if (degree <= 32) {
        crc32->zeroes_[j] = lo;
      } else if (degree <= 64) {
        crc64->zeroes_[j] = lo;
      } else {
        crc128->zeroes_[j].lo = lo;
        crc128->zeroes_[j].hi = hi;
      }
      j++;
    }
  }

  // Calculate the entries in the roll table, used for rolling checksums
  // of a fixed length.
  // Extend the powers of two in the one-byte extension table by the roll
  // length.
  int i = 256;
  do {
    i >>= 1;
    result->ExtendByZeroes(&t[0][i].lo, &t[0][i].hi, roll_length);
  } while (i != 0);
  // Calculate the non-powers of two using CRC(a xor b) == CRC(a) xor CRC(b)
  for (int i = 2; i != 256;  i <<= 1) {
    for (int j = i+1; j != (i << 1); j++) {
      t[0][j].lo = t[0][i].lo ^ t[0][j-i].lo;
      t[0][j].hi = t[0][i].hi ^ t[0][j-i].hi;
    }
  }
  // Now xor the CRC of (binary) 100000001 followed by
  // the roll length of zeroes.   This will be xored into every
  // entry.   This will simultaneously roll out the CRC
  // of the empty string that's been pushed one byte too far,
  // and roll in the CRC of the empty string in the correct place again.
  result->Empty(&lo, &hi);
  const std::uint8_t x = 0x80;
  result->Extend(&lo, &hi, &x, 1);
  result->ExtendByZeroes(&lo, &hi, roll_length);
  for (int i = 0; i != 256; i++) {
    t[0][i].lo ^= lo;
    t[0][i].hi ^= hi;
  }

  // Put the roll table into the object.
  if (degree <= 32) {
    for (int i = 0; i != 256; i++) {
      crc32->roll_[i] = t[0][i].lo;
    }
  } else if (degree <= 64) {
    for (int i = 0; i != 256; i++) {
      crc64->roll_[i] = t[0][i].lo;
    }
  } else {
    for (int i = 0; i != 256; i++) {
      crc128->roll_[i] = t[0][i];
    }
  }

  delete[] t;

  return result;
}

// The CRC of the empty string is always the CRC polynomial itself.
void CRCImpl::Empty(std::uint64_t *lo, std::uint64_t *hi) const {
  *lo = this->poly_lo_;
  *hi = this->poly_hi_;
}


// The 128-bit implementation

void CRC128::Extend(std::uint64_t *lo, std::uint64_t *hi, const void *bytes, size_t length)
                      const {
  const std::uint8_t *p = static_cast<const std::uint8_t *>(bytes);
  const std::uint8_t *e = p + length;
  union {
    std::uint64_t x64;
    std::uint32_t x32[2];
  } l, h;
  l.x64 = *lo;
  h.x64 = *hi;
  if (false) {      // this code works, but isn't an optimization
                    // on pentium 4.  It may be on Opteron...
    // point x at MIN(first 4-byte aligned byte in string, end of string)
    const std::uint8_t *x = p + ((zero_ptr - p) & 3);
    if (x > e) {
      x = e;
    }
    // Process bytes until finished or p is 4-byte aligned
    while (p != x) {
      int c = (l.x32[LO] & 0xff) ^ *p++;
      l.x32[LO] >>= 8;
      l.x32[LO] |= l.x32[HI] << 24;
      l.x32[HI] >>= 8;
      l.x32[HI] |= h.x32[LO] << 24;
      l.x64 ^= this->table0_[c].lo;
      h.x32[LO] >>= 8;
      h.x32[LO] |= h.x32[HI] << 24;
      h.x32[HI] >>= 8;
      h.x64 ^= this->table0_[c].hi;
    }
    // point x at MIN(last 4-byte aligned byte in string, end of string)
    x = e - ((e - zero_ptr) & 3);
    // Process bytes 4 at a time
    while (p < x) {
      std::uint32_t c = l.x32[LO] ^ WORD(p);
      p += 4;
      l.x32[LO] = l.x32[HI];
      l.x32[HI] = h.x32[LO];
      l.x64 ^= this->table3_[c & 0xff].lo;
      l.x64 ^= this->table2_[(c >> 8) & 0xff].lo;
      l.x64 ^= this->table1_[(c >> 16) & 0xff].lo;
      l.x64 ^= this->table0_[c >> 24].lo;
      h.x32[LO] = h.x32[HI];
      h.x32[HI] = 0;
      h.x64 ^= this->table3_[c & 0xff].hi;
      h.x64 ^= this->table2_[(c >> 8) & 0xff].hi;
      h.x64 ^= this->table1_[(c >> 16) & 0xff].hi;
      h.x64 ^= this->table0_[c >> 24].hi;
    }
  }
  // Process the last few bytes
  while (p != e) {
    int c = (l.x32[LO] & 0xff) ^ *p++;
    l.x32[LO] >>= 8;
    l.x32[LO] |= l.x32[HI] << 24;
    l.x32[HI] >>= 8;
    l.x32[HI] |= h.x32[LO] << 24;
    l.x64 ^= this->table0_[c].lo;
    h.x32[LO] >>= 8;
    h.x32[LO] |= h.x32[HI] << 24;
    h.x32[HI] >>= 8;
    h.x64 ^= this->table0_[c].hi;
  }
  *lo = l.x64;
  *hi = h.x64;
}

void CRC128::ExtendByZeroes(std::uint64_t *lo, std::uint64_t *hi, size_t length) const {
  // Process the low order SMALL_BITS of the length by simply
  // using Extend() on an array of bytes that are zero.
  int small_part = (length & ((1 << SMALL_BITS)-1));
  if (small_part != 0) {
    this->Extend(lo, hi, zeroes, small_part);
  }
  length >>= SMALL_BITS;
  if (length != 0) {              // if the length was at least 2**SMALL_BITS
    std::uint64_t l = *lo;
    std::uint64_t h = *hi;
    std::uint64_t onebit = 1;
    std::uint64_t poly_lo = this->poly_lo_;
    std::uint64_t poly_hi = this->poly_hi_;
    onebit <<= this->degree_ - 65;
    // For each pair of bits in length
    // (after the low-oder bits have been removed)
    // we lookup the appropriate polynomial in the zeroes_ array
    // and do a polynomial long multiplication (mod the CRC polynomial)
    // to extend the CRC by the appropriate number of bits.
    for (int i = 0; length != 0; i += 3, length >>= 2) {
      int c = length & 3;       // pick next two bits
      if (c != 0) {             // if they are not zero,
                                // multiply by entry in table
        std::uint64_t m_lo = this->zeroes_[c+i-1].lo;
        std::uint64_t m_hi = this->zeroes_[c+i-1].hi;
        std::uint64_t result_lo = 0;
        std::uint64_t result_hi = 0;
        for (std::uint64_t one = onebit; one != 0; one >>= 1) {  // high half
          if ((h & one) != 0) {
            result_lo ^= m_lo;
            result_hi ^= m_hi;
          }
          if (m_lo & 1) {
            m_lo = (m_lo >> 1) ^ (m_hi << 63) ^ poly_lo;
            m_hi = (m_hi >> 1) ^ poly_hi;
          } else {
            m_lo = (m_lo >> 1) ^ (m_hi << 63);
            m_hi = (m_hi >> 1);
          }
        }
        onebit = 1;
        onebit <<= 63;
        for (std::uint64_t one = onebit; one != 0; one >>= 1) {  // low half
          if ((l & one) != 0) {
            result_lo ^= m_lo;
            result_hi ^= m_hi;
          }
          if (m_lo & 1) {
            m_lo = (m_lo >> 1) ^ (m_hi << 63) ^ poly_lo;
            m_hi = (m_hi >> 1) ^ poly_hi;
          } else {
            m_lo = (m_lo >> 1) ^ (m_hi << 63);
            m_hi = (m_hi >> 1);
          }
        }
        l = result_lo;
        h = result_hi;
      }
    }
    *lo = l;
    *hi = h;
  }
}

void CRC128::Roll(std::uint64_t *lo, std::uint64_t *hi, std::uint8_t o_byte, std::uint8_t i_byte) const {
  std::uint64_t l = *lo;
  std::uint64_t h = *hi;
  int c = (l & 0xff) ^ i_byte;
  // Roll in i_byte and out o_byte
  *lo = this->table0_[c].lo ^ (l >> 8) ^ (h << 56) ^ this->roll_[o_byte].lo;
  *hi = this->table0_[c].hi ^ (h >> 8) ^ this->roll_[o_byte].hi;
}


//  The 64-bit implementation

void CRC64::Extend(std::uint64_t *lo, std::uint64_t *hi, const void *bytes, size_t length)
                      const {
  const std::uint8_t *p = static_cast<const std::uint8_t *>(bytes);
  const std::uint8_t *e = p + length;
  union {
    std::uint64_t x64;
    std::uint32_t x32[2];
  } l;
  l.x64 = *lo;
  // point x at MIN(first 4-byte aligned byte in string, end of string)
  // HACK: This variable is declared "volatile" in order to prevent
  // gcc from putting in a register.  The code runs significantly
  // slower if volatile is removed.
  const std::uint8_t *volatile x = p + ((zero_ptr - p) & 3);
  if (x > e) {
    x = e;
  }
  // Process bytes until finished or p is 4-byte aligned
  while (p != x) {
    int c = (l.x32[LO] & 0xff) ^ *p++;
    l.x32[LO] >>= 8;
    l.x32[LO] |= l.x32[HI] << 24;
    l.x32[HI] >>= 8;
    l.x64 ^= this->table0_[c];
  }
  // point x at MIN(last 4-byte aligned byte in string, end of string)
  x = e - ((e - zero_ptr) & 3);
  // Process bytes 4 at a time
  while (p < x) {
    std::uint32_t c = l.x32[LO] ^ WORD(p);
    p += 4;
    l.x32[LO] = l.x32[HI];
    l.x32[HI] = 0;
    l.x64 ^= this->table3_[c & 0xff];
    l.x64 ^= this->table2_[(c >> 8) & 0xff];
    l.x64 ^= this->table1_[(c >> 16) & 0xff];
    l.x64 ^= this->table0_[c >> 24];
  }
  // Process the last few bytes
  while (p != e) {
    int c = (l.x32[LO] & 0xff) ^ *p++;
    l.x32[LO] >>= 8;
    l.x32[LO] |= l.x32[HI] << 24;
    l.x32[HI] >>= 8;
    l.x64 ^= this->table0_[c];
  }
  *lo = l.x64;
}

void CRC64::ExtendByZeroes(std::uint64_t *lo, std::uint64_t *hi, size_t length) const {
  // Process the low order SMALL_BITS of the length by simply
  // using Extend() on an array of bytes that are zero.
  int small_part = (length & ((1 << SMALL_BITS)-1));
  if (small_part != 0) {
    this->Extend(lo, hi, zeroes, small_part);
  }
  length >>= SMALL_BITS;
  if (length != 0) {          // if the length was at least 2**SMALL_BITS
    std::uint64_t l = *lo;
    std::uint64_t onebit = 1;
    onebit <<= this->degree_ - 1;
    // For each pair of bits in length
    // (after the low-oder bits have been removed)
    // we lookup the appropriate polynomial in the zeroes_ array
    // and do a polynomial long multiplication (mod the CRC polynomial)
    // to extend the CRC by the appropriate number of bits.
    for (int i = 0; length != 0; i += 3, length >>= 2) {
      int c = length & 3;       // pick next two bits
      if (c != 0) {             // if they are not zero,
                                // multiply by entry in table
        std::uint64_t m = this->zeroes_[c+i-1];
        std::uint64_t result = 0;
        for (std::uint64_t one = onebit; one != 0; one >>= 1) {
          if ((l & one) != 0) {
            result ^= m;
          }
          if (m & 1) {
            m = (m >> 1) ^ poly_lo_;
          } else {
            m = (m >> 1);
          }
        }
        l = result;
      }
    }
    *lo = l;
  }
}

void CRC64::Roll(std::uint64_t *lo, std::uint64_t *hi, std::uint8_t o_byte, std::uint8_t i_byte) const {
  std::uint64_t l = *lo;
  // Roll in i_byte and out o_byte
  *lo = this->table0_[(l & 0xff) ^ i_byte] ^ (l >> 8) ^ this->roll_[o_byte];
}


//  The 32-bit implementation

void CRC32::Extend(std::uint64_t *lo, std::uint64_t *hi, const void *bytes, size_t length)
                      const {
  const std::uint8_t *p = static_cast<const std::uint8_t *>(bytes);
  const std::uint8_t *e = p + length;
  std::uint32_t l = *lo;

  if (kNeedAlignedLoads) {
    // point x at first 4-byte aligned byte in string. this might be passed the
    // end of the string.
    const std::uint8_t *x = p + ((zero_ptr - p) & 3);
    if (x <= e) {
      // Process bytes until finished or p is 4-byte aligned
      while (p != x) {
        int c = (l & 0xff) ^ *p++;
        l = this->table0_[c] ^ (l >> 8);
      }
    }
  }

  // Process bytes 4 at a time
  while ((p + 4) <= e) {
    std::uint32_t c = l ^ WORD(p);
    p += 4;
    l = this->table3_[c & 0xff] ^
        this->table2_[(c >> 8) & 0xff] ^
        this->table1_[(c >> 16) & 0xff] ^
        this->table0_[c >> 24];
  }

  // Process the last few bytes
  while (p != e) {
    int c = (l & 0xff) ^ *p++;
    l = this->table0_[c] ^ (l >> 8);
  }
  *lo = l;
}

void CRC32::ExtendByZeroes(std::uint64_t *lo, std::uint64_t *hi, size_t length) const {
  // Process the low order SMALL_BITS of the length by simply
  // using Extend() on an array of bytes that are zero.
  int small_part = (length & ((1 << SMALL_BITS)-1));
  if (small_part != 0) {
    this->Extend(lo, hi, zeroes, small_part);
  }
  length >>= SMALL_BITS;
  if (length != 0) {          // if the length was at least 2**SMALL_BITS
    std::uint32_t l = *lo;
    std::uint32_t onebit = 1;
    onebit <<= this->degree_ - 1;
    // For each pair of bits in length
    // (after the low-oder bits have been removed)
    // we lookup the appropriate polynomial in the zeroes_ array
    // and do a polynomial long multiplication (mod the CRC polynomial)
    // to extend the CRC by the appropriate number of bits.
    for (int i = 0; length != 0; i += 3, length >>= 2) {
      int c = length & 3;       // pick next two bits
      if (c != 0) {             // if they are not zero,
                                // multiply by entry in table
        std::uint32_t m = this->zeroes_[c+i-1];
        std::uint32_t result = 0;
        for (std::uint32_t one = onebit; one != 0; one >>= 1) {
          if ((l & one) != 0) {
            result ^= m;
          }
          if (m & 1) {
            m = (m >> 1) ^ poly_lo_;
          } else {
            m = (m >> 1);
          }
        }
        l = result;
      }
    }
    *lo = l;
  }
}

void CRC32::Roll(std::uint64_t *lo, std::uint64_t *hi, std::uint8_t o_byte, std::uint8_t i_byte) const {
  std::uint32_t l = *lo;
  // Roll in i_byte and out o_byte
  *lo = this->table0_[(l & 0xff) ^ i_byte] ^ (l >> 8) ^ this->roll_[o_byte];
}

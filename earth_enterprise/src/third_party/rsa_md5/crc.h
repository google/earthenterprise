#if !defined(_CRC_H_)
#define _CRC_H_

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

#include <common/base/macros.h>
#include <khTypes.h>

// This class implements CRCs (aka Rabin Fingerprints).
// Treats the input as a polynomial with coefficients in Z(2),
// and finds the remainder when divided by an irreducible polynomial
// of the appropriate length.
// It handles all CRC sizes from 8 to 128 bits.
// The input string is prefixed with a "1" bit, and has "degree" "0" bits
// appended to it before the remainder is found.   This ensures that
// short strings are scrambled somewhat.

// A polynomial is represented by the bit pattern formed by its coefficients,
// but with the highest order bit not stored.
// The highest degree coefficient is stored in the lowest numbered bit
// in the the lowest adderessed byte.   Thus, in what follows,
// the highest degree coeficient that is stored is in the low order bit
// of "lo" or "*lo".

// Typical usage:
//
// // prepare to do 64-bit CRCs using the default polynomial.  No rolling hash.
// CRC *crc = CRC::Default(64, 0);
// ...
// uint64 lo;   // declare a lo,hi pair to hold the CRC
// uint64 hi;
// crc->Empty(&lo, &hi);      // Initialize to CRC of empty string
// crc->Extend(&lo, &hi, "hello", 5);     // Get CRC of "hello"
// ...
//
// // prepare to use a 32-bit rolling hash over 6 bytes
// CRC *crc = CRC::Default(32, 6);
// ...
// uint64 lo;   // declare a lo,hi pair to hold the CRC
// uint64 hi;
// crc->Empty(&lo, &hi);      // Initialize to CRC of empty string
// crc->Extend(&lo, &hi, data, 6);     // Get CRC of first 6 bytes
// for (int i = 6; i != sizeof (data); i++) {
//   crc->Roll(&lo, &hi, data[i-6], data[i]); // Move window by one byte
//   // lo,hi is CRC of bytes data[i-5...i]
// }
// ...
//

class CRC {
 public:
  // Initialize all the tables for CRC's of a given bit length "degree"
  // using a default polynomial of the given length.
  //
  // The argument "roll_length" is used by subsequent calls to
  // Roll().
  // Returns a handle that MUST NOT be destroyed with delete.
  // The default polynomials are those in POLYS[8...128].
  // Handles returned by Default() MUST NOT be deleted.
  // Identical calls to Default() yield identical handles.
  static CRC *Default(int degree, size_t roll_length);

  // Initialize all the tables for CRC's of a given bit length "degree"
  // using an arbitrary CRC polynomial.
  // Normally, you would use Default() instead of New()---see above.
  //
  // Requires that "lo,hi" contain an irreducible polynomial of degree "degree"
  // Requires 8 <= degree && degree <= 128
  // Any irreducible polynomial of the correct degree will work.
  // See the POLYS array for suitable irredicible polynomials.
  //
  // The argument "roll_length" is used by subsequent calls to
  // Roll().
  // Each call to New() yeilds a pointer to a new object
  // that may be deallocated with delete.
  static CRC *New(uint64 lo, uint64 hi, int degree, size_t roll_length);

  virtual ~CRC();

  // Place the CRC of the empty string in "*lo,*hi"
  virtual void Empty(uint64 *lo, uint64 *hi) const = 0;

  // If "*lo,*hi" is the CRC of bytestring A, place the CRC of
  // the bytestring formed from the concatenation of A and the "length"
  // bytes at "bytes" into "*lo,*hi".
  virtual void Extend(/*INOUT*/ uint64 *lo, /*INOUT*/ uint64 *hi,
                      const void *bytes, size_t length) const = 0;

  // Equivalent to Extend(lo, hi, bytes, length) where "bytes"
  // points to an array of "length" zero bytes.
  virtual void ExtendByZeroes(/*INOUT*/ uint64 *lo, /*INOUT*/ uint64 *hi,
                              size_t length) const = 0;

  // If "*lo,*hi" is the CRC of a byte string of length "roll_length"
  // (which is an argument to New() and Default()) that consists of
  // byte "o_byte" followed by string S, set "*lo,*hi" to the CRC of
  // the string that consists of S followed by the byte "i_byte".
  virtual void Roll(/*INOUT*/ uint64 *lo, /*INOUT*/ uint64 *hi,
                    uint8 o_byte, uint8 i_byte) const = 0;

  // POLYS[] is an array of valid triples that may be given to New()
  static const struct Poly {
    uint64 lo;                      // first half suitable CRC polynomial
    uint64 hi;                      // second half of suitable CRC polynomial
    int degree;                     // degree of suitable CRC polynomial
  } *const POLYS;
  // It is guaranteed that no two entries in POLYS[] are identical,
  // that POLYS[i] cnotains a polynomial of degree i for 8 <= i <= 128,
  // that POLYS[0] and POLYS[1] contains polynomials of degree 32,
  // that POLYS[2] and POLYS[3] contains polynomials of degree 64,
  // that POLYS[4] and POLYS[5] contains polynomials of degree 96, and
  // that POLYS[6] and POLYS[7] contains polynomials of degree 128.

  static const int N_POLYS;         // Number of elements in POLYS array.

 protected:
  CRC();      // Clients may not call constructor;
                // use Default() or New() instead.

 private:
  DISALLOW_COPY_AND_ASSIGN(CRC);
};

#endif

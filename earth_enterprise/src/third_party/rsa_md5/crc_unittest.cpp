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

// This is the unit test for crc.cpp and crc32.cpp


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "crc.h"
#include "crc32.h"

// Same as CRC::Extend, but one byte at a time.

static
void MyExtend(CRC *crc, std::uint64_t *lo, std::uint64_t *hi, const char *str, size_t len) {
  for (std::uint32_t i = 0; i != len; i++) {
    crc->Extend(lo, hi, &str[i], 1);
  }
}

#define STRING_AND_LEN(_s)  { _s, sizeof(_s)-1 }

#define CHECK(a) \
  { if (!(a)) { \
      fprintf(stderr, "FAILED at %s:%d" \
      " CHECK(" #a ")\n", __FILE__, __LINE__); \
      exit(2);\
    } \
  }

#define CHECK_EQ(a,b) \
  { if ((a) != (b)) { \
      fprintf(stderr, "FAILED at %s:%d" \
      " CHECK_EQ(" #a "," #b ")\n", __FILE__, __LINE__); \
      exit(2);\
    } \
  }

int main (int argc, char *argv[]) {
  bool FLAGS_print_output = false;

  static const struct {     // a list of test strings to try
    const char *str;
    int len;
  } strings[] = {
    STRING_AND_LEN(""),
    STRING_AND_LEN("\0"),
    STRING_AND_LEN("\1"),
    STRING_AND_LEN("\0\1"),
    STRING_AND_LEN("hello"),
    STRING_AND_LEN("the quick brown fox jumps over the lazy dog"),
    STRING_AND_LEN("To be, or not to be: that is the question:\n"
                   "Whether 'tis nobler in the mind to suffer\n"
                   "The slings and arrows of outrageous fortune,\n"
                   "Or to take arms against a sea of troubles,\n"
                   "And by opposing end them?\n")
  };
  // for various roll lengths, for all polynomials, for all test strings,
  // we check that Extend() and ExtendByZeroes() get the same answer
  // as doing the same thing one byte at a time.
  // We also check that the rolling CRC gets the same answer as simply
  // calculating the CRC on each successive string.
  for (int r = 1; r < 16; r += 13) {   // try various roll lengths
    for (int i = 0; i != CRC::N_POLYS; i++) {   // try all polynomials
      CRC *crc = CRC::New(CRC::POLYS[i].lo,
                          CRC::POLYS[i].hi,
                          CRC::POLYS[i].degree, r);
      // Try all test strings
      for (int j = 0; j != sizeof (strings) / sizeof (strings[0]); j++) {
        std::uint64_t lo;
        std::uint64_t hi;

        if (FLAGS_print_output) {
          printf ("%s %d   %d\n",
                  strings[j].str, strings[j].len, CRC::POLYS[i].degree);
        }
        crc->Empty(&lo, &hi);
        if (FLAGS_print_output) {
          printf ("%016llx%016llx\n",
                  (long long unsigned int)hi,
                  (long long unsigned int)lo);
        }
        crc->Extend(&lo, &hi, strings[j].str, strings[j].len);
        if (FLAGS_print_output) {
          printf("%016llx%016llx\n",
                 (long long unsigned int)hi,
                 (long long unsigned int)lo);
        }

        std::uint64_t mlo;
        std::uint64_t mhi;
        crc->Empty(&mlo, &mhi);
        MyExtend(crc, &mlo, &mhi, strings[j].str, strings[j].len);
        if (FLAGS_print_output) {
          printf ("%016llx%016llx\n",
                  (long long unsigned int)mhi,
                  (long long unsigned int)mlo);
        }
        CHECK_EQ(mlo, lo);
        CHECK_EQ(mhi, hi);

        int z = random () & 0xffff;
        crc->ExtendByZeroes(&lo, &hi, z);
        for (int k = 0; k != z; k++) {
          MyExtend(crc, &mlo, &mhi, "\0", 1);
        }

        if (strings[j].len > r) {
          crc->Empty(&lo, &hi);
          crc->Extend(&lo, &hi, strings[j].str, r);
          crc->Empty(&mlo, &mhi);
          MyExtend(crc, &mlo, &mhi, strings[j].str, r);
          CHECK_EQ(mlo, lo);
          CHECK_EQ(mhi, hi);
          for (int k = r; k != strings[j].len; k++) {
            crc->Roll(&lo, &hi, strings[j].str[k-r], strings[j].str[k]);
            crc->Empty(&mlo, &mhi);
            MyExtend(crc, &mlo, &mhi, &strings[j].str[k-r+1], r);
            CHECK_EQ(mlo, lo);
            CHECK_EQ(mhi, hi);
          }
        }
        if (FLAGS_print_output) {
          printf("\n");
        }
      }
      delete crc;
    }
  }

  // Check that the default polynomials work
  CRC *crc13_8_1 = CRC::Default(13, 8);
  CRC *crc13_8_2 = CRC::Default(13, 8);
  CRC *crc13_9_1 = CRC::Default(13, 9);
  CRC *crc13_9_2 = CRC::Default(13, 9);
  CHECK_EQ(crc13_8_1, crc13_8_2);
  CHECK_EQ(crc13_9_1, crc13_9_2);
  CHECK(crc13_8_1 != crc13_9_1);
  std::uint64_t lo0;
  std::uint64_t hi0;
  crc13_8_1->Empty(&lo0, &hi0);
  crc13_8_1->Extend(&lo0, &hi0, "hello", 5);
  CRC *crc = CRC::New(CRC::POLYS[13].lo, CRC::POLYS[13].hi,
                      CRC::POLYS[13].degree, 8);
  CHECK(crc13_8_1 != crc);
  std::uint64_t lo1;
  std::uint64_t hi1;
  crc->Empty(&lo1, &hi1);
  crc->Extend(&lo1, &hi1, "hello", 5);
  delete crc;
  CHECK_EQ(lo0, lo1);
  CHECK_EQ(hi0, hi1);

  // Test crc32

  // Test strings of all 0

  const int nzeros = 4;
  const std::uint32_t crc32_0[nzeros] =
    { 0x838d55bd, 0xf13cf8ed, 0x71eea7d9, 0x7f3f0d27 };
  const unsigned char zeros[nzeros] = { 0, 0, 0, 0 };

  for (std::uint32_t i = 1; i <= 4; ++i) {
    std::uint32_t crcval = Crc32(zeros, i);
    CHECK_EQ(crcval, crc32_0[i-1]);
  }

  // Try all test strings
  static const std::uint32_t string_crc32[] = {
    0xb9e50c1d,
    0x838d55bd,
    0xf75de6fc,
    0x85ec4bac,
    0x2dc01ebc,
    0xc319fe90,
    0xb692c92d };

  for (int j = 0; j != sizeof (strings) / sizeof (strings[0]); j++) {
    std::uint32_t crcval = Crc32(strings[j].str, strings[j].len);
    CHECK_EQ(crcval, string_crc32[j]);
  }

  fprintf(stderr, "Passed.\n");
  return 0;
}

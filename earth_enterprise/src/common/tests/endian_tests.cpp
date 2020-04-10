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

//
// Unit tests for khEndian

#include <functional>
#include <khEndian.h>
#include <khGuard.h>
#include <UnitTest.h>

class khEndianUnitTest : public UnitTest<khEndianUnitTest> {
 public:
  bool TestDefines() {
    std::uint32_t test = 0x12345678;
#if __BYTE_ORDER == __BIG_ENDIAN
    TestAssert(((std::uint8_t*)&test)[0] == 0x12);
    TestAssert(((std::uint8_t*)&test)[1] == 0x34);
    TestAssert(((std::uint8_t*)&test)[2] == 0x56);
    TestAssert(((std::uint8_t*)&test)[3] == 0x78);
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    TestAssert(((std::uint8_t*)&test)[0] == 0x78);
    TestAssert(((std::uint8_t*)&test)[1] == 0x56);
    TestAssert(((std::uint8_t*)&test)[2] == 0x34);
    TestAssert(((std::uint8_t*)&test)[3] == 0x12);
#endif
    return true;
  }

  template <class T>
  bool TestOldLowLevelType(T test, T swapped) {
    T be   = HostToBigEndian(test);
    T le   = HostToLittleEndian(test);
    T res1 = BigEndianToHost(be);
    T res2 = LittleEndianToHost(le);
    
    TestAssert(res1 == test);
    TestAssert(res2 == test);
#if __BYTE_ORDER == __BIG_ENDIAN
    TestAssert(test == be);
    TestAssert(swapped == le);
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    TestAssert(test == le);
    TestAssert(swapped == be);
#endif
    return true;
  }


  bool TestOldLowLevel() {
    return 
      TestOldLowLevelType<std::uint8_t> (0x12, 0x12) &&
      TestOldLowLevelType<std::uint16_t>(0x1234, 0x3412) &&
      TestOldLowLevelType<std::uint32_t>(0x12345678UL, 0x78563412UL) &&
      TestOldLowLevelType<std::uint64_t>(0x1234567890123456ULL,
                                  0x5634129078563412ULL) &&
      TestOldLowLevelType<std::int8_t> (0x12, 0x12) && 
      TestOldLowLevelType<std::int16_t>(0x1234, 0x3412) &&
      TestOldLowLevelType<std::int32_t>(0x12345678L, 0x78563412L) &&
      TestOldLowLevelType<std::int64_t>(0x1234567890123456LL, 0x5634129078563412LL);
  }


  template <class WBuffer, class RBuffer>
  bool TestBasicStreamTyped(void) {
    const std::uint8_t  t1      = 0x12;
    const std::uint16_t t2      = 0x3456;
    const std::string t3 = "This old man, he played one...";
    const std::uint32_t t4      = 0x34567890UL;
    const bool   t5      = false;
    const std::uint64_t t6      = 0x3456789012345678ULL;
    const bool   t7      = true;
    WBuffer wbuf;
    wbuf << t1 << t2 << t3 << t4 << t5 << t6 << t7;

    std::uint8_t  r1;
    std::uint16_t r2;
    std::string r3;
    std::uint32_t r4;
    bool   r5;
    std::uint64_t r6;
    bool   r7;
    RBuffer rbuf(wbuf.data(), wbuf.size());
    rbuf >> r1 >> r2 >> r3 >> r4 >> r5 >> r6 >> r7;

    bool success = ((t1 == r1) &&
                    (t2 == r2) &&
                    (t3 == r3) &&
                    (t4 == r4) &&
                    (t5 == r5) &&
                    (t6 == r6) &&
                    (t7 == r7) &&
                    (wbuf.size() == sizeof(t1) + sizeof(t2) + t3.size() + 1
                     + sizeof(t4) + sizeof(t5) + sizeof(t6) + sizeof(t7))
                    );

    // Test that reset does what it should do
    wbuf.reset();
    success = success &&
              (wbuf.size() == 0) &&
              (wbuf.CurrPos() == 0);
    return success;
  }

  bool TestBasicStream(void) {
    bool success = true;
    if (!TestBasicStreamTyped<LittleEndianWriteBuffer,
        LittleEndianReadBuffer>()) {
      fprintf(stderr, "TestBasicStream: LittleEndian FAILED\n");
      success = false;
    }
    if (!TestBasicStreamTyped<BigEndianWriteBuffer,
        BigEndianReadBuffer>()) {
      fprintf(stderr, "TestBasicStream: BigEndian FAILED\n");
      success = false;
    }
    return success;
  }

  bool TestFixedString(void) {
    LittleEndianWriteBuffer wbuf;
    const std::uint32_t t = 0x12345678;
    const std::string magic = "Google Magic";
    wbuf << FixedLengthString(magic, 16)      << t
         << FixedLengthString("Google Magic", 8)       << t
         << FixedLengthString("Google Magic", 16, ' ') << t;

    std::string r1;
    std::uint32_t      r2;
    std::string r3;
    std::uint32_t      r4;
    std::string r5;
    std::uint32_t      r6;
    LittleEndianReadBuffer rbuf(wbuf.data(), wbuf.size());
    rbuf >> FixedLengthString(r1, 16) >> r2
         >> FixedLengthString(r3, 8)  >> r4
         >> FixedLengthString(r5, 16) >> r6;

    bool success = ((strcmp(r1.c_str(), "Google Magic")==0) &&
                    (r2 == t) &&
                    (r3 == "Google M") &&
                    (r4 == t) &&
                    (r5 == "Google Magic    ") &&
                    (r6 == t)
                    );
    if (!success) {
      fprintf(stderr, "r1: '%s' ?= '%s'\n", r1.c_str(), "Google Magic");
      fprintf(stderr, "r3: '%s' ?= '%s'\n", r3.c_str(), "Google M");
      fprintf(stderr, "r5: '%s' ?= '%s'\n", r5.c_str(), "Google Magic    ");
      fprintf(stderr, "r5.size() = %u\n", (unsigned int)r5.size());
      fprintf(stderr, "r6: %u ?= %u\n", r6, t);
    }
    return success;
  }

  enum Color { Orange, Red, Green, Blue };
  bool TestEncodeAs(void) {
    LittleEndianWriteBuffer wbuf;
    const bool b1 = true;
    bool b2 = false;
    Color color = Green;
    wbuf << EncodeAs<std::uint8_t>(b1) << EncodeAs<std::int16_t>(b2)
         << EncodeAs<std::uint8_t>(color);

    bool r1;
    bool r2;
    Color r3;
    LittleEndianReadBuffer rbuf(wbuf.data(), wbuf.size());
    rbuf >> DecodeAs<std::uint8_t>(r1) >> DecodeAs<std::int16_t>(r2)
         >> DecodeAs<std::uint8_t>(r3);

    bool success = ((b1 == r1) &&
                    (b2 == r2) &&
                    (color == r3) &&
                    (wbuf.size() == 2 * sizeof(std::uint8_t) + sizeof(std::int16_t))
                    );
                    
    return success;
  }

  template <class WBuffer, class RBuffer>
  bool TestRandomAccessTyped(void) {
    std::uint8_t w1 = 0xC2;
    std::uint32_t w2 = 0xABCD0123;
    std::uint64_t w3 = 0xFEDCBA9876543210LL;
    std::int32_t w4 = -87654;
    std::string w5 = "This is a test. This is only a test.";
    std::uint32_t w6 = 0x6543210F;

    WBuffer wbuf;
    wbuf.Seek(sizeof(w1) + sizeof(w2) + sizeof(w3) + sizeof(w4));
    wbuf << w5;
    wbuf.Seek(sizeof(w1) + sizeof(w2));
    wbuf << w3 << w4;
    wbuf.Seek(0);
    wbuf << w1 << w2;
    wbuf.Seek(wbuf.size());
    wbuf << w6;
    bool success = wbuf.size() ==
                   (sizeof(w1) + sizeof(w2) + sizeof(w3) + sizeof(w4)
                    + w5.size() + 1 + sizeof(w6));

    std::uint8_t r1;
    std::uint32_t r2;
    std::uint64_t r3;
    std::int32_t r4;
    std::string r5;
    std::uint32_t r6;

    RBuffer rbuf(wbuf.data(), wbuf.size());
    rbuf >> r1 >> r2 >> r3 >> r4 >> r5 >> r6;

    success = success &&
              (w1 == r1) &&
              (w2 == r2) &&
              (w3 == r3) &&
              (w4 == r4) &&
              (w5 == r5) &&
              (w6 == r6);

    // Test that reset does what it should do
    wbuf.reset();
    success = success &&
              (wbuf.size() == 0) &&
              (wbuf.CurrPos() == 0);
    return success;
  }

  bool TestRandomAccess(void) {
    bool success = true;
    if (!TestRandomAccessTyped<LittleEndianWriteBuffer,
        LittleEndianReadBuffer>()) {
      fprintf(stderr, "TestRandomAccess: LittleEndian FAILED\n");
      success = false;
    }
    if (!TestRandomAccessTyped<BigEndianWriteBuffer,
        BigEndianReadBuffer>()) {
      fprintf(stderr, "TestRandomAccess: BigEndian FAILED\n");
      success = false;
    }
    return success;
  }

  template <class WBuffer, class RBuffer>
  bool TestReadBufferFactoryTyped(void) {
    bool result = true;

    // Test that the NewBuffer factory method works properly
    WBuffer wbuf;
    const int rbuf_size = 5;
    for (std::int32_t i = 0; i < 3 * rbuf_size; ++i) {
      wbuf << i;
    }

    RBuffer rbuf1(wbuf.data(), rbuf_size * sizeof(std::int32_t));

    for (int i = 0; i < rbuf_size; ++i) {
      std::int32_t j;
      rbuf1 >> j;
      result = result && (i == j);
    }

    khDeleteGuard<EndianReadBuffer>
      rbuf2(rbuf1.NewBuffer(&wbuf[rbuf_size * sizeof(std::int32_t)],
                            rbuf_size * sizeof(std::int32_t)));
    for (int i = 0; i < rbuf_size; ++i) {
      std::int32_t j;
      *rbuf2 >> j;
      result = result && (i+rbuf_size == j);
    }

    khDeleteGuard<EndianReadBuffer> rbuf3(rbuf1.NewBuffer());
    rbuf3->SetValue(std::string(&wbuf[2 * rbuf_size * sizeof(std::int32_t)],
                                rbuf_size * sizeof(std::int32_t)));
    for (int i = 0; i < rbuf_size; ++i) {
      std::int32_t j;
      *rbuf3 >> j;
      result = result && (i+2*rbuf_size == j);
    }

    return result;
  }

  bool TestReadBufferFactory(void) {
    bool success = true;
    if (!TestReadBufferFactoryTyped<LittleEndianWriteBuffer,
        LittleEndianReadBuffer>()) {
      fprintf(stderr, "TestReadBufferFactory: LittleEndian FAILED\n");
      success = false;
    }
    if (!TestReadBufferFactoryTyped<BigEndianWriteBuffer,
        BigEndianReadBuffer>()) {
      fprintf(stderr, "TestReadBufferFactory: BigEndian FAILED\n");
      success = false;
    }
    return success;
  }

  khEndianUnitTest(void) : BaseClass("khEndian") {
    REGISTER(TestDefines);
    REGISTER(TestOldLowLevel);
    REGISTER(TestBasicStream);
    REGISTER(TestFixedString);
    REGISTER(TestEncodeAs);
    REGISTER(TestRandomAccess);
    REGISTER(TestReadBufferFactory);
  }
};

int main(int argc, char *argv[]) {
  khEndianUnitTest tests;

  return tests.Run();
}

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

// Unit tests for version reference generator (classes VerRefGen, _VerRefDef,
// _VerRefDefIterator).

#include <gtest/gtest.h>

#include <string>

#include "common/verref_storage.h"
#include "common/verref_storage_io.h"
#include "common/khStringUtils.h"

namespace {

const std::string kAssetName =
    "Resources/Terrain/SRTM_N05W163.ktasset/CombinedRP.kta/packgen.kta/"
    "packlevel16.kta";

static std::string GetVersionRef(
    const int ver_num,
    const std::string& asset_name = kAssetName) {
  return asset_name + "?version=" + Itoa(ver_num);
}

}  // namespace

void PrintTo(const VerRefGen::iterator& val, ::std::ostream* os) {
  *os << *val;
}

// Tests for class _VerRefDef.
class _VerRefDefTest : public testing::Test {
};

// Tests for class _VerRefGenIterator.
class _VerRefGenIteratorTest : public testing::Test {
};

// Tests for class VerRefGen.
class VerRefGenTest : public testing::Test {
};


// _VerRefDefTest.
// Tests initialization of _VerRefDef.
TEST_F(_VerRefDefTest, Init) {
  std::string name = kAssetName;
  int ver_num = 48;
  _VerRefDef verref_def(name, ver_num);
  std::string result_verref = verref_def;
  std::string expected_verref = GetVersionRef(ver_num, name);
  ASSERT_EQ(expected_verref, result_verref);
}

// Tests initialization of _VerRefDef by reference path.
TEST_F(_VerRefDefTest, RefInit) {
  std::string verref = GetVersionRef(48);

  _VerRefDef verref_def(verref);
  std::string result_verref = verref_def;
  ASSERT_EQ(verref, result_verref);
}

// Tests operators of _VerRefDef.
TEST_F(_VerRefDefTest, Operators) {
  std::string verref_1 = GetVersionRef(10);
  std::string verref_2 = GetVersionRef(15);

  _VerRefDef a(verref_1);
  _VerRefDef b(verref_2);
  _VerRefDef c(verref_2);

  // operators <, >, ==, <=, >=.
  ASSERT_LT(a, b);
  ASSERT_GT(b, a);
  ASSERT_EQ(b, c);
  ASSERT_LE(b, c);
  ASSERT_GE(b, c);

  // operators +, -.
  ASSERT_EQ(b, a + 5);
  ASSERT_EQ(a, b - 5);

  // opertor ++.
  for (int i = 0; i < 5; ++i, ++a) {}
  ASSERT_EQ(b, a);

  // operator --.
  a = _VerRefDef(verref_1);
  for (int i = 0; i < 5; ++i, --b) {}
  ASSERT_EQ(a, b);
}

// _VerRefGenIteratorTest.
// Tests initialization of _VerRefGenIterator.
TEST_F(_VerRefGenIteratorTest, Init) {
  std::string verref = GetVersionRef(12);

  VerRefGen::iterator it = VerRefGen::iterator(verref);

  std::string result_ref = *it;
  ASSERT_EQ(verref, result_ref);
}

// Tests operators of _VerRefDefIterator.
TEST_F(_VerRefGenIteratorTest, Operators) {
  std::string verref = GetVersionRef(12);

  VerRefGen::iterator it = VerRefGen::iterator(verref);

  // operator++, operator--.
  ++it;
  std::string expected_ref = GetVersionRef(11);
  ASSERT_EQ(expected_ref, *it);
  --it;
  ASSERT_EQ(verref, *it);

  // operator+=, operator-=, operator+, operator-.
  VerRefGen::iterator it_a = it + 3;
  expected_ref = GetVersionRef(9);
  ASSERT_EQ(expected_ref, *it_a);

  it_a = it - 3;
  expected_ref = GetVersionRef(15);
  ASSERT_EQ(expected_ref, *it_a);

  // operators <, >, <=, >=.
  VerRefGen::iterator it_b = it_a + 5;
  ASSERT_LT(it_a, it_b);
  ASSERT_GT(it_b, it_a);

  it_b = it_a;
  ASSERT_LE(it_a, it_b);
  ASSERT_GE(it_b, it_a);
}

// VerRefGenTest.
// Tests initialization of VerRefGen, begin(), end().
TEST_F(VerRefGenTest, Init) {
  size_t expected_size = 48;
  std::string verref(GetVersionRef(expected_size));
  VerRefGen verref_gen(verref);
  std::string result_verref = ToString(verref_gen);
  ASSERT_EQ(verref, result_verref);
  ASSERT_EQ(expected_size, verref_gen.size());

  // begin(), end().
  std::string expected_verref = GetVersionRef(expected_size);
  ASSERT_EQ(expected_verref, *verref_gen.begin());
  expected_verref = GetVersionRef(0);
  ASSERT_EQ(expected_verref, *verref_gen.end());
}

// Tests insert(), size() and tracing.
TEST_F(VerRefGenTest, InsertAndIterator) {
  VerRefGen verref_gen;

  size_t expected_size = 10;
  for (size_t i = 0; i < expected_size; ++i) {
    verref_gen.insert(verref_gen.begin(), GetVersionRef(i + 1));
  }

  std::string result_verref = ToString(verref_gen);
  std::string expected_verref = GetVersionRef(10);
  ASSERT_EQ(expected_verref, result_verref);
  ASSERT_EQ(expected_size, verref_gen.size());

  int i = expected_size;

  for (VerRefGen::const_iterator it = verref_gen.begin();
       it != verref_gen.end(); ++it, --i) {
    ASSERT_EQ(GetVersionRef(i), *it);
  }
  ASSERT_EQ(GetVersionRef(0), *verref_gen.end());
}

// Tests front(), back().
TEST_F(VerRefGenTest, FrontBack) {
  VerRefGen verref_gen;

  size_t expected_size = 10;
  for (size_t i = 0; i < expected_size; ++i) {
    verref_gen.insert(verref_gen.begin(), GetVersionRef(i + 1));
  }

  ASSERT_EQ(expected_size, verref_gen.size());

  std::string front_val = verref_gen.front();
  std::string expected_verref = GetVersionRef(expected_size);
  ASSERT_EQ(expected_verref, front_val);

  std::string back_val = verref_gen.back();
  expected_verref = GetVersionRef(1);
  ASSERT_EQ(expected_verref, back_val);
}


int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

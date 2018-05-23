// Copyright 2017 the Open GEE Contributors
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

#include <gtest/gtest.h>
#include "rewritedbroot.h"
#include <string>

using namespace std;

int translationEntryId = 0;

TEST(ReplaceReferencedKmlTest, NoKmlTest) {
  geProtoDbroot dbroot;
  string before = dbroot.ToTextString();
  ReplaceReferencedKml("http://localhost", "", false, &dbroot);
  string after = dbroot.ToTextString();
  ASSERT_EQ(before, after);
}

void addKmlUrlAsValue(const string & url, geProtoDbroot * dbroot) {
  keyhole::dbroot::NestedFeatureProto * nested = dbroot->add_nested_feature();
  keyhole::dbroot::StringIdOrValueProto * kmlUrl = nested->mutable_kml_url();
  kmlUrl->set_value(url);
}

TEST(ReplaceReferencedKmlTest, NestedEntryReplaceTest) {
  const string BEFORE_URL_1 = "http://server.name/kmlfile.kmz";
  const string BEFORE_URL_2 = "http://other.server.name/newfile.kml";
  const string AFTER_BASE_URL = "https://different.server";
  const string AFTER_URL_1 = AFTER_BASE_URL + "/dbroot_kml_000.kml";
  const string AFTER_URL_2 = AFTER_BASE_URL + "/dbroot_kml_001.kml";
  geProtoDbroot dbroot;
  
  addKmlUrlAsValue(BEFORE_URL_1, &dbroot);
  dbroot.add_nested_feature();
  addKmlUrlAsValue(BEFORE_URL_2, &dbroot);
  dbroot.add_nested_feature();
  dbroot.add_nested_feature();

  ASSERT_TRUE(dbroot.nested_feature(0).has_kml_url());
  ASSERT_EQ(dbroot.nested_feature(0).kml_url().value(), BEFORE_URL_1);
  ASSERT_TRUE(dbroot.nested_feature(2).has_kml_url());
  ASSERT_EQ(dbroot.nested_feature(2).kml_url().value(), BEFORE_URL_2);

  ReplaceReferencedKml(AFTER_BASE_URL, "", false, &dbroot);

  ASSERT_EQ(dbroot.nested_feature(0).kml_url().value(), AFTER_URL_1);
  ASSERT_FALSE(dbroot.nested_feature(1).has_kml_url());
  ASSERT_EQ(dbroot.nested_feature(2).kml_url().value(), AFTER_URL_2);
  ASSERT_FALSE(dbroot.nested_feature(3).has_kml_url());
  ASSERT_FALSE(dbroot.nested_feature(4).has_kml_url());
}

TEST(ReplaceReferencedKmlTest, NestedEntryPreserveTest) {
  const string FILE_1 = "kmlfile.kmz";
  const string FILE_2 = "newfile.kml";
  const string BEFORE_URL_1 = "http://server.name/path_a/" + FILE_1;
  const string BEFORE_URL_2 = "http://other.server.name/path_b/" + FILE_2;
  const string AFTER_BASE_URL = "https://different.server/path_c";
  const string AFTER_URL_1 = AFTER_BASE_URL + "/" + FILE_1;
  const string AFTER_URL_2 = AFTER_BASE_URL + "/" + FILE_2;
  geProtoDbroot dbroot;
  
  dbroot.add_nested_feature();
  dbroot.add_nested_feature();
  addKmlUrlAsValue(BEFORE_URL_1, &dbroot);
  dbroot.add_nested_feature();
  addKmlUrlAsValue(BEFORE_URL_2, &dbroot);

  ASSERT_TRUE(dbroot.nested_feature(2).has_kml_url());
  ASSERT_EQ(dbroot.nested_feature(2).kml_url().value(), BEFORE_URL_1);
  ASSERT_TRUE(dbroot.nested_feature(4).has_kml_url());
  ASSERT_EQ(dbroot.nested_feature(4).kml_url().value(), BEFORE_URL_2);

  ReplaceReferencedKml(AFTER_BASE_URL, "", true, &dbroot);

  ASSERT_FALSE(dbroot.nested_feature(0).has_kml_url());
  ASSERT_FALSE(dbroot.nested_feature(1).has_kml_url());
  ASSERT_EQ(dbroot.nested_feature(2).kml_url().value(), AFTER_URL_1);
  ASSERT_FALSE(dbroot.nested_feature(3).has_kml_url());
  ASSERT_EQ(dbroot.nested_feature(4).kml_url().value(), AFTER_URL_2);
}

keyhole::dbroot::StringEntryProto * addTranslationEntry(geProtoDbroot * dbroot) {
  keyhole::dbroot::StringEntryProto * translationEntry = dbroot->add_translation_entry();
  translationEntry->set_string_id(translationEntryId);
  ++translationEntryId;
  return translationEntry;
}

int addKmlUrlAsTranslationEntry(const string & url, geProtoDbroot * dbroot) {
  keyhole::dbroot::StringEntryProto * translationEntry = addTranslationEntry(dbroot);
  int id = translationEntry->string_id();
  translationEntry->set_string_value(url);
  keyhole::dbroot::NestedFeatureProto * nested = dbroot->add_nested_feature();
  keyhole::dbroot::StringIdOrValueProto * kmlUrl = nested->mutable_kml_url();
  kmlUrl->set_string_id(id);
  return id;
}

TEST(ReplaceReferencedKmlTest, TranslationEntryReplaceTest) {
  const string BEFORE_URL_1 = "http://server.name/kmlfile.kmz";
  const string BEFORE_URL_2 = "http://other.server.name/newfile.kml";
  const string AFTER_BASE_URL = "https://different.server";
  const string AFTER_URL_1 = AFTER_BASE_URL + "/dbroot_kml_000.kml";
  const string AFTER_URL_2 = AFTER_BASE_URL + "/dbroot_kml_001.kml";
  geProtoDbroot dbroot;
  
  translationEntryId = 0;
  
  addTranslationEntry(&dbroot);
  int id1 = addKmlUrlAsTranslationEntry(BEFORE_URL_1, &dbroot);
  addTranslationEntry(&dbroot);
  addTranslationEntry(&dbroot);
  dbroot.add_nested_feature();
  dbroot.add_nested_feature();
  dbroot.add_nested_feature();
  int id2 = addKmlUrlAsTranslationEntry(BEFORE_URL_2, &dbroot);
  addTranslationEntry(&dbroot);

  ASSERT_TRUE(dbroot.nested_feature(0).has_kml_url());
  ASSERT_EQ(dbroot.nested_feature(0).kml_url().string_id(), id1);
  ASSERT_EQ(dbroot.translation_entry(id1).string_value(), BEFORE_URL_1);
  ASSERT_TRUE(dbroot.nested_feature(4).has_kml_url());
  ASSERT_EQ(dbroot.nested_feature(4).kml_url().string_id(), id2);
  ASSERT_EQ(dbroot.translation_entry(id2).string_value(), BEFORE_URL_2);

  ReplaceReferencedKml(AFTER_BASE_URL, "", false, &dbroot);

  ASSERT_EQ(dbroot.nested_feature(0).kml_url().string_id(), id1);
  ASSERT_EQ(dbroot.translation_entry(id1).string_value(), AFTER_URL_1);
  ASSERT_FALSE(dbroot.nested_feature(1).has_kml_url());
  ASSERT_FALSE(dbroot.nested_feature(2).has_kml_url());
  ASSERT_FALSE(dbroot.nested_feature(3).has_kml_url());
  ASSERT_EQ(dbroot.nested_feature(4).kml_url().string_id(), id2);
  ASSERT_EQ(dbroot.translation_entry(id2).string_value(), AFTER_URL_2);
}

TEST(ReplaceReferencedKmlTest, TranslationEntryPreserveTest) {
  const string FILE_1 = "kmlfile.kmz";
  const string FILE_2 = "newfile.kml";
  const string BEFORE_URL_1 = "http://server.name/path_a/" + FILE_1;
  const string BEFORE_URL_2 = "http://other.server.name/path_b/" + FILE_2;
  const string AFTER_BASE_URL = "https://different.server/path_c";
  const string AFTER_URL_1 = AFTER_BASE_URL + "/" + FILE_1;
  const string AFTER_URL_2 = AFTER_BASE_URL + "/" + FILE_2;
  geProtoDbroot dbroot;

  translationEntryId = 0;

  dbroot.add_nested_feature();
  dbroot.add_nested_feature();
  int id1 = addKmlUrlAsTranslationEntry(BEFORE_URL_1, &dbroot);
  int id2 = addKmlUrlAsTranslationEntry(BEFORE_URL_2, &dbroot);
  dbroot.add_nested_feature();
  addTranslationEntry(&dbroot);
  addTranslationEntry(&dbroot);
  addTranslationEntry(&dbroot);

  ASSERT_TRUE(dbroot.nested_feature(2).has_kml_url());
  ASSERT_EQ(dbroot.nested_feature(2).kml_url().string_id(), id1);
  ASSERT_EQ(dbroot.translation_entry(id1).string_value(), BEFORE_URL_1);
  ASSERT_TRUE(dbroot.nested_feature(3).has_kml_url());
  ASSERT_EQ(dbroot.nested_feature(3).kml_url().string_id(), id2);
  ASSERT_EQ(dbroot.translation_entry(id2).string_value(), BEFORE_URL_2);

  ReplaceReferencedKml(AFTER_BASE_URL, "", true, &dbroot);

  ASSERT_FALSE(dbroot.nested_feature(0).has_kml_url());
  ASSERT_FALSE(dbroot.nested_feature(1).has_kml_url());
  ASSERT_EQ(dbroot.nested_feature(2).kml_url().string_id(), id1);
  ASSERT_EQ(dbroot.translation_entry(id1).string_value(), AFTER_URL_1);
  ASSERT_EQ(dbroot.nested_feature(3).kml_url().string_id(), id2);
  ASSERT_EQ(dbroot.translation_entry(id2).string_value(), AFTER_URL_2);
  ASSERT_FALSE(dbroot.nested_feature(4).has_kml_url());
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

// Copyright 2018 Google Inc.
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


// Unit Tests for DB manifest

#include <string>
#include <gtest/gtest.h>

#include "common/notify.h"
#include "common/khSimpleException.h"
#include "common/geFilePool.h"
#include "common/khConstants.h"
#include "common/khxml/khxml.h"
#include "common/khxml/khdom.h"
#include "fusion/dbmanifest/dbmanifest.h"

class DbManifestTest : public testing::Test {

  protected:

  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

  public:

  geFilePool file_pool_;
  const std::string test_folder_;
  const std::string vol_root_;
  const std::string assets_root_;
  const std::string projects_root_;
  const std::string vector_project_root_;
  const std::string fake_vector_project_name_;
  const std::string fake_vector_project_root_;
  const std::string poi_file_path_;
  const std::string poi_data_file_path_;
  const std::string dbs_root_;
  const std::string fake_db_name_;
  const std::string fake_db_root_;
  const std::string ge_db_key_root_;
  const std::string ge_unified_index_root_;
  const std::string db_path_v1_;
  const std::string toc_file_spec1_;
  const std::string toc_path1_;
  const std::string db_header_path_;
 
  DbManifestTest()
    :test_folder_(khCreateTmpDir("geManifestTesting"))
    ,vol_root_(khComposePath(test_folder_, "gevol_test"))
    ,assets_root_(khComposePath(vol_root_, "assets"))
    ,projects_root_(khComposePath(assets_root_, "Projects"))
    ,vector_project_root_(khComposePath(assets_root_, "Vector"))
    ,fake_vector_project_name_("fakeVectorProject")
    ,fake_vector_project_root_(khComposePath(vector_project_root_, fake_vector_project_name_ + kVectorProjectSuffix))
    ,poi_file_path_(khComposePath(fake_vector_project_root_, "layer005.kva/poi.kva/ver001/poifile"))
    ,poi_data_file_path_(khComposePath(fake_vector_project_root_, "layer005.kva/poi.kva/ver001/poifile0"))
    ,dbs_root_(khComposePath(assets_root_, "Databases"))
    ,fake_db_name_("fakeDB")
    ,fake_db_root_(khComposePath(dbs_root_, fake_db_name_ + kDatabaseSuffix))
    ,ge_db_key_root_(khComposePath(fake_db_root_, kGedbKey))
    ,ge_unified_index_root_(khComposePath(fake_db_root_, kUnifiedIndexKey, "ver001/unified.geindex"))
    ,db_path_v1_(khComposePath(ge_db_key_root_, "ver001", kGedbBase))
    ,toc_file_spec1_(kPostamblePrefix + kDefaultLocaleSuffix)
    ,toc_path1_(khComposePath(db_path_v1_, kDbrootsDir, toc_file_spec1_))
    ,db_header_path_(khComposePath(db_path_v1_, kHeaderXmlFile))
    {}

  ~DbManifestTest() {
    khPruneDir(test_folder_);
  }

  void CreateFakeDbFolderWithPoiData(const std::string& file_loc = std::string()) {
    std::string actual_db_header_path;
    std::string actual_poi_file_path;
    std::string actual_poi_data_file_path;

    if (!file_loc.empty()) {
      actual_db_header_path = khComposePath(file_loc, db_header_path_);
      actual_poi_file_path = khComposePath(file_loc, poi_file_path_);
      actual_poi_data_file_path = khComposePath(file_loc, poi_data_file_path_);
    }
    else {
      actual_db_header_path = db_header_path_;
      actual_poi_file_path = poi_file_path_;
      actual_poi_data_file_path = poi_data_file_path_;
    }

    // Create a header file for the DB that has one POI file it points to.
    khDomDeleteGuard dom(TransferOwnership(CreateEmptyDocument("DbHeader")));
    khxml::DOMElement* root_elm = dom->getDocumentElement();
    AddElement(root_elm, "index_path", ge_unified_index_root_);
    khxml::DOMElement* toc_paths_elm = dom->createElement(ToXMLStr("toc_paths"));
    root_elm->appendChild(toc_paths_elm);
    AddElement(toc_paths_elm, "toc_path", toc_path1_);
    khxml::DOMElement* search_tabs_elm = dom->createElement(ToXMLStr("search_tabs"));
    root_elm->appendChild(search_tabs_elm);
    khxml::DOMElement* poi_file_paths_elm = dom->createElement(ToXMLStr("poi_file_paths"));
    root_elm->appendChild(poi_file_paths_elm);
    AddElement(poi_file_paths_elm, "poi_file_path", poi_file_path_);
    khxml::DOMElement* icons_dirs_elm = dom->createElement(ToXMLStr("icons_dirs"));
    root_elm->appendChild(icons_dirs_elm);
    AddElement(root_elm, "database_version", "3");
    AddElement(root_elm, "db_type", "TYPE_GEDB");
    AddElement(root_elm, "use_google_imagery", "0");
    AddElement(root_elm, "is_mercator", "0");
    AddElement(root_elm, "fusion_host", "fusion.tst");

    // write out the header file
    khEnsureParentDir(actual_db_header_path);
    WriteDocument(dom, actual_db_header_path);

    // Create the fake poi file
    khDomDeleteGuard poi_file_dom(TransferOwnership(CreateEmptyDocument("POISearchFile")));
    khxml::DOMElement* poi_file_root_elm = poi_file_dom->getDocumentElement();
    khxml::DOMElement* search_table_values_elm = poi_file_dom->createElement(ToXMLStr("SearchTableValues"));
    poi_file_root_elm->appendChild(search_table_values_elm);
    AddElement(search_table_values_elm, "SearchDataFile", poi_data_file_path_);

    // write out the poi file
    khEnsureParentDir(actual_poi_file_path);
    WriteDocument(poi_file_dom, actual_poi_file_path);

    // write out the poi data file (empty file for these tests)
    khMakeEmptyFile(actual_poi_data_file_path);
  }
};

// Basic constructor tests
TEST_F(DbManifestTest, EnforcesAbsolutePath) {
  std::string not_an_absolute_path("../not/an/absolute/path");
  EXPECT_THROW(TransferOwnership(new DbManifest(&not_an_absolute_path)), khSimpleException);
}

TEST_F(DbManifestTest, AssetRootDB) {
  khDeleteGuard<DbManifest> db_manifest;
  std::string db_path(db_path_v1_);

  CreateFakeDbFolderWithPoiData();

  EXPECT_NO_THROW(db_manifest = TransferOwnership(new DbManifest(&db_path)));
  EXPECT_EQ(db_path_v1_, db_path); // should be unchanged

  // TODO(RAW): add more checking
}

// TODO(RAW): cover all use cases: see comments in dbmanifest.cpp

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

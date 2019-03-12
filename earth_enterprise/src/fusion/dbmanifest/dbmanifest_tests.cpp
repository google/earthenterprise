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

struct MockGetIndextManifest : public DbManifest::IndextManifest {
  std::vector<ManifestEntry> manifest_;
  void operator()(geFilePool& /*filePool*/,
                  const std::string& /*index_path*/,
                  std::vector<ManifestEntry> &manifest,
                  const std::string& /*tmpdir*/,
                  const std::string& /*disconnect_prefix*/,
                  const std::string& /*publish_prefix*/) {
                    manifest = manifest_; // for now we keep things simple
                  }
};

bool LessThan(const ManifestEntry& lhs_entry, const ManifestEntry& rhs_entry) {
  return (lhs_entry.orig_path < rhs_entry.orig_path);
}

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
  const std::string dbs_root_;
  const std::string mock_db_name_;
  const std::string mock_db_root_;
  const std::string ge_db_key_root_;
  const std::string ge_unified_index_root_;
  const std::string db_path_v1_;
  const std::string toc_file_spec1_;
  const std::string toc_path_;
  const std::string db_header_path_;
 
  DbManifestTest()
    :test_folder_(khCreateTmpDir("geManifestTesting"))
    ,vol_root_(khComposePath(test_folder_, "gevol_test"))
    ,assets_root_(khComposePath(vol_root_, "assets"))
    ,dbs_root_(khComposePath(assets_root_, "Databases"))
    ,mock_db_name_("mockDB")
    ,mock_db_root_(khComposePath(dbs_root_, mock_db_name_ + kDatabaseSuffix))
    ,ge_db_key_root_(khComposePath(mock_db_root_, kGedbKey))
    ,ge_unified_index_root_(khComposePath(mock_db_root_, kUnifiedIndexKey, "ver001/unified.geindex"))
    ,db_path_v1_(khComposePath(ge_db_key_root_, "ver001", kGedbBase))
    ,toc_file_spec1_(kPostamblePrefix + "." + kDefaultLocaleSuffix)
    ,toc_path_(khComposePath(db_path_v1_, kDbrootsDir, toc_file_spec1_))
    ,db_header_path_(khComposePath(db_path_v1_, kHeaderXmlFile))
    {}

  ~DbManifestTest() {
    khPruneDir(test_folder_);
  }

  struct PoiEntry {
    std::string poi_file;
    std::vector<std::string> poi_data_files;
  };

  struct MockDbCreationParameters {
    std::string prefix_loc;
    std::vector<PoiEntry> poi_entries;
    std::vector<std::string> icon_files;
  };

  // nice for debugging when things go wrong
  // NOTE: none of this shows up unless you do `export KH_NFY_LEVEL=6`
  // before running unit test
  void DumpManifest(const std::vector<ManifestEntry>& manifest) {
    for(size_t idx=0; idx < manifest.size(); ++idx) {
      const ManifestEntry& entry = manifest[idx];
      notify(NFY_WARN, "org=%s:cur=%s;data_size=%ld",
        entry.orig_path.c_str(), entry.current_path.c_str(), entry.data_size);
      for(size_t jdx=0; jdx < entry.dependents.size(); ++jdx) {
        const ManifestEntry& dep_entry = entry.dependents[jdx];
        notify(NFY_WARN, "  dep_org=%s:dep_cur=%s;dep_data_size=%ld",
          dep_entry.orig_path.c_str(), dep_entry.current_path.c_str(), dep_entry.data_size);
      }
    }
  }

  void CreateMockDbFolderWithPoiData(const MockDbCreationParameters& params = MockDbCreationParameters()) {
    std::string actual_db_header_path;
    std::string actual_toc_path;

    if (!params.prefix_loc.empty()) {
      actual_db_header_path = khComposePath(params.prefix_loc, db_header_path_);
      actual_toc_path = khComposePath(params.prefix_loc, toc_path_);
    }
    else {
      actual_db_header_path = db_header_path_;
      actual_toc_path = toc_path_;
    }

    // Create a header file for the DB that has one POI file it points to.
    std::unique_ptr<GEDocument> dom = CreateEmptyDocument("DbHeader");
    khxml::DOMElement* root_elm = dom->getDocumentElement();
    AddElement(root_elm, "index_path", ge_unified_index_root_);
    khxml::DOMElement* toc_paths_elm =
        root_elm->getOwnerDocument()->createElement(ToXMLStr("toc_paths"));
    root_elm->appendChild(toc_paths_elm);
    AddElement(toc_paths_elm, "toc_path", toc_path_);
    khxml::DOMElement* search_tabs_elm =
        root_elm->getOwnerDocument()->createElement(ToXMLStr("search_tabs"));
    root_elm->appendChild(search_tabs_elm);
    khxml::DOMElement* poi_file_paths_elm =
        root_elm->getOwnerDocument()->createElement(ToXMLStr("poi_file_paths"));
    root_elm->appendChild(poi_file_paths_elm);
    khxml::DOMElement* icons_dirs_elm =
        root_elm->getOwnerDocument()->createElement(ToXMLStr("icons_dirs"));
    root_elm->appendChild(icons_dirs_elm);
    AddElement(root_elm, "database_version", "3");
    AddElement(root_elm, "db_type", "TYPE_GEDB");
    AddElement(root_elm, "use_google_imagery", "0");
    AddElement(root_elm, "is_mercator", "0");
    AddElement(root_elm, "fusion_host", "fusion.tst");

    std::set<std::string> icon_paths; // no hash maps without C++11...

    for (size_t idx = 0; idx < params.icon_files.size(); ++idx) {
      std::string actual_icon_loc;
      std::string parent_path = khDirname(params.icon_files[idx]);
      if (!params.prefix_loc.empty()) {
        actual_icon_loc = khComposePath(params.prefix_loc, params.icon_files[idx]);
      } else {
        actual_icon_loc = params.icon_files[idx];
      }
      // we can just write empty files for testing purposes
      khEnsureParentDir(actual_icon_loc);
      khMakeEmptyFile(actual_icon_loc);

      if (icon_paths.end() == icon_paths.find(parent_path)) {
        icon_paths.insert(parent_path);
        // add path to header xml
        AddElement(icons_dirs_elm, "icons_dir", parent_path);
      }
    }

    for (size_t idx = 0; idx < params.poi_entries.size(); ++idx) {
      const PoiEntry& poi_entry = params.poi_entries[idx];
      std::string actual_poi_file_path;

      if (!params.prefix_loc.empty()) {
        actual_poi_file_path = khComposePath(params.prefix_loc, poi_entry.poi_file);
      }
      else {
        actual_poi_file_path = poi_entry.poi_file;
      }
      // add element to header xml
      AddElement(poi_file_paths_elm, "poi_file_path", poi_entry.poi_file);

      // Create the mock poi file
      std::unique_ptr<GEDocument> poi_file_dom = CreateEmptyDocument("POISearchFile");
      khxml::DOMElement* poi_file_root_elm = poi_file_dom->getDocumentElement();
      khxml::DOMElement* search_table_values_elm =
          poi_file_root_elm->getOwnerDocument()->createElement(ToXMLStr("SearchTableValues"));
      poi_file_root_elm->appendChild(search_table_values_elm);

      for (size_t jdx = 0; jdx < poi_entry.poi_data_files.size(); ++jdx) {
        const std::string& poi_data_file_path = poi_entry.poi_data_files[jdx];
        std::string actual_poi_data_file_path;

        if (!params.prefix_loc.empty()) {
          actual_poi_data_file_path = khComposePath(params.prefix_loc, poi_data_file_path);
        }
        else {
          actual_poi_data_file_path = poi_data_file_path;
        }
        AddElement(search_table_values_elm, "SearchDataFile", poi_data_file_path);

        // write out the poi data file (empty file for these tests)
        khEnsureParentDir(actual_poi_data_file_path);
        khMakeEmptyFile(actual_poi_data_file_path);
      }

      // write out the poi file
      khEnsureParentDir(actual_poi_file_path);
      WriteDocument(poi_file_dom.get(), actual_poi_file_path);
    }

    // write out the header file
    khEnsureParentDir(actual_db_header_path);
    WriteDocument(dom.get(), actual_db_header_path);

    // create a dummy empty index file
    khEnsureParentDir(actual_toc_path);
    khMakeEmptyFile(actual_toc_path);
  }

  bool IsEquivalent(std::vector<ManifestEntry>* manifest1,
                    std::vector<ManifestEntry>* manifest2) {

    // sort no matter what.  Makes things easier to figure out
    // what is missing when we dump the manifests
    std::sort(manifest1->begin(), manifest1->end(), LessThan);
    std::sort(manifest2->begin(), manifest2->end(), LessThan);
    if (manifest1->size() != manifest2->size()) {
      return false;
    }

    for (size_t idx=0; idx < manifest1->size(); ++idx) {
      ManifestEntry& entry1 = manifest1->at(idx);
      ManifestEntry& entry2 = manifest2->at(idx);
      if (IsEquivalent(&(entry1.dependents), &(entry2.dependents)) == false ||
          entry1.orig_path != entry2.orig_path ||
          entry1.current_path != entry2.current_path ||
          entry1.data_size != entry2.data_size) {
            return false;
      }
    }

    return true;
  }
  
};

// Basic constructor tests
TEST_F(DbManifestTest, EnforcesAbsolutePath) {
  std::string not_an_absolute_path("../not/an/absolute/path");
  EXPECT_THROW(TransferOwnership(new DbManifest(&not_an_absolute_path)), khSimpleException);
}

// Calling GetPushManifest for asset root test.  This test only gets file entries from 
// the mocked out get_index_manifest() call and has no Poi files or icons in the mocked 
// out DB folder.  Also uses the same manifest vector for both steam and search manifest.
TEST_F(DbManifestTest, AssetRootGetPushManifestNoPoiNoIconsSingleManifest) {
  khDeleteGuard<MockGetIndextManifest>
    mock_get_index_manifest(TransferOwnership(new MockGetIndextManifest()));
   khDeleteGuard<DbManifest::IndextManifest> get_index_manifest;
  khDeleteGuard<DbManifest> db_manifest;
  std::string db_path(db_path_v1_);
  std::vector<ManifestEntry> single_manifest;
  std::vector<ManifestEntry> expected_manifest;

  CreateMockDbFolderWithPoiData();

  // initialize mock get index manifest call
  // create unitialized entries to avoid OS calls for file sizes that don't exist
  mock_get_index_manifest->manifest_.resize(3);
  // for testing purposes keep it simple and stupid.  No i/o is done to these files
  mock_get_index_manifest->manifest_[0].orig_path = khComposePath(assets_root_, "SomeDataDir/somedatafile1");
  mock_get_index_manifest->manifest_[0].current_path = mock_get_index_manifest->manifest_[0].orig_path;
  mock_get_index_manifest->manifest_[0].data_size = 1024;
  mock_get_index_manifest->manifest_[1].orig_path = khComposePath(assets_root_, "SomeDataDir/somedatafile2");
  mock_get_index_manifest->manifest_[1].current_path = mock_get_index_manifest->manifest_[0].orig_path;
  mock_get_index_manifest->manifest_[1].data_size = 1024;
  mock_get_index_manifest->manifest_[2].orig_path = khComposePath(assets_root_, "SomeDataDir/somedatafile3");
  mock_get_index_manifest->manifest_[2].current_path = mock_get_index_manifest->manifest_[0].orig_path;
  mock_get_index_manifest->manifest_[2].data_size = 1024;

  expected_manifest = mock_get_index_manifest->manifest_;
  expected_manifest.resize(expected_manifest.size() + 1);
  expected_manifest.back().orig_path = toc_path_;
  expected_manifest.back().current_path = toc_path_;
  expected_manifest.back().data_size = 0;
  expected_manifest.push_back(ManifestEntry(db_header_path_));

  // change pointer to base type
  get_index_manifest = TransferOwnership(mock_get_index_manifest);

  EXPECT_NO_THROW(db_manifest = TransferOwnership(new DbManifest(&db_path, get_index_manifest)));
  EXPECT_EQ(db_path_v1_, db_path); // should be unchanged
  EXPECT_TRUE(db_manifest->Prefix().empty());

  // calling method under test
  db_manifest->GetPushManifest(file_pool_, &single_manifest, &single_manifest, "", "");

  bool is_equivalent = IsEquivalent(&expected_manifest, &single_manifest);
  if (!is_equivalent) {
    notify(NFY_WARN, "Dumping expected manifest:");
    DumpManifest(expected_manifest);
    notify(NFY_WARN, "Dumping actual manifest:");
    DumpManifest(single_manifest);
  }

  EXPECT_TRUE(is_equivalent);
}

// Calling GetPushManifest for asset root test.  This test gets file entries from the mocked out 
// get_index_manifest() call and also creates fake Poi files and icon files in the mocked out 
// DB folder.  Also uses the same manifest vector for both steam and search manifest.
TEST_F(DbManifestTest, AssetRootGetPushManifestSingleManifest) {
  MockDbCreationParameters db_create_params;
  khDeleteGuard<MockGetIndextManifest>
    mock_get_index_manifest(TransferOwnership(new MockGetIndextManifest()));
   khDeleteGuard<DbManifest::IndextManifest> get_index_manifest;
  khDeleteGuard<DbManifest> db_manifest;
  std::string db_path(db_path_v1_);
  std::vector<ManifestEntry> single_manifest;
  std::vector<ManifestEntry> expected_manifest;

  db_create_params.poi_entries.resize(1);
  db_create_params.poi_entries[0].poi_file = khComposePath(assets_root_, "SomeDataDir/poidata/poifile");;
  db_create_params.poi_entries[0].poi_data_files.push_back(khComposePath(assets_root_, "SomeDataDir/poidata/poifile0"));
  db_create_params.icon_files.push_back(khComposePath(assets_root_, "SomeDataDir/inconpath1/icon1.png"));
  db_create_params.icon_files.push_back(khComposePath(assets_root_, "SomeDataDir/inconpath1/icon2.png"));
  db_create_params.icon_files.push_back(khComposePath(assets_root_, "SomeDataDir/inconpath1/icon3.png"));
  db_create_params.icon_files.push_back(khComposePath(assets_root_, "SomeDataDir/inconpath2/icon4.png"));
  db_create_params.icon_files.push_back(khComposePath(assets_root_, "SomeDataDir/inconpath2/icon5.gnp"));  // negitive test

  CreateMockDbFolderWithPoiData(db_create_params);

  // initialize mock get index manifest call
  // create unitialized entries to avoid OS calls for file sizes that don't exist
  mock_get_index_manifest->manifest_.resize(3);
  // for testing purposes keep it simple and stupid.  No i/o is done to these files
  mock_get_index_manifest->manifest_[0].orig_path = khComposePath(assets_root_, "SomeDataDir/somedatafile1");
  mock_get_index_manifest->manifest_[0].current_path = mock_get_index_manifest->manifest_[0].orig_path;
  mock_get_index_manifest->manifest_[0].data_size = 1024;
  mock_get_index_manifest->manifest_[1].orig_path = khComposePath(assets_root_, "SomeDataDir/somedatafile2");
  mock_get_index_manifest->manifest_[1].current_path = mock_get_index_manifest->manifest_[0].orig_path;
  mock_get_index_manifest->manifest_[1].data_size = 1024;
  mock_get_index_manifest->manifest_[2].orig_path = khComposePath(assets_root_, "SomeDataDir/somedatafile3");
  mock_get_index_manifest->manifest_[2].current_path = mock_get_index_manifest->manifest_[0].orig_path;
  mock_get_index_manifest->manifest_[2].data_size = 1024;

  expected_manifest = mock_get_index_manifest->manifest_;
  expected_manifest.push_back(ManifestEntry(db_create_params.poi_entries[0].poi_file));
  expected_manifest.back().dependents.push_back(ManifestEntry(db_create_params.poi_entries[0].poi_data_files[0]));
  expected_manifest.push_back(ManifestEntry(toc_path_));
  expected_manifest.push_back(ManifestEntry(db_header_path_));
  expected_manifest.push_back(ManifestEntry(db_create_params.icon_files[0]));
  expected_manifest.push_back(ManifestEntry(db_create_params.icon_files[1]));
  expected_manifest.push_back(ManifestEntry(db_create_params.icon_files[2]));
  expected_manifest.push_back(ManifestEntry(db_create_params.icon_files[3]));

  // change pointer to base type
  get_index_manifest = TransferOwnership(mock_get_index_manifest);

  EXPECT_NO_THROW(db_manifest = TransferOwnership(new DbManifest(&db_path, get_index_manifest)));
  EXPECT_EQ(db_path_v1_, db_path); // should be unchanged
  EXPECT_TRUE(db_manifest->Prefix().empty());

  // calling method under test
  db_manifest->GetPushManifest(file_pool_, &single_manifest, &single_manifest, "", "");

  bool is_equivalent = IsEquivalent(&expected_manifest, &single_manifest);
  if (!is_equivalent) {
    notify(NFY_WARN, "Dumping expected manifest:");
    DumpManifest(expected_manifest);
    notify(NFY_WARN, "Dumping actual manifest:");
    DumpManifest(single_manifest);
  }

  EXPECT_TRUE(is_equivalent);
}

// Calling GetPushManifest for asset root test.  This test gets file entries from the mocked
// out get_index_manifest() call and also creates fake Poi files and icon files in the mocked
// out DB folder.  Also uses the seperate manifest vectors for steam and search manifest.
TEST_F(DbManifestTest, AssetRootGetPushManifest) {
  MockDbCreationParameters db_create_params;
  khDeleteGuard<MockGetIndextManifest>
    mock_get_index_manifest(TransferOwnership(new MockGetIndextManifest()));
   khDeleteGuard<DbManifest::IndextManifest> get_index_manifest;
  khDeleteGuard<DbManifest> db_manifest;
  std::string db_path(db_path_v1_);
  std::vector<ManifestEntry> stream_manifest;
  std::vector<ManifestEntry> search_manifest;
  std::vector<ManifestEntry> expected_stream_manifest;
  std::vector<ManifestEntry> expected_search_manifest;

  db_create_params.poi_entries.resize(1);
  db_create_params.poi_entries[0].poi_file = khComposePath(assets_root_, "SomeDataDir/poidata/poifile");;
  db_create_params.poi_entries[0].poi_data_files.push_back(khComposePath(assets_root_, "SomeDataDir/poidata/poifile0"));
  db_create_params.icon_files.push_back(khComposePath(assets_root_, "SomeDataDir/inconpath1/icon1.png"));
  db_create_params.icon_files.push_back(khComposePath(assets_root_, "SomeDataDir/inconpath1/icon2.png"));
  db_create_params.icon_files.push_back(khComposePath(assets_root_, "SomeDataDir/inconpath1/icon3.png"));
  db_create_params.icon_files.push_back(khComposePath(assets_root_, "SomeDataDir/inconpath2/icon4.png"));
  db_create_params.icon_files.push_back(khComposePath(assets_root_, "SomeDataDir/inconpath2/icon5.gnp"));  // negitive test

  CreateMockDbFolderWithPoiData(db_create_params);

  // initialize mock get index manifest call
  // create unitialized entries to avoid OS calls for file sizes that don't exist
  mock_get_index_manifest->manifest_.resize(3);
  // for testing purposes keep it simple and stupid.  No i/o is done to these files
  mock_get_index_manifest->manifest_[0].orig_path = khComposePath(assets_root_, "SomeDataDir/somedatafile1");
  mock_get_index_manifest->manifest_[0].current_path = mock_get_index_manifest->manifest_[0].orig_path;
  mock_get_index_manifest->manifest_[0].data_size = 1024;
  mock_get_index_manifest->manifest_[1].orig_path = khComposePath(assets_root_, "SomeDataDir/somedatafile2");
  mock_get_index_manifest->manifest_[1].current_path = mock_get_index_manifest->manifest_[0].orig_path;
  mock_get_index_manifest->manifest_[1].data_size = 1024;
  mock_get_index_manifest->manifest_[2].orig_path = khComposePath(assets_root_, "SomeDataDir/somedatafile3");
  mock_get_index_manifest->manifest_[2].current_path = mock_get_index_manifest->manifest_[0].orig_path;
  mock_get_index_manifest->manifest_[2].data_size = 1024;

  expected_stream_manifest = mock_get_index_manifest->manifest_;
  expected_stream_manifest.push_back(ManifestEntry(db_create_params.poi_entries[0].poi_file));
  expected_search_manifest.push_back(ManifestEntry(db_create_params.poi_entries[0].poi_file));
  expected_stream_manifest.back().dependents.push_back(ManifestEntry(db_create_params.poi_entries[0].poi_data_files[0]));
  expected_search_manifest.back().dependents.push_back(ManifestEntry(db_create_params.poi_entries[0].poi_data_files[0]));
  expected_stream_manifest.push_back(ManifestEntry(toc_path_));
  expected_stream_manifest.push_back(ManifestEntry(db_header_path_));
  expected_stream_manifest.push_back(ManifestEntry(db_create_params.icon_files[0]));
  expected_stream_manifest.push_back(ManifestEntry(db_create_params.icon_files[1]));
  expected_stream_manifest.push_back(ManifestEntry(db_create_params.icon_files[2]));
  expected_stream_manifest.push_back(ManifestEntry(db_create_params.icon_files[3]));

  // change pointer to base type
  get_index_manifest = TransferOwnership(mock_get_index_manifest);

  EXPECT_NO_THROW(db_manifest = TransferOwnership(new DbManifest(&db_path, get_index_manifest)));
  EXPECT_EQ(db_path_v1_, db_path); // should be unchanged
  EXPECT_TRUE(db_manifest->Prefix().empty());

  // calling method under test
  db_manifest->GetPushManifest(file_pool_, &stream_manifest, &search_manifest, "", "");

  bool is_equivalent = IsEquivalent(&expected_stream_manifest, &stream_manifest);
  if (!is_equivalent) {
    notify(NFY_WARN, "Dumping expected stream manifest:");
    DumpManifest(expected_stream_manifest);
    notify(NFY_WARN, "Dumping actual stream manifest:");
    DumpManifest(stream_manifest);
  }

  EXPECT_TRUE(is_equivalent);

  is_equivalent = IsEquivalent(&expected_search_manifest, &search_manifest);
  if (!is_equivalent) {
    notify(NFY_WARN, "Dumping expected search manifest:");
    DumpManifest(expected_search_manifest);
    notify(NFY_WARN, "Dumping actual search manifest:");
    DumpManifest(search_manifest);
  }

  EXPECT_TRUE(is_equivalent);
}

// Calling GetPushManifest for disconnected push test.  This test gets file entries from the 
// mocked out get_index_manifest() call and also creates fake Poi files and icon files in the 
// mocked out DB folder.  Also uses the seperate manifest vectors for steam and search manifest.
TEST_F(DbManifestTest, DisconnectedRootGetPushManifest) {
  MockDbCreationParameters db_create_params;
  khDeleteGuard<MockGetIndextManifest>
    mock_get_index_manifest(TransferOwnership(new MockGetIndextManifest()));
   khDeleteGuard<DbManifest::IndextManifest> get_index_manifest;
  khDeleteGuard<DbManifest> db_manifest;
  const std::string disconnect_path(khComposePath(test_folder_, "disconnected"));
  std::string db_path(khComposePath(disconnect_path, db_path_v1_));
  std::vector<ManifestEntry> stream_manifest;
  std::vector<ManifestEntry> search_manifest;
  std::vector<ManifestEntry> expected_stream_manifest;
  std::vector<ManifestEntry> expected_search_manifest;

  db_create_params.prefix_loc = disconnect_path;
  db_create_params.poi_entries.resize(1);
  db_create_params.poi_entries[0].poi_file = khComposePath(assets_root_, "SomeDataDir/poidata/poifile");;
  db_create_params.poi_entries[0].poi_data_files.push_back(khComposePath(assets_root_, "SomeDataDir/poidata/poifile0"));
  db_create_params.icon_files.push_back(khComposePath(assets_root_, "SomeDataDir/inconpath1/icon1.png"));
  db_create_params.icon_files.push_back(khComposePath(assets_root_, "SomeDataDir/inconpath1/icon2.png"));
  db_create_params.icon_files.push_back(khComposePath(assets_root_, "SomeDataDir/inconpath1/icon3.png"));
  db_create_params.icon_files.push_back(khComposePath(assets_root_, "SomeDataDir/inconpath2/icon4.png"));
  db_create_params.icon_files.push_back(khComposePath(assets_root_, "SomeDataDir/inconpath2/icon5.gnp"));  // negitive test

  CreateMockDbFolderWithPoiData(db_create_params);

  // initialize mock get index manifest call
  // create unitialized entries to avoid OS calls for file sizes that don't exist
  mock_get_index_manifest->manifest_.resize(3);
  // for testing purposes keep it simple and stupid.  No i/o is done to these files
  mock_get_index_manifest->manifest_[0].orig_path = khComposePath(disconnect_path, assets_root_, "SomeDataDir/somedatafile1");
  mock_get_index_manifest->manifest_[0].current_path = mock_get_index_manifest->manifest_[0].orig_path;
  mock_get_index_manifest->manifest_[0].data_size = 1024;
  mock_get_index_manifest->manifest_[1].orig_path = khComposePath(disconnect_path, assets_root_, "SomeDataDir/somedatafile2");
  mock_get_index_manifest->manifest_[1].current_path = mock_get_index_manifest->manifest_[0].orig_path;
  mock_get_index_manifest->manifest_[1].data_size = 1024;
  mock_get_index_manifest->manifest_[2].orig_path = khComposePath(disconnect_path, assets_root_, "SomeDataDir/somedatafile3");
  mock_get_index_manifest->manifest_[2].current_path = mock_get_index_manifest->manifest_[0].orig_path;
  mock_get_index_manifest->manifest_[2].data_size = 1024;

  expected_stream_manifest = mock_get_index_manifest->manifest_;
  expected_stream_manifest[0].orig_path = expected_stream_manifest[0].orig_path.substr(disconnect_path.length());
  expected_stream_manifest[1].orig_path = expected_stream_manifest[1].orig_path.substr(disconnect_path.length());
  expected_stream_manifest[2].orig_path = expected_stream_manifest[2].orig_path.substr(disconnect_path.length());
  expected_stream_manifest.
    push_back(ManifestEntry(db_create_params.poi_entries[0].poi_file,
    khComposePath(disconnect_path, db_create_params.poi_entries[0].poi_file)));
  expected_search_manifest.
    push_back(ManifestEntry(db_create_params.poi_entries[0].poi_file,
    khComposePath(disconnect_path, db_create_params.poi_entries[0].poi_file)));
  expected_stream_manifest.back().dependents.
    push_back(ManifestEntry(db_create_params.poi_entries[0].poi_data_files[0],
    khComposePath(disconnect_path, db_create_params.poi_entries[0].poi_data_files[0])));
  expected_search_manifest.back().dependents.
    push_back(ManifestEntry(db_create_params.poi_entries[0].poi_data_files[0],
    khComposePath(disconnect_path, db_create_params.poi_entries[0].poi_data_files[0])));
  expected_stream_manifest.push_back(ManifestEntry(toc_path_, khComposePath(disconnect_path, toc_path_)));
  expected_stream_manifest.push_back(ManifestEntry(db_header_path_, khComposePath(disconnect_path, db_header_path_)));
  expected_stream_manifest.
    push_back(ManifestEntry(db_create_params.icon_files[0],
    khComposePath(disconnect_path, db_create_params.icon_files[0])));
  expected_stream_manifest.
    push_back(ManifestEntry(db_create_params.icon_files[1],
    khComposePath(disconnect_path, db_create_params.icon_files[1])));
  expected_stream_manifest.
    push_back(ManifestEntry(db_create_params.icon_files[2],
    khComposePath(disconnect_path, db_create_params.icon_files[2])));
  expected_stream_manifest.
    push_back(ManifestEntry(db_create_params.icon_files[3],
    khComposePath(disconnect_path, db_create_params.icon_files[3])));

  // change pointer to base type
  get_index_manifest = TransferOwnership(mock_get_index_manifest);

  EXPECT_NO_THROW(db_manifest = TransferOwnership(new DbManifest(&db_path, get_index_manifest)));
  EXPECT_EQ(db_path_v1_, db_path); // should be changed now
  EXPECT_EQ(db_manifest->Prefix(), disconnect_path);

  // calling method under test
  db_manifest->GetPushManifest(file_pool_, &stream_manifest, &search_manifest, "", "");

  bool is_equivalent = IsEquivalent(&expected_stream_manifest, &stream_manifest);
  if (!is_equivalent) {
    notify(NFY_WARN, "Dumping expected stream manifest:");
    DumpManifest(expected_stream_manifest);
    notify(NFY_WARN, "Dumping actual stream manifest:");
    DumpManifest(stream_manifest);
  }

  EXPECT_TRUE(is_equivalent);

  is_equivalent = IsEquivalent(&expected_search_manifest, &search_manifest);
  if (!is_equivalent) {
    notify(NFY_WARN, "Dumping expected search manifest:");
    DumpManifest(expected_search_manifest);
    notify(NFY_WARN, "Dumping actual search manifest:");
    DumpManifest(search_manifest);
  }

  EXPECT_TRUE(is_equivalent);
}

// TODO(RAW): cover all use cases: see comments in dbmanifest.cpp

int main(int argc, char **argv) {
  if (getNotifyLevel() < NFY_WARN) {
    setNotifyLevel(NFY_WARN); // force notify level to warning
  }
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
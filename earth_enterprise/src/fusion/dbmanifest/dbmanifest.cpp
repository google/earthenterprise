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


// The use-cases for dbmanifest:
// A database is in the assetroot:
//  - header either exists or not (db version cleaned);
//  - stream_prefix is empty;
//  - search_prefix is empty;
//  - prefix_ is empty;
// Used API: GetPushManifest, GetDisconnectedManifest(), GetDbFilesList().
//
// A database is in the publishroot:
//  - header exists;
//  - stream_prefix is not empty;
//  - search_prefix is not empty;
//  - prefix_ equals to stream_prefix;
//  Note: It is allowed that header doesn't exist for db in the publishroot.
// In this case we are trying to load it from assetroot, but for manifest files
// we behave like the database is in the publishroot.
// Used API: GetPublishManifest(), GetDisconnectedManifest().
//
// A database is in a disconnected db directory:
//  - header exists;
//  - stream_prefix is empty;
//  - prefix is a base directory of disconnected database;
// Used API: GetPushManifest.


#include "fusion/dbmanifest/dbmanifest.h"

#include <set>

#include <qurl.h>
#include <qstring.h>
#include <qdir.h>
#include <qstringlist.h>

#include "fusion/autoingest/JsonUtils.h"
#include "fusion/autoingest/.idl/storage/MapLayerJSConfig.h"
#include "fusion/autoingest/.idl/Locale.h"
#include "fusion/autoingest/.idl/storage/VectorDBRootConfig.h"
#include "fusion/autoingest/.idl/storage/RasterDBRootConfig.h"
#include "common/gedbroot/proto_dbroot.h"
#include "common/gedbroot/proto_dbroot_util.h"
#include "common/gedbroot/eta2proto_dbroot.h"
#include "common/geindex/IndexManifest.h"
#include "common/serverdb/serverdb.h"
#include "common/geFilePool.h"
#include "common/khFileUtils.h"
#include "common/khSimpleException.h"
#include "common/khStringUtils.h"
#include "common/khstl.h"
#include "common/notify.h"
#include "common/khxml/khdom.h"

struct DefaultGetIndextManifest : public DbManifest::IndextManifest {
  void operator()(geFilePool &filePool,
                  const std::string &index_path,
                  std::vector<ManifestEntry> &manifest,
                  const std::string &tmpdir,
                  const std::string& disconnect_prefix,
                  const std::string& publish_prefix) {
                    geindex::IndexManifest::GetManifest(filePool,
                                                        index_path,
                                                        manifest,
                                                        tmpdir,
                                                        disconnect_prefix,
                                                        publish_prefix);
                  }
};

khDeleteGuard<DbManifest::IndextManifest>&
  GetIndexManifestFunc(khDeleteGuard<DbManifest::IndextManifest>& GetIndexManifest)
{
  if (!GetIndexManifest) {
    GetIndexManifest = TransferOwnership(new DefaultGetIndextManifest());
  }

  return GetIndexManifest;
}

DbManifest::DbManifest(std::string* db_path,
                       khDeleteGuard<IndextManifest>& GetIndexManifest)
  : GetIndextManifest_(TransferOwnership(GetIndexManifestFunc(GetIndexManifest))),
    db_path_(*db_path),
    search_prefix_(khGetSearchPrefix(db_path_)),
    is_time_machine_enabled_(false) {
  Init(db_path);
}

DbManifest::DbManifest(std::string* db_path)
  : GetIndextManifest_(TransferOwnership(new DefaultGetIndextManifest())),
    db_path_(*db_path),
    search_prefix_(khGetSearchPrefix(db_path_)),
    is_time_machine_enabled_(false) {
  Init(db_path);
}

void DbManifest::Init(std::string* db_path) {
  if (!khIsAbspath(db_path_)) {
    throw khSimpleException("'")
      << db_path_ << "' is not an absolute path.";
  }

  const std::string stream_prefix = khGetStreamPrefix(db_path_);

  // Note: A header exists for a database in assetroot until the
  // database is cleaned. The header is copied to a publishroot when pushing
  // a database.
  // Though, still keep the old behavior - checking if a header exists and look
  // for it in the assetroot. If for some reason a header doesn't exist in the
  // publishroot, we will try to read it from the assetroot.
  header_exists_ =  khExists(khComposePath(db_path_, kHeaderXmlFile));
  std::string actual_db = header_exists_ ?
      db_path_ : db_path_.substr(stream_prefix.length());

  bool actual_db_header_exists = header_exists_;
  if (!header_exists_) {
     actual_db_header_exists = khExists(
         khComposePath(actual_db, kHeaderXmlFile));
  }

  // Note: Try to load only when a header file (gedb/header.xml) exists.
  // A header doesn't exist when a database version has been cleaned.
  if ((header_exists_ || actual_db_header_exists) &&
      !fusion_config_.LoadAndResolvePaths(
          actual_db, kHeaderXmlFile, &prefix_)) {
    throw khSimpleException("Unable to load '")
        << khComposePath(db_path_, kHeaderXmlFile) << "'";
  }

  if (!header_exists_ && actual_db_header_exists) {
    // Note: the header doesn't exist in the original directory specified by
    // the db_path, while it exists in the assetroot for this database.
    assert(prefix_.empty());
    // Behave like read the header XML from DB (the original DB path).
    prefix_ = stream_prefix;
  }

  if (!prefix_.empty()) {
    // Get assetroot db path.
    *db_path = GetOrig(*db_path);
  }

  notify(NFY_INFO, "DbManifest ctor - source db path: %s",
         db_path_.c_str());
  notify(NFY_INFO, "DbManifest ctor - original (assetroot) db path: %s",
         db_path->c_str());
  notify(NFY_INFO, "DbManifest ctor - actual db path: %s", actual_db.c_str());
  notify(NFY_INFO, "DbManifest ctor - db path prefix: %s", prefix_.c_str());
  notify(NFY_INFO, "DbManifest ctor - header_exists: %d", header_exists_);
  notify(NFY_INFO, "DbManifest ctor - actual_db_header_exists: %d",
         actual_db_header_exists);
  notify(NFY_INFO, "DbManifest ctor - stream_prefix: %s",
         stream_prefix.c_str());
  notify(NFY_INFO, "DbManifest ctor - search_prefix: %s",
         search_prefix_.c_str());

  // Note: When a database version has been cleaned, a header doesn't exist.
  // We are using this property when getting a database manifest. If there is
  // no header, we are trying to read a database manifest from the .filelist.
  // The .filelist is a database manifest files list generated when building
  // a Fusion database.
  header_exists_ |= actual_db_header_exists;

  // Get time machine info if header exists.
  if (header_exists_) {
    SetTimeMachineEnabled();
  }
}

void DbManifest::SetTimeMachineEnabled() {
  assert(header_exists_);
  // See if this db has time machine enabled.
  // If so, we'll see the timemachine dbroot file in the toc list.
  for (std::vector<std::string>::const_iterator path =
           fusion_config_.toc_paths_.begin();
       path != fusion_config_.toc_paths_.end(); ++path) {
    if (EndsWith(*path, kBinaryTimeMachineDbroot)) {
      is_time_machine_enabled_ = true;
      break;
    }
  }

  // Double check kDatedImageryChannelsFileName in case the database is
  // generated by Fusion 4.x, where timemachine dbroot does not exist yet.
  if (!is_time_machine_enabled_) {
    std::string dated_imagery_channel_map_file =
        khComposePath(Prefixed(fusion_config_.index_path_),
                      kDatedImageryChannelsFileName);
    is_time_machine_enabled_ = khExists(dated_imagery_channel_map_file);
  }
}

bool DbManifest::IsMapDb() const {
  if (header_exists_) {
    return (fusion_config_.db_type_ == DbHeader::TYPE_MAPSDB);
  } else {
    return (khBasename(db_path_) == kMapdbBase);
  }
}

void DbManifest::GetPushManifest(geFilePool &file_pool,
                                 std::vector<ManifestEntry>* stream_manifest,
                                 std::vector<ManifestEntry>* search_manifest,
                                 const std::string& tmp_dir,
                                 const std::string& publish_prefix) {
  notify(NFY_INFO,
         "GetPushManifest - publish_prefix: %s", publish_prefix.c_str());

  // Get the index portion of the manifest (index + packetfile paths)
  // from the IndexManifest and time machine files, and poi files.
  GetManifest(file_pool, stream_manifest, search_manifest, tmp_dir,
              publish_prefix);

  // Get the icon files from the icon dirs listed in GedbFusionConfig.
  std::vector<std::string> icons;
  for (unsigned int i = 0; i < fusion_config_.icons_dirs_.size(); ++i) {
    khFindFilesInDir(
        Prefixed(fusion_config_.icons_dirs_[i]), icons, ".png");
  }
  for (std::vector<std::string>::const_iterator icon = icons.begin();
       icon != icons.end(); ++icon) {
    stream_manifest->push_back(ManifestEntry(GetOrig(*icon), *icon));
  }
}

// Gets the stream manifest (index + packetfile paths) from the IndexManifest,
// and gets the POI files to search_manifest.
// This part of code is reused for disconnected-send, AddDatabase, SyncDatabase.
//
// The tmp_dir is a directory to put manifest files. It can be empty string.
// In this case manifest files won't be copied.
// The publish_prefix is used when we get a db push manifest for a disconnected
// database.
// Note: for delta-disconnected db when getting push manifest (registering and
// pushing a database), we need to check index and packetfiles in both
// a disconnected db directory and the publishroot.
void DbManifest::GetManifest(geFilePool &file_pool,
                             std::vector<ManifestEntry>* stream_manifest,
                             std::vector<ManifestEntry>* search_manifest,
                             const std::string& tmp_dir,
                             const std::string& publish_prefix) {
  search_manifest->clear();

  // Note: since GEE-5.1.2 we are creating database manifest files list when
  // building a database version, and keep it when cleaning. It allows us to
  // get a database manifest for cleaned database versions (delta-disconnected
  // publishing use-case).
  // So, if header doesn't exist, we are trying to get db manifest from
  // .filelist.
  if (!header_exists_) {
    const std::string fileslist_filepath = khComposePath(
        khDirname(db_path_),
        IsMapDb() ? kMapdbAuxBase : kGedbAuxBase,
        kDbManifestFilesListFile);
    notify(NFY_DEBUG,
           "GetManifest: No db header."
           " Trying to get db manifest as for a cleaned database.");
    if (khExists(fileslist_filepath)) {
      // Do not check file existence loading files list.
      bool kCheckExistence = false;
      khFileList files_list(kCheckExistence);
      files_list.AddFromFilelist(fileslist_filepath);
      stream_manifest->reserve(files_list.size());
      for (khFileList::const_iterator i = files_list.begin();
           i != files_list.end(); ++i) {
        // Note: Set file size to zero since we do not have
        // information about file sizes in a list of manifest files. It is
        // a use-case when we get a manifest files list in gedisconnectedsend
        // for --havepath, --havepathfile.
        stream_manifest->push_back(ManifestEntry(*i, *i, 0));
      }
    } else {
      // Note: neither db header nor db manifest files list may not exist.
      // e.g. for Fusion database version built before GEE-5.1.2 after this
      // version has been cleaned.
      throw khSimpleException(
          "Couldn't get a database manifest for '") << db_path_ <<
          "'.\nNeither header nor db manifest files list exists."
          " For example, a database version has been cleaned, "
          " while a db manifest files list has not been generated when"
          " building database (GEE-5.1.1 or earlier version).";
    }
    return;
  }

  // Get the stream manifest (index + packetfile paths) from the IndexManifest.
  if (prefix_.empty()) {
    // The case we get manifest files for a database asset (a database in the
    // assetroot).
    (*GetIndextManifest_)(file_pool, fusion_config_.index_path_,
                                        *stream_manifest, tmp_dir, "", "");
    notify(NFY_DEBUG, "Processing %ld POI files with empty prefix", fusion_config_.poi_file_paths_.size());
    for (unsigned int i = 0; i < fusion_config_.poi_file_paths_.size(); ++i) {
      const std::string poi_file = fusion_config_.poi_file_paths_[i];
      if (!khExists(poi_file)) {
        notify(NFY_WARN, "Missing POI file '%s'.", poi_file.c_str());
      } else {
        if (stream_manifest != search_manifest) {
          stream_manifest->push_back(ManifestEntry(poi_file));
        }
        search_manifest->push_back(ManifestEntry(poi_file));
        GetPoiDataFiles(&(stream_manifest->back()), &(search_manifest->back()));
      }
    }
    // These file paths are listed directly in the GedbFusionConfig.
    // Used for creating dbroot.
    for (unsigned int i = 0; i < fusion_config_.toc_paths_.size(); ++i) {
      const std::string& orig = fusion_config_.toc_paths_[i];
      stream_manifest->push_back(ManifestEntry(orig));
    }
  } else {
    // The case we get db manifest either for a disconnected database or
    // a database in the publishroot.
    // Note: when getting index manifest for delta-disconnected db we look
    // for index and packet files in both locations: disconnected db path and
    // publishroot.
    (*GetIndextManifest_)(
        file_pool, Prefixed(fusion_config_.index_path_),
        *stream_manifest, tmp_dir, prefix_, publish_prefix);

    // Now correct orig path for all.
    const size_t num_entries = stream_manifest->size();
    for (size_t i = 0; i < num_entries; ++i) {
      std::string& orig = (*stream_manifest)[i].orig_path;
      if (StartsWith(orig, prefix_)) {
        orig = GetOrig(orig);
      } else if (!publish_prefix.empty() && StartsWith(orig, publish_prefix)) {
        orig = orig.substr(publish_prefix.length());
      }
    }

    notify(NFY_DEBUG, "Processing %ld POI files with prefix", fusion_config_.poi_file_paths_.size());
    // The *.poi file paths are in the GedbFusionConfig.
    for (unsigned int i = 0; i < fusion_config_.poi_file_paths_.size(); ++i) {
      const std::string& orig = fusion_config_.poi_file_paths_[i];
      const std::string prefixed = Prefixed(orig);
      std::string curr = prefixed;
      if (!khExists(curr) && !search_prefix_.empty()) {  // a published DB
        curr = search_prefix_ + orig;
      }
      if (!khExists(curr)) {
        notify(NFY_WARN, "Missing POI file [%s:%s:%s].", orig.c_str(), prefixed.c_str(), curr.c_str());
      } else {
        if (stream_manifest != search_manifest) {
          stream_manifest->push_back(ManifestEntry(orig, curr));
        }
        search_manifest->push_back(ManifestEntry(orig, curr));
        GetPoiDataFiles(&(stream_manifest->back()), &(search_manifest->back()));
      }
    }

    // These file paths are listed directly in the GedbFusionConfig.
    // Used for creating dbroot.
    for (unsigned int i = 0; i < fusion_config_.toc_paths_.size(); ++i) {
      const std::string& orig = fusion_config_.toc_paths_[i];
      stream_manifest->push_back(ManifestEntry(orig, Prefixed(orig)));
    }
  }
  GetXmlManifest(stream_manifest);
}

// Read POI file and extract the data file names from within it and then add
// them to the manifest entries as dependant files
void DbManifest::GetPoiDataFiles(ManifestEntry* stream_manifest_entry,
                                 ManifestEntry* search_manifest_entry)
{
  const std::string& poi_file = search_manifest_entry->current_path;
  assert(stream_manifest_entry->current_path == search_manifest_entry->current_path);
  notify(NFY_DEBUG,
        "Parsing POI file %s looking for data files", poi_file.c_str());
    std::unique_ptr<GEDocument> doc = ReadDocument(poi_file);
    if (doc) {
      try {
        if (khxml::DOMElement *root = doc->getDocumentElement()) {
          if (khxml::DOMElement* el_search_table = GetFirstNamedChild(root, "SearchTableValues")) {
            std::vector<std::string> data_files;
            FromElementWithChildName(el_search_table, "SearchDataFile", data_files);
            if (data_files.empty()) {
              notify(NFY_WARN,
                    "No data files defined for POI file %s", poi_file.c_str());
            }
            for (unsigned int i = 0; i < data_files.size(); ++i) {

              const std::string& org_data_file = data_files[i];
              const std::string prefixed_data_file = Prefixed(org_data_file);
              std::string cur_data_file = prefixed_data_file;

              if (!khExists(cur_data_file) && !search_prefix_.empty()) {  // a published DB
                cur_data_file = search_prefix_ + org_data_file;
              }
              if (!khExists(cur_data_file)) {
                notify(NFY_WARN, "Missing POI data file [%s:%s:%s].",
                  org_data_file.c_str(),
                  prefixed_data_file.c_str(),
                  cur_data_file.c_str());
              } else {
                // poi data files have to be set aside until after AddDB processing is done
                // so we hide them in a special location now within the manifest and will
                // use them when we crate an 'upload' manifest during DBsync processing
                if (stream_manifest_entry != search_manifest_entry) {
                  notify(NFY_DEBUG, "Adding POI datafile as a dependent file to stream manifest: [%s:%s]",
                    org_data_file.c_str(),
                    cur_data_file.c_str());
                  stream_manifest_entry->dependents.push_back(ManifestEntry(org_data_file, cur_data_file));
                }
                notify(NFY_DEBUG, "Adding POI datafile as a dependent file to search manifest: [%s:%s]",
                  org_data_file.c_str(),
                  cur_data_file.c_str());
                search_manifest_entry->dependents.push_back(ManifestEntry(org_data_file, cur_data_file));
              }
            }
          } else {
            notify(NFY_WARN,
                  "No search table element found in POI file %s", poi_file.c_str());
          }
        } else {
          notify(NFY_WARN,
                 "No document element loading POI file %s", poi_file.c_str());
        }
      } catch(const std::exception &e) {
        notify(NFY_WARN, "%s while loading POI file %s", e.what(), poi_file.c_str());
      } catch(...) {
        notify(NFY_WARN, "Unable to load POI file %s", poi_file.c_str());
      }
    } else {
      notify(NFY_WARN, "Unable to read POI file %s", poi_file.c_str());
    }
}

std::string DbManifest::LocalesFilename() const {
  return khComposePath(db_path_, LocaleSet::LocalesFilename());
}

std::string DbManifest::VectorLayerConfigFile() const {
  return khComposePath(db_path_, kDbVectorLayerConfigFile);
}

std::string DbManifest::ImageryLayerConfigFile() const {
  return khComposePath(db_path_, kDbImageryLayerConfigFile);
}

void DbManifest::AddCheckingForOrig(
    const std::string& curr, std::vector<ManifestEntry>* manifest) const {
  if (prefix_.empty()) {
    manifest->push_back(ManifestEntry(curr));
  } else {
    const std::string orig = GetOrig(curr);
    manifest->push_back(ManifestEntry(orig, khExists(curr) ? curr : orig));
  }
}

void DbManifest::AddCheckingForOrigIfExists(
    const std::string& curr, std::vector<ManifestEntry>* manifest) const {
  if (prefix_.empty()) {
    if (khExists(curr)) {
      manifest->push_back(ManifestEntry(curr));
    }
  } else {
    const std::string orig = GetOrig(curr);
    if (khExists(curr)) {
      manifest->push_back(ManifestEntry(orig, curr));
    } else if (khExists(orig)) {
      manifest->push_back(ManifestEntry(orig));
    }
  }
}

// May throw exception if needed files are not found
// Adds header. Needed for recreating manifest.
// Adds the optional locales, vector and imagery layer configs. Those are used
// by the JSON generator.
void DbManifest::GetXmlManifest(std::vector<ManifestEntry>* manifest) const {
  // Add the header for this gedb
  AddCheckingForOrig(khComposePath(db_path_, kHeaderXmlFile), manifest);

  // We need several files for generating the JSON files during a disconnected
  // publish.
  // Locales, and a Vector config and Raster config
  // (different for Maps and Earth) for generating layer information in
  // the JSON.

  // Add the optional locales, vector and imagery layer configs for this gedb.
  // They are used by the Json generator during publish.
  AddCheckingForOrigIfExists(LocalesFilename(), manifest);
  AddCheckingForOrigIfExists(VectorLayerConfigFile(), manifest);
  AddCheckingForOrigIfExists(ImageryLayerConfigFile(), manifest);
}

// Assumed to be on either an asset (a db is in the assetroot) or a published DB
// (a db in in the publishroot).
// In case of asset, the prefix_ is empty, else the prefix_ is a stream prefix.
// Always give priority to DB.
void DbManifest::GetDisconnectedManifest(geFilePool& file_pool,
                                         std::vector<ManifestEntry>& manifest,
                                         const std::string& tmp_dir) {
  // Get the index portion of the manifest (index + packetfile paths)
  // from the IndexManifest and time machine files, and poi files.
  GetManifest(file_pool, &manifest, &manifest, tmp_dir, "");

  if (!header_exists_) {
    // Note: It is a case when we are trying to get db manifest for cleaned
    // database version (from the .filelist). Reaching this point, we have done
    // everything in GetManifest(), so just return.
    return;
  }

  if (fusion_config_.icons_dirs_.size()) {
    // Get the icon files from the icon dirs listed in GedbFusionConfig.
    std::vector<std::string> icons;
    for (unsigned int i = 0; i < fusion_config_.icons_dirs_.size(); ++i) {
      khFindFilesInDir(
          Prefixed(fusion_config_.icons_dirs_[i]), icons, ".png");
    }
    for (std::vector<std::string>::const_iterator icon = icons.begin();
         icon != icons.end(); ++icon) {
      manifest.push_back(ManifestEntry(GetOrig(*icon), *icon));
    }
  }
}

// Gets database manifest files list.
// Note: used by the gedbgen/mapdbgen tools to create for a database a list of
// manifest files which then be used to get a db manifest for a cleaned db
// version (delta-disconnected publishing use-case).
void DbManifest::GetDbFilesList(geFilePool &file_pool,
                                std::vector<std::string> *file_list) {
  // Get database manifest.
  std::vector<ManifestEntry> manifest;
  const std::string kLocalTmpDir = "";  // Get manifest only.
  GetDisconnectedManifest(file_pool, manifest, kLocalTmpDir);

  // Build files list from manifest entries.
  file_list->reserve(manifest.size());
  for (std::vector<ManifestEntry>::const_iterator entry = manifest.begin();
       entry != manifest.end(); ++entry) {
    file_list->push_back(entry->orig_path);
  }
}

bool DbManifest::GetDbrootsAndServerConfig(
    const std::string& stream_url,
    const std::string& end_snippet_proto,
    const std::string& server_prefix,
    const std::string& tmpdir,
    std::vector<ManifestEntry>& manifest) {
  using gedbroot::geEta2ProtoDbroot;
  using gedbroot::CreateTimemachineDbroot;

  // Create ServerdbConfig.
  ServerdbConfig server_config;
  server_config.index_path = server_prefix + fusion_config_.index_path_;

  // As of 3.2, we now publish some additional files for JSON communication
  // of the database specifics.
  GetGEJsonFiles(stream_url, tmpdir, &server_config, &manifest);

  // Process each dbroot.
  for (size_t i = 0; i < fusion_config_.toc_paths_.size(); ++i) {
    const std::string toc_local = Prefixed(fusion_config_.toc_paths_[i]);
    notify(NFY_DEBUG, "Reading dbroot file: %s = ", toc_local.c_str());

    std::string base_name = khBasename(toc_local);
    std::string binary_dbroot_name;
    std::string suffix;
    std::string dbroot_local;
    khDeleteGuard<geProtoDbroot> proto_dbroot;
    const bool EXPECT_EPOCH = true;

    bool isPostambleProto = StartsWith(base_name, kPostamblePrefix + ".");
    bool isPostambleEta = StartsWith(base_name, kPostamblePrefixEta + ".");

    if (isPostambleProto || isPostambleEta) {
      // This is a postamble file that needs further processing.
      if (isPostambleProto) {  // Intermediate proto dbroot.
        suffix = base_name.substr(kPostamblePrefix.length() + 1);
        proto_dbroot =  TransferOwnership(new geProtoDbroot(
            toc_local, geProtoDbroot::kProtoFormat));
      } else {  // Ascii ETA dbroot.
        proto_dbroot =  TransferOwnership(new geProtoDbroot());
        geEta2ProtoDbroot eta2proto(&*proto_dbroot);

        suffix = base_name.substr(kPostamblePrefixEta.length() + 1);
        eta2proto.ConvertFile(toc_local, fusion_config_.database_version_);

        // Set proto_imagery to "true" to serve imagery packets in protobuf
        // format.
        if (proto_dbroot->imagery_present()) {
          proto_dbroot->set_proto_imagery(true);
        }
        if (!proto_dbroot->IsValid(EXPECT_EPOCH)) {
          notify(NFY_FATAL, "Got invalid proto dbroot when converting from ETA "
                 "dbroot file: %s", fusion_config_.toc_paths_[i].c_str());
        }

        // Generate timemachine dbroot as well if it is a timemachine database.
        if (IsTimeMachineEnabled()) {
          std::string tm_in_local = tmpdir + kBinaryTimeMachineDbroot;
          CreateTimemachineDbroot(tm_in_local,
                                  fusion_config_.database_version_);
          std::string tm_on_server = kDbrootsDir + kBinaryTimeMachineDbroot;
          manifest.push_back(ManifestEntry(tm_on_server, tm_in_local));
          server_config.toc_paths[kTimeMachineTOCSuffix] = tm_on_server;
        }
      }

      // Get the localization suffix.
      assert(suffix != kTimeMachineTOCSuffix);

      // Setup other local vars needs at the end of this for loop.
      binary_dbroot_name = kBinaryDbrootPrefix + "." + suffix;
      dbroot_local = tmpdir + "/" + binary_dbroot_name;

      // Merge the end_snippet section to proto dbroot.
      MergeEndSnippet(end_snippet_proto, &*proto_dbroot);

      // Add timemachine url to main dbroot if timemachine exists
      SetTimeMachineUrl(stream_url, &*proto_dbroot);

      // Write out the final dbroot in the format the client wants.
      proto_dbroot->Write(dbroot_local, geProtoDbroot::kEncodedFormat);
    } else {
      // This is not a postamble. Assumption: it's ready to go as is.
      if (StartsWith(base_name, kBinaryDbrootPrefix + ".")) {
        // This is a final proto dbroot.
        suffix = base_name.substr(kBinaryDbrootPrefix.length() + 1);
        binary_dbroot_name = base_name;
        dbroot_local = toc_local;
      } else {
        notify(NFY_FATAL, "INTERNAL ERROR: Unrecognized dbroot name: %s",
               fusion_config_.toc_paths_[i].c_str());
      }
    }

    // Add server config entry (Relative path).
    server_config.toc_paths[suffix] = kDbrootsDir + binary_dbroot_name;

    // Add Manifest entry.
    const std::string dbroot_on_server = kDbrootsDir + binary_dbroot_name;
    manifest.push_back(ManifestEntry(dbroot_on_server, dbroot_local));
  }


  // Add icons dir entries.
  std::set<std::string> icon_set;
  for (unsigned int i = 0; i < fusion_config_.icons_dirs_.size(); ++i) {
    const std::string ith_icon_dir = Prefixed(fusion_config_.icons_dirs_[i]);
    QDir dir(ith_icon_dir.c_str());
    QStringList list = dir.entryList(QDir::Files);
    for (auto j = 0; j < list.size(); ++j) {
      // We are merging icons from 3 different sources. Avoid duplicate entries.
      const std::string icon_entry = kIconsDir
                                   + std::string(list[j].toUtf8().constData());
      // Add server config entry (Relative path).
      if (icon_set.insert(icon_entry).second) {
        // Add manifest entry (Absolute path).
        const std::string dst_file = icon_entry;
        const std::string src_file = khComposePath(ith_icon_dir,
                                                   std::string(list[j].toUtf8().constData()));
        manifest.push_back(ManifestEntry(dst_file, src_file));
      }
    }
  }

  // List the icons in the server config.
  std::copy(icon_set.begin(), icon_set.end(),
            std::back_inserter(server_config.icons));

  // Set the db type in the server config to TYPE_GEDB.
  server_config.db_type = ServerdbConfig::TYPE_GEDB;

  // Finally save the server_config and add it to the manifest.
  const std::string server_config_src_path = khComposePath(
      tmpdir, kServerConfigFileName);
  if (!server_config.Save(server_config_src_path))
    return false;
  manifest.push_back(ManifestEntry(
      kServerConfigFileName, server_config_src_path));

  return true;
}

void DbManifest::MergeEndSnippet(const std::string& end_snippet_proto,
                                 geProtoDbroot* proto_dbroot) const {
  // Check if there are snippets to merge in proto dbroot.
  if (end_snippet_proto.empty())
    return;

  // TODO: add Combine(std::string&) to geProtoDbroot?
  keyhole::dbroot::DbRootProto snippet_dbroot;
  snippet_dbroot.ParseFromString(end_snippet_proto);

  notify(NFY_DEBUG, "MergeEndSnippet - END_SNIPPET_PROTO restored: %s",
         snippet_dbroot.DebugString().c_str());

  proto_dbroot->MergeFrom(snippet_dbroot);
}

void DbManifest::SetTimeMachineUrl(
    const std::string& stream_url, geProtoDbroot* proto_dbroot) const {
  if (IsTimeMachineEnabled()) {
    std::string url = stream_url + "?db=tm";
    // expected url of form http://khmdb.xxx.com/?db=tm
    // This is a different server from the perspective of the client
    // These are the options needed to support time machine as of GE 4.4.
    proto_dbroot->mutable_end_snippet()->mutable_time_machine_options()
        ->set_server_url(url);
  }
}

bool DbManifest::GetLayerDefsAndServerConfig(
    const std::string& stream_url,
    const std::string& server_prefix,
    const std::string& tmpdir,
    std::vector<ManifestEntry>& manifest) {
  // Create ServerdbConfig
  ServerdbConfig server_config;
  server_config.index_path = server_prefix + fusion_config_.index_path_;

  // As of 3.2, we now publish some additional files for JSON communication
  // of the database specifics.
  GetMapsJsonFiles(stream_url, tmpdir, &server_config, &manifest);

  // Localized layers.js files
  for (size_t i = 0; i < fusion_config_.toc_paths_.size(); ++i) {
    const std::string postamble = Prefixed(fusion_config_.toc_paths_[i]);
    std::string base_name = khBasename(postamble);

    // Note: Since GEE v4.5 a filename of output layers definition
    // is 'layers.js.postamble.LOCALE' (GEE <= v4.4.1 - 'layers.js.LOCALE').

    // Check if it is layers.js.postamble.LOCALE file (GEE 4.5 and higher).
    std::string::size_type suffix_idx =
        base_name.find(kLayerDefsPostamblePrefix);
    if (suffix_idx != std::string::npos) {
      suffix_idx += kLayerDefsPostamblePrefix.length() + 1;
    } else {  // Check if it is layers.js -file. (GEE <=4.4).
      suffix_idx = base_name.find(kLayerDefsPrefix);
      if (suffix_idx != std::string::npos) {
        suffix_idx += kLayerDefsPrefix.length() + 1;
      } else {
        // Not v5 postambles.
        continue;
      }
    }

    // Get the localization suffix.
    std::string suffix = base_name.substr(suffix_idx);

    if (suffix.length() == 0)
      notify(NFY_WARN, "Missing default layer definitions file.");

    std::string publish_layerdefs_name = kLayerDefsPrefix + "." + suffix;

    // Add server url the existing layerdefs js files and create new ones.
    std::string layerdefs_buffer = "var stream_url = \"" + stream_url + "\";\n";
    // Add the include google layers flag to help determine whether to fetch
    // google mercator map tiles.
    layerdefs_buffer += "var include_google_layers = ";
    if (fusion_config_.use_google_imagery_)
      layerdefs_buffer += "true;\n";
    else
      layerdefs_buffer += "false;\n";
    layerdefs_buffer += "var tile_layer_defs = new Array();\n";
    khReadStringFromFile(postamble, layerdefs_buffer);
    khWriteStringToFile(
        tmpdir + "/" + publish_layerdefs_name, layerdefs_buffer);

    // Add server config entry (Relative path).
    server_config.toc_paths[suffix] = kLayerDefsDir + publish_layerdefs_name;

    // Add Manifest entry.
    manifest.push_back(ManifestEntry(
        kLayerDefsDir + publish_layerdefs_name,
        tmpdir + "/" + publish_layerdefs_name));
  }

  // Add icons dir entries.
  std::set<std::string> icon_set;
  for (unsigned int i = 0; i < fusion_config_.icons_dirs_.size(); ++i) {
    const std::string ith_icon_dir = Prefixed(fusion_config_.icons_dirs_[i]);
    QDir dir(ith_icon_dir.c_str());
    QStringList list = dir.entryList(QDir::Files);
    for (auto j = 0; j < list.size(); ++j) {
      // We are merging icons from different sources. Avoid duplicate entries.
      const std::string icon_entry = kIconsDir
                                   + std::string(list[j].toUtf8().constData());
      // Add server config entry (Relative path).
      if (icon_set.insert(icon_entry).second) {
        // Add manifest entry (Absolute path).
        const std::string dst_file = icon_entry;
        const std::string src_file = khComposePath(ith_icon_dir,
                                                   std::string(list[j].toUtf8().constData()));
        manifest.push_back(ManifestEntry(dst_file, src_file));
      }
    }
  }

  // List the icons in the server config.
  std::copy(icon_set.begin(), icon_set.end(),
            std::back_inserter(server_config.icons));

  // Set the db type in the server config to TYPE_MAPDB.
  server_config.db_type = ServerdbConfig::TYPE_MAPDB;

  // Finally save the server_config and add it to the manifest.
  if (!server_config.Save(tmpdir + "/" + kServerConfigFileName))
    return false;
  manifest.push_back(ManifestEntry(
      kServerConfigFileName,
      tmpdir + "/" + kServerConfigFileName));
  return true;
}

// Utility to load the locales which are stored in the gedb during the
// gedbgen. If no locales are found or fail to load, then a warning is printed.
void DbManifest::LoadLocales(LocaleSet* localeSet) const {
  std::string filename = LocalesFilename();
  if (!khExists(filename) || !localeSet->Load(filename)) {
    notify(NFY_DEBUG, "Publishing only with default locale. "
           "No locales file found.");
  }
}

void DbManifest::GetGEJsonFiles(const std::string& stream_url,
                                const std::string& tmpdir,
                                ServerdbConfig* server_config,
                                std::vector<ManifestEntry>* manifest) const {
  // The vector and imagery layer info is in the
  // DbRootGenConfig and RasterDBRootGenConfig files respectively.
  // gedbgen caches them in the gedb.  They may be missing if there are no
  // vectors or imagery.
  // The DBRootGenConfig has all the vector layer information that we need.
  std::vector<LayerConfig> vector_layers;
  {
    std::string vector_config_file = VectorLayerConfigFile();
    if ((!vector_config_file.empty()) && khExists(vector_config_file)) {
      DBRootGenConfig dbrootgen_config;
      if (dbrootgen_config.Load(vector_config_file)) {
        // We are really just interested in the layers.
        vector_layers = dbrootgen_config.layers;
      } else {
       // Specific message already emitted.
        notify(NFY_FATAL, "Unable to load config file %s",
               vector_config_file.c_str());
      }
    }
  }

  // The RasterDBRootGenConfig file is found similarly to the DBRootGenConfig.
  std::vector<RasterDBRootGenConfig::Layer> raster_layers;
  {
    std::string imagery_config_file = ImageryLayerConfigFile();
    if ((!imagery_config_file.empty()) &&
        khExists(imagery_config_file)) {
      RasterDBRootGenConfig raster_dbrootgen_config;
      if (raster_dbrootgen_config.Load(imagery_config_file)) {
         // We are really just interested in the layers.
         raster_layers = raster_dbrootgen_config.layers_;
      } else {
        // Specific message already emitted.
        notify(NFY_FATAL, "Unable to load config file %s",
               imagery_config_file.c_str());
      }
    }
  }

  // Need to parse the stream url.
  QUrl qurl(stream_url.c_str());
  // Iterate through the current locales.
  LocaleSet locale_set;
  LoadLocales(&locale_set);
  for (int i = -1; i < static_cast<int>(locale_set.supportedLocales.size());
       ++i) {
    std::string locale = i < 0 ?
        kDefaultLocaleSuffix : std::string(locale_set.supportedLocales[i]);

    // Create the JSON buffer for the database and this locale.
    std::string json_text = JsonUtils::GEJsonBuffer(stream_url,
                                                    qurl.host().toUtf8().constData(),
                                                    qurl.protocol().toUtf8().constData(),
                                                    raster_layers,
                                                    vector_layers,
                                                    locale);

    // Write the JSON file and add it to the manifest.
    // add server config entry (Relative path) and
    // manifest entry (absolute path).
    std::string base_path = kEarthJsonPath + "." + locale;
    server_config->json_paths[locale] = base_path;
    std::string output_filename = tmpdir + "/" + base_path;
    khWriteStringToFile(output_filename, json_text);
      manifest->push_back(ManifestEntry(
          base_path,
          output_filename));
  }
}

void DbManifest::GetMapsJsonFiles(
    const std::string& stream_url,
    const std::string& tmpdir,
    ServerdbConfig* server_config,
    std::vector<ManifestEntry>* manifest) const {
  // The vector and imagery layer info is in the
  // MapLayerJSConfig for imagery and layer respectively.
  // mapdbgen caches them in the mapdb.  They may be missing if there are no
  // vectors or imagery.
  // Collect the layer info from the MapLayerJSConfigs (one for imagery and
  // one for Map Layers).
  // This whole design of the MapLayerJS (and entire Map DB) is a complete
  // mess, but this is the easiest way to get the layer info
  // without a major rewrite.
  std::vector<MapLayerJSConfig::Layer> layers;

  // Get the imagery layer defs if they exist.
  {
    std::string imagery_config_file = ImageryLayerConfigFile();
    if ((!imagery_config_file.empty()) && khExists(imagery_config_file)) {
      MapLayerJSConfig js_config;
      if (js_config.Load(imagery_config_file)) {
        layers.insert(layers.end(), js_config.layers_.begin(),
                      js_config.layers_.end());
      } else {
        // specific message already emitted
        notify(NFY_FATAL, "Unable to load config file %s",
               imagery_config_file.c_str());
      }
    }
  }
  // Get the map vector layer defs if they exist.
  {
    std::string vector_config_file = VectorLayerConfigFile();
    if ((!vector_config_file.empty()) && khExists(vector_config_file)) {
      MapLayerJSConfig js_config;
      if (js_config.Load(vector_config_file)) {
        layers.insert(layers.end(), js_config.layers_.begin(),
                      js_config.layers_.end());
      } else {
        // specific message already emitted
        notify(NFY_FATAL, "Unable to load config file %s",
               vector_config_file.c_str());
      }
    }
  }

  // Iterate through the current locales.
  LocaleSet locale_set;
  LoadLocales(&locale_set);
  // Need to parse the stream url.
  QUrl qurl(stream_url.c_str());
  for (int i = -1; i < static_cast<int>(locale_set.supportedLocales.size());
       ++i) {
    std::string locale = i < 0 ?
        kDefaultLocaleSuffix : std::string(locale_set.supportedLocales[i]);

    // Create the JSON buffer for the database and this locale.
    std::string json_text = JsonUtils::MapsJsonBuffer(stream_url,
                                                      qurl.host().toUtf8().constData(),
                                                      qurl.protocol().toUtf8().constData(),
                                                      layers,
                                                      fusion_config_,
                                                      locale);

    // Write the JSON file and add it to the manifest.
    // add server config entry (Relative path) and
    // manifest entry (absolute path).
    std::string base_path = kMapsJsonPath + "." + locale;
    server_config->json_paths[locale] = base_path;
    std::string output_filename = tmpdir + "/" + base_path;
    khWriteStringToFile(output_filename, json_text);
    if (manifest) {
      manifest->push_back(ManifestEntry(
          base_path,
          output_filename));
    }
  }
}

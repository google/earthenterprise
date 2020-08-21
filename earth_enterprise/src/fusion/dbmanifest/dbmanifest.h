/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_DBMANIFEST_DBMANIFEST_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_DBMANIFEST_DBMANIFEST_H_

#include <string>
#include <vector>

#include "fusion/dbgen/.idl/DBConfig.h"
#include "common/ManifestEntry.h"
#include "common/khGuard.h"
#include "common/khstl.h"


class ServerdbConfig;
class LocaleSet;
class geFilePool;
class geProtoDbroot;

// DbManifest

class DbManifest {
 public:

  struct IndextManifest {
    virtual void operator()(geFilePool &filePool,
                          const std::string &index_path,
                          std::vector<ManifestEntry> &manifest,
                          const std::string &tmpdir,
                          const std::string& disconnect_prefix,
                          const std::string& publish_prefix) = 0;
    virtual ~IndextManifest(){}
  };

  // May throw an exception.
  // Returns the prefix removed db_path (assetroot db path).
  explicit DbManifest(std::string* db_path);
  // used for testing
  DbManifest(std::string* db_path,
             // alternate indext manifest getter. Ownership will
             // be transfered
             khDeleteGuard<IndextManifest>& GetIndexManifest);

  // Gets push- manifest files (index-, packet- files, time machine files,
  // POI files, icon files).
  void GetPushManifest(geFilePool &file_pool,
                       std::vector<ManifestEntry>* stream_manifest,
                       std::vector<ManifestEntry>* search_manifest,
                       const std::string& tmp_dir,
                       const std::string& publish_prefix);

  // Gets publish manifest files.
  // stream_url - stream URL (server host + publishing target path).
  // end_snippet_proto - proto dbroot end snippet section in binary format
  // server_prefix - server prefix (published database directory prefix).
  // tmpdir - temp. directory for publish manifest files.
  // manifest - list to put in publish manifest entries.
  //
  // For gedb creates following files in db_path_
  // [1] json/earth.json.DEFAULT
  // [2] dbroots/* (for DEFAULT, all present locales, & optionally time machine)
  // [3] icons/*
  // [4] serverdb.config
  // For mapdb creates the following files in db_path_
  // [1] json/maps.json.DEFAULT
  // [2] search_tabs/searchtabs.js
  // [3] layers/layers.js.DEFAULT
  // [4] icons/*
  // [5] serverdb.config
  bool GetPublishManifest(const std::string& stream_url,
                          const std::string& end_snippet_proto,
                          const std::string& server_prefix,
                          const std::string& tmpdir,
                          std::vector<ManifestEntry>& manifest) {
    return ((fusion_config_.db_type_ == DbHeader::TYPE_GEDB) ?
            GetDbrootsAndServerConfig(stream_url,
                                      end_snippet_proto,
                                      server_prefix,
                                      tmpdir,
                                      manifest) :
            GetLayerDefsAndServerConfig(stream_url,
                                        server_prefix,
                                        tmpdir,
                                        manifest));
  }

  void GetDisconnectedManifest(geFilePool& file_pool,
                               std::vector<ManifestEntry>& manifest,
                               const std::string& tmp_dir);

  // Gets database manifest files list.
  void GetDbFilesList(geFilePool &file_pool,
                      std::vector<std::string> *file_list);

  unsigned int GetDbType() const { return uint(fusion_config_.db_type_); }

  bool IsMapDb() const;

  std::string Prefixed(const std::string& file_name) const {
    return prefix_ + file_name;
  }

  std::string GetCurr(const std::string& file_name) const {
    // Try prefixed entry first and then the original file.
    if (!prefix_.empty()) {
      std::string curr = Prefixed(file_name);
      if (khExists(curr)) {
        return curr;
      }
    }
    return khExists(file_name) ? file_name : "";
  }

  std::string GetOrig(const std::string& file_name) const {
    assert(StartsWith(file_name, prefix_));
    return file_name.substr(prefix_.size());
  }

  const std::string& Prefix() const { return prefix_; }

  const std::string& FusionHost() const { return fusion_config_.fusion_host_; }

  // Whether Fusion database uses Google base map.
  bool UseGoogleBasemap() const { return fusion_config_.use_google_imagery_; }

 private:
  DbManifest();

  // GetManifest can work on manifest of a delta disconnected_send.
  // In such a case a missing file (which can only happen in packet file, as
  // that is only thing that can be common between different databases),
  // is searched at publish_prefix/Asset_root_based_path_of_packetfile.
  // If that doesn't exist then it is assumed to be case of incomplete delta
  // disconnected_send.
  void GetManifest(geFilePool &file_pool,
                   std::vector<ManifestEntry>* stream_manifest,
                   std::vector<ManifestEntry>* search_manifest,
                   const std::string& tmp_dir,
                   const std::string& publish_prefix);

// Read POI file and extract the data file names from within it and then add
// them to the manifest entries as dependant files
void GetPoiDataFiles(ManifestEntry* stream_manifest_entry,
                     ManifestEntry* search_manifest_entry);

  // For mapdb and creates the following files in db_path_:
  // [1] json/maps.json.DEFAULT
  // [2] search_tabs/searchtabs.js
  // [3] layers/layers.js.DEFAULT
  // [4] icons/*
  // [5] serverdb.config
  bool GetLayerDefsAndServerConfig(const std::string& stream_url,
                                   const std::string& server_prefix,
                                   const std::string& tmpdir,
                                   std::vector<ManifestEntry>& manifest);
  // For gedb and creates follwing files in db_path_:
  // [1] json/earth.json.DEFAULT
  // [2] dbroots/* (for DEFAULT, all present locales, & optionally time machine)
  // [3] icons/*
  // [4] serverdb.config
  bool GetDbrootsAndServerConfig(const std::string& stream_url,
                                 const std::string& end_snippet_proto,
                                 const std::string& server_prefix,
                                 const std::string& tmpdir,
                                 std::vector<ManifestEntry>& manifest);

  void MergeEndSnippet(const std::string& end_snippet_proto,
                       geProtoDbroot* proto_dbroot) const;

  // Sets the timemachine url if timemachine is enabled on this db.
  void SetTimeMachineUrl(const std::string& stream_url,
                         geProtoDbroot *proto_dbroot) const;

  // Checks whether time machine is enabled and sets flag accordingly.
  // It is based on simple test whether time machine is enabled for
  // the current db - checking that either a timemachine dbroot is listed in
  // the config file or a dated imagery channels map file exists.
  void SetTimeMachineEnabled();

  // Returns whether the timemachine is enabled.
  bool IsTimeMachineEnabled() const { return is_time_machine_enabled_; }

  // JSON utilities
  // Create and add the GE Database JSON files to the manifest.
  // stream_url: the stream server url
  // tmpdir: the directory where temporary files are written before publishing
  // server_config: server config object which maintains a list of json
  //                contents. This method adds the json files to the config.
  // manifest: the manifest of files to be published. This method adds the json
  //           files to this list.
  void GetGEJsonFiles(const std::string& stream_url,
                      const std::string& tmpdir,
                      ServerdbConfig* server_config,
                      std::vector<ManifestEntry>* manifest) const;

  // Create and add the Maps Database JSON files to the manifest.
  // stream_url: the stream server url
  // tmpdir: the directory where temporary files are written before publishing
  // server_config: server config object which maintains a list of json
  //                contents. This method adds the json files to the config.
  // manifest: the manifest of files to be published. This method adds the json
  //           files to this list.
  void GetMapsJsonFiles(
      const std::string& stream_url,
      const std::string& tmpdir,
      ServerdbConfig* server_config,
      std::vector<ManifestEntry>* manifest) const;

  // Filenames needed for publishing for JSON file generation.
  // These files are written by gedbgen and mapdbgen.
  std::string LocalesFilename() const;
  std::string VectorLayerConfigFile() const;
  std::string ImageryLayerConfigFile() const;

  // Utility to safely load the locales given that this may be a
  // disconnected publish, in which case the locales are in a special file.
  // If no locales are loaded a warning is printed.
  // localSet: the output locales will be loaded or empty if no locales
  //           file is found.
  // a warning will be displayed if no locale file is found.
  void LoadLocales(LocaleSet* locale_set) const;

  void AddCheckingForOrig(
    const std::string& curr, std::vector<ManifestEntry>* manifest) const;

  void AddCheckingForOrigIfExists(
    const std::string& curr, std::vector<ManifestEntry>* manifest) const;

  void GetXmlManifest(std::vector<ManifestEntry>* manifest) const;

  // shared initialization code ( no shared constructors until C++11 :( )
  void Init(std::string* db_path);

  const khDeleteGuard<IndextManifest> GetIndextManifest_;

  const std::string db_path_;
  const std::string search_prefix_;
  std::string prefix_;
  DbHeader fusion_config_;
  bool header_exists_;
  bool is_time_machine_enabled_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_DBMANIFEST_DBMANIFEST_H_

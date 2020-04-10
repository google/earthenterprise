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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GEPUBLISH_PUBLISHERCLIENT_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GEPUBLISH_PUBLISHERCLIENT_H_


#include <string>
#include <vector>

#include "fusion/gepublish/PublishHelper.h"
#include "common/ManifestEntry.h"
#include "autoingest/.idl/ServerCombination.h"

class geProgress;

// Auxiliary structures used for reporting list of registered and published
// Fusion databases and Portables.
struct DbInfo {
  DbInfo(const std::string& _path,
         const std::string& _description)
      : path(_path),
        description(_description) {
  }
  virtual ~DbInfo() {
  }
  std::string path;              // Path in assetroot or portable directory.
  std::string description;       // Description.
};

struct FusionDbInfo : public DbInfo {
  FusionDbInfo(const std::string& _path,
               const std::string& _description,
               const std::string& _fusion_hostname)
      : DbInfo(_path, _description),
        fusion_hostname(_fusion_hostname) {
  }
  std::string fusion_hostname;   // Fusion hostname.
};

struct PortableInfo : public DbInfo {
  PortableInfo(const std::string& _path,
               const std::string& _description)
      : DbInfo(_path, _description) {
  }
};

struct PublishInfo {
  PublishInfo(const std::string& _target_path,
              const std::string& _vh_name)
      : target_path(_target_path),
        vh_name(_vh_name) {
  }

  virtual ~PublishInfo() {
  }

  std::string target_path;
  std::string vh_name;
};

struct PublishedFusionDbInfo : public FusionDbInfo, public PublishInfo {
  PublishedFusionDbInfo(const std::string& _path,
                        const std::string& _description,
                        const std::string& _fusion_hostname,
                        const std::string& _target_path,
                        const std::string& _vh_name)
      : FusionDbInfo(_path, _description, _fusion_hostname),
        PublishInfo(_target_path, _vh_name) {
  }
};

struct PublishedPortableInfo : public PortableInfo, public PublishInfo {
  PublishedPortableInfo(const std::string& _path,
                        const std::string& _description,
                        const std::string& _target_path,
                        const std::string& _vh_name)
      : PortableInfo(_path, _description),
        PublishInfo(_target_path, _vh_name) {
  }
};


typedef std::vector<FusionDbInfo> FusionDbInfoVector;
typedef std::vector<PublishedFusionDbInfo> PublishedFusionDbInfoVector;
typedef std::vector<PortableInfo> PortableInfoVector;
typedef std::vector<PublishedPortableInfo> PublishedPortableInfoVector;


class PublisherClient : public PublishHelper {
 public:
  PublisherClient(const std::string& fusion_host,
                  const ServerConfig& stream_server,
                  const ServerConfig& search_server,
                  geProgress* progress,
                  geAuth* auth,
                  const bool force_copy = false);
  virtual ~PublisherClient();

  bool PingServersWithTimeout();

  // Get list of databases and portables registered on server.
  bool ListDatabases(ServerType server_type,
                     FusionDbInfoVector *const db_infos,
                     PortableInfoVector *const portable_infos);

  // Get list of published databases and portables.
  bool PublishedDatabases(
      ServerType server_type,
      PublishedFusionDbInfoVector *const published_db_infos,
      PublishedPortableInfoVector *const published_portable_infos);

  bool ListTargetPaths(ServerType server_type,
                       std::vector<std::string>* target_paths);
  bool QueryDatabaseDetails(ServerType server_type, const std::string& db_name,
                            std::vector<std::string>* file_names);

  // Retrieves the publish context for a target path, like search defs etc..
  bool QueryPublishContext(ServerType server_type,
                           const std::string& in_target_path,
                           std::vector<std::string>* publish_context);

  // Sends the list of files from the current db to the postgres
  // database.
  bool AddDatabase(const std::string& db_name,
                   const std::string& db_pretty_name);

  // Deletes database from the server.
  bool DeleteDatabase(const std::string& db_name);

  // Uploads Stream and Search data on GEE Server and publishes on Search VS.
  bool PushDatabase(const std::string& db_name);

  // Uploads the dbRoot related files (dbRoots, icons and
  // serverdb.config) to the server and publishes stream data.
  bool PublishDatabase(const std::string& in_db_name,
                       const std::string& in_target_path,
                       const std::string& vh_name = "",
                       const bool default_db = false,
                       const bool poi_search = false,
                       const bool enhanced_search = false,
                       const bool serve_wms = false);
            

  // Re-publish database to be served with the existing target path.
  bool RepublishDatabase(const std::string& in_db_name,
                         const std::string& in_target_path);

  // swap two published targets.
  bool SwapTargets(const std::string& in_target_path_a,
                   const std::string& in_target_path_b);

  // Unpublish database served from specified target path.
  bool UnpublishDatabase(const std::string& in_target_path);

  bool ListVirtualHosts(std::vector<std::string>* const vs_names,
                        std::vector<std::string>* const vs_urls,
                        bool do_ping = false);

  bool QueryVirtualHostDetails(const std::string& vs_name,
                               std::string* const vs_url);
  bool AddVirtualHost(const std::string& vs_name,
                      const std::string& vs_url,
                      bool vs_ssl,
                      const std::string& vs_cache_level);
  bool DeleteVirtualHost(const std::string& vs_name);
  //  bool DisableVirtualServer(
  //      ServerType server_type, const std::string& vs_name);

  bool ListSearchDefs(std::vector<std::string>* searchdef_names,
                      std::vector<std::string>* searchdef_contents);

  bool GarbageCollect(ServerType server_type,
                      std::uint32_t* delete_count, std::uint64_t* delete_size);

  bool Cleanup(ServerType server_type, std::string *cleaned_portables_data);

  static VsType GetVsType(const std::string& str_type) {
    return (str_type == "ge") ? TYPE_GE
            : (str_type == "map") ? TYPE_MAP : TYPE_BOTH;
  }
  static std::string GetStrVsType(VsType vs_type) {
    return (vs_type == TYPE_GE) ? "ge"
            : (vs_type == TYPE_MAP) ? "map" : "both";
  }

 private:
  bool IsServerHostSameAsPublishingHost(const ServerType server_type,
                                        bool* failed_to_find);
  bool IsFusionHostSameAsPublishingHost() const;
  bool IsServerHostSameAsFusionHost(const ServerType server_type,
                                    bool* failed_to_find);

  // Uploads on server all of the files (Stream and Search data) from
  // the Fusion database (indexes, packets, icons, etc) that are out of date
  // with the copies on the server.
  // Note: Syncing Search data, additionally, the POI-files are parsed and
  // the POI tables are created in gepoi database.
  bool SyncDatabase(const std::string& db_name);

  bool PingServer(ServerType server_type);
  bool PingServerWithTimeout(ServerType server_type);
  bool SyncFiles(ServerType server_type,
                 const std::string& args,
                 const std::vector<ManifestEntry>& entries,
                 const std::string& tmpdir);
  bool LocalTransfer(ServerType server_type,
                     const std::string& src_path,
                     const std::string& dest_path,
                     bool prefer_copy);
  bool LocalTransferWithRetry(const std::string& server_prefix,
                     const std::string& host_root,
                     ServerType server_type,
                     const std::string& tmpdir,
                     const std::string& current_path,
                     const std::string& orig_path);
  bool UploadFiles(ServerType server_type,
                   const std::vector<ManifestEntry>& entries,
                   const std::string& tmpdir,
                   bool report_progress);
  std::string GetServerPrefix(ServerType server_type);
  std::string GetHostRoot(ServerType server_type);

  bool GetVsUrl(ServerType server_type, const std::string& vs_name,
                std::string* vs_url);
  bool CompletePublishUrl(ServerType server_type, std::string* url);
  bool GetServerHost(ServerType server_type,
                     std::string* server_host, std::string* server_host_full);
  int LocateEntry(const std::vector<ManifestEntry>& entries,
                  const std::string& file_path);

  // Return the normalized db name for all publishing requests to the server.
  // input_db_name: is the gedb path name.
  // return: the normalized db name (gedb path without trailing "/").
  static std::string NormalizeDbName(const std::string input_db_name);

  // Normalizes target path.
  // Returns target path starting with "/" and without trailing "/".
  static std::string NormalizeTargetPath(const std::string &target_path);

  // Member variables.
  std::string stream_server_prefix_;
  std::string search_server_prefix_;
  std::string stream_host_root_;
  std::string search_host_root_;
  geProgress* progress_;
  // Force copy specifies to copy the files, don't try to do any links.
  const bool force_copy_;

  // Error messages
  static const std::string kErrInternal;
  static const std::string kErrInvalidDb;
  static const std::string kErrPing;
  static const std::string kErrPingStream;
  static const std::string kErrPingSearch;
  static const std::string kErrHeaderMismatch;
  static const std::string kErrServerPrefix;
  static const std::string kErrServerHost;
  static const std::string kErrAllowSymLinks;
  static const std::string kErrGetManifest;
  static const std::string kErrTmpDir;
  static const std::string kErrServerManifest;
  static const std::string kErrSyncStream;
  static const std::string kErrSyncSearch;
  static const std::string kErrVsUrl;
  static const std::string kErrPublishManifest;
  static const std::string kErrVsUrlMissing;
  static const std::string kErrJarPathMissing;
  static const std::string kErrClassNameMissing;
  static const std::string kErrVsCacheLevelIncorrect;
  static const std::string kErrUploadFailed;
  static const std::string kErrLnCpFailed;
  static const std::string kErrFilePaths;
  static const std::string kErrFileMissing;
  static const std::string kErrChmodFailure;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GEPUBLISH_PUBLISHERCLIENT_H_

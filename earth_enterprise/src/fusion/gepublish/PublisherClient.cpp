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


#include "fusion/gepublish/PublisherClient.h"

#include <time.h>
#include <qurl.h>

#include "fusion/gepublish/PublisherConstants.h"
#include "fusion/gepublish/CurlRequest.h"
#include "fusion/dbmanifest/dbmanifest.h"
#include "autoingest/.idl/storage/AssetDefs.h"
#include "common/geFilePool.h"
#include "common/khConstants.h"
#include "common/khFileUtils.h"
#include "common/khstl.h"
#include "common/khStringUtils.h"
#include "common/notify.h"
#include "common/geInstallPaths.h"
#include "common/khSpawn.h"
#include "common/khSimpleException.h"
#include "common/geProgress.h"


// Delimiter for multi-part parameters (array-parameters).
const char kMultiPartParameterDelimiter = ',';

const std::string PublisherClient::kErrInternal = "Internal error.";
const std::string PublisherClient::kErrInvalidDb = "Invalid database: ";
const std::string PublisherClient::kErrPing = "Unable to contact server.";
const std::string PublisherClient::kErrPingStream =
    "Unable to contact stream data server.";
const std::string PublisherClient::kErrPingSearch =
    "Unable to contact search data server.";
const std::string PublisherClient::kErrHeaderMismatch = kErrInternal
    + " Header size mismatch.";
const std::string PublisherClient::kErrServerPrefix = kErrInternal +
    " Could not get server prefix/host root.";
const std::string PublisherClient::kErrServerHost = kErrInternal +
    " Could not get server host name.";
const std::string PublisherClient::kErrAllowSymLinks = kErrInternal +
    " Could not get symlink config.";
const std::string PublisherClient::kErrGetManifest = kErrInternal +
    " Could not get database manifest.";
const std::string PublisherClient::kErrTmpDir = kErrInternal +
    " Unable to create temp dir.";
const std::string PublisherClient::kErrServerManifest = kErrInternal +
    " Could not get server manifest.";
const std::string PublisherClient::kErrSyncStream =
    "Stream server push failed.";
const std::string PublisherClient::kErrSyncSearch =
    "Search server push failed.";
const std::string PublisherClient::kErrVsUrl = kErrInternal +
    " Could not get Virtual Server URL.";
const std::string PublisherClient::kErrPublishManifest = kErrInternal +
    " Could not get publish manifest.";
const std::string PublisherClient::kErrVsUrlMissing =
    "Virtual Server URL missing.";
const std::string PublisherClient::kErrJarPathMissing = "Jar Path missing.";
const std::string PublisherClient::kErrClassNameMissing = "Class Name missing.";
const std::string PublisherClient::kErrVsCacheLevelIncorrect =
    "Incorrect cache level.";
const std::string PublisherClient::kErrUploadFailed = "Upload failed.";
const std::string PublisherClient::kErrLnCpFailed = kErrInternal +
    " Link/Copy failed.";
const std::string PublisherClient::kErrFilePaths = kErrInternal +
    " Could not get file paths from server.";
const std::string PublisherClient::kErrFileMissing = kErrInternal +
    " Could not locate file in manifest.";
const std::string PublisherClient::kErrChmodFailure = kErrInternal +
    " Failed to chmod tmp files.";

namespace {

void AppendMultiPartManifestParam(const std::vector<ManifestEntry> &entries,
                                  std::string *args) {
  if (entries.empty())
    return;

  std::string file_paths = "&FilePath=";
  std::string file_sizes = "&FileSize=";

  // Add first item without delimiter.
  file_paths += entries[0].orig_path;
  file_sizes += Itoa(entries[0].data_size);
  for (size_t i = 1; i < entries.size(); ++i) {
    const ManifestEntry &entry = entries[i];
    file_paths += kMultiPartParameterDelimiter + entry.orig_path;
    file_sizes += kMultiPartParameterDelimiter + Itoa(entry.data_size);
  }
  *args += file_paths;
  *args += file_sizes;
}

}  // namespace


PublisherClient::PublisherClient(const std::string& fusion_host,
                                 const ServerConfig& stream_server,
                                 const ServerConfig& search_server,
                                 geProgress* progress,
                                 geAuth* auth,
                                 const bool force_copy)
    : PublishHelper(fusion_host, stream_server, search_server, auth),
      progress_(progress),
      force_copy_(force_copy) {
}

PublisherClient::~PublisherClient() {
}


std::string PublisherClient::GetServerPrefix(ServerType server_type) {
  std::string& server_prefix = (server_type == STREAM_SERVER)
    ? stream_server_prefix_ : search_server_prefix_;
  // Return if we have a cached version
  if (!server_prefix.empty())
    return server_prefix;
  std::vector<std::string> vec_prefix;
  std::string args = "Cmd=Query&QueryCmd=ServerPrefix";
  if (!ProcessGetRequest(server_type, &args,
          "Gepublish-ServerPrefix", &vec_prefix) || vec_prefix.size() != 1)
    return server_prefix;

  CleanString(&vec_prefix[0], " \r");
  server_prefix = vec_prefix[0];
  if (server_prefix[server_prefix.size()-1] == '/')
    server_prefix = server_prefix.substr(0, server_prefix.size()-1);
  return server_prefix;
}


std::string PublisherClient::GetHostRoot(ServerType server_type) {
  std::string& host_root = (server_type == STREAM_SERVER)
    ? stream_host_root_ : search_host_root_;
  // Return if we have a cached version
  if (!host_root.empty())
    return host_root;
  std::vector<std::string> vec_prefix;
  std::string args = "Cmd=Query&QueryCmd=HostRoot";
  if (!ProcessGetRequest(server_type, &args,
          "Gepublish-HostRoot", &vec_prefix) || vec_prefix.size() != 1)
    return host_root;

  CleanString(&vec_prefix[0], " \r");
  host_root = vec_prefix[0];
  return host_root;
}


bool PublisherClient::GetVsUrl(ServerType server_type,
                               const std::string& vs_name,
                               std::string* vs_url) {
  assert(vs_url);
  std::vector<std::string> vec_vsurl;
  std::string args = "Cmd=Query&QueryCmd=VsDetails&VsName=" + vs_name;
  if (!ProcessGetRequest(server_type, &args,
          "Gepublish-VsUrl", &vec_vsurl) || vec_vsurl.size() != 1)
    return false;
  *vs_url = vec_vsurl[0];
  CleanString(vs_url, "\r");
  return true;
}


bool PublisherClient::GetServerHost(ServerType server_type,
                                    std::string* server_host,
                                    std::string* server_host_full) {
  assert(server_host);
  assert(server_host_full);
  std::vector<std::string> vec_serverhost;
  HeaderMap header_map;
  header_map[PublisherConstants::kHdrServerHost] = std::vector<std::string>();
  header_map[PublisherConstants::kHdrServerHostFull] =
      std::vector<std::string>();
  std::string args = "Cmd=Query&QueryCmd=ServerHost";
  if (!ProcessGetRequest(server_type, &args, &header_map)) {
    AppendErrMsg(kErrServerHost);
    return false;
  }

  if (header_map[PublisherConstants::kHdrServerHost].size() != 1 ||
      header_map[PublisherConstants::kHdrServerHostFull].size() != 1) {
    AppendErrMsg(kErrHeaderMismatch);
    return false;
  }

  *server_host = header_map[PublisherConstants::kHdrServerHost][0];
  *server_host_full = header_map[PublisherConstants::kHdrServerHostFull][0];
  return true;
}


bool PublisherClient::CompletePublishUrl(ServerType server_type,
                                         std::string* url) {
  assert(url);
  // relative urls must be completed before writing them in dbroot/js files.
  // we must use the true server host name queried from the server instead
  // of the one used to publish (coz we may be publishing via a ssh tunnel.
  if ((*url)[0] != '/')
    return true;

  std::string server_host, server_host_full;
  if (!GetServerHost(server_type, &server_host, &server_host_full))
    return false;
  *url = "http://" + server_host_full + *url;
  return true;
}


bool PublisherClient::PingServer(ServerType server_type) {
  std::vector<std::string> empty_vec;
  std::string args = "Cmd=Ping";
  return ProcessGetRequest(server_type, &args, "", &empty_vec);
}

bool PublisherClient::PingServerWithTimeout(ServerType server_type) {
  int orig_timeout = timeout_secs_;
  timeout_secs_ = kOptionalPingTimeoutSecs;
  bool result = PingServer(server_type);
  timeout_secs_ = orig_timeout;
  return result;
}

bool PublisherClient::PingServersWithTimeout() {
  if (!PingServerWithTimeout(STREAM_SERVER)) {
    AppendErrMsg(kErrPingStream);
    return false;
  }

  if (!PingServerWithTimeout(SEARCH_SERVER)) {
    AppendErrMsg(kErrPingSearch);
    return false;
  }

  return true;
}


bool PublisherClient::ListDatabases(ServerType server_type,
                                    FusionDbInfoVector *const db_infos,
                                    PortableInfoVector *const portable_infos) {
  assert(db_infos);
  assert(portable_infos);

  std::string args = "Cmd=Query&QueryCmd=ListDbs";
  HeaderMap header_map;
  header_map[PublisherConstants::kHdrDbName] = std::vector<std::string>();
  header_map[PublisherConstants::kHdrDbPrettyName] = std::vector<std::string>();
  header_map[PublisherConstants::kHdrHostName] = std::vector<std::string>();
  if (!ProcessGetRequest(server_type, &args, &header_map)) {
    AppendErrMsg(kErrPing);
    notify(NFY_DEBUG, "ProcessGetRequest() failed.");
    return false;
  }

  const std::vector<std::string>& db_names =
      header_map[PublisherConstants::kHdrDbName];
  const std::vector<std::string>& db_pretty_names =
      header_map[PublisherConstants::kHdrDbPrettyName];
  const std::vector<std::string>& host_names =
      header_map[PublisherConstants::kHdrHostName];
  if (db_names.size() != db_pretty_names.size() ||
      db_pretty_names.size() != host_names.size()) {
    AppendErrMsg(kErrHeaderMismatch);
    return false;
  }

  for (size_t i = 0; i < db_names.size(); ++i) {
    if (host_names[i].empty()) {
      portable_infos->push_back(PortableInfo(db_names[i], db_pretty_names[i]));
    } else {
      db_infos->push_back(
          FusionDbInfo(db_names[i], db_pretty_names[i], host_names[i]));
    }
  }

  return true;
}


bool PublisherClient::PublishedDatabases(
    ServerType server_type,
    std::vector<PublishedFusionDbInfo> *const published_db_infos,
    std::vector<PublishedPortableInfo> *const published_portable_infos) {
  assert(published_db_infos);
  assert(published_portable_infos);

  std::string args = "Cmd=Query&QueryCmd=PublishedDbs";
  HeaderMap header_map;
  header_map[PublisherConstants::kHdrTargetPath] = std::vector<std::string>();
  header_map[PublisherConstants::kHdrVsName] = std::vector<std::string>();
  header_map[PublisherConstants::kHdrDbName] = std::vector<std::string>();
  header_map[PublisherConstants::kHdrDbPrettyName] = std::vector<std::string>();
  header_map[PublisherConstants::kHdrHostName] = std::vector<std::string>();
  if (!ProcessGetRequest(server_type, &args, &header_map)) {
    AppendErrMsg(kErrPing);
    notify(NFY_DEBUG, "ProcessGetRequest() failed.");
    return false;
  }

  const std::vector<std::string>& target_paths =
      header_map[PublisherConstants::kHdrTargetPath];
  const std::vector<std::string>& vs_names =
      header_map[PublisherConstants::kHdrVsName];
  const std::vector<std::string>& db_names =
      header_map[PublisherConstants::kHdrDbName];
  const std::vector<std::string>& db_pretty_names =
      header_map[PublisherConstants::kHdrDbPrettyName];
  const std::vector<std::string>& host_names =
      header_map[PublisherConstants::kHdrHostName];

  if (target_paths.size() != vs_names.size() ||
      vs_names.size() != db_names.size() ||
      db_names.size() != db_pretty_names.size() ||
      vs_names.size() != host_names.size()) {
    AppendErrMsg(kErrHeaderMismatch);
    return false;
  }

  for (size_t i = 0; i < target_paths.size(); ++i) {
    if (host_names[i].empty()) {
      published_portable_infos->push_back(
          PublishedPortableInfo(db_names[i], db_pretty_names[i],
                                target_paths[i], vs_names[i]));
    } else {
      published_db_infos->push_back(
          PublishedFusionDbInfo(db_names[i], db_pretty_names[i], host_names[i],
                                target_paths[i], vs_names[i]));
    }
  }

  return true;
}


bool PublisherClient::ListTargetPaths(ServerType server_type,
                                      std::vector<std::string>* target_paths) {
  assert(target_paths);
  std::string args = "Cmd=Query&QueryCmd=ListTgs";
  HeaderMap header_map;
  header_map[PublisherConstants::kHdrTargetPath] = std::vector<std::string>();
  if (!ProcessPublishGetRequest(server_type, &args, &header_map)) {
    AppendErrMsg(kErrPing);
    notify(NFY_DEBUG, "ProcessGetRequest() failed.");
    return false;
  }
  *target_paths = header_map[PublisherConstants::kHdrTargetPath];
  return true;
}


bool PublisherClient::QueryDatabaseDetails(
    ServerType server_type,
    const std::string& input_db_name,
    std::vector<std::string>* file_names) {
  assert(file_names);
  std::string db_name = NormalizeDbName(input_db_name);

  std::string args = "Cmd=Query&QueryCmd=DbDetails&DbName=" + db_name;
  return ProcessGetRequest(
      server_type, &args, "Gepublish-FileName", file_names);
}


bool PublisherClient::QueryPublishContext(
    ServerType server_type,
    const std::string& in_target_path,
    std::vector<std::string>* publish_context) {

  try {
    assert(publish_context);
    std::string target_path = NormalizeTargetPath(in_target_path);
    notify(NFY_DEBUG, "Normalized target path is :%s", target_path.c_str());


    // Log the start of the targetdetails command, including time.
    // This process will log all NFY_DEBUG messages.
    notify(NFY_DEBUG, "\n:-----------------Start TargetDetails\n"
           "  %s", target_path.c_str());

    // Ping the stream server before we do anything.
    if (!PingServer(STREAM_SERVER)) {
      AppendErrMsg(kErrPingStream);
      return false;
    }
    notify(NFY_DEBUG, "Contacted STREAM_SERVER");

    std::string args = "Cmd=Query&QueryCmd=PublishedDbDetails&TargetPath="
            + target_path;

    if (!ProcessPublishGetRequest(
        STREAM_SERVER, &args, PublisherConstants::kHdrData, publish_context)) {
      notify(NFY_DEBUG, "ProcessPublishGetRequest(STREAM_SERVER) failed.");
      return false;
    }
    notify(NFY_DEBUG, "ProcessPublishGetRequest(STREAM_SERVER) succeeded.");

    // Print a successful completion message to the log.
    notify(NFY_DEBUG, "-----------------TargetDetails (Stream) successful\n");

    return true;
  } catch(const std::exception &e) {
    // Give a reasonable message, if we don't get a message,
    // direct them to a logfile if one exists.
    std::string error_message = e.what() ? e.what() : "";
    notify(NFY_DEBUG, "Caught Exception: %s", error_message.c_str());

    // Construct a reasonable message if we don't have one already.
    if (error_message.empty()) {
      error_message = "TargetDetails (Stream) failed.";
      // If we have a logfile, and get an unexplained exception here,
      // direct them to look at the logfile.
      const std::string& log_filename = LogFilename();
      if (!log_filename.empty())
        error_message += "\nPlease see the error log\n   " + log_filename +
            "\nfor more details.";
    }
    AppendErrMsg(error_message);
  }
  notify(NFY_DEBUG, "-----------------TargetDetails (Stream) FAILED\n");
  return false;
}

bool PublisherClient::AddDatabase(const std::string& input_db_name,
                                  const std::string& db_pretty_name) {
  std::string norm_input_db_name = NormalizeDbName(input_db_name);
  notify(NFY_DEBUG, "\n-----------------Start AddDatabase\n"
         "  db: %s : %s", db_pretty_name.c_str(), norm_input_db_name.c_str());

  try {
    // Ping the stream server before we do anything.
    if (!PingServer(STREAM_SERVER)) {
      AppendErrMsg(kErrPingStream);
      return false;
    }
    notify(NFY_DEBUG, "Add Database Contacted STREAM_SERVER");
    if (!PingServer(SEARCH_SERVER)) {
      AppendErrMsg(kErrPingSearch);
      return false;
    }
    notify(NFY_DEBUG, "Add Database Contacted SEARCH_SERVER");

    // Will throw an exception.
    std::string db_name = norm_input_db_name;
    // Note: returns asset specific database name.
    DbManifest db_manifest(&db_name);

    // Create a file pool that will be used by the index manifest.

    std::string publish_prefix;
    { // Since index manifest uses publish_prefix, we can safely use
      // STREAM_SERVER for determining publish_prefix.
      bool failed_to_find = true;
      const bool is_server_host_same_as_publishing =
          IsServerHostSameAsPublishingHost(STREAM_SERVER, &failed_to_find);
      if (failed_to_find) {
        return false;
      }
      if (is_server_host_same_as_publishing)  {
        const std::string server_prefix = GetServerPrefix(STREAM_SERVER);
        const std::string host_root = GetHostRoot(STREAM_SERVER);
        publish_prefix = khComposePath(server_prefix, host_root);
      } else if (IsFusionHostSameAsPublishingHost()) {
        publish_prefix = ".";
      }
    }

    geFilePool file_pool;
    std::vector<ManifestEntry> stream_manifest, search_manifest;
    db_manifest.GetPushManifest(
        file_pool, &stream_manifest, &search_manifest, "", publish_prefix);
    notify(NFY_DEBUG, "GetPushManifest %ld/%ld", stream_manifest.size(), search_manifest.size());

    std::string args = "Cmd=AddDb&DbName=" + db_name + "&DbPrettyName=" +
        db_pretty_name;
    std::string stream_args(args);

    AppendMultiPartManifestParam(stream_manifest, &stream_args);

    // Get database modification time based on {gedb/mapdb}/header.xml
    // timestamp.
    std::string header_file_path = norm_input_db_name + "/" + kHeaderXmlFile;
    std::uint64_t header_file_size = 0;
    time_t header_file_mtime = 0;
    if (!khGetFileInfo(header_file_path, header_file_size, header_file_mtime)) {
      throw khSimpleErrnoException(
          "Unable to get file info (modification time, size) for ")
          << header_file_path;
    }

    std::string db_timestamp =
        GetUtcTimeStringWithFormat(header_file_mtime,
                                   kISO_8601_DateTimeFormatString);

    stream_args += "&DbTimestamp=" + db_timestamp;

    // Get database size based on file sizes from push manifest.
    std::uint64_t db_size = 0;
    for (size_t i = 0; i < stream_manifest.size(); ++i) {
      db_size += stream_manifest[i].data_size;
    }
    stream_args += "&DbSize=" + Itoa(db_size);

    // Get database properties.
    if (db_manifest.UseGoogleBasemap()) {
      stream_args += "&DbUseGoogleBasemap=1";
    }

    std::vector<std::string> empty_vec;
    notify(NFY_DEBUG, "ProcessGetRequest STREAM_SERVER started");
    if (!ProcessGetRequest(STREAM_SERVER, &stream_args, "", &empty_vec)) {
      notify(NFY_DEBUG, "ProcessGetRequest(STREAM_SERVER) failed.");
      return false;
    }
    notify(NFY_DEBUG, "ProcessGetRequest STREAM_SERVER finished.");

    std::string search_args(args);

    AppendMultiPartManifestParam(search_manifest, &search_args);

    // Do not register the database with the search server if we dont have any
    // POI files.
    if (search_manifest.size() > 0) {
      notify(NFY_DEBUG, "ProcessGetRequest SEARCH_SERVER started");
      if (!ProcessGetRequest(SEARCH_SERVER, &search_args, "", &empty_vec)) {
        notify(NFY_DEBUG, "ProcessGetRequest(SEARCH_SERVER) failed.");
        return false;
      }
      notify(NFY_DEBUG, "ProcessGetRequest SEARCH_SERVER finished.");
    }

    notify(NFY_DEBUG, "-----------------AddDatabase successful\n");

    return true;
  } catch(const std::exception &e) {
    AppendErrMsg(e.what());
  }
  return false;
}


bool PublisherClient::DeleteDatabase(const std::string& input_db_name) {
  std::string db_name = NormalizeDbName(input_db_name);

  notify(NFY_DEBUG, "\n-----------------Start DeleteDatabase\n"
         "  db: %s", db_name.c_str());

  // Ping the stream server before we do anything.
  if (!PingServer(STREAM_SERVER)) {
    AppendErrMsg(kErrPingStream);
    return false;
  }
  notify(NFY_DEBUG, "Delete Database Contacted STREAM_SERVER");
  if (!PingServer(SEARCH_SERVER)) {
    AppendErrMsg(kErrPingSearch);
    return false;
  }
  notify(NFY_DEBUG, "Delete Database Contacted SEARCH_SERVER");

  std::string args = "Cmd=DeleteDb&DbName=" + db_name;
  std::string search_args(args);
  std::vector<std::string> empty_vec;
  if (!ProcessGetRequest(STREAM_SERVER, &args, "", &empty_vec)) {
    notify(NFY_DEBUG, "ProcessGetRequest(STREAM_SERVER) failed.");
    return false;
  }
  notify(NFY_DEBUG, "ProcessGetRequest(STREAM_SERVER) finished.");

  if (!ProcessGetRequest(SEARCH_SERVER, &search_args, "", &empty_vec)) {
    notify(NFY_DEBUG, "ProcessGetRequest(SEARCH_SERVER) failed.");
    return false;
  }
  notify(NFY_DEBUG, "ProcessGetRequest(SEARCH_SERVER) finished.");

  notify(NFY_DEBUG, "-----------------DeleteDatabase successful\n.");

  return true;
}

std::string PublisherClient::NormalizeDbName(const std::string input_db_name) {
  // We want to ensure that all DB names on server do not end in "/".
  // The user may errantly add the slash without knowing it and cause duplicate
  // copies of the same db to exist.
  return ReplaceSuffix(input_db_name, "/", "");
}

std::string PublisherClient::NormalizeTargetPath(
    const std::string &target_path) {
  std::string norm_target_path;
  if (target_path.empty()) {
    notify(NFY_DEBUG , "\n------------ target_path is empty ---------");
    return norm_target_path;
  }
  norm_target_path = (target_path[0] != '/') ?
      ("/" + target_path) : target_path;
  notify(NFY_DEBUG, "\n-------- normalized target path --------");
  return ReplaceSuffix(norm_target_path, "/", "");
}

// Push Stream and Search data on GEE Server.
bool PublisherClient::PushDatabase(const std::string& db_name) {
  // Ping the stream server before we do anything.
  if (!PingServer(STREAM_SERVER)) {
    AppendErrMsg(kErrPingStream);
    return false;
  }
  notify(NFY_DEBUG, "Push Database Contacted STREAM_SERVER");
  if (!PingServer(SEARCH_SERVER)) {
    AppendErrMsg(kErrPingSearch);
    return false;
  }
  notify(NFY_DEBUG, "Push Database Contacted SEARCH_SERVER");

  std::string norm_db_name = NormalizeDbName(db_name);

  // Sync Stream and Search data for specified database.
  return SyncDatabase(norm_db_name);
}

bool PublisherClient::SyncDatabase(const std::string& db_name) {
  try {
    const std::string server_prefix = GetServerPrefix(STREAM_SERVER);
    const std::string host_root = GetHostRoot(STREAM_SERVER);
    const std::string publish_prefix = khComposePath(server_prefix, host_root);

    notify(NFY_DEBUG, "-----------------Start SyncDatabase\n"
           "  serverprefix: %s hostroot: %s.",
           server_prefix.c_str(), host_root.c_str());

    if (server_prefix.empty() || host_root.empty()) {
      AppendErrMsg(kErrServerPrefix);
      return false;
    }

    // It may throw an exception on an error.
    // Note: returns asset specific database name.
    std::string asset_db_name = db_name;
    DbManifest db_manifest(&asset_db_name);

    // Create temporary directory for the DB and Index manifests.
    std::string tmpdir = khCreateTmpDir("gepublish.");
    if (tmpdir.empty()) {
      AppendErrMsg(kErrTmpDir);
      return false;
    }
    notify(NFY_DEBUG, "Created gepublish tempdir");

    // Create a guard object that ensures that the temporary directory gets
    // deleted automatically when we return.
    khCallGuard<const std::string&, bool> prune_guard(khPruneDir, tmpdir);

    std::vector<ManifestEntry> manifest_entries;
    geFilePool file_pool;
    db_manifest.GetPushManifest(file_pool, &manifest_entries,
                                &manifest_entries, tmpdir, publish_prefix);
    notify(NFY_DEBUG, "GetPushManifest");

    // chmod all the temporary files to 0755 so that the publisher servlet
    // can access it just in case the user's umask disables access to all
    // completely.
    CmdLine cmdline;
    cmdline << "chmod" << "-R" << "755" << tmpdir;
    if (!cmdline.System()) {
      AppendErrMsg(kErrChmodFailure);
      return false;
    }
    notify(NFY_DEBUG, "chmod tmpdir 0755");

    // Transfer Stream data.
    std::string args = "Cmd=SyncDb&DbName=" + asset_db_name;
    notify(NFY_DEBUG, "SyncFiles(STREAM_SERVER)...");
    if (!SyncFiles(STREAM_SERVER, args, manifest_entries, tmpdir)) {
      AppendErrMsg(kErrSyncStream);
      return false;
    }
    notify(NFY_DEBUG, "SyncFiles(STREAM_SERVER) done.");

    // Transfer search data (POI files), create POI tables.
    notify(NFY_DEBUG, "SyncFiles(SEARCH_SERVER)...");
    if (!SyncFiles(SEARCH_SERVER, args, manifest_entries, tmpdir)) {
      AppendErrMsg(kErrSyncSearch);
      return false;
    }
    notify(NFY_DEBUG, "SyncFiles(SEARCH_SERVER) done.");
    notify(NFY_DEBUG, "-----------------SyncDatabase successful\n.");

    return true;
  } catch(const std::exception &e) {
    AppendErrMsg(e.what());
  }
  notify(NFY_DEBUG, "-----------------SyncDatabase FAILED\n.");
  return false;
}

bool PublisherClient::PublishDatabase(const std::string& in_db_name,
                                      const std::string& in_target_path,
                                      const std::string& vh_name,
                                      const bool ec_default_db,
                                      const bool poi_search,
                                      const bool enhanced_search,
                                      const bool serve_wms) {
  try {
    std::string target_path = NormalizeTargetPath(in_target_path);
    if (target_path.empty()) {
      AppendErrMsg("The target path is not specified.\n");
      return false;
    }

    std::string stream_vs_name = vh_name;
    if (stream_vs_name.empty()) {
      stream_vs_name = stream_server_.vs;
    }

    if (stream_vs_name.empty()) {
      AppendErrMsg("The Virtual Host name is not specified.\n");
      return false;
    }

    std::string db_name = NormalizeDbName(in_db_name);

    // Log the start of the publish command, including time.
    // This process will log all NFY_DEBUG messages.
    notify(NFY_DEBUG, "\n-----------------Start Publishing\n"
           "  %s target: %s", db_name.c_str(), target_path.c_str());

    // Ping the stream server before we do anything.
    if (!PingServer(STREAM_SERVER)) {
      AppendErrMsg(kErrPingStream);
      return false;
    }
    notify(NFY_DEBUG, "Contacted STREAM_SERVER");

    // To get asset_root specific DB name.
    // will throw an exception
    DbManifest db_manifest(&db_name);

    std::string db_id;

    std::string stream_args = "Cmd=PublishDb&DbName=" + db_name;
    stream_args += "&VirtualHostName=" + stream_vs_name;
    stream_args += "&TargetPath=" + target_path;
    stream_args += "&DbType=" + Itoa(db_manifest.GetDbType());
    if (poi_search) {
      stream_args += "&SearchDefName=POISearch";
      if (enhanced_search) {
        stream_args += "&PoiFederated=1";
      }
    }
    if (ec_default_db) {
      stream_args += "&EcDefaultDb=1" ;
    }
    if (serve_wms) {
      stream_args += "&ServeWms=1";
    }
    std::vector<std::string> empty_vector;
    if (!ProcessPublishGetRequest(
        STREAM_SERVER, &stream_args, "", &empty_vector)) {
      notify(NFY_DEBUG, "ProcessPublishGetRequest(STREAM_SERVER) failed.");
      return false;
    }
    notify(NFY_DEBUG, "ProcessPublishGetRequest(STREAM_SERVER) succeded.");

    // Print a successful completion message to the log.
    notify(NFY_DEBUG, "-----------------Publish (Stream) successful\n");

    return true;
  } catch(const std::exception &e) {
    // Give a reasonable message, if we don't get a message,
    // direct them to a logfile if one exists.
    std::string error_message = e.what() ? e.what() : "";
    notify(NFY_DEBUG, "Caught Exception: %s", error_message.c_str());

    // Construct a reasonable message if we don't have one already.
    if (error_message.empty()) {
      error_message = "Publish (Stream) failed.";
      // If we have a logfile, and get an unexplained exception here,
      // direct them to look at the logfile.
      const std::string& log_filename = LogFilename();
      if (!log_filename.empty())
        error_message += "\nPlease see the error log\n   " + log_filename +
            "\nfor more details.";
    }
    AppendErrMsg(error_message);
  }
  notify(NFY_DEBUG, "-----------------Publish (Stream) FAILED\n.");
  return false;
}

bool PublisherClient::RepublishDatabase(const std::string& in_db_name,
                                        const std::string& in_target_path) {
  try {
    std::string target_path = NormalizeTargetPath(in_target_path);
    if (target_path.empty()) {
      AppendErrMsg("The target path is not specified.\n");
      return false;
    }

    std::string db_name = NormalizeDbName(in_db_name);

    // Log the start of the republish command, including time.
    // This process will log all NFY_DEBUG messages.
    notify(NFY_DEBUG, "\n:-----------------Start RepublishDb\n"
           "  %s target: %s", db_name.c_str(), target_path.c_str());

    // Ping the stream server before we do anything.
    if (!PingServer(STREAM_SERVER)) {
      AppendErrMsg(kErrPingStream);
      return false;
    }
    notify(NFY_DEBUG, "Contacted STREAM_SERVER");

    // To get asset_root specific DB name.
    // will throw an exception
    DbManifest db_manifest(&db_name);

    std::string db_id;

    std::string stream_args = "Cmd=RepublishDb&DbName=" + db_name;
    stream_args += "&TargetPath=" + target_path;

    std::vector<std::string> empty_vector;
    if (!ProcessPublishGetRequest(
        STREAM_SERVER, &stream_args, "", &empty_vector)) {
      notify(NFY_DEBUG, "ProcessPublishGetRequest(STREAM_SERVER) failed.");
      return false;
    }
    notify(NFY_DEBUG, "ProcessPublishGetRequest(STREAM_SERVER) succeeded.");

    // Print a successful completion message to the log.
    notify(NFY_DEBUG, "-----------------RepublishDb (Stream) successful\n.");

    return true;
  } catch(const std::exception &e) {
    // Give a reasonable message, if we don't get a message,
    // direct them to a logfile if one exists.
    std::string error_message = e.what() ? e.what() : "";
    notify(NFY_DEBUG, "Caught Exception: %s", error_message.c_str());

    // Construct a reasonable message if we don't have one already.
    if (error_message.empty()) {
      error_message = "RepublishDb (Stream) failed.";
      // If we have a logfile, and get an unexplained exception here,
      // direct them to look at the logfile.
      const std::string& log_filename = LogFilename();
      if (!log_filename.empty())
        error_message += "\nPlease see the error log\n   " + log_filename +
            "\nfor more details.";
    }
    AppendErrMsg(error_message);
  }
  notify(NFY_DEBUG, "-----------------RepublishDb (Stream) FAILED\n.");
  return false;
}

bool PublisherClient::SwapTargets(const std::string& in_target_path_a,
                                  const std::string& in_target_path_b) {
  try {
    std::string target_path_a = NormalizeTargetPath(in_target_path_a);
    std::string target_path_b = NormalizeTargetPath(in_target_path_b);

    if (target_path_a.empty() || target_path_b.empty()) {
      AppendErrMsg("The target path is not specified.\n");
      return false;
    }

    // Log the start of the swaptargets command, including time.
    // This process will log all NFY_DEBUG messages.
    notify(NFY_DEBUG, "\n:-----------------Start SwapTargets\n"
       "  %s with target: %s", target_path_a.c_str(), target_path_b.c_str());

    // Ping the stream server before we do anything.
    if (!PingServer(STREAM_SERVER)) {
      AppendErrMsg(kErrPingStream);
      return false;
    }
    notify(NFY_DEBUG, "Contacted STREAM_SERVER");

    std::string stream_args = "Cmd=SwapTargets&TargetPathA="
         + target_path_a + "&TargetPathB=" + target_path_b;

    std::vector<std::string> empty_vector;
    if (!ProcessPublishGetRequest(
        STREAM_SERVER, &stream_args, "", &empty_vector)) {
      notify(NFY_DEBUG, "ProcessPublishGetRequest(STREAM_SERVER) failed.");
      return false;
    }
    notify(NFY_DEBUG, "ProcessPublishGetRequest(STREAM_SERVER) succeeded.");

    // Print a successful completion message to the log.
    notify(NFY_DEBUG, "-----------------SwapTargets (Stream) successful\n.");

    return true;
  } catch(const std::exception &e) {
    // Give a reasonable message, if we don't get a message,
    // direct them to a logfile if one exists.
    std::string error_message = e.what() ? e.what() : "";
    notify(NFY_DEBUG, "Caught Exception: %s", error_message.c_str());

    // Construct a reasonable message if we don't have one already.
    if (error_message.empty()) {
      error_message = "SwapTargets (Stream) failed.";
      // If we have a logfile, and get an unexplained exception here,
      // direct them to look at the logfile.
      const std::string& log_filename = LogFilename();
      if (!log_filename.empty())
        error_message += "\nPlease see the error log\n   " + log_filename +
            "\nfor more details.";
    }
    AppendErrMsg(error_message);
  }
  notify(NFY_DEBUG, "-----------------SwapTargets (Stream) FAILED\n");

  return false;
}

bool PublisherClient::UnpublishDatabase(const std::string& in_target_path) {
  try {
    std::string target_path = NormalizeTargetPath(in_target_path);
    if (target_path.empty()) {
      std::string error_message =
          "The target path is not specified or not valid: " +
          in_target_path + "\n";
      AppendErrMsg(error_message);
      return false;
    }
    notify(NFY_DEBUG, "\n-----------------Start Unpublishing\n"
           "  target: %s", target_path.c_str());

    std::string args = "Cmd=UnPublishDb&TargetPath=" + target_path;
    std::vector<std::string> empty_vector;
    if (!ProcessPublishGetRequest(STREAM_SERVER, &args, "", &empty_vector)) {
      notify(NFY_DEBUG, "ProcessPublishGetRequest(STREAM_SERVER) failed.");
      return false;
    }

    notify(NFY_DEBUG, "ProcessPublishGetRequest(STREAM_SERVER) succeded.");

    // Print a successful completion message to the log.
    notify(NFY_DEBUG, "-----------------Unpublish successful\n.");
    return true;
  }  catch(const std::exception &e) {
    // Give a reasonable message, if we don't get a message,
    // direct them to a logfile if one exists.
    std::string error_message = e.what() ? e.what() : "";
    notify(NFY_DEBUG, "Caught Exception: %s", error_message.c_str());

    // Construct a reasonable message if we don't have one already.
    if (error_message.empty()) {
      error_message = "Unpublish failed.";
      // If we have a logfile, and get an unexplained exception here,
      // direct them to look at the logfile.
      const std::string& log_filename = LogFilename();
      if (!log_filename.empty())
        error_message += "\nPlease see the error log\n   " + log_filename +
            "\nfor more details.";
    }
    AppendErrMsg(error_message);
  }
  notify(NFY_DEBUG, "-----------------Unpublish FAILED\n.");
  return false;
}


bool PublisherClient::ListVirtualHosts(std::vector<std::string>* vs_names,
                                       std::vector<std::string>* vs_urls,
                                       bool do_ping) {
  // In certain cases the StreamServlet requests will hang.
  // Need to attempt a server ping with timeout in some situations to avoid
  // hanging fusion.
  if (do_ping && !PingServerWithTimeout(STREAM_SERVER)) {
      AppendErrMsg(kErrPingStream);
      return false;
  }

  std::string args = "Cmd=Query&QueryCmd=ListVss";
  HeaderMap header_map;
  header_map[PublisherConstants::kHdrVsName] = std::vector<std::string>();
  header_map[PublisherConstants::kHdrVsUrl] = std::vector<std::string>();
  if (!ProcessPublishGetRequest(STREAM_SERVER, &args, &header_map)) {
    notify(NFY_DEBUG, "ProcessGetRequest() failed.");
    return false;
  }

  *vs_names = header_map[PublisherConstants::kHdrVsName];
  *vs_urls = header_map[PublisherConstants::kHdrVsUrl];

  for (unsigned int i = 0; i < vs_urls->size(); ++i) {
    std::string& vs_url = (*vs_urls)[i];
    if (vs_url[0] == '/')
      vs_url = ServerUrl(STREAM_SERVER) + vs_url;
  }
  return true;
}


bool PublisherClient::QueryVirtualHostDetails(const std::string& vs_name,
                                              std::string* const vs_url) {
  assert(vs_url);
  std::vector<std::string> vec_urls;
  std::string args = "Cmd=Query&QueryCmd=VsDetails&VsName=" + vs_name;
  if (ProcessPublishGetRequest(
          STREAM_SERVER, &args, "Gepublish-VsUrl", &vec_urls)) {
    if (!vec_urls.empty()) {
      *vs_url = vec_urls[0];
      if ((*vs_url)[0] == '/') {
        *vs_url = ServerUrl(STREAM_SERVER) + *vs_url;
      }
    }
    return true;
  }
  return false;
}


bool PublisherClient::AddVirtualHost(const std::string& vs_name,
                                     const std::string& vs_url,
                                     bool vs_ssl,
                                     const std::string& vs_cache_level) {
  if (vs_url.empty()) {
    AppendErrMsg(kErrVsUrlMissing);
    return false;
  }

  // Make sure the cache level is either not set or is set to [1 - 3].
  if (!vs_cache_level.empty()) {
    int level = atoi(vs_cache_level.c_str());
    if (level < 1 || level > 3) {
      AppendErrMsg(kErrVsCacheLevelIncorrect);
      return false;
    }
  }

  std::string args = "Cmd=AddVs&VsName=" + vs_name;
  args += "&VsUrl=" + vs_url;

  if (vs_ssl) {
    args += "&VsSsl=1";
  }

  if (!vs_cache_level.empty()) {
    args += "&VsCacheLevel=" + vs_cache_level;
  }
  std::vector<std::string> empty_vec;
  if (!ProcessPublishGetRequest(STREAM_SERVER, &args, "", &empty_vec)) {
    notify(NFY_DEBUG, "ProcessGetRequest() failed.");
    return false;
  }
  return true;
}


bool PublisherClient::DeleteVirtualHost(const std::string& vs_name) {
  std::string args = "Cmd=DeleteVs&VsName=" + vs_name;
  std::vector<std::string> empty_vec;
  if (!ProcessPublishGetRequest(STREAM_SERVER, &args, "", &empty_vec)) {
    notify(NFY_DEBUG, "ProcessGetRequest() failed.");
    return false;
  }
  return true;
}

// TODO: drop DisableVirtualServer() whether it is not needed.
#if 0
bool PublisherClient::DisableVirtualServer(ServerType server_type,
                                           const std::string& vs_name) {
  std::string args = "Cmd=DisableVs&VsName=" + vs_name;
  std::vector<std::string> empty_vec;
  if (!ProcessGetRequest(server_type, &args, "", &empty_vec)) {
    notify(NFY_DEBUG, "ProcessGetRequest() failed.");
    return false;
  }
  return true;
}
#endif

// TODO: implement getting list of search definitions for
// console tool.
bool PublisherClient::ListSearchDefs(
    std::vector<std::string>* searchdef_names,
    std::vector<std::string>* searchdef_contents) {
  assert(searchdef_names);
  assert(searchdef_contents);
#if 0
  std::string args = "Cmd=Query&QueryCmd=ListSearchDefs";
  HeaderMap header_map;
  header_map[PublisherConstants::kHdrPluginName] = std::vector<std::string>();
  header_map[PublisherConstants::kHdrPluginClassName] =
    std::vector<std::string>();
  if (!ProcessGetRequest(SEARCH_SERVER, &args, &header_map)) {
    AppendErrMsg(kErrPing);
    notify(NFY_DEBUG, "ProcessGetRequest() failed.");
    return false;
  }
  *plugin_names = header_map[PublisherConstants::kHdrPluginName];
  *plugin_class_names = header_map[PublisherConstants::kHdrPluginClassName];
  if (plugin_names->size() != plugin_class_names->size()) {
    AppendErrMsg(kErrHeaderMismatch);
    return false;
  }
#endif
  return true;
}

bool PublisherClient::GarbageCollect(ServerType server_type,
                                     std::uint32_t* delete_count,
                                     std::uint64_t* delete_size) {
  std::string args = "Cmd=GarbageCollect";
  HeaderMap header_map;
  header_map[PublisherConstants::kHdrDeleteCount] = std::vector<std::string>();
  header_map[PublisherConstants::kHdrDeleteSize] = std::vector<std::string>();
  if (!ProcessGetRequest(server_type, &args, &header_map)) {
    notify(NFY_DEBUG, "ProcessGetRequest() failed.");
    return false;
  }
  if (header_map[PublisherConstants::kHdrDeleteCount].size() > 0)
    *delete_count =
        atoi(header_map[PublisherConstants::kHdrDeleteCount][0].c_str());
  if (header_map[PublisherConstants::kHdrDeleteSize].size() > 0) {
    char* p;
    *delete_size = strtoll(
        header_map[PublisherConstants::kHdrDeleteSize][0].c_str(), &p, 10);
  }
  return true;
}

bool PublisherClient::Cleanup(ServerType server_type,
                              std::string *cleaned_portables_data) {
  assert(cleaned_portables_data);
  std::string args = "Cmd=Cleanup";
  HeaderMap header_map;
  header_map[PublisherConstants::kHdrData] = std::vector<std::string>();
  if (!ProcessGetRequest(server_type, &args, &header_map)) {
    notify(NFY_DEBUG, "ProcessGetRequest() failed.");
    return false;
  }
  if (header_map[PublisherConstants::kHdrData].size() > 0) {
    *cleaned_portables_data = header_map[PublisherConstants::kHdrData][0];
  }
  return true;
}


bool PublisherClient::LocalTransfer(ServerType server_type,
                                    const std::string& src_path,
                                    const std::string& dest_path,
                                    bool prefer_copy) {
  std::string args = "Cmd=LocalTransfer";
  args += "&FilePath=" + src_path + "&DestFilePath=" + dest_path;

  // force_copy overrides prefer_copy logic and specifies just do copy.
  if (force_copy_) {
    args += "&ForceCopy=Y";
  } else if (prefer_copy) {
    // Try creating a hard link at first and then if prefer_copy is Y,
    // just do copy, else try creating a symbolic link and then do copy.
    // Note: seems prefer_copy is backdoor for fusion UI
    // application. Because we run it not under root or some fusion-user, and
    // fusion is not setuid-application, we have no permission to copy files
    // into published_dbs directory. Here, backdoor is a chance to create
    // hard links for files.
    args += "&PreferCopy=Y";
  }

  std::vector<std::string> empty_vec;
  return ProcessGetRequest(server_type, &args, "", &empty_vec);
}

bool PublisherClient::LocalTransferWithRetry(const std::string& server_prefix,
                                          const std::string& host_root,
                                          ServerType server_type,
                                          const std::string& tmpdir,
                                          const std::string& current_path,
                                          const std::string& orig_path) {

  // Note: all files coming to upload should exist.
  assert(khExists(current_path));
  std::string dest_path(server_prefix + "/" +
                        host_root + orig_path);
  assert(current_path != dest_path);  // We send only delta always
  // The logic for prefer_copy was fixed in 3.0.3.
  // It used to set prefer_copy = !tmp_dir.empty(), when
  // in fact the desired effect is only to prefer copies for
  // source files that are in the tmp_dir, and to defer to the
  // server setting for AllowSymLinks: Y in PUBLISH_ROOT/.config.

  // If the source file is in the temp directory, we want to copy (hard
  // link is OK whenever possible).
  // Otherwise, a symbolic link is OK whenever possible.
  bool prefer_copy = !tmpdir.empty() && current_path.find(tmpdir) == 0;

  notify(NFY_VERBOSE, "Transfering '%s' to '%s'.\n\tServer type: %s\n\tPrefer copy: %s",
    current_path.c_str(),
    dest_path.c_str(),
    (server_type == STREAM_SERVER) ? kStreamSpace.c_str() : kSearchSpace.c_str(),
    prefer_copy?"true":"false");

  // Many LocalTransfers in succession may sometimes overwhelm a server.
  // To be safe we do a handful of retries with increasing delays between
  // attempts.
  int tries = 4;
  int sleep_secs = 15;
  while (!LocalTransfer(
              server_type, current_path, dest_path, prefer_copy)) {
    if (--tries == 0) {
      return false;
    }
    notify(NFY_DEBUG, "Retrying Local Transfer.");
    sleep(sleep_secs);  // Sleep before the next retry.
    sleep_secs *= 2;  // Double the sleep time after each retry.
  }

  return true;
}


bool PublisherClient::IsServerHostSameAsPublishingHost(
    const ServerType server_type, bool* failed_to_find) {
  const std::string publishing_host = khHostname();
  std::string server_host, server_host_full;
  *failed_to_find = !GetServerHost(server_type,
                                   &server_host, &server_host_full);
  if (*failed_to_find) {
    return false;
  } else {
    return (server_host == publishing_host ||
            server_host_full == publishing_host);
  }
}


bool PublisherClient::IsFusionHostSameAsPublishingHost() const {
  const std::string publishing_host = khHostname();
  return (fusion_host_ == publishing_host);
}


bool PublisherClient::IsServerHostSameAsFusionHost(
    const ServerType server_type, bool* failed_to_find) {
  std::string server_host, server_host_full;
  *failed_to_find = !GetServerHost(server_type,
                                   &server_host, &server_host_full);
  if (*failed_to_find) {
    return false;
  } else {
    return server_host == fusion_host_ || server_host_full == fusion_host_;
  }
}


bool PublisherClient::UploadFiles(ServerType server_type,
                                  const std::vector<ManifestEntry>& entries,
                                  const std::string& tmpdir,
                                  bool report_progress) {
  const std::string server_prefix = GetServerPrefix(server_type);
  const std::string host_root = GetHostRoot(server_type);
  if (server_prefix.empty() || host_root.empty()) {
    AppendErrMsg(kErrServerPrefix);
    return false;
  }

  // If the server is running on the same machine as the publishing fusion then
  // either create hard-links (if on the same partition) or direct copy.
  // Get the server canonical hostname as well as just the hostname so that
  // we don't run into problems of host != host.domain.
  bool failed_to_find;
  const bool is_server_host_same_as_publishing =
      IsServerHostSameAsPublishingHost(server_type, &failed_to_find);
  if (failed_to_find) {
    return false;
  }

  std::int64_t processed_size = 0;
  if (is_server_host_same_as_publishing)  {
    notify(NFY_DEBUG, "Transfering files locally");
    size_t num_entries = entries.size();
    for (size_t i = 0; i < num_entries; ++i) {
      const ManifestEntry& entry = entries[i];

      if (!LocalTransferWithRetry(server_prefix, host_root, server_type,
                                  tmpdir, entry.current_path, entry.orig_path)) {
        return false;
      }

      if (report_progress && progress_ != NULL) {
        if (!progress_->incrementDone(entry.data_size)) {
          notify(NFY_WARN, "Push interrupted");
          return false;
        }
        processed_size += entry.data_size;
        notify(NFY_DEBUG, "Done files: %zd/%zd, bytes: %zd",
               i + 1, num_entries, processed_size);
      }
    }
    return true;
  } else {
    notify(NFY_DEBUG, "Transfering files remotely");
    std::string dav_root = (server_type == STREAM_SERVER) ?
        kStreamSpace : kSearchSpace;
    std::string url = ServerUrl(server_type) + "/" + dav_root;
    const ServerConfig& server = Server(server_type);
    UploadRequest request(username_, password_, url, server.cacert_ssl,
                          server.insecure_ssl, host_root, &entries);
    return request.Start(report_progress, progress_);
  }
}


bool PublisherClient::SyncFiles(
    ServerType server_type, const std::string& _args,
    const std::vector<ManifestEntry>& manifest_entries,
    const std::string& tmpdir) {
  std::string lookfor_header = "Gepublish-FileName";
  bool successful = false;
  int max_retries = 2;
  int cur_retry = 0;
  while (!successful && cur_retry < max_retries) {
    if (cur_retry > 0 && server_type == SEARCH_SERVER) {
      notify(NFY_DEBUG,
             "Parsing POI files and filling POI database when syncing...");
    } else if (cur_retry > 0) {
      notify(NFY_DEBUG,
             "Processing stream files when syncing...");
    }

    // Get the delta list of files to complete this DB.
    // Note: historically, ProcessGetRequest() modifies args
    // by inserting additional parameters (e.g. Host). Here, we create a copy of
    // source args and send to ProcessGetRequest() to eliminate duplicating of
    // parameters in HTTP request on next iteration.
    std::string args = _args;
    std::vector<std::string> file_paths;
    if (!ProcessGetRequest(server_type, &args, lookfor_header, &file_paths)) {
      AppendErrMsg(kErrFilePaths);
      return false;
    }

    if (cur_retry > 0 && server_type == SEARCH_SERVER) {
      notify(NFY_DEBUG, "Done.");
    }

    if (file_paths.size() > 0) {
      std::vector<ManifestEntry> upload_entries;
      std::string remove_chars = "\r\n";
      for (size_t i = 0; i < file_paths.size(); ++i) {
        // Clean the path name. Remove spaces, newlines.
        CleanString(&file_paths[i], remove_chars);
        int index = LocateEntry(manifest_entries, file_paths[i]);
        if (index == -1) {
          AppendErrMsg(kErrFileMissing);
          notify(NFY_DEBUG, "Cannot locate: %s", file_paths[i].c_str());
          return false;
        }

        upload_entries.push_back(manifest_entries[index]);

        // some files are 'dependent files' that are not part of the original manifest reported to
        // server but we still need to upload those files as they complete the specified file in the
        // original manifest.  We can't include the file in the original manifest because in cases
        // like search manifests server does special processing on files in that manifest that
        // can't be done on the dependent files.
        for(size_t dep_idx = 0; dep_idx < manifest_entries[index].dependents.size(); ++dep_idx) {
          upload_entries.push_back(manifest_entries[index].dependents[dep_idx]);
        }
      }

      // Compute total files size for progress reporting on first try only.
      if (progress_ != NULL && cur_retry == 0) {
        std::uint64_t total_size = 0;
        for (size_t i = 0; i < upload_entries.size(); ++i) {
          total_size += upload_entries[i].data_size;
        }
        notify(NFY_DEBUG, "Total files to push: %zu, size (bytes): %zu",
               upload_entries.size(), total_size);
        progress_->incrementTotal(static_cast<std::int64_t>(total_size));
      } else {
        notify(NFY_DEBUG, "Total files to push: %zu",
               upload_entries.size());
      }

      bool REPORT_PROGRESS = true;
      if (!UploadFiles(server_type, upload_entries, tmpdir, REPORT_PROGRESS)) {
        AppendErrMsg(kErrUploadFailed);
        return false;
      }
    } else {
      successful = true;
      notify(NFY_DEBUG, "No files to push");
    }
    ++cur_retry;
  }

  if (!successful) {
    notify(NFY_DEBUG,
           "Couldn't push all Fusion DB files. See SyncDb-command response.");
  }

  return successful;
}


int PublisherClient::LocateEntry(const std::vector<ManifestEntry>& entries,
                                 const std::string& file_path) {
  int index = -1;
  for (unsigned int i = 0; i < entries.size(); ++i) {
    if (entries[i].orig_path == file_path)
      return i;
  }

  return index;
}

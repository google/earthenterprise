/*
 * Copyright 2017 Google Inc.
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

// Python extension library that is used by GEE Server Publish Manager for
// creating publish manifest files.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GEPUBLISH_GEPUBLISHMANAGERHELPER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GEPUBLISH_GEPUBLISHMANAGERHELPER_H_

#include <Python.h>  // Note: should be first in include list.

#include <cstddef>
#include <string>
#include <vector>

#include "common/ManifestEntry.h"

// PublishConfig - set of parameters used for building publish manifest
// structures.
struct PublishConfig {
  PublishConfig();
  PublishConfig(const std::string &_fusion_host,
                const std::string &_db_path,
                const std::string &_stream_url,
                const std::string &_end_snippet_proto,
                const std::string &_server_prefix);
  // Fusion hostname which database is registered with.
  std::string fusion_host;
  // Database path/name (asset root DB path, path to gedb/mapdb).
  std::string db_path;
  // Stream URL (server host + publishing target path).
  std::string stream_url;
  // Proto dbroot end snippet section in binary format.
  std::string end_snippet_proto;
  // Server prefix (published database directory prefix).
  std::string server_prefix;
};

// Helper processor for managing publish manifest files generating.
// Uses DbManifest processor for building publish manifest files.
// Usage:
// helper = libgepublishmanagerhelper.PublishManagerHelper(logger)
// publish_manifest = ManifestEntryVector()
// helper.GetPublishManifest(..., publish_manifest)
// for item in publish_manifest:
//   print item.current_path
//   print item.orig_path
//   print item.data_size
// helper.Reset()
class PublishManagerHelper {
 public:
  PublishManagerHelper();
  explicit PublishManagerHelper(PyObject *py_logger);
  ~PublishManagerHelper();

  // Creates publish manifest files (dbroot/layers, json, icons,
  // serverdb.config) and returns the list of manifest entries.
  // publish_config - publishing parameters.
  // manifest - list to put in publish manifest entries.
  void GetPublishManifest(
      const PublishConfig &publish_config,
      std::vector<ManifestEntry> *manifest);

  // Resets processor.
  void Reset();

  const char* TmpDir() const;

  // Returns all the log messages collected when processor does its job.
  const std::string& ErrMsg();

 private:
  // Builds publish manifest files.
  bool BuildPublishManifest(const PublishConfig &publish_config,
                            std::vector<ManifestEntry> *manifest);

  void AppendErrMsg(const std::string& msg);

  std::string err_msg_;
  std::string tmp_dir_;
  PyObject *py_logger_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GEPUBLISH_GEPUBLISHMANAGERHELPER_H_

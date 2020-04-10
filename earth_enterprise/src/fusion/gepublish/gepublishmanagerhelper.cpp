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

#include "fusion/gepublish/gepublishmanagerhelper.h"
#include "fusion/dbmanifest/dbmanifest.h"
#include "common/khFileUtils.h"
#include "common/khSpawn.h"
#include "common/khStringUtils.h"

// Error messages
const std::string kErrInternal = "Internal error.";
const std::string kErrTmpDir = kErrInternal +
    " Unable to create temp dir.";
const std::string kErrPublishManifest = kErrInternal +
    " Could not get publish manifest.";
const std::string kErrChmodFailure = kErrInternal +
    " Failed to chmod tmp files.";

namespace {

void LogToPyLogger(PyObject* _py_logger,
                   const std::string &_method, const std::string &_msg) {
  if (!_py_logger)
    return;

  std::string method = _method;
  std::string format = "s";
  std::string msg = _msg;
  char *pmethod = &method[0];
  char *pformat = &format[0];
  char *pmsg = &msg[0];
  PyObject_CallMethod(_py_logger, pmethod, pformat, pmsg);
}
}  // unnamed namespace.

PublishConfig::PublishConfig() {
}

PublishConfig::PublishConfig(const std::string &_fusion_host,
                             const std::string &_db_path,
                             const std::string &_stream_url,
                             const std::string &_end_snippet_proto,
                             const std::string &_server_prefix):
    fusion_host(_fusion_host),
    db_path(_db_path),
    stream_url(_stream_url),
    end_snippet_proto(_end_snippet_proto),
    server_prefix(_server_prefix) {
}


PublishManagerHelper::PublishManagerHelper()
  : py_logger_(NULL)  {
}

PublishManagerHelper::PublishManagerHelper(PyObject *py_logger)
    : py_logger_(py_logger) {
}

PublishManagerHelper::~PublishManagerHelper() {
  Reset();
}

// TODO: log to python logger instead of collecting error messages?
void PublishManagerHelper::GetPublishManifest(
    const PublishConfig &publish_config,
    std::vector<ManifestEntry> *manifest) {
  assert(manifest);
  Reset();

  if (!BuildPublishManifest(publish_config, manifest)) {
    manifest->clear();
    // Gettting publish manifest failed.
    std::string err_msg = ErrMsg() +
        "\nCreating publish manifest failed.\n";
    LogToPyLogger(py_logger_, "error", err_msg);
  }
}

void PublishManagerHelper::Reset() {
  if (!tmp_dir_.empty()) {
    khPruneDir(tmp_dir_);
    tmp_dir_.clear();
  }

  err_msg_.clear();
}

const char* PublishManagerHelper::TmpDir() const {
  return tmp_dir_.c_str();
}

const std::string& PublishManagerHelper::ErrMsg() {
  if (err_msg_.empty())
    err_msg_ = kErrInternal;
  return err_msg_;
}

bool PublishManagerHelper::BuildPublishManifest(
    const PublishConfig &publish_config,
    std::vector<ManifestEntry> *stream_manifest) {
  assert(stream_manifest);
  try {
    // {gedb/,mapdb/} path is  {server_prefix}/{fusion_host}{db_path}.
    std::string publish_db_path =
        publish_config.server_prefix + "/" +
        publish_config.fusion_host +
        publish_config.db_path;

    // Get the dbroots/layerdefs and server config.
    // will throw an exception
    DbManifest db_manifest(&publish_db_path);

    // Create temporary directory for the DB manifest.
    assert(tmp_dir_.empty());
    tmp_dir_ = khCreateTmpDir("gepublish.");
    if (tmp_dir_.empty()) {
      AppendErrMsg(kErrTmpDir);
      return false;
    }

    // TODO: get fusion_host and server prefix in db_manifest.
    // We can get them from publish_db_path.
    if (!db_manifest.GetPublishManifest(
            publish_config.stream_url,
            publish_config.end_snippet_proto,
            publish_config.server_prefix + "/" + publish_config.fusion_host,
            tmp_dir_,
            *stream_manifest)) {
      AppendErrMsg(kErrPublishManifest);
      return false;
    }

    // chmod all the temporary files to 0755 so that the publisher servlet
    // can access it just in case the user's umask disables access to all
    // completely.
    CmdLine cmdline;
    cmdline << "chmod" << "-R" << "755" << tmp_dir_;
    if (!cmdline.System()) {
      AppendErrMsg(kErrChmodFailure);
      return false;
    }

    return true;
  } catch(const std::exception &e) {
    // Give a reasonable message, if we don't get a message,
    // direct them to a logfile if one exists.
    std::string error_message = e.what() ? e.what() : "";

    // Construct a reasonable message if we don't have one already.
    if (error_message.empty()) {
      error_message = "Unknown error.";
    }
    AppendErrMsg(error_message.c_str());
  }
  return false;
}

void PublishManagerHelper::AppendErrMsg(const std::string& msg) {
  if (err_msg_.empty())
    err_msg_ = msg;
  else
    err_msg_ += "\n" + msg;
}

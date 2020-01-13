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

#include "common/LoggingManager.h"

#include "common/khFileUtils.h"
#include "common/notify.h"

namespace {
// Local handler for "notify".
// This handler allows us to override the system notify function for use
// by the LoggingManager's internal notify handler.
void LoggingManagerNotifyHandler(void* logging_manager_void,
                                 khNotifyLevel severity,
                                 const char* fmt,
                                 va_list ap) {
  LoggingManager* logging_manager =
      static_cast<LoggingManager*>(logging_manager_void);
  if (logging_manager != NULL)
    logging_manager->NotifyHandler(static_cast<int>(severity), fmt, ap);
}
}  // unnamed namespace

// Note: currently, LoggingManager is used only in
// PublisherClient(PublishHelper). In current implementation, it can be used
// in processors separated in time in their executing.
//
// TODO:
// - ensure that class has only one instance with specified file_prefix.
// - support set of handlers for notify system.

LoggingManager::LoggingManager(std::string dirname,
                               std::string file_prefix,
                               int log_files_to_keep,
                               std::string log_files_to_keep_env_variable) {
  // get a logfilename
  logfilename_ = GetLogFileNameAndCleanupLogs(dirname,
                                              file_prefix,
                                              log_files_to_keep,
                                              log_files_to_keep_env_variable);

  // Open the logfile.
  logfile_ = NULL;
  if (!logfilename_.empty()) {
    logfile_ = ::fopen(logfilename_.c_str(), "a");
    if (logfile_ == NULL) {
      std::string error_message = std::string("Unable to open log file ") +
                                  logfilename_ + ": " + khstrerror(errno);
      AppendErrMsg(error_message);
    } else {
      // Override notify for now to make sure we log to the logfile
      // in addition to stderr.
      setNotifyHandler(LoggingManagerNotifyHandler, this);
    }
  }
}

LoggingManager::~LoggingManager() {
  // Clean up the notify handler override.
  setNotifyHandler(NULL, NULL);
  if (logfile_) {
    ::fclose(logfile_);
  }
}

void LoggingManager::AppendErrMsg(const std::string& msg) {
  // Make a notification, so this will be logged to the log file.
  notify(NFY_WARN, "%s", msg.c_str());
  if (err_msg_.empty())
    err_msg_ = msg;
  else
    err_msg_ += "\n" + msg;
}


const std::string& LoggingManager::ErrMsg() {
  if (err_msg_.empty())
    err_msg_ = "Internal error.";
  return err_msg_;
}


// Notify handler will send all DEBUG and worse messages to the log,
// and other messages will go to stderr as determined by the calling process
// (i.e., using the current notify settings so as other process behavior is not
// affected).
void LoggingManager::NotifyHandler(
    int severity_int, const char* fmt, va_list ap) {
  khNotifyLevel severity = static_cast<khNotifyLevel>(severity_int);

  // Can't use "ap" twice so we copy before using it.
  // Note: this is tricky because some compilers allow "reuse" so we
  // were getting away with it.
  va_list ap_copy;
  va_copy(ap_copy, ap);

  // Pass 1: process the notify using our format.
  if (severity <= NFY_DEBUG) {
    NotifyPrefix = "[time]";
    std::string prefix = GetNotifyPrefixValue(severity, TimePrefix);
    fprintf(logfile_, "%s", prefix.c_str());
    vfprintf(logfile_, fmt, ap);
    fprintf(logfile_, "\n");
    fflush(logfile_);
  }

  // Pass 2: process using notify defaults to stderr.
  if (severity <= getNotifyLevel()) {
    std::string prefix = GetNotifyPrefixValue(severity, NotifyPrefix);
    fprintf(stderr, "%s", prefix.c_str());
    vfprintf(stderr, fmt, ap_copy);
    fprintf(stderr, "\n");
  }

  // Handle FATAL messages by exiting (this is the standard behavior of notify).
  if (severity == NFY_FATAL)
    exit(-1);

  va_end(ap);
}

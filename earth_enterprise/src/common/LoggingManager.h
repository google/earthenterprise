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

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_LOGGINGMANAGER_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_LOGGINGMANAGER_H_

#include <stdio.h>
#include <stdarg.h>
#include <string>

// LoggingManager allows to collect all the error messages internally in
// string container, overrides system notify handler with internal one for
// logging error messages into log file and manages log files.
class LoggingManager {
 public:
  // Constructor will create a filename of the form:
  // dirname/file_prefix.YYYY-MM-DD-HH:MM:SS.log and override system notify
  // handler with internal one.
  // It will also clean up older logfiles than the N most recent (including
  // this new one) where N is specified by log_files_to_keep which can be
  // overriden by the log_files_to_keep_env_variable.
  // If dirname is empty, it will attempt to put logfile in the following
  // directories: ~/.fusion, TMPDIR, /tmp
  // if no directory is specified or a valid directory found, or if
  // log_files_to_keep <= 0, then logfile is not created.
  // Also, notify handler is overridden.
  LoggingManager(std::string dirname,
                 std::string file_prefix,
                 int log_files_to_keep,
                 std::string log_files_to_keep_env_variable);
  ~LoggingManager();

  // Notify handler function to override default one.
  // Notify handler will send all DEBUG and worse messages to the log file,
  // and other messages will go to stderr as determined by the calling
  // process (i.e., using the current notify settings so as other process
  // behavior is not affected).
  void NotifyHandler(int severity, const char* fmt, va_list ap);

  // Appends error message to internal string container and logfile using
  // notify()-call.
  void AppendErrMsg(const std::string& msg);

  // Returns collected error messages.
  const std::string& ErrMsg();

  // Returns log-filename whether logfile has been created, otherwise it
  // returns empty string.
  const std::string& LogFilename() const {
    return logfilename_;
  }

 private:
  // Error messages string container.
  std::string err_msg_;

  // Logfile information.
  std::string logfilename_;
  FILE* logfile_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_LOGGINGMANAGER_H_

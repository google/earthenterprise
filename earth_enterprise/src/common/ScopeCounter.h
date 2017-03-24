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

// khBackTrace Implementation

#ifndef COMMON_SCOPECOUNTER__H__
#define COMMON_SCOPECOUNTER__H__

// Fake dependency for forcing a rebuild based on the build command switch.
#include <enableInternalUseFlag.h>
// This class is only to be used on internal builds using the internal=1
// flag in scons.
#ifdef GEE_INTERNAL_USE_ONLY

#include <string>
#include <vector>
#include <map>
#include <khThread.h>

// ScopeCounter is a utility method for globally tracking the currently active
// Methods. This is written to be thread safe.
// example usage:
// void f() {
//   SCOPE_COUNTER;
//   ...
// }
// It will track the function and thread scope, and in some other function
// You can call:
// ScopeCounter::DumpOpenScopeCounts();
// to get a print out to stderr of the open methods tracked using SCOPE_COUNTER.
#define SCOPE_COUNTER ScopeCounter scope_counter(__FUNCTION__);
// Use SCOPE_COUNTER_MANUAL to require a manual "Close()" of the ScopeCounter
// rather than for the count to close on destruction.
#define SCOPE_COUNTER_MANUAL ScopeCounter scope_counter(__FUNCTION__, false);
class ScopeCounter {
public:
  ScopeCounter(const char* function_name,
               bool auto_close = true,
               bool print_each_start_stop = false)
  : auto_close_(auto_close), print_each_start_stop_(print_each_start_stop) {
    function_name_ = Identifier(function_name);
    scope_map_.Start(function_name_, print_each_start_stop_);
  }
  ~ScopeCounter() {
    if (auto_close_) {
      scope_map_.End(function_name_, print_each_start_stop_);
    }
  }

  // Add a message to be displayed at the end with this function scope if
  // it is active during the DumpOpenScope call.
  void AddMessage(std::string message) {
    scope_map_.AddMessage(function_name_, message);
  }
  void Close() {
    scope_map_.End(function_name_, print_each_start_stop_);
  }
  // Print out to stderr of the open methods tracked using ScopeCounter.
  static void DumpOpenScope() { scope_map_.DumpOpenScope(); }

private:
  // Create a string identifier that is unique to the function and thread.
  std::string Identifier(const char* function_name) const;

  std::string function_name_;
  bool auto_close_; // Specifies whether the End() call is made on destruction.
  // Statically track the scope counts.
  bool print_each_start_stop_; // Specifies whether we print each IN/OUT of the
                               // scope.
  class ScopeMap {
  public:
    ScopeMap() {}
    ~ScopeMap() {}

    // Increment the scope count.
    void Start(std::string& function_name, bool print_each_start_stop);
    // Decrement the scope count
    void End(std::string& function_name, bool print_each_start_stop);

    // Add a message for teh specified function_name. Will be printed out in
    // DumpOpenScope if the function_name scope is still open.
    void AddMessage(std::string& function_name, std::string& message);

    // Print out to stderr of the open methods tracked using ScopeCounter.
    void DumpOpenScope();

    std::map<std::string, int> scope_map_;
    std::map<std::string, std::string> scope_message_map_;
    khMutex mutex_;
  };
  static ScopeMap scope_map_;
};

#endif

#endif  // COMMON_SCOPECOUNTER__H__



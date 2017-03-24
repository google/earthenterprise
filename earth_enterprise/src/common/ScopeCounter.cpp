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

// khBackTrace Implementation

// Fake dependency for forcing a rebuild based on the build command switch.
#include <enableInternalUseFlag.h>
// This class is only to be used on internal builds using the internal=1
// flag in scons.
#ifdef GEE_INTERNAL_USE_ONLY

#include "ScopeCounter.h"
#include "khStringUtils.h"
#include <iostream>
#include <pthread.h>

ScopeCounter::ScopeMap ScopeCounter::scope_map_;

// Create a string identifier that is unique to the function and thread.
std::string ScopeCounter::Identifier(const char* function_name) const {
   pthread_t thread_id = pthread_self();

  std::string identifier = std::string(function_name) + ":" + Itoa(thread_id);
  return identifier;
}

void ScopeCounter::ScopeMap::Start(std::string& function_name,
                                 bool print_each_start_stop) {
  khLockGuard lock(mutex_);
  if (print_each_start_stop) {
    std::cerr << "IN: " << function_name << std::endl;
  }
  scope_map_[function_name]++;
}

void ScopeCounter::ScopeMap::End(std::string& function_name,
                                 bool print_each_start_stop) {
  khLockGuard lock(mutex_);
  if (print_each_start_stop) {
    std::cerr << "OUT: " << function_name << std::endl;
  }
  scope_map_[function_name]--;
}

void ScopeCounter::ScopeMap::AddMessage(std::string& function_name, std::string& message) {
  khLockGuard lock(mutex_);
  scope_message_map_[function_name] = message;
}


void ScopeCounter::ScopeMap::DumpOpenScope() {
  khLockGuard lock(mutex_);
  std::map<std::string, int>::const_iterator iter = scope_map_.begin();
  std::cerr << "The following methods are in progress:" << std::endl;
  for(; iter != scope_map_.end(); ++iter) {
    if (iter->second > 0) {
      std::cerr << iter->first << " : " << iter->second <<
        " : " << scope_message_map_[iter->first] << std::endl;
    }
  }
}

#endif

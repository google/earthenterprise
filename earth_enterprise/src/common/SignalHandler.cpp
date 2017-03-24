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

// SignalHandler utility functions
//

#include "common/SignalHandler.h"
// for memset
#include <string.h>

#include <iostream>  // NOLINT(readability/streams)

// Fake dependency for forcing a rebuild based on the build command switch.
#include <enableInternalUseFlag.h>  // NOLINT(build/include_order)
// This class is only to be used on internal builds using the internal=1
// flag in scons.
#ifdef GEE_INTERNAL_USE_ONLY

#include "common/khBackTrace.h"
#include "common/ScopeCounter.h"


int create_sigaction(int signal_id, signal_handler_func_type handler) {
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_sigaction = handler;
  sigfillset(&action.sa_mask);
  action.sa_flags = SA_SIGINFO;
  return sigaction(signal_id, &action, NULL);
}

// Basic handler, will print out basics and a stack trace.
void signal_handler(int signal_id, siginfo_t* info, void* context) {
  std::cerr << "Signal " << signal_id << " ";
  switch (signal_id) {
  case SIGSEGV:
    std::cerr  << "SIGSEGV";
    break;
  case SIGILL:
    std::cerr  << "SIGILL";
    break;
  case SIGABRT:
    std::cerr  << "SIGABRT";
    break;
  case SIGFPE:
    std::cerr  << "SIGFPE";
    break;
  case SIGBUS:
    std::cerr  << "SIGBUS";
    break;
  }
  std::cerr  << std::endl;
  std::cerr << "  error: " << info->si_errno << std::endl;
  std::cerr << "  code: " << info->si_code << std::endl;
  std::cerr << "  address: " << info->si_addr << std::endl;
  ScopeCounter::DumpOpenScope();
  khBackTrace();
  exit(-1);
}

// add signal handlers
void AddSignalHandlers() {
  create_sigaction(SIGSEGV, signal_handler);
  create_sigaction(SIGILL, signal_handler);
  create_sigaction(SIGABRT, signal_handler);
  create_sigaction(SIGFPE, signal_handler);
  create_sigaction(SIGBUS, signal_handler);
}

#endif

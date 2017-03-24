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

// SignalHandler utility functions

#ifndef COMMON_SIGNALHANDLER__H__
#define COMMON_SIGNALHANDLER__H__


// Fake dependency for forcing a rebuild based on the build command switch.
#include <enableInternalUseFlag.h>
// This class is only to be used on internal builds using the internal=1
// flag in scons.
#ifdef GEE_INTERNAL_USE_ONLY

#include <signal.h>

// Typedef for signal handler functions.
typedef void signal_handler_func_type(int signal_id,
                                      siginfo_t* info, void* context);

// Create a signal handler for the specified signal.
int create_sigaction(int signal_id, signal_handler_func_type handler);

// Adds basic signal handler for critical signals.
// When caught, will print summary and backtrace and scope counts if any.
void AddSignalHandlers();

#endif
#endif  // COMMON_SIGNALHANDLER__H__



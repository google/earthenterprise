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

#ifndef COMMON_KHBACKTRACE__H__
#define COMMON_KHBACKTRACE__H__

// Fake dependency for forcing a rebuild based on the build command switch.
#include <enableInternalUseFlag.h>
// This class is only to be used on internal builds using the internal=1
// flag in scons.
#ifdef GEE_INTERNAL_USE_ONLY

// Print a backtrace of the current stack.
// Useful for debugging runtime errors.
void khBackTrace();

#define TRACE_MESSAGE(message) \
  fprintf(stderr, "%s: %s : %d : %s\n", \
          __FUNCTION__ , __FILE__, __LINE__, message)

#endif
#endif  // COMMON_KHBACKTRACE__H__


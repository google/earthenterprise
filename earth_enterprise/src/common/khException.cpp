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


#include "khException.h"
#include <stdlib.h>

#if (__GNUC__ == 2)
static void
myTerminateHandler(void)
{
  notify(NFY_WARN, "Terminating with unknown, uncaught exception");
  abort();
}
#else
static void
myTerminateHandler(void)
    try
{
  throw;
} catch (const std::exception &e) {
  notify(NFY_WARN, "Terminating with uncaught exception: %s", e.what());
  abort();
} catch (...) {
  notify(NFY_WARN, "Terminating with unknown, uncaught exception");
  abort();
}
#endif

static void
myUnexpectedHandler(void)
    try
{
  throw;
} catch (const std::exception &e) {
  notify(NFY_WARN, "Terminating with unexpected exception: %s", e.what());
  abort();
} catch (...) {
  notify(NFY_WARN, "Terminating with unknown, unexpected exception");
  abort();
}


void
ReportBadExceptions(void) {
  std::set_terminate(myTerminateHandler);
  std::set_unexpected(myUnexpectedHandler);
}

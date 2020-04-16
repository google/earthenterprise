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



#include "geCapabilities.h"
#include <notify.h>
#include <khException.h>
#include <khGuard.h>

geCapabilitiesGuard::geCapabilitiesGuard(cap_value_t cap1,
                                         cap_value_t cap2,
                                         cap_value_t cap3,
                                         cap_value_t cap4) {
  caplist_.push_back(cap1);
  if (cap2 != -1) caplist_.push_back(cap2);
  if (cap3 != -1) caplist_.push_back(cap3);
  if (cap4 != -1) caplist_.push_back(cap4);
  AdjustCapabilities(CAP_SET);
}

geCapabilitiesGuard::~geCapabilitiesGuard(void) {
  try {
    AdjustCapabilities(CAP_CLEAR);
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
  }
}

void geCapabilitiesGuard::AdjustCapabilities(cap_flag_value_t action) {
  cap_t cap = cap_get_proc();
  if (!cap) {
    throw khErrnoException(kh::tr("Unable to get process capabilities"));
  }
  typedef void (*CompatibleFuncType)(cap_t);
  khCallGuard<cap_t> free_cap_guard(CompatibleFuncType(cap_free), cap);

  for (unsigned int i = 0; i < caplist_.size(); ++i) {
    if (cap_set_flag(cap, CAP_EFFECTIVE, 1, &caplist_[i], action) == -1) {
      throw khErrnoException(kh::tr("Unable to set capability value (%1)")
                             .arg(caplist_[i]));
    }
  }

  if (cap_set_proc(cap) == -1) {
    throw khErrnoException(kh::tr("Unable to set process capabilities"));
  }

#if 0
  char *dump = cap_to_text(cap, 0);
  fprintf(stderr, "%s\n", dump);
  cap_free(dump);
#endif

}

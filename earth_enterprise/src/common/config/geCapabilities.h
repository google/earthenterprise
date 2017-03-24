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

//

#ifndef COMMON_GECONFIGURE_GECAPABILITIES_H__
#define COMMON_GECONFIGURE_GECAPABILITIES_H__

#include <sys/capability.h>
#include <vector>

// ****************************************************************************
// ***  Used by a program that has more permitted than effective permissions in
// ***  order to increase its effective permissions.
// ***
// ***  For example: a configure script that was started as root but later
// ***  called geSwitchUser(kGEFusionUsername), may want to add back in certain
// ***  capabilities (like chmod'ing files not owned by fusion user) while
// ***  still running as the fusion user.
// ***
// ***  Capabilities are maintained until object is destroyed
// ***
// ***  A list of CAP_ values can be found in the capabilities(7) manpage.
// ***
// ***  usage:
// ***    {
// ***      geCapabilitiesGuard cap_guard(
// ***        CAP_CHOWN,         // let me chown files
// ***        CAP_FOWNER         // let me chmod files I dont own
// ***        );
// ***
// ***      chown(...);
// ***      chmod(...);
// ***    }
// ****************************************************************************


class geCapabilitiesGuard {
public:
  // will throw if unable to acquire capabilities
  geCapabilitiesGuard(cap_value_t cap1,      cap_value_t cap2 = -1,
                      cap_value_t cap3 = -1, cap_value_t cap4 = -1);
  ~geCapabilitiesGuard(void);

private:
  void AdjustCapabilities(cap_flag_value_t action);

  std::vector<cap_value_t> caplist_;
};


#endif // COMMON_GECONFIGURE_GECAPABILITIES_H__

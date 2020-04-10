/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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

#ifndef COMMON_CONFIG_GECONFUTIL_H__
#define COMMON_CONFIG_GECONFUTIL_H__

#include <string>
#include <common/khTypes.h>

class geUserId;

// will throw
void AssertRunningAsRoot(void);
std::string GetAndValidateHostname(void);

// will throw
// uid == 0 -> let system pick uid/gid
void AddGEGroup(const std::string &groupname, int gid = 0);
void AddGEUser(const std::string &username, int uid = 0);

// For Server-only installs, there is no systemrc file, instead usernames
// are stored in gevars.sh.
// These utilities will retrieve these names from gevars.sh or throw an
// exception.
std::string ApacheUserNameServerOnly();
std::string FusionUserNameServerOnly();
std::string UserGroupnameServerOnly();

unsigned int GetNumCPUs(void);

// Return the physical memory size in bytes.
// Return 0 if the value can't be safely found.
 std::uint64_t GetPhysicalMemorySize(void);

// Return the Max number of open file descriptors.
// if requested is <= 0 -> max(1, RLIMIT_NOFILE - requested)
// if requested is positive -> min(RLIMIT_NOFILE, requested),
unsigned int GetMaxFds(int requested);

bool IsRedHat(void);


#endif // COMMON_CONFIG_GECONFUTIL_H__

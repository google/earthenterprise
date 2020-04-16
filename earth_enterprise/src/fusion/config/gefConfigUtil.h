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



#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_CONFIG_GEFCONFIGUTIL_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_CONFIG_GEFCONFIGUTIL_H_


#include <string>
//#include "common/khTypes.h"
#include <cstdint>

static const std::uint32_t kMaxNumJobsLimit = 128;
static const std::uint32_t kMaxNumJobsLimit_2 = kMaxNumJobsLimit / 2;

class Systemrc;
class VolumeDefList;

// Note: should be called by all config tools.
// Makes sure we're running as root,
// makes sure fusion isn't running,
// makes sure we have a valid hostname,
// switches to fusion user,
// returns valid hostname or throws if any of the above fail.
std::string ValidateHostReadyForConfig(void);


// Fusion now assumes systemrc exists in the proper location which is now
// created during installation.
// An exception is thrown if the file does not exist.
void LoadSystemrc(Systemrc &systemrc, bool override_cache = false);
// Command-line defaults for asset root and maxjobs.
std::string CommandlineAssetRootDefault(void);
 unsigned int CommandlineNumCPUsDefault(void);

void LoadVolumesOrThrow(const std::string &assetroot, VolumeDefList &volumes);
void SaveVolumesOrThrow(const std::string &assetroot,
                        const VolumeDefList &volumes);

// This is checked automatically by ValidateHostReadyForConfig.
bool IsFusionRunning(void);

void SwitchToUser(const std::string username,
                  const std::string group_name);

// Returns max number of concurrent jobs which defaults to min of the values:
// number of CPUs or maximum allowable number of concurrent jobs.
 std::uint32_t GetMaxNumJobs();

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_CONFIG_GEFCONFIGUTIL_H_

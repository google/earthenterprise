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



#include "fusion/config/gefConfigUtil.h"

#include <cstdlib>
#include <algorithm>

#include "fusion/autoingest/.idl/Systemrc.h"
#include "fusion/autoingest/.idl/VolumeStorage.h"
#include "fusion/autoingest/geAssetRoot.h"
#include "common/khSpawn.h"
#include "common/khException.h"
#include "common/geUsers.h"
#include "common/khFileUtils.h"
#include "common/config/geConfigUtil.h"
#include "common/config/geCapabilities.h"

namespace {
const uint32 kMaxNumJobsDefault = 8;

uint32 GetDefaultMaxNumJobs() {
  char* variable;
  uint32 max_num_jobs = kMaxNumJobsDefault;
  if ((variable = getenv("KH_GOOGLE_MAX_NUM_JOBS")) != NULL) {
    char *endptr = NULL;
    const uint32 value = static_cast<uint32>(
        std::strtoul(variable, &endptr, 0));
    if (endptr != variable) {
      max_num_jobs = value;
    }
  }
  return max_num_jobs;
}

}  // namespace

// ****************************************************************************
// ***  ValidateHostReadyForConfig
// ****************************************************************************
bool IsFusionRunning(void) {
  return (geCheckPidFile("gesystemmanager") ||
          geCheckPidFile("geresourceprovider"));
}

namespace {
void AssertFusionNotRunning(void) {
  if (IsFusionRunning()) {
    throw khException(kh::tr("Please stop fusion before proceeding.\n"
                             "(e.g. /etc/init.d/gefusion stop)"));
  }
}
}

std::string ValidateHostReadyForConfig(void) {
  AssertRunningAsRoot();
  AssertFusionNotRunning();
  return GetAndValidateHostname();
}

void LoadSystemrc(Systemrc &systemrc) {
  static Systemrc cached_systemrc;
  static bool     use_cached = false;

  if (use_cached) {
    systemrc = cached_systemrc;
    return;
  }

  if (khExists(Systemrc::Filename())) {
    use_cached = true;
    // load into a tmp to avoid a partial load on an earlier file
    // affecting the defaults for this load
    Systemrc tmp;
    if (tmp.Load()) {
      uint32 max_num_jobs = GetMaxNumJobs();

      // If uninitialized or greater than maximum allowable number of
      // concurrent jobs, the maxjobs defaults to the min of the values:
      // maximum allowable number of concurrent jobs or limit on max number
      // of jobs.
      if (tmp.maxjobs == 0 || tmp.maxjobs > max_num_jobs) {
        tmp.maxjobs = std::min(max_num_jobs, kMaxNumJobsLimit);
      }
      systemrc = cached_systemrc = tmp;
    }
  } else {
    throw khException(kh::tr("'%1' is missing").arg(Systemrc::Filename()));
  }
}

std::string CommandlineAssetRootDefault(void) {
  Systemrc systemrc;
  LoadSystemrc(systemrc);
  return systemrc.assetroot;
}

uint32 CommandlineNumCPUsDefault(void) {
  Systemrc systemrc;
  LoadSystemrc(systemrc);
  return systemrc.maxjobs;
}

// ****************************************************************************
// ***  Volume routines
// ****************************************************************************
void LoadVolumesOrThrow(const std::string &assetroot, VolumeDefList &volumes) {
  std::string volumefname =
    geAssetRoot::Filename(assetroot, geAssetRoot::VolumeFile);
  if (!khExists(volumefname) || !volumes.Load(volumefname)) {
    throw khException(kh::tr("Unable to load volumes for %1")
                      .arg(assetroot));
  }
}

void SaveVolumesOrThrow(const std::string &assetroot,
                        const VolumeDefList &volumes) {
  std::string volumefname =
    geAssetRoot::Filename(assetroot, geAssetRoot::VolumeFile);
  if (!volumes.Save(volumefname)) {
    throw khException(kh::tr("Unable to save volumes for %1")
                      .arg(assetroot));
  }
  (void)khChmod(volumefname, geAssetRoot::FilePerms(geAssetRoot::VolumeFile));
}

void SwitchToUser(const std::string username,
                  const std::string group_name) {
  geUserId ge_user(username, group_name);
  ge_user.SwitchEffectiveToThis();
}

uint32 GetMaxNumJobs() {
  uint32 max_num_jobs = 0;

  // Get maximum allowable number of concurrent jobs.
  // Note: KH_MAX_NUM_JOBS_COEFF can be used to build GEE Fusion licensing
  // KH_MAX_NUM_JOBS_COEFF*kMaxNumJobsDefault (8/16/24..) concurrent jobs.
#ifdef KH_MAX_NUM_JOBS_COEFF
  max_num_jobs = kMaxNumJobsDefault *
      static_cast<uint32>(KH_MAX_NUM_JOBS_COEFF);
#endif

  // Note: Apply an internal multiplier in case of GEE Fusion is built
  // with maximum number of concurrent jobs equals 0 (internal usage).
  if (max_num_jobs == 0) {
    max_num_jobs = GetDefaultMaxNumJobs();
  }

  // Set the max_num_jobs to the min of the values: number of CPUs or max
  // allowable number of jobs.
  max_num_jobs = std::min(max_num_jobs, GetNumCPUs());

  return max_num_jobs;
}

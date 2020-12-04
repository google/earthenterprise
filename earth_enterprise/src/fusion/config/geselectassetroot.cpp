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



#include <stdio.h>
#include <algorithm>

#include "fusion/config/gefConfigUtil.h"
#include "fusion/config/AssetRootStatus.h"
#include "fusion/autoingest/.idl/Systemrc.h"
#include "fusion/autoingest/.idl/VolumeStorage.h"
#include "fusion/autoingest/geAssetRoot.h"
#include "common/notify.h"
#include "common/khGetopt.h"
#include "common/khFileUtils.h"
#include "common/config/gePrompt.h"
#include "common/config/geCapabilities.h"
#include "common/khException.h"

namespace {

void usage(const char* prog, const char* msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(
      stderr,
      "\n"
      "usage: %s --lock\n"
      "       %s --unlock\n"
      "       %s --assetroot=<dir> [options]\n"
      "Select an asset root for fusion to work with.\n"
      "Supported options are:\n"
      "  --help, -?:              Displays this help message\n"
      "  --lock:                  Disable selecting asset roots for this machine\n"
      "  --unlock:                Enable selecting asset roots for this machine\n"
      "  --assetroot <dir>:       Path to asset root\n"
      "  --noprompt:              do not prompt for more information, returns -1\n"
      "                           to indicate an error if command fails or has insufficient arguments\n"
      "\n"
      "Additional options when --assetroot specified:\n"
      "  --role {master|slave}: This machine's role in asset root [default: master]\n"
      "  --numcpus <number>:    Number of CPUs to use [default: %u]\n",
      prog, prog, prog,
      CommandlineNumCPUsDefault());

  exit(1);
}

enum Role { RoleMaster, RoleSlave };

void SaveSystemrcOrThrow(const Systemrc &rc) {
  geCapabilitiesGuard cap_guard(
      CAP_DAC_OVERRIDE,     // let me read/write all files
      CAP_DAC_READ_SEARCH,  // let me traverse all dirs
      CAP_CHOWN,            // let me chown files
      CAP_FOWNER);          // let me chmod files I dont own
  if (!rc.Save()) {
    throw khException(
        kh::tr("Unable to save systemrc"));
  }
}

// ********************************************
void SetLocked(bool locked) {
  Systemrc systemrc;
  LoadSystemrc(systemrc);
  systemrc.locked = locked;
  SaveSystemrcOrThrow(systemrc);
}

void SetAssetRootMaster(const std::string &assetroot,
                        const std::string &host) {
  VolumeDefList volumes;
  LoadVolumesOrThrow(assetroot, volumes);
  volumes.volumedefs[geAssetRoot::VolumeName].host = host;
  SaveVolumesOrThrow(assetroot, volumes);
}

void ValidateForSlave(const AssetRootStatus &status) {
  if (status.IsThisMachineMaster()) {
    throw khException(kh::tr(
"This machine is currently the master for %1.\n"
"You cannot change it's role to slave.\n"
"If you want a different machine to be the master, run the following\n"
"on that machine:\n"
"  geselectassetroot --assetroot %2 --role master")
                      .arg(status.assetroot_.c_str())
                      .arg(status.assetroot_.c_str()));
  }

  // we don't care about any other asset root status here
  // even if it's broken, it's not our job as a slave to try to fix it
}

void ValidateForMaster(const AssetRootStatus &status) {
  if (!status.IsThisMachineMaster() &&
      khExists(geAssetRoot::Filename(status.assetroot_,
                                     geAssetRoot::ActiveFile))) {
    throw khException(kh::tr(
"%1 is currently running as the master for %2.\n"
"You need to shut it down before setting this machine to be the master.")
                      .arg(status.master_host_.c_str())
                      .arg(status.assetroot_.c_str()));
  }

  // Now see if the rest of the status checks are OK
  // technically we could ignore everything except for ownership
  // problems since we don't care about them in order to do our job of
  // switching. But doing this he're lets them now _before_ they switch
  // that there is a problem.
  if (status.AssetRootNeedsUpgrade()) {
    status.ThrowAssetRootNeedsUpgrade();
  } else if (status.SoftwareNeedsUpgrade()) {
    status.ThrowSoftwareNeedsUpgrade();
  }
  if (status.AssetRootNeedsRepair()) {
    status.ThrowAssetRootNeedsRepair();
  }
}

void SelectAssetRoot(const AssetRootStatus &status,
                     const std::string &username,
                     const std::string &groupname,
                     Role role,
                     std::uint32_t numcpus,
                     bool noprompt) {
  // This will throw an exception if the systemrc file does not exist.
  Systemrc systemrc;
  LoadSystemrc(systemrc);

  // make sure we're allowed to switch the asset root
  if (systemrc.locked) {
    throw khException(kh::tr(
"This machine is not allowed to switch its asset root.\n"
"To reenable asset root switching, have your admin run the following comand:\n"
"  geselectassetroot --unlock"));
  }


  // validate the assetroot status - just the basics.
  // we don't care if it's out of data, etc at this point since we may not be
  // allowed to switch to it anyway
  {
    QString missing_msg = kh::tr(
"%1 doesn't exist.\n");
    if (!status.dir_exists_) {
      throw khException(missing_msg.arg(status.assetroot_.c_str()));
    }
    QString invalid_msg = kh::tr(
"%1 isn't a valid assetroot.\n").arg(status.assetroot_.c_str());
    if (!status.dir_exists_ ||
        !status.has_volumes_) {
      throw khException(invalid_msg);
    }
  }


  // do some error checking and confirmation with the user
  // before proceeding
  if (role == RoleSlave) {
    ValidateForSlave(status);

  } else {
    ValidateForMaster(status);

    if (!status.IsThisMachineMaster()) {
      QString msg = kh::tr(
"%1 is currently defined as the master for %2.\n"
"Do you want to switch the master to be %3")
                    .arg(status.master_host_.c_str())
                    .arg(status.assetroot_.c_str())
                    .arg(status.thishost_.c_str());
      if (noprompt) {
        msg = kh::tr(
"%1 is currently defined as the master for %2.\n"
"Cannot configure %3 machine as the master")
                    .arg(status.master_host_.c_str())
                    .arg(status.assetroot_.c_str())
                    .arg(status.thishost_.c_str());
        throw khException(msg);
      }
      if (!geprompt::confirm(msg, 'N')) {
        throw khException(kh::tr("Aborted by user"));
      }

      SetAssetRootMaster(status.assetroot_, status.thishost_);
    }
  }

  systemrc.assetroot      = status.assetroot_;
  systemrc.fusionUsername = username;
  systemrc.userGroupname  = groupname;
  systemrc.maxjobs        = numcpus;
  systemrc.locked         = false;
  SaveSystemrcOrThrow(systemrc);
}

}  // namespace


// ********************************************
int main(int argc, char *argv[]) {
  try {
    khGetopt options;
    bool help    = false;
    bool lock    = false;
    bool unlock  = false;
    std::string assetroot;
    std::string username = Systemrc::FusionUsername();
    std::string groupname = Systemrc::UserGroupname();
    Role role    = RoleMaster;
    std::uint32_t numcpus = CommandlineNumCPUsDefault();
    int argn;
    bool noprompt = false;

    options.helpOpt(help);
    options.opt("lock", lock);
    options.opt("unlock", unlock);
    options.opt("assetroot", assetroot);
    options.opt("noprompt", noprompt);
    options.choiceOpt("role", role,
                      makemap<std::string, Role>(
                          "master", RoleMaster,
                          "slave",  RoleSlave));
    options.opt("numcpus", numcpus,
                &khGetopt::RangeValidator<std::uint32_t, 1, kMaxNumJobsLimit>);

    options.setExclusiveRequired("lock", "unlock", "assetroot");

    if (!options.processAll(argc, argv, argn) || help) {
      usage(argv[0]);
    }

    numcpus = std::min(numcpus, GetMaxNumJobs());

    // Make sure I'm root, everything is shut down,
    // and switch to fusion user
    std::string thishost = ValidateHostReadyForConfig();
    SwitchToUser(username, groupname);

    if (lock) {
      SetLocked(true);
    } else if (unlock) {
      SetLocked(false);
    } else {
      AssetRootStatus status(assetroot, thishost, username, groupname);
      SelectAssetRoot(status, username, groupname, role, numcpus, noprompt);
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "\n%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}

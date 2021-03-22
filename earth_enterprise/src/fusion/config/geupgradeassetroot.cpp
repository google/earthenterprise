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
#include <notify.h>
#include <khGetopt.h>
#include <khFileUtils.h>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/sysman/.idl/FusionUniqueId.h>
#include <config/gefConfigUtil.h>
#include <config/AssetRootStatus.h>
#include <config/FixAssetRoot.h>
#include <autoingest/plugins/RasterProjectAsset.h>
#include <DottedVersion.h>
#include <autoingest/geAssetRoot.h>
#include <autoingest/AssetTraverser.h>
#include <fusionversion.h>
#include <khSpawn.h>
#include <autoingest/.idl/Systemrc.h>

// local function declarations
void ValidateAssetRootForUpgrade(const AssetRootStatus &status, bool noprompt,
                                 bool chown);
void WarnAboutPendingTasks(void);
void UpgradeAssetRoot(const DottedVersion &version, bool secure);
void UpgradeFrom2_4_X(void);
void UpgradeFrom2_5_X(const DottedVersion &version);
void UpgradeFrom3_0_X(const DottedVersion &version);

void
usage(const char* prog, const char* msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf
    (stderr,
     "\n"
     "usage: %s [options]\n"
     "Upgrade an existing asset root\n"
     "Supported options are:\n"
     "  --help, -?:          Displays this help message\n"
     "  --assetroot <dir>:   Path to asset root [default: %s]\n"
     "  --username <name>:   Fusion username (default gefusionuser)\n"
     "  --groupname <name>:  Group membership of Fusion user (default gegroup)\n"
     "  --chown :            Fix file privileges\n"
     "  --noprompt:          do not prompt for more information, returns -1\n"
     "                       to indicate an error if command fails or has insufficient arguments\n"
     "  --secure             Removes world read and write permissions.\n",
     prog, CommandlineAssetRootDefault().c_str());
  exit(1);
}

int
main(int argc, char *argv[]) {
  try {
    // process commandlinw options
    int argn;
    bool help = false;
    std::string assetroot = CommandlineAssetRootDefault();
    std::string username = Systemrc::FusionUsername();
    std::string groupname = Systemrc::UserGroupname();
    bool noprompt = false;
    bool chown = false;
    bool secure = false;

    khGetopt options;
    options.helpOpt(help);
    options.opt("assetroot", assetroot);
    options.opt("username", username);
    options.opt("groupname", groupname);
    options.opt("noprompt", noprompt);
    options.opt("chown", chown);
    options.opt("secure", secure);

    if (!options.processAll(argc, argv, argn) || help) {
      usage(argv[0]);
    }

    // Make sure I'm root, everything is shut down,
    // and switch to fusion user
    std::string thishost = ValidateHostReadyForConfig();
    SwitchToUser(username, groupname);

    // validate the asset root
    AssetRootStatus status(assetroot, thishost, username, groupname);
    ValidateAssetRootForUpgrade(status, noprompt, chown);

    // short circuit
    if (!status.AssetRootNeedsUpgrade()) {
      printf("No upgrade needed.\n");
      return 0;
    }

    // tell the other libs what dir to use for the asset root
    AssetDefs::OverrideAssetRoot(status.assetroot_);

    // do the actual upgrade
    UpgradeAssetRoot(status.version_, secure);


  } catch (const std::exception &e) {
    notify(NFY_FATAL, "\n%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}

void ValidateAssetRootForUpgrade(const AssetRootStatus &status, bool noprompt,
                                 bool chown) {
  if (status.assetroot_.empty()) {
    throw khException(kh::tr("No asset root has been specified."));
  }

  if (!status.dir_exists_) {
    throw khException(kh::tr("%1 doesn't exist.").arg(status.assetroot_.c_str()));
  }

  if (!status.has_volumes_) {
    throw khException(kh::tr("%1 isn't a valid asset root.")
                      .arg(status.assetroot_.c_str()));
  }

  if (!status.IsThisMachineMaster()) {
    throw khException(kh::tr(
"%1 is currently defined as the master for %2.\n"
"To upgrade, you must run the following command from %3:\n"
"  geupgradeassetroot --assetroot %4\n"
"If the asset root's hostname, %5, is not \n"
"set to the fully qualified hostname, \n"
"then run the following command to fix it:\n"
"  geconfigureassetroot --assetroot %6 --fixmasterhost\n"
"then run geupgradeassetroot again.")
                      .arg(status.master_host_.c_str())
                      .arg(status.assetroot_.c_str())
                      .arg(status.master_host_.c_str())
                      .arg(status.assetroot_.c_str())
                      .arg(status.master_host_.c_str())
                      .arg(status.assetroot_.c_str())
                      );
  }

  // Make sure it's not too old
  if (DottedVersion(status.version_) < DottedVersion("2.4")) {
    throw khException(kh::tr(
"%1 is configured for Google Earth Fusion version %2.\n"
"Upgrades are not possible from versions older than 2.4.")
                      .arg(status.assetroot_.c_str())
                      .arg(status.version_.c_str()));
  }

  // we don't check AssetRootNeedsUpgrade here
  // that happens in the function that called us

  if (status.SoftwareNeedsUpgrade()) {
    status.ThrowSoftwareNeedsUpgrade();
  }

  if (!status.owner_ok_ && chown) {
    // will throw if user doesn't confirm
    PromptUserAndFixOwnership(status.assetroot_, noprompt);
  }

  // don't check for unique id's. They might be missing just because we need
  // an upgrade. Skipping that check here won't hurt, since we'll catch it in
  // in the daemon check tools before launching the daemons.
}


void UpgradeAssetRoot(const DottedVersion &version, bool secure) {

  // Since we're upgrading, go ahead and fix perms on special files
  FixSpecialPerms(AssetDefs::AssetRoot(), secure);

  WarnAboutPendingTasks();

  if (version < DottedVersion("2.5")) {
    UpgradeFrom2_4_X();
    // will cascade to other ifs below
  }

  if (version < DottedVersion("3.0")) {
    UpgradeFrom2_5_X(version);
    // will cascade to other ifs below
  }

  if (version < DottedVersion("3.1")) {
    UpgradeFrom3_0_X(version);
    // will cascade to other ifs below
  }

  // update the .fusionversion file w/ the current software version
  geAssetRoot::WriteVersion(AssetDefs::AssetRoot());

  printf("Upgraded %s to version %s.\n",
         AssetDefs::AssetRoot().c_str(), GEE_VERSION);
}


void UpgradeFrom2_4_X(void) {
}


void UpgradeFrom2_5_X(const DottedVersion &version) {
  // touch the FusionUniqueId.xml file
  if (!khExists(geAssetRoot::Filename(AssetDefs::AssetRoot(),
                                      geAssetRoot::UniqueIdFile))) {
    FusionUniqueId::TouchNew(AssetDefs::AssetRoot());
  }
}

void UpgradeFrom3_0_X(const DottedVersion &version) {
}

void WarnAboutPendingTasks(void) {
  // Check for .task files - we should have caught this before we got to the
  // upgrade step, but now that we have gottern here there is nothing we can
  // do but delete the old task files.
  std::vector<std::string> taskfiles;
  khFindFilesInDir(geAssetRoot::Dirname(AssetDefs::AssetRoot(),
                                        geAssetRoot::StateDir),
                   taskfiles, ".task");
  if (taskfiles.size() > 0) {
    fprintf(stderr, "***** WARNING *****\n");
    fprintf(stderr, "Found tasks pending from before a software upgrade:\n");
    for (unsigned int i = 0; i < taskfiles.size(); ++i) {
      std::string target;
      if (khReadSymlink(taskfiles[i], target)) {
        fprintf(stderr, "    %s\n", target.c_str());
      }
    }
    fprintf(stderr,
            "These tasks are now stuck as \"Waiting\" or \"InProgress\".\n"
            "You will need to cancel and then resume these tasks.\n"
            "If Fusion is unable to complete any of these tasks, make a small change\n"
            "in the corresponding asset and build it again.\n\n");
    for (unsigned int i = 0; i < taskfiles.size(); ++i) {
      (void)khUnlink(taskfiles[i]);
    }
  }
}


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
#include <config/RecoverIds.h>
#include <config/gePrompt.h>
#include <config/geConfigUtil.h>
#include <DottedVersion.h>
#include <autoingest/geAssetRoot.h>
#include <geUsers.h>
#include <config/geCapabilities.h>
#include <autoingest/.idl/VolumeStorage.h>
#include <khSpawn.h>
#include <autoingest/.idl/Systemrc.h>
#include "common/khException.h"


namespace {

void usage(const char *prog, const char *msg = 0, ...) {
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
     "usage:\n"
     "  %s {--new|--repair|--editvolumes|--fixmasterhost|--addvolume|--removevolume|--listvolumes [options]}\n"
     "Configure this machine to run Google Earth Fusion.\n"
     "  --help, -?                  Display this usage message\n"
     "\n"
     "Options:\n"
     "  --assetroot <dir>           Path to asset root [current: %s]\n"
     "                              It is shown in the commands below as\n"
     "                              mandatory or optional. If optional, then\n"
     "                              the current asset root is used if it is\n"
     "                              not specified.\n"
     "  --noprompt                  Do not prompt for more information,\n"
     "                              returns -1 to indicate an error if\n"
     "                              command fails or has insufficient\n"
     "                              arguments.\n"
     "  --chown                     Attempt to fix privileges.\n"
     "  --secure                    Removes world read and write permissions.\n"
     "When creating a new asset root, additional options are available:\n"
     "  --srcvol <dir>              Path to source volume.\n"
     "\n"
     "Commands:\n"
     "  --new --assetroot <dir>     Create new asset root in specified\n"
     "    [--srcvol <dir>]          directory.\n"
     "  --repair                    Repair various inconsistencies in the\n"
     "    [--assetroot <dir>]       asset root\n"
     "  --editvolumes               Follow the prompts to add a volume to the\n"
     "    [--assetroot <dir>]       selected asset root or, modify the local\n"
     "                              path definition for an existing volume,\n"
     "                              or to add a volume definition.\n"
     "  --listvolumes               List available volumes in the asset root.\n"
     "    [--assetroot <dir>]\n"
     "  --fixmasterhost             Change the asset root host entry to match\n"
     "    [--assetroot <dir>]       the current host name.\n"
     "  --addvolume <volume_name>:<dir>\n"
     "    [--assetroot <dir>]       Add a volume with the given name and\n"
     "                              directory.\n"
     "  --removevolume <volume_name>\n"
     "    [--assetroot <dir>]       Remove a volume with the given name.\n"
     "\n",
     prog, CommandlineAssetRootDefault().c_str());
  exit(1);
}

void ValidateAssetRootForConfigure(const AssetRootStatus &status,
                                   bool iscreate, bool isfixmaster,
                                   bool noprompt, bool chown);
void MakeNewAssetRoot(const AssetRootStatus &status,
                      const std::string &srcvol,
                      const std::string &username,
                      const std::string &groupname,
                      bool noprompt,
                      bool secure);
void RepairExistingAssetRoot(const AssetRootStatus &status,
                             bool noprompt,
                             bool secure);
void AddVolume(const AssetRootStatus &status,
               const std::string &volume_name, const std::string &volume_dir);
void RemoveVolume(const AssetRootStatus &status, const std::string &volume_name);
void EditVolumes(const AssetRootStatus &status);
void ListVolumes(const AssetRootStatus &status);
void FixMasterHost(const AssetRootStatus &status);

}  // namespace


int main(int argc, char *argv[]) {
  try {
    // ********** process commandline options **********
    int argn;
    bool help  = false;
    bool editvolumes = false;
    bool create      = false;
    bool repair      = false;
    bool fixmasterhost = false;
    bool listvolumes = false;
    bool secure = false;
    std::string assetroot = CommandlineAssetRootDefault();
    std::string username = Systemrc::FusionUsername();
    std::string groupname = Systemrc::UserGroupname();
    std::string srcvol;
    std::string addvolume;
    std::string removevolume;
    bool noprompt = false;
    bool chown = false;

    khGetopt options;
    options.helpOpt(help);
    options.opt("new", create);
    options.opt("repair", repair);
    options.opt("editvolumes", editvolumes);
    options.opt("assetroot", assetroot);
    options.opt("fixmasterhost", fixmasterhost);
    options.opt("srcvol", srcvol);
    options.opt("addvolume", addvolume);
    options.opt("removevolume", removevolume);
    options.opt("listvolumes", listvolumes);
    options.opt("noprompt", noprompt);
    options.opt("chown", chown);
    options.opt("secure", secure);
    options.setExclusiveRequired(makeset(std::string("new"),
                                         std::string("repair"),
                                         std::string("editvolumes"),
                                         std::string("addvolume"),
                                         std::string("removevolume"),
                                         std::string("listvolumes"),
                                         std::string("fixmasterhost")));

    if (!options.processAll(argc, argv, argn) || help) {
      usage(argv[0]);
    }

    if (argn < argc) {
      usage(argv[0], "Unrecognized parameters specified");
    }

    // Make sure I'm root, everything is shut down,
    // and switch to fusion user
    std::string thishost;
    if (listvolumes) {
      AssertRunningAsRoot();
      thishost = GetAndValidateHostname();
    } else {
      thishost = ValidateHostReadyForConfig();
    }
    printf("Switching to (%s, %s)\n", username.c_str(), groupname.c_str());
    SwitchToUser(username, groupname);

    // validate the asset root
    printf("Setting up assetroot status object\n");
    AssetRootStatus status(assetroot, thishost, username, groupname);
    printf("Validating assetroot\n");
    ValidateAssetRootForConfigure(status, create, fixmasterhost, noprompt,
                                  chown);

    // tell the other libraries what assetroot we're going to be using
    AssetDefs::OverrideAssetRoot(status.assetroot_);

    if (create) {
      printf("Making new assetroot ....\n");
      MakeNewAssetRoot(status, srcvol, username, groupname, noprompt, secure);
      if (!noprompt && !editvolumes &&
          geprompt::confirm(kh::tr(
                                "Would you like to add more volumes"),
                            'N')) {
        editvolumes = true;
      }
    }
    // don't make an if-ladder here. We need to run multiple sometimes
    if (repair) {
      RepairExistingAssetRoot(status, noprompt, secure);
    }
    if (editvolumes) {
      EditVolumes(status);
    }
    if (fixmasterhost) {
      FixMasterHost(status);
    }
    if (listvolumes) {
      ListVolumes(status);
    }
    if (!addvolume.empty()) {
      std::string::size_type index = addvolume.find(":", 0);
      if (index == std::string::npos) {
        throw khException(kh::tr("'%1' isn't a valid volume specifier.\n"
                             "Use --addvolume <volume_name>:<dir>")
                      .arg(addvolume.c_str()));
      }
      std::string volume_name = addvolume.substr(0, index);
      std::string volume_directory = addvolume.substr(index+1);
      if (volume_name.empty()) {
        throw khException(kh::tr("'%1' is missing a volume name.\n"
                             "Use --addvolume <volume_name>:<dir>")
                      .arg(addvolume.c_str()));
      }
      if (volume_directory.empty()) {
        throw khException(kh::tr("'%1' is missing a volume directory.\n"
                             "Use --addvolume <volume_name>:<dir>")
                      .arg(addvolume.c_str()));
      }
      AddVolume(status, volume_name, volume_directory);
    }
    if (!removevolume.empty()) {
      RemoveVolume(status, removevolume);
    }
    if (!listvolumes) {
      printf("Configured %s.\n", status.assetroot_.c_str());
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "\n%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}


namespace {

void ValidateAssetRootForConfigure(const AssetRootStatus &status,
                                   bool iscreate, bool isfixmaster,
                                   bool noprompt, bool chown) {
  QString UseNewMsg = kh::tr(
      "%1 isn't an existing assetroot.\n"
      "You must use --new to make a new one.")
      .arg(status.assetroot_.c_str());

  if (status.assetroot_.empty()) {
    throw khException(kh::tr("No asset root has been specified."));
  }

  if (!status.dir_exists_) {
    if (!iscreate) {
      throw khException(UseNewMsg);
    }

    // don't check anything else - we're going to make a new one
    return;
  }

  if (!status.has_volumes_) {
    if (!iscreate) {
      throw khException(UseNewMsg);
    }
    if (noprompt) {
      // Do nothing, create a new asset root here.
    } else if (!geprompt::confirm(kh::tr(
        "%1 already exists.\n"
        "But it does not contain a valid asset root.\n"
        "Do you want to create a new asset root there")
                                  .arg(status.assetroot_.c_str()), 'N')) {
      throw khException(kh::tr("Aborted by user"));
    }

    // don't check anything else - we're going to make a new one here
    return;
  }

  if (iscreate) {
    throw khException(
        kh::tr("%1 is an existing assetroot.\n"
               "You cannot configure it with --new.")
        .arg(status.assetroot_.c_str()));
  }


  if (!isfixmaster && !status.IsThisMachineMaster()) {
    throw khException(kh::tr(
        "%1 is currently defined as the master for %2.\n"
        "You must run geconfigureassetroot from %3")
                      .arg(status.master_host_.c_str())
                      .arg(status.assetroot_.c_str())
                      .arg(status.master_host_.c_str()));
  }

  if (!isfixmaster && status.SoftwareNeedsUpgrade()) {
    status.ThrowSoftwareNeedsUpgrade();
  }

  // we don't check AssetRootNeedsUpgrade() on existing asset roots. That's not
  // our job. it's the update tool's job.

  if (!status.owner_ok_ && chown) {
    // will throw if user doesn't confirm
    PromptUserAndFixOwnership(status.assetroot_, noprompt);
  }

  // don't check for unique id's. It doesn't affect whether or not we can
  // configure this directory.
}



void MakeNewAssetRoot(const AssetRootStatus &status,
                      const std::string &in_srcvol,
                      const std::string &username,
                      const std::string &groupname,
                      bool noprompt,
                      bool secure) {
  geUserId fusion_user(username, groupname);

  // escalate my permissions and try to create the assetroot
  {
    geCapabilitiesGuard cap_guard(
        CAP_DAC_OVERRIDE,     // let me read all files
        CAP_DAC_READ_SEARCH,  // let me traverse all dirs
        CAP_CHOWN,            // let me chown files
        CAP_FOWNER);          // let me chmod files I dont own

    if (MakeSpecialDirs(status.assetroot_, fusion_user, secure)) {
      // we had to chown some of them. There might be more too.
      // Let's just chown the whole tree right now
      PromptUserAndFixOwnership(status.assetroot_, noprompt);
    }
  }


  // setup assetoot volume
  VolumeDefList voldefs;
  voldefs.volumedefs[geAssetRoot::VolumeName] =
    VolumeDefList::VolumeDef(status.assetroot_,  // netpath
                             status.thishost_,   // host
                             status.assetroot_,  // localpath
                             100000000,          // reserveSpace
                             false);             // isTmp

  // maybe setup src volume
  std::string srcvol = in_srcvol;
  if (noprompt) {
    // Need to create the source volume directory if it doesn't exist.
    if (!khDirExists(srcvol)) {
      geCapabilitiesGuard cap_guard(
          CAP_DAC_OVERRIDE,     // let me read all files
          CAP_DAC_READ_SEARCH,  // let me traverse all dirs
          CAP_CHOWN,            // let me chown files
          CAP_FOWNER);          // let me chmod files I dont own
      if (!khMakeDir(srcvol)) {
        notify(NFY_WARN, "Unable to create %s", srcvol.c_str());
      }
    }
  } else {
    // Ask user to confirm the source volume.
    if (srcvol.empty()) {
      if (geprompt::confirm(
              "No source volume specified - do you want to create one", 'Y')) {
        srcvol = geprompt::enterDirname(
            "Enter directory for source volume",
            khComposePath(khDirname(status.assetroot_), "src"));
      }
    } else {
      srcvol = geprompt::enterDirname("Enter directory for source volume",
                                      srcvol, true /* skip first prompt */);
    }
  }
  // If a src volume was specified, fix the permissions and add it to
  // the volume defs.
  if (!srcvol.empty()) {
    if (!khChmod(srcvol, 0777)) {
      notify(NFY_WARN, "Unable to open permissions on %s", srcvol.c_str());
    }
    voldefs.volumedefs["src"] =
      VolumeDefList::VolumeDef(srcvol ,           // netpath
                               status.thishost_,  // host
                               srcvol,            // localpath
                               100000000,         // reserverSpace
                               false);            // isTmp
  }

  // write the volumelist file
  SaveVolumesOrThrow(status.assetroot_, voldefs);

  // touch an empty FusionUniqueId.xml
  FusionUniqueId::TouchNew(status.assetroot_);

  // write the fusion version file
  geAssetRoot::WriteVersion(status.assetroot_);
}


void RepairExistingAssetRoot(const AssetRootStatus &status,
                             bool noprompt,
                             bool secure) {
  // fix perms on special files
  FixSpecialPerms(status.assetroot_, secure);

  if (!status.AssetRootNeedsUpgrade() && !status.unique_ids_ok_) {
    // if we have to upgrade, don't try to fix the ids yet.
    // They might get fixed by the upgrade.
    // Will pul assetroot from AssetDefs()::AssetRoot()
    PromptUserAndFixUniqueIds(noprompt);
  }
}

// ****************************************************************************
// ***  ListVolumes
// ****************************************************************************
void listCurrentVolumes(VolumeDefList *const voldefs, std::vector<std::string> volnames) {
    printf("\nCurrent Volume Definitions:\n");
    printf("--- name: netpath,localpath,isTmp ---\n");
    unsigned int i = 0;
    for (std::vector<std::string>::const_iterator vn = volnames.begin();
         vn != volnames.end(); ++vn, ++i) {
      printf("%d)  %s: %s,%s,%s\n",
             i+1,
             vn->c_str(),
             voldefs->volumedefs[*vn].netpath.c_str(),
             voldefs->volumedefs[*vn].localpath.c_str(),
             voldefs->volumedefs[*vn].isTmp ? "Y" : "N");
    }
    printf("-------------------------------------\n");
}

void ListHostVolumes(const std::string &host, VolumeDefList *const voldefs) {
  std::vector<std::string> volnames;
  for (const auto& v : voldefs->volumedefs) {
    if (v.second.host == host) {
      volnames.push_back(v.first);
    }
  }
  listCurrentVolumes(voldefs, volnames);
}

void ListVolumes(const AssetRootStatus &status) {
  VolumeDefList voldefs;
  LoadVolumesOrThrow(status.assetroot_, voldefs);
  ListHostVolumes(status.thishost_, &voldefs);
}


// ****************************************************************************
// ***  EditVolumes
// ****************************************************************************
void EditHostVolumes(const std::string &host, VolumeDefList *const voldefs) {
  std::string assetroot;
  std::vector<std::string> volnames;

  for (const auto& v : voldefs->volumedefs) {
    if (v.first == geAssetRoot::VolumeName) {
      assetroot = v.second.netpath;
    }
    if (v.second.host == host) {
      volnames.push_back(v.first);
    }
  }

  std::string volume_prefix = khDirname(assetroot);
  while (1) {
    listCurrentVolumes(voldefs, volnames);
    char editchoice =
      geprompt::choose("{F}inished Editing. {A}dd Volume. {M}odify Volume.",
                      'F', 'A', 'M',
                      'F');
    if (editchoice == 'F') {
      break;
    } else if (editchoice == 'A') {
      std::string volname = geprompt::enterString("Enter volume name");
      if (volname.size()) {
        if (volname.find('/') != std::string::npos) {
          notify(NFY_WARN, "Volume names may not contain '/'");
          continue;
        }
        VolumeDefList::VolumeDefMap::const_iterator found =
          voldefs->volumedefs.find(volname);
        if (found != voldefs->volumedefs.end()) {
          notify(NFY_WARN, "A volume named '%s' already exists",
                 volname.c_str());
        } else {
          std::string netpath =
            geprompt::enterDirname("Enter netpath",
                                  khComposePath(volume_prefix, volname));
          bool error = false;
          for (VolumeDefList::VolumeDefMap::const_iterator v =
                 voldefs->volumedefs.begin();
               v != voldefs->volumedefs.end(); ++v) {
            if (v->second.netpath == netpath) {
              notify(NFY_WARN,
                     "Volume '%s' already has netpath of '%s'",
                     v->first.c_str(),
                     netpath.c_str());
              error = true;
              break;
            }
          }
          if (!error) {
            std::string localpath =
              geprompt::enterDirname("Enter localpath", netpath);
            bool istmp = false;
            voldefs->volumedefs[volname] =
              VolumeDefList::VolumeDef(netpath,
                                       host,
                                       localpath,
                                       100000000,
                                       istmp);
            volnames.push_back(volname);
          }
        }
      }
    } else if (editchoice == 'M') {
      if (volnames.empty()) {
        notify(NFY_WARN, "No volumes to modify");
      } else {
        int idx = geprompt::chooseNumber("Enter volume number to modify",
                                        1, volnames.size()+1);
        std::string localpath =
          geprompt::enterDirname("Enter new localpath",
                                voldefs->volumedefs[volnames[idx-1]].localpath);
        voldefs->volumedefs[volnames[idx-1]].localpath = localpath;
        bool istmp = false;
        voldefs->volumedefs[volnames[idx-1]].isTmp = istmp;
      }
    }
  }
}

void AddVolume(const AssetRootStatus &status,
               const std::string &volume_name,
               const std::string &volume_dir) {
  VolumeDefList voldefs;
  LoadVolumesOrThrow(status.assetroot_, voldefs);
  VolumeDefList oldvoldefs = voldefs;

  // We must add volume_name, volume_dir to the voldefs
  // Check for a unique volume name.
  VolumeDefList::VolumeDefMap::const_iterator found =
          voldefs.volumedefs.find(volume_name);
  if (found != voldefs.volumedefs.end()) {
    throw khException(kh::tr("A volume named '%1' already exists")
                  .arg(volume_name.c_str()));
  }

  // Check for a unique netpath.
  for (VolumeDefList::VolumeDefMap::const_iterator v =
         voldefs.volumedefs.begin();
       v != voldefs.volumedefs.end(); ++v) {
    if (v->second.netpath == volume_dir) {
       throw khException(kh::tr("A netpath named '%1' already exists")
                .arg(volume_name.c_str()));
    }
  }

  // Update the Volume def.
  bool istmp = false;
  voldefs.volumedefs[volume_name] =
    VolumeDefList::VolumeDef(volume_dir,
                             status.thishost_,
                             volume_dir,
                             100000000,
                             istmp);

  // Update the volumes.
  SaveVolumesOrThrow(status.assetroot_, voldefs);
  printf("Volumes modified\n");
}


void RemoveVolume(const AssetRootStatus &status,
                  const std::string &volume_name) {

  VolumeDefList voldefs;
  LoadVolumesOrThrow(status.assetroot_, voldefs);
  VolumeDefList oldvoldefs = voldefs;

  // Check that the volume exists
  VolumeDefList::VolumeDefMap::iterator found =
          voldefs.volumedefs.find(volume_name);
  if (found == voldefs.volumedefs.end()) {
    throw khException(kh::tr("The volume named '%1' does not exist")
                  .arg(volume_name.c_str()));
  }

  // Update the Volume def.
  voldefs.volumedefs.erase(found);

  // Update the volumes.
  SaveVolumesOrThrow(status.assetroot_, voldefs);

  printf("Volumes modified\n");
}

void EditVolumes(const AssetRootStatus &status) {
  VolumeDefList voldefs;
  LoadVolumesOrThrow(status.assetroot_, voldefs);
  VolumeDefList oldvoldefs = voldefs;
  EditHostVolumes(status.thishost_, &voldefs);
  if (oldvoldefs == voldefs) {
    printf("No volume changes to save.\n");
  } else {
    SaveVolumesOrThrow(status.assetroot_, voldefs);
    printf("Volumes modified\n");
  }
}


void FixMasterHost(const AssetRootStatus &status) {
  VolumeDefList voldefs;
  LoadVolumesOrThrow(status.assetroot_, voldefs);
  VolumeDefList oldvoldefs = voldefs;

  // find the master
  std::string master;
  for (VolumeDefList::VolumeDefMap::const_iterator v =
         voldefs.volumedefs.begin();
       v != voldefs.volumedefs.end(); ++v) {
    if (v->first == geAssetRoot::VolumeName) {
      master = v->second.host;
      break;
    }
  }
  // change all with the master host to this host
  for (VolumeDefList::VolumeDefMap::iterator v =
         voldefs.volumedefs.begin();
       v != voldefs.volumedefs.end(); ++v) {
    if (v->second.host == master) {
      v->second.host = status.thishost_;
    }
  }


  if (oldvoldefs == voldefs) {
    printf("No volume changes to save.\n");
  } else {
    SaveVolumesOrThrow(status.assetroot_, voldefs);
    printf("Volumes modified\n");
  }
}

}  // namespace

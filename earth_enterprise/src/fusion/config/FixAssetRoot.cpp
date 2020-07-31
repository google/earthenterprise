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



#include "FixAssetRoot.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <config/gePrompt.h>
#include <autoingest/geAssetRoot.h>
#include <autoingest/.idl/Systemrc.h>
#include <khFileUtils.h>
#include <khException.h>
#include <khSpawn.h>
#include <geUsers.h>


namespace {
void FixDirPerms(const std::string &fname, unsigned int mode) {
  struct stat64 sb;
  if (::stat64(fname.c_str(), &sb) == 0) {
    if (sb.st_mode != mode) {
      if (!khChmod(fname, mode)) {
        throw khException(kh::tr("Unable to chmod %1 0x%2")
                          .arg(fname).arg(mode, 8));
      }
    }
  } else if (!khMakeDir(fname, mode)) {
    throw khException(kh::tr("Unable to mkdir %1")
                      .arg(fname));
  } else {
    // just in case the umask clobberd the perms.
    khChmod(fname, mode);
  }
}
void FixFilePerms(const std::string &fname, unsigned int mode) {
  struct stat64 sb;
  if (::stat64(fname.c_str(), &sb) == 0) {
    if (sb.st_mode != mode) {
      if (!khChmod(fname, mode)) {
        throw khException(kh::tr("Unable to chmod %1 0x%2")
                          .arg(fname).arg(mode, 8));
      }
    }
  }
}
} // namespace



void FixSpecialPerms(const std::string &assetroot, bool secure) {
  if (secure) {
    for (geAssetRoot::SpecialDir dir = geAssetRoot::FirstSpecialDir;
        dir <= geAssetRoot::LastSpecialDir;
        dir = geAssetRoot::SpecialDir(int(dir)+1)) {
      FixDirPerms(geAssetRoot::Dirname(assetroot, dir),
                  geAssetRoot::SecureDirPerms(dir));
    }
  }
  else {
    for (geAssetRoot::SpecialDir dir = geAssetRoot::FirstSpecialDir;
        dir <= geAssetRoot::LastSpecialDir;
        dir = geAssetRoot::SpecialDir(int(dir)+1)) {
      FixDirPerms(geAssetRoot::Dirname(assetroot, dir),
                  geAssetRoot::DirPerms(dir));
    }
  }
  for (geAssetRoot::SpecialFile file = geAssetRoot::FirstSpecialFile;
       file <= geAssetRoot::LastSpecialFile;
       file = geAssetRoot::SpecialFile(int(file)+1)) {
    FixFilePerms(geAssetRoot::Filename(assetroot, file),
                 geAssetRoot::FilePerms(file));
  }
}





void PromptUserAndFixOwnership(const std::string &assetroot, bool noprompt) {
  QString msg = kh::tr(
"Some key files in %1 have incorrect user and/or group ownership.\n"
"This is likely due to an upgrade from and earlier version of\n"
"Google Earth Fusion which wrote files as keyhole:users.\n"
"This version of Google Earth Fusion writes files as %2:%3 and %2:%4.\n"
"\n"
"This tool will now run the following commands:\n"
"    chown -R %2:%3 %1\n"
"    chown %2:%4 %5 %6\n"
"Depending on the size of your asset root, this could take a while.\n")
                .arg(assetroot, Systemrc::FusionUsername(), Systemrc::UserGroupname(), 
                     Systemrc::GuiGroupname(), assetroot + "/.userdata", 
                     assetroot + "/.config");

  if (!noprompt) {
    // confirm with user that it's OK to chown
    fprintf(stderr, "%s", (const char *)msg.utf8());
    if (!geprompt::confirm(kh::tr("Proceed with chown"), 'Y')) {
      throw khException(kh::tr("Aborted by user"));
    }
  }


  std::string tochown = assetroot;
  while (khSymlinkExists(tochown)) {
    (void)khReadSymlink(tochown, tochown);
  }


  // do the chown
  CmdLine gegroup_chown;
  gegroup_chown << "chown"
          << "-R"
          << Systemrc::FusionUsername() + ":" + Systemrc::UserGroupname()
          << tochown;
  if (!gegroup_chown.System(CmdLine::SpawnAsRealUser)) {
    throw khException(kh::tr("Unable to change ownership of asset root"));
  }

  CmdLine gegui_chown;
  gegui_chown << "chown"
          << Systemrc::FusionUsername() + ":" + Systemrc::GuiGroupname()
          << tochown + "/.userdata"
          << tochown + "/.config";
  if (!gegui_chown.System(CmdLine::SpawnAsRealUser)) {
    throw khException(kh::tr("Unable to change ownership of configuration files in the asset root"));
  }
}


namespace {

bool
MakeDirWithAttrs(const std::string &dirname, mode_t mode, const geUserId &user)
{
  bool user_changed = false;
  struct stat64 sb;
  if (stat64(dirname.c_str(), &sb) < 0) {
    if (!khMakeDir(dirname, mode)) {
      throw khException(kh::tr("Unable to mkdir %1")
                        .arg(dirname));
    } else {
      // just incase the umask clobbered the perms
      khChmod(dirname, mode);
    }
    if (stat64(dirname.c_str(), &sb) < 0) {
      throw khException(kh::tr("Unable to stat %1")
                        .arg(dirname));
    }

  }

  if ((sb.st_uid != user.Uid()) ||
      (sb.st_gid != user.Gid())) {
    if (::chown(dirname.c_str(), user.Uid(), user.Gid()) < 0) {
      throw khErrnoException(
          kh::tr("Unable to 'chown %1:%2 %3'")
          .arg(user.Uid()).arg(user.Gid()).arg(dirname));
    }

    if (stat64(dirname.c_str(), &sb) < 0) {
      throw khException(kh::tr("Unable to stat %1")
                        .arg(dirname));
    }
  }

  if ((sb.st_mode != mode) && (::chmod(dirname.c_str(), mode) < 0)) {
    throw khErrnoException(
        kh::tr("Unable to 'chmod 0%1 %2'")
        .arg(mode, 0, 8).arg(dirname));
  }

  return user_changed;
}

} // namespace


bool MakeSpecialDirs(const std::string &assetroot,
                     const geUserId &fusion_user,
                     const geUserId &gui_user,
                     bool secure) {
  bool user_changed = false;

  // make any parents of the asset root w/ more restrictive permissisons
  if (!khEnsureParentDir(assetroot, 0755)) {
    throw khException(kh::tr("Cannot create parent directories for %1")
                      .arg(assetroot));
  }

  // now make/fix the other dirs
  for (geAssetRoot::SpecialDir dir = geAssetRoot::FirstSpecialDir;
       dir <= geAssetRoot::LastSpecialDir;
       dir = geAssetRoot::SpecialDir(int(dir)+1)) {
    geUserId user = fusion_user;
    // Check if the directory is either .userdata or .config
    if (dir == geAssetRoot::SpecialDir::ConfigDir || dir == geAssetRoot::SpecialDir::UserDataDir) {
      user = gui_user;
    }
    if (secure) {
      if (MakeDirWithAttrs(geAssetRoot::Dirname(assetroot, dir),
                          geAssetRoot::SecureDirPerms(dir),
                          user)) {
        user_changed = true;
      }
    }
    else {
      if (MakeDirWithAttrs(geAssetRoot::Dirname(assetroot, dir),
                          geAssetRoot::DirPerms(dir),
                          user)) {
        user_changed = true;
      }
    }
  }

  return user_changed;
}

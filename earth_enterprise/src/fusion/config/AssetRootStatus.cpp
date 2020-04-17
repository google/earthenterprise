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



#include "AssetRootStatus.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <khFileUtils.h>
#include <khException.h>
#include <geUsers.h>
#include <config/geCapabilities.h>
#include <fusionversion.h>
#include <autoingest/sysman/.idl/FusionUniqueId.h>
#include <autoingest/geAssetRoot.h>
#include <autoingest/.idl/Systemrc.h>
#include <autoingest/.idl/VolumeStorage.h>
#include <DottedVersion.h>


namespace {
bool BadFileOwnership(const std::string &fname, const geUserId &user) {
  struct stat64 sb;
  if (::stat64(fname.c_str(), &sb) == 0) {
    return ((sb.st_uid != user.Uid()) ||
            (sb.st_gid != user.Gid()));
  } else {
    return false;
  }
}
} // namespace




AssetRootStatus::AssetRootStatus(const std::string &dirname,
                                 const std::string &thishost,
                                 const std::string &username,
                                 const std::string &groupname) :
    // initialize members to worst-case scenario
    assetroot_(khNormalizeDir(dirname,
                              false /* want trailing '/' */)),
    thishost_(thishost),
    dir_exists_(false),
    has_volumes_(false),
    master_host_(),
    master_active_(true),
    version_(),
    unique_ids_ok_(false),
    owner_ok_(false)
{
  // since this may be an old assetroot (owned by a different user)
  // we need to beef up our capabilities while making these checks
  geCapabilitiesGuard cap_guard(
      CAP_DAC_OVERRIDE,      // let me read all files
      CAP_DAC_READ_SEARCH);  // let me traverse all dirs

  if (assetroot_.empty()) {
    return;
  }

  // check the directory itself
  if (!(dir_exists_ = khDirExists(assetroot_))) {
    // there is nothing else I can check
    return;
  }

  // check the volumes list
  {
    std::string volumefname =
      geAssetRoot::Filename(assetroot_, geAssetRoot::VolumeFile);
    if (khExists(volumefname)) {
      VolumeDefList volumes;
      if (volumes.Load(volumefname)) {
        master_host_ = volumes.VolumeHost(geAssetRoot::VolumeName);
        has_volumes_ = !master_host_.empty();
      }
    }
    if (!has_volumes_) {
      return;
    }
  }

  // is master active
  master_active_ =
    khExists(geAssetRoot::Filename(assetroot_, geAssetRoot::ActiveFile));

  // asset root version
  version_ = geAssetRoot::ReadVersion(assetroot_);

  // unique id file
  try {
    FusionUniqueId::Init(assetroot_);
    unique_ids_ok_ = true;
  } catch (...) {
  }

  // check file ownerships
  {
    owner_ok_ = true;
    geUserId fusion_user(username, groupname);
    for (geAssetRoot::SpecialDir dir = geAssetRoot::FirstSpecialDir;
         dir <= geAssetRoot::LastSpecialDir;
         dir = geAssetRoot::SpecialDir(int(dir)+1)) {
      std::string dirname = geAssetRoot::Dirname(assetroot_, dir);
      if (BadFileOwnership(dirname, fusion_user)) {
        owner_ok_ = false;
      }
    }
    for (geAssetRoot::SpecialFile file = geAssetRoot::FirstSpecialFile;
         file <= geAssetRoot::LastSpecialFile;
         file = geAssetRoot::SpecialFile(int(file)+1)) {
      std::string filename = geAssetRoot::Filename(assetroot_, file);
      if (BadFileOwnership(filename, fusion_user)) {
        owner_ok_ = false;
      }
    }
  }
}


bool AssetRootStatus::IsMaster(const std::string &host) const {
  return host == master_host_;
}

bool AssetRootStatus::AssetRootNeedsUpgrade(void) const {
  return DottedVersion(version_) < DottedVersion(GEE_VERSION);
}

bool AssetRootStatus::SoftwareNeedsUpgrade(void) const {
  return DottedVersion(version_) > DottedVersion(GEE_VERSION);
}

bool AssetRootStatus::AssetRootNeedsRepair(void) const {
  return (!unique_ids_ok_ || !owner_ok_);
}



std::string AssetRootStatus::FromHostString(void) const {
  bool is_master = (thishost_ == master_host_);
  return is_master
    ? QString("").toUtf8().constData()
    : kh::tr(" from %1").arg(master_host_.c_str()).toUtf8().constData();
}

void AssetRootStatus::ThrowAssetRootNeedsRepair(void) const {
  QString repair_msg = kh::tr(
"%1 needs to be repaired.\n"
"You must run the following command%2:\n"
"  geconfigureassetroot --repair --assetroot %3\n");
  throw khException(repair_msg
                    .arg(assetroot_.c_str())
                    .arg(FromHostString().c_str())
                    .arg(assetroot_.c_str())
                    );
}

void AssetRootStatus::ThrowAssetRootNeedsUpgrade(void) const {
  QString root_upgrade_msg = kh::tr(
"This machine is running fusion software version %1.\n"
"%2 is configured to use version %3.\n"
"In order to use %4 from this machine, it must be upgraded.\n"
"Run the following command%5:\n"
"  geupgradeassetroot --assetroot %6\n");
  throw khException(root_upgrade_msg
                    .arg(GEE_VERSION)
                    .arg(assetroot_.c_str())
                    .arg(version_.c_str())
                    .arg(assetroot_.c_str())
                    .arg(FromHostString().c_str())
                    .arg(assetroot_.c_str())
                    );

}

void AssetRootStatus::ThrowSoftwareNeedsUpgrade(void) const {
  QString software_upgrade_msg = kh::tr(
"This machine is running fusion software version %1.\n"
"%2 is configured to use version %3.\n"
"In order to use %4 from this machine, you must upgrade\n"
"the fusion software on this machine to version %5.\n");
  throw khException(software_upgrade_msg
                    .arg(GEE_VERSION)
                    .arg(assetroot_.c_str())
                    .arg(version_.c_str())
                    .arg(assetroot_.c_str())
                    .arg(version_.c_str())
                    );
}






void
AssetRootStatus::ValidateForDaemonCheck(void) const {
  QString not_set_msg = kh::tr(
"No asset root has been selected.\n"
"You must run one of the following commands:\n"
"  geselectassetroot --assetroot <valid asset root path>\n"
"  geconfigureassetroot --new [--assetroot <new asset root path>]\n");
  QString not_root_msg = kh::tr(
"%1 %2.\n"
"You must run one of the following commands:\n"
"  geselectassetroot --assetroot <valid asset root path>\n"
"  geconfigureassetroot --new --assetroot %3\n");


  if (assetroot_.empty()) {
    throw khException(not_set_msg);
  }

  if (!dir_exists_) {
    throw khException(not_root_msg
                      .arg(assetroot_.c_str())
                      .arg(kh::tr("doesn't exist"))
                      .arg(assetroot_.c_str())
                      );
  }

  if (!has_volumes_) {
    throw khException(not_root_msg
                      .arg(assetroot_.c_str())
                      .arg(kh::tr("isn't a valid asset root"))
                      .arg(assetroot_.c_str())
                      );
  }


  if (AssetRootNeedsUpgrade()) {
    ThrowAssetRootNeedsUpgrade();
  } else if (SoftwareNeedsUpgrade()) {
    ThrowSoftwareNeedsUpgrade();
  }
  if (AssetRootNeedsRepair()) {
    ThrowAssetRootNeedsRepair();
  }
}

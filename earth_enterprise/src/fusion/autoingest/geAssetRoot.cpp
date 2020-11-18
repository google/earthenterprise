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

#include "geAssetRoot.h"
#include <fusionversion.h>
#include <khFileUtils.h>
#include <khException.h>

namespace geAssetRoot {

const char VolumeName[] = "assetroot";

struct WantPerms {
  std::string name_;
  int         perms_;
};


// ****************************************************************************
// ***  Special Directories
// ****************************************************************************

// Both special dirs structs must be in same order as SpecialDirs enum
// These are the standard permissions
const WantPerms special_dirs[] = {
    {"",           0755},
    {".config",    0777},
    {".state",     0755},
    {".userdata",  0777},
    {".privatedb", 0700}
};

// These are the secure permissions
const WantPerms secure_special_dirs[] = {
    {"",           0755},
    {".config",    0775},
    {".state",     0755},
    {".userdata",  0775},
    {".privatedb", 0700}
};

const unsigned int num_special_dirs = sizeof(special_dirs)/sizeof(special_dirs[0]);

std::string Dirname(const std::string& assetroot, SpecialDir dir) {
  return khComposePath(assetroot, special_dirs[dir].name_);
}

int DirPerms(SpecialDir dir) {
  return special_dirs[dir].perms_;
}

int SecureDirPerms(SpecialDir dir) {
  return secure_special_dirs[dir].perms_;
}

// ****************************************************************************
// ***  Special Files
// ****************************************************************************

// must be in same order as SpecialFiles enum
const WantPerms special_files[] = {
  {".config/.fusionversion",        0644},
  {".config/volumes.xml",           0644},
  {".privatedb/FusionUniqueId.xml", 0600},
  {".state/.active",                0644}
};
const unsigned int num_special_files = sizeof(special_files)/sizeof(special_files[0]);

std::string Filename(const std::string& assetroot, SpecialFile file) {
  return khComposePath(assetroot, special_files[file].name_);
}
int FilePerms(SpecialFile file) {
  return special_files[file].perms_;
}


// ****************************************************************************
// ***  Version routines
// ****************************************************************************
std::string ReadVersion(const std::string &assetroot) {
  const std::string filename = Filename(assetroot, VersionFile);
  std::string version;
  if (khExists(filename) && khReadStringFromFile(filename, version)) {
    return version;
  }
  return std::string();
}

void WriteVersion(const std::string &assetroot) {
  const std::string filename = Filename(assetroot, VersionFile);
  if (!khWriteStringToFile(filename, GEE_VERSION)) {
    throw khException(kh::tr("Unable to write %1").arg(filename.c_str()));
  }
  khChmod(filename, FilePerms(VersionFile));
}

void AssertVersion(const std::string &assetroot) {
  const std::string version = ReadVersion(assetroot);
  const std::string fusionVer = GEE_VERSION;

  // NOTE: the exception below should never trigger since we call
  // gefdaemoncheck before launching the daemons.
  // This routine is called from the daemons just to be safe.
  if (version != fusionVer) {
    throw khException(kh::tr(
"This machine is running fusion software version %1.\n"
"%2 is configured to use version %3.\n"
"Unable to proceed.")
                      .arg(fusionVer.c_str())
                      .arg(assetroot.c_str())
                      .arg(version.c_str()));
  }
}



} // namespace geAssetRoot

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


#ifndef FUSION_AUTOINGEST_GEASSETROOT_H__
#define FUSION_AUTOINGEST_GEASSETROOT_H__

#include <string>

namespace geAssetRoot {



extern const char VolumeName[];

// ****************************************************************************
// ***  Special directories
// ****************************************************************************
enum SpecialDir {
  RootDir,       FirstSpecialDir = RootDir,
  ConfigDir,
  StateDir,
  UserDataDir,
  PrivateDBDir,  LastSpecialDir  = PrivateDBDir
};
extern std::string Dirname(const std::string& assetroot, SpecialDir dir);
extern int DirPerms(SpecialDir dir);
extern int SecureDirPerms(SpecialDir dir);


// ****************************************************************************
// ***  Special files
// ****************************************************************************
enum SpecialFile {
  VersionFile,   FirstSpecialFile = VersionFile,
  VolumeFile,
  UniqueIdFile,
  ActiveFile,    LastSpecialFile  = ActiveFile
};
extern std::string Filename(const std::string& assetroot, SpecialFile file);
extern int FilePerms(SpecialFile file);


// ****************************************************************************
// ***  Version routines
// ****************************************************************************
extern std::string ReadVersion(const std::string &assetroot);

// only configure and upgrade tools should call this
// will throw on error
extern void WriteVersion(const std::string &assetroot);

// fusion daemons will call this
void AssertVersion(const std::string &assetroot);




} // namespace geAssetRoot


#endif // FUSION_AUTOINGEST_GEASSETROOT_H__

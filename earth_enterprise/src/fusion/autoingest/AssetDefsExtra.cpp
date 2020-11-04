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


#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/.idl/Systemrc.h>
#include <notify.h>
#include <khConstants.h>
#include <khFileUtils.h>
#include <khException.h>
#include <khGuard.h>
#include "khVolumeManager.h"
#include "AssetVersionRef.h"
#include "Asset.h"
#include <khstl.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <geAssetRoot.h>

const std::string&
AssetDefs::AssetRoot(void)
{
  return Systemrc::AssetRoot();
}

std::string
AssetDefs::MasterHostName(void)
{
  return theVolumeManager.VolumeHost(geAssetRoot::VolumeName);
}

std::string
AssetDefs::AssetPathToFilename(const std::string &assetpath)
{
  if (assetpath.empty() || (assetpath[0] == '/') || khIsURI(assetpath))
    return assetpath;
  return khComposePath(AssetDefs::AssetRoot(), assetpath);
}


std::string
AssetDefs::FilenameToAssetPath(const std::string &filepath)
{
  std::string basedir = AssetDefs::AssetRoot()+"/";
  // find prefix that matches the asset root & strip it off
  if (StartsWith(filepath, basedir)) {
    if (filepath.size() > basedir.size()) {
      return filepath.substr(basedir.size(), std::string::npos);
    } else {
      // filepath is exactly AssetPath() + "/"
      // since that would result in an empty assetpath (which is
      // invalid), re just return the full filepath
      return filepath;
    }
  } else {
    return filepath;
  }
}

std::string
AssetDefs::NormalizeAssetName(const std::string &name,
                              Type type,
                              const std::string &subtype)
{
  std::string assetname = name;
  if (assetname.empty()) {
    throw khException(kh::tr("Empty asset name"));
  }
  if (assetname[0] == '/') {
    throw khException(kh::tr("Invalid asset name '%1'. Starts with /")
                      .arg(assetname.c_str()));
  }

  if (!AssetDefs::ValidateAssetName(assetname)) {
    throw khException(kh::tr("The asset name you specified:\n  %1\n"
                             "contains at least one invalid character.\n"
                             "The following characters are not allowed: \n"
                             "    & % ' \\ \" * = + ~ ` ? < > : ; and the "
                             "space character.\n\n"
                             "For pre-existing assets, please see the\n"
                             "GEE Admin Guide for steps to resolve the\n"
                             "issue(s).\n").arg(assetname.c_str()));
  }
  //
  // Remove double(or multiple) fwd slashes(//) from pathname
  //
  std::size_t found = 0;
  found = assetname.find("//");
  while (found != std::string::npos) {
    assetname.erase(assetname.begin()+found);
    found = assetname.find("//" , found);
  }
  //
  // Remove last fwd single slash from pathname
  //
  if (assetname[assetname.size()-1] == '/') {
    assetname.resize(assetname.size()-1);
  }
  return khEnsureExtension(assetname, FileExtension(type, subtype));
}

bool AssetDefs::ValidateAssetName(const std::string& name) {
  // TODO: if efficiency is an issue here, we can make an LUT
  // for the character validity tests.
  // Need to check for invalid characters, except a version identifier suffix
  // which is valid as an assetname.
  unsigned int length_to_check = name.size();
  const std::string kVersionSuffix = "?version=";
  std::size_t pos = name.find(kVersionSuffix);
  if (pos != std::string::npos) {
    length_to_check = pos;  // No need to check the version spec when checking
                            // for invalid characters.

    // Check all characters beyond the "=" character...they must be numerals.
    bool found_numeral = false;

    for(unsigned int i = length_to_check + kVersionSuffix.size();
        i < name.size(); ++i) {
      char c = name[i];

      if (c < '0' || c > '9') {
        return false;  // Required to end in a numeral if version suffix is used.
      }
      found_numeral = true;
    }

    // If there's no numeral at the end, it's an invalid name.
    if (!found_numeral) {
      return false;
    }
  }

  const std::string kInvalidAssetNameCharacters("&%'\" \\*=+~`?<>;:");
  for(uint i = 0; i < length_to_check; ++i) {
      if (kInvalidAssetNameCharacters.find(name[i]) != std::string::npos) {
          return false;
      }
  }
  return true;
}

bool
AssetDefs::HaveAssetFile(const std::string &ref)
{
  return (ref.size() &&
          (ref[0] != '/') &&
          khExists(AssetImpl::XMLFilename(ref)));
}

std::string
AssetDefs::GuessAssetRef(const std::string &name)
{
  if (name.empty()) {
    throw khException(kh::tr("Empty asset name"));
  }
  if (name[0] == '/') {
    throw khException(kh::tr("Invalid asset name '%1'. Starts with '/'")
                      .arg(name.c_str()));
  }

  // check to see if it's already fully qualified
  if (HaveAssetFile(name)) {
    return name;
  }

  // now start guessing
  std::string base = khBasename(name);
  std::string dirname = khDirname(name);
  std::string path;
  if (dirname == ".") {
    dirname = "";
    path = AssetRoot();
  } else {
    path = AssetPathToFilename(dirname);
    dirname += "/";
  }

  std::vector<std::string> matches;
  {
    DIR *dir = opendir(path.c_str());
    if (!dir) {
      throw khException(kh::tr("No such asset: '%1'").arg(name.c_str()));
    }
    khDIRCloser closer(dir);

    struct dirent64 *entry;
    while ((entry = readdir64(dir))) {
      if (strcmp(entry->d_name, ".") == 0) continue;
      if (strcmp(entry->d_name, "..") == 0) continue;
      std::string fname = entry->d_name;
      if (base == khDropExtension(fname)) {
        std::string ref = dirname + fname;
        if (HaveAssetFile(ref)) {
          matches.push_back(ref);
        }
      }
    }
  }
  if (matches.empty()) {
    throw khException(kh::tr("No such asset: '%1'").arg(name.c_str()));
  } else if (matches.size() > 1) {
    throw khException(kh::tr("Asset '%1' is ambiguous:\n").arg(name.c_str()) +
                      join(matches.begin(), matches.end(), "\n").c_str());
  }
  return matches[0];
}

AssetVersionRef
AssetDefs::GuessAssetVersionRef(const std::string &name,
                                const std::string &version)
{
  AssetVersionRef verref = version.empty() ?
                           AssetVersionRef(name) : AssetVersionRef(name, version);

  return AssetVersionRef(GuessAssetRef(verref.AssetRef()), verref.Version());
}


std::string
AssetDefs::FileExtension(Type type, const std::string &subtype)
{
  std::string prefix;

  switch (type) {
    case AssetDefs::Invalid:
      return std::string();
    case AssetDefs::Database:
      if (subtype == kDatabaseSubtype)
        return kDatabaseSuffix;
      else if (subtype == kMapDatabaseSubtype)
        return kMapDatabaseSuffix;
      else if (subtype == kMercatorMapDatabaseSubtype)
        return kMercatorMapDatabaseSuffix;
      else
        return kDbKeySuffix;
    case AssetDefs::Vector:
      prefix = ".kv";
      break;
    case AssetDefs::Imagery:
      prefix = ".ki";
      break;
    case AssetDefs::Terrain:
      prefix = ".kt";
      break;
    case AssetDefs::Map:
      prefix = ".km";
      break;
    case AssetDefs::KML:
      prefix = ".kml";
      break;
  }
  if (subtype == "Source")
    return prefix + "source";
  else if (subtype == kProductSubtype)
    return prefix + "asset";
  else if (subtype == kMercatorProductSubtype)
    return prefix + "masset";
  else if (subtype == kProjectSubtype)
    return prefix + "project";
  else if (subtype == kMercatorProjectSubtype)
    return prefix + "mproject";
  else if (subtype == kLayer)
    return prefix + "layer";
  else
    return prefix + "a";
}


std::string
AssetDefs::SubAssetName(const std::string &parentAssetRef,
                        const std::string &basename,
                        Type type,
                        const std::string &subtype)
{
  return khComposePath(parentAssetRef,
                       basename + FileExtension(type, subtype));
}

void
AssetDefs::OverrideAssetRoot(const std::string &assetroot)
{
  Systemrc::OverrideAssetRoot(assetroot);
}

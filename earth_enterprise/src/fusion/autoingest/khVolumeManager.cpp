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


#include "khVolumeManager.h"
#include <notify.h>
#include <khstl.h>
#include <khFileUtils.h>
#include <khException.h>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/geAssetRoot.h>

// for khHostname
#include <khSpawn.h>


static bool
CheckAndNormalizePath(const std::string &desc, std::string &path)
{
  if (!khIsAbspath(path)) {
    notify(NFY_WARN, "%s ('%s') is not an absolute path",
           desc.c_str(), path.c_str());
    return false;
  }

  // trim any trailing '/'
  while ((path.size() > 1) && path[path.size()-1] == '/') {
    path.erase(path.size()-1, 1);
  }

  // check after trimming /'s from end
  if (path == "/") {
    notify(NFY_WARN, "%s may not be \"/\"", desc.c_str());
  }

  return true;
}


// ****************************************************************************
// ***  khVolumeManager
// ****************************************************************************
khVolumeManager::khVolumeManager(void)
{
}

void
khVolumeManager::Init(void)
{
  try {
    if (!khExists(VolumeListFilename())) {
      throw khException(kh::tr("'%1' doesn't exist")
                        .arg(VolumeListFilename().c_str()));
    }


    if (!Load(VolumeListFilename())) {
      throw khException(kh::tr("Load error"));
    }

    // Make sure all the paths are valid
    bool volsok = true;
    for (VolumeDefMap::iterator v = volumedefs.begin();
         v != volumedefs.end(); ++v) {
      if (!CheckAndNormalizePath(v->first + " netpath", v->second.netpath)) {
        volsok = false;
      }
      if (!CheckAndNormalizePath(v->first + " localpath",
                                 v->second.localpath)) {
        volsok = false;
      }
    }

    // Make sure we have an asset root
    VolumeDefMap::const_iterator found =
      volumedefs.find(geAssetRoot::VolumeName);
    if (found == volumedefs.end()) {
      notify(NFY_WARN, "Cannot find volume definition for \"%s\"",
             geAssetRoot::VolumeName);
      volsok = false;
    } else if (AssetDefs::AssetRoot() != found->second.netpath) {
      notify(NFY_WARN,
             "\"%s\" volume's netpath must match systemrc's assetroot",
             geAssetRoot::VolumeName);
      volsok = false;
    }

    // bail if there were problems
    if (!volsok) {
      throw khException(kh::tr("Error in volume definitions."));
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "Unable to load volumes: %s\n"
           "Please have your system administrator run the following command:\n"
           "  geconfigureassetroot --new --assetroot %s",
           e.what(), AssetDefs::AssetRoot().c_str());

  } catch (...) {
    notify(NFY_FATAL, "Unable to load volumes: %s\n"
           "Please have your system administrator run the following command:\n"
           "  geconfigureassetroot --new --assetroot %s",
           "Unknown error", AssetDefs::AssetRoot().c_str());
  }
}


// ****************************************************************************
// ***  NetworkPathOf
// ****************************************************************************
std::string
khVolumeManager::NetworkPathOf(const khFusionURI &uri) const
{
  if (volumedefs.empty()) {
    const_cast<khVolumeManager*>(this)->Init();
  }
  std::string targetVol = uri.Volume();
  VolumeDefMap::const_iterator found = volumedefs.find(targetVol);
  if (found != volumedefs.end()) {
    return found->second.netpath + "/" + uri.Path();
  }
  return std::string();
}


// ****************************************************************************
// ***  LocalPathOf
// ****************************************************************************
std::string
khVolumeManager::LocalPathOf(const khFusionURI &uri) const
{
  if (volumedefs.empty()) {
    const_cast<khVolumeManager*>(this)->Init();
  }
  std::string targetVol = uri.Volume();
  VolumeDefMap::const_iterator found = volumedefs.find(targetVol);
  if (found != volumedefs.end()) {
    return found->second.localpath + "/" + uri.Path();
  }
  return std::string();
}


// ****************************************************************************
// ***  DeduceURIFromPath
// ****************************************************************************
khFusionURI
khVolumeManager::DeduceURIFromPath(const std::string &path) const
{
  if (volumedefs.empty()) {
    const_cast<khVolumeManager*>(this)->Init();
  }
  std::string thishost = khHostname();

  for (VolumeDefMap::const_iterator i = volumedefs.begin();
       i != volumedefs.end(); ++i) {
    std::string volname = i->first;
    const VolumeDef &vol(i->second);
    // does the path match this volume's netpath?
    if ((path.size() > vol.netpath.size() + 1) &&
        StartsWith(path, vol.netpath + "/")) {
      return khFusionURI(volname, path.substr(vol.netpath.size()+1));
    }
    // is this a local volume? does the path match the localpath?
    if ((vol.host == thishost) &&
        (path.size() > vol.localpath.size() + 1) &&
        StartsWith(path, vol.localpath + "/")) {
      return khFusionURI(volname,
                         path.substr(vol.localpath.size()+1));
    }
  }

  return khFusionURI();
}



// ****************************************************************************
// ***  GetLocalTmpVolumes
// ****************************************************************************
void
khVolumeManager::GetLocalTmpVolumes(const std::string &host,
                                    std::vector<std::string> &volnames) const
{
  if (volumedefs.empty()) {
    const_cast<khVolumeManager*>(this)->Init();
  }
  for (VolumeDefMap::const_iterator i = volumedefs.begin();
       i != volumedefs.end(); ++i) {
    if (i->second.isTmp && (host == i->second.host)) {
      volnames.push_back(i->first);
    }
  }
}

// ****************************************************************************
// ***  GetHostVolumes
// ****************************************************************************
void
khVolumeManager::GetHostVolumes(const std::string &host,
                                std::vector<std::string> &volnames) const
{
  if (volumedefs.empty()) {
    const_cast<khVolumeManager*>(this)->Init();
  }
  for (VolumeDefMap::const_iterator i = volumedefs.begin();
       i != volumedefs.end(); ++i) {
    if (host == i->second.host) {
      volnames.push_back(i->first);
    }
  }
}


// ****************************************************************************
// ***  GetRemoteTmpVolumes
// ****************************************************************************
void
khVolumeManager::GetRemoteTmpVolumes(const std::string &host,
                                     std::vector<std::string> &volnames) const
{
  if (volumedefs.empty()) {
    const_cast<khVolumeManager*>(this)->Init();
  }
  for (VolumeDefMap::const_iterator i = volumedefs.begin();
       i != volumedefs.end(); ++i) {
    if (i->second.isTmp && (host != i->second.host)) {
      volnames.push_back(i->first);
    }
  }
}

std::string
khVolumeManager::VolumeHost(const std::string &volname) const
{
  if (volumedefs.empty()) {
    const_cast<khVolumeManager*>(this)->Init();
  }
  VolumeDefMap::const_iterator found = volumedefs.find(volname);
  if (found != volumedefs.end()) {
    return found->second.host;
  } else {
    return std::string();
  }
}

const VolumeDefList::VolumeDef*
khVolumeManager::GetVolumeDef(const std::string &volname) const
{
  if (volumedefs.empty()) {
    const_cast<khVolumeManager*>(this)->Init();
  }
  VolumeDefMap::const_iterator found = volumedefs.find(volname);
  if (found != volumedefs.end()) {
    return &found->second;
  } else {
    return 0;
  }
}


// ****************************************************************************
// ***  static methods
// ****************************************************************************
std::string
khVolumeManager::VolumeListFilename(void)
{
  return geAssetRoot::Filename(AssetDefs::AssetRoot(),
                               geAssetRoot::VolumeFile);
}

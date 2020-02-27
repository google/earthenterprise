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


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <fcntl.h>

#include <qstring.h>

#include <gstAssetManager.h>
#include <gstSource.h>
#include <gstKVPFile.h>
#include <gstKVPTable.h>
#include <autoingest/Asset.h>
#include <autoingest/khAssetManagerProxy.h>
#include <khException.h>
#include <khFileUtils.h>


// -----------------------------------------------------------------------------

gstAssetManager::gstAssetManager(const char* n)
    : assetroot(n) {
}

gstAssetManager::~gstAssetManager() {
}

bool gstAssetManager::buildAsset(const gstAssetHandle& handle, QString& error) {
  if (!handle->isValid()) {
    error = kh::tr("No asset specified");
    return false;
  }

  bool needed;
  bool success = khAssetManagerProxy::BuildAsset(handle->getAsset()->GetRef(),
                                                 needed, error);
  if (success && !needed) {
    error = kh::tr("Nothing to do. Already up to date.");
    success = false;
  }
  return success;
}

bool gstAssetManager::rebuildAsset(const gstAssetHandle& handle,
                                   QString& error) {
  if (!handle->isValid()) {
    error = kh::tr("No asset specified");
    return false;
  }

  AssetVersion ver(handle->getAsset()->CurrVersionRef());
  if (ver) {
    return khAssetManagerProxy::RebuildVersion(ver->GetRef(), error);
  } else {
    error = kh::tr("No current version");
    return false;
  }
}


bool gstAssetManager::cancelAsset(const gstAssetHandle& handle,
                                  QString& error) {
  if (!handle->isValid()) {
    error = kh::tr("No asset specified");
    return false;
  }

  AssetVersion ver(handle->getAsset()->CurrVersionRef());
  if (ver) {
    return khAssetManagerProxy::CancelVersion(ver->GetRef(), error);
  } else {
    error = kh::tr("No current version");
    return false;
  }
}

bool gstAssetManager::ImportVector(const std::string& folder,
                                   const VectorProductImportRequest& asset,
                                   QString& msg) {
  VectorProductImportRequest req(asset);

  req.assetname =
    AssetDefs::FilenameToAssetPath(folder + "/" + asset.assetname);

  if (!khAssetManagerProxy::VectorProductImport(req, msg)) {
    notify(NFY_WARN,
           "Error importing vector asset named %s into directory %s\n%s\n",
           asset.assetname.c_str(), folder.c_str(),
           msg.latin1());
    return false;
  }

  return true;
}

bool gstAssetManager::ImportRaster(const std::string& folder,
                                   const RasterProductImportRequest& asset,
                                   QString& msg) {
  RasterProductImportRequest req(asset);

  req.assetname =
    AssetDefs::FilenameToAssetPath(folder + "/" + asset.assetname);

  if (!khAssetManagerProxy::RasterProductImport(req, msg)) {
    notify(NFY_WARN,
           "Error importing raster asset named %s into directory %s\n%s\n",
           asset.assetname.c_str(), folder.c_str(),
           msg.latin1());
    return false;
  }

  return true;
}


bool gstAssetManager::makeDir(const std::string& assetdir, QString &error)
{
  if (khExists(AssetDefs::AssetPathToFilename(assetdir))) {
    error = kh::tr("'%1' already exists").arg(assetdir.c_str());
    return false;
  }

  return khAssetManagerProxy::MakeAssetDir(assetdir, error);
}

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
#include <qregexp.h>
#include <gstAssetManager.h>
#include <khConstants.h>

#include "gstAssetGroup.h"

QString shortAssetName(const QString& n) {
  QString sname = n;
  QStringList suffix;
  suffix << kVectorAssetSuffix << kImageryAssetSuffix
         << kMercatorImageryAssetSuffix << kTerrainAssetSuffix
         << kVectorProjectSuffix << kImageryProjectSuffix
         << kMercatorImageryProjectSuffix << kTerrainProjectSuffix
         << kDatabaseSuffix << kMapLayerSuffix << kMapProjectSuffix
         << kMapDatabaseSuffix << kMercatorMapDatabaseSuffix
         << kVectorLayerSuffix;
  for (QStringList::Iterator it = suffix.begin(); it != suffix.end(); ++it) {
    if (sname.endsWith(*it)) {
      sname.truncate(sname.length() - (*it).length());
      break;
    }
  }
  return sname;
}

bool isAssetPath(const QString& str) {
  static QRegExp rx(".*\\.k[vit](asset|project)$|"
                    ".*\\.k[i](masset|mproject)$|"
                    ".*\\.kdatabase$|"
                    ".*\\.km(layer|project|database|mdatabase)|"
                    ".*\\.kvlayer", true, false);
  return rx.exactMatch(str);
}

// -----------------------------------------------------------------------------

gstAssetHandleImpl::gstAssetHandleImpl(const gstAssetFolder& f,
                                       const QString& n)
    : parent_folder_(f), name_(n) {
}

gstAssetHandleImpl::~gstAssetHandleImpl() {
}

const gstAssetFolder &gstAssetHandleImpl::getParentFolder() const {
  return parent_folder_;
}

bool gstAssetHandleImpl::isValid() const {
  return (parent_folder_.isValid() &&
          AssetDefs::HaveAssetFile(std::string(relativePath().latin1())) &&
          getAsset()->type != AssetDefs::Invalid);
}

Asset gstAssetHandleImpl::getAsset() const {
  return Asset(relativePath().latin1());
}

QString gstAssetHandleImpl::getName() const {
  return name_;
}

QString gstAssetHandleImpl::relativePath() const {
  QString path;

  if (!parent_folder_.relativePath().isEmpty())
    path = parent_folder_.relativePath() + QChar('/');

  path += name_;

  return path;
}

QString gstAssetHandleImpl::fullPath() const {
  return parent_folder_.fullPath() + QChar('/') + name_;
}


// -----------------------------------------------------------------------------

gstAssetFolder::gstAssetFolder(const QString& n)
    : dir_name_(n) {
}

QString gstAssetFolder::name() const {
  return QDir(dir_name_).dirName();
}

gstAssetFolder gstAssetFolder::getParentFolder() const {
  QDir dir(dir_name_);
  dir.cdUp();
  return gstAssetFolder(dir.absPath());
}

gstAssetFolder gstAssetFolder::getChildFolder(const QString& name) const {
  return gstAssetFolder(dir_name_ + "/" + name);
}

QString gstAssetFolder::relativePath() const {
  QString assetroot(AssetDefs::AssetRoot().c_str());
  QString relative = fullPath();
  if (relative.startsWith(assetroot))
    relative = relative.remove(0, assetroot.length());
  if (relative.startsWith("/"))
    relative = relative.remove(0, 1);
  return relative;
}

QString gstAssetFolder::fullPath() const {
  return dir_name_;
}

std::vector<gstAssetHandle> gstAssetFolder::getAssetHandles() const {
  std::vector<gstAssetHandle> list;

  QDir dir(dir_name_);
  dir.setNameFilter(
      "*.k?asset;*.kimasset;*.k?project;*.kimproject;*.kdatabase;"
      "*.kmlayer;*.kmproject;*.kmdatabase;*.kmmdatabase;*.kvlayer");
  QStringList files = dir.entryList(QDir::Dirs, QDir::Name | QDir::IgnoreCase);
  for (QStringList::Iterator it = files.begin(); it != files.end(); ++it) {
    gstAssetHandle handle = gstAssetHandleImpl::Create(*this, *it);
    if (handle->isValid()) {
      list.push_back(handle);
    } else {
      notify(NFY_DEBUG, "Invalid asset found! dirname=%s file=%s",
             dir_name_.latin1(), (*it).latin1());
    }
  }

  return list;
}

std::vector<gstAssetFolder> gstAssetFolder::getAssetFolders() const {
  std::vector<gstAssetFolder> list;

  QDir dir(dir_name_);
  QStringList files = dir.entryList(QDir::Dirs, QDir::Name | QDir::IgnoreCase);
  for (QStringList::Iterator it = files.begin(); it != files.end(); ++it) {
    // do not add if this is an asset, or if it starts with a dot ('.')
    if (!isAssetPath(*it) && !(*it).startsWith("."))
      list.push_back(gstAssetFolder(dir.filePath(*it)));
  }

  return list;
}


bool gstAssetFolder::newFolder(const QString& name,
                               QString &error) const {
  QDir dir(dir_name_);

  return theAssetManager->makeDir
    (AssetDefs::FilenameToAssetPath(dir.filePath(name).latin1()), error);
}

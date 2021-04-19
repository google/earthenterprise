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
#include <Qt/qregexp.h>
#include <gstAssetManager.h>
#include <khConstants.h>
#include "gstAssetGroup.h"


std::string shortAssetName(const char* n) {

  static const std::vector<std::string> suffixes =
  {
      kVectorAssetSuffix, kImageryAssetSuffix, kMercatorImageryAssetSuffix,
      kTerrainAssetSuffix, kVectorProjectSuffix, kImageryProjectSuffix,
      kMercatorImageryProjectSuffix, kTerrainProjectSuffix,
      kDatabaseSuffix, kMapLayerSuffix, kMapProjectSuffix,
      kMapDatabaseSuffix, kMercatorMapDatabaseSuffix, kVectorLayerSuffix
  };

  std::string saname { n };

 /*
	code takes in a long asset name, checks to see if it has one
	of the appropriate suffixes (e.g. .kiasset, kmmdatabase, .kmlayer, etc)
	if it is found, it strils off the suffix and returns the short name,
	if a known suffix is not found, the long asset name will be returned

	i.e. shortAssetName("AnAssetName.kiasset") will return "AnAssetName",
	     shortAssetName("AnAssetName.notvalid") will return "AnAssetName.notvalid"
 */
  for (const auto& elem : suffixes)
  {
      auto pos = saname.rfind(elem);
      if (pos != std::string::npos)
      {
          saname = saname.substr(0,pos);
          break;
      }
  }

  return saname;
}

std::string shortAssetName(const std::string& str)
{
    return shortAssetName(str.c_str());
}

std::string shortAssetName(const QString& str)
{
    return shortAssetName(str.toStdString().c_str());
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
  std::string rpath { relativePath().toStdString() };
  return Asset(rpath);
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
             dir_name_.latin1(), it->latin1());
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
    if (!isAssetPath(*it) && !it->startsWith("."))
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

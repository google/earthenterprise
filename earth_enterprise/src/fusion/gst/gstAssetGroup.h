/*
 * Copyright 2017 Google Inc.
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

#ifndef KHSRC_FUSION_GST_GSTASSETGROUP_H__
#define KHSRC_FUSION_GST_GSTASSETGROUP_H__

#include <vector>
#include <Qt/qglobal.h>
#include <Qt/qstring.h>
#include <Qt/qstringlist.h>
#include <Qt/qdir.h>
#include <khRefCounter.h>

#include <autoingest/Asset.h>
#include <autoingest/AssetVersion.h>

class gstAssetHandleImpl;

typedef khRefGuard<gstAssetHandleImpl> gstAssetHandle;

std::string shortAssetName(const char*);
std::string shortAssetName(const std::string&);
std::string shortAssetName(const QString&);
bool isAssetPath(const QString&);

// -----------------------------------------------------------------------------

class gstAssetFolder {
 public:
  gstAssetFolder(const QString& name);

  QString name() const;
  QString relativePath() const;
  QString fullPath() const;

  std::vector<gstAssetHandle> getAssetHandles() const;
  std::vector<gstAssetFolder> getAssetFolders() const;

  gstAssetFolder getParentFolder() const;
  gstAssetFolder getChildFolder(const QString&) const;

  bool newFolder(const QString&, QString&) const;

  bool isValid() const { return !dir_name_.isEmpty(); }

 private:
  QString dir_name_;
};

// -----------------------------------------------------------------------------

class gstAssetHandleImpl : public khRefCounter {
 public:
  static inline gstAssetHandle Create(const gstAssetFolder& p,const QString& n) {
    return khRefGuardFromNew(new gstAssetHandleImpl(p, n));
  }
  virtual ~gstAssetHandleImpl();

  QString getName() const;
  QString fullPath() const;

  Asset getAsset() const;

  const gstAssetFolder& getParentFolder() const;

  bool isValid() const;

 private:
  gstAssetHandleImpl(const gstAssetFolder&, const QString&);
  QString relativePath() const;

  gstAssetFolder parent_folder_;
  QString name_;
};

#endif  // !KHSRC_FUSION_GST_GSTASSETGROUP_H__

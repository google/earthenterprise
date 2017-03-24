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

#ifndef FUSION_AUTOINGEST_ASSETTRAVERSER_H__
#define FUSION_AUTOINGEST_ASSETTRAVERSER_H__

#include <string>
#include <set>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/Asset.h>
#include <sys/stat.h>

struct stat64;


class AssetTraverser {
 public:
  AssetTraverser(void);
  virtual ~AssetTraverser(void);
  void RequestType(AssetDefs::Type type, const std::string &subtype);
  void Traverse(void);

 protected:

  typedef std::pair<AssetDefs::Type, std::string> WantType;
  typedef std::set<WantType> WantSet;
  typedef std::set<std::string> ExtensionSet;
  typedef std::pair<dev_t, ino_t> DirId;

  virtual void HandleAsset(Asset &asset) = 0;
  void ContinueSearch(int fd,
                      const std::string &parent,
                      const std::string &relpath,
                      const std::string &restorepath);
  void HandleAssetFile(const std::string &dir, const std::string &fname);

  DirId GetDirId(const struct stat64 &stat_info);

  const std::string assetroot_;
  ExtensionSet WantExtensions;
  ExtensionSet IgnoreExtensions;
  WantSet      wanted_;
  std::set<DirId> visited_dirs_;
};


#endif // FUSION_AUTOINGEST_ASSETTRAVERSER_H__

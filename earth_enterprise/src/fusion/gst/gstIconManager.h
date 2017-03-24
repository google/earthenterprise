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

#ifndef KHSRC_FUSION_GST_GSTICONMANAGER_H__
#define KHSRC_FUSION_GST_GSTICONMANAGER_H__

#include <qstring.h>
#include <gstTypes.h>
#include <autoingest/.idl/storage/LayerConfig.h>

// Valid icon types are
//   IconReference::Internal
//   IconReference::External

class gstIcon {
 public:
  gstIcon(const QString& href, IconReference::Type t);
  gstIcon(const IconReference& o) : config(o.type, o.href) { }
  gstIcon(const IconConfig& cfg) : config(cfg) { }
  gstIcon() : config(IconReference::Internal, "771") { }

  gstIcon(const gstIcon& from) { config = from.getConfig(); }

  QString href() const { return config.href; }
  IconReference::Type type(void) const { return config.type; }
  IconReference IconRef(void) const {
    return IconReference(config.type, config.href);
  }

  IconConfig getConfig() const { return config; }

  bool isInternal() const {
    return type() == IconReference::Internal;
  }

  // for equality we just need to look for name and type
  // this is coz the only part of the code that calls for
  // == on gstIcon is the pixhash in pixmapmanager
  bool operator==(const gstIcon& o) const {
    return (config.href == o.getConfig().href &&
            config.type == o.getConfig().type);
  }

  bool operator!=(const gstIcon &o) const {
    return !operator==( o );
  }

  // needed since gstIcon is used as a std::map key
  bool operator<(const gstIcon& that) const {
    return config.href < that.getConfig().href;
  }

 private:
  IconConfig config;
};

class gstIconManager {
 public:
  gstIconManager(const std::string& path);
  ~gstIconManager();

  static void init(const std::string& path);

  bool AddIcon(const std::string& path);
  bool Validate(const std::string& path);
  bool DeleteIcon(const std::string& path);

  int IconCount(IconReference::Type c) const {
    return collection_[c].size();
  }

  const gstIcon& GetIcon(IconReference::Type c, int idx) const {
    return collection_[c][idx];
  }

  std::string GetFullPath(IconReference::Type c, int idx) const;

  void clearIcons(IconReference::Type c);

 private:
  std::string icon_path_[2];
  std::vector<gstIcon> collection_[2];

  void SetPath(IconReference::Type, const std::string& path);
  const std::string& GetPath(IconReference::Type) const;
  bool CopyIcon(const std::string& src, const std::string& dst);
};

extern gstIconManager* theIconManager;

#endif  // !KHSRC_FUSION_GST_GSTICONMANAGER_H__

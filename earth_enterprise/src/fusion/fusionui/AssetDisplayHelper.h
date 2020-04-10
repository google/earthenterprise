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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_ASSETDISPLAYHELPER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_ASSETDISPLAYHELPER_H_

#include <string>
#include <Qt/qimage.h>

#include <autoingest/.idl/storage/AssetDefs.h>
#include <Qt/qpixmap.h>

class AssetDisplayHelper {
 public:
  // Be careful if you rearrange this enum. There are arrays defined in the
  // .cpp that rely on this order.
  enum AssetKey {
    Key_Invalid,
    Key_VectorProduct,
    Key_VectorLayer,
    Key_VectorProject,
    Key_ImageryProduct,
    Key_MercatorImageryProduct,
    Key_ImageryProject,
    Key_MercatorImageryProject,
    Key_TerrainProduct,
    Key_TerrainProject,
    Key_Database,
    Key_MapLayer,
    Key_MapProject,
    Key_MapDatabase,
    Key_MercatorMapDatabase,
    Key_KMLProject,
    Key_Unknown
  };

  AssetDisplayHelper(AssetDefs::Type t, const std::string &st)
      : key_(MakeKey(t, st)) {
  }

  bool operator==(const AssetDisplayHelper& b) const {
    return key_ == b.key_;
  }

  bool operator!=(const AssetDisplayHelper& b) const {
    return !( *this == b );
  }

  inline AssetKey        GetKey() const { return key_; }
  inline unsigned int            GetSortKey() const { return key_; }
  inline AssetDefs::Type GetType()    const { return AssetType(key_); }
  inline std::string     GetSubType() const { return AssetSubType(key_); }
  inline QPixmap         GetPixmap()  const { return Pixmap(key_); }
  inline QString         PrettyName() const { return PrettyName(key_); }

  static AssetDefs::Type AssetType   (AssetKey key);
  static std::string     AssetSubType(AssetKey key);
  static QPixmap         Pixmap      (AssetKey key);
  static QString         PrettyName  (AssetKey key);
  static QPixmap         LoadPixmap  (const QString& name);

 private:
  static AssetKey MakeKey(AssetDefs::Type t, const std::string &st);

  AssetKey key_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_ASSETDISPLAYHELPER_H_

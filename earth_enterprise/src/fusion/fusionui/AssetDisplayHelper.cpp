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


#include <khConstants.h>
#include <QtGui/QMimeSource>
#include <Qt/q3mimefactory.h>
#include <Qt/qobject.h>
#include <Qt/q3dragobject.h>
#include <map>

#include "AssetDisplayHelper.h"

using QImageDrag = Q3ImageDrag;
using QMimeSourceFactory = Q3MimeSourceFactory;

AssetDisplayHelper::AssetKey AssetDisplayHelper::MakeKey(
    AssetDefs::Type t, const std::string &st) {
  std::string sub = ((st == "Asset") ||
                     (st == kResourceSubtype)) ? kProductSubtype : st;

  switch (t) {
    case AssetDefs::Invalid:
      return Key_Invalid;
    case AssetDefs::Vector:
      if (sub == kProductSubtype) {
        return Key_VectorProduct;
      } else if (sub == kLayer) {
        return Key_VectorLayer;
      } else if (sub == kProjectSubtype) {
        return Key_VectorProject;
      } else {
        return Key_Unknown;
      }
    case AssetDefs::Imagery:
      if (sub == kProductSubtype) {
        return Key_ImageryProduct;
      } else if (sub == kMercatorProductSubtype) {
        return Key_MercatorImageryProduct;
      } else if (sub == kProjectSubtype) {
        return Key_ImageryProject;
      } else if (sub == kMercatorProjectSubtype) {
        return Key_MercatorImageryProject;
      } else {
        return Key_Unknown;
      }
    case AssetDefs::Terrain:
      if (sub == kProductSubtype) {
        return Key_TerrainProduct;
      } else if (sub == kProjectSubtype) {
        return Key_TerrainProject;
      } else {
        return Key_Unknown;
      }
    case AssetDefs::Database:
      if (sub == kDatabaseSubtype) {
        return Key_Database;
      } else if (sub == kMapDatabaseSubtype) {
        return Key_MapDatabase;
      } else if (sub == kMercatorMapDatabaseSubtype) {
        return Key_MercatorMapDatabase;
      }  else {
        return Key_Unknown;
      }
    case AssetDefs::Map:
      if (sub == kLayer) {
        return Key_MapLayer;
      } else if (sub == kProjectSubtype) {
        return Key_MapProject;
      } else {
        return Key_Unknown;
      }
    case AssetDefs::KML:
      return Key_KMLProject;
  }

  // should get here, but silences compiler warnings
  return Key_Unknown;
}

AssetDefs::Type AssetDisplayHelper::AssetType(AssetKey key) {
  static AssetDefs::Type types[] = {
    AssetDefs::Invalid,
    AssetDefs::Vector,
    AssetDefs::Vector,
    AssetDefs::Vector,
    AssetDefs::Imagery,
    AssetDefs::Imagery,
    AssetDefs::Imagery,
    AssetDefs::Imagery,
    AssetDefs::Terrain,
    AssetDefs::Terrain,
    AssetDefs::Database,
    AssetDefs::Map,
    AssetDefs::Map,
    AssetDefs::Database,
    AssetDefs::Database,
    AssetDefs::KML,
    AssetDefs::Invalid
  };
  return types[static_cast< unsigned int> (key)];
}

std::string     AssetDisplayHelper::AssetSubType(AssetKey key) {
  static std::string sub_types[] = {
    std::string(),
    std::string(kProductSubtype),
    std::string(kLayer),
    std::string(kProjectSubtype),
    std::string(kProductSubtype),
    std::string(kMercatorProductSubtype),
    std::string(kProjectSubtype),
    std::string(kMercatorProjectSubtype),
    std::string(kProductSubtype),
    std::string(kProjectSubtype),
    std::string(kDatabaseSubtype),
    std::string(kLayer),
    std::string(kProjectSubtype),
    std::string(kMapDatabaseSubtype),
    std::string(kMercatorMapDatabaseSubtype),
    std::string(kProjectSubtype),
    std::string()
  };

  return sub_types[static_cast< unsigned int> (key)];
}

QPixmap AssetDisplayHelper::Pixmap(AssetKey key) {
  static const char* icon_names[] = {
    "closed_folder.xpm",
    "vector_asset.png",
    "vector_layer.png",
    "vector_project.png",
    "imagery_asset.png",
    "imagery_asset_mercator.png",
    "imagery_project.png",
    "imagery_project_mercator.png",
    "terrain_asset.png",
    "terrain_project.png",
    "database.png",
    "map_layer.png",
    "map_project.png",
    "map_database.png",
    "map_database_mercator.png",
    "kml_project.png",
    "failed_asset.png"
  };

  return LoadPixmap(icon_names[static_cast< unsigned int> (key)]);
}

QString AssetDisplayHelper::PrettyName(AssetKey key)  {
  static const std::array<QString, 17> name_pairs
  {{
    QObject::tr("folder"),
    QObject::tr("Vector Resource"),
    QObject::tr("Vector Layer"),
    QObject::tr("Vector Project"),
    QObject::tr("Imagery Resource"),
    QObject::tr("Mercator Imagery Resource"),
    QObject::tr("Flat Imagery Project"),
    QObject::tr("Mercator Imagery Project"),
    QObject::tr("Terrain Resource"),
    QObject::tr("Terrain Project"),
    QObject::tr("Database"),
    QObject::tr("Map Layer"),
    QObject::tr("Map Project"),
    QObject::tr("Flat Projection Map Database"),
    QObject::tr("Mercator Map Database"),
    QObject::tr("KML Project"),
    QObject::tr("Failed")
  }};

  return name_pairs[static_cast< unsigned int> (key)];
}

QPixmap AssetDisplayHelper::LoadPixmap(const QString& name) {
  const QMimeSource* m = QMimeSourceFactory::defaultFactory()->data(name);
  if (!m)
    return QPixmap();
  QPixmap pix;
  QImageDrag::decode(m, pix);
  return pix;
}

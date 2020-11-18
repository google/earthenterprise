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


#include <khConstants.h>
#include <Qt/qpixmap.h>
#include <Qt/q3iconview.h>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <fusionversion.h>
#include <Qt/qdialog.h>
#include "NewAsset.h"

using QIconView = Q3IconView;
using QIconViewItem = Q3IconViewItem;

class NewAssetItem : public QIconViewItem {
 public:
  NewAssetItem(QIconView* parent, AssetDefs::Type t, const std::string &st);
  AssetDisplayHelper DisplayHelper() const { return display_helper_; }

 private:
  AssetDisplayHelper display_helper_;
};

NewAssetItem::NewAssetItem(QIconView* parent, AssetDefs::Type t,
                             const std::string &st)
    : QIconViewItem(parent),
      display_helper_(t, st) {
  setText(display_helper_.PrettyName());
  setPixmap(display_helper_.GetPixmap());
}

// -----------------------------------------------------------------------------

NewAsset::NewAsset(QWidget* parent)
    : NewAssetBase(parent) {
  QIconViewItem* first_item =
         new NewAssetItem(icon_view, AssetDefs::Vector, kProductSubtype);
  (void) new NewAssetItem(icon_view, AssetDefs::Vector, kProjectSubtype);

  if (GetFusionProductType() != FusionLT) {
    (void) new NewAssetItem(icon_view, AssetDefs::Imagery, kProductSubtype);
    (void) new NewAssetItem(icon_view, AssetDefs::Imagery,
                            kMercatorProductSubtype);
    (void) new NewAssetItem(icon_view, AssetDefs::Imagery, kProjectSubtype);
    (void) new NewAssetItem(icon_view, AssetDefs::Imagery,
                            kMercatorProjectSubtype);

    (void) new NewAssetItem(icon_view, AssetDefs::Terrain, kProductSubtype);
    (void) new NewAssetItem(icon_view, AssetDefs::Terrain, kProjectSubtype);
  }

  (void) new NewAssetItem(icon_view, AssetDefs::Map, kLayer);
  (void) new NewAssetItem(icon_view, AssetDefs::Map, kProjectSubtype);

  (void) new NewAssetItem(icon_view, AssetDefs::Database, kDatabaseSubtype);
  (void) new NewAssetItem(icon_view, AssetDefs::Database, kMapDatabaseSubtype);
  (void) new NewAssetItem(icon_view, AssetDefs::Database,
                          kMercatorMapDatabaseSubtype);

  first_item->setSelected(true);
}

NewAsset::~NewAsset() {
}

AssetDisplayHelper::AssetKey NewAsset::ChooseAssetType() {
  if (QDialog::exec() != QDialog::Accepted) {
    return AssetDisplayHelper(AssetDefs::Invalid, "").GetKey();
  }

  NewAssetItem* item = dynamic_cast<NewAssetItem*>(icon_view->currentItem());
  if (item) {
    return item->DisplayHelper().GetKey();
  } else {
    return AssetDisplayHelper(AssetDefs::Invalid, "").GetKey();
  }
}

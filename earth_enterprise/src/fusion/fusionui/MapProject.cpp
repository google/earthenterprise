// Copyright 2017-2020 Google Inc.
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


#include <autoingest/khAssetManagerProxy.h>
#include <autoingest/plugins/MapLayerAsset.h>

#include "MapProject.h"
#include "LayerItemBase.h"
#include "AssetChooser.h"
#include "LayerLegendDialog.h"
#include "AssetDerivedImpl.h"
#include "ProjectLayerView.h"

#include <Qt/q3popupmenu.h>
using QPopupMenu = Q3PopupMenu;
#include <Qt/qpushbutton.h>
#include <Qt/q3header.h>
#include <Qt/qmessagebox.h>
#include <Qt/qgroupbox.h>
#include <Qt/qlineedit.h>
#include <Qt/qobject.h>
#include <Qt/q3listview.h>
using QListViewItem = Q3ListViewItem;
class AssetBase;

// ****************************************************************************
// ***  MapProjectDefs
// ****************************************************************************
MapProjectDefs::SubmitFuncType MapProjectDefs::kSubmitFunc =
     &khAssetManagerProxy::MapProjectEdit;


// ****************************************************************************
// ***  MapLayerItem
// ****************************************************************************
class MapLayerItem : public LayerItemBase {
  friend class MapProjectWidget;
 public:
  MapLayerItem(Q3ListView* parent, const QString& asset_path);
  MapLayerItem(Q3ListView* parent, const MapProjectConfig::LayerItem& layer_cfg);
  void Init();
  const MapProjectConfig::LayerItem& GetConfig() const {
    return layer_item_config_;
  }
  LayerLegend GetEffectiveLegend(void);
  void SetLegend(const LayerLegend& legend);
  void DefaultLegend();

 private:
  MapProjectConfig::LayerItem layer_item_config_;
};

MapLayerItem::MapLayerItem(Q3ListView* parent, const QString& asset_path)
  : LayerItemBase(parent) {
  layer_item_config_.assetRef = asset_path.toUtf8().constData();
  Init();
}

MapLayerItem::MapLayerItem(Q3ListView* parent,
                           const MapProjectConfig::LayerItem& layer_cfg)
  : LayerItemBase(parent),
    layer_item_config_(layer_cfg) {
  Init();
}

void MapLayerItem::Init() {
  MapLayerAsset layer_asset(layer_item_config_.assetRef);
  std::string san;
  if (layer_asset) {
    if (layer_item_config_.legend.UseDefault()) {
      setText(0, layer_asset->config.legend.defaultLocale.name + "<DEFAULT>");
    } else {
      setText(0, layer_item_config_.legend.GetValue().defaultLocale.name);
    }
    san = shortAssetName(layer_item_config_.assetRef);
    setText(1, san.c_str());
    AssetDisplayHelper a(layer_asset->type, layer_asset->subtype);
    setPixmap(1, a.GetPixmap());
  } else {
    setText(0, "<INVALID>");
    san = shortAssetName(layer_item_config_.assetRef);
    setText(1, san.c_str());
    setPixmap(1, AssetDisplayHelper::Pixmap(AssetDisplayHelper::Key_Unknown));
  }
}

LayerLegend MapLayerItem::GetEffectiveLegend(void) {
  if (layer_item_config_.legend.UseDefault()) {
    MapLayerAsset layer_asset(layer_item_config_.assetRef);
    if (layer_asset) {
      return layer_asset->config.legend;
    }
  }

  return layer_item_config_.legend.GetValue();
}

void MapLayerItem::SetLegend(const LayerLegend& legend) {
  layer_item_config_.legend = legend;
  Init();
}

void MapLayerItem::DefaultLegend() {
  layer_item_config_.legend.SetUseDefault();
  Init();
}


// ****************************************************************************
// ***  MapProjectWidget
// ****************************************************************************
MapProjectWidget::MapProjectWidget(QWidget *parent, AssetBase* base)
  : ProjectWidget(parent), AssetWidgetBase(base) {
  add_group_btn->hide();
  // HidePreview();
  HideLegend();
  HideUuid();
  ListView()->EnableAssetDrops(AssetDefs::Map, kLayer);
  ListView()->setColumnText(0, kh::tr("Legend Name"));
  ListView()->addColumn(kh::tr(kLayer.c_str()));
  ListView()->header()->show();
}

void MapProjectWidget::Prefill(const MapProjectEditRequest &req) {
  const MapProjectConfig& cfg = req.config;
  for (std::vector<MapProjectConfig::LayerItem>::const_reverse_iterator it =
         cfg.layers.rbegin();
       it != cfg.layers.rend(); ++it) {
    (void) new MapLayerItem(ListView(), *it);
  }

  HideGenericCheckbox();

  QListViewItem* first = ListView()->firstChild();
  if (first) {
    ListView()->SelectItem(first);
  }

  connect(ListView(), SIGNAL(doubleClicked(Q3ListViewItem*,
                                           const QPoint&, int)),
          this, SLOT(ModifyItem(Q3ListViewItem*, const QPoint&, int)));
}

void MapProjectWidget::AssembleEditRequest(MapProjectEditRequest *request) {
  // assemble layers
  request->config.layers.clear();
  QListViewItem* item = ListView()->firstChild();
  while (item) {
    MapLayerItem* map_layer_item = static_cast<MapLayerItem*>(item);
    request->config.layers.push_back(map_layer_item->GetConfig());
    item = map_layer_item->Next();
  }
}

LayerItemBase* MapProjectWidget::NewLayerItem() {
  AssetChooser chooser(ListView(), AssetChooser::Open,
                       AssetDefs::Map, kLayer);
  if (chooser.exec() != QDialog::Accepted)
    return NULL;

  QString newpath;
  if (!chooser.getFullPath(newpath))
    return NULL;

  return NewLayerItem(newpath);
}

LayerItemBase* MapProjectWidget::NewLayerItem(const QString& assetref) {
  ListView()->setFocus();

  // check to make sure we don't already have this one
  QListViewItem* list_item = ListView()->firstChild();
  while (list_item) {
    MapLayerItem* map_layer_item = static_cast<MapLayerItem*>(list_item);
    if (map_layer_item->layer_item_config_.assetRef == assetref.toUtf8().constData()) {
      QMessageBox::critical(
          this, "Error" ,
          kh::tr("Map layer '%1' already exists in this project")
          .arg(assetref),
          kh::tr("OK"), 0, 0, 0);
      return 0;
    }
    list_item = map_layer_item->Next();
  }

  MapLayerItem* item = new MapLayerItem(ListView(), assetref);
  // all new items get added to the bottom
  while (item->CanMoveDown())
    item->MoveDown();
  ListView()->SelectItem(item);
  return item;
}

void MapProjectWidget::ModifyItem(QListViewItem* item, const QPoint& pt,
                                  int col) {
  if (item == 0 || col == -1)
    return;

  MapLayerItem* layer_item = static_cast<MapLayerItem*>(item);
  LayerLegendDialog legend_dialog(
      this, LocaleDetails::MapLayerMode,
      layer_item->GetEffectiveLegend());
  if (legend_dialog.exec() == QDialog::Accepted) {
    layer_item->SetLegend(legend_dialog.GetLegend());
  }
}

void MapProjectWidget::ContextMenu(QListViewItem* item, const QPoint& point,
                                   int col) {
  if (item == 0 || col == -1)
    return;

  enum { USE_DEFAULT, OVERRIDE_DEFAULT };

  MapLayerItem* layer_item = static_cast<MapLayerItem*>(item);
  QPopupMenu menu(this);
  menu.insertItem(kh::tr("Use Layer Defaults"), USE_DEFAULT);
  menu.insertItem(kh::tr("Override Layer Defaults"), OVERRIDE_DEFAULT);

  switch(menu.exec(point)) {
    case USE_DEFAULT:
      layer_item->DefaultLegend();
      break;
    case OVERRIDE_DEFAULT:
      {
        LayerLegendDialog legend_dialog(
            this, LocaleDetails::MapLayerMode,
            layer_item->GetEffectiveLegend());
        if (legend_dialog.exec() == QDialog::Accepted) {
          layer_item->SetLegend(legend_dialog.GetLegend());
        }
      }
      break;
  }
}


// ****************************************************************************
// ***  MapProject
// ****************************************************************************
MapProject::MapProject(QWidget* parent) :
    AssetDerived<MapProjectDefs, MapProject>(parent)
{
}

MapProject::MapProject(QWidget* parent, const Request& req) :
    AssetDerived<MapProjectDefs, MapProject>(parent, req)
{
}


// Explicit instantiation of base class
template class AssetDerived<MapProjectDefs, MapProject>;

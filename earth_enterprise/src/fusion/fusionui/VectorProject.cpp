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


#include <autoingest/khAssetManagerProxy.h>

#include "VectorProject.h"
#include "LayerItemBase.h"
#include "AssetChooser.h"
#include "PixmapManager.h"
#include "AssetDerivedImpl.h"
#include "ProjectManager.h"
using QListViewItem = Q3ListViewItem;
using QListView = Q3ListView;

// ****************************************************************************
// ***  VectorProjectDefs
// ****************************************************************************
VectorProjectDefs::SubmitFuncType VectorProjectDefs::kSubmitFunc =
     &khAssetManagerProxy::VectorProjectEdit;


// ****************************************************************************
// ***  VectorFilterItem
// ****************************************************************************
class VectorFilterItem : public LayerItemBase {
 public:
  VectorFilterItem(QListViewItem* parent, const DisplayRuleConfig& cfg);
};

VectorFilterItem::VectorFilterItem(QListViewItem* parent,
                                   const DisplayRuleConfig& cfg)
  : LayerItemBase(parent) {
  std::vector< unsigned int>  fill_rgba, outline_rgba;
  fill_rgba.resize(4, 255);
  outline_rgba.resize(4, 255);

  if (cfg.feature.enabled()) {
    fill_rgba = cfg.feature.style.polyColor;
    outline_rgba = cfg.feature.style.lineColor;
  } else if (cfg.site.enabled) {
    fill_rgba = cfg.site.style.label.normalColor;
    outline_rgba = cfg.site.style.label.highlightColor;
  }
  setPixmap(0, ColorBox::Pixmap(
    fill_rgba[0], fill_rgba[1], fill_rgba[2],
    outline_rgba[0], outline_rgba[1], outline_rgba[2]));
  setText(0, cfg.name);
}

// ****************************************************************************
// ***  VectorLayerItem
// ****************************************************************************
class VectorLayerItem : public LayerItemBase {
 public:
  VectorLayerItem(QListView* parent, const QString& asset_path);
  VectorLayerItem(QListView* parent, const LayerConfig& layer_cfg);
  void Init();
  const LayerConfig GetConfig() const {
    return layer_config_;
  }

 private:
  LayerConfig layer_config_;
};

VectorLayerItem::VectorLayerItem(QListView* parent, const QString& asset_path)
  : LayerItemBase(parent) {
  layer_config_.assetRef = asset_path.toUtf8().constData();

  layer_config_.defaultLocale.ClearDefaultFlags();
  layer_config_.defaultLocale.name_ = QFileInfo(asset_path).baseName(true);
  layer_config_.defaultLocale.icon_  = IconReference(IconReference::Internal,
                                                     kDefaultIconName.c_str());
  layer_config_.displayRules.push_back(DisplayRuleConfig(kh::tr("default select all")));

  // XXX need to pick a good default here
  // XXX should be based on the asset source type
  layer_config_.displayRules[0].feature.featureType = VectorDefs::LineZ;

  Init();
}

VectorLayerItem::VectorLayerItem(QListView* parent,
                           const LayerConfig& layer_cfg)
  : LayerItemBase(parent),
    layer_config_(layer_cfg) {
  Init();
}

void VectorLayerItem::Init() {
  // always clear out all children first
  // since this item will be re-inited after the children
  // have been changed
  while (firstChild())
    delete firstChild();

  setText(0, layer_config_.defaultLocale.name_);

  std::string san = shortAssetName(layer_config_.assetRef);
  setText(1, san.c_str());

  std::vector< unsigned int>  fill_rgba, outline_rgba;
  for (std::vector<DisplayRuleConfig>::iterator rule = layer_config_.displayRules.begin();
       rule != layer_config_.displayRules.end(); ++rule) {
    (void) new VectorFilterItem(this, *rule);
  }
}

#if 0
// ****************************************************************************
// ***  VectorProjectWidget
// ****************************************************************************
VectorProjectWidget::VectorProjectWidget(QWidget *parent) :
    ProjectWidget(parent)
{
}

void VectorProjectWidget::Prefill(const VectorProjectEditRequest &req) {
  const VectorProjectConfig& cfg = req.config;
  for (std::vector<LayerConfig>::const_iterator it = cfg.layers.begin();
       it != cfg.layers.end(); ++it) {
    (void) new VectorLayerItem(ListView(), *it);
  }

  QListViewItem* first = ListView()->firstChild();
  if (first) {
    ListView()->setSelected(first, true);
  }
}

void VectorProjectWidget::AssembleEditRequest(
    VectorProjectEditRequest *request) {
  // assemble layers
  request->config.layers.clear();
  QListViewItem* item = ListView()->firstChild();
  while (item) {
    VectorLayerItem* vector_layer_item = static_cast<VectorLayerItem*>(item);
    request->config.layers.push_back(vector_layer_item->GetConfig());
    item = vector_layer_item->Next();
  }
}

LayerItemBase* VectorProjectWidget::NewLayerItem(QListView* listview) {
  AssetChooser chooser(listview, AssetChooser::Open, AssetDefs::Vector,
                       kProductSubtype);
  if (chooser.exec() != QDialog::Accepted)
    return NULL;

  QString asset_path;
  if (!chooser.getFullPath(asset_path))
    return NULL;

  return new VectorLayerItem(listview, asset_path);
}

void VectorProjectWidget::ModifyItem(QListViewItem* item, const QPoint& pt,
                                     int col) {
}

void VectorProjectWidget::ContextMenu(QListViewItem* item, const QPoint& pt,
                                      int col) {
}

#if 0
void VectorProject::EnablePreview(bool enable) {
  if (enable) {
    connect(GfxView::instance, SIGNAL(drawVectors(const gstDrawState&)),
            this, SLOT(DrawFeatures(const gstDrawState&)));
  } else {
    disconnect(GfxView::instance, SIGNAL(drawVectors(const gstDrawState&)),
               this, SLOT(DrawFeatures(const gstDrawState&)));
  }
}

void VectorProject::DrawFeatures(const gstDrawState& state) {
  printf("drawing %d layers\n", layer_listview->childCount());
}

void VectorProject::DrawLabels(QPainter* painter, const gstDrawState& state) {
}
#endif
#endif


// ****************************************************************************
// ***  VectorProject
// ****************************************************************************
VectorProject::VectorProject(QWidget* parent) :
    AssetDerived<VectorProjectDefs, VectorProject>(parent)
{
}

VectorProject::VectorProject(QWidget* parent, const Request& req) :
    AssetDerived<VectorProjectDefs, VectorProject>(parent, req)
{
}

void VectorProject::FinalPreInit(void) {
  // We never preserve the indexVersion. That's up to the system manager
  saved_edit_request_.config.indexVersion = 1;
}


// Explicit instantiation of base class
template class AssetDerived<VectorProjectDefs, VectorProject>;

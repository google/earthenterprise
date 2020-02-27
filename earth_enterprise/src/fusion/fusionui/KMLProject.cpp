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


#include <autoingest/khAssetManagerProxy.h>

#include "KMLProject.h"
#include "LayerItemBase.h"
#include "ProjectLayerView.h"
#include "AssetChooser.h"
#include "AssetDerivedImpl.h"
#include <Qt/qmessagebox.h>

class AssetBase;

// ****************************************************************************
// ***  KMLProjectDefs
// ****************************************************************************
KMLProjectDefs::SubmitFuncType KMLProjectDefs::kSubmitFunc =
     &khAssetManagerProxy::KMLProjectEdit;


// ****************************************************************************
// ***  KMLLayerItem
// ****************************************************************************
class KMLLayerItem : public LayerItemBase {
 public:
  KMLLayerItem(Q3ListView* parent, const KMLProjectConfig::LayerItem& cfg);
  KMLLayerItem(Q3ListView* parent, const QString& asset_path);

  // inherited from Q3ListViewItem
  virtual QString text(int col) const;

  KMLProjectConfig::LayerItem& GetConfig() { return config_; }
  void AdjustLevel();

 private:
  KMLProjectConfig::LayerItem config_;
};

KMLLayerItem::KMLLayerItem(Q3ListView* parent, const KMLProjectConfig::LayerItem& cfg)
  : LayerItemBase(parent),
    config_(cfg) {
}

KMLLayerItem::KMLLayerItem(Q3ListView* parent, const QString& asset_path)
  : LayerItemBase(parent) {
  config_.assetRef = asset_path.toUtf8().constData();
  Asset asset(config_.assetRef);
}

QString KMLLayerItem::text(int col) const {
  if (col == 0) {
    return QString(config_.assetRef.c_str());
  } else {
    return QString();
  }
}

void KMLLayerItem::AdjustLevel() {
  while (CanMoveUp()) {
    SwapPosition(Previous());
    Q3ListViewItem::listView()->sort();
    Q3ListViewItem::listView()->setSelected(this, true);
  }
  while (CanMoveDown()) {
    SwapPosition(Next());
    Q3ListViewItem::listView()->sort();
    Q3ListViewItem::listView()->setSelected(this, true);
  }
}

// ****************************************************************************
// ***  KMLProjectWidget
// ****************************************************************************
KMLProjectWidget::KMLProjectWidget(QWidget *parent, AssetBase* base) :
    ProjectWidget(parent), AssetWidgetBase(base)
{
}

void KMLProjectWidget::Prefill(const KMLProjectEditRequest &req) {
  const KMLProjectConfig& cfg = req.config;
  for (std::vector<KMLProjectConfig::LayerItem>::const_iterator it =
         cfg.layers.begin(); it != cfg.layers.end(); ++it) {
    (void) new KMLLayerItem(ListView(), *it);
  }
}

void KMLProjectWidget::AssembleEditRequest(KMLProjectEditRequest *request) {
  request->config.layers.clear();
  Q3ListViewItem* item = ListView()->firstChild();
  while (item) {
    KMLLayerItem* kml_layer_item = static_cast<KMLLayerItem*>(item);
    request->config.layers.push_back(kml_layer_item->GetConfig());
    item = kml_layer_item->Next();
  }
}

LayerItemBase* KMLProjectWidget::NewLayerItem() {
  AssetChooser chooser(ListView(), AssetChooser::Open,
                       AssetDefs::Imagery, kProductSubtype);

  if (chooser.exec() != QDialog::Accepted)
    return NULL;

  QString newpath;
  if (!chooser.getFullPath(newpath))
    return NULL;

  KMLLayerItem* item = new KMLLayerItem(ListView(), newpath);
  item->AdjustLevel();
  return item;
}

LayerItemBase* KMLProjectWidget::NewLayerItem(const QString& assetref) {
  // check to make sure we don't already have this one
  Q3ListViewItem* list_item = ListView()->firstChild();
  while (list_item) {
    KMLLayerItem* layer_item = static_cast<KMLLayerItem*>(list_item);
    if (layer_item->GetConfig().assetRef == assetref.toUtf8().constData()) {
      QMessageBox::critical(
          this, "Error" ,
          kh::tr("'%1' already exists in this project")
          .arg(assetref),
          kh::tr("OK"), 0, 0, 0);
      return 0;
    }
    list_item = layer_item->Next();
  }

  KMLLayerItem* item = new KMLLayerItem(static_cast<Q3ListView*>(ListView()), assetref);
  item->AdjustLevel();
  return item;
}


// ****************************************************************************
// ***  KMLProject
// ****************************************************************************
KMLProject::KMLProject(QWidget* parent) :
    AssetDerived<KMLProjectDefs, KMLProject>(parent)
{
  // We never preserve the indexVersion. That's up to the system manager
  saved_edit_request_.config.indexVersion = 1;
}

KMLProject::KMLProject(QWidget* parent, const Request& req) :
    AssetDerived<KMLProjectDefs, KMLProject>(parent, req)
{
  // We never preserve the indexVersion. That's up to the system manager
  saved_edit_request_.config.indexVersion = 1;
}


// Explicit instantiation of base class
template class AssetDerived<KMLProjectDefs, KMLProject>;

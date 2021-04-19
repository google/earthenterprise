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

//

#include "fusion/fusionui/RasterProjectWidget.h"
#include <memory>
#include <string>
#include <array>
#include "fusion/fusionui/LayerItemBase.h"
#include "fusion/fusionui/ProjectLayerView.h"
#include "fusion/fusionui/AssetChooser.h"
#include "fusion/fusionui/ConfigWidgets.h"
#include "fusion/fusionui/LayerLegendController.h"
#include "fusion/fusionui/Preferences.h"
#include "fusion/fusionui/GfxView.h"
#include "common/khTileAddr.h"
#include "fusion/gst/gstFeature.h"
#include "fusion/gst/gstGeode.h"
#include "fusion/autoingest/.idl/storage/RasterProjectConfig.h"
#include "fusion/autoingest/plugins/MercatorRasterProductAsset.h"
#include "fusion/autoingest/plugins/RasterProductAsset.h"

#include <Qt/q3popupmenu.h>
#include <Qt/qmenu.h>
#include <Qt/q3header.h>
#include <Qt/qlineedit.h>
#include <Qt/qmessagebox.h>

using QPopupMenu = Q3PopupMenu;
// ****************************************************************************
// ***  RasterLayerItem
// ****************************************************************************
class RasterLayerItem : public LayerItemBase {
 public:
  RasterLayerItem(Q3ListView* parent, const InsetStackItem& cfg,
                  const std::string& date_string, unsigned int level_diff,
                  bool is_mercator);
  RasterLayerItem(Q3ListView* parent, const QString& asset_path,
                  unsigned int level_diff, bool is_mercator);

  // Inherited from Q3ListViewItem
  virtual QString text(int col) const;

  // Inherited from LayerItemBase
  // CanMoveUp is true if the max level of the previous raster inset in the
  // list is less than or equal to this insets max level.
  virtual bool CanMoveUp() const;
  // CanMoveDown is true if the max level of the next raster inset in the
  // list is greater than or equal to this insets max level.
  virtual bool CanMoveDown() const;

  // The ability to move up and down depends whether we wish to honor the
  // acquisition dates (i.e., newer imagery on top).
  // CanMoveUp is true if the max level of the previous raster inset in the
  // list is less than or equal to this insets max level.
  // If "newer_imagery_first" we also check the date as a secondary constraint.
  bool CanMoveUp(bool newer_imagery_first) const;
  // CanMoveDown is true if the max level of the next raster inset in the
  // list is greater than or equal to this insets max level.
  // If "newer_imagery_first" we also check the date as a secondary constraint.
  bool CanMoveDown(bool newer_imagery_first) const;

  InsetStackItem& GetConfig() { return config_; }
  unsigned int MaxLevel() const { return config_.maxlevel; }
  std::string Date() const { return date_string_; }
  void AdjustLevel(bool newer_imagery_first);
  void Draw(const gstDrawState& state);
  const gstBBox& BBox() const { return bbox_; }

 private:
  inline unsigned int ProductToDisplayLevel(unsigned int product_level) const {
    return product_level - level_diff_;
  }
  void InitMetaData();
  void InitBBox();

  InsetStackItem config_;
  std::string date_string_;  // The Acquisition Date in "YYYY-MM-DD" format.
  unsigned int level_diff_;
  gstBBox bbox_;
  const bool is_mercator_;
};

namespace {

static unsigned int CalcLayerDiff(AssetDefs::Type asset_type) {
  switch (asset_type) {
    case AssetDefs::Imagery:
      return ImageryToProductLevel(0);
    case AssetDefs::Terrain:
      return TmeshToProductLevel(0);
    default: {
      assert(0);
    }
  }
  // silence warnings
  return 0;
}

static std::string GetAcquisitionDate(const std::string &asset_path,
                                     bool is_mercator) {
  if (is_mercator) {
    MercatorRasterProductAsset asset(asset_path);
    return asset->GetAcquisitionDate();
  } else {
    RasterProductAsset asset(asset_path);
    return asset->GetAcquisitionDate();
  }
}

}  // namespace


RasterLayerItem::RasterLayerItem(Q3ListView* parent, const InsetStackItem& cfg,
                                 const std::string& date_string,
                                 unsigned int level_diff, bool is_mercator)
  : LayerItemBase(parent),
    config_(cfg),
    date_string_(date_string),
    level_diff_(level_diff),
    is_mercator_(is_mercator) {
  InitBBox();
}

RasterLayerItem::RasterLayerItem(Q3ListView* parent, const QString& asset_path,
                                 unsigned int level_diff, bool is_mercator)
  : LayerItemBase(parent),
    level_diff_(level_diff),
    is_mercator_(is_mercator) {
  config_.dataAsset = asset_path.toUtf8().constData();

  InitMetaData();
  InitBBox();
}

void RasterLayerItem::InitMetaData() {
  if (is_mercator_) {
    MercatorRasterProductAsset asset(config_.dataAsset);
    config_.maxlevel = asset->GetMaxLevel();
    date_string_ = asset->GetAcquisitionDate();
  } else {
    RasterProductAsset asset(config_.dataAsset);
    config_.maxlevel = asset->GetMaxLevel();
    date_string_ = asset->GetAcquisitionDate();
  }
}

void RasterLayerItem::InitBBox() {
  Asset asset(config_.dataAsset);
  AssetVersion ver(asset->GetLastGoodVersionRef());
  if (ver) {
    double n, s, e, w;
    if (sscanf((const char*)ver->meta.GetValue("extents").utf8(),
                              "%lf,%lf,%lf,%lf", &n, &s, &e, &w) == 4) {
      if (is_mercator_) {
        bbox_.init(khTilespace::NormalizeMeter(w),
                   khTilespace::NormalizeMeter(e),
                   khTilespace::NormalizeMeter(s),
                   khTilespace::NormalizeMeter(n));
      } else {
        bbox_.init(khTilespace::Normalize(w), khTilespace::Normalize(e),
                   khTilespace::Normalize(s), khTilespace::Normalize(n));
      }
    }
  }
}

static int LevelColors[][3] = {
  { 255, 0, 0 },
  { 0, 255, 0 },
  { 0, 0, 255 },
  { 255, 255, 0 },
  { 255, 0, 255 },
  { 0, 255, 255 }
};

void RasterLayerItem::Draw(const gstDrawState& state) {
  gstBBox box = bbox_;
  if (state.IsMercatorPreview() && !is_mercator_) {
    MercatorProjection::NormalizeToMercatorFromFlat(&box.n, &box.s);
  } else if (!state.IsMercatorPreview() && is_mercator_) {
    MercatorProjection::NormalizeToFlatFromMercator(&box.n, &box.s);
  }
  if (!box.Valid())
    return;

  gstGeodeHandle geode = gstGeodeImpl::Create(box);

  int c = ProductToImageryLevel(config_.maxlevel) % 6;
  gstFeaturePreviewConfig style;
  style.line_color_[0] = LevelColors[c][0];
  style.line_color_[1] = LevelColors[c][1];
  style.line_color_[2] = LevelColors[c][2];
  style.line_color_[3] = 255;
  style.poly_color_[0] = LevelColors[c][0];
  style.poly_color_[1] = LevelColors[c][1];
  style.poly_color_[2] = LevelColors[c][2];
  style.poly_color_[3] = 100;

  geode->Draw(state, style);
}

QString RasterLayerItem::text(int col) const {
  if (col == 0) {
    QString level;
    if (config_.overridemax == 0) {
      level = QString(" (%1)").arg(ProductToDisplayLevel(config_.maxlevel));
    } else {
      level = QString(" (override=%1)").arg(ProductToDisplayLevel(
          config_.overridemax));
    }

    std::string san = shortAssetName(config_.dataAsset);
    return QString(san.c_str()) + level;
  } else {
    return QString();
  }
}

bool RasterLayerItem::CanMoveUp() const {
  // We don't care about time when the user initiates the move.
  bool newer_imagery_first = false;
  return CanMoveUp(newer_imagery_first);
}

bool RasterLayerItem::CanMoveDown() const {
  // We don't care about time when the user initiates the move.
  bool newer_imagery_first = false;
  return CanMoveDown(newer_imagery_first);
}

bool RasterLayerItem::CanMoveUp(bool newer_imagery_first) const {
  RasterLayerItem* prev_item = dynamic_cast<RasterLayerItem*>(Previous());
  if (newer_imagery_first) {
    // If the previous item is nonexistent or the MaxLevel is higher then
    // no move possible.
    if (!prev_item || prev_item->MaxLevel() > MaxLevel())
      return false;
    // If the MaxLevel of previous item is less than this max level, move is ok.
    if (prev_item->MaxLevel() < MaxLevel())
      return true;
    // Otherwise, it must be same date or newer than date of the previous item.
    return (prev_item->Date().compare(Date()) <= 0);
  } else {  // Ignore the date.
    return prev_item && prev_item->MaxLevel() <= MaxLevel();
  }
}

bool RasterLayerItem::CanMoveDown(bool newer_imagery_first) const {
  RasterLayerItem* next_item = dynamic_cast<RasterLayerItem*>(Next());
  if (newer_imagery_first) {
    // If the next item is nonexistent or the MaxLevel is less then no move
    // possible.
    if (!next_item || next_item->MaxLevel() < MaxLevel())
      return false;
    // If the MaxLevel of next item is higher than this max level, move is ok.
    if (next_item->MaxLevel() > MaxLevel())
      return true;
    // Otherwise, the date of next item must be newer than date of this item.
    return (next_item->Date().compare(Date()) > 0);
  } else {  // Ignore the date.
    return next_item && next_item->MaxLevel() >= MaxLevel();
  }
}

void RasterLayerItem::AdjustLevel(bool newer_imagery_first) {
  while (CanMoveDown(newer_imagery_first)) {
    SwapPosition(Next());
  }
  while (CanMoveUp(newer_imagery_first)) {
    SwapPosition(Previous());
  }
}

// ****************************************************************************
// ***  RasterProjectWidget
// ****************************************************************************
RasterProjectWidget::RasterProjectWidget(QWidget *parent,
                                         AssetDefs::Type asset_type,
                                         const std::string& sub_type)
    : ProjectWidget(parent),
      asset_type_(asset_type),
      sub_type_(sub_type),
      is_mercator_(sub_type == kMercatorProductSubtype),
      level_diff_(CalcLayerDiff(asset_type_)),
      legend_config_(),
      legendManager(this) {
  if (asset_type_ == AssetDefs::Terrain) {
    HideLegend();
  }

  // create widget controllers for legend
  LayerLegendController::Create(legendManager,
                                layer_legend,
                                ((asset_type == AssetDefs::Imagery)
                                 ? LocaleDetails::ImageryLayerMode
                                 : LocaleDetails::TerrainLayerMode),
                                &legend_config_);

  // remove all columns but one, hide the header and set the resize mode
  // such that the remaining column will take up the whole width
  while (ListView()->columns() > 1) {
    ListView()->removeColumn(0);
  }
  ListView()->header()->hide();
  ListView()->setResizeMode(Q3ListView::AllColumns);
  ListView()->EnableAssetDrops(asset_type, sub_type_.c_str());
  SetGenericCheckboxText(kh::tr("Preview"));

  // UUID is currently only exposed to Internal Expert users.
  if (Preferences::GoogleInternal) {
    ShowUuid();
  }

  // TODO: allow turning off of TimeMachine capability.
  if (asset_type_ == AssetDefs::Imagery && !is_mercator_) {
    // && LicenseManager::TimeMachineCapable
    ShowTimeMachineCheckbox();
  }

  if (asset_type_ == AssetDefs::Terrain) {
    ShowOverlayTerrainControls();
  }
}

void RasterProjectWidget::Prefill(const RasterProjectEditRequest &req) {
  // sync LayerLegend to widgets
  legend_config_ = req.config.legend;
  legendManager.SyncToWidgets();

  const RasterProjectConfig& cfg = req.config;
  for (std::vector<InsetStackItem>::const_iterator it =
         cfg.insets.begin();
       it != cfg.insets.end(); ++it) {
    std::string date_string = GetAcquisitionDate(it->dataAsset, is_mercator_);
    (void) new RasterLayerItem(ListView(), *it, date_string, level_diff_,
                               is_mercator_);
  }

  // UUID is currently only exposed to Internal Expert users,
  // but this is taken care of by hiding this edit box from them.
  // Here let's make sure something gets recorded when the edit is sent.
  if (cfg.asset_uuid_ == "") {
    uuid_edit->setText(create_uuid_string().c_str());
  } else {
    uuid_edit->setText(cfg.asset_uuid_.c_str());
  }

  if (cfg.is_timemachine_project_ && CheckForValidDates())
    SetTimeMachineCheckboxState(true);

  if (asset_type_ == AssetDefs::Terrain) {
    SetBuildOverlayTerrainCheckboxState(cfg.is_overlay_terrain_project_);
    if (cfg.is_overlay_terrain_project_) {
      SetOverlayTerrainStartLevelSpinbox(cfg.overlay_terrain_start_level_);
      SetOverlayTerrainResourcesMinLevelSpinbox(
          cfg.overlay_terrain_resources_min_level_);
    }
  }

  connect(this, SIGNAL(ZoomToSource(const gstBBox&)),
          GfxView::instance, SLOT(zoomToBox(const gstBBox&)));
  connect(ListView(), SIGNAL(selectionChanged()),
          GfxView::instance, SLOT(updateGL()));
  connect(this, SIGNAL(RedrawPreview()),
          GfxView::instance, SLOT(updateGL()));

  HideGroupButton();
  emit RedrawPreview();
}

RasterProjectWidget::~RasterProjectWidget() {
  ListView()->clear();
  emit RedrawPreview();
}

void RasterProjectWidget::AssembleEditRequest(
    RasterProjectEditRequest *request) {
  // get LayerLegend
  legendManager.SyncToConfig();
  if (legend_config_.defaultLocale.name.GetValue().isEmpty()) {
    throw khException(kh::tr("Missing legend name"));
  }
  request->config.legend = legend_config_;
  request->config.asset_uuid_ = uuid_edit->text().ascii();

  // TODO: allow turning off of TimeMachine capability via GUI.
    // && LicenseManager::TimeMachineCapable && is_timemachine_project_
  request->config.is_timemachine_project_ = GetTimeMachineCheckboxState();

  if (asset_type_ == AssetDefs::Terrain) {
    request->config.is_overlay_terrain_project_ =
        GetBuildOverlayTerrainCheckboxState();
    if (request->config.is_overlay_terrain_project_) {
      request->config.overlay_terrain_start_level_ =
          GetOverlayTerrainStartLevelSpinbox();
      request->config.overlay_terrain_resources_min_level_ =
          GetOverlayTerrainResourcesMinLevelSpinbox();
    }
  }

  // assemble layers
  request->config.insets.clear();
  Q3ListViewItem* item = ListView()->lastItem();
  while (item) {
    RasterLayerItem* image_layer_item =
      static_cast<RasterLayerItem*>(item);
    request->config.insets.push_back(image_layer_item->GetConfig());
    item = image_layer_item->Previous();
  }
}

void RasterProjectWidget::ContextMenu(Q3ListViewItem* item,
                                       const QPoint& pos, int) {
  RasterLayerItem* image_layer_item = static_cast<RasterLayerItem*>(item);
  if (!image_layer_item)
    return;

  enum { ZOOM_TO_LAYER, MOVE_UP, MOVE_DOWN };

  khDeleteGuard<QPopupMenu> maxoverride_menu;
  InsetStackItem& cfg = image_layer_item->GetConfig();
  unsigned int insetmin = 0;
  unsigned int insetmax = 0;
  GetMinMaxLevels(cfg.dataAsset, insetmin, insetmax);
  unsigned int current_level = (cfg.overridemax == 0) ? insetmax : cfg.overridemax;
  LevelSlider* overridemax_level_slider = new LevelSlider(
      1, static_cast<int>(ProductToDisplayLevel(insetmax)),
      static_cast<int>(ProductToDisplayLevel(current_level)), this);

  QPopupMenu menu(dynamic_cast<QWidget*>(this));
  menu.insertItem(kh::tr("&Zoom to Layer"), ZOOM_TO_LAYER);
  int id = menu.insertItem(kh::tr("Move Layer &Up"), MOVE_UP);

  if (!image_layer_item->CanMoveUp())
    menu.setItemEnabled(id, false);

  id = menu.insertItem(kh::tr("Move Layer &Down"), MOVE_DOWN);
  if (!image_layer_item->CanMoveDown())
    menu.setItemEnabled(id, false);
  menu.insertSeparator();
  maxoverride_menu = TransferOwnership(new QPopupMenu());
  // should be defined through QT3_SUPPORT... ?
  maxoverride_menu->insertItem(QString(), overridemax_level_slider,0);
  menu.insertItem(kh::tr("Adjust Max Level Override"),
                  (QPopupMenu*)maxoverride_menu);

  int menu_id = menu.exec(pos);

  switch (menu_id) {
    case MOVE_UP:
      image_layer_item->MoveUp();
      ListView()->SelectItem(image_layer_item);
      return;
    case MOVE_DOWN:
      image_layer_item->MoveDown();
      ListView()->SelectItem(image_layer_item);
      return;
    case ZOOM_TO_LAYER:
      if (GfxView::instance->state()->IsMercatorPreview() && !is_mercator_) {
        gstBBox box = image_layer_item->BBox();
        MercatorProjection::NormalizeToMercatorFromFlat(&box.n, &box.s);
        emit ZoomToSource(box);
      } else if (!GfxView::instance->state()->IsMercatorPreview() &&
                 is_mercator_) {
        gstBBox box = image_layer_item->BBox();
        MercatorProjection::NormalizeToFlatFromMercator(&box.n, &box.s);
        emit ZoomToSource(box);
      } else {
        emit ZoomToSource(image_layer_item->BBox());
      }
      return;
    default:
      // must be level adjust
      // the menu_id is not valid for this option because
      // it is actually a sub-menu
      cfg.maxlevel = DisplayToProductLevel(
          (unsigned int)overridemax_level_slider->Value());

      if (cfg.maxlevel == insetmax) {
        cfg.overridemax = 0;
      } else {
        cfg.overridemax = cfg.maxlevel;
      }

      bool enforce_newer_first = false;  // When the user initiates a move,
                                         // we don't enforce "newer first".
      image_layer_item->AdjustLevel(enforce_newer_first);
      ListView()->SelectItem(image_layer_item);
  }

  emit RedrawPreview();
}

LayerItemBase* RasterProjectWidget::NewLayerItem() {
  AssetChooser chooser(ListView(), AssetChooser::Open,
                       asset_type_, sub_type_.c_str());
  if (chooser.exec() != QDialog::Accepted)
    return NULL;

  QString newpath;
  if (!chooser.getFullPath(newpath))
    return NULL;

  // Check that the layer has a valid acquisition date if timemachine is active.
  if (GetTimeMachineCheckboxState() && !CheckForValidDates(newpath.toUtf8().constData()))
    return NULL;
  return NewLayerItem(newpath);
}

LayerItemBase* RasterProjectWidget::NewLayerItem(const QString& assetref) {
  // give listview the focus so keyboard navigation will work as expected
  ListView()->setFocus();

  // check to make sure we don't already have this one
  Q3ListViewItem* list_item = ListView()->firstChild();
  while (list_item) {
    RasterLayerItem* layer_item = static_cast<RasterLayerItem*>(list_item);
    if (layer_item->GetConfig().dataAsset == assetref.toUtf8().constData()) {
      QMessageBox::critical(
          this, "Error" ,
          kh::tr("'%1' already exists in this project")
          .arg(assetref),
          kh::tr("OK"), 0, 0, 0);
      return 0;
    }
    list_item = layer_item->Next();
  }

  // Check that the layer has a valid acquisition date if timemachine is active.
  if (GetTimeMachineCheckboxState() && !CheckForValidDates(assetref.toUtf8().constData()))
    return 0;

  RasterLayerItem* item = new RasterLayerItem(ListView(), assetref, level_diff_,
                                              is_mercator_);
  bool enforce_newer_first = true;  // When the user adds a new raster inset,
                                    // we enforce "newer first".
  item->AdjustLevel(enforce_newer_first);
  ListView()->SelectItem(item);
  return item;
}

#if 0
void RasterProjectWidget::SelectLayer() {
  ProjectWidget::SelectLayer();
  emit RedrawPreview();
}
#endif

void RasterProjectWidget::GenericCheckboxToggled(bool state) {
  if (state) {
    connect(GfxView::instance, SIGNAL(drawVectors(const gstDrawState&)),
            this, SLOT(DrawFeatures(const gstDrawState&)));
  } else {
    disconnect(GfxView::instance, SIGNAL(drawVectors(const gstDrawState&)),
               this, SLOT(DrawFeatures(const gstDrawState&)));
  }
  emit RedrawPreview();
}

bool RasterProjectWidget::CheckForValidDates(const std::string& resource) {
  std::vector<std::string> resources;
  if (resource.empty()) {
    // check to make sure we don't already have this one
    Q3ListViewItem* list_item = ListView()->firstChild();
    while (list_item) {
      RasterLayerItem* layer_item = static_cast<RasterLayerItem*>(list_item);
      resources.push_back(layer_item->GetConfig().dataAsset);
      list_item = layer_item->Next();
    }
  } else {
    resources.push_back(resource);
  }

  std::string missing_dates;
  for (unsigned int i = 0; i < resources.size(); ++i) {
    const std::string& resource_i = resources[i];
    std::string date_string = GetAcquisitionDate(resource_i, is_mercator_);

    if (date_string == kUnknownDate) {
      missing_dates += "\n\t" + resource_i;
    }
  }
  if (!missing_dates.empty()) {
    std::string message =
      "Imagery resources in time machine projects are required to have "
      "valid acquisition dates.\n"
      "The following imagery resource(s) do not have acquisition dates "
      "specified:" + missing_dates;
    QMessageBox::critical(
              this, "Error" ,
              kh::tr(
                  "Imagery resources in time machine projects are required to "
                  "have valid acquisition dates.\n"
                  "The following imagery resource(s) do not have acquisition "
                  "dates specified:%1.\n").arg(missing_dates.c_str()),
              "OK", 0, 0, 0);
    return false;
  }
  return true;
}

void RasterProjectWidget::TimeMachineCheckboxToggled(bool state) {
  if (state && !CheckForValidDates())
    state = false;

  SetTimeMachineCheckboxState(state);
}

void RasterProjectWidget::DrawFeatures(const gstDrawState& state) {
  Q3ListViewItem* item = ListView()->firstChild();
  Q3ListViewItem* selected_item = ListView()->selectedItem();
  while (item) {
    RasterLayerItem* image_layer_item = static_cast<RasterLayerItem*>(item);
    if (image_layer_item == selected_item) {
      gstDrawState selected_state = state;
      selected_state.mode |= DrawFilled;
      image_layer_item->Draw(selected_state);
    } else {
      image_layer_item->Draw(state);
    }
    item = image_layer_item->Next();
  }
}

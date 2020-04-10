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


#include "fusion/fusionui/MapLayer.h"

#include <utility>

#include <Qt/qglobal.h>
#include <Qt/qobject.h>
#include <Qt/qcolor.h>
#include <Qt/qcolordialog.h>
#include <Qt/qcombobox.h>
#include <Qt/q3header.h>
#include <Qt/qinputdialog.h>
#include <Qt/qlabel.h>
#include <Qt/qlineedit.h>
#include <Qt/q3listbox.h>
#include <Qt/qmessagebox.h>
#include <Qt/qpainter.h>
#include <Qt/q3popupmenu.h>
#include <Qt/qpushbutton.h>
#include <Qt/qspinbox.h>
#include <Qt/qtextedit.h>
#include <Qt/q3widgetstack.h>
#include <Qt/qgroupbox.h>
#include <Qt/qapplication.h>
#include <Qt/qtooltip.h>
#include <Qt/qcheckbox.h>
#include <Qt/qobject.h>
#include <SkBitmap.h>
#include <SkImageDecoder.h>
#include <Qt/qstring.h>
#include <Qt/q3filedialog.h>


#include "fusion/fusionui/AssetChooser.h"
#include "fusion/fusionui/AssetNotes.h"
#include "fusion/fusionui/GfxView.h"
#include "fusion/fusionui/LayerItemBase.h"
#include "fusion/fusionui/QueryRules.h"
#include "fusion/fusionui/SourceFileDialog.h"
#include "fusion/fusionui/SearchFieldDialog.h"
#include "fusion/fusionui/TextStyle.h"
#include "fusion/fusionui/FieldGeneratorController.h"
#include "fusion/fusionui/ScriptFilterController.h"
#include "fusion/fusionui/LayerLegendController.h"
#include "fusion/fusionui/AssetDerivedImpl.h"
#include "fusion/fusionui/Preferences.h"
#include "fusion/fusionui/ThematicFilter.h"
#include "khException.h"
#include "fusion/autoingest/khAssetManagerProxy.h"
#include "fusion/autoingest/plugins/MapLayerAsset.h"
#include "fusion/gst/maprender/SGLHelps.h"
#include "fusion/gst/gstLayer.h"
#include "fusion/gst/gstRecord.h"
#include "fusion/gst/gstSource.h"
#include "fusion/gst/gstSourceManager.h"
#include "fusion/gst/gstAssetGroup.h"
#include "common/khTileAddrConsts.h"
#include "common/geInstallPaths.h"

class AssetBase;
using QListViewItemIterator = Q3ListViewItemIterator;
using QListView = Q3ListView;
using QPopupMenu = Q3PopupMenu;
using QHeader = Q3Header;

// ****************************************************************************
// ***  MapProjectDefs
// ****************************************************************************
MapLayerDefs::SubmitFuncType MapLayerDefs::kSubmitFunc =
    &khAssetManagerProxy::MapLayerEdit;

// -----------------------------------------------------------------------------

class MapFilterItem : public LayerItemBase {
 public:
  MapFilterItem(QListViewItem* parent, const MapDisplayRuleConfig& cfg);

  virtual QString text(int col) const;

  MapDisplayRuleConfig& Config();

  void SetName(const QString& text);

  std::string AssetRef(void) const;
  const gstSharedSource& Source(void) const;

 private:
  MapDisplayRuleConfig config_;
};

MapFilterItem::MapFilterItem(QListViewItem* parent,
                             const MapDisplayRuleConfig& cfg)
  : LayerItemBase(parent),
    config_(cfg) {
}

QString MapFilterItem::text(int col) const {
  if (col == 0) {
    return config_.name;
  } else {
    return QString::null;
  }
}

MapDisplayRuleConfig& MapFilterItem::Config() {
  return config_;
}

void MapFilterItem::SetName(const QString& text) {
  config_.name = text;
}

// -----------------------------------------------------------------------------

class MapAssetItem : public LayerItemBase {
 public:
  MapAssetItem(QListView* parent, const MapSubLayerConfig& cfg);

  virtual QString text(int col) const;

  MapSubLayerConfig& Config();
  MapSubLayerConfig ConfigCopy();
  void SetConfig(const MapSubLayerConfig& cfg) { config_ = cfg; }
  QStringList FieldDescriptors();
  std::string AssetRef(void) const;
  const gstSharedSource& Source(void) const;
  void BuildChildren();

 private:
  MapSubLayerConfig config_;
  mutable gstSharedSource source_;
  mutable gstHeaderHandle header_;
};

MapAssetItem::MapAssetItem(QListView* parent, const MapSubLayerConfig& cfg)
  : LayerItemBase(parent),
    config_(cfg) {
  BuildChildren();
  setOpen(true);
}

void MapAssetItem::BuildChildren() {
  // when importing a template, this is called again so ensure
  // that all children are deleted before adding new ones
  while (firstChild())
    delete firstChild();

  // Insert display rules in reverse order (Qt inserts each at front)
  for (std::vector<MapDisplayRuleConfig>::const_reverse_iterator it =
           config_.display_rules.rbegin();
       it != config_.display_rules.rend();
       ++it) {
    (void) new MapFilterItem(this, *it);
  }
}

QString MapAssetItem::text(int col) const {
  if (col == 0) {
    return config_.asset_ref.c_str();
  } else {
    return QString::null;
  }
}

MapSubLayerConfig MapAssetItem::ConfigCopy() {
  MapSubLayerConfig config = config_;
  config.display_rules.clear();
  QListViewItem* item = firstChild();
  while (item) {
    MapFilterItem* filter_item = static_cast<MapFilterItem*>(item);
    config.display_rules.push_back(filter_item->Config());
    item = filter_item->nextSibling();
  }
  return config;
}

MapSubLayerConfig& MapAssetItem::Config() {
  return config_;
}

QStringList MapAssetItem::FieldDescriptors() {
  if (!header_) {
    if (!Source()) {
      return QStringList();
    }
    header_ = Source()->GetAttrDefs(0);
  }
  QStringList field_desc;
  for (unsigned int col = 0; col < header_->numColumns(); ++col)
    field_desc.append(header_->Name(col));
  return field_desc;
}

std::string MapAssetItem::AssetRef(void) const {
  return config_.asset_ref;
}

const gstSharedSource& MapAssetItem::Source(void) const {
  if (!source_) {
    source_ = theSourceManager->TryGetSharedAssetSource(config_.asset_ref);
  }
  return source_;
}


std::string MapFilterItem::AssetRef(void) const {
  return static_cast<const MapAssetItem*>(parent())->AssetRef();
}
const gstSharedSource& MapFilterItem::Source(void) const {
  return static_cast<const MapAssetItem*>(parent())->Source();
}

// -----------------------------------------------------------------------------

namespace {
const int SwitchFilterEventId = static_cast<int>(QEvent::User);
}

class SwitchFilterEvent : public QCustomEvent {
 public:
  SwitchFilterEvent(QListViewItem *listItem_)
      : QCustomEvent(SwitchFilterEventId),
        listItem(listItem_) { }

  QListViewItem *listItem;
};


namespace {
  QString UntitledNameText(kh::tr("Untitled"));
}

MapLayerWidget::MapLayerWidget(QWidget* parent, AssetBase* base)
    : MapLayerWidgetBase(parent),
      AssetWidgetBase(base),
      previous_filter_(NULL),
      previous_asset_(NULL),
      legend_config_(),
      legendManager(this),
      displayRuleManager(this),
      filterManager(this),
      active_menu_(0) {
  // create widget controllers for legend
  LayerLegendController::Create(legendManager,
                                layer_legend,
                                LocaleDetails::MapLayerMode,
                                &legend_config_);


  details_stack->raiseWidget(1);
  asset_listview->addColumn(QString::null);
  asset_listview->header()->setStretchEnabled(true, 0);
  asset_listview->header()->hide();
  asset_listview->setSorting(-1);

  connect(poly_mode, SIGNAL(activated(int)),
          this, SLOT(UpdatePolyEnabled(int)));

  // Disable or enable choice buttons depending on choice of point_fill_outline
  // menu choice.
  connect(point_fill_outline, SIGNAL(activated(int)),
          this,
          SLOT(UpdatePointDrawStyle(int)));

  // Disable or enable choice buttons depending on choice of Marker Type choice
  connect(point_marker, SIGNAL(activated(int)),
          this, SLOT(UpdatePointMarker(int)));

  connect(this, SIGNAL(RedrawPreview()),
          GfxView::instance, SLOT(updateGL()));
  preview_enabled_ = false;
}

MapLayerWidget::~MapLayerWidget() {
  if (texture_) {
    theTextureManager->removeTexture(texture_);
    emit RedrawPreview();
  }
}

void MapLayerWidget::PopulateAdvancedDisplayRuleSettings(
  const MapSubLayerConfig& config) {
  featureDuplicationCheck->setChecked(config.allowFeatureDuplication);
  emptyLayerCheck->setChecked(config.allowEmptyLayer);
}

void MapLayerWidget::PopulateSearchFields(const MapSubLayerConfig& config) {
  fields_table->verticalHeader()->hide();
  fields_table->setLeftMargin(0);
  fields_table->setColumnStretchable(1, true);
  fields_table->setNumRows(0);

  for (unsigned int row = 0; row < config.searchFields.size(); ++row) {
    fields_table->setNumRows(row + 1);
    fields_table->setText(row, 0, config.searchFields[row].name);
    fields_table->setText(row, 1,
        SearchField::UseTypeToString(config.searchFields[row].use_type));
    fields_table->adjustRow(row);
  }
  for (int col = 0; col < fields_table->numCols(); ++col)
    fields_table->adjustColumn(col);
}

void MapLayerWidget::SearchFieldSelectionChanged() {
  UpdateSearchFieldButtons();
}

void MapLayerWidget::AddSearchField() {
  SearchFieldDialog dialog(this, attributes);
  if (dialog.exec() == QDialog::Accepted) {
    int row = fields_table->numRows();
    fields_table->setNumRows(row + 1);
    fields_table->setText(row, 0, dialog.GetFieldName());
    fields_table->setText(row, 1, dialog.GetFieldUse());
    fields_table->adjustRow(row);
    attributes.remove(dialog.GetFieldName());
  }
  for (int col = 0; col < fields_table->numCols(); ++col)
    fields_table->adjustColumn(col);
  UpdateSearchFieldButtons();
}

void MapLayerWidget::DeleteSearchField() {
  int row = fields_table->currentRow();
  if ( row == -1 )
    return;

  if (QMessageBox::warning(this, "Warning",
                           QObject::trUtf8("Confirm delete.\n\n"),
                           kh::tr("OK"), kh::tr("Cancel"),
                           QString::null, 1, 1) == 0) {
    fields_table->removeRow(row);
    if (previous_asset_)
      UpdateAvailableSearchAttributes(previous_asset_->AssetRef().c_str());
  }
  UpdateSearchFieldButtons();
}

void MapLayerWidget::UpdateAvailableSearchAttributes(QString assetRef) {
  attributes.clear();
  std::string aRef { assetRef.toUtf8().constData() };
  Asset vector_resource_asset(aRef);
  if (vector_resource_asset->type != AssetDefs::Vector ||
      vector_resource_asset->subtype != kProductSubtype) {
    notify(NFY_WARN, "Invalid product: %s %d ", aRef.c_str(),
           vector_resource_asset->type);
    return;
  }

  AssetVersion ver(vector_resource_asset->GetLastGoodVersionRef());
  if (!ver) {
    notify(NFY_WARN,
           "Unable to get good version of asset %s.\n", assetRef.toUtf8().constData());
    return;
  }

  gstSource* src = new gstSource(ver->GetOutputFilename(0).c_str());
  if (src->Open() != GST_OKAY) {
    QMessageBox::critical(
        this, kh::tr("Error"),
        QString(
            kh::tr("File has unknown problems:\n\n%1")).arg(src->name()),
        kh::tr("OK"), 0, 0, 0);
    return;
  }

  QStringList cols;
  const gstHeaderHandle &record_header = src->GetAttrDefs(0);
  if (record_header && record_header->numColumns() != 0) {
    for (unsigned int col = 0; col < record_header->numColumns(); ++col) {
      if (!SubLayerHasSearchField( record_header->Name(col)))
        cols.push_back(record_header->Name(col));
    }
  }
  attributes = cols;
}

bool MapLayerWidget::SubLayerHasSearchField(QString field) {
  for (int i = 0; i < fields_table->numRows(); ++i) {
    if (fields_table->text(i, 0) == field) return true;
  }
  return false;
}

void MapLayerWidget::MoveSearchFieldUp() {
  int curr_row = fields_table->currentRow();
  SwapSearchFieldRows(curr_row - 1, curr_row);
  fields_table->selectRow(curr_row - 1);
}

void MapLayerWidget::MoveSearchFieldDown() {
  int curr_row = fields_table->currentRow();
  SwapSearchFieldRows(curr_row, curr_row + 1);
  fields_table->selectRow(curr_row + 1);
}

void MapLayerWidget::SwapSearchFieldRows(int row0, int row1) {
  for (int col = 0; col < 2; ++col) {
    QString tmp = fields_table->text(row0, col);
    fields_table->setText(row0, col, fields_table->text(row1, col));
    fields_table->setText(row1, col, tmp);
  }
}

void MapLayerWidget::UpdateSearchFieldButtons() {
  add_field_btn->setEnabled(attributes.size() > 0);
  int row = fields_table->currentRow();
  delete_field_btn->setEnabled(row != -1);
  move_field_up_btn->setEnabled(row > 0);
  move_field_down_btn->setEnabled(row != -1 &&
                                  row < (fields_table->numRows() - 1));
}

void MapLayerWidget::UpdatePolyEnabled(int pos) {
  switch (static_cast<VectorDefs::PolygonDrawMode>(pos)) {
    case VectorDefs::FillAndOutline:
      poly_fill_frame->setEnabled(true);
      poly_outline_frame->setEnabled(true);
      break;
    case VectorDefs::OutlineOnly:
      poly_fill_frame->setEnabled(false);
      poly_outline_frame->setEnabled(true);
      break;
    case VectorDefs::FillOnly:
      poly_fill_frame->setEnabled(true);
      poly_outline_frame->setEnabled(false);
      break;
  }
}

// Update the choosability of "Fill Color", "Outline Width" and "Outline Color"
// combo boxes depending on choice in Visibility input
// Outline and Filled / Outline Only / Filled Only
void MapLayerWidget::UpdatePointDrawStyle(int point_mode) {
  switch (static_cast<VectorDefs::PolygonDrawMode>(point_mode)) {
    case VectorDefs::FillAndOutline:
      point_fill_color_label->setEnabled(true);
      point_fill_color->setEnabled(true);
      point_outline_width_label->setEnabled(true);
      point_outline_width->setEnabled(true);
      point_outline_color_label->setEnabled(true);
      point_outline_color->setEnabled(true);
      break;
    case VectorDefs::OutlineOnly:
      point_fill_color_label->setEnabled(false);
      point_fill_color->setEnabled(false);
      point_outline_width_label->setEnabled(true);
      point_outline_width->setEnabled(true);
      point_outline_color_label->setEnabled(true);
      point_outline_color->setEnabled(true);
      break;
    case VectorDefs::FillOnly:
      point_fill_color_label->setEnabled(true);
      point_fill_color->setEnabled(true);
      point_outline_width_label->setEnabled(false);
      point_outline_width->setEnabled(false);
      point_outline_color_label->setEnabled(false);
      point_outline_color->setEnabled(false);
      break;
    default:
      char buff[256];
      snprintf(buff, sizeof(buff),
               "Bad value \'%d\' for VectorDefs::PolygonDrawMode\n"
               "Should be one of %d, %d, %d\n", point_mode,
               VectorDefs::FillAndOutline, VectorDefs::OutlineOnly,
               VectorDefs::FillOnly);
      throw khException(kh::tr(buff));
  }
}

// Disable or enable choice buttons depending on choice of Marker Type choice
void MapLayerWidget::UpdatePointMarker(int point_marker) {
  bool is_point_icon_img = false;
  bool is_point_width = false;
  bool is_point_height = false;
  bool is_point_fill_outline_frame = false;
  switch (static_cast<VectorDefs::PointMarker>(point_marker)) {
    case VectorDefs::Circle:
    case VectorDefs::Square:
    case VectorDefs::EquilateralTriangle:
      is_point_width = true;
      is_point_fill_outline_frame = true;
      break;
    case VectorDefs::Oval:
    case VectorDefs::Rectangle:
    case VectorDefs::Triangle:
      is_point_width = true;
      is_point_height = true;
      is_point_fill_outline_frame = true;
      break;
    case VectorDefs::Icon:
      is_point_icon_img = true;
      break;
    default:
      char buff[256];
      snprintf(buff, sizeof(buff),
               "Bad value \'%d\' for VectorDefs::PointMarker\n"
               "Should be one in range [%d, %d]\n", point_marker,
               VectorDefs::Circle, VectorDefs::Icon);
      throw khException(kh::tr(buff));
  }
  point_icon_img_label->setEnabled(is_point_icon_img);
  point_icon_img_button->setEnabled(is_point_icon_img);
  point_width_label->setEnabled(is_point_width);
  point_width->setEnabled(is_point_width);
  point_height_label->setEnabled(is_point_height);
  point_height->setEnabled(is_point_height);
  point_fill_outline_frame->setEnabled(is_point_fill_outline_frame);
}

MapFilterItem* MapLayerWidget::SelectedFilterItem() {
  return dynamic_cast<MapFilterItem*>(asset_listview->selectedItem());
}

MapAssetItem* MapLayerWidget::SelectedAssetItem() {
  return dynamic_cast<MapAssetItem*>(asset_listview->selectedItem());
}

void MapLayerWidget::ChooseAsset() {
  AssetChooser chooser(this, AssetChooser::Open, AssetDefs::Vector,
                       kProductSubtype);
  if (chooser.exec() != QDialog::Accepted)
    return;

  QString newpath;
  if (!chooser.getFullPath(newpath))
    return;

  {
    std::string npath { newpath.toUtf8().constData() };
    // Check if it's ready for us to use
    Asset asset(npath);
    if (!asset) {
      QMessageBox::critical(this, "Error" ,
                            kh::tr("%1 isn't a valid asset.").arg(newpath),
                            kh::tr("OK"), 0, 0, 0);
      return;
    }
    AssetVersion ver(asset->GetLastGoodVersionRef());
    if (!ver) {
      QMessageBox::critical(this, "Error" ,
                            kh::tr("%1 doesn't have a good version.").arg(newpath),
                            kh::tr("OK"), 0, 0, 0);
      return;
    }
  }


  MapSubLayerConfig config;
  config.asset_ref = newpath.toUtf8().constData();
  config.display_rules.resize(1);
  config.display_rules[0].name = kh::tr("default select all");
  MapAssetItem *new_item = new MapAssetItem(asset_listview, config);
                                   // note: new item is owned by parent
  while (new_item->CanMoveDown())  // move to last place
    new_item->MoveDown();
}

void MapLayerWidget::ChangeDrawMode(int mode) {
}

void MapLayerWidget::ChangeFeatureType(int new_type) {
}

QString MapLayerWidget::RuleName(const QString& caption,
                                   const QString& initial_name) {
  QString text;

  while (1) {
    bool ok = false;
    text = QInputDialog::getText(caption,
                                 kh::tr("New Rule Name:"),
                                 QLineEdit::Normal,
                                 initial_name,
                                 &ok, this);
    if (!ok) {
      text.truncate(0);
      break;
    } else if (text.isEmpty()) {
        QMessageBox::critical(this, kh::tr("Rule Name Error"),
                              kh::tr("Empty names are not allowed."),
                              kh::tr("OK"), 0, 0, 0);
      continue;
    } else if (text.find(QChar('"')) != -1) {
        QMessageBox::critical(this, kh::tr("Rule Name Error"),
                              kh::tr("Rule names cannot contain \""),
                              kh::tr("OK"), 0, 0, 0);

      continue;
    } else {
      break;
    }
  }

  return text;
}

void MapLayerWidget::customEvent(QEvent* e) {
  QListViewItem *listItem = 0;

  // Make sure this is really an event that we sent.
  switch (static_cast<int>(e->type())) {
    case SwitchFilterEventId: {
      SwitchFilterEvent* rlEvent = dynamic_cast<SwitchFilterEvent*>(e);
      listItem = rlEvent->listItem;
      break;
    }
    default:
      /* not mine */
      return;
  }

  if (!listItem) {
    return;
  }

  // make sure that the listitem that we squirelled away is still valid
  QListViewItemIterator it(asset_listview);
  while (it.current()) {
    if (it.current() == listItem) {
      break;
    }
    ++it;
  }
  if (!it.current()) {
    return;
  }

  // Dispatch it to the appropriate routine
  switch (static_cast<int>(e->type())) {
    case SwitchFilterEventId:
      if (active_menu_) {
        active_menu_->hide();
        active_menu_ = 0;
      }
      asset_listview->setCurrentItem(listItem);
      break;
  }
}

void MapLayerWidget::SelectionChanged() {
  QListViewItem* item = asset_listview->selectedItem();

  UpdateButtons(item);
  CurrentItemChanged(item);
}

void MapLayerWidget::UpdateButtons(QListViewItem* item) {
  // always remove all tooltips.
  // valid items will have them added back appropriately
  QToolTip::remove(new_rule_btn);
  QToolTip::remove(delete_rule_btn);
  QToolTip::remove(copy_rule_btn);
  QToolTip::remove(move_rule_up_btn);
  QToolTip::remove(move_rule_down_btn);

  if (item == NULL) {
    new_rule_btn->setEnabled(false);
    delete_rule_btn->setEnabled(false);
    copy_rule_btn->setEnabled(false);
    move_rule_up_btn->setEnabled(false);
    move_rule_down_btn->setEnabled(false);
    return;
  }

  MapAssetItem* asset_item = dynamic_cast<MapAssetItem*>(item);
  if (asset_item) {
    new_rule_btn->setEnabled(true);
    delete_rule_btn->setEnabled(asset_item->SiblingCount() != 1);
    copy_rule_btn->setEnabled(false);
    move_rule_up_btn->setEnabled(asset_item->CanMoveUp());
    move_rule_down_btn->setEnabled(asset_item->CanMoveDown());

    QToolTip::add(new_rule_btn, kh::tr("New Rule"));
    QToolTip::add(delete_rule_btn, kh::tr("Delete Resource"));
    QToolTip::add(move_rule_up_btn, kh::tr("Move Resource Up"));
    QToolTip::add(move_rule_down_btn, kh::tr("Move Resource Down"));
    return;
  }

  MapFilterItem* filter_item = dynamic_cast<MapFilterItem*>(item);
  if (filter_item) {
    new_rule_btn->setEnabled(true);
    delete_rule_btn->setEnabled(filter_item->SiblingCount() != 1);
    copy_rule_btn->setEnabled(true);
    move_rule_up_btn->setEnabled(filter_item->CanMoveUp());
    move_rule_down_btn->setEnabled(filter_item->CanMoveDown());

    QToolTip::add(new_rule_btn, kh::tr("New Rule"));
    QToolTip::add(delete_rule_btn, kh::tr("Delete Rule"));
    QToolTip::add(copy_rule_btn, kh::tr("Copy Rule"));
    QToolTip::add(move_rule_up_btn, kh::tr("Move Rule Up"));
    QToolTip::add(move_rule_down_btn, kh::tr("Move Rule Down"));
  }
}

void MapLayerWidget::CurrentItemChanged(QListViewItem* item) {
  if (previous_filter_ && (item != previous_filter_)) {
    if (!TrySaveFilter(previous_filter_)) {
      // we can't call asset_listview->setSelected directly
      // since that will confuse out caller (QListView) who is right in the
      // middle of doing a switch already
      QApplication::postEvent(this,
                              new SwitchFilterEvent(previous_filter_));
      return;
    }
    previous_filter_ = 0;
  }

  if (previous_asset_ && (item != previous_asset_)) {
    ApplyAssetEdits(previous_asset_);
    previous_asset_ = 0;
  }

  MapAssetItem* asset_item = dynamic_cast<MapAssetItem*>(item);
  if (asset_item) {
    SelectAsset(asset_item);
    UpdateAvailableSearchAttributes(asset_item->AssetRef().c_str());
  }

  MapFilterItem* filter_item = dynamic_cast<MapFilterItem*>(item);
  if (filter_item) {
    details_stack->raiseWidget(0);
    SelectFilter(filter_item);
  } else {
    details_stack->raiseWidget(1);
  }
}

void MapLayerWidget::ContextMenu(
    QListViewItem* item, const QPoint& point, int col) {
  MapAssetItem* asset_item = dynamic_cast<MapAssetItem*>(item);
  if (asset_item) {
    QPopupMenu menu;
    int up_id = menu.insertItem(kh::tr("Move Resource Up"),
                                this, SLOT(MoveItemUp()));
    if (!asset_item->CanMoveUp())
      menu.setItemEnabled(up_id, false);
    int down_id = menu.insertItem(kh::tr("Move Resource Down"),
                                  this, SLOT(MoveItemDown()));
    if (!asset_item->CanMoveDown())
      menu.setItemEnabled(down_id, false);
    menu.insertSeparator();
    menu.insertItem(
        kh::tr("Export Configuration From Template..."),
        this, SLOT(ExportTemplate()));
    menu.insertItem(
        kh::tr("Import Configuration From Template..."),
        this, SLOT(ImportTemplate()));
    menu.insertSeparator();
    int del_id =
        menu.insertItem(kh::tr("Delete Resource"), this, SLOT(DeleteItem()));
    if (asset_item->SiblingCount() == 1)
      menu.setItemEnabled(del_id, false);
    menu.insertSeparator();
    menu.insertItem(kh::tr("New Rule"), this, SLOT(NewRule()));
    active_menu_ = &menu;
    menu.exec(point);
    active_menu_ = 0;
    return;
  }

  MapFilterItem* filter_item = dynamic_cast<MapFilterItem*>(item);
  if (filter_item) {
    QPopupMenu menu;
    menu.insertItem(kh::tr("New Rule"), this, SLOT(NewRule()));
    menu.insertItem(kh::tr("Copy Rule"), this, SLOT(CopyRule()));
    menu.insertItem(kh::tr("Rename Rule"), this, SLOT(RenameRule()));
    int del_id = menu.insertItem(kh::tr("Delete Rule"), this, SLOT(DeleteItem()));
    if (filter_item->SiblingCount() == 1)
      menu.setItemEnabled(del_id, false);
    menu.insertSeparator();
    int up_id = menu.insertItem(kh::tr("Move Rule Up"), this, SLOT(MoveItemUp()));
    if (!filter_item->CanMoveUp())
      menu.setItemEnabled(up_id, false);
    int down_id =
        menu.insertItem(kh::tr("Move Rule Down"), this, SLOT(MoveItemDown()));
    if (!filter_item->CanMoveDown())
      menu.setItemEnabled(down_id, false);
    active_menu_ = &menu;
    menu.exec(point);
    active_menu_ = 0;
    return;
  }
}

void MapLayerWidget::RenameRule() {
  MapFilterItem* filter_item = SelectedFilterItem();
  assert(filter_item);
  MapDisplayRuleConfig cfg = filter_item->Config();

  QString filter_name = RuleName(kh::tr("Rename Rule"), cfg.name);
  if (!filter_name.isEmpty()) {
    filter_item->SetName(filter_name);
  }
}

void MapLayerWidget::NewRule() {
  QString filter_name = RuleName(kh::tr("New Rule Name:"), kh::tr("Untitled"));
  if (!filter_name.isEmpty()) {
    MapDisplayRuleConfig cfg;
    cfg.name = filter_name;
    MapFilterItem* filter_item = SelectedFilterItem();
    MapFilterItem* new_filter_item = NULL;  // note: owned by parent
    if (filter_item) {
      new_filter_item = new MapFilterItem(filter_item->parent(), cfg);
    } else {
      MapAssetItem* asset_item = SelectedAssetItem();
      assert(asset_item);
      new_filter_item = new MapFilterItem(asset_item, cfg);
    }
    assert(new_filter_item);
    while (new_filter_item->CanMoveDown())  // move to last place
      new_filter_item->MoveDown();
  }
}

void MapLayerWidget::CopyRule() {
  if (previous_filter_) {
    if (!TrySaveFilter(previous_filter_))
      return;
    previous_filter_ = 0;
  }

  MapFilterItem* filter_item = SelectedFilterItem();
  assert(filter_item);

  MapDisplayRuleConfig from_cfg = filter_item->Config();

  QString filter_name = RuleName(kh::tr("New Rule Name:"),
                                   from_cfg.name + kh::tr(" (copy)"));

  if (!filter_name.isEmpty()) {
    MapDisplayRuleConfig cfg = from_cfg;
    cfg.name = filter_name;
    (void) new MapFilterItem(filter_item->parent(), cfg);
  }
}

void MapLayerWidget::DeleteItem() {
  // TODO: set focus on previous item if last one is deleted or
  // new current item if deleted item is in the middle of the list.
  // TODO: Refactoring: remove previous_filter_,
  // previous_asset_ and use selectedItem()?.
  previous_filter_ = NULL;
  previous_asset_ = NULL;
  delete asset_listview->selectedItem();
}

void MapLayerWidget::MoveItemUp() {
  LayerItemBase* item =
    dynamic_cast<LayerItemBase*>(asset_listview->selectedItem());
  assert(item);
  item->SwapPosition(item->Previous());
  UpdateButtons(item);
}

void MapLayerWidget::MoveItemDown() {
  LayerItemBase* item =
    dynamic_cast<LayerItemBase*>(asset_listview->selectedItem());
  assert(item);
  item->SwapPosition(item->Next());
  UpdateButtons(item);
}

void MapLayerWidget::ExportTemplate() {
  MapAssetItem* item =
    dynamic_cast<MapAssetItem*>(asset_listview->selectedItem());
  assert(item);

  Q3FileDialog fd(this);
  fd.setCaption(kh::tr("Export Template"));
  fd.setMode(Q3FileDialog::AnyFile);
  fd.addFilter(kh::tr("Fusion Map Template File (*.kmdsp)"));

  if (fd.exec() != QDialog::Accepted)
    return;

  MapSubLayerConfig cfg = item->ConfigCopy();
  QString template_file(fd.selectedFile());
  if (!template_file.endsWith(".kmdsp"))
    template_file += QString(".kmdsp");
  if (!cfg.Save(template_file.toUtf8().constData())) {
    QMessageBox::critical(this, "Error",
                          kh::tr("Unable to save file: ") + template_file,
                          kh::tr("OK"), 0, 0, 0);
  }
}

void MapLayerWidget::ImportTemplate() {
  MapAssetItem* item =
    dynamic_cast<MapAssetItem*>(asset_listview->selectedItem());
  assert(item);

  OpenWithHistoryDialog fd(this, kh::tr("Import Template"),
                           "maplayertemplatehistory.xml");
  fd.addFilter(kh::tr("Fusion Map Template File (*.kmdsp)"));
  if (fd.exec() != QDialog::Accepted)
    return;

  // Vector syntax so as to be able to call CheckFontSanity
  std::vector<MapSubLayerConfig> template_cfg_vector(1);
  MapSubLayerConfig& template_cfg = template_cfg_vector[0];
  if (!template_cfg.Load(fd.selectedFile().latin1())) {
    QMessageBox::critical(this, kh::tr("Error"),
          QString(kh::tr("Unable to open Fusion Map Template file:\n\n%1")).
          arg(fd.selectedFile()),
          kh::tr("OK"), 0, 0, 0);
    return;
  }
  CheckFontSanity(this, &template_cfg_vector);

  // this is where some validation code should eventually go

  // apply the template to a copy and save it back into the gstLayer
  // make a copy of the config that we can modify
  MapSubLayerConfig tmp_config = item->ConfigCopy();
  tmp_config.ApplyTemplate(template_cfg);
  item->SetConfig(tmp_config);
  item->BuildChildren();
}

void MapLayerWidget::ChooseFilterMatchLogic(int index) {
  if (index == 0 || index == 1) {
    filter_stack->raiseWidget(0);
  } else {
    filter_stack->raiseWidget(1);
  }
}

bool MapLayerWidget::TrySaveFilter(MapFilterItem* item) throw() {
  bool success = false;
  try {
    ApplyFilterEdits(item);
    success = true;
  } catch(const khException &e) {
    QMessageBox::critical(this, kh::tr("Error"), e.qwhat(), kh::tr("OK"), 0, 0, 0);
  } catch(const std::exception &e) {
    QMessageBox::critical(this, kh::tr("Error"), e.what(), kh::tr("OK"), 0, 0, 0);
  } catch(...) {
    QMessageBox::critical(this, kh::tr("Error"), kh::tr("Unknown error"),
                          kh::tr("OK"), 0, 0, 0);
  }
  return success;
}

void MapLayerWidget::TrySaveFilterThrowExceptionIfFails(MapFilterItem* item) {
  try {
    ApplyFilterEdits(item);
  } catch(...) {
    throw;
  }
}

void MapLayerWidget::SelectAsset(MapAssetItem* item) {
  if (item == previous_asset_) {
    // we are being put back to the previously selected item due to an error
    // don't do anything else here
    return;
  }

  const MapSubLayerConfig config = item->ConfigCopy();
  PopulateSearchFields(config);
  PopulateAdvancedDisplayRuleSettings(config);

  previous_asset_ = item;
}

void MapLayerWidget::SelectFilter(MapFilterItem* item) {
  if (item == previous_filter_) {
    // we are being put back to the previously selected item due to an error
    // don't do anything else here
    return;
  }

  workingDisplayRule = item->Config();
  displayRuleManager.clear();
  filterManager.clear();

  // ********** feature tab **********
  ComboContents dispCombo;
  std::map<VectorDefs::FeatureDisplayType, int> featureToIndexMap;
  featureToIndexMap[VectorDefs::PointZ] = dispCombo.size();
  dispCombo.push_back(std::make_pair(static_cast<int>(VectorDefs::PointZ),
                                     kh::tr("Label Only")));
  featureToIndexMap[VectorDefs::LineZ] = dispCombo.size();
  dispCombo.push_back(std::make_pair(static_cast<int>(VectorDefs::LineZ),
                                     kh::tr("Lines")));
  featureToIndexMap[VectorDefs::PolygonZ] = dispCombo.size();
  dispCombo.push_back(std::make_pair(static_cast<int>(VectorDefs::PolygonZ),
                                     kh::tr("Polygons")));
  featureToIndexMap[VectorDefs::IconZ] = dispCombo.size();
  dispCombo.push_back(std::make_pair(static_cast<int>(VectorDefs::IconZ),
                                     kh::tr("Points")));
  std::vector<WidgetControllerManager*> managers =
    EnumStackController<VectorDefs::FeatureDisplayType>::Create(
        displayRuleManager, draw_as_combo, draw_as_stack,
        &workingDisplayRule.feature.displayType,
        dispCombo);

  MapLabelConfig& mapLabelConfig = workingDisplayRule.site.label;

  // === label stack widget ===
  WidgetControllerManager& labelManager =
      *managers[featureToIndexMap[VectorDefs::PointZ]];
  ConstSetterController<bool>::Create(
      labelManager, &mapLabelConfig.hasOutlineLabel, false);
  ConstSetterController<bool>::Create(
      labelManager, &workingDisplayRule.site.label.enabled, true);
  FieldGeneratorController::Create(labelManager,
                                   label_mode, label_edit,
                                   false,  // lineEditEmptyOk
                                   label_button,
                                   &workingDisplayRule.site.label.text,
                                   item->Source(), QStringList(),
                                   "Label");
  RangeSpinController<std::uint32_t>::Create(
      labelManager, label_minlevel, label_maxlevel,
      &workingDisplayRule.site.label.levelRange,
      geRange<std::uint32_t>(0U, MaxClientLevel));
  TextStyleButtonController::Create(labelManager, label_style,
                                    &workingDisplayRule.site.label.textStyle);


  MapFeatureConfig& mapFeatureConfig = workingDisplayRule.feature;
  // === line stack widget ===
  WidgetControllerManager& lineManager =
      *managers[featureToIndexMap[VectorDefs::LineZ]];
  ConstSetterController<bool>::Create(
      lineManager, &mapLabelConfig.hasOutlineLabel, false);
  ConstSetterController<bool>::Create(
      lineManager, &workingDisplayRule.site.label.enabled, false);
  RangeSpinController<std::uint32_t>::Create(
      lineManager, line_minlevel, line_maxlevel,
      &workingDisplayRule.feature.levelRange,
      geRange<std::uint32_t>(0U, MaxClientLevel));
  ColorButtonController::Create(lineManager, line_color,
                                &workingDisplayRule.feature.stroke_color);
  FloatEditController::Create(lineManager, line_thickness,
                              &workingDisplayRule.feature.stroke_width,
                              0.0, 25.0, 1);

  CheckableController<QCheckBox>::Create(
      lineManager, draw_as_roads_check,
      &workingDisplayRule.feature.drawAsRoads);

  // -----
  {
    WidgetControllerManager *boxManager =
      CheckableController<QGroupBox>::Create(
          lineManager, line_label_box,
          &workingDisplayRule.feature.label.enabled);
    FieldGeneratorController::Create(*boxManager,
                                     line_label_mode, line_label_edit,
                                     false,   // lineEditEmptyOk
                                     line_label_button,
                                     &workingDisplayRule.feature.label.text,
                                     item->Source(), QStringList(),
                                     "Label");
    RangeSpinController<std::uint32_t>::Create(
        *boxManager, line_label_minlevel, line_label_maxlevel,
        &workingDisplayRule.feature.label.levelRange,
        geRange<std::uint32_t>(0U, MaxClientLevel));
    TextStyleButtonController::Create(
        *boxManager, line_label_style,
        &workingDisplayRule.feature.label.textStyle);
  }
  // -----
  {
    WidgetControllerManager *boxManager =
      CheckableController<QGroupBox>::Create(
          lineManager, line_shield_box,
          &workingDisplayRule.feature.shield.enabled);

    FieldGeneratorController::Create(*boxManager,
                                     line_shield_mode, line_shield_edit,
                                     true,  // lineEditEmptyOk
                                     line_shield_button,
                                     &workingDisplayRule.feature.shield.text,
                                     item->Source(), QStringList(),
                                     "Shield");
    SpinBoxController<int>::Create(*boxManager, left_margin,
                                   &mapFeatureConfig.shield.left_margin_,
                                   MinDisplayMarginPxl, MaxDisplayMarginPxl);
    SpinBoxController<int>::Create(*boxManager, top_margin,
                                   &mapFeatureConfig.shield.top_margin_,
                                   MinDisplayMarginPxl, MaxDisplayMarginPxl);
    SpinBoxController<int>::Create(*boxManager, right_margin,
                                   &mapFeatureConfig.shield.right_margin_,
                                   MinDisplayMarginPxl, MaxDisplayMarginPxl);
    SpinBoxController<int>::Create(*boxManager, bottom_margin,
                                   &mapFeatureConfig.shield.bottom_margin_,
                                   MinDisplayMarginPxl, MaxDisplayMarginPxl);
    RangeSpinController<std::uint32_t>::Create(
        *boxManager, line_shield_minlevel, line_shield_maxlevel,
        &workingDisplayRule.feature.shield.levelRange,
        geRange<std::uint32_t>(0U, MaxClientLevel));
    TextStyleButtonController::Create(
        *boxManager, line_shield_style,
        &workingDisplayRule.feature.shield.textStyle);
    MapIconController::Create(*boxManager, icon_image_type, icon_image_scaling,
                              icon_image_button, line_shield_fill_color_label,
                              line_shield_fill_color,
                              line_shield_outline_color_label,
                              line_shield_outline_color,
                              &workingDisplayRule.feature.shield, NULL);

    // Set line shield group box enable property depends on status of
    // draw_as_roads checkbox.
    line_shield_box->setEnabled(draw_as_roads_check->state() == QCheckBox::On);
  }


  // === polygon stack widget ===
  WidgetControllerManager& polygonManager =
      *managers[featureToIndexMap[VectorDefs::PolygonZ]];
  ConstSetterController<bool>::Create(
      polygonManager, &mapLabelConfig.hasOutlineLabel, false);
  ConstSetterController<bool>::Create(
      polygonManager, &workingDisplayRule.feature.shield.enabled, false);
  RangeSpinController<std::uint32_t>::Create(
      polygonManager, poly_minlevel, poly_maxlevel,
      &workingDisplayRule.feature.levelRange,
      geRange<std::uint32_t>(0U, MaxClientLevel));
  ComboContents polyMode;
  polyMode.push_back(
      std::make_pair(static_cast<int>(VectorDefs::FillAndOutline),
                     kh::tr("Outline and Fill")));
  polyMode.push_back(std::make_pair(static_cast<int>(VectorDefs::OutlineOnly),
                                    kh::tr("Outline Only")));
  polyMode.push_back(std::make_pair(static_cast<int>(VectorDefs::FillOnly),
                                    kh::tr("Fill Only")));
  EnumComboController<VectorDefs::PolygonDrawMode>::Create(
      polygonManager, poly_mode,
      &workingDisplayRule.feature.polygonDrawMode,
      polyMode);
  UpdatePolyEnabled(
      static_cast<int>(workingDisplayRule.feature.polygonDrawMode));
  ColorButtonController::Create(polygonManager, poly_fill_color,
                                &workingDisplayRule.feature.fill_color);
  ColorButtonController::Create(polygonManager, poly_outline_color,
                                &workingDisplayRule.feature.stroke_color);
  FloatEditController::Create(polygonManager, poly_outline_width,
                              &workingDisplayRule.feature.stroke_width,
                              0.0, 25.0, 1);
  // -----
  {
    WidgetControllerManager *boxManager =
      CheckableController<QGroupBox>::Create(
          polygonManager, poly_clabel_box,
          &workingDisplayRule.site.label.enabled);
    FieldGeneratorController::Create(*boxManager,
                                     poly_clabel_mode, poly_clabel_edit,
                                     false,  // lineEditEmptyOk
                                     poly_clabel_button,
                                     &workingDisplayRule.site.label.text,
                                     item->Source(), QStringList(),
                                     "Centered Label");
    RangeSpinController<std::uint32_t>::Create(
        *boxManager, poly_clabel_minlevel, poly_clabel_maxlevel,
        &workingDisplayRule.site.label.levelRange,
        geRange<std::uint32_t>(0U, MaxClientLevel));
    TextStyleButtonController::Create(
        *boxManager, poly_clabel_style,
        &workingDisplayRule.site.label.textStyle);

    CheckableController<QCheckBox>::Create(
        *boxManager, poly_clabel_displayall_check,
        &workingDisplayRule.site.label.displayAll);
  }
  // -----
  {
    WidgetControllerManager *boxManager =
      CheckableController<QGroupBox>::Create(
          polygonManager, poly_olabel_box,
          &workingDisplayRule.feature.label.enabled);
    FieldGeneratorController::Create(*boxManager,
                                     poly_olabel_mode, poly_olabel_edit,
                                     false,  // lineEditEmptyOk
                                     poly_olabel_button,
                                     &workingDisplayRule.feature.label.text,
                                     item->Source(), QStringList(),
                                     "Outline Label");
    RangeSpinController<std::uint32_t>::Create(
        *boxManager, poly_olabel_minlevel, poly_olabel_maxlevel,
        &workingDisplayRule.feature.label.levelRange,
        geRange<std::uint32_t>(0U, MaxClientLevel));
    TextStyleButtonController::Create(
        *boxManager, poly_olabel_style,
        &workingDisplayRule.feature.label.textStyle);
  }

  // === point stack widget ===
  WidgetControllerManager& pointManager =
      *managers[featureToIndexMap[VectorDefs::IconZ]];
  ConstSetterController<bool>::Create(
      pointManager, &mapLabelConfig.enabled, true);
  ConstSetterController<bool>::Create(
      pointManager, &mapFeatureConfig.shield.enabled, true);
  // The visibility levels input; Point tab -> Visibility .. Through fields
  RangeSpinController<std::uint32_t>::Create(
      pointManager, point_minlevel, point_maxlevel,
      &mapLabelConfig.levelRange, geRange<std::uint32_t>(0U, MaxClientLevel));

  {  // The point rendering shape input; Marker Type
    ComboContents pointMarker;
    for (int i = 0; i <= static_cast<int>(VectorDefs::Icon); ++i) {
        pointMarker.push_back(
                  std::make_pair(i, ToString((VectorDefs::PointMarker)i).c_str()));
    }
    EnumComboController<VectorDefs::PointMarker>::Create(
        pointManager, point_marker, &mapFeatureConfig.pointMarker,
        pointMarker);
  }
  // The icon image button
  IconButtonController::Create(pointManager, *point_icon_img_button,
                               mapFeatureConfig.shield.icon_href_,
                               mapFeatureConfig.shield.icon_type_);
  // The point marker width; Width
  FloatEditController::Create(pointManager, point_width,
                              &mapFeatureConfig.pointWidth,
                              0.0, MaxDisplayMarginPxl, 4);
  // The point marker height; Height
  FloatEditController::Create(pointManager, point_height,
                              &mapFeatureConfig.pointHeight,
                              0.0, MaxDisplayMarginPxl, 4);

  {  // The point fill type; Fill Type
    ComboContents pointFillOutline;
    for (int i = 0; i <= static_cast<int>(VectorDefs::FillOnly); ++i) {
      pointFillOutline.push_back(
          std::make_pair(i, ToString((VectorDefs::PolygonDrawMode)i).c_str()));
    }
    EnumComboController<VectorDefs::PolygonDrawMode>::Create(
        pointManager, point_fill_outline, &mapFeatureConfig.polygonDrawMode,
        pointFillOutline);
  }

  ColorButtonController::Create(pointManager, point_fill_color,
                                &mapFeatureConfig.fill_color);
  ColorButtonController::Create(pointManager, point_outline_color,
                                &mapFeatureConfig.stroke_color);
  FloatEditController::Create(pointManager, point_outline_width,
                              &mapFeatureConfig.stroke_width,
                              0.0, MaxDisplayMarginPxl, 1);
  // Enable or disable these input widgets
  UpdatePointDrawStyle(static_cast<int>(mapFeatureConfig.polygonDrawMode));
  UpdatePointMarker(static_cast<int>(mapFeatureConfig.pointMarker));

  // The label widget in point.
  {
    WidgetControllerManager *boxManager =
      CheckableController<QGroupBox>::Create(
          pointManager, point_center_label_box,
          &mapFeatureConfig.isPointLabelEnabled);
    FieldGeneratorController::Create(*boxManager,
                                     point_center_label_mode,
                                     point_center_label_edit,
                                     false,  // lineEditEmptyOk
                                     point_center_label_button,
                                     &mapLabelConfig.text,
                                     item->Source(), QStringList(),
                                     point_center_label_box->title());
    TextStyleButtonController::Create(*boxManager, point_center_label_style,
                                      &mapFeatureConfig.shield.textStyle);
    // The center label margins around the text
    SpinBoxController<int>::Create(*boxManager, point_left_margin,
                                   &mapFeatureConfig.shield.left_margin_,
                                   MinDisplayMarginPxl, MaxDisplayMarginPxl);
    SpinBoxController<int>::Create(*boxManager, point_top_margin,
                                   &mapFeatureConfig.shield.top_margin_,
                                   MinDisplayMarginPxl, MaxDisplayMarginPxl);
    SpinBoxController<int>::Create(*boxManager, point_right_margin,
                                      &mapFeatureConfig.shield.right_margin_,
                                      MinDisplayMarginPxl, MaxDisplayMarginPxl);
    SpinBoxController<int>::Create(*boxManager, point_bottom_margin,
                                   &mapFeatureConfig.shield.bottom_margin_,
                                   MinDisplayMarginPxl, MaxDisplayMarginPxl);
    {
      WidgetControllerManager *resizeManager =
        CheckableController<QGroupBox>::Create(
            *boxManager, point_marker_resizer_box,
            &mapFeatureConfig.isMarkerResizerEnabled);
      {
        ComboContents resizerMode;
        resizerMode.push_back(
            std::make_pair(
                static_cast<int>(MapShieldConfig::IconFixedAspectStyle),
                kh::tr("Fixed")));
        resizerMode.push_back(
            std::make_pair(
                static_cast<int>(MapShieldConfig::IconVariableAspectStyle),
                kh::tr("Variable")));
        EnumComboController<MapShieldConfig::ShieldIconScaling>::Create(
            *resizeManager, point_marker_scaling,
            &mapFeatureConfig.shield.scaling_,
            resizerMode);
      }
    }
  }

  // The Outer Label widget in point.
  {
    WidgetControllerManager *boxManager =
      CheckableController<QGroupBox>::Create(
          pointManager, point_outer_label_box,
          &mapLabelConfig.hasOutlineLabel);
    FieldGeneratorController::Create(*boxManager,
                                     point_outer_label_mode,
                                     point_outer_label_edit,
                                     false,  // lineEditEmptyOk
                                     point_outer_label_button,
                                     &mapLabelConfig.outlineText,
                                     item->Source(), QStringList(),
                                     point_outer_label_box->title());
    TextStyleButtonController::Create(*boxManager, point_outer_label_style,
                                      &mapLabelConfig.textStyle);

    {
      ComboContents labelPosition;
      for (int i = 0; i <= static_cast<int>(VectorDefs::Right); ++i) {
          labelPosition.push_back(
                      std::make_pair(i, ToString((VectorDefs::EightSides)i).c_str()));
      }
      EnumComboController<VectorDefs::EightSides>::Create(
          *boxManager, point_outer_label_position,
          &mapFeatureConfig.labelPositionRelativeToPoint,
          labelPosition);
    }
  }

  // ********** filter tab **********
  query_rules->setFieldDesc(
      static_cast<MapAssetItem*>(item->parent())->FieldDescriptors());
  query_rules->init(workingDisplayRule.filter);
  match_logic_combo->setCurrentItem(workingDisplayRule.filter.match);
  ScriptFilterController::Create(filterManager,
                                 js_expression_edit, scriptFilterButton,
                                 &workingDisplayRule.filter.matchScript,
                                 item->Source(), QStringList(),
                                 "Filter Expression");
  ChooseFilterMatchLogic(workingDisplayRule.filter.match);

  previous_filter_ = item;
}

void MapLayerWidget::ApplyFilterEdits(MapFilterItem *item) {
  // extract feature tab
  displayRuleManager.SyncToConfig();
  { // check for sanity
    std::string error;
    if (!workingDisplayRule.IsValid(error)) {
        throw khException(error);
    }
  }

  // extract filter tab
  workingDisplayRule.filter = query_rules->getConfig();
  workingDisplayRule.filter.match =
      (FilterConfig::MatchType)match_logic_combo->currentItem();
  filterManager.SyncToConfig();

  MapDisplayRuleConfig& cfg = item->Config();
  // if the name was changed, the new name will only exist in the
  // filter item so we must copy it over
  QString name = cfg.name;
  cfg = workingDisplayRule;
  cfg.name = name;
}

void MapLayerWidget::ApplyAssetEdits(MapAssetItem *item) {
  std::vector<SearchField> searchFields;
  for (int row = 0; row < fields_table->numRows(); ++row) {
    searchFields.push_back(
        SearchField(
            fields_table->text(row, 0),
            SearchField::UseTypeFromString(fields_table->text(row, 1))));
  }
  item->Config().searchFields = searchFields;

  // update Display Rule settings.
  item->Config().allowFeatureDuplication = featureDuplicationCheck->isChecked();
  item->Config().allowEmptyLayer = emptyLayerCheck->isChecked();
}

// This routine collects all GUI inputs and is used to decide whether save
// button should be enabled or not. Save button is enabled only when the latest
// input is different from the last input. This method needs to be reentrant as
// the user may save his input edits more than once resulting into call of this
// method more than once (in the life-span of this widget).
void MapLayerWidget::AssembleEditRequest(MapLayerEditRequest* request) {
  // get LayerLegend
  legendManager.SyncToConfig();
  if (request->assetname != AssetBase::untitled_name.toUtf8().constData()) {
    const QString &legend_name = legend_config_.defaultLocale.name.GetValue();
    if (legend_name.stripWhiteSpace().isEmpty()) {
      throw khException(kh::tr("Missing legend name"));
    }
  }
  request->config.legend = legend_config_;


  // assemble display rules
  if (previous_filter_) {
    // Note: tookout "previous_filter_ = 0" as that was causing
    // in non-reentrant MapLayerWidget::AssembleEditRequest
    TrySaveFilterThrowExceptionIfFails(previous_filter_);
  }
  if (previous_asset_) {
    ApplyAssetEdits(previous_asset_);
    previous_asset_ = 0;
  }

  request->config.subLayers.clear();
  QListViewItem* item = asset_listview->firstChild();
  while (item) {
    MapAssetItem* asset_item = static_cast<MapAssetItem*>(item);
    request->config.subLayers.push_back(asset_item->ConfigCopy());
    item = asset_item->Next();
  }
}

void MapLayerWidget::Prefill(const MapLayerEditRequest& request_orig) {
  MapLayerEditRequest request = request_orig;
  CheckFontSanity(this, &request.config.subLayers);
  // sync LayerLegend to widgets
  legend_config_ = request.config.legend;
  legendManager.SyncToWidgets();

  for (std::vector<MapSubLayerConfig>::const_reverse_iterator it =
           request.config.subLayers.rbegin();
       it != request.config.subLayers.rend();
       ++it) {
    (void) new MapAssetItem(asset_listview, *it);
  }
  if (request.config.subLayers.size()) {
    asset_listview->setCurrentItem(asset_listview->lastItem());
  } else {
    UpdateButtons(NULL);
  }
}

void MapLayerWidget::TogglePreview() {
  try {
    preview_enabled_ = !preview_enabled_;
    if (preview_enabled_) {
      MapLayerEditRequest req;
      AssembleEditRequest(&req);
      QString error;
      bool same_config = previously_rendered_config_ == req.config;
      if (!same_config ||
          is_mercator_preview_ != Preferences::getConfig().isMercatorPreview) {
        // cleanup existing texture if necessary
        if (texture_) {
          theTextureManager->removeTexture(texture_);
          texture_ = NewMapTexture(req.config, texture_, error,
                                   Preferences::getConfig().isMercatorPreview);
        } else {
          texture_ = NewMapTexture(req.config, gstTextureGuard()/* prev */,
                                   error,
                                   Preferences::getConfig().isMercatorPreview);
        }
        if (!texture_)
          throw khException(error);
        theTextureManager->AddOverlayTexture(texture_);
        if (!same_config) {
          previously_rendered_config_ = req.config;
        }
        is_mercator_preview_ = Preferences::getConfig().isMercatorPreview;
      }
      preview_btn->setText(kh::tr("Disable Preview"));
    } else {
      preview_btn->setText(kh::tr("Preview"));
    }
    asset_listview->setEnabled(!preview_enabled_);
    details_stack->setEnabled(!preview_enabled_);
    add_asset_btn->setEnabled(!preview_enabled_);

    texture_->setEnabled(preview_enabled_);

    emit RedrawPreview();
    return;
  } catch(const khException &e) {
    QMessageBox::critical(this, kh::tr("Error"), e.qwhat(), kh::tr("OK"), 0, 0, 0);
  } catch(const std::exception &e) {
    QMessageBox::critical(this, kh::tr("Error"), e.what(), kh::tr("OK"), 0, 0, 0);
  } catch(...) {
    QMessageBox::critical(this, kh::tr("Error"), kh::tr("Unknown error"),
                          kh::tr("OK"), 0, 0, 0);
  }
  preview_enabled_ = !preview_enabled_;
}

// ****************************************************************************
// ***  MapLayer
// ****************************************************************************
MapLayer::MapLayer(QWidget* parent)
  : AssetDerived<MapLayerDefs, MapLayer>(parent) {
}

MapLayer::MapLayer(QWidget* parent, const Request& req)
  : AssetDerived<MapLayerDefs, MapLayer>(parent, req) {
}

// TODO: Add other sanity checks as and when found appropriate.
bool MapFeatureConfig::IsValid(std::string& error) const {
  bool check_icon = false;
  if (displayType == VectorDefs::LineZ) {
    check_icon = shield.enabled && shield.style_ == MapShieldConfig::IconStyle;
  } else if (displayType == VectorDefs::IconZ) {
    check_icon = pointMarker == VectorDefs::Icon;
  }
  if (check_icon) {
    const std::string& icon_href = shield.icon_href_;
    if (icon_href.empty()) {
      error += "The 'Icon Image' needs to be specified.";
      return false;
    }
    IconReference ref(shield.icon_type_, icon_href.c_str());
    SkBitmap icon;
    if (!SkImageDecoder::DecodeFile(ref.SourcePath().c_str(), &icon)) {
      error = error + "The icon file '" + ref.SourcePath().c_str() +
               "' cannot be read/decoded. Check permissions and/or contents.";
      return false;
    }
  }
  return true;
}

// MapLayer, check for font sanity
bool MapLayerWidget::CheckFontSanity(
    QWidget* widget, std::vector<MapSubLayerConfig>* sub_layers_ptr) {
  std::vector<MapSubLayerConfig>& sub_layers = *sub_layers_ptr;
  std::set<maprender::FontInfo> missing_fonts;
  const size_t num_sublayers = sub_layers.size();
  for (size_t i = 0; i < num_sublayers; ++i) {
    std::vector<MapDisplayRuleConfig>& display_rules =
        sub_layers[i].display_rules;
    const size_t num_display_rules = display_rules.size();
    for (size_t j = 0; j < num_display_rules; ++j) {
      maprender::FontInfo::CheckTextStyleSanity(
          &display_rules[j].feature.label.textStyle, &missing_fonts);
      maprender::FontInfo::CheckTextStyleSanity(
          &display_rules[j].feature.shield.textStyle, &missing_fonts);
      maprender::FontInfo::CheckTextStyleSanity(
          &display_rules[j].site.label.textStyle, &missing_fonts);
    }
  }
  if (!missing_fonts.empty()) {
    TextStyle::ShowMissingFontsDialog(widget, missing_fonts);
    return false;
  }
  return true;
}

void MapLayerWidget::ThematicFilterButtonClicked() {
  MapAssetItem* asset_item = SelectedAssetItem();
  if (asset_item == NULL)
    return;  // Only applicable to a MapAssetItem.

  // Need to get the gstSource for the MapAssetItem.
  const gstSharedSource& shared_source = asset_item->Source();
  if (!shared_source)
    return;
  gstSource* source = asset_item->Source()->GetRawSource();
  if (!source)
    return;

  // Create the ThematicFilter dialog, and if it returns true,
  // then add the returned display rules to the MapAssetItem.

  int src_layer_num = 0;
  ThematicFilter thematic_filter(this, source, src_layer_num);
  std::vector<MapDisplayRuleConfig> filters;
  if (thematic_filter.DefineNewFilters(&filters)) {
    MapSubLayerConfig tmp_config = asset_item->ConfigCopy();
    tmp_config.display_rules.resize(0);
    tmp_config.display_rules.reserve(filters.size());
    std::vector<MapDisplayRuleConfig>::const_iterator it = filters.begin();
    for (; it != filters.end(); ++it) {
      tmp_config.display_rules.push_back(*it);
    }
    asset_item->SetConfig(tmp_config);  // Replace the config.
    asset_item->BuildChildren();  // Rebuild the UI for the MapAssetItem.
  }
}

void MapLayerWidget::ToggleDrawAsRoads(int state) {
  line_shield_box->setEnabled(QCheckBox::On == state);
}

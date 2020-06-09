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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_MAPLAYER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_MAPLAYER_H_

#include <vector>
#include <Qt/q3listview.h>
using QListViewItem = Q3ListViewItem;
#include "autoingest/.idl/storage/MapLayerConfig.h"
#include "fusion/gst/gstTextureManager.h"
#include "fusion/fusionui/WidgetControllers.h"

#include "fusion/fusionui/AssetWidgetBase.h"
#include "fusion/fusionui/AssetDerived.h"
#include "fusion/fusionui/AssetDisplayHelper.h"
#include "maplayerwidgetbase.h"

class AssetBase;
class QColor;
class MapFilterItem;
class MapAssetItem;

class MapLayerWidget : public MapLayerWidgetBase,
                       public AssetWidgetBase {
  Q_OBJECT

 public:
  MapLayerWidget(QWidget* parent, AssetBase* base);
  ~MapLayerWidget();

  void Prefill(const MapLayerEditRequest& req);
  void AssembleEditRequest(MapLayerEditRequest* req);

  // Check for availability of ttf files in fontlist to satisfy the fonts used
  // by map sublayers. Report all the missing fonts by an error dialog and also
  // use default font for those. The user will have option not to save these
  // changes and go back and update fontlist rather.
  static bool CheckFontSanity(QWidget* widget,
                              std::vector<MapSubLayerConfig>* sub_layers_ptr);

  // inherited from MapLayerWidgetBase
  virtual void ChangeDrawMode(int);
  virtual void ChangeFeatureType(int);
  virtual void ChooseAsset();
  virtual void SelectionChanged();
  virtual void ContextMenu(QListViewItem*, const QPoint& point, int col);
  virtual void RenameRule();
  virtual void NewRule();
  virtual void CopyRule();
  virtual void DeleteItem();
  virtual void MoveItemUp();
  virtual void MoveItemDown();
  virtual void TogglePreview();
  virtual void ChooseFilterMatchLogic(int index);
  virtual void ExportTemplate();
  virtual void ImportTemplate();

  virtual void ToggleDrawAsRoads(int state);

  // Open the thematic filter dialog.
  virtual void ThematicFilterButtonClicked();

  // search fields:
  virtual void PopulateSearchFields(const MapSubLayerConfig& config);
  virtual void SearchFieldSelectionChanged();
  virtual void AddSearchField();
  virtual void DeleteSearchField();
  virtual void MoveSearchFieldUp();
  virtual void MoveSearchFieldDown();
  virtual void UpdateSearchFieldButtons();
  virtual void UpdateAvailableSearchAttributes(QString assetRef);
  virtual void SwapSearchFieldRows(int row0, int row1);

  // Populate the GUI values for advanced display rule settings given
  // the config.
  void PopulateAdvancedDisplayRuleSettings(const MapSubLayerConfig& config);

 public slots:
  void UpdatePolyEnabled(int);
  void UpdatePointMarker(int);

  void UpdatePointDrawStyle(int point_mode);

 private:
  MapFilterItem* SelectedFilterItem();
  MapAssetItem* SelectedAssetItem();

  QStringList attributes;

  // these 3 are stolen from SelectionRules and probably should be
  // put in a common location someday...
  QColor ChooseColor(QColor color);

  void ApplyFilterEdits(MapFilterItem* item);
  void ApplyAssetEdits(MapAssetItem* item);

  bool TrySaveFilter(MapFilterItem* item) throw();
  void TrySaveFilterThrowExceptionIfFails(MapFilterItem* item);
  QString RuleName(const QString& caption, const QString& orig_name);
  void SelectFilter(MapFilterItem* item);
  void SelectAsset(MapAssetItem* item);
  void UpdateButtons(QListViewItem* item);
  virtual void CurrentItemChanged(QListViewItem* item);

  virtual void customEvent(QEvent *e) final;

  bool SubLayerHasSearchField(QString field);

  khMetaData meta_;
  int selected_filter_;
  MapFilterItem* previous_filter_;
  MapAssetItem* previous_asset_;
  gstTextureGuard texture_;
  bool preview_enabled_;
  MapLayerConfig previously_rendered_config_;
  bool is_mercator_preview_;
  LayerLegend legend_config_;

  MapDisplayRuleConfig    workingDisplayRule;
  WidgetControllerManager legendManager;
  WidgetControllerManager displayRuleManager;
  WidgetControllerManager filterManager;
  QPopupMenu *active_menu_;
};

class MapLayerDefs {
 public:
  typedef MapLayerEditRequest  Request;
  typedef MapLayerWidget      Widget;
  typedef bool (*SubmitFuncType)(const Request&, QString &, int);

  static const AssetDisplayHelper::AssetKey kAssetDisplayKey =
      AssetDisplayHelper::Key_MapLayer;
  static SubmitFuncType        kSubmitFunc;
};

class MapLayer : public AssetDerived<MapLayerDefs, MapLayer>,
                       public MapLayerDefs {
 public:
  MapLayer(QWidget* parent);
  MapLayer(QWidget* parent, const Request& req);
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_MAPLAYER_H_

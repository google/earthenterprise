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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_SELECTIONRULES_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_SELECTIONRULES_H_

#include <Qt/qspinbox.h>
#include "fusion/gst/gstRecord.h"
#include "fusion/gst/gstIconManager.h"
#include "autoingest/.idl/storage/LayerConfig.h"

#include "fusionui/.ui/selectionrulesbase.h"

class LabelFormat;
class gstLayer;

class SelectionRules : public SelectionRulesBase {
 public:
  SelectionRules(QWidget *parent, const LayerConfig &, gstLayer *layer_,
                 const gstHeaderHandle &hdr, int id = 0);

  LayerConfig getConfig() const { return config; }
  void minLevel(int, int);

  // inherited from QDialog
  virtual void accept();

  // inherited from SelectionRulesBase
  virtual void CopyRule();
  virtual void DeleteRule();
  virtual void MoveRuleDown();
  virtual void MoveRuleUp();
  virtual void NewRule();
  virtual void RenameRule();
  virtual void SelectRule(int index);
  virtual void selectTab(QWidget* tab);
  virtual void activateFeatureType(int index);
  virtual void changeElevationMode(int mode);
  virtual void enableSiteLabel(bool enable);
  virtual void toggleDrawAsRoads(bool roads);
  virtual void changePointDecimation(int p);
  virtual void changeRoadLabelType(int type);
  virtual void changePolygonDrawMode(int mode);
  virtual void editSiteLabel();
  virtual void editRoadLabel();
  virtual void editHeightVariable();
  virtual void editPopupText();
  virtual void editBalloonStyleText();
  virtual void chooseSiteIcon();
  virtual void chooseSiteNormalLabelColor();
  virtual void chooseSiteNormalIconColor();
  virtual void chooseRoadLabelColor();
  virtual void chooseLineLineColor();
  virtual void choosePolygonFillColor();
  virtual void choosePolygonLineColor();
  virtual void chooseRoadShield();
  virtual void chooseSiteHighlightLabelColor();
  virtual void chooseSiteHighlightIconColor();
  virtual void activateFilterLogic(int f);
  virtual void editScriptFilter();
  virtual void compileAndAccept();
  virtual void labelModeChanged(int mode);
  virtual void popupLabelModeChanged(int mode);
  virtual void roadLabelModeChanged(int mode);
  virtual void thematicFilter();
  virtual void chooseBalloonBgColor();
  virtual void chooseBalloonFgColor();
  virtual void checkMaxLevel(int level);
  // This takes the ComboBox Id of the mode rather than the config value
  // of the mode.
  virtual void changeBalloonStyleMode(int balloon_mode_combo_box_id);
  virtual void checkMaxBuildLevel(int level);

 private:
  // Note: this function should be the handler of
  // currentIndexChanged(int)-signal in Qt4.1 and higher.
  void toggleFeatureType(int index);

  void UpdateBalloonStyleWidgets(const SiteConfig& cfg);
  void BalloonStyleWidgetsToConfig(SiteConfig* cfg);

  void UpdatePopupWidgets(const SiteConfig& cfg);

  void UpdateElevationModeWidgets(const StyleConfig& cfg);
  void ElevationModeWidgetsToConfig(StyleConfig* cfg);

  void updatePointWidgets();
  void updateLineWidgets();
  void updatePolygonWidgets();
  void updateFeatureWidgets();
  void updatePointVisibility();
  void updateLineVisibility();
  void updatePolygonVisibility();
  void doUpdateConfig(VectorDefs::FeatureDisplayType feature_type);
  void updateConfig();
  void updateLastConfig();
  void updatePointConfig();
  void updateLineConfig();
  void updatePolygonConfig();
  void applyQueryChanges();
  void InsertFilter(const DisplayRuleConfig& cfg);
  // Call this after updating End Level and/or Max Resolution Level.
  // This is to keep track of whether we're beyond the recommended build levels
  // so that we can warn the user in as non-obtrusive way as possible.
  void SetIsBeyondRecommendedBuildLevels();

  // Map the config balloon mode to our combo box id.
  int BalloonComboBoxId(SiteConfig::BalloonStyleMode mode);
  // Map the balloon combo box id to our config balloon mode id.
  SiteConfig::BalloonStyleMode BalloonStyleMode(int combo_box_id);

  QColor VectorToQColor(const std::vector< unsigned int>  &vec);
  void QColorToVector(QColor color, std::vector< unsigned int> * vec);
  QColor chooseColor(QColor color);

  // Print an appropriate warning if the user tries to build very deep levels.
  void displayMaxBuildLevelWarning();

  bool hide_warnings_;  // Turn off warnings during init of class.
  bool is_beyond_recommended_build_level_;  // Keep track if they are beyond
                                            // recommended build levels.

  LayerConfig config;
  gstIcon siteIcon_;      // these should be replaced
  gstIcon roadShield;     // by custom iconButtons

  int selected_filter_;
  gstLayer* layer_;  // needed for ScriptEditor, so it can get header and source

  // Keeps last selected feature type for selected rule. It is used to update
  // feature config with last feature type to save current settings before
  // toggling feature type.
  int last_feature_type_idx_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_SELECTIONRULES_H_

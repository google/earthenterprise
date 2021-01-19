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


#include "fusion/fusionui/SelectionRules.h"

#include <unistd.h>


#include <Qt/q3deepcopy.h>
#include <Qt/q3table.h>

#include <Qt/qmessagebox.h>
#include <Qt/qinputdialog.h>
#include <Qt/qfiledialog.h>
#include <Qt/qcombobox.h>
#include <Qt/qlineedit.h>
#include <Qt/qpushbutton.h>
#include <Qt/qstringlist.h>
#include <Qt/qcolordialog.h>
#include <Qt/qcheckbox.h>
#include <Qt/qlabel.h>
#include <Qt/qgroupbox.h>
#include <Qt/qspinbox.h>
#include <Qt/qtextedit.h>
#include <Qt/qlayout.h>
#include <Qt/qvalidator.h>
#include <Qt/qtabwidget.h>
#include <Qt/q3widgetstack.h>
#include <Qt/qimage.h>
#include <Qt/q3dragobject.h>
#include <Qt/qbuttongroup.h>
#include <common/khTileAddrConsts.h>
#include "fusion/fusionui/QueryRules.h"
#include "fusion/fusionui/SiteIcons.h"
#include "fusion/fusionui/LabelFormat.h"
#include "fusion/fusionui/PixmapManager.h"
#include "fusion/fusionui/AdvancedLabelOptions.h"
#include "fusion/fusionui/Preferences.h"
#include "fusion/fusionui/ScriptEditor.h"
#include "fusion/fusionui/ThematicFilter.h"
#include "fusion/gst/gstLayer.h"
#include "fusion/gst/gstSource.h"
#include "fusion/gst/gstRecordJSContext.h"
#include "fusion/fusionui/BalloonStyleText.h"
using QImageDrag = Q3ImageDrag;
using QMimeSourceFactory = Q3MimeSourceFactory;

enum { QueryRulesStackId = 0, JavascriptStackId = 1 };

static QPixmap uic_load_pixmap(const QString& name) {
  const QMimeSource* m = QMimeSourceFactory::defaultFactory()->data(name);
  if (!m)
    return QPixmap();
  QPixmap pix;
  QImageDrag::decode(m, pix);
  return pix;
}

SelectionRules::SelectionRules(QWidget *parent,
                               const LayerConfig &cfg, gstLayer* layer,
                               const gstHeaderHandle &hdr, int idx)
    : SelectionRulesBase(parent, 0, false, 0),
      hide_warnings_(true),
      is_beyond_recommended_build_level_(false),
      config(cfg),
      selected_filter_(-1),
      layer_(layer),
      last_feature_type_idx_(-1) {
  lineLineWidthEdit->setValidator(
      new QDoubleValidator(0.5, 5.0, 2, lineLineWidthEdit));
  polygonLineWidthEdit->setValidator(
      new QDoubleValidator(0.5, 5.0, 2, polygonLineWidthEdit));
  siteNormalLabelScaleEdit->setValidator(
      new QDoubleValidator(0.5, 5.0, 2, siteNormalLabelScaleEdit));
  siteHighlightLabelScaleEdit->setValidator(
      new QDoubleValidator(0.5, 5.0, 2, siteHighlightLabelScaleEdit));
  roadLabelScaleEdit->setValidator(
      new QDoubleValidator(0.5, 5.0, 2, roadLabelScaleEdit));
  siteNormalIconScaleEdit->setValidator(
      new QDoubleValidator(0.5, 5.0, 2, siteNormalIconScaleEdit));
  siteNormalIconScaleEdit->setValidator(
      new QDoubleValidator(0.5, 5.0, 2, siteHighlightIconScaleEdit));
  roadShieldScaleEdit->setValidator(
      new QDoubleValidator(0.5, 5.0, 2, roadShieldScaleEdit));

  for (std::vector<DisplayRuleConfig>::const_iterator it =
           config.displayRules.begin(); it != config.displayRules.end(); ++it)
    filterList->insertItem((*it).name);

  if (hdr->numColumns() != 0) {
    queryTab->setEnabled(true);
    QStringList fdesc;
    for (unsigned int ii = 0; ii < hdr->numColumns(); ++ii)
      fdesc.append(hdr->Name(ii));
    queryScroller->setFieldDesc(fdesc);
  } else {
    queryTab->setEnabled(false);
  }

  if (idx < static_cast<int>(config.displayRules.size())) {
    filterList->setCurrentItem(idx);
  }

  featureDuplicationCheck->setChecked(cfg.allowFeatureDuplication);
  emptyLayerCheck->setChecked(cfg.allowEmptyLayer);

  if (!Preferences::ExpertMode) {
    featureIDBox->hide();
    featureIDLabel->hide();
    siteIDBox->hide();
    siteIDLabel->hide();
  }

  // Calculate the recommended build level from source.
  std::uint32_t maxResolutionLevel =
    layer->GetSource()->RecommendedMaxResolutionLevel();
  std::uint32_t minResolutionLevel =
    layer->GetSource()->RecommendedMinResolutionLevel();
  std::uint32_t efficientResolutionLevel =
    layer->GetSource()->RecommendedEfficientResolutionLevel();
  if (maxResolutionLevel > 0) {
    featureRecommendedResolutionLevelLabel->setText(kh::tr(
      "Min/Max/Efficient resolution level: %1/%2/%3").
      arg(minResolutionLevel).
      arg(maxResolutionLevel).
      arg(efficientResolutionLevel));
  } else {
    featureRecommendedResolutionLevelLabel->setText(kh::tr(
      "Min/Max/Efficient resolution level: not available"));
  }

  // Ok to show warnings after initialization.
  hide_warnings_ = false;
}

void SelectionRules::MoveRuleDown() {
  updateConfig();
  selected_filter_ = -1;

  // modifying the list text triggers a signal which we don't want just yet
  filterList->blockSignals(true);

  int id = filterList->currentItem();
  DisplayRuleConfig from_cfg = config.displayRules[id];
  DisplayRuleConfig to_cfg = config.displayRules[id + 1];

  filterList->changeItem(from_cfg.name, id + 1);
  filterList->changeItem(to_cfg.name, id);

  config.displayRules[id] = to_cfg;
  config.displayRules[id + 1] = from_cfg;

  filterList->blockSignals(false);

  filterList->setSelected(id + 1, true);
}

void SelectionRules::MoveRuleUp() {
  updateConfig();
  selected_filter_ = -1;

  filterList->blockSignals(true);

  int id = filterList->currentItem();
  DisplayRuleConfig from_cfg = config.displayRules[id];
  DisplayRuleConfig to_cfg = config.displayRules[id - 1];

  filterList->changeItem(from_cfg.name, id - 1);
  filterList->changeItem(to_cfg.name, id);

  config.displayRules[id] = to_cfg;
  config.displayRules[id - 1] = from_cfg;

  filterList->blockSignals(false);
  filterList->setSelected(id - 1, true);
}

void SelectionRules::InsertFilter(const DisplayRuleConfig& cfg) {
  config.displayRules.push_back(cfg);
  filterList->insertItem(Q3DeepCopy<QString>(cfg.name));
  selected_filter_ = -1;
  filterList->setSelected(filterList->numRows() - 1, true);
}

void SelectionRules::NewRule() {
  updateConfig();

  bool ok = false;

  QString filter_name = QInputDialog::getText(
      kh::tr("New Rule"), kh::tr("New Rule Name:"),
      QLineEdit::Normal, kh::tr("untitled"), &ok, this);

  if (ok && !filter_name.isEmpty()) {
    DisplayRuleConfig cfg;
    cfg.name = Q3DeepCopy<QString>(filter_name);
    InsertFilter(cfg);
  }
}

void SelectionRules::CopyRule() {
  updateConfig();

  int id = filterList->currentItem();
  const DisplayRuleConfig& from_cfg = config.displayRules[id];

  bool ok = false;

  QString filter_name = QInputDialog::getText(
      kh::tr("Copy Rule"), kh::tr("New Rule Name:"),
      QLineEdit::Normal, from_cfg.name + " (copy)", &ok, this);

  if (ok && !filter_name.isEmpty()) {
    DisplayRuleConfig cfg = from_cfg;
    cfg.name = Q3DeepCopy<QString>(filter_name);
    // must zero out style id's
    cfg.feature.style.id = 0;
    cfg.site.style.id = 0;
    InsertFilter(cfg);
  }
}

void SelectionRules::thematicFilter() {
  ThematicFilter thematic_filter(this, layer_);
  std::vector<DisplayRuleConfig> filters;
  if (thematic_filter.DefineNewFilters(&filters)) {
    // remove all existing filters
    // filterList->blockSignals(true);
    filterList->clear();
    config.displayRules.clear();
    for (std::vector<DisplayRuleConfig>::iterator it = filters.begin();
         it != filters.end(); ++it) {
      InsertFilter(*it);
    }
    // filterList->blockSignals(false);
    filterList->setSelected(filterList->numRows() - 1, true);
  }
}

void SelectionRules::DeleteRule() {
  filterList->blockSignals(true);

  int id = filterList->currentItem();

  filterList->removeItem(id);

  int newid = (id >= filterList->numRows()) ?  filterList->numRows() - 1 : id;

  std::vector<DisplayRuleConfig>::iterator it = config.displayRules.begin();
  it += id;
  config.displayRules.erase(it);

  filterList->blockSignals(false);

  selected_filter_ = -1;
  filterList->setSelected(newid, true);
}

void SelectionRules::RenameRule() {
  int id = filterList->currentItem();
  DisplayRuleConfig cfg = config.displayRules[id];

  QString text = cfg.name;
  while (1) {
    bool ok = false;
    text = QInputDialog::getText(kh::tr("Rename Rule"),
                                 kh::tr("New Rule Name:"),
                                 QLineEdit::Normal, text,
                                 &ok, this);

    if (ok && !text.isEmpty()) {
      if (text.find(QChar('"')) != -1) {
        QMessageBox::critical(this, kh::tr("Rule Name Error"),
                              kh::tr("Rule names cannot contain \""),
                              kh::tr("OK"), 0, 0, 0);

        continue;
      }
      filterList->blockSignals(true);
      config.displayRules[id].name = Q3DeepCopy<QString>(text);
      filterList->changeItem(text, id);
      filterList->blockSignals(false);
      break;
    } else {
      break;
    }
  }
}

void SelectionRules::selectTab(QWidget* w) {
  // Don't display warnings when switching tabs.
  hide_warnings_ = true;
  SelectRule(filterList->currentItem());

  // Ok to display warnings again.
  hide_warnings_ = false;
}

void SelectionRules::SelectRule(int idx) {
  if (idx == -1)
    return;

  updateConfig();
  selected_filter_ = idx;

  // Update last feature index.
  last_feature_type_idx_ =
      config.displayRules[selected_filter_].feature.featureType;

  move_rule_up_btn->setEnabled(idx != 0);

  int n = filterList->numRows();
  move_rule_down_btn->setEnabled(idx < (n - 1));

  delete_rule_btn->setEnabled(n > 1);

  updateFeatureWidgets();

  if (queryTab->isEnabled()) {
    FilterConfig& filtercfg = config.displayRules[selected_filter_].filter;
    matchLogicCombo->setCurrentItem(static_cast<int>(filtercfg.match));
    activateFilterLogic(static_cast<int>(filtercfg.match));
    queryScroller->init(filtercfg);
    scriptFilterEdit->setText(filtercfg.matchScript);
  }
}

void SelectionRules::activateFilterLogic(int logic) {
  switch (logic) {
    case FilterConfig::MatchAll:
    case FilterConfig::MatchAny:
      filterStack->raiseWidget(QueryRulesStackId);
      break;
    case FilterConfig::JSExpression:
      filterStack->raiseWidget(JavascriptStackId);
      break;
  }
}

void SelectionRules::activateFeatureType(int index) {
  if (index != last_feature_type_idx_) {
    updateLastConfig();

    // Update last feature index.
    last_feature_type_idx_ = index;

    // Toggle feature type.
    toggleFeatureType(index);
  }
}

void SelectionRules::toggleFeatureType(int index) {
  // Update feature type in FeatureConfig of current rule.
  FeatureConfig &featurecfg = config.displayRules[selected_filter_].feature;
  featurecfg.featureType = static_cast<VectorDefs::FeatureDisplayType>(index);

  switch (static_cast<VectorDefs::FeatureDisplayType>(index)) {
    case VectorDefs::PointZ:
      updatePointVisibility();
      break;
    case VectorDefs::LineZ:
      updateLineVisibility();
      break;
    case VectorDefs::PolygonZ:
      updatePolygonVisibility();
      break;
    // The following exceptions because IconZ is used only for map layer.
    case VectorDefs::IconZ:
      throw khException(
          kh::tr("Invalid drawFeatureType VectorDefs::IconZ"));
      break;
  }

  // Update widgets with new feature type.
  updateFeatureWidgets();
}

void SelectionRules::enableSiteLabel(bool flag) {
  siteIDLabel->setEnabled(flag);
  siteIDBox->setEnabled(flag);
  siteStartLevelLabel->setEnabled(flag);
  siteStartLevelBox->setEnabled(flag);
  siteEndLevelLabel->setEnabled(flag);
  siteEndLevelBox->setEnabled(flag);

  siteLabelGroup->setEnabled(flag);
  siteIconGroup->setEnabled(flag);
}

void SelectionRules::changeElevationMode(int mode) {
  StyleConfig::AltitudeMode elevMode = (StyleConfig::AltitudeMode) mode;
  if (elevMode == StyleConfig::ClampToGround) {
    extrude_check->setEnabled(false);
    custom_height_box->setEnabled(false);
  } else if (elevMode == StyleConfig::Relative ||
             elevMode == StyleConfig::Absolute) {
    extrude_check->setEnabled(true);
    custom_height_box->setEnabled(true);
  }
}

void SelectionRules::changePointDecimation(int method) {
  SiteConfig::PointDecimationMode decimationMode =
      static_cast<SiteConfig::PointDecimationMode>(method);
  bool enable = (decimationMode == SiteConfig::RepSubset);

  // Attempts to enable/disable two layout containers of these widgets failed;
  // hence the long version of enabling each label and control.
  pointDecimationMinQuadEdit->setEnabled(enable);
  labelPointDecimationMinQuadEdit->setEnabled(enable);
  pointDecimationMaxQuadEdit->setEnabled(enable);
  labelPointDecimationMaxQuadEdit->setEnabled(enable);
  pointDecimationRatioBox->setEnabled(enable);
  labelPointDecimationRatioBox->setEnabled(enable);
  labelSuffixPointDecimationRatioBox->setEnabled(enable);
}

void SelectionRules::toggleDrawAsRoads(bool drawAsRoads) {
  roadLabelTypeLabel->setEnabled(drawAsRoads);
  roadLabelTypeCombo->setEnabled(drawAsRoads);
  if (drawAsRoads) {
    drawLabelCheck->setChecked(false);
    labelWidgetStack->raiseWidget(1);
    checkMaxLevel(featureEndLevelBox->value());
  } else {
    labelWidgetStack->raiseWidget(0);
  }
}

void SelectionRules::changeRoadLabelType(int type) {
  if (type == static_cast<int>(FeatureConfig::Label))
    roadShieldGroup->setEnabled(false);
  else
    roadShieldGroup->setEnabled(true);
}

void SelectionRules::checkMaxLevel(int max_level) {
  displayMaxBuildLevelWarning();
}


void SelectionRules::checkMaxBuildLevel(int max_build_level) {
  displayMaxBuildLevelWarning();
}


void SelectionRules::SetIsBeyondRecommendedBuildLevels() {
  VectorDefs::FeatureDisplayType featureType =
      static_cast<VectorDefs::FeatureDisplayType>(
          featureTypeCombo->currentItem());

  if (featureType == VectorDefs::PointZ) {
    is_beyond_recommended_build_level_ = false;  // Never relevant for points.
  } else {
    // Only time we want to warn if they're really going to build levels beyond
    // the max recommended build levels, i.e., when both max_level (visible
    // level) and max_build_level are greater than the recommended.
    // Note: we have a potentially different recommendation for roads.
    bool draw_as_roads = drawAsRoadsCheck->isChecked();
    int max_recommended_level = static_cast<int>(draw_as_roads ?
        kMaxRecommendedRoadLevel : kMaxRecommendedBuildLevel);

    int max_level = featureEndLevelBox->value();
    int max_build_level = featureMaxResolutionLevelBox->value();
    is_beyond_recommended_build_level_ =
      max_level > max_recommended_level &&
      max_build_level > max_recommended_level;
  }
}

void SelectionRules::displayMaxBuildLevelWarning() {
  bool was_beyond_recommendation = is_beyond_recommended_build_level_;
  // Update the value of is_beyond_recommended_build_level_.
  // Keep track of whether we're currently beyond the recommended build levels.
  SetIsBeyondRecommendedBuildLevels();

  if (hide_warnings_) {
    return;  // Do nothing.
  }

  // Only display the warning if we're now beyond recommendation and were
  // previously ok.
  if (is_beyond_recommended_build_level_ && !was_beyond_recommendation) {
    bool draw_as_roads = drawAsRoadsCheck->isChecked();
    int max_recommended_level = static_cast<int>(draw_as_roads ?
        kMaxRecommendedRoadLevel : kMaxRecommendedBuildLevel);
    QString geometry_type = draw_as_roads ? "roads" : "lines and polygons";
    QMessageBox::warning(this, kh::tr("Max resolution level warning"),
                         kh::tr("Having a visibility level and a max resolution "
                            "level greater than %1 for %2\n"
                            "can take a significant amount of processing "
                            "time and disk space.\n\n"
                            "Note that you can choose a visibility level "
                            "greater than the max resolution level to see the\n"
                            "geometry at higher zoom levels while limiting "
                            "the processing time and disk space required.\n"
                            "The trade-off is visual fidelity of the layer "
                            "geometry at zoom levels beyond the max\n"
                            "resolution level in exchange for processing time."
                            "\n\n"
                            "For example, if the max resolution level is %3, "
                            "max visibility level is 24, and max\n"
                            "simplification error is 0.5 pixels, then the "
                            "vector will have roughly:\n"
                            "  2^(22-%4) * 0.5 = %5 pixels error (worst case)\n"
                            "when viewed at level 22 in the "
                            "Google Earth Client.\n\n"
                            "You may choose to increase the max resolution "
                            "level to improve the visual fidelity, but this\n"
                            "will be at the expense of processing time and "
                            "disk space which may be prohibitive depending\n"
                            "on the geographic extents of the layer and "
                            "complexity of the layer geometry.").
                         arg(max_recommended_level).
                         arg(geometry_type).
                         arg(max_recommended_level).
                         arg(max_recommended_level).
                         arg(static_cast<int>(
                          pow(2, 22-max_recommended_level)*0.5)),
                         kh::tr("OK"), 0, 0, 0);
  }
}

void SelectionRules::changePolygonDrawMode(int mode) {
  if (mode == static_cast<int>(VectorDefs::FillAndOutline)) {
    polygonFillColorLabel->setEnabled(true);
    polygonFillColorBtn->setEnabled(true);
    polygonLineColorLabel->setEnabled(true);
    polygonLineColorBtn->setEnabled(true);
    polygonLineWidthLabel->setEnabled(true);
    polygonLineWidthEdit->setEnabled(true);
  } else if (mode == static_cast<int>(VectorDefs::OutlineOnly)) {
    polygonFillColorLabel->setEnabled(false);
    polygonFillColorBtn->setEnabled(false);
    polygonLineColorLabel->setEnabled(true);
    polygonLineColorBtn->setEnabled(true);
    polygonLineWidthLabel->setEnabled(true);
    polygonLineWidthEdit->setEnabled(true);
  } else if (mode == static_cast<int>(VectorDefs::FillOnly)) {
    polygonFillColorLabel->setEnabled(true);
    polygonFillColorBtn->setEnabled(true);
    polygonLineColorLabel->setEnabled(false);
    polygonLineColorBtn->setEnabled(false);
    polygonLineWidthLabel->setEnabled(false);
    polygonLineWidthEdit->setEnabled(false);
  }
}

void SelectionRules::editSiteLabel() {
  switch (siteLabelModeCombo->currentItem()) {
    case LabelConfig::Original:
      {
      LabelFormat label_format(this, layer_->GetSourceAttr(),
                               siteLabelText->text(),
                               LabelFormat::SingleLine);
      if (label_format.exec() == QDialog::Accepted)
        siteLabelText->setText(label_format.GetText());
      }
      break;
    case LabelConfig::JSExpression: {
      QString script = siteLabelText->text();
      QStringList contextScripts = layer_->GetExternalContextScripts();
      if (!config.layerContextScript.isEmpty()) {
        contextScripts.push_back(config.layerContextScript);
      }
      if (ScriptEditor::Run(this,
                            layer_->GetSharedSource(),
                            script, ScriptEditor::Expression,
                            contextScripts)) {
        siteLabelText->setText(script);
      }
      break;
    }
  }
}

void SelectionRules::editRoadLabel() {
  switch (roadLabelModeCombo->currentItem()) {
    case LabelConfig::Original:
      {
      LabelFormat label_format(this, layer_->GetSourceAttr(),
                               roadLabelEdit->text(),
                               LabelFormat::SingleLine);
      if (label_format.exec() == QDialog::Accepted)
        roadLabelEdit->setText(label_format.GetText());
      }
      break;
    case LabelConfig::JSExpression: {
      QString script = roadLabelEdit->text();
      QStringList contextScripts = layer_->GetExternalContextScripts();
      if (!config.layerContextScript.isEmpty()) {
        contextScripts.push_back(config.layerContextScript);
      }
      if (ScriptEditor::Run(this, layer_->GetSharedSource(),
                            script, ScriptEditor::Expression,
                            contextScripts)) {
        roadLabelEdit->setText(script);
      }
      break;
    }
  }
}

void SelectionRules::editHeightVariable() {
  LabelFormat label_format(this, layer_->GetSourceAttr(),
                           custom_height_var_name_edit->text(),
                           LabelFormat::SingleLine);
  if (label_format.exec() == QDialog::Accepted)
    custom_height_var_name_edit->setText(label_format.GetText());
}

void SelectionRules::editPopupText() {
  switch (popupTextModeCombo->currentItem()) {
    case LabelConfig::Original:
      {
        LabelFormat label_format(this, layer_->GetSourceAttr(),
                                 sitePopupText->text(),
                                 LabelFormat::MultiLine);
        if (label_format.exec() == QDialog::Accepted)
          sitePopupText->setText(label_format.GetText());
      }
      break;
    case LabelConfig::JSExpression: {
      QString script = sitePopupText->text();
      QStringList contextScripts = layer_->GetExternalContextScripts();
      if (!config.layerContextScript.isEmpty()) {
        contextScripts.push_back(config.layerContextScript);
      }
      if (ScriptEditor::Run(this, layer_->GetSharedSource(),
                            script, ScriptEditor::Expression,
                            contextScripts)) {
        sitePopupText->setText(script);
      }
      break;
    }
  }
}

void SelectionRules::chooseSiteIcon() {
  if (SiteIcons::selectIcon(this, PixmapManager::NormalIcon, &siteIcon_)) {
    QPixmap pix = thePixmapManager->GetPixmap(siteIcon_,
                                              PixmapManager::RegularPair);
    siteIconBtn->setPixmap(pix);
  }
}

void SelectionRules::chooseRoadShield() {
  if (SiteIcons::selectIcon(this, PixmapManager::NormalIcon, &roadShield)) {
    QPixmap pix = thePixmapManager->GetPixmap(roadShield,
                                              PixmapManager::NormalIcon);
    roadShieldBtn->setPixmap(pix);
  }
}

void SelectionRules::chooseLineLineColor() {
  QPalette palette;
  QColor init_color = lineLineColorBtn->paletteBackgroundColor();

  palette.setColor(QPalette::Button, chooseColor(init_color));
  lineLineColorBtn->setAutoFillBackground(true);
  lineLineColorBtn->setPalette(palette);
  lineLineColorBtn->setFlat(true);
  lineLineColorBtn->update();
}

void SelectionRules::choosePolygonFillColor() {
  QColor init_color = polygonFillColorBtn->paletteBackgroundColor();
  polygonFillColorBtn->setPaletteBackgroundColor(chooseColor(init_color));
}

void SelectionRules::choosePolygonLineColor() {
  QColor init_color = polygonLineColorBtn->paletteBackgroundColor();
  polygonLineColorBtn->setPaletteBackgroundColor(chooseColor(init_color));
}

void SelectionRules::chooseSiteNormalLabelColor() {
  QColor init_color = siteNormalLabelColorBtn->paletteBackgroundColor();
  siteNormalLabelColorBtn->setPaletteBackgroundColor(chooseColor(init_color));
}

void SelectionRules::chooseSiteHighlightLabelColor() {
  QColor init_color = siteHighlightLabelColorBtn->paletteBackgroundColor();
  siteHighlightLabelColorBtn->setPaletteBackgroundColor(
      chooseColor(init_color));
}

void SelectionRules::chooseSiteNormalIconColor() {
  QColor init_color = siteNormalIconColorBtn->paletteBackgroundColor();
  siteNormalIconColorBtn->setPaletteBackgroundColor(chooseColor(init_color));
}

void SelectionRules::chooseSiteHighlightIconColor() {
  QColor init_color = siteHighlightIconColorBtn->paletteBackgroundColor();
  siteHighlightIconColorBtn->setPaletteBackgroundColor(chooseColor(init_color));
}

void SelectionRules::chooseRoadLabelColor() {
  QColor init_color = roadLabelColorBtn->paletteBackgroundColor();
  roadLabelColorBtn->setPaletteBackgroundColor(chooseColor(init_color));
}

void SelectionRules::chooseBalloonBgColor() {
  QColor init_color = balloon_bgcolor_btn->paletteBackgroundColor();
  balloon_bgcolor_btn->setPaletteBackgroundColor(chooseColor(init_color));
}

void SelectionRules::chooseBalloonFgColor() {
  QColor init_color = balloon_textcolor_btn->paletteBackgroundColor();
  balloon_textcolor_btn->setPaletteBackgroundColor(chooseColor(init_color));
}

void SelectionRules::changeBalloonStyleMode(int balloon_mode_combo_box_id) {
  switch (balloon_mode_combo_box_id) {
    case 0:  // default
      balloon_details_frame->setEnabled(true);
      balloon_style_stack->raiseWidget(1);
      break;
    case 1:  // advanced
      balloon_details_frame->setEnabled(true);
      balloon_style_stack->raiseWidget(0);
      break;
  }
}

void SelectionRules::editBalloonStyleText() {
  BalloonStyleText dialog(this, balloon_style_text->text());
  if (dialog.exec() == QDialog::Accepted)
    balloon_style_text->setText(dialog.GetText());
}

QColor SelectionRules::chooseColor(QColor color) {
  QRgb init_color = color.rgb();
  //Might need to switch to this format in the future
  //QColor new_color = QColorDialog::getColor(init_color, (QWidget *)this, "Select Color", QColorDialog::DontUseNativeDialog);
  QRgb rgba = QColorDialog::getRgba(init_color, 0, this);

  return QColor(rgba);
}

void SelectionRules::updatePointVisibility() {
  featureWidgetStack->raiseWidget(0);
  labelWidgetStack->raiseWidget(0);

  featureIDLabel->setEnabled(false);
  featureIDBox->setEnabled(false);
  featureStartLevelLabel->setEnabled(false);
  featureStartLevelBox->setEnabled(false);
  featureEndLevelLabel->setEnabled(false);
  featureEndLevelBox->setEnabled(false);
  featureMaxResolutionLevelLabel->setEnabled(false);
  featureRecommendedResolutionLevelLabel->setEnabled(false);
  featureMaxResolutionLevelBox->setEnabled(false);
  featureMaxErrorEdit->setEnabled(false);
  featureMaxErrorLabel->setEnabled(false);
  featureMaxErrorLabel2->setEnabled(false);

  changePointDecimation(pointDecimationBox->selectedId());
  enableSiteLabel(drawLabelCheck->isChecked());
  changeElevationMode(elev_mode_combo->currentItem());
}

void SelectionRules::updateLineVisibility() {
  featureWidgetStack->raiseWidget(1);
  labelWidgetStack->raiseWidget(0);

  featureIDLabel->setEnabled(true);
  featureIDBox->setEnabled(true);
  featureStartLevelLabel->setEnabled(true);
  featureStartLevelBox->setEnabled(true);
  featureEndLevelLabel->setEnabled(true);
  featureEndLevelBox->setEnabled(true);
  featureMaxResolutionLevelLabel->setEnabled(true);
  featureRecommendedResolutionLevelLabel->setEnabled(true);
  featureMaxResolutionLevelBox->setEnabled(true);
  featureMaxErrorEdit->setEnabled(true);
  featureMaxErrorLabel->setEnabled(true);
  featureMaxErrorLabel2->setEnabled(true);

  toggleDrawAsRoads(drawAsRoadsCheck->isChecked());
  changeRoadLabelType(roadLabelTypeCombo->currentItem());
  changeElevationMode(elev_mode_combo->currentItem());
  enableSiteLabel(drawLabelCheck->isChecked());
}

void SelectionRules::updatePolygonVisibility() {
  featureWidgetStack->raiseWidget(2);
  labelWidgetStack->raiseWidget(0);

  featureIDLabel->setEnabled(true);
  featureIDBox->setEnabled(true);
  featureStartLevelLabel->setEnabled(true);
  featureStartLevelBox->setEnabled(true);
  featureEndLevelLabel->setEnabled(true);
  featureEndLevelBox->setEnabled(true);
  featureMaxResolutionLevelLabel->setEnabled(true);
  featureRecommendedResolutionLevelLabel->setEnabled(true);
  featureMaxResolutionLevelBox->setEnabled(true);
  featureMaxErrorEdit->setEnabled(true);
  featureMaxErrorLabel->setEnabled(true);
  featureMaxErrorLabel2->setEnabled(true);

  changePolygonDrawMode(polygonDrawModeCombo->currentItem());
  enableSiteLabel(drawLabelCheck->isChecked());
  changeElevationMode(elev_mode_combo->currentItem());
}

int SelectionRules::BalloonComboBoxId(SiteConfig::BalloonStyleMode mode) {
  // As of 3.2, we save 2 balloon modes: id 1 and 2 corresponding to
  // 0 and 1 in our combo box.
  assert(mode == SiteConfig::Advanced || mode == SiteConfig::Basic);
  return static_cast<int>(mode) - 1;
}

SiteConfig::BalloonStyleMode SelectionRules::BalloonStyleMode(
  int combo_box_id) {
  assert(combo_box_id == 0 || combo_box_id == 1);
  // As of 3.2, we save 2 balloon modes: id 1 and 2 corresponding to
  // 0 and 1 in our combo box.
  if (combo_box_id == 1) {
    return SiteConfig::Advanced;
  }
  return SiteConfig::Basic;
}

void SelectionRules::UpdateBalloonStyleWidgets(const SiteConfig& site_cfg) {
  balloon_style_text->setText(site_cfg.balloonText);
  balloon_bgcolor_btn->setPaletteBackgroundColor(
      VectorToQColor(site_cfg.balloonBgColor));
  balloon_textcolor_btn->setPaletteBackgroundColor(
      VectorToQColor(site_cfg.balloonFgColor));
  balloon_header_check->setChecked(site_cfg.balloonInsertHeader);
  balloon_directions_check->setChecked(site_cfg.balloonInsertDirections);
  int balloon_combo_box_id = BalloonComboBoxId(site_cfg.balloonStyleMode);
  balloon_style_options->setCurrentItem(balloon_combo_box_id);
  changeBalloonStyleMode(balloon_combo_box_id);
}

void SelectionRules::UpdateElevationModeWidgets(const StyleConfig& cfg) {
  elev_mode_combo->setCurrentItem(static_cast<int>(cfg.altitudeMode));
  extrude_check->setChecked(cfg.extrude);
  custom_height_box->setChecked(static_cast<bool>(cfg.enableCustomHeight));
  custom_height_var_name_edit->setText(cfg.customHeightVariableName);
  custom_height_offset_edit->setText(QString::number(cfg.customHeightOffset));
  custom_height_scale_edit->setText(QString::number(cfg.customHeightScale));
}

void SelectionRules::UpdatePopupWidgets(const SiteConfig& cfg) {
  siteIconGroup->setChecked(cfg.enablePopup);

  const IconConfig& icon_cfg = cfg.style.icon;
  siteIcon_ = gstIcon(icon_cfg.href, icon_cfg.type);
  const QPixmap &pix = thePixmapManager->GetPixmap(siteIcon_,
                                                   PixmapManager::RegularPair);
  if (!pix.isNull()) {
    siteIconBtn->setPixmap(pix);
  } else {
    siteIconBtn->setText("?");
  }

  siteNormalIconColorBtn->setPaletteBackgroundColor(
      VectorToQColor(icon_cfg.normalColor));
  siteHighlightIconColorBtn->setPaletteBackgroundColor(
      VectorToQColor(icon_cfg.highlightColor));

  siteNormalIconScaleEdit->setText(QString::number(icon_cfg.normalScale));
  siteHighlightIconScaleEdit->setText(QString::number(icon_cfg.highlightScale));

  popupTextModeCombo->setCurrentItem(cfg.popupTextMode);

  sitePopupText->setText(cfg.popupText);
}

void SelectionRules::updatePointWidgets() {
  FeatureConfig &featurecfg = config.displayRules[selected_filter_].feature;
  SiteConfig &sitecfg = config.displayRules[selected_filter_].site;
  StyleConfig &fstylecfg = featurecfg.style;
  StyleConfig &stylecfg = sitecfg.style;
  LabelConfig &labelcfg = stylecfg.label;

  // feature widget stack
  featureIDBox->setValue(fstylecfg.id);
  pointDecimationBox->setButton(static_cast<int>(sitecfg.decimationMode));
  changePointDecimation(static_cast<int>(sitecfg.decimationMode));
  pointDecimationMaxQuadEdit->setText(QString::number(sitecfg.maxQuadCount));
  pointDecimationMinQuadEdit->setText(QString::number(sitecfg.minQuadCount));
  pointDecimationRatioBox->setValue(static_cast<int>(
                                        sitecfg.decimationRatio * 100));
  suppressDuplicatesCheck->setChecked(sitecfg.suppressDuplicateSites);

  // label widget stack
  drawLabelCheck->setChecked(static_cast<bool>(sitecfg.enabled));
  siteLabelModeCombo->setCurrentItem(labelcfg.labelMode);
  siteLabelText->setText(labelcfg.label);
  siteNormalLabelColorBtn->setPaletteBackgroundColor(
      VectorToQColor(labelcfg.normalColor));
  siteHighlightLabelColorBtn->setPaletteBackgroundColor(
      VectorToQColor(labelcfg.highlightColor));
  siteNormalLabelScaleEdit->setText(QString::number(labelcfg.normalScale));
  siteHighlightLabelScaleEdit->setText(
      QString::number(labelcfg.highlightScale));
  siteIDBox->setValue(stylecfg.id);
  siteStartLevelBox->setValue(sitecfg.minLevel);
  siteEndLevelBox->setValue(sitecfg.maxLevel);

  UpdatePopupWidgets(sitecfg);
  UpdateBalloonStyleWidgets(sitecfg);
  UpdateElevationModeWidgets(stylecfg);

  updatePointVisibility();

  // Keep track of whether we're currently beyond the recommended build levels.
  SetIsBeyondRecommendedBuildLevels();
}

void SelectionRules::updateLineWidgets() {
  FeatureConfig &featurecfg = config.displayRules[selected_filter_].feature;
  SiteConfig &sitecfg = config.displayRules[selected_filter_].site;
  StyleConfig &fstylecfg = featurecfg.style;
  StyleConfig &sstylecfg = sitecfg.style;
  LabelConfig &flabelcfg = fstylecfg.label;
  LabelConfig &slabelcfg = sstylecfg.label;
  IconConfig &ficoncfg = fstylecfg.icon;

  // feature widget stack
  featureIDBox->setValue(fstylecfg.id);
  featureStartLevelBox->setValue(featurecfg.minLevel);
  featureEndLevelBox->setValue(featurecfg.maxLevel);
  featureMaxResolutionLevelBox->setValue(featurecfg.maxBuildLevel);

  featureMaxErrorEdit->setText(QString::number(featurecfg.maxError));
  drawAsRoadsCheck->setChecked(featurecfg.drawAsRoads);
  roadLabelTypeCombo->setCurrentItem(featurecfg.roadLabelType);

  QPalette palette;
  palette.setColor(QPalette::Button, VectorToQColor(fstylecfg.lineColor));

  lineLineColorBtn->setAutoFillBackground(true);
  lineLineColorBtn->setPalette(palette);
  lineLineColorBtn->setFlat(true);
  lineLineColorBtn->update();

  lineLineWidthEdit->setText(QString::number(fstylecfg.lineWidth));

  // label widget stack
  drawLabelCheck->setChecked(static_cast<bool>(sitecfg.enabled));
  siteLabelModeCombo->setCurrentItem(slabelcfg.labelMode);
  siteLabelText->setText(slabelcfg.label);
  siteNormalLabelColorBtn->setPaletteBackgroundColor(
      VectorToQColor(slabelcfg.normalColor));
  siteHighlightLabelColorBtn->setPaletteBackgroundColor(
      VectorToQColor(slabelcfg.highlightColor));
  siteNormalLabelScaleEdit->setText(QString::number(slabelcfg.normalScale));
  siteHighlightLabelScaleEdit->setText(
      QString::number(slabelcfg.highlightScale));
  siteIDBox->setValue(sstylecfg.id);
  siteStartLevelBox->setValue(sitecfg.minLevel);
  siteEndLevelBox->setValue(sitecfg.maxLevel);

  UpdatePopupWidgets(sitecfg);
  UpdateBalloonStyleWidgets(sitecfg);

  roadLabelModeCombo->setCurrentItem(flabelcfg.labelMode);
  roadLabelEdit->setText(flabelcfg.label);
  roadLabelFormatCheck->setChecked(flabelcfg.reformat);
  roadLabelColorBtn->setPaletteBackgroundColor(
      VectorToQColor(flabelcfg.normalColor));
  roadLabelScaleEdit->setText(QString::number(flabelcfg.normalScale));
  gstIcon fIcon(ficoncfg.href, ficoncfg.type);
  const QPixmap &fpix = thePixmapManager->GetPixmap(
      fIcon, PixmapManager::NormalIcon);
  if (!fpix.isNull()) {
    roadShieldBtn->setPixmap(fpix);
  }
  roadShield = fIcon;
  roadShieldScaleEdit->setText(QString::number(ficoncfg.normalScale));

  UpdateElevationModeWidgets(fstylecfg);

  updateLineVisibility();

  // Keep track of whether we're currently beyond the recommended build levels.
  SetIsBeyondRecommendedBuildLevels();
}

void SelectionRules::updatePolygonWidgets() {
  FeatureConfig &featurecfg = config.displayRules[selected_filter_].feature;
  SiteConfig &sitecfg = config.displayRules[selected_filter_].site;
  StyleConfig &fstylecfg = featurecfg.style;
  StyleConfig &sstylecfg = sitecfg.style;
  LabelConfig &slabelcfg = sstylecfg.label;

  // feature widget stack
  featureIDBox->setValue(fstylecfg.id);
  featureStartLevelBox->setValue(featurecfg.minLevel);
  featureEndLevelBox->setValue(featurecfg.maxLevel);
  featureMaxResolutionLevelBox->setValue(featurecfg.maxBuildLevel);
  featureMaxErrorEdit->setText(QString::number(featurecfg.maxError));
  polygonDrawModeCombo->setCurrentItem(
      static_cast<int>(featurecfg.polygonDrawMode));
  polygonFillColorBtn->setPaletteBackgroundColor(
      VectorToQColor(fstylecfg.polyColor));
  polygonLineColorBtn->setPaletteBackgroundColor(
      VectorToQColor(fstylecfg.lineColor));
  polygonLineWidthEdit->setText(QString::number(fstylecfg.lineWidth));

  // label widget stack
  drawLabelCheck->setChecked(static_cast<bool>(sitecfg.enabled));
  siteLabelModeCombo->setCurrentItem(slabelcfg.labelMode);
  siteLabelText->setText(slabelcfg.label);
  siteNormalLabelColorBtn->setPaletteBackgroundColor(
      VectorToQColor(slabelcfg.normalColor));
  siteHighlightLabelColorBtn->setPaletteBackgroundColor(
      VectorToQColor(slabelcfg.highlightColor));
  siteNormalLabelScaleEdit->setText(QString::number(slabelcfg.normalScale));
  siteHighlightLabelScaleEdit->setText(QString::number(
                                           slabelcfg.highlightScale));
  siteIDBox->setValue(sstylecfg.id);
  siteStartLevelBox->setValue(sitecfg.minLevel);
  siteEndLevelBox->setValue(sitecfg.maxLevel);

  UpdatePopupWidgets(sitecfg);
  UpdateBalloonStyleWidgets(sitecfg);
  UpdateElevationModeWidgets(fstylecfg);

  updatePolygonVisibility();

  // Keep track of whether we're currently beyond the recommended build levels.
  SetIsBeyondRecommendedBuildLevels();
}

void SelectionRules::updateFeatureWidgets() {
  FeatureConfig &featurecfg = config.displayRules[selected_filter_].feature;
  featureTypeCombo->setCurrentItem(static_cast<int>(featurecfg.featureType));

  switch (featurecfg.featureType) {
    case VectorDefs::PointZ:
      updatePointWidgets();
      break;
    case VectorDefs::LineZ:
      updateLineWidgets();
      break;
    case VectorDefs::PolygonZ:
      updatePolygonWidgets();
      break;
    // The following exceptions because IconZ is used only for map layer.
    case VectorDefs::IconZ:
      throw khException(
          kh::tr("Invalid drawFeatureType VectorDefs::IconZ"));
      break;
  }
}

void SelectionRules::QColorToVector(QColor color, std::vector< unsigned int> * vec) {
  QRgb rgb = color.rgb();
  vec->resize(4);
  (*vec)[0] = qRed(rgb);
  (*vec)[1] = qGreen(rgb);
  (*vec)[2] = qBlue(rgb);
  (*vec)[3] = qAlpha(rgb);
}

QColor SelectionRules::VectorToQColor(const std::vector< unsigned int> & vec) {
  QRgb rgba = (vec[3] << 24) |    // a
              (vec[0] << 16) |    // r
              (vec[1] << 8) |     // g
              vec[2];            // b
  return QColor(rgba);
}

void SelectionRules::BalloonStyleWidgetsToConfig(SiteConfig* cfg) {
  cfg->balloonText = balloon_style_text->text();
  QColorToVector(balloon_bgcolor_btn->paletteBackgroundColor(),
                 &cfg->balloonBgColor);
  QColorToVector(balloon_textcolor_btn->paletteBackgroundColor(),
                 &cfg->balloonFgColor);
  cfg->balloonStyleMode =
      BalloonStyleMode(balloon_style_options->currentItem());
  cfg->balloonInsertHeader = balloon_header_check->isChecked();
  cfg->balloonInsertDirections = balloon_directions_check->isChecked();
}

void SelectionRules::ElevationModeWidgetsToConfig(StyleConfig* cfg) {
  cfg->altitudeMode = static_cast<StyleConfig::AltitudeMode>(
      elev_mode_combo->currentItem());
  cfg->extrude = extrude_check->isChecked();
  cfg->enableCustomHeight = custom_height_box->isChecked();
  cfg->customHeightVariableName = custom_height_var_name_edit->text();
  cfg->customHeightOffset = custom_height_offset_edit->text().toFloat();
  cfg->customHeightScale = custom_height_scale_edit->text().toFloat();
}

void SelectionRules::updatePointConfig() {
  SiteConfig &sitecfg = config.displayRules[selected_filter_].site;
  StyleConfig &sstylecfg = sitecfg.style;
  LabelConfig &slabelcfg = sstylecfg.label;
  IconConfig &siconcfg = sstylecfg.icon;

  sitecfg.decimationMode = static_cast<SiteConfig::PointDecimationMode>(
      pointDecimationBox->selectedId());
  if (sitecfg.decimationMode == SiteConfig::RepSubset) {
    sitecfg.maxQuadCount = pointDecimationMaxQuadEdit->text().toInt();
    sitecfg.minQuadCount = pointDecimationMinQuadEdit->text().toInt();
    sitecfg.decimationRatio = (pointDecimationRatioBox->value())/100.0;
  }
  sitecfg.suppressDuplicateSites = suppressDuplicatesCheck->isChecked();

  sitecfg.enabled = drawLabelCheck->isChecked();
  sstylecfg.id = siteIDBox->value();
  sitecfg.minLevel = siteStartLevelBox->value();
  sitecfg.maxLevel = siteEndLevelBox->value();
  slabelcfg.labelMode = static_cast<LabelConfig::BuildMode>(
      siteLabelModeCombo->currentItem());
  slabelcfg.label = siteLabelText->text();
  QColorToVector(siteNormalLabelColorBtn->paletteBackgroundColor(),
                 &slabelcfg.normalColor);
  QColorToVector(siteHighlightLabelColorBtn->paletteBackgroundColor(),
                 &slabelcfg.highlightColor);
  slabelcfg.normalScale = siteNormalLabelScaleEdit->text().toFloat();
  slabelcfg.highlightScale = siteHighlightLabelScaleEdit->text().toFloat();
  sitecfg.enablePopup = siteIconGroup->isChecked();
  siconcfg.href = siteIcon_.href();
  siconcfg.type = siteIcon_.type();
  QColorToVector(siteNormalIconColorBtn->paletteBackgroundColor(),
                 &siconcfg.normalColor);
  QColorToVector(siteHighlightIconColorBtn->paletteBackgroundColor(),
                 &siconcfg.highlightColor);
  siconcfg.normalScale = siteNormalIconScaleEdit->text().toFloat();
  siconcfg.highlightScale = siteHighlightIconScaleEdit->text().toFloat();
  sitecfg.popupTextMode = static_cast<LabelConfig::BuildMode>(
      popupTextModeCombo->currentItem());
  sitecfg.popupText = sitePopupText->text();

  BalloonStyleWidgetsToConfig(&sitecfg);
  ElevationModeWidgetsToConfig(&sstylecfg);
}

void SelectionRules::updateLineConfig() {
  FeatureConfig &featurecfg = config.displayRules[selected_filter_].feature;
  SiteConfig &sitecfg = config.displayRules[selected_filter_].site;
  StyleConfig &fstylecfg = featurecfg.style;
  StyleConfig &sstylecfg = sitecfg.style;
  LabelConfig &slabelcfg = sstylecfg.label;
  IconConfig &siconcfg = sstylecfg.icon;
  LabelConfig &flabelcfg = fstylecfg.label;
  IconConfig &ficoncfg = fstylecfg.icon;

  fstylecfg.id = featureIDBox->value();
  featurecfg.minLevel = featureStartLevelBox->value();
  featurecfg.maxLevel = featureEndLevelBox->value();
  featurecfg.maxBuildLevel = featureMaxResolutionLevelBox->value();
  featurecfg.maxError = featureMaxErrorEdit->text().toFloat();
  featurecfg.drawAsRoads = drawAsRoadsCheck->isChecked();
  featurecfg.roadLabelType = static_cast<FeatureConfig::RoadLabelType>(
      roadLabelTypeCombo->currentItem());

  QColorToVector(lineLineColorBtn->paletteBackgroundColor(),
                 &fstylecfg.lineColor);


  fstylecfg.lineWidth = lineLineWidthEdit->text().toFloat();

  flabelcfg.labelMode = static_cast<LabelConfig::BuildMode>(
      roadLabelModeCombo->currentItem());
  flabelcfg.label = roadLabelEdit->text();
  flabelcfg.reformat = roadLabelFormatCheck->isChecked();
  QColorToVector(roadLabelColorBtn->paletteBackgroundColor(),
                 &flabelcfg.normalColor);
  flabelcfg.normalScale = roadLabelScaleEdit->text().toFloat();
  ficoncfg.href = roadShield.href();
  ficoncfg.type = roadShield.type();
  ficoncfg.normalScale = roadShieldScaleEdit->text().toFloat();

  sitecfg.position = VectorDefs::LineCenter;
  sitecfg.enabled = drawLabelCheck->isChecked();
  sstylecfg.id = siteIDBox->value();
  sitecfg.minLevel = siteStartLevelBox->value();
  sitecfg.maxLevel = siteEndLevelBox->value();
  slabelcfg.labelMode = static_cast<LabelConfig::BuildMode>(
      siteLabelModeCombo->currentItem());
  slabelcfg.label = siteLabelText->text();
  QColorToVector(siteNormalLabelColorBtn->paletteBackgroundColor(),
                 &slabelcfg.normalColor);
  QColorToVector(siteHighlightLabelColorBtn->paletteBackgroundColor(),
                 &slabelcfg.highlightColor);
  slabelcfg.normalScale = siteNormalLabelScaleEdit->text().toFloat();
  slabelcfg.highlightScale = siteHighlightLabelScaleEdit->text().toFloat();
  sitecfg.enablePopup = siteIconGroup->isChecked();
  siconcfg.href = siteIcon_.href();
  siconcfg.type = siteIcon_.type();
  QColorToVector(siteNormalIconColorBtn->paletteBackgroundColor(),
                 &siconcfg.normalColor);
  QColorToVector(siteHighlightIconColorBtn->paletteBackgroundColor(),
                 &siconcfg.highlightColor);
  siconcfg.normalScale = siteNormalIconScaleEdit->text().toFloat();
  siconcfg.highlightScale = siteHighlightIconScaleEdit->text().toFloat();
  sitecfg.popupTextMode = static_cast<LabelConfig::BuildMode>(
      popupTextModeCombo->currentItem());
  sitecfg.popupText = sitePopupText->text();

  BalloonStyleWidgetsToConfig(&sitecfg);
  ElevationModeWidgetsToConfig(&sstylecfg);
  ElevationModeWidgetsToConfig(&fstylecfg);
}

void SelectionRules::updatePolygonConfig() {
  FeatureConfig &featurecfg = config.displayRules[selected_filter_].feature;
  SiteConfig &sitecfg = config.displayRules[selected_filter_].site;
  StyleConfig &fstylecfg = featurecfg.style;
  StyleConfig &sstylecfg = sitecfg.style;
  LabelConfig &slabelcfg = sstylecfg.label;
  IconConfig &siconcfg = sstylecfg.icon;

  fstylecfg.id = featureIDBox->value();
  featurecfg.minLevel = featureStartLevelBox->value();
  featurecfg.maxLevel = featureEndLevelBox->value();
  featurecfg.maxBuildLevel = featureMaxResolutionLevelBox->value();
  featurecfg.maxError = featureMaxErrorEdit->text().toFloat();
  featurecfg.polygonDrawMode = static_cast<VectorDefs::PolygonDrawMode>(
      polygonDrawModeCombo->currentItem());
  QColorToVector(polygonFillColorBtn->paletteBackgroundColor(),
                 &fstylecfg.polyColor);
  QColorToVector(polygonLineColorBtn->paletteBackgroundColor(),
                 &fstylecfg.lineColor);
  fstylecfg.lineWidth = polygonLineWidthEdit->text().toFloat();

  sitecfg.position = VectorDefs::AreaCenter;
  sitecfg.enabled = drawLabelCheck->isChecked();
  sstylecfg.id = siteIDBox->value();
  sitecfg.minLevel = siteStartLevelBox->value();
  sitecfg.maxLevel = siteEndLevelBox->value();
  slabelcfg.labelMode = static_cast<LabelConfig::BuildMode>(
      siteLabelModeCombo->currentItem());
  slabelcfg.label = siteLabelText->text();
  QColorToVector(siteNormalLabelColorBtn->paletteBackgroundColor(),
                 &slabelcfg.normalColor);
  QColorToVector(siteHighlightLabelColorBtn->paletteBackgroundColor(),
                 &slabelcfg.highlightColor);
  slabelcfg.normalScale = siteNormalLabelScaleEdit->text().toFloat();
  slabelcfg.highlightScale = siteHighlightLabelScaleEdit->text().toFloat();
  sitecfg.enablePopup = siteIconGroup->isChecked();
  siconcfg.href = siteIcon_.href();
  siconcfg.type = siteIcon_.type();
  QColorToVector(siteNormalIconColorBtn->paletteBackgroundColor(),
                 &siconcfg.normalColor);
  QColorToVector(siteHighlightIconColorBtn->paletteBackgroundColor(),
                 &siconcfg.highlightColor);
  siconcfg.normalScale = siteNormalIconScaleEdit->text().toFloat();
  siconcfg.highlightScale = siteHighlightIconScaleEdit->text().toFloat();
  sitecfg.popupTextMode = static_cast<LabelConfig::BuildMode>(
      popupTextModeCombo->currentItem());
  sitecfg.popupText = sitePopupText->text();

  BalloonStyleWidgetsToConfig(&sitecfg);
  ElevationModeWidgetsToConfig(&sstylecfg);
  ElevationModeWidgetsToConfig(&fstylecfg);
}

void SelectionRules::doUpdateConfig(
    VectorDefs::FeatureDisplayType feature_type) {
  applyQueryChanges();

  switch (feature_type) {
    case VectorDefs::PointZ:
      updatePointConfig();
      break;
    case VectorDefs::LineZ:
      updateLineConfig();
      break;
    case VectorDefs::PolygonZ:
      updatePolygonConfig();
      break;
    // The following exceptions because IconZ is used only for map layer.
    case VectorDefs::IconZ:
      throw khException(
          kh::tr("Invalid drawFeatureType VectorDefs::IconZ"));
      break;
  }
}

void SelectionRules::updateConfig() {
  if (selected_filter_ == -1)
    return;

  FeatureConfig &featurecfg = config.displayRules[ selected_filter_ ].feature;
  VectorDefs::FeatureDisplayType featureType =
      static_cast<VectorDefs::FeatureDisplayType>(
          featureTypeCombo->currentItem());
  featurecfg.featureType = featureType;

  doUpdateConfig(featurecfg.featureType);
}

void SelectionRules::updateLastConfig() {
  if (selected_filter_ == -1)
    return;

  assert(last_feature_type_idx_ != -1);

  FeatureConfig &featurecfg = config.displayRules[ selected_filter_ ].feature;
  assert(static_cast<int>(featurecfg.featureType) == last_feature_type_idx_);

  doUpdateConfig(featurecfg.featureType);
}

void SelectionRules::applyQueryChanges() {
  FilterConfig &filtercfg = config.displayRules[ selected_filter_ ].filter =
      queryScroller->getConfig();
  filtercfg.matchScript = scriptFilterEdit->text();
  filtercfg.match = (FilterConfig::MatchType) matchLogicCombo->currentItem();
}

void SelectionRules::accept() {
  // update the current display rule config
  updateConfig();

  // update layer config stuff
  config.allowFeatureDuplication = featureDuplicationCheck->isChecked();
  config.allowEmptyLayer = emptyLayerCheck->isChecked();

  SelectionRulesBase::accept();
}

void SelectionRules::editScriptFilter() {
  QString script = scriptFilterEdit->text();
  QStringList contextScripts = layer_->GetExternalContextScripts();
  if (!config.layerContextScript.isEmpty()) {
    contextScripts.push_back(config.layerContextScript);
  }
  if (ScriptEditor::Run(this, layer_->GetSharedSource(),
                        script, ScriptEditor::Expression,
                        contextScripts)) {
    scriptFilterEdit->setText(script);
  }
}

void SelectionRules::compileAndAccept() {
  // update the current display rule config
  updateConfig();

  // gte the common pieces once
  QStringList contextScripts = layer_->GetExternalContextScripts();
  if (!config.layerContextScript.isEmpty()) {
    contextScripts.push_back(config.layerContextScript);
  }
  gstHeaderHandle recordHeader = layer_->GetSourceAttr();

  // Check each filter for a JS expression
  for (std::vector<DisplayRuleConfig>::const_iterator it =
           config.displayRules.begin(); it != config.displayRules.end(); ++it) {
    if ((it->filter.match == FilterConfig::JSExpression) &&
        !it->filter.matchScript.isEmpty()) {
      QString compilationError;
      if (!gstRecordJSContextImpl::CompileCheck(recordHeader, contextScripts,
                                               it->filter.matchScript,
                                               compilationError)) {
        QMessageBox::critical(this, kh::tr("JavaScript Error"),
                              kh::tr("JavaScript Error (%1 - filter):\n%2")
                              .arg(it->name)
                              .arg(compilationError),
                              kh::tr("OK"), 0, 0, 0);
        return;
      }
    }
  }

  // check each display rule for JS labels & popups
  for (std::vector<DisplayRuleConfig>::const_iterator it =
           config.displayRules.begin(); it != config.displayRules.end(); ++it) {
    if (it->feature.HasRoadLabelJS()) {
      QString compilationError;
      if (!gstRecordJSContextImpl::CompileCheck(recordHeader, contextScripts,
                                                it->feature.style.label.label,
                                                compilationError)) {
        QMessageBox::critical(this, kh::tr("JavaScript Error"),
                              kh::tr("JavaScript Error (%1 - label):\n%2")
                              .arg(it->name)
                              .arg(compilationError),
                              kh::tr("OK"), 0, 0, 0);
        return;
      }
    }

    if (it->site.HasLabelJS()) {
      QString compilationError;
      if (!gstRecordJSContextImpl::CompileCheck(recordHeader, contextScripts,
                                                it->site.style.label.label,
                                                compilationError)) {
        QMessageBox::critical(this, kh::tr("JavaScript Error"),
                              kh::tr("JavaScript Error (%1 - label):\n%2")
                              .arg(it->name)
                              .arg(compilationError),
                              kh::tr("OK"), 0, 0, 0);
        return;
      }
    }

    if (it->site.HasPopupTextJS()) {
      QString compilationError;
      if (!gstRecordJSContextImpl::CompileCheck(recordHeader, contextScripts,
                                                it->site.popupText,
                                                compilationError)) {
        QMessageBox::critical(this, kh::tr("JavaScript Error"),
                              kh::tr("JavaScript Error (%1 - popup):\n%2")
                              .arg(it->name)
                              .arg(compilationError),
                              kh::tr("OK"), 0, 0, 0);
        return;
      }
    }
  }

  // check each display rule for balloonText with embedded quote
  for (std::vector<DisplayRuleConfig>::const_iterator it =
           config.displayRules.begin();
        it != config.displayRules.end(); ++it) {
    if (it->site.enabled &&
        it->site.enablePopup &&
        (it->site.balloonText.indexOf('"') != -1)) {
        QMessageBox::critical(this, kh::tr("Error"),
            kh::tr("BalloonStyle Text for (%1) contains a \"\nThis is not allowed.")
            .arg(it->name),
            kh::tr("OK"), 0, 0, 0);
        return;
    }
  }

  accept();
}

void SelectionRules::labelModeChanged(int newMode) {
  siteLabelText->setText(QString());
}

void SelectionRules::popupLabelModeChanged(int newMode) {
  sitePopupText->setText(QString());
}

void SelectionRules::roadLabelModeChanged(int newMode) {
  roadLabelEdit->setText(QString());
}

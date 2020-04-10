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

//

#ifndef FUSION_FUSIONUI_TEXTSTYLE_H__
#define FUSION_FUSIONUI_TEXTSTYLE_H__
#include <Qt/qobjectdefs.h>
#include <Qt/qglobal.h>
#include <Qt/qobject.h>
#include <Qt/qpixmap.h>
#include <Qt/qlabel.h>
#include <Qt/qpushbutton.h>
#include <memory>
#include <autoingest/.idl/storage/MapSubLayerConfig.h>
#include <autoingest/.idl/MapTextStyle.h>
#include "WidgetControllers.h"
#include <Qt/qwidget.h>
#include "textstylebase.h"
#include <Qt/qsharedpointer.h>

class FontDef {
 public:
  QString name;
  std::vector<MapTextStyleConfig::FontWeight> weights;

  FontDef(const QString &name_,
          const std::vector<MapTextStyleConfig::FontWeight> &weights_)
      : name(name_), weights(weights_) { }
};

namespace maprender {
  class FontInfo;
}

class TextStyleBase;
class TextStyle : public QWidget {
  Q_OBJECT

 public:
  QSharedPointer<TextStyleBase> base;
  TextStyle(QWidget *parent,
            const MapTextStyleConfig &config_);
  virtual ~TextStyle(void) = default;
  const MapTextStyleConfig& Config(void) const { return config; }

  // from QDialog
  virtual void accept();

  // from TextStyleBase
  virtual void WidgetChanged();
  virtual void SavedClicked(int);
  virtual void StoreStyle(QWidget* btn);

  static void ShowMissingFontsDialog(
      QWidget* widget, const std::set<maprender::FontInfo>& missing_fonts);

 public slots:
  void FontChanged(int);

 private:
  void GeneratePreview();
  void SyncToWidgets(void);
  void SyncToConfig(void);
  void UpdateWeightCombo(int fontPos);
  void UpdateFontCombos(void);

  static const unsigned int kMaxSavedStyles = 10;
  MapTextStyleConfig config;
  std::vector<FontDef> fonts;
  std::vector<bool> haveSave;
  std::vector<MapTextStyleConfig> savedConfigs;
  QPixmap pixmap;

  WidgetControllerManager manager;

  MapTextStyleSet orig_text_styles;
};


// ****************************************************************************
// ***  TextStyleButtonController
// ****************************************************************************
class TextStyleButtonController : public WidgetController
{
  Q_OBJECT

 public:
  static void Create(WidgetControllerManager &manager,
                     QPushButton *button_,
                     MapTextStyleConfig *config_);
 public slots:
  void clicked();

 public:
  virtual void SyncToConfig(void);
 protected:
  virtual void SyncToWidgetsImpl(void);

 private:
  TextStyleButtonController(WidgetControllerManager &manager,
                            QPushButton *button_,
                            MapTextStyleConfig *config_);
  QPushButton *button;
  MapTextStyleConfig *config;
  MapTextStyleConfig  workingConfig;
};

#endif // FUSION_FUSIONUI_TEXTSTYLE_H__

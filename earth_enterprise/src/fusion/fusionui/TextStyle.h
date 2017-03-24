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

//

#ifndef FUSION_FUSIONUI_TEXTSTYLE_H__
#define FUSION_FUSIONUI_TEXTSTYLE_H__

#include "textstylebase.h"
#include <qpixmap.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <autoingest/.idl/storage/MapSubLayerConfig.h>
#include <autoingest/.idl/MapTextStyle.h>
#include "WidgetControllers.h"

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

class TextStyle : public TextStyleBase {
  Q_OBJECT

 public:
  TextStyle(QWidget *parent,
            const MapTextStyleConfig &config_);
  virtual ~TextStyle(void);
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

  static const uint kMaxSavedStyles = 10;
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


// ****************************************************************************
// ***  Preview Label
// ****************************************************************************
class TextPreviewLabel : public QLabel {
  Q_OBJECT

 public:
  TextPreviewLabel(QWidget* parent, const char* name);

  // inherited from QWidget
  // support drag
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseMoveEvent(QMouseEvent* event);

  void UpdateConfig(const MapTextStyleConfig& config);
 
 private:
  bool dragging_;
  MapTextStyleConfig config_;
};


// ****************************************************************************
// ***  Style Save Button
// ****************************************************************************

class StyleSaveButton : public QPushButton {
  Q_OBJECT

 public:
  StyleSaveButton(QWidget* parent, const char* name);

  // inherited from QWidget
  // support drop
  virtual void dragEnterEvent(QDragEnterEvent* event);
  virtual void dropEvent(QDropEvent* event);
  virtual void dragLeaveEvent(QDragLeaveEvent* event);

 signals:
  void StyleChanged(QWidget* btn);
};

#endif // FUSION_FUSIONUI_TEXTSTYLE_H__

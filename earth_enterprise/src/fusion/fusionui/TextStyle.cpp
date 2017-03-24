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

//

#include "TextStyle.h"
#include <qcombobox.h>
#include <qspinbox.h>
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qimage.h>
#include <qdragobject.h>
#include <qpixmap.h>
#include <qmessagebox.h>
#include <SkFontHost.h>

#include <common/geInstallPaths.h>
#include <common/khFileUtils.h>
#include <common/khstl.h>
#include <gst/maprender/TextRenderer.h>
#include <gst/maprender/SGLHelps.h>

void TextStyle::ShowMissingFontsDialog(
  QWidget* widget, const std::set<maprender::FontInfo>& missing_fonts) {
    std::string message = "The following fonts are not found in ";
  message.append(khComposePath(kGESharePath, "fonts/fontlist"));
  std::set<maprender::FontInfo>::const_iterator const end = missing_fonts.end();
  for (std::set<maprender::FontInfo>::const_iterator i = missing_fonts.begin();
       i != end; ++i) {
    message.append("\n    " + i->name_);
    if (i->style_ == SkTypeface::kBold) {
      message.append("-Bold");
    } else if (i->style_ == SkTypeface::kItalic) {
      message.append("-Italic");
    } else if (i->style_ == SkTypeface::kBoldItalic) {
      message.append("-Bold-Italic");
    }
  }
  message.append("\nUsing default Sans font instead");
  QMessageBox::critical(widget, "Error", message, "OK", 0, 0, 0);
}


// ****************************************************************************
// ***  TextStyle
// ****************************************************************************
TextStyle::TextStyle(QWidget *parent,
                     const MapTextStyleConfig &config_) :
    TextStyleBase(parent),
    config(config_),
    haveSave(kMaxSavedStyles, false),
    savedConfigs(kMaxSavedStyles),
    manager(this)
{
  // Populate list with avilable fonts
  const std::map<maprender::FontInfo, SkTypeface*>& font_map = maprender::FontInfo::GetFontMap();
  const std::map<maprender::FontInfo, SkTypeface*>::const_iterator end = font_map.end();
  for (std::map<maprender::FontInfo, SkTypeface*>::const_iterator i = font_map.begin();
       i != end;) {
    std::vector<MapTextStyleConfig::FontWeight> weights;
    const std::string& font_name = i->first.name_;
    do {
      switch (i->first.style_) {
        case SkTypeface::kNormal:
          weights.push_back(MapTextStyleConfig::Regular);
          break;
        case SkTypeface::kBold:
          weights.push_back(MapTextStyleConfig::Bold);
          break;
        case SkTypeface::kItalic:
          weights.push_back(MapTextStyleConfig::Italic);
          break;
        case SkTypeface::kBoldItalic:
          weights.push_back(MapTextStyleConfig::BoldItalic);
          break;
      }
    } while (++i != end && font_name == i->first.name_);

    fonts.push_back(FontDef(font_name, weights));
  }

  // populate font combo with valid values from fontlist
  font_combo->clear();
  for (uint i = 0; i < fonts.size(); ++i) {
    font_combo->insertItem(fonts[i].name);
  }
  font_combo->setEnabled(true);

  // Create the standard widget controllers
  ColorButtonController::Create(manager, color_button, &config.color);
  SpinBoxController<uint>::Create(manager, size_spin, &config.size, 6, 100);
  {
    WidgetControllerManager *boxManager =
      CheckableController<QGroupBox>::Create(manager, outline_button_group,
                                             &config.drawOutline);
    ColorButtonController::Create(*boxManager, outline_color_button,
                                  &config.outlineColor);
    FloatEditController::Create(*boxManager, outline_thickness_edit,
                                &config.outlineThickness, 0.0, 5.0, 1);
  }

  // Do the manual stuff for my font combos
  UpdateFontCombos();

  // Generate the initial preview
  GeneratePreview();

  // hook up the signals/slots
  connect(&manager,    SIGNAL(widgetChanged()), this, SLOT(WidgetChanged()));
  connect(&manager,    SIGNAL(widgetTextChanged()),
          this, SLOT(WidgetChanged()));
  connect(font_combo,  SIGNAL(activated(int)),  this, SLOT(FontChanged(int)));
  connect(style_combo, SIGNAL(activated(int)),  this, SLOT(WidgetChanged()));

  if (orig_text_styles.Load()) {
    std::set<maprender::FontInfo> missing_fonts;
    for (std::map<uint, MapTextStyleConfig>::iterator it =
         orig_text_styles.configs.begin();
         it != orig_text_styles.configs.end(); ++it) {
      if (it->first > kMaxSavedStyles)
        continue;
      haveSave[it->first] = true;
      savedConfigs[it->first] = it->second;
      maprender::FontInfo::CheckTextStyleSanity(&savedConfigs[it->first],
                                                &missing_fonts);

      QButton* button = saved_buttongroup->find(it->first);
      if (button) {
        button->setPixmap(
          maprender::TextStyleToPixmap(
          savedConfigs[it->first], button->paletteBackgroundColor(),
          12 /* fixedSize */));
      }
    }
    if (!missing_fonts.empty()) {
      ShowMissingFontsDialog(parent, missing_fonts);
    }
  } else {
    QMessageBox::warning(
        parent, tr("Error"),
        tr("Unable to load saved text styles.\n") +
        tr("Check console for more details."),
        tr("OK"), QString::null, QString::null);
  }
}

TextStyle::~TextStyle(void) {
}

void TextStyle::accept() {
  MapTextStyleSet new_text_styles;
  for (uint i = 0; i < kMaxSavedStyles; ++i) {
    if (haveSave[i]) {
      new_text_styles.configs[i] = savedConfigs[i];
    }
  }
  if (new_text_styles != orig_text_styles) {
    if (!new_text_styles.Save()) {
      QMessageBox::warning(
          this, tr("Error"),
          tr("Unable to save text styles.\n") +
          tr("Check console for more details."),
          tr("OK"), QString::null, QString::null);
    }
  }
  TextStyleBase::accept();
}

void
TextStyle::FontChanged(int pos)
{
  UpdateWeightCombo(pos);
  WidgetChanged();
}

void
TextStyle::WidgetChanged(void)
{
  SyncToConfig();
  GeneratePreview();
}

void
TextStyle::SavedClicked(int pos)
{
  if (haveSave[pos]) {
    if (savedConfigs[pos] != config) {
      config = savedConfigs[pos];
      SyncToWidgets();
      GeneratePreview();
    }
  }
}

void TextStyle::StoreStyle(QWidget* btn) {
  int id = saved_buttongroup->id(static_cast<QButton*>(btn));
  haveSave[id] = true;
  savedConfigs[id] = config;
}

void
TextStyle::UpdateWeightCombo(int fontPos)
{
  // populate the style (weight) combo
  style_combo->clear();
  int weightPos = 0;
  for (uint i = 0 ; i < fonts[fontPos].weights.size(); ++i) {
    if (fonts[fontPos].weights[i] == config.weight) {
      weightPos = i;
    }
    style_combo->insertItem(ToString(fonts[fontPos].weights[i]));
  }
  style_combo->setCurrentItem(weightPos);
  if (fonts[fontPos].weights.size() > 1) {
    style_combo->setEnabled(true);
  } else {
    style_combo->setEnabled(false);
  }
}

void
TextStyle::UpdateFontCombos(void)
{
  // Find which font is specified in the config
  uint pos = 0;
  for (; pos < fonts.size(); ++pos) {
    if (config.font == fonts[pos].name)
      break;
  }
  // if not found, fall back to the first one
  if (pos == fonts.size()) {
    pos = 0;
  }

  // set the font widget
  font_combo->setCurrentItem(pos);

  UpdateWeightCombo(pos);
}


void
TextStyle::SyncToWidgets(void)
{
  manager.SyncToWidgets();

  UpdateFontCombos();
}

void
TextStyle::SyncToConfig(void)
{
  manager.SyncToConfig();

  // get the configs from the font combos
  int fontPos = font_combo->currentItem();
  int weightPos = style_combo->currentItem();
  config.font = fonts[fontPos].name;
  config.weight = fonts[fontPos].weights[weightPos];
}

void
TextStyle::GeneratePreview(void)
{
  preview_label->UpdateConfig(config);
}



// ****************************************************************************
// ***  TextStyleButtonController
// ****************************************************************************
namespace {

void
TextStyleToButton(QPushButton *button, const MapTextStyleConfig &config)
{
  button->setPixmap(maprender::TextStyleToPixmap(
                        config, button->paletteBackgroundColor(),
                        12 /* fixedSize */));
}

}




void
TextStyleButtonController::clicked(void)
{
  TextStyle textStyle(button, workingConfig);
  if (textStyle.exec() == QDialog::Accepted) {
    if (textStyle.Config() != workingConfig) {
      workingConfig = textStyle.Config();
      TextStyleToButton(button, workingConfig);
      EmitChanged();
    }
  }
}

TextStyleButtonController::TextStyleButtonController(
    WidgetControllerManager &manager,
    QPushButton *button_,
    MapTextStyleConfig *config_) :
    WidgetController(manager),
    button(button_),
    config(config_),
    workingConfig(*config)
{
  TextStyleToButton(button, workingConfig);
  connect(button, SIGNAL(clicked()), this, SLOT(clicked()));
}


void
TextStyleButtonController::SyncToConfig(void)
{
  *config = workingConfig;
}

void
TextStyleButtonController::SyncToWidgetsImpl(void)
{
  workingConfig = *config;
  TextStyleToButton(button, workingConfig);
}

void
TextStyleButtonController::Create(WidgetControllerManager &manager,
                                  QPushButton *button_,
                                  MapTextStyleConfig *config_)
{
  (void) new TextStyleButtonController(manager, button_, config_);
}


// ****************************************************************************
// ***  Preview Label
// ****************************************************************************

TextPreviewLabel::TextPreviewLabel(QWidget* parent, const char* name)
  : QLabel(parent, name),
    dragging_(false) {
  setFrameShape(QLabel::LineEditPanel);
  setFrameShadow(QLabel::Sunken);
  setScaledContents(false);
  setAlignment(int(QLabel::AlignCenter));
}

void TextPreviewLabel::mousePressEvent(QMouseEvent* event) {
  QLabel::mousePressEvent(event);
  dragging_ = true;
}

void TextPreviewLabel::mouseMoveEvent(QMouseEvent* event) {
  if (dragging_) {
    QPixmap preview_pix = maprender::TextStyleToPixmap(
        config_, paletteBackgroundColor(),
        12 /* fixedSize */);
    QImageDrag* drag = new QImageDrag(preview_pix.convertToImage(), this);
    drag->setPixmap(preview_pix);
    drag->dragCopy();
    dragging_ = false;
  }
}

void TextPreviewLabel::UpdateConfig(const MapTextStyleConfig& config) {
  setPixmap(maprender::TextStyleToPixmap(
      config,
      paletteBackgroundColor()));
  config_ = config;
}

// ****************************************************************************
// ***  Style Save Button
// ****************************************************************************

StyleSaveButton::StyleSaveButton(QWidget* parent, const char* name)
  : QPushButton(parent, name) {
}

void StyleSaveButton::dragEnterEvent(QDragEnterEvent* event) {
  if (QImageDrag::canDecode(event)) {
    event->accept();
    setDown(true);
  }
}

void StyleSaveButton::dragLeaveEvent(QDragLeaveEvent* event) {
  setDown(false);
}

void StyleSaveButton::dropEvent(QDropEvent* event) {
  QPixmap pixmap;
  if (QImageDrag::decode(event, pixmap))
    setPixmap(pixmap);
  emit StyleChanged(this);
}

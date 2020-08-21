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
#include <Qt/qobjectdefs.h>
#include "TextStyle.h"
#include "StyleSave.h"
#include "TextPreviewLabel.h"
#include <Qt/q3combobox.h>
#include <Qt/qspinbox.h>
#include <Qt/qbuttongroup.h>
#include <Qt/q3button.h>
#include <Qt/qlabel.h>
#include <Qt/qpushbutton.h>
#include <Qt/qgroupbox.h>
#include <Qt/qimage.h>
#include <Qt/q3dragobject.h>
#include <Qt/qpixmap.h>
#include <Qt/qmessagebox.h>
#include <SkFontHost.h>
#include "khException.h"
#include <common/geInstallPaths.h>
#include <common/khFileUtils.h>
#include <common/khstl.h>
#include <gst/maprender/TextRenderer.h>
#include <gst/maprender/SGLHelps.h>
#include <Qt/qevent.h>
using QImageDrag = Q3ImageDrag;
using QButton = Q3Button;


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
  QMessageBox::critical(widget, "Error", message.c_str(), "OK", 0, 0, 0);
}


// ****************************************************************************
// ***  TextStyle
// ****************************************************************************
TextStyle::TextStyle(QWidget *parent,
                     const MapTextStyleConfig &config_) :
    QWidget(parent),
    base(new TextStyleBase),
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

    fonts.push_back(FontDef(font_name.c_str(), weights));
  }

  // populate font combo with valid values from fontlist
  base->font_combo->clear();
  for (unsigned int i = 0; i < fonts.size(); ++i) {
    base->font_combo->insertItem(fonts[i].name);
  }
  base->font_combo->setEnabled(true);

  // Create the standard widget controllers
  ColorButtonController::Create(manager, base->color_button, &config.color);
  SpinBoxController< unsigned int>::Create(manager, base->size_spin, &config.size, 6, 100);
  {
    WidgetControllerManager *boxManager =
      CheckableController<QGroupBox>::Create(manager, base->outline_button_group,
                                             &config.drawOutline);
    ColorButtonController::Create(*boxManager, base->outline_color_button,
                                  &config.outlineColor);
    FloatEditController::Create(*boxManager, base->outline_thickness_edit,
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
  connect(base->font_combo,  SIGNAL(activated(int)),  this, SLOT(FontChanged(int)));
  connect(base->style_combo, SIGNAL(activated(int)),  this, SLOT(WidgetChanged()));

  if (orig_text_styles.Load()) {
    std::set<maprender::FontInfo> missing_fonts;
    for (const auto& it : orig_text_styles.configs) {
      if (it.first > kMaxSavedStyles)
        continue;
      haveSave[it.first] = true;
      savedConfigs[it.first] = it.second;
      maprender::FontInfo::CheckTextStyleSanity(&savedConfigs[it.first],
                                                &missing_fonts);

      QButton* button = static_cast<QButton*>(base->saved_buttongroup->find(it.first));
      if (button) {
        button->setPixmap(
          maprender::TextStyleToPixmap(
          savedConfigs[it.first], button->paletteBackgroundColor(),
          12 /* fixedSize */));
      }
    }
    if (!missing_fonts.empty()) {
      ShowMissingFontsDialog(parent, missing_fonts);
    }
  } else {
    QMessageBox::warning(
        parent, kh::tr("Error"),
        kh::tr("Unable to load saved text styles.\n") +
        kh::tr("Check console for more details."),
        kh::tr("OK"), QString::null, QString::null);
  }
}

void TextStyle::accept() {
  MapTextStyleSet new_text_styles;
  for (unsigned int i = 0; i < kMaxSavedStyles; ++i) {
    if (haveSave[i]) {
      new_text_styles.configs[i] = savedConfigs[i];
    }
  }
  if (new_text_styles != orig_text_styles) {
    if (!new_text_styles.Save()) {
      QMessageBox::warning(
          this, kh::tr("Error"),
          kh::tr("Unable to save text styles.\n") +
          kh::tr("Check console for more details."),
          kh::tr("OK"), QString::null, QString::null);
    }
  }
  //TextStyleBase::accept();
  base->accept();
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
  int id = base->saved_buttongroup->id(static_cast<QButton*>(btn));
  haveSave[id] = true;
  savedConfigs[id] = config;
}

void
TextStyle::UpdateWeightCombo(int fontPos)
{
  // populate the style (weight) combo
  base->style_combo->clear();
  int weightPos = 0;
  for (unsigned int i = 0 ; i < fonts[fontPos].weights.size(); ++i) {
    if (fonts[fontPos].weights[i] == config.weight) {
      weightPos = i;
    }
    // need index number
    base->style_combo->insertItem(ToString(fonts[fontPos].weights[i]).c_str());
  }
  base->style_combo->setCurrentItem(weightPos);
  if (fonts[fontPos].weights.size() > 1) {
    base->style_combo->setEnabled(true);
  } else {
    base->style_combo->setEnabled(false);
  }
}

void
TextStyle::UpdateFontCombos(void)
{
  // Find which font is specified in the config
  unsigned int pos = 0;
  for (; pos < fonts.size(); ++pos) {
    if (config.font == fonts[pos].name)
      break;
  }
  // if not found, fall back to the first one
  if (pos == fonts.size()) {
    pos = 0;
  }

  // set the font widget
  base->font_combo->setCurrentItem(pos);

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
  int fontPos = base->font_combo->currentItem();
  int weightPos = base->style_combo->currentItem();
  config.font = fonts[fontPos].name;
  config.weight = fonts[fontPos].weights[weightPos];
}

void
TextStyle::GeneratePreview(void)
{
  base->preview_label->UpdateConfig(config);
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
  if (textStyle.base->exec() == QDialog::Accepted) {
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
  : QLabel(name, parent),
    dragging_(false) {
  setFrameShape(QFrame::StyledPanel);
  setFrameShadow(QLabel::Sunken);
  setScaledContents(false);
  setAlignment(Qt::AlignCenter);
}

TextPreviewLabel::TextPreviewLabel(QDialog* parent)
  : QLabel(parent), dragging_(false)
{
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QLabel::Sunken);
    setScaledContents(false);
    setAlignment(Qt::AlignCenter);
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
  : QPushButton(name, parent) {
}

StyleSaveButton::StyleSaveButton(Q3ButtonGroup* parent)
  : QPushButton(parent)
{}

void StyleSaveButton::dragEnterEvent(QDragEnterEvent* event) {
  if (QImageDrag::canDecode(dynamic_cast<const QMimeSource*>(event))) {
    event->accept();
    setDown(true);
  }
}

void StyleSaveButton::dragLeaveEvent(QDragLeaveEvent* event) {
  setDown(false);
}

void StyleSaveButton::dropEvent(QDropEvent* event) {
  QPixmap pixmap;
  if (QImageDrag::decode(dynamic_cast<const QMimeSource*>(event), pixmap))
    setPixmap(pixmap);
  emit StyleChanged(this);
}

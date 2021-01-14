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

#include "fusion/fusionui/WidgetControllers.h"

#include <Qt/qpushbutton.h>
#include <Qt/qcolordialog.h>
#include <Qt/qgroupbox.h>
#include <Qt/qspinbox.h>
#include <Qt/qlineedit.h>
#include <Qt/q3textedit.h>
#include <Qt/qvalidator.h>
#include <Qt/qdatetime.h>
#include <Qt/qlabel.h>

using QTextEdit = Q3TextEdit;

#include <autoingest/.idl/storage/MapSubLayerConfig.h>

#include "fusion/fusionui/SiteIcons.h"
#include "fusion/fusionui/PixmapManager.h"


// ****************************************************************************
// ***  WidgetController
// ****************************************************************************
WidgetController::DontEmitGuard::DontEmitGuard(WidgetController *wc) :
    wc_(wc)
{
  ++wc_->dontEmit;
}

WidgetController::DontEmitGuard::~DontEmitGuard(void) {
  --wc_->dontEmit;
}

WidgetController::WidgetController(WidgetControllerManager &manager_) :
    manager(&manager_), dontEmit(0)
{
  manager->controllers.push_back(this);
}

void
WidgetController::EmitChanged(void)
{
  if (dontEmit == 0) {
    manager->EmitChanged();
  }
}

void
WidgetController::EmitTextChanged(void)
{
  if (dontEmit == 0) {
    manager->EmitTextChanged();
  }
}

QWidget*
WidgetController::PopupParent(void) {
  return manager->popupParent;
}

// ****************************************************************************
// ***  WidgetControllerManager
// ****************************************************************************
void
WidgetControllerManager::EmitChanged(void)
{
  emit widgetChanged();
}
void
WidgetControllerManager::EmitTextChanged(void)
{
  emit widgetTextChanged();
}



// ****************************************************************************
// ***  ColorButtonController
// ****************************************************************************
ColorButtonController::ColorButtonController(WidgetControllerManager &manager,
                                             QPushButton *button_,
                                             QColor *config_) :
    WidgetController(manager),
    button(button_),
    config(config_)
{
  MySyncToWidgetsImpl();
  connect(button, SIGNAL(clicked()), this, SLOT(clicked()));
}

void
ColorButtonController::clicked(void)
{
  QColor orig_color = button->paletteBackgroundColor();
  QColor new_color = QColor(QColorDialog::getRgba(orig_color.rgb(), 0,
                                                  button));

  if (new_color != orig_color) {
    QPalette palette;
    palette.setColor(QPalette::Button, new_color);
    button->setAutoFillBackground(true);
    button->setPalette(palette);
    button->setFlat(true);
    button->update();

    EmitChanged();
  }
}

void
ColorButtonController::SyncToConfig(void)
{
  *config = button->paletteBackgroundColor();
}

void
ColorButtonController::SyncToWidgetsImpl(void)
{
  MySyncToWidgetsImpl();
}

void
ColorButtonController::MySyncToWidgetsImpl(void)
{
  QPalette palette;
  palette.setColor(QPalette::Button, *config);

  button->setAutoFillBackground(true);
  button->setPalette(palette);
  button->setFlat(true);
  button->update();

}

void ColorButtonController::Create(WidgetControllerManager &manager,
                                   QPushButton *button, QColor *config) {
  (void) new ColorButtonController(manager, button, config);
}

IconButtonController::IconButtonController(WidgetControllerManager &manager,
                                           QPushButton& button,
                                           std::string& icon_href,
                                           IconReference::Type& icon_type) :
    WidgetController(manager),
    button_(button), icon_href_(icon_href), icon_type_(icon_type) {
  MySyncToWidgetsImpl();
  connect(&button_, SIGNAL(clicked()), this, SLOT(Clicked()));
}

void
IconButtonController::Clicked(void)
{
  if (SiteIcons::selectIcon(&button_, PixmapManager::NormalIcon, &shield_)) {
    SetIcon();
  }
}

void
IconButtonController::SetIcon()
{
  const QPixmap pix = thePixmapManager->GetPixmap(shield_,
                                                  PixmapManager::NormalIcon);
  if (!pix.isNull()) button_.setPixmap(pix);
}

void
IconButtonController::SyncToConfig(void)
{
  icon_href_ = shield_.IconRef().href.toUtf8().constData();
  icon_type_ = shield_.IconRef().type;
}

void
IconButtonController::MySyncToWidgetsImpl(void)
{
  shield_ = gstIcon(IconReference(icon_type_, icon_href_.c_str()));
  SetIcon();
}

void
IconButtonController::SyncToWidgetsImpl(void)
{
  MySyncToWidgetsImpl();
}

// ****************************************************************************
// ***  MapIconController
// ****************************************************************************

MapIconController::MapIconController(WidgetControllerManager &manager,
                                     QComboBox *combo_type,
                                     QComboBox *combo_scaling,
                                     QPushButton *button,
                                     QLabel *fill_color_label,
                                     QPushButton *fill_color_button,
                                     QLabel *outline_color_label,
                                     QPushButton *outline_color_button,
                                     MapShieldConfig *config,
                                     QGroupBox *center_label_group_box) :
    WidgetController(manager),
    combo_type_(combo_type),
    combo_scaling_(combo_scaling),
    button_(button),
    fill_color_label_(fill_color_label),
    fill_color_button_(fill_color_button),
    outline_color_label_(outline_color_label),
    outline_color_button_(outline_color_button),
    config_(config),
    center_label_group_box_(center_label_group_box)
{
  ColorButtonController::Create(manager, fill_color_button,
                                &(config_->fill_color_));
  ColorButtonController::Create(manager, outline_color_button,
                                &(config_->box_color_));
  IconButtonController::Create(manager, *button, config->icon_href_,
                               config->icon_type_);
  MySyncToWidgetsImpl();
  connect(combo_type, SIGNAL(activated(int)), this, SLOT(ShieldStyleChanged(int)));
}

void
MapIconController::ShieldStyleChanged(int index)
{
  bool is_icon = (MapShieldConfig::IconStyle == index);
  combo_scaling_->setEnabled(is_icon);
  button_->setEnabled(is_icon);
  fill_color_label_->setEnabled(!is_icon);
  fill_color_button_->setEnabled(!is_icon);
  outline_color_button_->setEnabled(!is_icon);
  outline_color_label_->setEnabled(!is_icon);
  if (!is_icon && center_label_group_box_ != NULL) {
    center_label_group_box_->setChecked(true);
  }
}

void
MapIconController::SyncToConfig(void)
{
  switch (combo_type_->currentItem()) {
    case 0:
      (*config_).style_ = MapShieldConfig::OvalStyle;
      break;
    case 1:
      (*config_).style_ = MapShieldConfig::BoxStyle;
      break;
    case 2:
      (*config_).style_ = MapShieldConfig::IconStyle;
      switch (combo_scaling_->currentItem()) {
        case 0:
          (*config_).scaling_ = MapShieldConfig::IconFixedSizeStyle; break;
        case 1:
          (*config_).scaling_ = MapShieldConfig::IconFixedAspectStyle; break;
        case 2:
          (*config_).scaling_ = MapShieldConfig::IconVariableAspectStyle; break;
      }
  }
}

void
MapIconController::SyncToWidgetsImpl(void)
{
  MySyncToWidgetsImpl();
}

void
MapIconController::MySyncToWidgetsImpl(void)
{
  switch (config_->style_) {
    case MapShieldConfig::OvalStyle:
      combo_type_->setCurrentItem(0); break;
    case MapShieldConfig::BoxStyle:
      combo_type_->setCurrentItem(1); break;
    case MapShieldConfig::IconStyle:
    switch (config_->scaling_) {
      case MapShieldConfig::IconFixedSizeStyle:
        combo_type_->setCurrentItem(2);
        combo_scaling_->setCurrentItem(0);
        break;
      case MapShieldConfig::IconFixedAspectStyle:
        combo_type_->setCurrentItem(2);
        combo_scaling_->setCurrentItem(1);
        break;
      case MapShieldConfig::IconVariableAspectStyle:
        combo_type_->setCurrentItem(2);
        combo_scaling_->setCurrentItem(2);
        break;
    }
  }
  ShieldStyleChanged(combo_type_->currentItem());
}

void MapIconController::Create(WidgetControllerManager &manager,
                               QComboBox *combo_type,
                               QComboBox *combo_scaling,
                               QPushButton *button,
                               QLabel *fill_color_label,
                               QPushButton *fill_color_button,
                               QLabel *outline_color_label,
                               QPushButton *outline_color_button,
                               MapShieldConfig *config,
                               QGroupBox *center_label_group_box) {
  (void) new MapIconController(manager, combo_type, combo_scaling, button,
                               fill_color_label, fill_color_button,
                               outline_color_label, outline_color_button,
                               config, center_label_group_box);
}


// ****************************************************************************
// ***  SpinBoxControllerBase
// ****************************************************************************
SpinBoxControllerBase::SpinBoxControllerBase(
    WidgetControllerManager &manager,
    QSpinBox *spin_,
    int initialValue,
    int minValue, int maxValue) :
    WidgetController(manager),
    spin(spin_)
{
  spin->setMinValue(minValue);
  spin->setMaxValue(maxValue);
  spin->setValue(initialValue);
  connect(spin, SIGNAL(valueChanged(int)), this, SLOT(EmitChanged()));
}

int
SpinBoxControllerBase::GetValue(void) const
{
  return spin->value();
}

void
SpinBoxControllerBase::SetValue(int v)
{
  return spin->setValue(v);
}


// ****************************************************************************
// ***  RangeSpinControllerBase
// ****************************************************************************
RangeSpinControllerBase::RangeSpinControllerBase(
    WidgetControllerManager &manager,
    QSpinBox *minSpin_, QSpinBox *maxSpin_,
    const geRange<int> &initial,
    const geRange<int> &limit) :
    WidgetController(manager),
    minSpin(minSpin_),
    maxSpin(maxSpin_)
{
  minSpin->setMinValue(limit.min);
  maxSpin->setMaxValue(limit.max);

  SetValue(initial);

  connect(minSpin, SIGNAL(valueChanged(int)), this, SLOT(MinChanged(int)));
  connect(maxSpin, SIGNAL(valueChanged(int)), this, SLOT(MaxChanged(int)));
}

void
RangeSpinControllerBase::MinChanged(int val) {
  maxSpin->setMinValue(val);  // Whenever min changes update the clamp for max
  EmitChanged();
}

void
RangeSpinControllerBase::MaxChanged(int val) {
  minSpin->setMaxValue(val);  // Whenever max changes update the clamp for min
  EmitChanged();
}


geRange<int>
RangeSpinControllerBase::GetValue(void) const
{
  return geRange<int>(minSpin->value(), maxSpin->value());
}


// Note: RangeSpinControllerBase which takes input for a range (low, hi)
// has two types of clamping:
// Clamp1 (min_for_low, max_for_hi) and Clamp2 (max_for_low, min_for_hi)
// Clamp1 is during construction time and doesn't change for lifetime.
// Clamp2 is updated, anytime low and hi values are updated so as to avoid an
// invalid range with low > hi.
// Since SetValue is a method to update both low and hi we need to
// 1. update Clamp2
// 2. and then set the value
// Note that doing in reverse order will not work as Clamp2 may disable
// updating the values itself.
// This method safely assumes val.min <= val.max as it is from a valid database.
void
RangeSpinControllerBase::SetValue(const geRange<int> &val)
{
  minSpin->setMaxValue(val.max);
  maxSpin->setMinValue(val.min);
  minSpin->setValue(val.min);
  maxSpin->setValue(val.max);
}


// ****************************************************************************
// ***  FloatEditController
// ****************************************************************************
void FloatEditController::Create(WidgetControllerManager &manager,
                                 QLineEdit *lineEdit, float *config,
                                 float min, float max, int decimals)
{
  (void) new FloatEditController(manager, lineEdit, config,
                                 min, max, decimals);
}

void
FloatEditController::SyncToConfig(void)
{
  *config = lineEdit->text().toFloat();
}

void
FloatEditController::SyncToWidgetsImpl(void)
{
  lineEdit->setText(QString::number(*config));
}

FloatEditController::FloatEditController(WidgetControllerManager &manager,
                                         QLineEdit *lineEdit_, float *config_,
                                         float min, float max, int decimals) :
    WidgetController(manager),
    lineEdit(lineEdit_),
    config(config_)
{
  lineEdit->setText(QString::number(*config));
  connect(lineEdit, SIGNAL(textChanged(const QString&)),
          this, SLOT(EmitTextChanged()));
  lineEdit->setValidator(new QDoubleValidator(min, max, decimals, lineEdit));
}


// ****************************************************************************
// ***  ComboController
// ****************************************************************************
void
ComboController::SyncToConfig(void)
{
  *config = contents[combo->currentItem()].first;
}

void
ComboController::SyncToWidgetsImpl(void)
{
  MySyncToWidgetsImpl();
}

ComboController::ComboController(WidgetControllerManager &manager,
                                 QComboBox *combo_, int *config_,
                                 const ComboContents &contents_) :
    WidgetController(manager),
    combo(combo_),
    config(config_),
    contents(contents_)
{
  // populate the combo
  combo->clear();
  for (unsigned int i = 0; i < contents.size(); ++i) {
    combo->insertItem(contents[i].second);
  }

  MySyncToWidgetsImpl();

  connect(combo, SIGNAL(activated(int)), this, SLOT(EmitChanged()));
}

void
ComboController::MySyncToWidgetsImpl(void)
{
  // Find which entry is in the config
  unsigned int pos = 0;
  for (; pos < contents.size(); ++pos) {
    if (*config == contents[pos].first)
      break;
  }
  // if not found, fall back to the first one
  if (pos == contents.size()) {
    pos = 0;
  }

  combo->setCurrentItem(pos);
}



// ****************************************************************************
// ***  TextWidgetController
// ****************************************************************************
template <class TextWidget>
void TextWidgetController<TextWidget>::SyncToConfig(void) {
  *config_ = text_edit_->text();
}

template <class TextWidget>
void TextWidgetController<TextWidget>::SyncToWidgetsImpl(void) {
  MySyncToWidgetsImpl();
}

template <class TextWidget>
void TextWidgetController<TextWidget>::Create(WidgetControllerManager &manager,
                                              TextWidget *text_edit,
                                              QString *config) {
  (void) new TextWidgetController(manager, text_edit, config);
}


template <class TextWidget>
TextWidgetController<TextWidget>::TextWidgetController(
    WidgetControllerManager &manager,
    TextWidget *text_edit,
    QString *config) :
    WidgetController(manager),
    text_edit_(text_edit),
    config_(config)
{
  MySyncToWidgetsImpl();
  Connect();
}

template <>
void TextWidgetController<QLineEdit>::Connect(void) {
  connect(text_edit_, SIGNAL(textChanged(const QString &)),
          this, SLOT(EmitTextChanged()));
}

template <>
void TextWidgetController<QTextEdit>::Connect(void) {
  connect(text_edit_, SIGNAL(textChanged()), this, SLOT(EmitTextChanged()));
}



template <class TextWidget>
void TextWidgetController<TextWidget>::MySyncToWidgetsImpl(void) {
  text_edit_->setText(*config_);
}

// explicit instantiations
template class TextWidgetController<QLineEdit>;
template class TextWidgetController<QTextEdit>;



// ****************************************************************************
// ***  TextWidgetMetaFieldController
// ****************************************************************************
template <class TextWidget>
void TextWidgetMetaFieldController<TextWidget>::SyncToConfig(void) {
  TextWidgetController<TextWidget>::SyncToConfig();
  if (storage_.isEmpty()) {
    config_->Erase(key_);
  } else {
    config_->SetValue(key_, storage_);
  }
}

template <class TextWidget>
void TextWidgetMetaFieldController<TextWidget>::SyncToWidgetsImpl(void) {
  MySyncToWidgetsImpl();
}


template <class TextWidget>
void TextWidgetMetaFieldController<TextWidget>::Create(
    WidgetControllerManager &manager,
    TextWidget *text_edit,
    khMetaData *config,
    const QString &key) {
  (void) new TextWidgetMetaFieldController(manager, text_edit,
                                           config, key);
}


template <class TextWidget>
TextWidgetMetaFieldController<TextWidget>::TextWidgetMetaFieldController(
    WidgetControllerManager &manager,
    TextWidget *text_edit,
    khMetaData *config,
    const QString &key) :
    QStringStorage(config->GetValue(key)),
    TextWidgetController<TextWidget>(
        manager, text_edit,
        &storage_),
    config_(config),
    key_(key)
{
}

template <class TextWidget>
void TextWidgetMetaFieldController<TextWidget>::MySyncToWidgetsImpl(void) {
  storage_ = config_->GetValue(key_);
  TextWidgetController<TextWidget>::MySyncToWidgetsImpl();
}

// explicit instantiations
template class TextWidgetMetaFieldController<QLineEdit>;
template class TextWidgetMetaFieldController<QTextEdit>;


// ****************************************************************************
// ***  TimestampInserterController
// ****************************************************************************
ButtonHandlerBase::ButtonHandlerBase(QPushButton *button) {
  connect(button, SIGNAL(clicked()), this, SLOT(HandleButtonClick()));
}

template <class TextWidget>
TimestampInserterController<TextWidget>::TimestampInserterController(
    WidgetControllerManager &manager,
    QPushButton *button,
    TextWidget *text_edit) :
    WidgetController(manager),
    ButtonHandlerBase(button),
    text_edit_(text_edit)
{
}

template <class TextWidget>
void TimestampInserterController<TextWidget>::HandleButtonClick(void)
{
  QDateTime now = QDateTime::currentDateTime();
  text_edit_->insert(now.toString("yyyy-MM-dd hh:mm"));
}

template class TimestampInserterController<QLineEdit>;
template class TimestampInserterController<QTextEdit>;

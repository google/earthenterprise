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

#ifndef FUSION_FUSIONUI_WIDGETCONTROLLERS_H__
#define FUSION_FUSIONUI_WIDGETCONTROLLERS_H__

#include <Qt/qobjectdefs.h>
#include <Qt/qobject.h>
#include <khGuard.h>
#include <geRange.h>
#include <Qt/qcombobox.h>
#include <Qt/q3widgetstack.h>
#include <khMetaData.h>
#include <gstSite.h>

class QPushButton;
class QSpinBox;
class QLineEdit;
class QLabel;
class QGroupBox;
class MapShieldConfig;

class WidgetControllerManager;
using QWidgetStack = Q3WidgetStack;

// ****************************************************************************
// ***  WidgetController
// ****************************************************************************
class WidgetController : public QObject {
  Q_OBJECT

  class DontEmitGuard {
   public:
    DontEmitGuard(WidgetController *wc);
    ~DontEmitGuard(void);
   private:
    WidgetController *wc_;
  };
  friend class DontEmitGuard;

 public:
  WidgetController(WidgetControllerManager &manager);
  virtual ~WidgetController(void) { }

  virtual void SyncToConfig(void) = 0;
  void DoSyncToWidgets(void) {
    DontEmitGuard guard(this);
    SyncToWidgetsImpl();
  }

 protected slots:
  void EmitChanged();
  void EmitTextChanged();
  QWidget* PopupParent(void);
  virtual void SyncToWidgetsImpl(void) = 0;

 private:

  WidgetControllerManager *manager;
  int dontEmit;
};


// ****************************************************************************
// ***  WidgetControllerManager
// ****************************************************************************
class WidgetControllerManager : public QObject {
  Q_OBJECT

  friend class WidgetController;

  signals:
  void widgetChanged();
  void widgetTextChanged();

 public:
  WidgetControllerManager(QWidget *popupParent_) :
      popupParent(popupParent_)
  { }

  void SyncToConfig(void) {
    for (unsigned int i = 0; i < controllers.size(); ++i) {
      controllers[i]->SyncToConfig();
    }
  }
  void SyncToWidgets(void) {
    for (unsigned int i = 0; i < controllers.size(); ++i) {
      controllers[i]->DoSyncToWidgets();
    }
  }
  void clear(void) {
    controllers.clear();
  }

 private:
  void EmitChanged(void);
  void EmitTextChanged(void);

  khDeletingVector<WidgetController> controllers;
  QWidget *popupParent;

};


// ****************************************************************************
// ***  ColorButtonController
// ****************************************************************************
class ColorButtonController : public WidgetController {
  Q_OBJECT

 public:
  static void Create(WidgetControllerManager &manager,
                     QPushButton *button, QColor *config);
public slots:
  void clicked();

 public:
  virtual void SyncToConfig(void);
 protected:
  virtual void SyncToWidgetsImpl(void);

 private:
  ColorButtonController(WidgetControllerManager &manager,
                        QPushButton *button_,
                        QColor *config_);
  void MySyncToWidgetsImpl(void);

  QPushButton *button;
  QColor *config;
};

// ****************************************************************************
// ***  IconButtonController
// ****************************************************************************
class IconButtonController : public WidgetController {
  Q_OBJECT

 public:
  static void Create(WidgetControllerManager& manager, QPushButton& button,
                     std::string& icon_href, IconReference::Type& icon_type) {
    (void) new IconButtonController(manager, button, icon_href, icon_type);
  }
public slots:
  void Clicked();
  void SetIcon();

 public:
  virtual void SyncToConfig(void);
 protected:
  virtual void SyncToWidgetsImpl(void);

 private:
  IconButtonController(WidgetControllerManager &manager, QPushButton& button,
                     std::string& icon_href, IconReference::Type& icon_type);
  void MySyncToWidgetsImpl(void);

  QPushButton& button_;
  std::string& icon_href_;
  IconReference::Type& icon_type_;
  gstIcon shield_;
};

// ****************************************************************************
// ***  MapIconController
// ****************************************************************************
class MapIconController : public WidgetController {
  Q_OBJECT

 public:
  static void Create(WidgetControllerManager &manager,
                     QComboBox *combo_type, QComboBox *combo_scaling,
                     QPushButton *button,
                     QLabel *fill_color_label, QPushButton *fill_color_button,
                     QLabel *outline_color_label,
                     QPushButton *outline_color_button,
                     MapShieldConfig *config,
                     QGroupBox *center_label_group_box);
public slots:
  void ShieldStyleChanged(int index);

 public:
  virtual void SyncToConfig(void);
 protected:
  virtual void SyncToWidgetsImpl(void);

 private:
  MapIconController(WidgetControllerManager &manager,
                    QComboBox *combo_type,
                    QComboBox *combo_scaling,
                    QPushButton *button,
                    QLabel *fill_color_label,
                    QPushButton *fill_color_button,
                    QLabel *outline_color_label,
                    QPushButton *outline_color_button,
                    MapShieldConfig *config,
                    QGroupBox *center_label_group_box);
  void MySyncToWidgetsImpl(void);

  QComboBox *combo_type_;
  QComboBox *combo_scaling_;
  QPushButton *button_;
  QLabel *fill_color_label_;
  QPushButton *fill_color_button_;
  QLabel *outline_color_label_;
  QPushButton *outline_color_button_;
  MapShieldConfig *config_;
  QGroupBox *center_label_group_box_;
};


// ****************************************************************************
// ***  SpinBoxControllerBase
// ****************************************************************************
class SpinBoxControllerBase : public WidgetController {
 public:
  virtual void SyncToConfig(void) = 0;

 protected:
  virtual void SyncToWidgetsImpl(void) = 0;
  SpinBoxControllerBase(WidgetControllerManager &manager,
                        QSpinBox *spin_,
                        int initialValue, int minValue, int maxValue);
  int  GetValue(void) const;
  void SetValue(int v);

 private:
  QSpinBox *spin;
};


template <class T>
class SpinBoxController : public SpinBoxControllerBase {
 public:
  template <class Y, class Z>
  static void Create(WidgetControllerManager &manager,
                     QSpinBox *spin_,
                     T *config_, Y minValue, Z maxValue) {
    (void) new SpinBoxController(manager, spin_, config_,
                                 static_cast<T>(minValue),
                                 static_cast<T>(maxValue));
  }

 public:
  virtual void SyncToConfig(void) {
    *config = static_cast<T>(GetValue());
  }

 protected:
  virtual void SyncToWidgetsImpl(void) {
    SetValue(static_cast<T>(*config));
  }

  SpinBoxController(WidgetControllerManager &manager,
                    QSpinBox *spin_,
                    T *config_, T minValue, T maxValue) :
      SpinBoxControllerBase(manager, spin_, static_cast<int>(*config_),
                            static_cast<int>(minValue),
                            static_cast<int>(maxValue)),
      config(config_)
  { }

 private:
  T *config;
};


// ****************************************************************************
// ***  CheckableController
// ****************************************************************************
template <class Widget>
class CheckableController : public WidgetController {
 public:
  static WidgetControllerManager*
  Create(WidgetControllerManager &manager, Widget *widget, bool *config) {
    CheckableController *tmp =
      new CheckableController(manager, widget, config);
    return &tmp->subManager;
  }

  virtual void SyncToConfig(void) {
    *config = widget->isEnabled() && widget->isChecked();
    if (*config) {
      subManager.SyncToConfig();
    }
  }

 protected:
  virtual void SyncToWidgetsImpl(void) {
    widget->setChecked(*config);
    subManager.SyncToWidgets();
  }

  CheckableController(WidgetControllerManager &manager,
                      Widget *widget_, bool *config_) :
      WidgetController(manager), widget(widget_), config(config_),
      subManager(this->PopupParent())
  {
    widget->setChecked(*config);
    connect(widget, SIGNAL(toggled(bool)), this, SLOT(EmitChanged()));
    connect(&subManager, SIGNAL(widgetChanged()),
            this, SLOT(EmitChanged()));
    connect(&subManager, SIGNAL(widgetTextChanged()),
            this, SLOT(EmitTextChanged()));
  }

 private:
  Widget *widget;
  bool   *config;
  WidgetControllerManager subManager;
};


// ****************************************************************************
// ***  RangeSpinController
// ****************************************************************************
class RangeSpinControllerBase : public WidgetController {
  Q_OBJECT

 public:
  virtual void SyncToConfig(void) = 0;

 protected slots:
  void MinChanged(int);
  void MaxChanged(int);

 protected:
  virtual void SyncToWidgetsImpl(void) = 0;
  RangeSpinControllerBase(WidgetControllerManager &manager,
                          QSpinBox *minSpin_, QSpinBox *maxSpin_,
                          const geRange<int> &initial,
                          const geRange<int> &limit_);
  geRange<int>  GetValue(void) const;
  void SetValue(const geRange<int> &val);

 private:
  QSpinBox *minSpin;
  QSpinBox *maxSpin;
};


template <class T>
class RangeSpinController : public RangeSpinControllerBase {
 public:
  static void Create(WidgetControllerManager &manager,
                     QSpinBox *minSpin_, QSpinBox *maxSpin_,
                     geRange<T> *config,
                     const geRange<T> &limit);

  virtual void SyncToConfig(void) {
    *config = geRange<T>(GetValue());
  }

 protected:
  virtual void SyncToWidgetsImpl(void) {
    SetValue(geRange<int>(*config));
  }
  RangeSpinController(WidgetControllerManager &manager,
                      QSpinBox *minSpin_, QSpinBox *maxSpin_,
                      geRange<T> *config_,
                      const geRange<T> &limit) :
      RangeSpinControllerBase(manager, minSpin_, maxSpin_,
                              geRange<int>(*config_),
                              limit),
      config(config_)
  { }

 private:
  geRange<T> *config;
};


template <class T>
void RangeSpinController<T>::Create(WidgetControllerManager &manager,
                                    QSpinBox *minSpin_, QSpinBox *maxSpin_,
                                    geRange<T> *config,
                                    const geRange<T> &limit)
{
  (void) new RangeSpinController(manager, minSpin_, maxSpin_, config, limit);
}


// ****************************************************************************
// ***  FloadEditController
// ****************************************************************************
class FloatEditController : public WidgetController {
 public:
  static void Create(WidgetControllerManager &manager,
                     QLineEdit *lineEdit, float *config,
                     float min, float max, int decimals);

  virtual void SyncToConfig(void);

 private:
  virtual void SyncToWidgetsImpl(void);
  FloatEditController(WidgetControllerManager &manager,
                      QLineEdit *lineEdit_, float *config_,
                      float min, float max, int decimals);

  QLineEdit *lineEdit;
  float     *config;
};


// ****************************************************************************
// ***  ComboController
// ***
// ***  Manage a combo box where you cpecify an 'int' value and a QString for
// ***  each entry in the combo. The config is populated with the value (not
// ***  the position).
// ****************************************************************************
typedef std::vector<std::pair<int, QString> > ComboContents;

class ComboController : public WidgetController {
 public:
  static void Create(WidgetControllerManager &manager,
                     QComboBox *combo, int *config,
                     const ComboContents &contents)
  {
    (void) new ComboController(manager, combo, config, contents);
  }

  virtual void SyncToConfig(void);

 protected:
  virtual void SyncToWidgetsImpl(void);
  ComboController(WidgetControllerManager &manager,
                  QComboBox *combo_, int *config_,
                  const ComboContents &contents_);

  void MySyncToWidgetsImpl(void);

  QComboBox *combo;
  int       *config;
  ComboContents contents;
};



// ****************************************************************************
// ***  EnumComboController
// ****************************************************************************
template <class Enum>
class EnumComboController : public ComboController {
 public:
  static void Create(WidgetControllerManager &manager,
                     QComboBox *combo, Enum *config,
                     const ComboContents &contents)
  {
    (void) new EnumComboController(manager, combo, config, contents);
  }

  virtual void SyncToConfig(void) {
    ComboController::SyncToConfig();
    *config = static_cast<Enum>(intConfig);
  }

 protected:
  virtual void SyncToWidgetsImpl(void) {
    intConfig = static_cast<int>(*config);
    ComboController::DoSyncToWidgets();
  }
  EnumComboController(WidgetControllerManager &manager,
                      QComboBox *combo_, Enum *config_,
                      const ComboContents &contents_) :
      ComboController(manager, combo_,
                      (intConfig = static_cast<int>(*config_),
                       &intConfig),
                       contents_),
      config(config_)
      // intConfig already initialized
  {
  }

  Enum     *config;
  int      intConfig;
};


// ****************************************************************************
// ***  EnumStackController
// ****************************************************************************
template <class Enum>
class EnumStackController : public ComboController {
 public:
  static std::vector<WidgetControllerManager*>
  Create(WidgetControllerManager &manager,
                     QComboBox *combo, QWidgetStack *stack,
                     Enum *config, const ComboContents &contents)
  {
    EnumStackController *tmp =
      new EnumStackController(manager, combo, stack, config, contents);
    return tmp->subManagers;
  }

  virtual void SyncToConfig(void) {
    ComboController::SyncToConfig();
    *config = static_cast<Enum>(intConfig);
    subManagers[intConfig]->SyncToConfig();
  }
 protected:
  virtual void SyncToWidgetsImpl(void) {
    intConfig = static_cast<int>(*config);
    ComboController::DoSyncToWidgets();
    for (unsigned int i = 0; i < subManagers.size(); ++i) {
      subManagers[i]->SyncToWidgets();
    }
    stack->raiseWidget(intConfig);
  }

  EnumStackController(WidgetControllerManager &manager,
                      QComboBox *combo_, QWidgetStack *stack_,
                      Enum *config_,
                      const ComboContents &contents_) :
      ComboController(manager, combo_,
                      (intConfig = static_cast<int>(*config_),
                       &intConfig),
                       contents_),
      config(config_),
      // intConfig already initialized
      stack(stack_)
  {
    subManagerOwner.reserve(contents_.size());
    subManagers.reserve(contents_.size());
    for (unsigned int i = 0; i < contents_.size(); ++i) {
      subManagerOwner.push_back(
          new WidgetControllerManager(this->PopupParent()));
      connect(subManagerOwner.back(), SIGNAL(widgetChanged()),
              this, SLOT(EmitChanged()));
      connect(subManagerOwner.back(), SIGNAL(widgetTextChanged()),
              this, SLOT(EmitTextChanged()));
      subManagers.push_back(subManagerOwner.back());
    }
    stack->raiseWidget(intConfig);
    connect(combo, SIGNAL(activated(int)), stack, SLOT(raiseWidget(int)));
  }

  Enum     *config;
  int      intConfig;
  QWidgetStack *stack;
  khDeletingVector<WidgetControllerManager> subManagerOwner;
  std::vector<WidgetControllerManager*> subManagers;
};


// ****************************************************************************
// ***  ConstSetterController
// ****************************************************************************
template <class Type>
class ConstSetterController : public WidgetController {
 public:
  static void
  Create(WidgetControllerManager &manager, Type *config, const Type &value) {
    (void) new ConstSetterController(manager, config, value);
  }

  virtual void SyncToConfig(void) {
    *config = value;
  }
 protected:
  virtual void SyncToWidgetsImpl(void) {
    // nothing to do
  }

  ConstSetterController(WidgetControllerManager &manager,
                        Type *config_, const Type &value_) :
      WidgetController(manager), config(config_), value(value_)
  {
  }

 private:
  Type *config;
  Type value;
};


// ****************************************************************************
// ***  TextWidgetController
// ****************************************************************************
template <class TextWidget>
class TextWidgetController : public WidgetController
{
 public:
  virtual void SyncToConfig(void);
 protected:
  virtual void SyncToWidgetsImpl(void);
  void MySyncToWidgetsImpl(void);

 public:
  static void Create(WidgetControllerManager &manager,
                     TextWidget *text_edit,
                     QString *config);

 protected:
  TextWidgetController(WidgetControllerManager &manager,
                       TextWidget *text_edit,
                       QString *config);

 private:
  void Connect(void);

  TextWidget     *text_edit_;
  QString        *config_;
};


// ****************************************************************************
// ***  TextWidgetMetaFieldController
// ****************************************************************************
class QStringStorage {
 protected:
  QStringStorage(const QString &s) : storage_(s) { }

  QString storage_;
};

template <class TextWidget>
class TextWidgetMetaFieldController : public QStringStorage,
                                      public TextWidgetController<TextWidget> {
 public:
  virtual void SyncToConfig(void);

 protected:
  virtual void SyncToWidgetsImpl(void);

 public:
  static void Create(WidgetControllerManager &manager,
                     TextWidget *text_edit,
                     khMetaData *config,
                     const QString &key);

 private:
  TextWidgetMetaFieldController(WidgetControllerManager &manager,
                                TextWidget *text_edit,
                                khMetaData *config,
                                const QString &key);
  void MySyncToWidgetsImpl(void);

  khMetaData *config_;
  const QString key_;
};


// ****************************************************************************
// ***  TimestampInserterController
// ****************************************************************************
class ButtonHandlerBase : public QObject {
  Q_OBJECT

 public:
  ButtonHandlerBase(QPushButton *button);

 protected slots:
  virtual void HandleButtonClick() { }
};



template <class TextWidget>
class TimestampInserterController : public WidgetController,
                                    public ButtonHandlerBase
{

 public:
  virtual void SyncToConfig(void) { /* noop */}
 protected:
  virtual void SyncToWidgetsImpl(void) { /* noop */ }

 public:
  static void Create(WidgetControllerManager &manager,
                     QPushButton *button,
                     TextWidget *text_edit) {
    (void) new TimestampInserterController(manager, button, text_edit);
  }

  virtual void HandleButtonClick(void);

 private:
  TimestampInserterController(WidgetControllerManager &manager,
                              QPushButton *button,
                              TextWidget *text_edit);

  TextWidget *text_edit_;
};




#endif // FUSION_FUSIONUI_WIDGETCONTROLLERS_H__

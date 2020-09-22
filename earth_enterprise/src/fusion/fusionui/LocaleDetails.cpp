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



#include <Qt/q3header.h>
using QHeader = Q3Header;
#include <Qt/qcombobox.h>
#include <Qt/qgroupbox.h>
#include <Qt/qlineedit.h>
#include <Qt/qvalidator.h>
#include <Qt/qregexp.h>
#include <Qt/qpushbutton.h>
#include <Qt/qlayout.h>
#include <Qt/qlabel.h>
#include <Qt/qstringlist.h>
#include <Qt/qmessagebox.h>
#include <Qt/q3popupmenu.h>
#include <Qt/qpainter.h>
#include <Qt/qcursor.h>

#include <gstIconManager.h>
#include <gstSource.h>

#include "LocaleDetails.h"
#include "LocaleManager.h"
#include "PixmapManager.h"
#include "SiteIcons.h"
#include "SourceFileDialog.h"
#include "ValidLayerNames.h"
#include "Preferences.h"
#include "LayerLegendWidget.h"

using QTable = Q3Table;
using QTableItem = Q3TableItem;
using QScrollView = Q3ScrollView;
using QPopupMenu = Q3PopupMenu;

// ****************************************************************************
// ***  ItemBase
// ****************************************************************************
class ItemBase : public QTableItem {
 public:
  ItemBase(QTable* table, QTableItem::EditType et, bool is_defaultable);
  virtual ~ItemBase() { }

  void SetUseDefault(bool mode);
  bool IsDefault() const { return use_default_; }

  bool IsDefaultable() const { return is_defaultable_; }

  // from QTableItem
  virtual void paint(QPainter* p, const QColorGroup& cg, const QRect& cr,
                     bool sel);

  virtual void MouseButtonPressed(int button, const QPoint& pos) = 0;
  virtual void ContextMenu(const QPoint& pos) = 0;
  virtual void SyncToWidgets(void) = 0;
  virtual void SyncToConfig(void) = 0;

 private:
  bool use_default_;
  bool is_defaultable_;
 protected:
  bool specialRender;
};

ItemBase::ItemBase(QTable* table, QTableItem::EditType et,
                   bool is_defaultable)
    : QTableItem(table, et, QString("")),
      is_defaultable_(is_defaultable),
      specialRender(false) {
  SetUseDefault(IsDefaultable());
}

void ItemBase::SetUseDefault(bool mode) {
  use_default_ = mode;
  if (mode) {
    setText("<default>");
    setPixmap(QPixmap());
  } else {
    setText(QString(""));
  }
}

void ItemBase::paint(
    QPainter* p, const QColorGroup& cg, const QRect& cr, bool sel) {
  if (use_default_) {
    QFont f = p->font();
    f.setBold(true);
    p->setFont(f);
    QColorGroup ngrp = cg;
    ngrp.setColor(QColorGroup::Text, QColor("red"));
    QTableItem::paint(p, ngrp, cr, sel);
  } else if (specialRender) {
    QFont f = p->font();
    f.setBold(true);
    p->setFont(f);
    QTableItem::paint(p, cg, cr, sel);
  } else {
    QTableItem::paint(p, cg, cr, sel);
  }
}


// ****************************************************************************
// ***  IconItem
// ****************************************************************************
class IconItem : public ItemBase {
 public:
  IconItem(QTable* table, bool is_defaultable,
           Defaultable<IconReference> &config) :
      ItemBase(table, QTableItem::Never, is_defaultable),
      config_(config) {
    SyncToWidgets();
  }
  virtual void MouseButtonPressed(int button, const QPoint& pos);
  virtual void ContextMenu(const QPoint& pos);

  void SyncToWidgets(void) {
    if (config_.UseDefault()) {
      SetUseDefault(true);
    } else {
      SetIcon(config_);
    }
  }
  void SyncToConfig(void) {
    if (IsDefault()) {
      config_.SetUseDefault();
    } else {
      config_ = icon_;
    }
  }
 private:
  void SetIcon(const IconReference& icon);
  IconReference icon_;

  Defaultable<IconReference> &config_;
};

void IconItem::SetIcon(const IconReference& icon) {
  SetUseDefault(false);
  icon_ = icon;
  QPixmap pix = thePixmapManager->GetPixmap(icon, PixmapManager::LayerIcon);
  setPixmap(pix);
}

void IconItem::ContextMenu(const QPoint& pos) {
  enum { OPT_DEFAULT, OPT_CHOOSE_ICON };

  QPopupMenu menu(table());
  if (IsDefaultable())
    menu.insertItem("Use default value", OPT_DEFAULT);
  menu.insertItem("Choose Icon", OPT_CHOOSE_ICON);

  switch (menu.exec(pos)) {
    case OPT_DEFAULT:
      SetUseDefault(true);
      break;
    case OPT_CHOOSE_ICON:
      MouseButtonPressed(0, pos);
      break;
  }
}

void IconItem::MouseButtonPressed(int button, const QPoint& pos) {

  gstIcon new_icon;
  if (!IsDefault()) {
    new_icon = gstIcon(icon_);
  }
  if (SiteIcons::selectIcon(table(), PixmapManager::LayerIcon, &new_icon)) {
    SetIcon(new_icon.IconRef());
  }
}


// ****************************************************************************
// ***  LookAtItem
// ****************************************************************************
class LookAtItem : public ItemBase {
 public:
  LookAtItem(QTable* table, bool is_defaultable,
             Defaultable<QString> &config);
  virtual void MouseButtonPressed(int button, const QPoint& pos);
  virtual void ContextMenu(const QPoint& pos);

  void SyncToWidgets(void) {
    if (config_.UseDefault()) {
      SetUseDefault(true);
    } else {
      SetLookAt(config_);
    }
  }
  void SyncToConfig(void) {
    if (IsDefault()) {
      config_.SetUseDefault();
    } else {
      config_ = text();
    }
  }
 private:
  void SetLookAt(const QString& str);

  Defaultable<QString> &config_;
};

LookAtItem::LookAtItem(QTable* table, bool is_defaultable,
                       Defaultable<QString> &config)
    : ItemBase(table, QTableItem::Never, is_defaultable), config_(config) {
  SyncToWidgets();
}

void LookAtItem::SetLookAt(const QString& str) {
  SetUseDefault(false);
  setText(str);
}

void LookAtItem::ContextMenu(const QPoint& pos) {
  enum { OPT_DEFAULT, OPT_SPECIFY_LOOKAT, OPT_REMOVE_LOOKAT };

  QPopupMenu menu(table());
  if (IsDefaultable())
    menu.insertItem("Use default value", OPT_DEFAULT);
  menu.insertItem("Specify Camera Pose", OPT_SPECIFY_LOOKAT);
  menu.insertItem("Remove Camera Pose", OPT_REMOVE_LOOKAT);
  if (text().isEmpty())
    menu.setItemEnabled(OPT_REMOVE_LOOKAT, false);

  switch (menu.exec(pos)) {
    case OPT_DEFAULT:
      SetUseDefault(true);
      break;
    case OPT_SPECIFY_LOOKAT:
      MouseButtonPressed(0, pos);
      break;
    case OPT_REMOVE_LOOKAT:
      SetLookAt("");
      break;
  }
}

void LookAtItem::MouseButtonPressed(int button, const QPoint& pos) {
  SourceFileDialog* dialog = SourceFileDialog::self();

  if (dialog->exec() != QDialog::Accepted)
    return;

  QStringList files = dialog->selectedFiles();
  QStringList::Iterator it;

  QString sourceFileName = files[0];

  gstSource src(sourceFileName.ascii());
  if (src.Open() != GST_OKAY) {
    QMessageBox::critical(table(), "Error" ,
                      QObject::tr("Unable to open the source file."),
                      QObject::tr("OK"), 0, 0, 0);
    return;
  }
  if (src.NumFeatures(0) != 1) {
    QMessageBox::critical(table(), "Error" ,
                      QObject::tr("The source has more than one feature.\n"
                                  "Multiple features are not supported at this time.\n"
                                  "Please use Google Earth to create a placemark."),
                      QObject::tr("OK"), 0, 0, 0);
    return;
  }

  try {
    gstRecordHandle record = src.GetAttributeOrThrow(0, 0);
    gstValue* value = record->FieldByName(QString("LookAt"));
    if (value) {
      QString lookAtString = value->ValueAsString().c_str();
      SetLookAt(lookAtString);
    } else {
      QMessageBox::critical(
          table(), "Error" ,
          QObject::tr("Cannot find an attribute named 'LookAt'."),
          QObject::tr("OK"), 0, 0, 0);
    }
  } catch (const khException &e) {
    QMessageBox::critical(table(), "Error" ,
                          QObject::tr("Cannot load attributes:\n") +
                          e.qwhat(),
                          QObject::tr("OK"), 0, 0, 0);
  } catch (const std::exception &e) {
    QMessageBox::critical(table(), "Error" ,
                          QObject::tr("Cannot load attributes:\n") +
                          e.what(),
                          QObject::tr("OK"), 0, 0, 0);
  } catch (...) {
    QMessageBox::critical(table(), "Error" ,
                          QObject::tr("Cannot load attributes:\n") +
                          "Unknown error",
                          QObject::tr("OK"), 0, 0, 0);
  }
}


// ****************************************************************************
// ***  TextItem
// ****************************************************************************
class TextItem : public ItemBase {
 public:
  typedef const QValidator* (*ValidatorFactory)(void);


  TextItem(QTable* table, bool is_defaultable,
           Defaultable<QString> &config,
           ValidatorFactory = 0);
  virtual void MouseButtonPressed(int button, const QPoint& pos);
  virtual void ContextMenu(const QPoint& pos);
  virtual void setContentFromEditor(QWidget* w);
  virtual QWidget* createEditor() const;

  void SyncToWidgets(void) {
    if (config_.UseDefault()) {
      SetUseDefault(true);
    } else {
      SetText(config_);
    }
  }
  void SyncToConfig(void) {
    if (IsDefault()) {
      config_.SetUseDefault();
    } else {
      config_ = text();
    }
  }
 protected:
  Defaultable<QString>* GetRefToConfig() { return &config_; }
 private:
  void SetText(const QString& text);
  Defaultable<QString> &config_;
 private:
  ValidatorFactory validator_factory_;
};


template<int bottom, int top>
class IntItem : public TextItem {
 public:
  static const QValidator* CreateIntValidator() {
    return new QIntValidator(bottom, top, NULL);
  };

  IntItem(QTable* table, bool is_defaultable, Defaultable<int>* config) :
      TextItem(table, is_defaultable,
               *(new Defaultable<QString>(config->UseDefault(),
                                          QString::number(config->GetValue()))),
               CreateIntValidator),
      config_(*config) {
      text_ = GetRefToConfig();
  }

  virtual void SyncToWidgets(void) {
    if (config_.UseDefault()) {
      SetUseDefault(true);
    } else {
      SetUseDefault(false);
      text_->GetMutableValue().setNum(config_.GetValue());
      setText(*text_);
    }
  }

  virtual void SyncToConfig(void) {
    if (IsDefault()) {
      config_.SetUseDefault();
      text_->SetUseDefault();
    } else {
      *text_ = text();
      config_ = text_->GetValue().toInt();
    }
  }

  ~IntItem() { delete text_; }

 private:

  Defaultable<QString>* text_;
  Defaultable<int>& config_;
  DISALLOW_IMPLICIT_CONSTRUCTORS(IntItem);
};


TextItem::TextItem(QTable* table, bool is_defaultable,
                   Defaultable<QString> &config,
                   ValidatorFactory validator_factory) :
    ItemBase(table, QTableItem::OnTyping, is_defaultable),
    config_(config),
    validator_factory_(validator_factory) {
  SyncToWidgets();
}

void TextItem::MouseButtonPressed(int button, const QPoint& pos) {
}

void TextItem::ContextMenu(const QPoint& pos) {
  if (!IsDefaultable())
    return;

  enum { OPT_DEFAULT };

  QPopupMenu menu(table());
  menu.insertItem("Use default value", OPT_DEFAULT);

  if (menu.exec(pos) == OPT_DEFAULT)
    SetUseDefault(true);
}

QWidget* TextItem::createEditor() const {
  QLineEdit* line_edit = new QLineEdit(table()->viewport());
  if (validator_factory_) {
    line_edit->setValidator((*validator_factory_)());
  }
  if (!IsDefault())
    line_edit->setText(text());
  return line_edit;
}

void TextItem::setContentFromEditor(QWidget* w) {
  QLineEdit* line_edit = dynamic_cast<QLineEdit*>(w);
  if (line_edit) {
    SetText(line_edit->text());
  }
}

void TextItem::SetText(const QString& text) {
  SetUseDefault(false);
  setText(text);
}


// ****************************************************************************
// ***  BoolItem
// ****************************************************************************
class BoolItem : public ItemBase {
 public:
  BoolItem(QTable* table, bool is_defaultable,
           Defaultable<bool> &config,
           const QString &true_str = QString("On"),
           const QString &false_str = QString("Off"));
  virtual void MouseButtonPressed(int button, const QPoint& pos);
  virtual void ContextMenu(const QPoint& pos);

  void SyncToWidgets(void) {
    if (config_.UseDefault()) {
      SetUseDefault(true);
    } else {
      SetBool(config_);
    }
  }
  void SyncToConfig(void) {
    if (IsDefault()) {
      config_.SetUseDefault();
    } else {
      config_ = GetBool();
    }
  }
 private:
  void SetBool(bool b);
  bool GetBool(void) { return text() == true_str_; }
  Defaultable<bool> &config_;
  QString true_str_;
  QString false_str_;
};

BoolItem::BoolItem(QTable* table, bool is_defaultable,
                   Defaultable<bool> &config,
                   const QString &true_str,
                   const QString &false_str)
    : ItemBase(table, QTableItem::Never, is_defaultable),
      config_(config),
      true_str_(true_str), false_str_(false_str)
{
  SyncToWidgets();
}

void BoolItem::SetBool(bool b) {
  SetUseDefault(false);
  QTableItem::setText(b ? true_str_ : false_str_);
}

void BoolItem::MouseButtonPressed(int button, const QPoint& pos) {
  if (IsDefault()) {
    SetBool(true);
  } else if (GetBool()) {
    SetBool(false);
  } else {
    if (!IsDefaultable()) {
      SetBool(true);
    } else {
      SetUseDefault(true);
    }
  }

  // why do we need this?
  table()->updateCell(row(), col());
}

void BoolItem::ContextMenu(const QPoint& pos) {
  enum { OPT_DEFAULT, OPT_ON, OPT_OFF };

  QPopupMenu menu(table());
  if (IsDefaultable()) {
    menu.insertItem("Use default value", OPT_DEFAULT);
    menu.insertSeparator();
  }
  menu.insertItem(true_str_, OPT_ON);
  menu.insertItem(false_str_, OPT_OFF);

  switch (menu.exec(pos)) {
    case OPT_DEFAULT:
      SetUseDefault(true);
      break;
    case OPT_ON:
      SetBool(true);
      break;
    case OPT_OFF:
      SetBool(false);
      break;
  }
}


// ****************************************************************************
// ***  LocaleColumn
// ****************************************************************************
class LocaleColumn {
 public:
  LocaleColumn(QTable *table,
               const std::vector<LocaleDetails::FieldEnum> &field_list,
               const QString& locale_name,
               const LocaleConfig &config,
               bool is_defaultable = true);
  void SyncToConfig(void);

  const LocaleConfig& GetConfig(void) const { return config_; }
  const QString& LocaleName(void) const { return locale_name_; }

  QString locale_name_;
  LocaleConfig config_;
  std::vector<ItemBase*> items_;
};



// ****************************************************************************
// ***  LocaleDetails
// ****************************************************************************
LocaleDetails::LocaleDetails(
    WidgetControllerManager &manager,
    LayerLegendWidget *layer_legend,
    EditMode edit_mode,
    LocaleConfig *default_locale_config,
    LocaleMap    *locale_map_config) :
    WidgetController(manager),
    table_(layer_legend->GetTable()),
    default_locale_config_(default_locale_config),
    locale_map_config_(locale_map_config),
    filtering_(true)

{

  // populate field list from edit_mode
  switch (edit_mode) {
    case LayerMode:
    case KMLLayerMode:
      field_list_.push_back(FIELD_ICON);
      field_list_.push_back(FIELD_NAME);
      field_list_.push_back(FIELD_INIT_STATE);
      field_list_.push_back(FIELD_DESC);
      field_list_.push_back(FIELD_LOOKAT);
      field_list_.push_back(FIELD_KMLURL);
      field_list_.push_back(FIELD_REQUIRED_VERSION);

      if (Preferences::GoogleInternal) {
        field_list_.push_back(FIELD_PROBABILITY);
        field_list_.push_back(FIELD_SAVE_LOCKED);
        field_list_.push_back(FIELD_REQUIRED_USER_AGENT);
        field_list_.push_back(FIELD_REQUIRED_CLIENT_CAPABILITIES);
        field_list_.push_back(FIELD_REQUIRED_VRAM);
        field_list_.push_back(FIELD_CLIENT_CONFIG_SCRIPT_NAME);
        field_list_.push_back(FIELD_3D_DATA_CHANNEL_BASE);
        field_list_.push_back(FIELD_REPLICA_DATA_CHANNEL_BASE);
      }
      break;
    case FolderMode:
      field_list_.push_back(FIELD_ICON);
      field_list_.push_back(FIELD_NAME);

      // The following line was removed in fusion 3.0 - 3.0.2 and reenabled in 3.0.3.
      // See comments in LayerConfigImpl's AfterLoad method for details!
      field_list_.push_back(FIELD_INIT_STATE);

      field_list_.push_back(FIELD_DESC);
      field_list_.push_back(FIELD_LOOKAT);
      field_list_.push_back(FIELD_KMLURL);
      field_list_.push_back(FIELD_REQUIRED_VERSION);

      if (Preferences::GoogleInternal) {
        field_list_.push_back(FIELD_REQUIRED_USER_AGENT);
        field_list_.push_back(FIELD_REQUIRED_VRAM);
        field_list_.push_back(FIELD_PROBABILITY);
      }
      break;
    case ImageryLayerMode:
    case TerrainLayerMode:
      field_list_.push_back(FIELD_ICON);
      field_list_.push_back(FIELD_NAME);
      field_list_.push_back(FIELD_INIT_STATE);
      field_list_.push_back(FIELD_LOOKAT);
      break;
    case MapLayerMode:
      field_list_.push_back(FIELD_ICON);
      field_list_.push_back(FIELD_NAME);
      field_list_.push_back(FIELD_INIT_STATE);
      break;
  };

  // configure the check box
  layer_legend->filter_check->setChecked(filtering_);
  connect(layer_legend->filter_check, SIGNAL(toggled(bool)),
          this, SLOT(FilterToggled(bool)));


  // configure the table widget
  table_->setNumRows(field_list_.size());
  table_->setNumCols(0);
  table_->setShowGrid(false);
  table_->setReadOnly(false);
  table_->setSorting(false);
  table_->setDragEnabled(true);
  table_->setColumnMovingEnabled(false);
  table_->setRowMovingEnabled(false);
  table_->setHScrollBarMode(QScrollView::Auto);
  table_->setVScrollBarMode(QScrollView::Auto);
  table_->setSelectionMode(QTable::NoSelection);



  QHeader* header = table_->verticalHeader();
  for (unsigned int i = 0; i < field_list_.size(); ++i) {
    int row = i;
    switch (field_list_[i]) {
      case FIELD_ICON:
        header->setLabel(row, "Icon");
        break;
      case FIELD_NAME:
        header->setLabel(row, "Name");
        break;
      case FIELD_INIT_STATE:
        header->setLabel(row, "Initial State");
        break;
      case FIELD_DESC:
        header->setLabel(row, "Description");
        break;
      case FIELD_LOOKAT:
        header->setLabel(row, "LookAt");
        break;
      case FIELD_REQUIRED_VERSION:
        header->setLabel(row, "Required Client Version");
        break;
      case FIELD_KMLURL:
        header->setLabel(row, "KML URL");
        break;
      case FIELD_REQUIRED_VRAM:
        header->setLabel(row, "Required VRAM");
        break;
      case FIELD_PROBABILITY:
        header->setLabel(row, "Probability");
        break;
      case FIELD_SAVE_LOCKED:
        header->setLabel(row, "Save Locked");
        break;
      case FIELD_REQUIRED_USER_AGENT:
        header->setLabel(row, "Required User Agent");
        break;
      case FIELD_REQUIRED_CLIENT_CAPABILITIES:
        header->setLabel(row, "Required Client Capabilities");
        break;
      case FIELD_CLIENT_CONFIG_SCRIPT_NAME:
        header->setLabel(row, "Client Config Script Name");
        break;
      case FIELD_3D_DATA_CHANNEL_BASE:
        header->setLabel(row, "3D Data Channel Base");
        break;
      case FIELD_REPLICA_DATA_CHANNEL_BASE:
        header->setLabel(row, "Replica Data Channel Base");
        break;
    }
    table_->adjustRow(row);
  }


  connect(table_, SIGNAL(clicked(int, int, int, const QPoint&)),
          this, SLOT(CellPressed(int, int, int, const QPoint&)));
  connect(table_, SIGNAL(contextMenuRequested(int, int, const QPoint&)),
          this, SLOT(ContextMenu(int, int, const QPoint&)));


  MySyncToWidgetsImpl();
}

LocaleDetails::~LocaleDetails(void) {
  // defined in .cpp so we have all the sub classes defined
}

void LocaleDetails::MySyncToWidgetsImpl(void) {
  locale_cols_.clear();
  table_->setNumCols(0);

  // populate locale_cols from config
  AddLocale("default", *default_locale_config_, false /* is_defaultable */);
  std::vector<QString> supported_locales = LocaleManager::SupportedLocales();
  LocaleConfig empty_locale;
  for (std::vector<QString>::iterator it = supported_locales.begin();
       it != supported_locales.end(); ++it) {
    LocaleIterator found = locale_map_config_->find(*it);
    if ((found != locale_map_config_->end()) &&
        (found->second != empty_locale)) {
      AddLocale(found->first, found->second);
    } else if (!filtering_) {
      AddLocale(*it, LocaleConfig());
    }
  }
}

void LocaleDetails::SyncToConfig(void) {
  assert(locale_cols_.size() > 0);

  locale_cols_[0]->SyncToConfig();
  *default_locale_config_ = locale_cols_[0]->GetConfig();

  LocaleConfig empty_locale;
  locale_map_config_->clear();
  for (unsigned int i = 1; i < locale_cols_.size(); ++i) {
    locale_cols_[i]->SyncToConfig();
    LocaleConfig new_locale = locale_cols_[i]->GetConfig();
    if (new_locale != empty_locale) {
      (*locale_map_config_)[locale_cols_[i]->LocaleName()] = new_locale;
    }
  }
}


void LocaleDetails::CellPressed(int row, int col, int btn, const QPoint& pos) {
  if (btn == Qt::RightButton || row == -1)
    return;

  ItemBase* locale_item = dynamic_cast<ItemBase*>(table_->item(row, col));
  if (locale_item) {
    // the pressed() signal pos coordinates are local to this widget,
    // unlike the global ones sent by the ContextMenuRequest() signal
    // so map them to the global coordinate system for use by QPopupMenu
    locale_item->MouseButtonPressed(btn, table_->mapToGlobal(pos));
  }
}

void LocaleDetails::ContextMenu(int row, int col, const QPoint& pos) {
  if (row == -1)
    return;
  ItemBase* locale_item = dynamic_cast<ItemBase*>(table_->item(row, col));
  if (locale_item) {
    locale_item->ContextMenu(pos);
  }
}


void LocaleDetails::FilterToggled(bool on) {
  filtering_ = on;
  SyncToConfig();
  SyncToWidgetsImpl();
}

void LocaleDetails::AddLocale(const QString& name,
                              const LocaleConfig& config,
                              bool is_defaultable) {
  locale_cols_.push_back(TransferOwnership(
                             new LocaleColumn(table_, field_list_,
                                              name, config,
                                              is_defaultable)));
}


void LocaleDetails::Create(WidgetControllerManager &manager,
                           LayerLegendWidget *layer_legend,
                           EditMode edit_mode,
                           LocaleConfig *default_locale_config,
                           LocaleMap *locale_map_config)
{
  (void) new LocaleDetails(manager, layer_legend, edit_mode,
                           default_locale_config, locale_map_config);
}



// ****************************************************************************
// ***  LocaleColumn
// ****************************************************************************
LocaleColumn::LocaleColumn(
    QTable *table,
    const std::vector<LocaleDetails::FieldEnum> &field_list,
    const QString& locale_name,
    const LocaleConfig &config,
    bool is_defaultable) :
    locale_name_(locale_name),
    config_(config)
{
  int col = table->numCols();
  table->setNumCols(col + 1);

  table->horizontalHeader()->setLabel(col, locale_name);

  for (unsigned int i = 0; i < field_list.size(); ++i) {
    int row = i;
    switch (field_list[i]) {
      case LocaleDetails::FIELD_ICON:
        items_.push_back(new IconItem(table, is_defaultable,
                                      config_.icon_));
        break;
      case LocaleDetails::FIELD_NAME:
        items_.push_back(new TextItem(table, is_defaultable,
                                      config_.name_, NewLayerNameValidator));
        break;
      case LocaleDetails::FIELD_INIT_STATE:
        items_.push_back(new BoolItem(table, is_defaultable,
                                      config_.is_checked_,
                                      "On", "Off"));
        break;
      case LocaleDetails::FIELD_DESC:
        items_.push_back(new TextItem(table, is_defaultable,
                                      config_.desc_));
        break;
      case LocaleDetails::FIELD_LOOKAT:
        items_.push_back(new LookAtItem(table, is_defaultable,
                                        config_.look_at_));
        break;
      case LocaleDetails::FIELD_REQUIRED_VERSION:
        items_.push_back(new TextItem(table, is_defaultable,
                                      config_.required_client_version_));
        break;
      case LocaleDetails::FIELD_KMLURL:
        items_.push_back(new TextItem(table, is_defaultable,
                                      config_.kml_layer_url_));
        break;
      case LocaleDetails::FIELD_REQUIRED_VRAM:
        items_.push_back(new TextItem(table, is_defaultable,
                                      config_.required_client_vram_));
        break;
      case LocaleDetails::FIELD_PROBABILITY:
        items_.push_back(new TextItem(table, is_defaultable,
                                      config_.probability_));
        break;
      case LocaleDetails::FIELD_SAVE_LOCKED:
        items_.push_back(new BoolItem(table, is_defaultable,
                                      config_.save_locked_,
                                      "Locked", "Unlocked"));
        break;
      case LocaleDetails::FIELD_REQUIRED_USER_AGENT:
        items_.push_back(new TextItem(table, is_defaultable,
                                      config_.required_user_agent_));
        break;
      case LocaleDetails::FIELD_REQUIRED_CLIENT_CAPABILITIES:
        items_.push_back(new TextItem(table, is_defaultable,
                                      config_.required_client_capabilities_));
        break;
      case LocaleDetails::FIELD_CLIENT_CONFIG_SCRIPT_NAME:
        items_.push_back(new TextItem(table, is_defaultable,
                                      config_.client_config_script_name_));
        break;
      case LocaleDetails::FIELD_3D_DATA_CHANNEL_BASE:
        items_.push_back(new IntItem<-1, INT_MAX>(table, is_defaultable,
                                     &config_.diorama_data_channel_base_));
        break;
      case LocaleDetails::FIELD_REPLICA_DATA_CHANNEL_BASE:
        items_.push_back(new IntItem<-1, INT_MAX>(
            table, is_defaultable,
            &config_.diorama_replica_data_channel_base_));
        break;
    }
    table->setItem(row, col, items_.back());
    table->adjustRow(row);
    table->adjustColumn(col);
  }
}


void LocaleColumn::SyncToConfig(void) {
  for (unsigned int i = 0; i < items_.size(); ++i) {
    items_[i]->SyncToConfig();
  }
}

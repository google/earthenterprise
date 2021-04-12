// Copyright 2020 the Open GEE Contributors.
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

#include <Qt/qaction.h>
#include <Qt/qframe.h>
#include <Qt/qimage.h>
#include <Qt/qlabel.h>
#include <Qt/qlayout.h>
#include <Qt/qmenubar.h>
#include <Qt/qmessagebox.h>
#include <Qt/qpixmap.h>
#include <Qt/qpushbutton.h>
#include <Qt/qtooltip.h>
#include <Qt/qvariant.h>
#include <Qt/qwhatsthis.h>
#include <khException.h>
#include <Qt/qobject.h>
#include <Qt/qmainwindow.h>
#include <Qt/qwidget.h>
#include "AssetBase.h"
#include "AssetChooser.h"
#include "AssetNotes.h"
#include "AssetManager.h"

#include <fusionui/.idl/layoutpersist.h>
#include <autoingest/khAssetManagerProxy.h>


QString AssetBase::untitled_name(QObject::tr("Untitled"));

AssetBase::AssetBase(QWidget* parent)
  : QMainWindow(parent, 0, Qt::WType_TopLevel) {
  setFocusPolicy(Qt::TabFocus);
  setCentralWidget(new QWidget(this));
  QGridLayout* asset_base_layout = new QGridLayout(centralWidget(), 2, 1, 11, 6);

  main_frame_ = new QFrame(centralWidget());
  main_frame_->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)7,
      (QSizePolicy::SizeType)7, 0, 0, main_frame_->sizePolicy().hasHeightForWidth()));
  main_frame_->setFrameShape(QFrame::NoFrame);
  main_frame_->setFrameShadow(QFrame::Raised);
  main_frame_layout_ = new QGridLayout(main_frame_, 1, 1, 0, 6);

  asset_base_layout->addWidget(main_frame_, 0, 0);

  error_msg_label_ = new QLabel(centralWidget());
  asset_base_layout->addWidget(error_msg_label_, 1, 0);

  // actions
  save_action_ = new QAction(this);
  save_action_->setIconSet(QIconSet(QPixmap(":/filesave.png")));
  saveas_action_ = new QAction(this);
  saveas_action_->setIconSet(QIconSet(QPixmap(":/filesaveas.png")));
  build_action_ = new QAction(this);
  //  build_action_->setIconSet(QIconSet(QPixmap::fromMimeSource("notes.png")));
  savebuild_action_ = new QAction(this);
  close_action_ = new QAction(this);
  close_action_->setIconSet(QIconSet(QPixmap(":/fileclose.png")));
  hidden_action_ = new QAction(this);
  hidden_action_->setToggleAction(true);
  notes_action_ = new QAction(this);
  notes_action_->setIconSet(QIconSet(QPixmap(":/notes.png")));

  // menubar
  menu_bar_ = new QMenuBar(this);

  file_menu_ = new QPopupMenu(this);
  save_action_->addTo(file_menu_);
  saveas_action_->addTo(file_menu_);
  file_menu_->insertSeparator();
  build_action_->addTo(file_menu_);
  savebuild_action_->addTo(file_menu_);
  file_menu_->insertSeparator();
  close_action_->addTo(file_menu_);

  menu_bar_->insertItem(QString(""), file_menu_, 1);

  edit_menu_ = new QPopupMenu(this);
  notes_action_->addTo(edit_menu_);
  hidden_action_->addTo(edit_menu_);
  menu_bar_->insertItem(QString(""), edit_menu_, 2);
  
  main_frame_layout_->setMenuBar(menu_bar_);

  languageChange();
  //resize(QSize(545, 317).expandedTo(minimumSizeHint()));

/*
From: https://wiki.qt.io/Porting_Qt_3_to_Qt_4_Issues#clearWState.28_WState_Polished_.29

"This function is internal in Qt3. But it is called from Designer-generated files. Maybe, Qt
developers think that Designer-generated files will be updated (re-generated) during the
porting process, so, this function is not documented.

May be removed in the ported code."

So, I assume it can just be removed

  clearWState(WState_Polished);
*/
  // signals and slots connections
  connect(save_action_, SIGNAL(activated()), this, SLOT(Save()));
  connect(saveas_action_, SIGNAL(activated()), this, SLOT(SaveAs()));
  connect(build_action_, SIGNAL(activated()), this, SLOT(Build()));
  connect(savebuild_action_, SIGNAL(activated()), this, SLOT(SaveAndBuild()));
  connect(close_action_, SIGNAL(activated()), this, SLOT(Close()));
  connect(notes_action_, SIGNAL(activated()), this, SLOT(EditNotes()));
  connect(file_menu_, SIGNAL(aboutToShow()), this, SLOT(AboutToShowFileMenu()));
  connect(file_menu_, SIGNAL(aboutToHide()), this, SLOT(AboutToHideFileMenu()));
  connect(hidden_action_, SIGNAL(toggled(bool)), this, SLOT(SetHidden(bool)));
  save_error_ = false;
  last_save_error_ = false;
}

AssetBase::~AssetBase() {
  // no need to delete child widgets, Qt does it all for us
}

void AssetBase::RestoreLayout() {
  if (Name() != untitled_name) {
    AssetManagerLayout::Size size = AssetManager::GetLayoutSize(Name());
    if (size.width != -1)
      resize(size.width, size.height);
  }
}

void AssetBase::resizeEvent(QResizeEvent* event) {
  if (Name() != untitled_name) {
    const QSize& sz = event->size();
    AssetManager::SetLayoutSize(Name(), sz.width(), sz.height());
  }
}

void AssetBase::closeEvent(QCloseEvent* event) {
  event->ignore();
  Close();
}

// Sets the strings of the subwidgets using the current
// language.
void AssetBase::languageChange() {
  setCaption(tr("Asset Base"));
  save_action_->setText(tr("Save"));
  save_action_->setMenuText(tr("&Save"));
  save_action_->setAccel(tr("Ctrl+S"));
  saveas_action_->setText(tr("Save As"));
  saveas_action_->setMenuText(tr("Save &As..."));
  build_action_->setText(tr("Build"));
  build_action_->setMenuText(tr("&Build"));
  savebuild_action_->setText(tr("Save and Build"));
  savebuild_action_->setMenuText(tr("&Save and Build"));
  close_action_->setText(tr("Close"));
  close_action_->setMenuText(tr("&Close"));
  close_action_->setAccel(tr("Ctrl+W"));
  hidden_action_->setText(tr("Hidden"));
  hidden_action_->setMenuText(tr("&Hidden"));
  notes_action_->setText(tr("Notes"));
  notes_action_->setMenuText(tr("&Notes"));
  if (menu_bar_->findItem(1))
    menu_bar_->findItem(1)->setText(tr("&File"));
  if (menu_bar_->findItem(2))
    menu_bar_->findItem(2)->setText(tr("&Edit"));
}

bool AssetBase::Save() {
  try {
    if (!EnsureNameValid()) {
      return false;
    }

    QString error_msg;
    if (!SubmitEditRequest(&error_msg, last_save_error_)) {
      QMessageBox::critical(this, tr("Save ") + AssetPrettyName(),
                            tr("Unable to save: ") + Name() + "\n" +
                            error_msg,
                            tr("OK"), 0, 0, 0);
    } else {
      SetSaveError(false);
      SetLastSaveError(false);
      return true;
    }
  } catch (const khException &e) {
    QMessageBox::critical(this, tr("Save ") + AssetPrettyName(),
                          tr("Unable to save: ") + Name() + "\n" +
                          e.qwhat(),
                          tr("OK"), 0, 0, 0);
  } catch (const std::exception &e) {
    QMessageBox::critical(this, tr("Save ") + AssetPrettyName(),
                          tr("Unable to save: ") + Name() + "\n" +
                          Name() + "\n" +
                          e.what(),
                          tr("OK"), 0, 0, 0);
  } catch (...) {
    QMessageBox::critical(this, tr("Save ") + AssetPrettyName(),
                          tr("Unable to save: ") + Name() + "\n" +
                          Name() + "\nUnknown error",
                          tr("OK"), 0, 0, 0);
  }
  SetSaveError(true);
  SetLastSaveError(true);
  AssetManager::self->selectFolder();
  AssetManager::self->refresh();
  return false;
}

bool AssetBase::SaveAs() {
  AssetChooser chooser(this, AssetChooser::Save, AssetType(), AssetSubtype());
  if (chooser.exec() != QDialog::Accepted)
    return false;

  QString newpath;
  (void) chooser.getFullPath(newpath);
  SetName(newpath);

  return Save();
}

// OkToQuit() is used at program shutdown and will terminate the
// shutdown if the save fails or the user chooses cancel
bool AssetBase::OkToQuit() {
  if (IsModified()) {
    switch (QMessageBox::warning(this, tr("Close ") + AssetPrettyName(),
                              tr("This ") + AssetPrettyName() +
                              tr(" has unsaved changes.\n\n") +
                              tr("Do you want to save them now?"),
                              tr("Yes"), tr("No"), tr("Cancel"))) {
      case 0:
        if (!Save())
          return false;
        break;
      case 1:
        break;
      case 2:
        return false;
        break;
    }
  }
  return true;
}

void AssetBase::Close() {
  if (IsModified()) {
    switch (QMessageBox::warning(this, tr("Close ") + AssetPrettyName(),
                              tr("This ") + AssetPrettyName() +
                              tr(" has unsaved changes.\n\n") +
                              tr("Do you want to save them now?"),
                              tr("Yes"), tr("No"), tr("Cancel"))) {
      case 0:
        if (!Save())
          return;
        break;
      case 1:
        break;
      case 2:
        return;
        break;
    }
  }
  AssetManager::self->selectFolder();
  AssetManager::self->refresh();
  delete this;
}

void AssetBase::EditNotes() {
  AssetNotes asset_notes(this, meta_.GetValue("notes"));
  if (asset_notes.exec() == QDialog::Accepted) {
      meta_.SetValue("notes", asset_notes.GetText());
  }
}

void AssetBase::SetHidden(bool on) {
  meta_.SetValue("hidden", on);
}

void AssetBase::InstallMainWidget() {
  QWidget* main = BuildMainWidget(main_frame_);
  main_frame_layout_->addWidget(main, 0, 0);
}

void AssetBase::SetName(const QString& text) {
  asset_path_ = text;
  std::string pretty_name { AssetPrettyName().toStdString() };
  std::string short_name { shortAssetName(text) };

  setCaption(QString(pretty_name.c_str()) + " : " + short_name.c_str());
  emit NameChanged(text);
}

QString AssetBase::Name() const {
  return asset_path_;
}

void AssetBase::SetMeta(const khMetaData& meta) {
  meta_ = meta;
  bool hidden = meta.GetAs("hidden", false);
  hidden_action_->setOn(hidden);
}

khMetaData AssetBase::Meta() const {
  return meta_;
}

bool AssetBase::EnsureNameValid() {
  if (asset_path_ == untitled_name) {
    AssetChooser chooser(this, AssetChooser::Save, AssetType(), AssetSubtype());
    if (chooser.exec() != QDialog::Accepted)
      return false;

    QString newpath;
    (void) chooser.getFullPath(newpath);
    SetName(newpath);
  }
  return true;
}


void AssetBase::Build(void) {
  bool dirty = IsModified();
  if (dirty || save_error_) {
    QMessageBox::critical(this, tr("Build ") + AssetPrettyName(),
                          tr("This ") + AssetPrettyName() +
                          tr(" has unsaved changes.\n\n") +
                          tr("You must save before building."),
                          tr("OK"), 0, 0, 0);
    return;
  }

  QString error;
  bool needed = false;;
  bool success = khAssetManagerProxy::BuildAsset((const char*)Name().utf8(),
                                                 needed, error);
  AssetManager::self->selectFolder();
  AssetManager::self->refresh();
  if (success && !needed) {
    error = kh::tr("Nothing to do. Already up to date.");
    success = false;
  }
  if (!success) {
    if (error.isEmpty())
      error =  tr("Build has unknown problems.\n");
    QMessageBox::critical(this, tr("Build ") + AssetPrettyName(),
                          error,
                          tr("OK"), 0, 0, 0);
  }
}


void AssetBase::SaveAndBuild(void) {
  if (Save()) {
    Build();
  }
}


void AssetBase::AboutToShowFileMenu() {
  bool dirty = IsModified();
  save_action_->setEnabled(dirty && !save_error_);
  saveas_action_->setEnabled(!save_error_);
  build_action_->setEnabled(!dirty);
  savebuild_action_->setEnabled(dirty && !save_error_);
}

void AssetBase::AboutToHideFileMenu() {
  // Since we only adjust the enabled state when the menu is up, we must
  // make sure it's always enabled when the menu is dismissed. Otherwise
  // they might not be able to use the save accelerator key to save a dirty
  // asset.
  save_action_->setEnabled(!save_error_);
  saveas_action_->setEnabled(!save_error_);
  build_action_->setEnabled(true);
  savebuild_action_->setEnabled(!save_error_);
}
 

void AssetBase::SetErrorMsg(const QString& text, bool red) {
  if (text.isEmpty()) {
    error_msg_label_->hide();
  } else {
    if (red) {
      error_msg_label_->setText("<font color=\"#ff0000\">" + text + "</font>");
    } else {
      error_msg_label_->setText(text);
    }
    error_msg_label_->show();
  }
}

void AssetBase::SetSaveError(bool state) {
  save_error_ = state;
  save_action_->setEnabled(!save_error_);
  saveas_action_->setEnabled(!save_error_);
  savebuild_action_->setEnabled(!save_error_);
}

void AssetBase::SetLastSaveError(bool state) {
  last_save_error_ = state;
}

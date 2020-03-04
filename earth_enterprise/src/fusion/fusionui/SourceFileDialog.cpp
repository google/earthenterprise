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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either  express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <vector>

#include <Qt/qlabel.h>
#include <Qt/qcombobox.h>
#include <Qt/q3popupmenu.h>
#include <Qt/qpushbutton.h>
#include <Qt/qcursor.h>
#include <Qt/qstringlist.h>
#include <Qt/qtextcodec.h>
#include <Qt/qlayout.h>

#include <khFileUtils.h>
#include <gstFormatManager.h>

#include "SourceFileDialog.h"
#include "Preferences.h"
#include "FileHistory.h"
#include "khException.h"

SourceFileDialog* SourceFileDialog::onlyone = NULL;

SourceFileDialog* SourceFileDialog::self() {
  if (onlyone == NULL)
    onlyone = new SourceFileDialog(NULL);
  return onlyone;
}

SourceFileDialog::SourceFileDialog(QWidget* parent)
    : Q3FileDialog(parent) {
  setCaption(kh::tr("Open"));
  history_btn_ = new FileHistory(this, Preferences::filepath(
                                     "fileaccesshistory.xml"));
  addToolButton(history_btn_);
  connect(history_btn_, SIGNAL(selectFile(const QString &)),
          this, SLOT(ChooseRecent(const QString &)));

  QStringList file_list = history_btn_->getFileList();
  if (file_list.size() != 0)
    setDir(khDirname(file_list[0].toUtf8().constData()).c_str());

  setMode(Q3FileDialog::ExistingFiles);

  for (int ii = 0; ii < theFormatManager()->GetFilterCount(); ++ii)
    addFilter(theFormatManager()->GetFilter(ii));

  setSelectedFilter(0);

  //////////////////////////////////////////////////////////////////////////////
  //
  // provide a popup to specify character encoding
  //
  QLabel* label = new QLabel("Select encoding:", this);
  QComboBox* codecCombo = new QComboBox(this);

  codecCombo->insertItem(kh::tr("<none>"));

  QTextCodec* codec;
  int count = 0;
  for (; (codec = QTextCodec::codecForIndex(count)); ++count)
    codecCombo->insertItem(codec->name(), 1);

  PrefsConfig prefs = Preferences::getConfig();
  for (int index = 0; index < count; ++index) {
    if (codecCombo->text(index) == prefs.characterEncoding) {
      codecCombo->setCurrentItem(index);
      break;
    }
  }

  addWidgets(label, codecCombo, NULL);

#if 0
  //////////////////////////////////////////////////////////////////////////////
  //
  // allow the specification of a template to apply
  //
  QLabel* khdspLabel = new QLabel("Apply template:", this);

  QComboBox* khdspCombo = new QComboBox(this);
  khdspCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  QPushButton* khdspChooser = new QPushButton(this);
  khdspChooser->setPixmap(QPixmap::fromMimeSource("fileopen_1.png"));
  khdspChooser->setMaximumSize(QSize(50, 50));

  khdspCombo->insertItem(tr("<none>"));

  addWidgets(khdspLabel, khdspCombo, khdspChooser);
  //////////////////////////////////////////////////////////////////////////////
#endif

  connect(codecCombo, SIGNAL(activated(const QString &)),
          this, SLOT(chooseCodec(const QString &)));
}

void SourceFileDialog::chooseCodec(const QString& str) {
  if (str == "<none>") {
    codec = QString::null;
  } else {
    codec = str;
  }
}

void SourceFileDialog::ChooseRecent(const QString& fname) {
  setSelection(fname);
}

void SourceFileDialog::accept() {
  history_btn_->addFiles(selectedFiles());
  Q3FileDialog::accept();
}

////////////////////////////////////////////////////////////////////////////////

DatabaseIndexDialog::DatabaseIndexDialog()
    : Q3FileDialog() {
  history_btn_ = new FileHistory(this, Preferences::filepath(
                                     "backgroundimagedb.xml"));
  setMode(Q3FileDialog::DirectoryOnly);
  addToolButton(history_btn_);
  connect(history_btn_, SIGNAL(selectFile(const QString &)),
          this, SLOT(ChooseRecent(const QString &)));

  QStringList fileList = history_btn_->getFileList();
  if (fileList.size() != 0)
    setDir(khDirname(fileList[0].toUtf8().constData()).c_str());
}

void DatabaseIndexDialog::ChooseRecent(const QString& fname) {
  setSelection(fname);
}

void DatabaseIndexDialog::accept() {
  history_btn_->addFile(selectedFile());
  Q3FileDialog::accept();
}

////////////////////////////////////////////////////////////////////////////////

AssetDialog::AssetDialog()
    : Q3FileDialog() {
  setMode(Q3FileDialog::DirectoryOnly);
  connect(this, SIGNAL(dirEntered(const QString &)),
          this, SLOT(dirEntered(const QString &)));
}


void AssetDialog::dirEntered(const QString& dir) {
  for (QStringList::Iterator it = dirMatchList.begin();
       it != dirMatchList.end(); ++it) {
    if (dir.endsWith(*it))
      accept();
  }
}

////////////////////////////////////////////////////////////////////////////////

OpenWithHistoryDialog::OpenWithHistoryDialog(QWidget *parent,
                                             const QString& caption,
                                             const QString& history_path)
  : Q3FileDialog(parent) {
  setCaption(caption);
  history_btn_ = new FileHistory(this, Preferences::filepath(history_path));
  addToolButton(history_btn_);
  connect(history_btn_, SIGNAL(selectFile(const QString &)),
          this, SLOT(ChooseRecent(const QString &)));

  QStringList file_list = history_btn_->getFileList();
  if (file_list.size() != 0)
    setDir(khDirname(file_list[0].toUtf8().constData()).c_str());

  setMode(Q3FileDialog::ExistingFile);
}

void OpenWithHistoryDialog::ChooseRecent(const QString& fname) {
  setSelection(fname);
}

void OpenWithHistoryDialog::accept() {
  history_btn_->addFile(Q3FileDialog::selectedFile());
  Q3FileDialog::accept();
}

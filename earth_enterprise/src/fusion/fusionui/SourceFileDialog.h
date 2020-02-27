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


#ifndef KHSRC_FUSION_FUSIONUI_SOURCEFILEDIALOG_H__
#define KHSRC_FUSION_FUSIONUI_SOURCEFILEDIALOG_H__

#include <Qt/qobjectdefs.h>
#include <Qt/q3filedialog.h>
#include <Qt/qstringlist.h>
#include <Qt/qcheckbox.h>
#include <fusionui/.idl/filehistory.h>

class QComboBox;
class FileHistory;

class SourceFileDialog : public Q3FileDialog {
  Q_OBJECT

 public:
  SourceFileDialog(QWidget* parent);
  QString getCodec() const { return codec; }
  static SourceFileDialog* self();

 protected slots:
  virtual void accept();
  void ChooseRecent(const QString&);
  void chooseCodec(const QString&);

 private:
  QString codec;
  FileHistory* history_btn_;
  static SourceFileDialog* onlyone;
};

class DatabaseIndexDialog : public Q3FileDialog {
  Q_OBJECT

 public:
  DatabaseIndexDialog();

 protected slots:
  virtual void accept();
  void ChooseRecent(const QString& );

 private:
  FileHistory* history_btn_;
};


class AssetDialog : public Q3FileDialog {
  Q_OBJECT

 public:
  AssetDialog();
  void addDirMatch(const QString& dir) {
    dirMatchList.push_back(dir);
  }

 protected slots:
  void dirEntered(const QString& dir);

 private:
  QStringList dirMatchList;
};

class OpenWithHistoryDialog : public Q3FileDialog {
  Q_OBJECT

 public:
  OpenWithHistoryDialog(QWidget *parent,
                        const QString& caption,
                        const QString& history_path);

 protected slots:
  virtual void accept();
  void ChooseRecent(const QString&);

 private:
  FileHistory* history_btn_;
};

#endif  // !KHSRC_FUSION_FUSIONUI_SOURCEFILEDIALOG_H__

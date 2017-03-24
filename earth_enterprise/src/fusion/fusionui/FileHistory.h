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


#ifndef KHSRC_FUSION_FUSIONUI_FILEHISTORY_H__
#define KHSRC_FUSION_FUSIONUI_FILEHISTORY_H__

#include <qpushbutton.h>
#include <qstringlist.h>
#include <fusionui/.idl/filehistory.h>

class FileHistory : public QPushButton {
  Q_OBJECT

 public:
  FileHistory(QWidget* parent, const QString& path);
  QStringList getFileList() const;
  void addFile(const QString& fname);
  void addFiles(const QStringList& flist);

 signals:
  void selectFile(const QString& path);

 protected slots:
  void chooseRecent();

 private:
  enum {
    kMaxHistoryLength = 20
  };
  void clearFileHistory();
  FileHistoryConfig history;
  QString filePath;
};

#endif  // !KHSRC_FUSION_FUSIONUI_FILEHISTORY_H__

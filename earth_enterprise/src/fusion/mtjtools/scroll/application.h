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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>
#include <qmainwindow.h>
#include "imageview.h"

class QTextEdit;

class ApplicationWindow: public QMainWindow
{
  Q_OBJECT

 public:
  ApplicationWindow();
  ~ApplicationWindow();
  void setGlobalHisIn(const std::string &in) {
    viewer->setGlobalHisIn(in);
  }
  void setGlobalLutOut(const std::string &out) {
    viewer->setGlobalLutOut(out);
  }
  void setLutWork(const std::string &lutwork_file) {
    viewer->setLutWork(lutwork_file);
  }

  void loadInitImage(const std::string &image_file_path);

 protected:
  void closeEvent( QCloseEvent* );

private slots:
//void newDoc();
void choose();
  void load( const QString &fileName );
  void save();
  void keyhelps();

 private:
  QString filename;
  ImageView *viewer;
  QString prevDir;
};

#endif

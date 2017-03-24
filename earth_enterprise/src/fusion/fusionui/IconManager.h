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


#ifndef KHSRC_FUSION_FUSIONUI_ICONMANAGER_H__
#define KHSRC_FUSION_FUSIONUI_ICONMANAGER_H__

#include <qfiledialog.h>
#include <qscrollview.h>
#include <qpixmap.h>
#include <qurl.h>

#include "iconmanagerbase.h"

class PixmapView : public QScrollView,
                   public QFilePreview {
 public:
  PixmapView(QWidget* parent);
  void setPixmap(const QPixmap& pix);

  // inherited from QScrollView
  virtual void drawContents(QPainter* p, int, int, int, int);

  // inherited from QFilePreview
  virtual void previewUrl(const QUrl& u);

 private:
  QPixmap pixmap;
};

class IconManager : public IconManagerBase {
 public:
  IconManager(QWidget* parent, bool modal, WFlags flags);

  // inherited from QWidget
  virtual void show();

  // inherited from IconManagerBase
  virtual void addIcon();
  virtual void deleteIcon();

 private:
  QStringList ChoosePixmaps(QWidget* parent);
};

#endif  // !KHSRC_FUSION_FUSIONUI_ICONMANAGER_H__

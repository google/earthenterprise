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


#ifndef KHSRC_FUSION_FUSIONUI_SITEICONS_H__
#define KHSRC_FUSION_FUSIONUI_SITEICONS_H__

#include <gstSite.h>
#include "siteiconsbase.h"
#include "PixmapManager.h"
#include <Qt/qicon.h>
#include <Qt/q3iconview.h>
using QIconViewItem = Q3IconViewItem;
using QIconView = Q3IconView;
class QPixmap;

class SiteIcons : public SiteIconsBase {
 public:
  // currently selected icon
  // this is always maintained correct against the GUI
  gstIcon selection;

  // will assert if there are no internal icons in gstIconManager
  SiteIcons(QWidget *parent, PixmapManager::IconType type);

  void setDefault(void) {
    (void)setSelection(gstIcon());
  }

  // returns whether it was able to select the given icon
  bool setSelection(const gstIcon& icon);

  // thanks to invariant, will always return a valid icon
  gstIcon getSelection(void) const {
    return selection;
  }

  static bool selectIcon(QWidget *parent, PixmapManager::IconType type,
                         gstIcon* icon);

 private:
  void accept();
  void standardIconView_selectionChanged(QIconViewItem* item);
  void customIconView_selectionChanged(QIconViewItem* item);
  void standardIconView_doubleClicked(QIconViewItem* item);
  void customIconView_doubleClicked(QIconViewItem* item);
};

#endif  // !KHSRC_FUSION_FUSIONUI_SITEICONS_H__

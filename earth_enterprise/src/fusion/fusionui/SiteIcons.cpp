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


#include <sys/types.h>
#include <dirent.h>
#include <Qt/qimage.h>
#include <Qt/qpixmap.h>
#include <Qt/q3iconview.h>
#include <Qt/qtabwidget.h>
#include <Qt/qmessagebox.h>

#include <khArray.h>
#include <notify.h>
#include <gstIconManager.h>

#include "SiteIcons.h"

SiteIcons::SiteIcons(QWidget *parent, PixmapManager::IconType type)
    : SiteIconsBase(parent, 0, true, 0) {
  IconReference::Type ctype = IconReference::Internal;
  for (int ii = 0; ii < theIconManager->IconCount(ctype); ++ii) {
    const gstIcon& icon = theIconManager->GetIcon(ctype, ii);
    const QPixmap& pix = thePixmapManager->GetPixmap(icon, type);
    new QIconViewItem(standardIconView, icon.href(), pix);
  }
  standardIconView->sort();  // Always sort after adding icons.

  ctype = IconReference::External;
  for (int ii = 0; ii < theIconManager->IconCount(ctype); ++ii) {
    const gstIcon& icon = theIconManager->GetIcon(ctype, ii);
    const QPixmap& pix = thePixmapManager->GetPixmap(icon, type);
    new QIconViewItem(customIconView, icon.href(), pix);
  }
  customIconView->sort();  // Always sort after adding icons.

  standardIconView->setItemsMovable(false);
  standardIconView->setResizeMode(QIconView::Adjust);

  // make sure that at least the default icon exists
  assert(standardIconView->findItem(gstIcon().href()));

  setDefault();
}

bool SiteIcons::setSelection(const gstIcon& icon) {
  QIconView* iconview[] = { standardIconView, customIconView };

  QIconViewItem* currIcon = iconview[icon.type()]->findItem(icon.href());
  if (currIcon) {
    iconTabs->setCurrentPage(icon.type());
    iconview[icon.type()]->setCurrentItem(currIcon);
    selection = icon;
  }
  return currIcon;
}

void SiteIcons::standardIconView_selectionChanged(QIconViewItem* item) {
  if (!item)
    return;

  customIconView->clearSelection();
  selection = gstIcon(item->text(), IconReference::Internal);
}

void SiteIcons::customIconView_selectionChanged(QIconViewItem* item) {
  if (!item)
    return;

  standardIconView->clearSelection();
  selection = gstIcon(item->text(), IconReference::External);
}

void SiteIcons::standardIconView_doubleClicked(QIconViewItem* item) {
  selection = gstIcon(item->text(), IconReference::Internal);
  accept();
}

void SiteIcons::customIconView_doubleClicked(QIconViewItem* item) {
  selection = gstIcon(item->text(), IconReference::External);
  accept();
}

bool SiteIcons::selectIcon(QWidget *parent, PixmapManager::IconType type,
                           gstIcon* icon) {
  SiteIcons dialog(parent, type);

  if (!dialog.setSelection(*icon)) {
    notify(NFY_WARN, "Can't find icon: %s", icon->href().ascii());
    dialog.setDefault();
  }

  int result = dialog.exec();
  if (result == QDialog::Accepted) {
    *icon = dialog.getSelection();
  }

  return (result == QDialog::Accepted);
}

void SiteIcons::accept() {
  QIconView* iconview[] = { standardIconView, customIconView };

  if (iconview[iconTabs->currentPageIndex()]->currentItem() == 0) {
    QMessageBox::critical(this, "Fusion",
                          tr("Please select an icon."));
  } else {
    SiteIconsBase::accept();
  }
}

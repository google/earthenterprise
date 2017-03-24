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


#include <qapplication.h>
#include <qpainter.h>
#include <qstringlist.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qiconview.h>
#include <qmessagebox.h>

#include "IconManager.h"
#include "PixmapManager.h"
#include "Preferences.h"

#include <gstIconManager.h>
#include <gstFileUtils.h>
#include <khFileUtils.h>
#include <fusionui/.idl/filehistory.h>

PixmapView::PixmapView(QWidget* parent)
    : QScrollView(parent) {
  viewport()->setBackgroundMode(PaletteBase);
}

void PixmapView::setPixmap(const QPixmap& pix) {
  pixmap = pix;
  resizeContents(pixmap.size().width(), pixmap.size().height());
  viewport()->repaint(false);
}

void PixmapView::drawContents(QPainter* p, int cx, int cy, int cw, int ch) {
  p->fillRect(cx, cy, cw, ch, colorGroup().brush(QColorGroup::Base));
  p->drawPixmap(0, 0, pixmap);
}

void PixmapView::previewUrl(const QUrl& u) {
  if (u.isLocalFile()) {
    QString path = u.path();
    QPixmap pix(path);
    if (!pix.isNull())
      setPixmap(pix);
  } else {
    qWarning("Previewing remote files not supported");
  }
}


// -----------------------------------------------------------------------------

IconManager::IconManager(QWidget* parent, bool modal, WFlags flags)
  : IconManagerBase(parent, 0, modal, flags) {
}

void IconManager::show() {
  for (int i = 0; i < theIconManager->IconCount(IconReference::External); ++i) {
    gstIcon icon = theIconManager->GetIcon(IconReference::External, i);
    const QPixmap& pix =
        thePixmapManager->GetPixmap(icon, PixmapManager::AllThree);
    new QIconViewItem(masterIconView, icon.href(), pix);
  }
  masterIconView->sort();  // Always sort after adding icons.
  IconManagerBase::show();
}

void IconManager::addIcon() {
  QStringList pixmaps = ChoosePixmaps(this);
  if (pixmaps.isEmpty())
    return;

  for (QStringList::Iterator it = pixmaps.begin(); it != pixmaps.end(); ++it) {
    gstFileInfo fi((*it).latin1());
    if (theIconManager->AddIcon((*it).latin1())) {
      thePixmapManager->updateExternal();
      QPixmap newpix = thePixmapManager->GetPixmap(
          gstIcon(fi.baseName(), IconReference::External),
          PixmapManager::AllThree);
      new QIconViewItem(masterIconView, fi.baseName(), newpix);
    } else {
      QMessageBox::warning(this, "Fusion",
                           tr("Problem adding custom icon.\n\n"
                               "Source %1 must:\n\n"
                               "-  either be RGBA or Palatte-Index type\n"
                               "-  have a file type of PNG\n"
                               "-  have a unique file name").arg(fi.name()),
                           tr("OK"), 0, 0, 1);
    }
  }
  masterIconView->sort();  // Always sort after adding icons.
}

void IconManager::deleteIcon() {
  QIconViewItem* currIcon = masterIconView->currentItem();

  if (currIcon == 0)
    return;

  if (QMessageBox::warning(this, "Fusion",
      tr("Are you sure you want to permanently delete icon \"%1\"")
          .arg(currIcon->text()),
      tr("Yes"), tr("Cancel"), 0, 1, 1) == 0) {
    if (theIconManager->DeleteIcon(currIcon->text().latin1())) {
      masterIconView->takeItem(currIcon);
      delete currIcon;
    }
  }
}

QStringList IconManager::ChoosePixmaps(QWidget *parent) {
  QString filter("PNG-Pixmaps (*.png *.PNG)");

  QFileDialog fd(QString::null, filter, parent, 0, true);
  fd.setMode(QFileDialog::ExistingFiles);
  fd.setContentsPreviewEnabled(true);
  PixmapView* pw = new PixmapView(&fd);
  fd.setContentsPreview(pw, pw);
  fd.setViewMode(QFileDialog::List);
  fd.setPreviewMode(QFileDialog::Contents);
  fd.setCaption(qApp->translate("qChoosePixmap", "Choose Images..."));

  IconManagerHistory history;
  if (khExists(Preferences::filepath("iconmanager.xml").latin1())) {
    if (history.Load(Preferences::filepath("iconmanager.xml").latin1())) {
      fd.setDir(history.lastdir);
    }
  }

  if (fd.exec() == QDialog::Accepted) {
    history.lastdir = fd.dirPath();
    history.Save(Preferences::filepath("iconmanager.xml").latin1());
    return fd.selectedFiles();
  } else {
    return QStringList();
  }
}

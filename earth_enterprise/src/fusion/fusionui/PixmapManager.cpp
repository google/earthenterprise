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

#include <Qt/qglobal.h>
#include <Qt/qpixmap.h>
#include <Qt/qimage.h>
#include <gstIconManager.h>
#include <gstFileUtils.h>

#include "PixmapManager.h"

static const char* const colorbox_xpm[] = {
  // width height ncolors chars_per_pixel
  "20 16 4 1",
  // colors
  "  c #f00000",
  "# c #000000",
  ". c #ffffff",
  "* c #000000",
  // pixels
  "********************",
  "*..................*",
  "*..##############..*",
  "*.################.*",
  "*.##            ##.*",
  "*.##            ##.*",
  "*.##            ##.*",
  "*.##            ##.*",
  "*.##            ##.*",
  "*.##            ##.*",
  "*.##            ##.*",
  "*.##            ##.*",
  "*.################.*",
  "*..##############..*",
  "*..................*",
  "********************"
};

QPixmap ColorBox::Pixmap(unsigned int fill_r, unsigned int fill_g, unsigned int fill_b,
                         unsigned int outline_r, unsigned int outline_g, unsigned int outline_b) {
  QImage box(colorbox_xpm);
  box.setColor(kFillColorId, qRgb(fill_r, fill_g, fill_b));
  box.setColor(kOutlineColorId, qRgb(outline_r, outline_g, outline_b));
  return QPixmap(box);
}

QColor ColorBox::FillColor(const QPixmap& pix) {
  QImage image = pix.toImage();
  return QColor(image.color(kFillColorId));
}

QColor ColorBox::OutlineColor(const QPixmap& pix) {
  QImage image = pix.toImage();
  return QColor(image.color(kOutlineColorId));
}

// -----------------------------------------------------------------------------

PixmapManager *thePixmapManager = NULL;

void PixmapManager::init() {
  if (thePixmapManager != NULL)
    return;
  thePixmapManager = new PixmapManager();
}

PixmapManager::PixmapManager() {
  IconReference::Type int_type = IconReference::Internal;
  for (int id = 0; id < theIconManager->IconCount(int_type); ++id) {
    std::string iconpath = theIconManager->GetFullPath(int_type, id);
    QImage img(iconpath.c_str());
    if (img.isNull()) {
      notify(NFY_WARN, "Unable to load internal icon %s, skipping.",
             iconpath.c_str());
      continue;
    }
    pix_map_[theIconManager->GetIcon(int_type, id)] = QPixmap::fromImage(img);
  }

  updateExternal();
}

void PixmapManager::updateExternal() {
  IconReference::Type ext_type = IconReference::External;
  for (int id = 0; id < theIconManager->IconCount(ext_type); ++id) {
    const gstIcon& icon = theIconManager->GetIcon(ext_type, id);
    PixmapMapIterator found = pix_map_.find(icon);
    // only add new ones
    if (found == pix_map_.end()) {
      std::string iconpath = theIconManager->GetFullPath(ext_type, id);
      QImage img(iconpath.c_str());
      if (img.isNull()) {
        notify(NFY_WARN, "Unable to load external icon %s, skipping.",
               iconpath.c_str());
        continue;
      }
      pix_map_[icon] = QPixmap::fromImage(img);
    }
  }
}

QPixmap PixmapManager::GetPixmap(const gstIcon& icon, IconType type) {
  gstIcon tmp = icon;

  PixmapMapIterator found = pix_map_.find(icon);

  if (found == pix_map_.end())
    return InvalidIconPixmap();

  QPixmap pix = found->second;

  // return the appropriate type of pix from the palette
  int width = pix.width();
  QImage img;
  switch (type) {
    case NormalIcon:
      img = pix.toImage();
      img = img.copy(0, width * 2, width, width);
      pix = QPixmap::fromImage(img);
      break;
    case HighlightIcon:
      img = pix.toImage();
      img = img.copy(0, width, width, width);
      pix = QPixmap::fromImage(img);
      break;
    case RegularPair:
      img = pix.toImage();
      img = img.copy(0, width, width, width*2);
      pix = QPixmap::fromImage(img);
      break;
    case LayerIcon:
      img = pix.toImage();
      img = img.copy(0, 0, 16, 16);
      pix = QPixmap::fromImage(img);
      break;
    case AllThree:
      break;
  }

  return pix;
}

QPixmap PixmapManager::InvalidIconPixmap() {
  static QPixmap invalidIcon(32, 32);
  invalidIcon.fill(Qt::red);

  return invalidIcon;
}

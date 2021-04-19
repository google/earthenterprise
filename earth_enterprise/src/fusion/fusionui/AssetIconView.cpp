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


#include <Qt/qimage.h>
#include <Qt/qstringlist.h>
#include <assert.h>
#include <Qt/qabstractbutton.h>
#include <Qt/q3mimefactory.h>
#include "AssetIconView.h"
#include "AssetDrag.h"
#include <QtCore/qglobal.h>

using QMimeSourceFactory = Q3MimeSourceFactory;
using QImageDrag = Q3ImageDrag;

// -----------------------------------------------------------------------------

static QPixmap uic_load_pixmap(const QString& name) {
  const QMimeSource *m = QMimeSourceFactory::defaultFactory()->data(name);
  if (!m)
    return QPixmap();
  QPixmap pix;
  QImageDrag::decode(m, pix);
  return pix;
}

// -----------------------------------------------------------------------------

QImage* AssetIcon::defaultImage = NULL;

AssetIcon::AssetIcon(QIconView* parent, gstAssetHandle handle, int initsz)
    : QIconViewItem(parent),
      asset_handle_(handle) {
  if (defaultImage == NULL)
    defaultImage = new QImage(uic_load_pixmap("preview_default.png").
                              convertToImage());
  std::string name = shortAssetName(handle->getName());
  setText(name.c_str());

  QImage img;
  AssetVersion ver(handle->getAsset()->CurrVersionRef());
  if (ver) {
    std::string previewpath = ver->PreviewFilename();
    if (previewpath.size() && img.load(previewpath.c_str())) {
      image_ = new QImage(img);
    } else {
      image_ = defaultImage;
    }
  } else {
    image_ = defaultImage;
  }

  resize(initsz);
}

AssetIcon::~AssetIcon() {
  if (image_ != defaultImage)
    delete image_;
}

void AssetIcon::resize(int sz) {
  QPixmap pix = QPixmap::fromImage(image_->scaled(sz, sz, Qt::KeepAspectRatio));
  setPixmap(pix);
  setPixmapRect(QRect(0, 0, sz, sz));
  calcRect();
}

// -----------------------------------------------------------------------------

AssetIconView::AssetIconView(QWidget* parent, const char* name, Qt::WFlags f)
    : QIconView(parent, name, f) {
  setItemsMovable(false);
}

void AssetIconView::startDrag() {
  AssetIcon* icon = static_cast<AssetIcon*>(currentItem());
  assert(icon != NULL);

  AssetDrag* ad = new AssetDrag(this, icon->getAssetHandle()->getAsset());
  QPixmap pix = QPixmap::fromImage(icon->image()->scaled(64, 64, Qt::KeepAspectRatio));
  ad->setPixmap(pix);
  ad->dragCopy();
}

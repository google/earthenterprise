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

#include <qpixmap.h>
#include <qimage.h>

#include <gstMisc.h>
#include <notify.h>
#include <gstIconManager.h>

#include "GlobalFusion.h"
#include "PixmapManager.h"

GlobalFusion *globalFusion = NULL;

void GlobalFusion::init() {
  if (globalFusion != NULL)
    return;
  globalFusion = new GlobalFusion();
}

//
// private constructor to force usage of GlobalFusion::init()
//
GlobalFusion::GlobalFusion() {
  PixmapManager::init();

  static LayerIcons icons[] = {
    { "dot", "773", NULL },
    { "american-flag", "541", NULL },
    { "asian-flag", "539", NULL },
    { "dining", "536", NULL },
    { "fast-food", "542", NULL },
    { "french-flag", "538", NULL },
    { "italian-flag", "540", NULL },
    { "mexican-flag", "537", NULL },
    { "misc-dining", "519", NULL },
    { "bang", "771", NULL },
    { "bars", "518", NULL },
    { "building", "768", NULL },
    { "auto", "543", NULL },
    { "note", "774", NULL },
    { "parks", "769", NULL },
    { "recreation", "513", NULL },
    { "transportation", "512", NULL },
    { "one-dollar", "514", NULL },
    { "three-dollars", "516", NULL },
    { "two-dollars", "515", NULL },
    { "four-dollars", "517", NULL }
  };

  layerIcons = icons;
  numLayerIcons = sizeof(icons) / sizeof(icons[0]);

  for (int ii = 0; ii < numLayerIcons; ++ii) {
    QPixmap pix = thePixmapManager->GetPixmap(
        gstIcon(icons[ii].texid, IconReference::Internal),
        PixmapManager::LayerIcon);
    if (!pix.isNull()) {
      icons[ii].pixmap = new QPixmap(pix);
    } else {
      notify(NFY_WARN, "Built-in icon database is corrupt, missing %s!",
             icons[ii].name);
      icons[ii].pixmap->fill();
    }
  }
}

int GlobalFusion::IconIndexByName(const char* name) {
  for (int ii = 0; ii < numLayerIcons; ++ii) {
    if (!strcmp(layerIcons[ii].name, name))
      return ii;
  }
  return 0;
}

const QPixmap& GlobalFusion::IconPixmapByName(const char* name) {
  int idx = IconIndexByName(name);
  return *layerIcons[idx].pixmap;
}

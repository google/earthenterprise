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


#ifndef KHSRC_FUSION_FUSIONUI_PIXMAPMANAGER_H__
#define KHSRC_FUSION_FUSIONUI_PIXMAPMANAGER_H__

#include <khHashTable.h>

class gstIcon;
class QPixmap;

class ColorBox {
 public:
  enum {
    kFillColorId = 0,
    kOutlineColorId = 1
  };

  static QPixmap Pixmap(uint fill_r, uint fill_g, uint fill_b,
                        uint outline_r, uint outline_g, uint outline_b);
  static QColor FillColor(const QPixmap& pix);
  static QColor OutlineColor(const QPixmap& pix);
};

// -----------------------------------------------------------------------------

typedef std::map<const gstIcon, QPixmap> PixmapMap;
typedef std::map<const gstIcon, QPixmap>::iterator PixmapMapIterator;

class PixmapManager {
 public:
  enum IconType { 
    NormalIcon = 1, 
    HighlightIcon = 2, 
    RegularPair = 3,
    LayerIcon = 4, 
    AllThree = 7 
  };
 public:
  static void init();

  QPixmap GetPixmap(const gstIcon &icon, IconType type);

  void updateExternal();

 private:
  static QPixmap InvalidIconPixmap();
  // must be created only once by calling init()
  PixmapManager();    

  PixmapMap pix_map_;
};

extern PixmapManager *thePixmapManager;

#endif // !KHSRC_FUSION_FUSIONUI_PIXMAPMANAGER_H__

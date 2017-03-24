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


#ifndef KHSRC_FUSION_FUSIONUI_GLOBALFUSION_H__
#define KHSRC_FUSION_FUSIONUI_GLOBALFUSION_H__

class QPixmap;

class GlobalFusion {
 public:
  static void init();

  struct LayerIcons {
    const char* name;
    const char* texid;
    QPixmap* pixmap;
  };

  LayerIcons* layerIcons;
  int numLayerIcons;

 private:
  GlobalFusion();
  int IconIndexByName(const char* n);
  const QPixmap& IconPixmapByName(const char* n);
};

extern GlobalFusion *globalFusion;

#endif  // !KHSRC_FUSION_FUSIONUI_GLOBALFUSION_H__

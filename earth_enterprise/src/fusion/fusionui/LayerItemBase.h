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


#ifndef KHSRC_FUSION_FUSIONUI_LAYERITEMBASE_H__
#define KHSRC_FUSION_FUSIONUI_LAYERITEMBASE_H__

//#include <Qt/Q3Listview.h>
#include <Qt/q3listview.h>
//using Q3ListViewItem = Q3ListViewItem;

class LayerItemBase : public Q3ListViewItem {
 public:
  LayerItemBase(Q3ListView* parent);
  LayerItemBase(Q3ListViewItem* parent);

  virtual bool CanMoveUp() const;
  virtual bool CanMoveDown() const;

  void SwapPosition(LayerItemBase* item);
  void MoveUp();
  void MoveDown();

  LayerItemBase* Previous() const;
  LayerItemBase* Next() const;
  int SiblingCount() const;
};

#endif  // !KHSRC_FUSION_FUSIONUI_LAYERITEMBASE_H__

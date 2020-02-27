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


#ifndef KHSRC_FUSION_FUSIONUI_THEMATICFILTER_H__
#define KHSRC_FUSION_FUSIONUI_THEMATICFILTER_H__

#include <thematicfilterbase.h>

#include <gstRecord.h>
#include <Qt/q3listbox.h>
using QListBoxItem = Q3ListBoxItem;

class gstLayer;
class gstSource;
class DisplayRuleConfig;
class MapDisplayRuleConfig;

class ThematicFilter : public ThematicFilterBase {
 public:
   ThematicFilter(QWidget* parent, gstLayer* layer);
   // Create a ThematicFilter dialog.
   // parent: is the parent window/widget.
   // source: is the source vector data
   // src_layer_num: is the layer number of the data within the source data.
   ThematicFilter(QWidget* parent, gstSource* source, int src_layer_num);
   virtual ~ThematicFilter();

  // from ThematicFilterBase
  virtual void ChooseStartColor();
  virtual void ChooseEndColor();
  virtual void ChangeNumClasses();
  virtual void SelectAttribute(QListBoxItem* item);

  bool DefineNewFilters(std::vector<DisplayRuleConfig>* configs);

  // Opens a dialog which allows the user to edit thematic display rules
  // for the input data source.
  // Returns true if the user pressed ok.
  // It will fill in the vector of MapDisplayRuleConfigs if it returns true.
  // If returning false, it is equivalent to ignore/cancel the operation.
  // Unfortunately, MapDisplayRuleConfig is slightly different than
  // DisplayRuleConfig, so the DisplayRuleConfig version of DefineNewFilters
  // can't be easily templated or shared.
  bool DefineNewFilters(std::vector<MapDisplayRuleConfig>* configs);

 private:
  // palette table col names
  enum {
    kFieldNameCol = 0,
    kBreakValCol = 1,
    kClassCountCol = 2,
    kNumCols = 3
  };

  void RedrawColors();
  void ComputeStatistics(int field);
  // Perform basic dialog initialization. To be called within the constructor
  // after the record header, source and src_layer_num are initialized.
  void Init();

  gstHeaderHandle record_header_;
  gstSource* source_;
  int src_layer_num_;
};

#endif  // !KHSRC_FUSION_FUSIONUI_THEMATICFILTER_H__

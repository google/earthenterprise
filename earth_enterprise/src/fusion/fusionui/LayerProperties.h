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


#ifndef KHSRC_FUSION_FUSIONUI_LAYERPROPERTIES_H__
#define KHSRC_FUSION_FUSIONUI_LAYERPROPERTIES_H__

#include <gstIconManager.h>
#include <autoingest/.idl/storage/LayerConfig.h>
#include "layerpropertiesbase.h"
#include "WidgetControllers.h"

class gstLayer;

class LayerProperties : public LayerPropertiesBase,
                        public WidgetControllerManager {
 public:
  LayerProperties(QWidget* parent, const LayerConfig& config, gstLayer *layer);
  LayerConfig GetConfig();

  // From LayerPropertiesBase
  virtual void editScript();
  virtual void compileAndAccept();
  virtual void AddSearchField();
  virtual void DeleteSearchField();
  virtual void MoveFieldUp();
  virtual void MoveFieldDown();
  virtual void UpdateButtons();

 private:
  QStringList AvailableAttributes();
  void SwapRows(int row0, int row1);

  LayerConfig layer_config_;
  gstLayer* layer_;  // needed for ScriptEditor - so it can get header & source
};

#endif  // !KHSRC_FUSION_FUSIONUI_LAYERPROPERTIES_H__

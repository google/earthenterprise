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


#ifndef KHSRC_FUSION_FUSIONUI_LAYERGROUPPROPERTIES_H__
#define KHSRC_FUSION_FUSIONUI_LAYERGROUPPROPERTIES_H__

#include "layergrouppropertiesbase.h"
#include <gstIconManager.h>
#include <autoingest/.idl/storage/LayerConfig.h>
#include "WidgetControllers.h"

class gstIcon;

class LayerGroupProperties : public LayerGroupPropertiesBase,
                             public WidgetControllerManager {
 public:
  LayerGroupProperties(QWidget* parent, const LayerConfig& config);
  LayerConfig GetConfig();

 private:
  virtual void toggleIsVisible(bool state);
  LayerConfig layer_config_;
};

#endif  // !KHSRC_FUSION_FUSIONUI_LAYERGROUPPROPERTIES_H__

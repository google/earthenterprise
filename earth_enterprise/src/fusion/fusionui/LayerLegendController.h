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

//

#ifndef FUSION_FUSIONUI_LAYERLEGENDCONTROLLER_H__
#define FUSION_FUSIONUI_LAYERLEGENDCONTROLLER_H__

#include "LocaleDetails.h"

class LayerLegendController : public WidgetController {
 public:
  typedef std::map<QString, LocaleConfig> LocaleMap;

  static inline void
  Create(WidgetControllerManager &manager,
         LayerLegendWidget *layer_legend,
         LocaleDetails::EditMode edit_mode,
         LayerLegend *config) {
    (void) new LayerLegendController(manager, layer_legend,
                                     edit_mode, config);
  }

  virtual void SyncToConfig(void);

 protected:
  virtual void SyncToWidgetsImpl(void) {
    MySyncToWidgetsImpl();
  }

 private:
  LayerLegendController(WidgetControllerManager &manager,
                        LayerLegendWidget *layer_legend,
                        LocaleDetails::EditMode edit_mode,
                        LayerLegend *config);
  void MySyncToWidgetsImpl(void);

  LayerLegend  *config_;
  LocaleConfig  default_locale_;
  LocaleMap     other_locales_;
  WidgetControllerManager sub_manager_;
};


#endif // FUSION_FUSIONUI_LAYERLEGENDCONTROLLER_H__

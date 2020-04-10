/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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

#ifndef FUSION_GST_MAPRENDER_PREVIEWCONTROLLER_H__
#define FUSION_GST_MAPRENDER_PREVIEWCONTROLLER_H__

#include <autoingest/.idl/storage/MapLayerConfig.h>
#include <gst/vectorquery/PreviewSelector.h>
#include <gst/vectorprep/PreviewPreparer.h>
#include "Renderer.h"
#include <common/base/macros.h>

class gstProgress;

namespace maprender {

class PreviewCombiner;
class PreviewRenderer;

class PreviewController {
 public:
  typedef vectorquery::PreviewSelector SelectorStep;
  typedef khSharedHandle<SelectorStep> SelectorStepHandle;
  typedef std::vector<SelectorStepHandle> SelectorVector;
  typedef vectorprep::PreviewPreparer<MapDisplayRuleConfig> PreparerStep;
  typedef PreviewCombiner CombinerStep;
  typedef PreviewRenderer RendererStep;

  PreviewController(const khTilespace &tilespace_,
                    double oversizeFactor_,
                    const MapLayerConfig &config_,
                    gstProgress &progress,
                    PreviewController *prev = 0,
                    bool debug_ = false);
  ~PreviewController(void);

  bool GetTile(std::uint64_t addr, unsigned char *outBuf);
  bool HasLevel(std::uint32_t level);
 private:
  void BuildChildren(SelectorVector &oldSelectors,
                     gstProgress &progress);

  const khTilespace& tilespace;
  double oversizeFactor;
  unsigned int addrShift;

  // we keep a copy ourselves. We pass references to subpieces of this config
  // to our various PreviewStp children
  MapLayerConfig config;

  // The selectors are the only piece we _might_ keep when updating our config
  SelectorVector selectors;
  khDeletingVector<PreparerStep> preparers;
  khDeleteGuard<CombinerStep> combiner;
  khDeleteGuard<RendererStep> renderer;

  khCache<QuadtreePath, khSharedHandle<RendererOutputTile> > cache;
  geMultiRange<std::uint32_t> validLevels;
  bool debug;

  DISALLOW_COPY_AND_ASSIGN(PreviewController);
};

} // namespace maprender

#endif // FUSION_GST_MAPRENDER_PREVIEWCONTROLLER_H__

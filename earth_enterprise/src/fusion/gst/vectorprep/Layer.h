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


#ifndef FUSION_GST_VECTORPREP_LAYER_H__
#define FUSION_GST_VECTORPREP_LAYER_H__

#include "DisplayRule.h"
#include <vectorquery/Tiles.h>

namespace vectorprep {


// ****************************************************************************
// ***  Layer<DisplayRuleConfig>
// ****************************************************************************
template <class DisplayRuleConfig>
class Layer {
 public:
  typedef DisplayRule<DisplayRuleConfig>  DisplayRuleType;
  typedef LayerTile<DisplayRuleConfig>  LayerTileType;

  Layer(const gstSharedSource &source,
        const khTilespace &tilespace_, double oversizeFactor_,
        const std::vector<DisplayRuleConfig> &drConfigs,
        const QStringList &jsContextScripts);
  ~Layer(void);
  bool Prepare(const QuadtreePath &path,
               LayerTileType *out,
               const vectorquery::LayerTile &in);
 private:
  const khTilespace& tilespace;
  double             oversizeFactor;
  gstHeaderHandle    srcHeader;
  gstRecordJSContext jsCX;
  khDeletingVector<DisplayRuleType> displayRules;
};


} // namespace vectorprep


#endif // FUSION_GST_VECTORPREP_LAYER_H__

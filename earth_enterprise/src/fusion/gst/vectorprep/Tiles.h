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

#ifndef FUSION_GST_VECTORPREP_TILES_H__
#define FUSION_GST_VECTORPREP_TILES_H__

#include <common/base/macros.h>
#include "gstGeode.h"
#include "gstRecord.h"
#include "gstVertex.h"

namespace vectorprep {

// ****************************************************************************
// ***  NOTE: These classes (and all the others in vectorprep) are shared by
// ***  all vector types (stream, map, mappoi, xml). They are also shared
// ***  between the GUI preview chain and the vectorfuse processing chain
// ****************************************************************************


// ****************************************************************************
// ***  FeatureTile
// ****************************************************************************
class FeatureTile {
 public:
  GeodeList  glist;
  RecordList rlist;

  inline FeatureTile(void) { }

  void Reset(void) {
    glist.resize(0);
    rlist.resize(0);
  }
 private:
  DISALLOW_COPY_AND_ASSIGN(FeatureTile);
};


// ****************************************************************************
// ***  SiteTile
// ****************************************************************************
class SiteTile {
 public:
  VertexList vlist;
  RecordList rlist;

  inline SiteTile(void) { }

  void Reset(void) {
    vlist.resize(0);
    rlist.resize(0);
  }
 private:
  DISALLOW_COPY_AND_ASSIGN(SiteTile);
};



// ****************************************************************************
// ***  DisplayRuleTile
// ****************************************************************************
template <class DisplayRuleConfig>
class DisplayRuleTile {
 public:
  typedef FeatureTile                FeatureTileType;
  typedef SiteTile                   SiteTileType;

  const DisplayRuleConfig &config;
  FeatureTileType   feature;
  SiteTileType      site;

  DisplayRuleTile(const DisplayRuleConfig &config_) :
      config(config_),
      feature(),
      site()
  { }

  void Reset(void) {
    feature.Reset();
    site.Reset();
  }
 private:
  DISALLOW_COPY_AND_ASSIGN(DisplayRuleTile);
};


// ****************************************************************************
// ***  LayerTile
// ****************************************************************************
template <class DisplayRuleConfig>
class LayerTile {
 public:
  typedef DisplayRuleTile<DisplayRuleConfig> DisplayRuleTileType;

  khDeletingVector<DisplayRuleTileType> displayRules;

  LayerTile(const std::vector<DisplayRuleConfig> &drConfigs) {
    displayRules.reserve(drConfigs.size());
    for (unsigned int i = 0; i < drConfigs.size(); ++i) {
      displayRules.push_back(new DisplayRuleTileType(drConfigs[i]));
    }
  }

  void Reset(void) {
    for (unsigned int i = 0; i < displayRules.size(); ++i) {
      displayRules[i]->Reset();
    }
  }
 private:
  DISALLOW_COPY_AND_ASSIGN(LayerTile);
};


} // namespace vectorprep

#endif // FUSION_GST_VECTORPREP_TILES_H__

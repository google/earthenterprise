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


#ifndef FUSION_GST_MAPRENDER_COMBINER_H__
#define FUSION_GST_MAPRENDER_COMBINER_H__

#include <gst/vectorprep/WorkflowProcessor.h>
#include <khTileAddr.h>
#include "LabelPaths.h"
#include "RenderTypes.h"

class MercatorProjection;

namespace maprender {

typedef vectorprep::WorkflowOutputTile<MapDisplayRuleConfig> CombinerInputTile;


class CombinerOutputTile {
 public:
  QuadtreePath path;
  khDeletingVector<SubLayer> subLayers;

  inline CombinerOutputTile(void) { }
 private:
  DISALLOW_COPY_AND_ASSIGN(CombinerOutputTile);
};

inline void ResetTile(CombinerOutputTile *t) {
  t->subLayers.resize(0);
}

void
GetTranslation(VectorDefs::EightSides dir, const MapSiteConfig& site_config,
               const SiteLabel& site_label, const SkRect& marker_box,
               SkPoint &point  /* OUT */);

class Combiner :
      public workflow::AggregateProcessor<CombinerOutputTile,
                                          CombinerInputTile>
{
 public:
  typedef CombinerOutputTile OutTile;
  typedef CombinerInputTile  InTile;
  typedef workflow::AggregateProcessor<OutTile, InTile> ProcessorBase;

  typedef vectorprep::LayerTile<MapDisplayRuleConfig> InLayerTile;
  typedef vectorprep::DisplayRuleTile<MapDisplayRuleConfig> InDisplayRuleTile;
  typedef vectorprep::FeatureTile InFeatureTile;
  typedef vectorprep::SiteTile    InSiteTile;




  inline Combiner(const khTilespace &tilespace_) :
      ProcessorBase(),
      tilespace(tilespace_),
      path_labeler_(tilespace.tileSize)
  { }

  virtual bool Process(OutTile *out, const std::vector<const InTile*> &in);

 private:
  class TranslationContext {
   public:
    TranslationContext(const khTilespace &tilespace, const QuadtreePath &path);
    void TranslatePoint(SkPoint *point, const gstVertex &vertex) const;

   private:
    double origin_x_;
    double origin_y_;
    double scale_x_;
    double scale_y_;
    std::uint32_t level_;
    const khTilespace& tilespace_;
  };


  RenderPath PathFromGeode(const TranslationContext &trans,
                           const gstGeodeHandle &geodeh);
  SkPoint    PointFromVertex(const TranslationContext &trans,
                             const gstVertex &vertex);
  void PlaceLabels(OutTile *out, const std::vector<const InTile*> &in);
  khTransferGuard<DisplayRule>
  ConvertDisplayRuleGeometry(const TranslationContext &trans,
                             const InDisplayRuleTile &in);
  khTransferGuard<SubLayer>
  ConvertSubLayerGeometry(const TranslationContext &trans,
                          const InLayerTile &in);
  void ConvertFeatureGeometry(Feature& feature,
                              const TranslationContext &trans,
                              const InFeatureTile &in);
  void ConvertSiteGeometry(Site& site,
                           const TranslationContext &trans,
                           const InSiteTile &in);

  const khTilespace& tilespace;

  LabelPaths path_labeler_;
};




} // namespace maprender

#endif // FUSION_GST_MAPRENDER_COMBINER_H__

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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_VECTORPREP_DISPLAYRULEIMPL_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_VECTORPREP_DISPLAYRULEIMPL_H_

#include "fusion/gst/vectorprep/Tiles.h"
#include "fusion/autoingest/FieldBinderDef.h"
#include "fusion/autoingest/.idl/storage/VectorDefs.h"
#include "fusion/gst/gstRecordJSContext.h"
#include "common/khTileAddr.h"
#include "fusion/gst/vectorquery/Tiles.h"
#include "fusion/gst/gstSourceManager.h"
#include "common/geMultiRange.h"

class QuadtreePath;

namespace vectorprep {

class Binder;

// ****************************************************************************
// ***  DisplayRuleBase
// ****************************************************************************
class DisplayRuleBase {
 protected:
  template <class FeatureConfig, class SiteConfig>
  DisplayRuleBase(const QString &contextName_,
                  const gstSharedSource &source,
                  const gstHeaderHandle &srcHeader_,
                  gstRecordJSContext &jsCX_,
                  const FeatureConfig &featureConfig,
                  const SiteConfig &siteConfig);
  ~DisplayRuleBase(void);

  bool Prepare(const gstBBox &cutBox,
               unsigned int level,
               FeatureTile *featureOut,
               SiteTile    *siteOut,
               const vectorquery::DisplayRuleTile &in);


 private:
  gstVertex GetSiteLocation(const gstGeodeHandle &geode);
  void ConvertFeatureType(gstGeodeHandle &geode);
  void ReduceFeatures(unsigned int level, FeatureTile *tile);
  void ReduceSites(SiteTile *tile);
  void ReducePolylines(FeatureTile *tile);
  void ReduceRoads(bool remove_overlapping_segments, unsigned int level,
                   FeatureTile *tile);
  void ReducePolygons(FeatureTile *tile);
  static void RemoveDuplicateSegments(GeodeList *glist);

  // Merges only at degree-2 vertices
  template<class T>
  static void RemoveDuplicateSegmentsAndJoinSegments(const T& glist);
  void RemoveOverlappingSegments(GeodeList* glist, int level);
  void CreateBinders(std::vector<FieldBinderDef> &defs,
                     khDeletingVector<Binder> *binders,
                     const QString &context);
  gstHeaderHandle CreateBoundHeader(const std::vector<FieldBinderDef> &defs);

  QString             contextName;
  gstSharedSource     source;
  gstHeaderHandle     srcHeader;
  gstRecordJSContext  jsCX;

  VectorDefs::FeatureDisplayType  featureDisplayType;
  VectorDefs::FeatureReduceMethod featureReduceMethod;
  khDeletingVector<Binder>        featureBinders;
  gstHeaderHandle                 featureBoundHeader;
  std::int32_t                           featureKeyFieldNum;
  geMultiRange<std::uint32_t>            featureValidLevels;

  VectorDefs::SiteDisplayPosition siteDisplayPosition;
  VectorDefs::SiteReduceMethod    siteReduceMethod;
  khDeletingVector<Binder>        siteBinders;
  gstHeaderHandle                 siteBoundHeader;
  std::int32_t                           siteKeyFieldNum;
  geMultiRange<std::uint32_t>            siteValidLevels;
};


}  // namespace vectorprep


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_VECTORPREP_DISPLAYRULEIMPL_H_

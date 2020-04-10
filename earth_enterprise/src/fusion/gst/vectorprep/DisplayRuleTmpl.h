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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_VECTORPREP_DISPLAYRULETMPL_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_VECTORPREP_DISPLAYRULETMPL_H_

#include "fusion/gst/vectorprep/DisplayRule.h"

namespace vectorprep {


// ****************************************************************************
// ***  DisplayRuleBase
// ****************************************************************************
template <class FeatureConfig, class SiteConfig>
DisplayRuleBase::DisplayRuleBase(const QString &contextName_,
                                 const gstSharedSource &source_,
                                 const gstHeaderHandle &srcHeader_,
                                 gstRecordJSContext &jsCX_,
                                 const FeatureConfig &featureConfig,
                                 const SiteConfig &siteConfig)
    : contextName(contextName_),
      source(source_),
      srcHeader(srcHeader_),
      jsCX(jsCX_),
      featureDisplayType(featureConfig.DisplayType()),
      featureReduceMethod(featureConfig.ReduceMethod()),
      featureBinders(),
      featureBoundHeader(),
      featureKeyFieldNum(),
      featureValidLevels(featureConfig.ValidLevels()),
      siteDisplayPosition(siteConfig.DisplayPosition(
          featureConfig.DisplayType())),
      siteReduceMethod(siteConfig.ReduceMethod()),
      siteBinders(),
      siteBoundHeader(),
      siteKeyFieldNum(),
      siteValidLevels(siteConfig.ValidLevels()) {
  std::vector<FieldBinderDef> defs;
  featureConfig.PopulateFieldBinderDefs(&defs);
  CreateBinders(defs, &featureBinders, contextName + "(feature): ");
  featureBoundHeader = CreateBoundHeader(defs);

  // Note: In UI we can specify different visibility range
  // for geometry and labels/shields. So some levels may have no labels
  // and shields. And featureKeyFieldNum may be equal to -1.
  if (featureReduceMethod == VectorDefs::ReduceFeatureRoads) {
    featureKeyFieldNum = featureBoundHeader->FieldPosByName("label");
    if (featureKeyFieldNum < 0) {
      featureKeyFieldNum = featureBoundHeader->FieldPosByName("shield");
    }
  } else if (featureReduceMethod == VectorDefs::ReduceFeaturePolylines) {
    featureKeyFieldNum = featureBoundHeader->FieldPosByName("label");
  }

  defs.clear();
  siteConfig.PopulateFieldBinderDefs(&defs);
  CreateBinders(defs, &siteBinders, contextName + "(site): ");
  siteBoundHeader = CreateBoundHeader(defs);
  if (siteReduceMethod == VectorDefs::ReduceSiteSuppressDuplicates) {
    siteKeyFieldNum = siteBoundHeader->FieldPosByName("label");
    if (siteKeyFieldNum < 0) {
      throw khException
        (kh::tr("Suppress Duplicates selected w/o a label"));
    }
  }
}


// ****************************************************************************
// ***  DisplayRule<DisplayRuleConfig>
// ****************************************************************************
template <class DisplayRuleConfig>
DisplayRule<DisplayRuleConfig>::DisplayRule(
    const gstSharedSource &source_,
    const gstHeaderHandle &srcHeader_,
    gstRecordJSContext &jsCX_,
    const DisplayRuleConfig &config) :
    DisplayRuleBase(config.name, source_,
                    srcHeader_, jsCX_,
                    config.feature, config.site) {
}

template <class DisplayRuleConfig>
bool DisplayRule<DisplayRuleConfig>::Prepare(
    const gstBBox &cutBox,
    unsigned int level,
    DisplayRuleTileType *out,
    const vectorquery::DisplayRuleTile &in) {
  FeatureTile *ftile = 0;
  if (out->config.feature.Enabled()) {
    ftile = &out->feature;
  } else {
    notify(NFY_VERBOSE, "Skipping features at level %d (disabled)",
           static_cast<int>(level));
  }
  SiteTile *stile    = 0;
  if (out->config.site.Enabled()) {
    stile = &out->site;
  } else {
    notify(NFY_VERBOSE, "Skipping sites at level %d (disabled)",
           static_cast<int>(level));
  }
  if (ftile || stile) {
    return DisplayRuleBase::Prepare(cutBox, level, ftile, stile, in);
  } else {
    return false;
  }
}



}  // namespace vectorprep

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_VECTORPREP_DISPLAYRULETMPL_H_

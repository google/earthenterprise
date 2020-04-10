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


#ifndef FUSION_GST_VECTORPREP_DISPLAYRULE_H__
#define FUSION_GST_VECTORPREP_DISPLAYRULE_H__

#include "DisplayRuleImpl.h"

namespace vectorprep {


// ****************************************************************************
// ***  DisplayRule<DisplayRuleConfig>
// ****************************************************************************
template <class DisplayRuleConfig>
class DisplayRule : public DisplayRuleBase {
 public:
  typedef DisplayRuleTile<DisplayRuleConfig>  DisplayRuleTileType;

  DisplayRule(const gstSharedSource &source,
              const gstHeaderHandle &srcHeader_,
              gstRecordJSContext &jsCX_,
              const DisplayRuleConfig &config);
  bool Prepare(const gstBBox &cutBox,
               unsigned int level,
               DisplayRuleTileType *out,
               const vectorquery::DisplayRuleTile &in);
};



} // namespace vectorprep

#endif // FUSION_GST_VECTORPREP_DISPLAYRULE_H__

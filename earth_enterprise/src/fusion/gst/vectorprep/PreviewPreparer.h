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

#ifndef FUSION_GST_VECTORPREP_PREVIEWPREPARER_H__
#define FUSION_GST_VECTORPREP_PREVIEWPREPARER_H__

#include "WorkflowProcessor.h"
#include <gst/vectorquery/Tiles.h>

namespace vectorprep {

// ****************************************************************************
// ***  Wrapper around vectorprep::WorkflowProcessor to turn it into a
// ***  workflow::ChainedPreviewStep
// ****************************************************************************


template <class DisplayRuleConfig>
class PreviewPreparer :
     public WorkflowProcessor<DisplayRuleConfig>,
     public workflow::ChainedPreviewStep<WorkflowOutputTile<DisplayRuleConfig>,
                                         vectorquery::WorkflowOutputTile>
{
 public:
  typedef WorkflowOutputTile<DisplayRuleConfig> OutTile;
  typedef vectorquery::WorkflowOutputTile InTile;
  typedef WorkflowProcessor<DisplayRuleConfig> ProcessorBase;
  typedef workflow::ChainedPreviewStep<OutTile, InTile> ChainBase;
  typedef workflow::PreviewStep<InTile> InputStepType;

  inline PreviewPreparer(const gstSharedSource &source,
                         const khTilespace &tilespace_,
                         double oversizeFactor_,
                         const std::vector<DisplayRuleConfig> &drConfigs,
                         const QStringList &jsContextScripts,
                         InputStepType &inputStep,
                         const khSharedHandle<InTile> &input) :
      ProcessorBase(source, tilespace_, oversizeFactor_,
                    drConfigs, jsContextScripts),
      ChainBase(static_cast<ProcessorBase&>(*this), inputStep, input)
  {
  }
};


} // namespace vectorprep

#endif // FUSION_GST_VECTORPREP_PREVIEWPREPARER_H__

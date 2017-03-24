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


#ifndef FUSION_GST_MAPRENDER_PREVIEWCOMBINER_H__
#define FUSION_GST_MAPRENDER_PREVIEWCOMBINER_H__

#include "Combiner.h"
#include <workflow/PreviewStep.h>


namespace maprender {


class PreviewCombiner :
      public Combiner,
      public workflow::AggregateChainedPreviewStep
      <CombinerOutputTile, CombinerInputTile> {
 public:
  typedef CombinerOutputTile OutTile;
  typedef CombinerInputTile  InTile;
  typedef workflow::AggregateProcessor<OutTile, InTile> ProcessorBase;
  typedef workflow::AggregateChainedPreviewStep<OutTile, InTile> ChainBase;
  typedef workflow::PreviewStep<InTile> InputStepType;


  PreviewCombiner(const khTilespace &tilespace_,
                  const std::vector<InputStepType*> &inputSteps_,
                  const std::vector<khSharedHandle<InTile> > &inputs_) :
      Combiner(tilespace_),
      ChainBase(static_cast<ProcessorBase&>(*this), inputSteps_, inputs_)
  { }
};



} // namespace maprender


#endif // FUSION_GST_MAPRENDER_PREVIEWCOMBINER_H__

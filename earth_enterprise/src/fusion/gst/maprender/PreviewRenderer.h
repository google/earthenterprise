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


#ifndef FUSION_GST_MAPRENDER_PREVIEWRENDERER_H__
#define FUSION_GST_MAPRENDER_PREVIEWRENDERER_H__

#include "Renderer.h"
#include <workflow/PreviewStep.h>


namespace maprender {


class PreviewRenderer :
      public Renderer,
      public workflow::ChainedPreviewStep
      <RendererOutputTile, RendererInputTile> {
 public:
  typedef RendererOutputTile OutTile;
  typedef RendererInputTile  InTile;
  typedef workflow::Processor<OutTile, InTile> ProcessorBase;
  typedef workflow::ChainedPreviewStep<OutTile, InTile> ChainBase;
  typedef workflow::PreviewStep<InTile> InputStepType;


  PreviewRenderer(InputStepType &inputStep, bool debug_) :
      Renderer(debug_),
      ChainBase(static_cast<ProcessorBase&>(*this), inputStep)
  { }
};


} // namespace maprender


#endif // FUSION_GST_MAPRENDER_PREVIEWRENDERER_H__

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

#ifndef FUSION_WORKFLOW_PREVIEWSTEP_H__
#define FUSION_WORKFLOW_PREVIEWSTEP_H__

#include <assert.h>
#include <khRefCounter.h>
#include "Processor.h"


namespace workflow {


// ****************************************************************************
// ***  Out is assumed to contain a 'path' member that tells what to fetch
// ****************************************************************************
template <class Out>
class PreviewStep {
 public:
  typedef Out OutTile;

  PreviewStep(void) { }
  virtual ~PreviewStep(void) { }
  virtual bool Fetch(Out *out) = 0;
 private:
  DISALLOW_COPY_AND_ASSIGN(PreviewStep);
};

template <class Out, class In>
class ChainedPreviewStep : public PreviewStep<Out> {
  typedef Processor<Out, In> ProcessorType;

  ProcessorType &processor;
  PreviewStep<In> &inputStep;
  khSharedHandle<In> input;
 public:
  ChainedPreviewStep(ProcessorType &processor_, PreviewStep<In> &inputStep_,
                     const khSharedHandle<In> &input_) :
      PreviewStep<Out>(), processor(processor_),
      inputStep(inputStep_), input(input_) { }
  ChainedPreviewStep(ProcessorType &processor_, PreviewStep<In> &inputStep_) :
      PreviewStep<Out>(), processor(processor_),
      inputStep(inputStep_),
      input(TransferOwnership(new In())) { }
  virtual ~ChainedPreviewStep(void) { }

  virtual bool Fetch(Out *out) {
    ResetTileOnException<In>  inGuard(&*input);
    ResetTile(&*input);
    input->path = out->GetPath();

    if (!inputStep.Fetch(&*input)) {
      return false;
    }

    ResetTileOnException<Out> outGuard(out);
    return processor.Process(out, *input);
  }
 private:
  DISALLOW_COPY_AND_ASSIGN(ChainedPreviewStep);
};

template <class Out, class In>
class AggregateChainedPreviewStep : public PreviewStep<Out> {
  typedef AggregateProcessor<Out, In> ProcessorType;
  typedef PreviewStep<In> InputStepType;

  ProcessorType &processor;
  const std::vector<InputStepType*> inputSteps;
  std::vector<khSharedHandle<In> > inputs;

 public:
  AggregateChainedPreviewStep(ProcessorType &processor_,
                              const std::vector<InputStepType*> &inputSteps_,
                              const std::vector<khSharedHandle<In> > &inputs_):
      PreviewStep<Out>(), processor(processor_),
      inputSteps(inputSteps_),
      inputs(inputs_.begin(), inputs_.end()) {
    assert(inputSteps.size() == inputs_.size());
  }
  AggregateChainedPreviewStep(ProcessorType &processor_,
                              const std::vector<InputStepType*> &inputSteps_) :
      PreviewStep<Out>(), processor(processor_),
      inputSteps(inputSteps_),
      inputs() {
    unsigned int count = inputSteps.size();
    assert(count > 0);
    inputs.resize(count);
    for (unsigned int i = 0; i < count; ++i) {
      inputs[i] = TransferOwnership(new In());
    }
  }
  virtual ~AggregateChainedPreviewStep(void) { }

  virtual bool Fetch(Out *out) {
    std::vector<const In*> found;
    try {
      for (unsigned int i = 0; i < inputs.size(); ++i) {
        ResetTile(&*inputs[i]);
        inputs[i]->SetPath(out->path);

        if (inputSteps[i]->Fetch(&*inputs[i])) {
          found.push_back(&*inputs[i]);
        }
      }
    } catch (...) {
      for (unsigned int i = 0; i < inputs.size(); ++i) {
        ResetTile(&*inputs[i]);
      }
      throw;
    }
    if (found.size() > 0) {
      ResetTileOnException<Out> outGuard(out);
      return processor.Process(out, found);
    } else {
      return false;
    }
  }
 private:
  DISALLOW_COPY_AND_ASSIGN(AggregateChainedPreviewStep);
};


} // namespace workflow


#endif // FUSION_WORKFLOW_PREVIEWSTEP_H__

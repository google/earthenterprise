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


#ifndef FUSION_GST_VECTORPREP_WORKFLOWPROCESSOR_H__
#define FUSION_GST_VECTORPREP_WORKFLOWPROCESSOR_H__


#include "Layer.h"
#include <gst/vectorquery/Tiles.h>
#include <workflow/Processor.h>
#include <khGuard.h>

namespace vectorprep {


// ****************************************************************************
// ***  Simple wrapper around vectorprep::Layer to expose the
// ***  workflow::Processor API
// ***
// ***  Will be used both for preview and for vactorfuse processing
// ****************************************************************************


template <class DisplayRuleConfig>
class WorkflowOutputTile : public LayerTile<DisplayRuleConfig> {
 public:

  inline WorkflowOutputTile(const std::vector<DisplayRuleConfig> &drConfigs) :
      LayerTile<DisplayRuleConfig>(drConfigs),
      path_(), generation_sequence_()
  { }

  void SetPath(const QuadtreePath& path) {
    path_ = path;
    generation_sequence_ = path_.GetGenerationSequence();
  }

  const QuadtreePath& GetPath() const { return path_; }

  inline bool IsGeneratedAfter(const WorkflowOutputTile &other) const {
    return generation_sequence_ > other.generation_sequence_;
  }

 private:
  QuadtreePath path_;
  std::uint64_t generation_sequence_;
  inline bool operator>(const WorkflowOutputTile &other) const;

  DISALLOW_COPY_AND_ASSIGN(WorkflowOutputTile);
};

template <class DisplayRuleConfig>
inline void ResetTile(WorkflowOutputTile<DisplayRuleConfig> *t) {
  t->Reset();
}

template <class DisplayRuleConfig>
class WorkflowProcessor :
      public workflow::Processor<WorkflowOutputTile<DisplayRuleConfig>,
                                 vectorquery::WorkflowOutputTile>,
      public Layer<DisplayRuleConfig>
{
 public:
  typedef WorkflowOutputTile<DisplayRuleConfig> OutTile;
  typedef vectorquery::WorkflowOutputTile InTile;

  inline WorkflowProcessor(const gstSharedSource &source,
                           const khTilespace &tilespace_,
                           double oversizeFactor_,
                           const std::vector<DisplayRuleConfig> &drConfigs,
                           const QStringList &jsContextScripts) :
      Layer<DisplayRuleConfig>(source, tilespace_,
                               oversizeFactor_, drConfigs, jsContextScripts)
  {
  }

  virtual bool Process(OutTile *out, const InTile &in) {
    return Layer<DisplayRuleConfig>::Prepare(out->GetPath(), out, in);
  }
};


} // namespace vectorprep

#endif // FUSION_GST_VECTORPREP_WORKFLOWPROCESSOR_H__

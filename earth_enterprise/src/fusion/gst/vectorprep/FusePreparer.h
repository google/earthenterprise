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

#ifndef FUSION_GST_VECTORPREP_FUSEPREPARER_H__
#define FUSION_GST_VECTORPREP_FUSEPREPARER_H__

#include <vector>
#include <qstringlist.h>
#include <mttypes/Queue.h>
#include <mttypes/Semaphore.h>
#include "WorkflowProcessor.h"
#include <gst/vectorquery/FuseSelector.h>



namespace vectorprep {

// ****************************************************************************
// ***  Even though this is a template, the members are defined in the .cpp
// ***  The .cpp also contains explit instantiations for the class based on the
// ***  known DisplayRuleConfig(s)
// ****************************************************************************
template <class DisplayRuleConfig>
class FusePreparer {
 public:
  typedef WorkflowProcessor<DisplayRuleConfig>  Processor;

  // dumb class to hold pointer
  // intentionally does no ownership management
  // whole purpose in life is to allow a raw pointer to have
  // an operator>
  class OutTile {
   public:
    typedef typename Processor::OutTile RawType;

    OutTile(void) : ptr_() { }
    explicit OutTile(RawType* ptr) : ptr_(ptr) { }
    inline RawType* operator->(void) const { return ptr_; }
    inline RawType& operator* (void) const { return *ptr_; }
    inline operator bool(void) const { return ptr_ != 0;}
    inline bool operator>(const OutTile &other) const {
      return ptr_->IsGeneratedAfter(*other.ptr_);
    }
    inline bool operator<(const OutTile &other) const {
      return other.ptr_->IsGeneratedAfter(*ptr_);
    }

   private:
    RawType* ptr_;
    bool operator==(const OutTile& other);
  };

  typedef mttypes::Queue<OutTile>              OutQueueType;
  typedef vectorquery::FuseSelector::QueueType  InQueueType;

  FusePreparer(vectorquery::FuseSelector &selector,
               const std::vector<DisplayRuleConfig> &disprule_configs,
               const QStringList &js_context_scripts,
               std::uint32_t semaphore_count);

  mttypes::Semaphore& OutputSemaphore(void) { return out_semaphore_; }
  OutQueueType&       OutputQueue(void)     { return out_queue_; }

  void Run(void);

 private:
  InQueueType        &in_queue_;
  InQueueType        &in_queue_return_;
  std::vector<DisplayRuleConfig> disprule_configs_;

  mttypes::Semaphore  out_semaphore_;
  static khMutexBase  processor_serializer_;
  OutQueueType        out_queue_;

  Processor processor_;
};


} // namespace vectorprep


#endif // FUSION_GST_VECTORPREP_FUSEPREPARER_H__

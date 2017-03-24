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

#ifndef FUSION_GST_VECTORQUERY_FUSESELECTOR_H__
#define FUSION_GST_VECTORQUERY_FUSESELECTOR_H__

#include "FilterGeoIndex.h"
#include <mttypes/Queue.h>

namespace vectorquery {


class FuseSelector : public FilterGeoIndexManager {
 public:
  typedef mttypes::Queue<WorkflowOutputTile*> QueueType;
  typedef mttypes::BatchingQueuePusher<WorkflowOutputTile*> QueuePusherType;

  FuseSelector(uint level, const std::string &product_path,
               const std::vector<std::string> &select_files,
               const khTilespace &tilespace, double oversize_factor,
               const uint32 in_queue_seed_size,
               const uint32 out_queue_batch_size,
               const std::string& progress_meter_prefix);

  virtual WorkflowOutputTile* GetEmptyOutputTile(void);
  virtual void HandleOutputTile(WorkflowOutputTile *tile);
  virtual void ReturnEmptyOutputTile(WorkflowOutputTile *tile);

  QueueType& InputQueue(void)  { return in_queue_; }
  QueueType& OutputQueue(void) { return out_queue_; }

  void Run(void);
 private:
  QueueType       in_queue_;
  QueueType       out_queue_;
  QueuePusherType out_queue_pusher_;
};



} // namespace vectorquery





#endif // FUSION_GST_VECTORQUERY_FUSESELECTOR_H__

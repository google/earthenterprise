// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include "FuseSelector.h"

namespace vectorquery {


FuseSelector::FuseSelector(unsigned int level, const std::string &product_path,
                           const std::vector<std::string> &select_files,
                           const khTilespace &tilespace,
                           double oversize_factor,
                           const std::uint32_t in_queue_seed_size,
                           const std::uint32_t out_queue_batch_size,
                           const std::string& progress_meter_prefix) :
    FilterGeoIndexManager(level,
                          theSourceManager->GetSharedSource(product_path),
                          select_files, tilespace, oversize_factor,
                          progress_meter_prefix),
    in_queue_(),
    out_queue_(),
    out_queue_pusher_(out_queue_batch_size, out_queue_)
{
  for (unsigned int i = 0; i < in_queue_seed_size; ++i) {
    in_queue_.Push(new WorkflowOutputTile(select_files.size()));
  }
}

WorkflowOutputTile* FuseSelector::GetEmptyOutputTile(void) {
  QueueType::PullHandle pull_handle;

  // will throw if aborted, or other error occurs
  in_queue_.ReleaseOldAndPull(pull_handle);

  // empty handle is the "Done" signal. This should never be called
  // on this queue
  assert(pull_handle);

  return pull_handle->Data();
}

void FuseSelector::HandleOutputTile(WorkflowOutputTile *tile) {
  out_queue_pusher_.Push(tile);
}

void FuseSelector::ReturnEmptyOutputTile(WorkflowOutputTile *tile) {
  in_queue_.Push(tile);
}

void FuseSelector::Run(void) {
  DoSelection();
  out_queue_pusher_.PushDone();
}

} // namespace vectorquery

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


#include <autoingest/.idl/storage/MapSubLayerConfig.h>

#include "FusePreparer.h"

namespace vectorprep {


template <class DisplayRuleConfig>
FusePreparer<DisplayRuleConfig>::FusePreparer(
    vectorquery::FuseSelector &selector,
    const std::vector<DisplayRuleConfig> &disprule_configs,
    const QStringList &js_context_scripts,
    std::uint32_t semaphore_count) :
    in_queue_(selector.OutputQueue()),
    in_queue_return_(selector.InputQueue()),
    disprule_configs_(disprule_configs),
    out_semaphore_(semaphore_count),
    out_queue_(),
    processor_(selector.GetSource(),
               selector.GetTilespace(),
               selector.GetOversizeFactor(),
               disprule_configs,
               js_context_scripts)
{
}


template <class DisplayRuleConfig>
void FusePreparer<DisplayRuleConfig>::Run(void) {
  InQueueType::PullHandle next;

  while (1) {
    out_semaphore_.Acquire(1);

    // this will block until something else is available
    in_queue_.ReleaseOldAndPull(next);

    if (!next) {
      out_queue_.PushDone();
      return;
    }

    OutTile out_tile(new typename OutTile::RawType(disprule_configs_));
    out_tile->SetPath(next->Data()->path);
    // jagadev: The process part of the operations is made serial as used codes
    // like gstKVPFile.cpp and gstKVPTable.cpp are not thread safe.
    // I have verified that performance is not impacted.
    {
      khLockGuard processor_serializer_lock(processor_serializer_);
      processor_.Process(&*out_tile, *next->Data());
    }
    out_queue_.Push(out_tile);
    in_queue_return_.Push(next->Data());
  }
}


// ****************************************************************************
// ***  Explicit instantiation
// ****************************************************************************
template class FusePreparer<MapDisplayRuleConfig>;

template<>
khMutexBase FusePreparer<MapDisplayRuleConfig>::processor_serializer_ =
    KH_MUTEX_BASE_INITIALIZER;

} // namespace vectorprep

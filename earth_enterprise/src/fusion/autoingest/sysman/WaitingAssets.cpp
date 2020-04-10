/*
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

#include "WaitingAssets.h"

void WaitingAssets::Update(
    const SharedString & ref,
    AssetDefs::State newState,
    AssetDefs::State oldState,
    std::uint32_t numWaitingFor) {
  if (newState == waitingState && numWaitingFor > 0) {
    waiting[ref] = numWaitingFor;
  }
  else if (oldState == waitingState) {
    waiting.erase(ref);
  }
}

bool WaitingAssets::DecrementAndCheckWaiting(const SharedString & ref) {
  auto asset = waiting.find(ref);
  if (IsWaiting(asset)) {
    --asset->second;
    if (asset->second > 0) {
      // Still waiting
      return true;
    }
    else {
      // This was the last one we were waiting for, so we're no longer waiting
      waiting.erase(asset);
      return false;
    }
  }
  // We were not waiting to begin with
  return false;
}
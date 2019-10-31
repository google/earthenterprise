/*
 * Copyright 2019 The Open GEE Contributors
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
    AssetDefs::State oldState) {
  if (newState == waitingState) {
    waiting.insert(ref);
  }
  else if (oldState == waitingState) {
    waiting.erase(ref);
  }
}

bool WaitingAssets::IsWaiting(const SharedString & ref) const {
  return waiting.find(ref) != waiting.end();
}

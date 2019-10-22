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

#ifndef WAITINGASSETS_H
#define WAITINGASSETS_H

#include "autoingest/.idl/storage/AssetDefs.h"
#include "common/SharedString.h"

#include <unordered_set>

class WaitingAssets {
  private:
    const AssetDefs::State waitingState;
    std::unordered_set<SharedString> waiting;
  public:
    WaitingAssets(AssetDefs::State waitingState) : waitingState(waitingState) {}
    void Update(
        const SharedString & ref,
        AssetDefs::State newState,
        AssetDefs::State oldState);
    bool IsWaiting(const SharedString & ref) const;
};

#endif // WAITINGASSETS_H

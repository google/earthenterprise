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

#include "AssetOperation.h"
#include "AssetVersionD.h"
#include "MiscConfig.h"
#include "StateUpdater.h"

void RebuildVersion(const SharedString & ref) {
  if (MiscConfig::Instance().GraphOperations) {
    // Rebuilding an already succeeded asset is quite dangerous!
    // Those who depend on me may have already finished their work with me.
    // If I rebuild, they have the right to recognize that nothing has
    // changed (based on my version # and their config) and just reuse their
    // previous results. Those previous results may reference my outputs.
    // But if my inputs reference disk files that could change (sources
    // overwritten with new versions), I may change some of my outputs,
    // thereby invalidating the cached work from later stages.
    //
    // For this reason, requests to rebuild 'Succeeded' versions will fail.
    // Assets marked 'Bad' were once succeeded, so they too are disallowed.
    // The same logic could hold true for 'Offline' as well.
    {
      // Limit the scope to release the AssetVersion as quickly as possible.
      AssetVersion version(ref);
      if (version->state & (AssetDefs::Succeeded | AssetDefs::Offline | AssetDefs::Bad)) {
        throw khException(kh::tr("%1 marked as %2. Refusing to resume.")
                          .arg(ToQString(ref), ToQString(version->state)));
      }
    }

    StateUpdater updater;
    updater.SetStateForRefAndDependents(ref, AssetDefs::New, AssetDefs::CanRebuild);
  }
  else {
    MutableAssetVersionD version(ref);
    version->Rebuild();
  }
}

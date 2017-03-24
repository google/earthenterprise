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

#ifndef FUSION_GST_VECTORQUERY_TILES_H__
#define FUSION_GST_VECTORQUERY_TILES_H__

#include <vector>
#include <quadtreepath.h>
#include <khGuard.h>

namespace vectorquery {

class DisplayRuleTile {
 public:
  std::vector<int> selectedIds;

  void Reset(void) {
    selectedIds.resize(0);
  }
};

class LayerTile {
 public:
  inline LayerTile(uint count) : displayRules(count) { }

  std::vector<DisplayRuleTile> displayRules;

  void Reset(void) {
    for (uint i = 0; i < displayRules.size(); ++i) {
      displayRules[i].Reset();
    }
  }
};


class WorkflowOutputTile : public LayerTile {
 public:
  inline WorkflowOutputTile(uint count) :
      LayerTile(count),
      path()
  { }

  QuadtreePath path;
};

inline void ResetTile(WorkflowOutputTile *t) {
  t->Reset();
}



} // namespace vectorquery

#endif // FUSION_GST_VECTORQUERY_TILES_H__

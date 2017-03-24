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


#ifndef FUSION_KHRASTER_ATTRIBUTIONBYEXTENTS_H__
#define FUSION_KHRASTER_ATTRIBUTIONBYEXTENTS_H__

#include <string>
#include <vector>
#include <khTypes.h>
#include <khExtents.h>
#include <khTileAddr.h>


class AttributionByExtents {
  class Inset {
   public:
    uint32            max_level_;
    khExtents<double> deg_extents_;
    uint32            inset_id_;
    std::string       acquisition_date_;

    Inset(const std::string &rppath, uint32 id);
  };

  // will we ordered low-res to high-res
  std::vector<Inset> insets;

public:
  AttributionByExtents(void) { }

  void AddInset(const std::string &rppath, uint32 id) {
    insets.push_back(Inset(rppath, id));
  }

  // Return the inset id for the most significant inset for the
  // specified tile.
  uint32 GetInsetId(const khTileAddr &addr) const;

  // Return the acquisition date for the most significant inset for the
  // specified tile.
  const std::string& GetAcquisitionDate(const khTileAddr &addr) const;

private:
  // Internal utility to find the inset within insets that occupies the most
  // area in the specified tile.
  uint32 GetInternalInsetIndex(const khTileAddr &addr) const;
};

#endif // FUSION_KHRASTER_ATTRIBUTIONBYEXTENTS_H__

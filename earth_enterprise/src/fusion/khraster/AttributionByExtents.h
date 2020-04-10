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


#ifndef FUSION_KHRASTER_ATTRIBUTIONBYEXTENTS_H__
#define FUSION_KHRASTER_ATTRIBUTIONBYEXTENTS_H__

#include <cstdint>
#include <string>
#include <vector>
#include <khExtents.h>
#include <khTileAddr.h>


class AttributionByExtents {
  class Inset {
   public:
    std::uint32_t            max_level_;
    khExtents<double> deg_extents_;
    std::uint32_t            inset_id_;
    std::string       acquisition_date_;

    Inset(const std::string &rppath, std::uint32_t id, const std::string &acquisitionDate);
  };

  // will we ordered low-res to high-res
  std::vector<Inset> insets;

public:
  AttributionByExtents(void) { }

  void AddInset(const std::string &rppath, std::uint32_t id, const std::string &acquisitionDate) {
    insets.push_back(Inset(rppath, id, acquisitionDate));
  }

  // Return the inset id for the most significant inset for the
  // specified tile.
  std::uint32_t GetInsetId(const khTileAddr &addr) const;

  // Return the acquisition date for the most significant inset for the
  // specified tile.
  const std::string& GetAcquisitionDate(const khTileAddr &addr) const;

private:
  // Internal utility to find the inset within insets that occupies the most
  // area in the specified tile.
  std::uint32_t GetInternalInsetIndex(const khTileAddr &addr) const;
};

#endif // FUSION_KHRASTER_ATTRIBUTIONBYEXTENTS_H__

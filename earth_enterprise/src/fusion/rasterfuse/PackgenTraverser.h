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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_PACKGENTRAVERSER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_PACKGENTRAVERSER_H_

#include <string>
//#include "common/khTypes.h"
#include <cstdint>
#include "common/khMTTypes.h"
#include "common/khTileAddr.h"
#include "common/khGuard.h"
#include "fusion/rasterfuse/TileFetcher.h"
#include "fusion/autoingest/.idl/storage/PacketLevelConfig.h"
#include "fusion/khraster/ffioRasterWriter.h"
#include "fusion/khraster/AttributionByExtents.h"
#include "common/khMTProgressMeter.h"


namespace fusion {
namespace rasterfuse {

// ****************************************************************************
// ***  PackgenTraverser
// ***  Will be inherited by ImageryPackgenTraverser and TmeshPackgenTraverser
// ***  Provides comon functionality between the two
// ***
// ***  Config object must contain the following definitions:
// ***     DataTile
// ***     PrepItem
// ***     WorkItem
// ***     Blender
// ***     Merger
// ****************************************************************************

template <class Config>
class PackgenTraverser {
 public:
  bool IsMercator() const { return is_mercator_; }

 protected:
  typedef typename Config::DataTile DataTile;
  typedef typename Config::PrepItem PrepItem;
  typedef typename Config::WorkItem WorkItem;
  typedef typename Config::Blender  Blender;
  typedef typename Config::Merger   Merger;

  typedef TileFetcher<DataTile> Fetcher;
  typedef khMTProgressMeter ProgressMeter;

  const bool                       is_mercator_;
  PacketLevelConfig                config;
  std::string                      output;
  ffio::raster::Subtype            cacheSubtype;
  khLevelCoverage                  productCoverage;
  khDeleteGuard<Fetcher>           fetcher;
  khDeleteGuard<ffio::raster::Writer<DataTile> > cached_blend_writer_;
  khDeleteGuard<
    ffio::raster::Writer<AlphaProductTile> > cached_blend_alpha_writer_;
  khDeleteGuard<ProgressMeter> progress;  // delayed instantiation
  AttributionByExtents             attributions;

  khMTQueue<PrepItem*> prepEmpty;
  khMTQueue<PrepItem*> prepFull;
  khMTQueue<WorkItem*> workEmpty;
  khMTQueue<WorkItem*> workFull;

  virtual PrepItem* NewPrepItem(void) = 0;
  virtual WorkItem* NewWorkItem(void) = 0;

  // Thread functions
  virtual void PrepLoop(void) = 0;
  void WorkLoop(void);
  void WriteLoop(void);

  PackgenTraverser(const PacketLevelConfig &config,
                   const std::string &output,
                   const khLevelCoverage &productCoverage_,
                   ffio::raster::Subtype cacheSubtype);
  virtual ~PackgenTraverser(void) { }
  void DoTraverse(unsigned int numWorkThreads);
};

}  // namespace rasterfuse
}  // namespace fusion

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_PACKGENTRAVERSER_H_

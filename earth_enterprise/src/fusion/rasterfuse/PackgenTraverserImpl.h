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

/******************************************************************************
File:        PackgenTraverserImpl.h
Description:

******************************************************************************/
#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_PACKGENTRAVERSERIMPL_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_PACKGENTRAVERSERIMPL_H_

#include "fusion/rasterfuse/RasterBlender.h"
#include "fusion/rasterfuse/RasterMerger.h"
#include "fusion/khraster/ffioRasterWriter.h"

namespace fusion {
namespace rasterfuse {

// ****************************************************************************
// ***  Out of line template functions for PackgenTraverser
// ***    - This file is included from ImageryPackgenTraverser.cpp
// ***    and TmeshPackgenTraverser.cpp
// ****************************************************************************

template <class Config>
PackgenTraverser<Config>::PackgenTraverser(
    const PacketLevelConfig &cfg, const std::string &output_,
    const khLevelCoverage &productCoverage_,
    ffio::raster::Subtype cacheSubtype_)
    // If the first file is in a *.kimasset directory it is mercator.
    : is_mercator_(!cfg.insets.empty() &&
                 cfg.insets[0].dataRP.find(
                     std::string(kMercatorImageryAssetSuffix) + "/") !=
                 std::string::npos),
      config(cfg),
      output(output_),
      cacheSubtype(cacheSubtype_),
      productCoverage(productCoverage_),
      fetcher(),
      attributions() {
  for (std::vector<PacketLevelConfig::Attribution>::iterator attribution =
         config.attributions.begin();
       attribution != config.attributions.end(); ++attribution) {
    if (!attribution->dataRP.empty() &&
        (attribution->fuid_resource_ != 0)) {
      std::string acquisitionDate = kUnknownDateTimeUTC;
      if (!attribution->acquisitionDate.empty()) {
        acquisitionDate = attribution->acquisitionDate;
      }
      attributions.AddInset(attribution->dataRP, attribution->fuid_resource_, acquisitionDate);
    }
  }
}

template <class Config>
void PackgenTraverser<Config>::DoTraverse(unsigned int numWorkThreads) {
  if (numWorkThreads == 0) {
    throw khException(kh::tr("No work threads. Cannot proceed."));
  }

  // Figure out if we're blending or merging
  typedef TileLoader<DataTile> Loader;
  khTransferGuard<Loader> loader;
  if (config.minify) {
    notify(NFY_NOTICE, "Merge level %u from level %u",
           config.coverage.level, config.coverage.level+1);
    loader = new Merger(productCoverage.level,
                        config.insets,
                        config.mergeTopDataRP,
                        config.mergeTopAlphaRP);
  } else {
    notify(NFY_NOTICE, "Blend level %u from products",
           config.coverage.level);
    loader = new Blender(productCoverage.level,
                         config.insets,
                         config.skipTransparent, IsMercator());
  }

  // If we need to cache the blend results, build writers. We must do this
  // here in Traverse because cache.pak and cache_alpha.pack are subdirs of
  // output which gets created by our derived class _after_ we finish our
  // constructor.
  if (config.cacheRaster) {
    notify(NFY_NOTICE, "Cache level %d results for later merging",
           config.coverage.level);
    cached_blend_writer_ = TransferOwnership(
        new ffio::raster::Writer<DataTile>(cacheSubtype,
                                           output + "/cache.pack",
                                           productCoverage));

    cached_blend_alpha_writer_ = TransferOwnership(
        new ffio::raster::Writer<AlphaProductTile>(ffio::raster::AlphaCached,
                                                   output + "/cache_alpha.pack",
                                                   productCoverage));
  }

  // Build the high level (caching) fetcher
  fetcher = TransferOwnership(
      new Fetcher(loader,  /* fetcher will assume ownsership */
                  cached_blend_writer_,
                  cached_blend_alpha_writer_));


  // Seed the prep queue with twice the number as we have work threads.
  // This means the PrepLoop will work until he has the next PrepItem
  // for each WorkLoop ready to go
  unsigned int numPrepItems = numWorkThreads*2;
  for (unsigned int i = 0; i < numPrepItems; ++i) {
    prepEmpty.push(NewPrepItem());
  }

  // Seed the work queue with twice the number as we have work threads.
  // This means the WorkLoop can always have one while the write loop
  // has another to stream to disk.
  unsigned int numWorkItems = numWorkThreads*2;
  for (unsigned int i = 0; i < numWorkItems; ++i) {
    workEmpty.push(NewWorkItem());
  }

  // start the work threads
  khThread* workThreads[numWorkThreads];
  for (unsigned int i = 0; i < numWorkThreads; ++i) {
    khFunctor<void> func =
      khFunctor<void>(std::mem_fun(&PackgenTraverser::WorkLoop), this);
    workThreads[i] = new khThread(func);
    workThreads[i]->run();
  }

  progress = TransferOwnership(
      new ProgressMeter(
          static_cast<std::uint64_t>(config.coverage.extents.width()) *
          static_cast<std::uint64_t>(config.coverage.extents.height())));

  // start the writer thread
  khFunctor<void> func =
    khFunctor<void>(std::mem_fun(&PackgenTraverser::WriteLoop),
                    this);
  khThread writeThread(func);
  writeThread.run();

  PrepLoop();

  // all tiles have been submitted to the work threads. Signal them to
  // exit once they've finished. Each one will pull one of these sentinal
  // entries out of the queue and then exit
  for (unsigned int i = 0; i < numWorkThreads; ++i) {
    prepFull.push(0);
  }

  // wait for each of the work threads, join with them and clean them up
  for (unsigned int i = 0; i < numWorkThreads; ++i) {
    workThreads[i]->join();
    delete workThreads[i];
  }

  // now that the work is done, signal the write thread and join w/ it
  workFull.push(0);
  writeThread.join();

  // cleanup
  for (unsigned int i = 0; i < numPrepItems; ++i) {
    PrepItem* prep = prepEmpty.pop();
    delete prep;
  }
  for (unsigned int i = 0; i < numWorkItems; ++i) {
    WorkItem* work = workEmpty.pop();
    delete work;
  }
}


template <class Config>
void PackgenTraverser<Config>::WorkLoop(void) {
  try {
    while (1) {
      // will block until there is something else in the queue to do
      PrepItem *prep = prepFull.pop();

      // 0 is a sentinal that the work is done - time for me to go away.
      if (!prep) {
        return;
      }

      // will block until there is an available WorkItem
      WorkItem *work = workEmpty.pop();

      // do the real/slow work
      work->DoWork(prep);

      // relrease prepItem
      prepEmpty.push(prep);

      // queue the results to be written
      workFull.push(work);
    }
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "WorkLoop: %s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "WorkLoop: unknown error");
  }
}


template <class Config>
void PackgenTraverser<Config>::WriteLoop(void) {
  try {
    while (1) {
      // will block until there is something else in the queue
      WorkItem *work = workFull.pop();

      // 0 is a sentinal that the work is done - time for me to go away.
      if (!work) {
        return;
      }

      // do the actual write
      work->DoWrite(progress);

      // release workItem
      workEmpty.push(work);
    }
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "WriteLoop: %s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "WriteLoop: unknown error");
  }
}

}  // namespace rasterfuse
}  // namespace fusion

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_PACKGENTRAVERSERIMPL_H_

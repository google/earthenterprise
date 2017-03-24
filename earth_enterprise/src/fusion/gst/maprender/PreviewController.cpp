// Copyright 2017 Google Inc.
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


#include "PreviewController.h"
#include "PreviewCombiner.h"
#include "PreviewRenderer.h"
#include <khraster/Extract.h>
#include <khraster/Premultiply.h>
#include <compressor.h>
#include <khFileUtils.h>

namespace maprender {

PreviewController::PreviewController(const khTilespace &tilespace_,
                                     double oversizeFactor_,
                                     const MapLayerConfig &config_,
                                     gstProgress &progress,
                                     PreviewController *prev,
                                     bool debug_) :
    tilespace(tilespace_),
    oversizeFactor(oversizeFactor_),
    addrShift(tilespace.tileSizeLog2 - tilespace.pixelsAtLevel0Log2),
    config(config_),
    cache(5 /* only cache a small number of these */),
    debug(debug_)
{
  SelectorVector dummy;
  BuildChildren(prev ? prev->selectors : dummy, progress);
}

PreviewController::~PreviewController(void)
{
}

void
PreviewController::BuildChildren(SelectorVector &oldSelectors,
                                 gstProgress &progress)
{
  // blow away our cache
  cache.clear();

  uint numSubs = config.subLayers.size();

  // no extern context scripts for now
  const QStringList jsContextScripts;

  validLevels = geMultiRange<uint32>();
  selectors.reserve(numSubs);
  preparers.reserve(numSubs);
  std::vector<workflow::PreviewStep<CombinerInputTile>*> combinerInputSteps;
  std::vector<khSharedHandle<CombinerInputTile> > combinerInputs;
  combinerInputs.reserve(numSubs);

  for (uint s = 0; s < numSubs; ++s) {
    // get the basic configuration to build the per-sublayer pieces
    MapSubLayerConfig &subConfig = config.subLayers[s];
    QueryConfig qconfig(subConfig, jsContextScripts);
    std::vector<geMultiRange<uint32> > subValidLevels;
    uint numRules = subConfig.display_rules.size();
    subValidLevels.reserve(numRules);
    for (uint i = 0; i < numRules; ++i) {
      subValidLevels.push_back(subConfig.display_rules[i].ValidLevels());
      validLevels = geMultiRange<uint32>::Union(
          validLevels, subConfig.display_rules[i].ValidLevels());
    }
    gstSharedSource source =
      theSourceManager->GetSharedAssetSource(subConfig.asset_ref);

    // see if we can reuse an existing selector step
    bool found = false;
    for (uint os = 0; os < oldSelectors.size(); ++os) {
      SelectorStepHandle old = oldSelectors[os];

      if (old && old->CanReuse(source, qconfig, subValidLevels)) {
        selectors.push_back(oldSelectors[os]);
        found = true;
        break;
      }
    }

    // if not, make a new one
    if (!found) {
      selectors.push_back(
          SelectorStepHandle(
              TransferOwnership(
                  new SelectorStep(source,
                                   qconfig, subValidLevels,
                                   tilespace, oversizeFactor))));
      // make all soft errors fatal. We should only be working
      // with pre-built vector resources, so we don't want to be lenient
      // with source errors
      SoftErrorPolicy soft_errors(0);
      selectors.back()->ApplyQueries(progress, soft_errors);
    }

    preparers.push_back(
        new PreparerStep(source,
                         tilespace, oversizeFactor,
                         subConfig.display_rules,
                         jsContextScripts,
                         *selectors[s],
                         khSharedHandle<SelectorStep::OutTile> (
                             TransferOwnership(
                                 new SelectorStep::OutTile(numRules)))
                         ));
    combinerInputSteps.push_back(&*preparers[s]);
    combinerInputs.push_back(
        khSharedHandle<CombinerInputTile>(
            TransferOwnership(
                new CombinerInputTile(subConfig.display_rules))));
  }

  combiner = TransferOwnership(new CombinerStep(tilespace, combinerInputSteps,
                                                combinerInputs));
  renderer = TransferOwnership(new RendererStep(*combiner, debug));

  oldSelectors.resize(0);
}



bool
PreviewController::GetTile(uint64 addr, uchar *outBuf) {
  // address of 256x256 requested
  uint32 previewLevel = LEVFROMADDR(addr);
  uint32 previewRow   = ROWFROMADDR(addr);
  uint32 previewCol   = COLFROMADDR(addr);

  // address of larger tile to render
  uint32 renderLevel = previewLevel;
  uint32 renderRow   = previewRow >> addrShift;
  uint32 renderCol   = previewCol >> addrShift;

  // Fetch the tile from cache
  QuadtreePath path(renderLevel, renderRow, renderCol);
  khSharedHandle<RendererOutputTile> found;
  if (!cache.Find(path, found)) {
    notify(NFY_VERBOSE, "Render lrc(%u,%u,%u)",
           renderLevel, renderRow, renderCol);
    // make a new one, render into it, add to cache
    found = TransferOwnership(new RendererOutputTile(tilespace.tileSize));
    found->path = path;
    if (!renderer->Fetch(&*found)) {
      // fill with transparent tile
      if (debug) {
        memset(&found->pixelBuf[0], 200,
               tilespace.tileSize * tilespace.tileSize * 4);
      } else {
        memset(&found->pixelBuf[0], 0,
               tilespace.tileSize * tilespace.tileSize * 4);
      }
    } else {
#if 0
      {
        uint options = 0;
#if __BYTE_ORDER == __BIG_ENDIAN
        options = PNGOPT_SWAPALPHA;
#else
        options = PNGOPT_BGR;
#endif
        khDeleteGuard<Compressor> png_compressor(
            TransferOwnership(NewPNGCompressor(tilespace.tileSize,
                                               tilespace.tileSize,
                                               4, options)));
        png_compressor->compress(&found->pixelBuf[0]);
        khWriteSimpleFile("/home/jdjohnso/tmp/gui-" +
                          path.AsString() + ".png",
                          png_compressor->data(),
                          png_compressor->dataLen());
      }
#endif

      UndoAlphaPremultiplyARGB((uint*)&found->pixelBuf[0],
                               tilespace.tileSize * tilespace.tileSize);
    }

    cache.Add(path, found);
  }



  uint32 extractTileRow = previewRow & ((1<<addrShift) - 1);
  uint32 extractTileCol = previewCol & ((1<<addrShift) - 1);

  // we're flipping the Y axis (invert the extract row)
  extractTileRow = ((1<<addrShift)-1) - extractTileRow;

  ExtractInterleavedSubBufferFlipY(
      reinterpret_cast<char*>(outBuf),
      khExtents<uint32>(khOffset<uint32>(RowColOrder,
                                         extractTileRow * 256,
                                         extractTileCol * 256),
                        khSize<uint32>(256, 256)),
      found->pixelBuf,
      khSize<uint32>(tilespace.tileSize, tilespace.tileSize),
      4 /* rgba, or argb, or somthing like that */);
  return true;
}

bool PreviewController::HasLevel(uint32 level) {
  return validLevels.Contains(level);
}

} // namespace maprender

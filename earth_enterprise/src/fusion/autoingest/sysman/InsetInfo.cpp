// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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


/******************************************************************************
File:        InsetInfo.cpp
Description:

Changes:
Description: Support for terrain "overlay" projects.

******************************************************************************/
#include "fusion/autoingest/sysman/InsetInfo.h"
#include "common/khException.h"
#include "autoingest/plugins/RasterProductAsset.h"
#include "autoingest/plugins/MercatorRasterProductAsset.h"


// ****************************************************************************
// ***  SimpleInsetInfo
// ****************************************************************************
template <typename ProductAssetVersion>
SimpleInsetInfo<ProductAssetVersion>::SimpleInsetInfo(
                                 const khTilespace & /* unused1 */,
                                 const ProductAssetVersion &resource,
                                 const CombinedRPAssetVersion &combinedrp,
                                 unsigned int  effectiveMaxProdLev,
                                 unsigned int  /* unused2 */,
                                 unsigned int  /* unused3 */) :
    resource_(resource),
    combinedrp_(combinedrp),
    effectiveMaxLevel(combinedrp->type == AssetDefs::Imagery ?
                      ProductToImageryLevel(effectiveMaxProdLev) :
                      ProductToTmeshLevel(effectiveMaxProdLev)),
    date_string_(resource->GetAcquisitionDate()),
    degExtents(combinedrp->GetExtents()),
    fullresLevel(combinedrp->type == AssetDefs::Imagery ?
                 ProductToImageryLevel(combinedrp->GetMaxLevel()) :
                 ProductToTmeshLevel(combinedrp->GetMaxLevel())) {
}

// ****************************************************************************
// ***  InsetInfo
// ****************************************************************************
template <typename ProductAssetVersion>
InsetInfo<ProductAssetVersion>::InsetInfo(
    const khTilespace &tilespace,
    const ProductAssetVersion &resource,
    const CombinedRPAssetVersion &combinedrp,
    unsigned int  effectiveMaxProdLev,
    unsigned int  beginCovLev,
    unsigned int  endCovLev)
    : SimpleInsetInfo<ProductAssetVersion>(tilespace, resource, combinedrp,
                    effectiveMaxProdLev, beginCovLev, endCovLev),
      coverage(tilespace,
               this->degExtents, this->fullresLevel,
               beginCovLev, endCovLev,
               0 /* step out */,
               0 /* padding */) {
  // use OutputFilenames to get dataRP & alphaRP filenames.
  // We can use OutputFilenames since these asset vers are by definition
  // already successfull and we're writing the filename to a
  // config file that needs it bound
  std::vector<std::string> productFilenames;
  productFilenames.reserve(2);
  this->combinedrp_->GetOutputFilenames(productFilenames);
  dataRPFile = productFilenames[0];
  if (productFilenames.size() > 1) {
    alphaRPFile = productFilenames[1];
  }
}


// ****************************************************************************
// ***  PacketGenInfo
// ****************************************************************************
void PacketGenInfo::PopulatePacketLevels(AssetVersion packgenVer) {
  if (packgenVer->children.size() != coverage.numLevels()) {
    notify(NFY_DEBUG, "PopulatePacketLevels: kids size() %lu, numLevels() %d",
           (unsigned long)packgenVer->children.size(), coverage.numLevels());
    // In version 3.1, startTMeshLevel changed from 8 to 4. If the terrain
    // project was built under 3.0, then it only has terrain packets and
    // minified resources up to level 8. That's okay: the code below checks for
    // existing children of the old packgenVer, and only keeps and reuses what
    // was actually built. Check as many things as we can to make sure that
    // we're dealing with an upgrade case, and if so allow it to pass.
    if (packgenVer->children.size() != coverage.numLevels() - 4 ||
        coverage.beginLevel() != 4) {
      throw khException
          (kh::tr("Internal Error: PacketGenInfo numchildren mismatch"));
    }
  }

  // store the ref to the packgen, it will be used as part of
  // subsequent packgen configs
  packetgenverref = packgenVer->GetRef();

  // we want our packetLevels to be indexed by level number, so size
  // it accordingly.
  packetLevels.resize(coverage.endLevel());

  unsigned int numkids = packgenVer->children.size();
  for (unsigned int c = 0; c < numkids; ++c) {
    // my children are in reverse level order, because that's how they
    // are built.
    unsigned int levelNum = coverage.endLevel() - c - 1;
    packetLevels[levelNum] = packgenVer->children[c];
  }
}

// Iterator to expose the others' coverage w/o copying
class CovIterator {
  const PacketGenInfo *curr;
 public:
  inline explicit CovIterator(const PacketGenInfo *c) : curr(c) { }

  inline const khInsetCoverage* operator->(void) const {
    return &curr->coverage;
  }
  inline const khInsetCoverage& operator*(void) const {
    return *operator->();
  }

  inline CovIterator operator++(void) {
    ++curr;
    return *this;
  }
  inline bool operator==(const CovIterator &o) const {
    return (curr == o.curr);
  }
  inline bool operator!=(const CovIterator &o) const {
    return !operator==(o);
  }
};


bool
PacketGenInfo::NarrowCoverage(const PacketGenInfo *beginOthers,
                              const PacketGenInfo *endOthers) {
  if (endOthers <= beginOthers) {
    // there are no others - there's no way to narrow
    return false;
  }

  return coverage.Narrow(CovIterator(beginOthers), CovIterator(endOthers));
}



// ****************************************************************************
// ***  CalcPacketGenInfo
// ****************************************************************************
template <class InsetInfo>
void CalcPacketGenInfo(const khTilespace            &tilespace,
                       const AssetDefs::Type         gentype,
                       const std::vector<InsetInfo> &insetInfos,
                       const std::uint32_t                  beginCoverageLevel,
                       std::vector<PacketGenInfo>   &genInfos,
                       const std::int32_t                   maxLevelDelta,
                       const bool   is_overlay_terrain_proj,
                       const std::uint32_t overlay_terrain_resources_min_level) {
  if (insetInfos.empty()) {
    genInfos.clear();
    return;
  }

  // As of client 4.0, the client no longer needs imagery padding. Bug1019418.
  std::uint32_t padding = 0;
  // Old comment and version (preserved for documentation):
  // Imagery insets need to be padded by four tiles in each direction.
  // This is is so a client "detail area" of 1024x0124 (currently the
  // biggest) will still be able to load all "real" high res data. The padded
  // tiles will be magnified versions of the tiles underneath.
  // unsigned int padding = (gentype == AssetDefs::Imagery) ? 4 : 0;

  // Terrain tiles need to step out one tile at each level. This makes sure
  // that there is only one level of detail difference between any two
  // terrain tiles.
  std::uint32_t stepout = (gentype == AssetDefs::Imagery) ? 0 : 1;

  // Optimization for Terrain "Overlay"-mode:
  // To build "Overlay"-insets we need only those tiles from
  // "Filling"-insets which are intersecting with "Overlay"-insets
  // extents. So we calculate DegreeExtents for "Filling"-insets based on
  // DegreeExtents of "Overlay"-insets.
  khExtents<double> deg_extents_filling;
  if (gentype == AssetDefs::Terrain && is_overlay_terrain_proj) {
    for (size_t i = 0; i < insetInfos.size(); ++i) {
      const InsetInfo &insetInfo = insetInfos[i];
      if (insetInfo.fullresLevel >= overlay_terrain_resources_min_level) {
        // it's "Overlay"-inset - grow extents.
        deg_extents_filling.grow(insetInfo.degExtents);
      }
    }
  }

  // populate list - inital coverage estimate from inset degExtents
  genInfos.reserve(insetInfos.size());

  for (size_t i = 0; i < insetInfos.size(); ++i) {
    const InsetInfo &insetInfo = insetInfos[i];
    AssetVersionRef verref =
      AssetVersionRef(insetInfo.combinedrp_.Ref());
    std::string assetname =
      AssetImpl::WorkingDir(verref.AssetRef())
      + "packgen" + AssetDefs::FileExtension(gentype, "PacketGen");

    std::uint32_t beginCovLevel = beginCoverageLevel;

    // "+ 1" since endLevel is "one beyond" max for ctor khInsetCoverage.
    // add maxLevelDelta in case insetLevels aren't the same as coverage levels.
    std::int32_t endLevel = insetInfo.effectiveMaxLevel + maxLevelDelta + 1;

    if (endLevel < static_cast<std::int32_t>(beginCovLevel)) {
      endLevel = static_cast<std::int32_t>(beginCovLevel);  // Coverage will be empty.
    }

    if (gentype == AssetDefs::Imagery) {  // Imagery inset.
      genInfos.push_back(
          PacketGenInfo(assetname,
                        khInsetCoverage(tilespace,
                                        insetInfo.degExtents,
                                        insetInfo.fullresLevel,
                                        beginCovLevel,
                                        static_cast<std::uint32_t>(endLevel),
                                        stepout,
                                        padding)));
    } else {   // Terrain inset.
      assert(gentype == AssetDefs::Terrain);

      TerrainInsetType terrain_inset_type = kNormalTerrainInset;
      if (is_overlay_terrain_proj) {
        terrain_inset_type =
            (insetInfo.fullresLevel <
             overlay_terrain_resources_min_level) ?
            kFillingTerrainInset : kOverlayTerrainInset;

        if (kFillingTerrainInset == terrain_inset_type) {
          // Skip building first level from range (lowest resolution
          // level) for "Filling"-terrain insets. It is not needed
          // to build "overlay" insets levels (for build level 8 we use
          // level 9, so we build "overlay"-insets for levels
          // [startLevel, endLevel] and "filling insets for levels
          // [startLevel+1, endLevel].
          ++beginCovLevel;
          if (endLevel < static_cast<std::int32_t>(beginCovLevel)) {
            // Coverage will be empty.
            endLevel =  static_cast<std::int32_t>(beginCovLevel);
          }
        }
      }

      genInfos.push_back(
          PacketGenInfo(
              assetname,
              terrain_inset_type,
              khInsetCoverage(tilespace,
                              ((kFillingTerrainInset == terrain_inset_type) ?
                               deg_extents_filling : insetInfo.degExtents),
                              insetInfo.fullresLevel,
                              beginCovLevel,
                              static_cast<std::uint32_t>(endLevel),
                              stepout,
                              padding)));
    }
  }

  // Narrow the work levels based on replicated work of later insets.
  // Don't bother checking the last one, nothing follows it.
  // Note: check above (insetInfos for empty) guarantees that genInfos
  // size is not equal 0.
  for (size_t i = 0; i < genInfos.size()-1; ++i) {
    genInfos[i].NarrowCoverage(&genInfos[i+1], &genInfos[genInfos.size()]);
  }
}

// *** explicit instantiation for our two types of InsetInfo ***
template
void CalcPacketGenInfo<SimpleInsetInfo<RasterProductAssetVersion> > (
    const khTilespace     &tilespace,
    const AssetDefs::Type  gentype,
    const std::vector<SimpleInsetInfo<RasterProductAssetVersion> > &insetInfo,
    const std::uint32_t           beginCovLevel,
    std::vector<PacketGenInfo> &genInfo,
    const std::int32_t            maxLevelDelta,
    const bool             is_overlay_terrain_proj,
    const std::uint32_t           overlay_terrain_resources_min_level);

template
void CalcPacketGenInfo<InsetInfo<RasterProductAssetVersion> >(
    const khTilespace     &tilespace,
    const AssetDefs::Type  gentype,
    const std::vector<InsetInfo<RasterProductAssetVersion> > &insetInfo,
    const std::uint32_t           beginCovLevel,
    std::vector<PacketGenInfo> &genInfo,
    const std::int32_t            maxLevelDelta,
    const bool             is_overlay_terrain_proj,
    const std::uint32_t           overlay_terrain_resources_min_level);

template
void CalcPacketGenInfo<SimpleInsetInfo<MercatorRasterProductAssetVersion> > (
    const khTilespace     &tilespace,
    const AssetDefs::Type  gentype,
    const std::vector<SimpleInsetInfo<MercatorRasterProductAssetVersion> >
        &insetInfo,
    const std::uint32_t           beginCovLevel,
    std::vector<PacketGenInfo> &genInfo,
    const std::int32_t            maxLevelDelta,
    const bool             is_overlay_terrain_proj,
    const std::uint32_t           overlay_terrain_resources_min_level);

template
void CalcPacketGenInfo<InsetInfo<MercatorRasterProductAssetVersion> > (
    const khTilespace     &tilespace,
    const AssetDefs::Type  gentype,
    const std::vector<InsetInfo<MercatorRasterProductAssetVersion> > &insetInfo,
    const std::uint32_t           beginCovLevel,
    std::vector<PacketGenInfo> &genInfo,
    const std::int32_t            maxLevelDelta,
    const bool             is_overlay_terrain_proj,
    const std::uint32_t           overlay_terrain_resources_min_level);

template <typename ProductAssetVersion>
void OverlapCalculator<ProductAssetVersion>::
    CalculateOverlap(std::vector< unsigned int> & neededIndexes,
                     const khInsetCoverage& gencov)
{
    if (env.type == AssetDefs::Imagery)
    {
        FindNeededImageryInsets(gencov,
                                env.insets,
                                env.numInsets,
                                neededIndexes,
                                env.beginMinifyLevel,
                                env.endMinifyLevel);
    }
    else
    {
        FindNeededTerrainInsets(gencov,
                                env.insets,
                                env.numInsets,
                                neededIndexes,
                                env.beginMinifyLevel,
                                env.endMinifyLevel);
    }
}

template class OverlapCalculator<MercatorRasterProductAssetVersion>;
template class OverlapCalculator<RasterProductAssetVersion>;

// ****************************************************************************
// ***  FindNeededImageryInsets
// ****************************************************************************
template <typename ProductAssetVersion>
void FindNeededImageryInsets(
    const khInsetCoverage        &gencov,
    const std::vector<const InsetInfo<ProductAssetVersion> *> &insets,
    unsigned int                          numInsets,
    std::vector< unsigned int>             &neededIndexes,
    unsigned int beginMinifyLevel,
    unsigned int endMinifyLevel) {
  // For now we know that gencov's levelExtents are simple minifications
  // of the maxres one. That means that we can do the intersection only at
  // the lowest res and know that we will get all the insets needed for
  // the other levels too. Later, NarrowCoverage may narrow the extents at
  // the various levels too. When that happens, we'll need to check for
  // intersections at each level to make sure we get them all.

  if (gencov.numLevels() == 0) {
    // catch degenerate case
    return;
  }

  unsigned int level = gencov.beginLevel();

  // Packgen will be caching the results - the cached results are
  // in product tilespace, so we need to do our intersection tests
  // snapped out to product tile boundaries
  // If gencov has more than one level, we're finding insets for a PacketGen
  // asset. Always do the align for PacketGen, so we make sure we have a
  // superset of the insets that each PacketLevelGen will need. PacketGen
  // doesn't really depend on the list returned from here. He just
  // pre-gathers them so the checks for PacketLevelGen don't have to compare
  // against ALL insets.
  bool needAlign = ((gencov.numLevels() > 1) ||
                    ((level > beginMinifyLevel)&&(level <= endMinifyLevel)));
  unsigned int alignSize = (RasterProductTilespaceBase.tileSize /
                    ClientImageryTilespaceBase.tileSize);

  khExtents<std::uint32_t> genExtents(gencov.levelExtents(level));
  if (needAlign) {
    genExtents.alignBy(alignSize);
  }

  for (unsigned int i = 0; i < numInsets; ++i) {
    // Aligning here would be redundant, so we save ourselves the effort.
    const khExtents<std::uint32_t> &iExtents
      (insets[i]->coverage.levelExtents(level));

    if (iExtents.intersects(genExtents)) {
      neededIndexes.push_back(i);
    }
  }
}

// Explicit instantiation
template
void
FindNeededImageryInsets(
    const khInsetCoverage        &gencov,
    const std::vector<const InsetInfo<RasterProductAssetVersion> *> &insets,
    unsigned int                          numInsets,
    std::vector< unsigned int>             &neededIndexes,
    unsigned int beginMinifyLevel,
    unsigned int endMinifyLevel);
template
void
FindNeededImageryInsets(
    const khInsetCoverage        &gencov,
    const std::vector<const InsetInfo<MercatorRasterProductAssetVersion> *>&
        insets,
    unsigned int                          numInsets,
    std::vector< unsigned int>             &neededIndexes,
    unsigned int beginMinifyLevel,
    unsigned int endMinifyLevel);


// ****************************************************************************
// ***  FindNeededTerrainInsets
// ***
// ***  Like above, but also accounts for the "extra" tiles needed (upper,
// ***  upper-right, right) in order to really have 33x33 instead of 32x32.
// ***  Assumption: Terrains are always in Flat projection.
// ****************************************************************************
template <typename ProductAssetVersion>
void FindNeededTerrainInsets(
    const khInsetCoverage        &gencov,
    const std::vector<const InsetInfo<ProductAssetVersion> *> &insets,
    unsigned int                          numInsets,
    std::vector< unsigned int>             &neededIndexes,
    unsigned int beginMinifyLevel,
    unsigned int endMinifyLevel) {
  // For now we know that gencov's levelExtents are simple minifications
  // of the maxres one. That means that we can do the intersection only at
  // the lowest res and know that we will get all the insets needed for
  // the other levels too. Later, NarrowCoverage may narrow the extents at
  // the various levels too. When that happens, we'll need to check for
  // intersections at each level to make sure we get them all.

  if (gencov.numLevels() == 0) {
    // catch degenerate case
    return;
  }

  unsigned int level = gencov.beginLevel();

  // Packgen will be caching the results - the cached results are
  // in product tilespace, so we need to do our intersection tests
  // snapped out to product tile boundaries
  // If gencov has more than one level, we're finding insets for a PacketGen
  // asset. Always do the align for PacketGen, so we make sure we have a
  // superset of the insets that each PacketLevelGen will need. PacketGen
  // doesn't really depend on the list returned from here. He just
  // pre-gathers them so the checks for PacketLevelGen don't have to compare
  // against ALL insets.
  bool needAlign = ((gencov.numLevels() > 1) ||
                    ((level > beginMinifyLevel)&&(level <= endMinifyLevel)));
  unsigned int alignSize = (RasterProductTilespaceBase.tileSize /
                    ClientTmeshTilespaceBase.tileSize);

  const khLevelCoverage levCov(gencov.levelCoverage(level));
  khExtents<std::uint32_t> genExtents(levCov.extents);
  khExtents<std::uint32_t> upperExtents
    (levCov.UpperCoverage(ClientTmeshTilespaceFlat).extents);
  khExtents<std::uint32_t> rightExtents
    (levCov.RightCoverage(ClientTmeshTilespaceFlat).extents);
  khExtents<std::uint32_t> upperRightExtents
    (levCov.UpperRightCoverage(ClientTmeshTilespaceFlat).extents);
  if (needAlign) {
    genExtents.alignBy(alignSize);
    upperExtents.alignBy(alignSize);
    rightExtents.alignBy(alignSize);
    upperRightExtents.alignBy(alignSize);
  }

  for (unsigned int i = 0; i < numInsets; ++i) {
    // Aligning here would be redundant, so we save ourselves the effort.
    const khExtents<std::uint32_t> &iExtents
      (insets[i]->coverage.levelExtents(level));

    if (iExtents.intersects(genExtents) ||
        iExtents.intersects(upperExtents) ||
        iExtents.intersects(rightExtents) ||
        iExtents.intersects(upperRightExtents)) {
      neededIndexes.push_back(i);
    }
  }
}

// Explicit instantiation
template
void
FindNeededTerrainInsets(
    const khInsetCoverage        &gencov,
    const std::vector<const InsetInfo<RasterProductAssetVersion> *> &insets,
    unsigned int                          numInsets,
    std::vector< unsigned int>             &neededIndexes,
    unsigned int beginMinifyLevel,
    unsigned int endMinifyLevel);
template
void
FindNeededTerrainInsets(
    const khInsetCoverage        &gencov,
    const std::vector<const InsetInfo<MercatorRasterProductAssetVersion> *>&
        insets,
    unsigned int                          numInsets,
    std::vector< unsigned int>             &neededIndexes,
    unsigned int beginMinifyLevel,
    unsigned int endMinifyLevel);

template class SimpleInsetInfo<RasterProductAssetVersion>;
template class SimpleInsetInfo<MercatorRasterProductAssetVersion>;
template class InsetInfo<RasterProductAssetVersion>;
template class InsetInfo<MercatorRasterProductAssetVersion>;

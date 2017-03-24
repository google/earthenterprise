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

/******************************************************************************
File:        InsetInfo.h
Description:

Changes:
Description: Support for terrain "overlay" projects.

******************************************************************************/
#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_SYSMAN_INSETINFO_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_SYSMAN_INSETINFO_H_

#include <string>
#include <functional>
#include "common/khExtents.h"
#include "common/khInsetCoverage.h"
#include "autoingest/plugins/CombinedRPAsset.h"


// TerrainInsetType enum defines types of terrain-inset in terrain
// project.
// In generic case (not "overlay" terrain project) a terrain-inset has type:
// "normal". In case of "overlay"-terrain project a terrain-inset may have
// type: "filling" or "overlay".
enum TerrainInsetType {
  kNormalTerrainInset  = 0,  // Regular terrain inset.
  kFillingTerrainInset = 1,  // "Filling" terrain inset.
  kOverlayTerrainInset = 2   // "Overlay" terrain inset.
};

template <typename ProductAssetVersion>
class SimpleInsetInfo {
 public:
  // information about the inset (from project config)
  ProductAssetVersion resource_;
  CombinedRPAssetVersion    combinedrp_;
  uint                      effectiveMaxLevel;
  std::string               date_string_;

  // cached information about the finished product - so we only query it once
  khExtents<double>         degExtents;
  uint                      fullresLevel;

  // This constructor used for just about everything
  // This must have the same signature as the constructor for InsetInfo
  // below. There are templates that can be working with either one of them.
  SimpleInsetInfo(const khTilespace& tile_space,
                  const ProductAssetVersion &resource,
                  const CombinedRPAssetVersion &combinedrp,
                  uint  effectiveMaxProdLev,
                  uint  unused2,
                  uint  unused3);

  // Comparison used for sorting the insets based on acquisition date then
  // max level, such that the order is oldest first followed by smallest
  // max level first.
  static bool Compare(const SimpleInsetInfo& info_x,
                      const SimpleInsetInfo& info_y) {
    if (info_x.date_string_ < info_y.date_string_)
      return true;
    if (info_x.effectiveMaxLevel < info_y.effectiveMaxLevel)
      return true;
    return false;
  }
};


template <typename ProductAssetVersion>
class InsetInfo : public SimpleInsetInfo<ProductAssetVersion> {
 public:
  // extra cached information about the finished product
  std::string               dataRPFile;
  std::string               alphaRPFile;

  // inset coverage for the inset (across all levels for the project)
  khInsetCoverage           coverage;

  InsetInfo(const khTilespace &tilespace,
            const ProductAssetVersion &resource,
            const CombinedRPAssetVersion &combinedrp,
            uint  effectiveMaxProdLev,
            uint  beginCovLev,
            uint  endCovLev);
};


class PacketGenInfo {
 public:
  std::string               assetname;

  // Type of terrain inset. it is used only for terrain assets.
  TerrainInsetType terrain_inset_type;

  // inset coverage of work to be done - gets successively narrowed
  khInsetCoverage           coverage;

  // cache of my sub asset versions - initially empty, filled after build
  // Indexed by level number! There will be leading "blanks".
  std::vector<AssetVersion> packetLevels;
  std::string               packetgenverref;

  uint beginSkipTransparentLevel;

  // Constructor for create PacketGenInfo with imagery inset.
  inline PacketGenInfo(const std::string &name,
                       const khInsetCoverage &cov) :
      assetname(name),
      terrain_inset_type(kNormalTerrainInset),
      coverage(cov),
      packetgenverref(),
      beginSkipTransparentLevel(100) {
  }

  // Constructor for create PacketGenInfo with terrain inset.
  inline PacketGenInfo(const std::string &name,
                       const TerrainInsetType _terrain_inset_type,
                       const khInsetCoverage &cov)
      : assetname(name),
        terrain_inset_type(_terrain_inset_type),
        coverage(cov),
        packetgenverref(),
        beginSkipTransparentLevel(100) {
  }

  void PopulatePacketLevels(AssetVersion packgenVer);

  bool NarrowCoverage(const PacketGenInfo *beginOthers,
                      const PacketGenInfo *endOthers);
};

// Predicate for searching the asset by name.
class AssetNameEqualToPred : public std::unary_function<PacketGenInfo, bool> {
 public:
  explicit AssetNameEqualToPred(const std::string &_val)
      : val_(_val) {
  }

  bool operator()(const PacketGenInfo &elem) const {
    return (elem.assetname == val_);
  }

 private:
  std::string val_;
};

template <class InsetInfo>
void CalcPacketGenInfo(const khTilespace            &tilespace,
                       const AssetDefs::Type         gentype,
                       const std::vector<InsetInfo> &insetInfos,
                       const uint32                  beginCoverageLevel,
                       std::vector<PacketGenInfo>   &genInfos,
                       const int32                   maxLevelDelta,
                       const bool   is_overlay_terrain_proj,
                       const uint32 overlay_terrain_resources_min_level);


template <typename ProductAssetVersion>
extern void
FindNeededImageryInsets(
    const khInsetCoverage        &gencov,
    const std::vector<const InsetInfo<ProductAssetVersion> *> &insets,
    uint                          numInsets,
    std::vector<uint>            &neededIndexes,
    uint beginMinifyLevel,
    uint endMinifyLevel);

template <typename ProductAssetVersion>
extern void
FindNeededTerrainInsets(
    const khInsetCoverage        &gencov,
    const std::vector<const InsetInfo<ProductAssetVersion> *> &insets,
    uint                          numInsets,
    std::vector<uint>            &neededIndexes,
    uint beginMinifyLevel,
    uint endMinifyLevel);


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_SYSMAN_INSETINFO_H_

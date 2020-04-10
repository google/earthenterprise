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
#include <memory>

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
  unsigned int                      effectiveMaxLevel;
  std::string               date_string_;

  // cached information about the finished product - so we only query it once
  khExtents<double>         degExtents;
  unsigned int                      fullresLevel;

  // This constructor used for just about everything
  // This must have the same signature as the constructor for InsetInfo
  // below. There are templates that can be working with either one of them.
  SimpleInsetInfo(const khTilespace& tile_space,
                  const ProductAssetVersion &resource,
                  const CombinedRPAssetVersion &combinedrp,
                  unsigned int  effectiveMaxProdLev,
                  unsigned int  unused2,
                  unsigned int  unused3);

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
            unsigned int  effectiveMaxProdLev,
            unsigned int  beginCovLev,
            unsigned int  endCovLev);
};


class PacketGenInfo {
 public:
  SharedString              assetname;

  // Type of terrain inset. it is used only for terrain assets.
  TerrainInsetType terrain_inset_type;

  // inset coverage of work to be done - gets successively narrowed
  khInsetCoverage           coverage;

  // cache of my sub asset versions - initially empty, filled after build
  // Indexed by level number! There will be leading "blanks".
  std::vector<AssetVersion> packetLevels;
  std::string               packetgenverref;

  unsigned int beginSkipTransparentLevel;

  // Constructor for create PacketGenInfo with imagery inset.
  inline PacketGenInfo(const SharedString &name,
                       const khInsetCoverage &cov) :
      assetname(name),
      terrain_inset_type(kNormalTerrainInset),
      coverage(cov),
      packetgenverref(),
      beginSkipTransparentLevel(100) {
  }

  // Constructor for create PacketGenInfo with terrain inset.
  inline PacketGenInfo(const SharedString &name,
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
                       const std::uint32_t                  beginCoverageLevel,
                       std::vector<PacketGenInfo>   &genInfos,
                       const std::int32_t                   maxLevelDelta,
                       const bool   is_overlay_terrain_proj,
                       const std::uint32_t overlay_terrain_resources_min_level);

// helper struct to make more generic calls from multiple locations possible
// can be modified to allow for easier usage in possible new algorithms
// for calculating overlap
template <typename ProductAssetVersion>
struct overlapEnvelope
{
    AssetDefs::Type type;
    std::vector<const InsetInfo<ProductAssetVersion> *> insets;
    unsigned int numInsets;
    unsigned int beginMinifyLevel;
    unsigned int endMinifyLevel;
    unsigned int level;
    overlapEnvelope() = default;
    overlapEnvelope(const AssetDefs::Type& _type,
                    const std::vector<const InsetInfo<ProductAssetVersion> *>& _insets,
                    unsigned int _numInsets,
                    unsigned int _beginMinifyLevel, unsigned int _endMinifyLevel, unsigned int _level)

   :                type(_type),
                    insets(_insets),
                    numInsets(_numInsets),
                    beginMinifyLevel(_beginMinifyLevel),
                    endMinifyLevel(_endMinifyLevel),
                    level(_level) {}
};

template <typename ProductAssetVersion>
class OverlapCalculator
{
private:
    void CalculateOverlap(std::vector< unsigned int> &, const khInsetCoverage&);
    overlapEnvelope<ProductAssetVersion> env;

public:
    OverlapCalculator(const overlapEnvelope<ProductAssetVersion>& _env) :
        env(_env) {}

    void setBeginMinifyLevel(unsigned int _begin) { env.beginMinifyLevel = _begin; }
    void setEndMinifyLevel(unsigned int _end) { env.endMinifyLevel = _end; }
    void setEnvelope(const overlapEnvelope<ProductAssetVersion>& _env) { env = _env; }
    std::vector< unsigned int>  PreprocessForInset(const khInsetCoverage& gencov)
    {
        std::vector< unsigned int>  neededIndexes;
        CalculateOverlap(neededIndexes, gencov);
        return neededIndexes;
    }

    std::vector< unsigned int>  GetOverlapForLevel(const khInsetCoverage& gencov)
    {
        std::vector< unsigned int>  neededIndexes;
        CalculateOverlap(neededIndexes, gencov);
        return neededIndexes;
    }
};

template <typename ProductAssetVersion>
extern void
FindNeededImageryInsets(
    const khInsetCoverage        &gencov,
    const std::vector<const InsetInfo<ProductAssetVersion> *> &insets,
    unsigned int                          numInsets,
    std::vector< unsigned int>             &neededIndexes,
    unsigned int beginMinifyLevel,
    unsigned int endMinifyLevel);

template <typename ProductAssetVersion>
extern void
FindNeededTerrainInsets(
    const khInsetCoverage        &gencov,
    const std::vector<const InsetInfo<ProductAssetVersion> *> &insets,
    unsigned int                          numInsets,
    std::vector< unsigned int>             &neededIndexes,
    unsigned int beginMinifyLevel,
    unsigned int endMinifyLevel);


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_SYSMAN_INSETINFO_H_

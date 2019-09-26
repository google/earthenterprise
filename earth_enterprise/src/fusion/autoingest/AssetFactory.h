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

#ifndef ASSET_FACTORY_H
#define ASSET_FACTORY_H

#include <common/SharedString.h>
#include <common/khMetaData.h>

// The goal of this namspace is to have a single place in the code where all
// asset creation is handled.
namespace AssetFactory
{
  template<class AssetType>
  AssetType Find(const std::string & ref, const AssetDefs::Type & type)
  {
    assert(type != AssetDefs::Invalid);
    const std::string subtype = AssetType::Impl::EXPECTED_SUBTYPE;
    try {
      AssetType asset(ref);
      if (asset &&
          (asset->type == type) &&
          (asset->subtype == subtype)) {
          return asset;
      }
    } catch (...) {
        // do nothing - don't even generate any warnings
    }
    return AssetType();
  }

  template<class AssetType>
  AssetType Find(const std::string & ref) {
    return Find<AssetType>(ref, AssetType::Impl::EXPECTED_TYPE);
  }


  // see where/how called
  template<class VersionType>
  void ValidateRefForInput(const std::string & ref, const AssetDefs::Type & type)
  {
    assert(type != AssetDefs::Invalid);
    using AssetType = typename VersionType::Impl::AssetType;
    const std::string subtype = VersionType::Impl::EXPECTED_SUBTYPE;
    if (AssetVersionRef(ref).Version() == "current") {
      AssetType asset = Find<AssetType>(ref, type);
      if (!asset) {
        throw std::invalid_argument(
            "No such " + ToString(type) + " " + subtype + " asset: " + ref);
      }
    } else {
      VersionType version = Find<VersionType>(ref, type);
      if (!version) {
        throw std::invalid_argument(
            "No such " + ToString(type) + " " + subtype + " asset version: " +
            ref);
      }
    }
  }  
  
  template<class VersionType>
  void ValidateRefForInput(const std::string & ref) {
    return ValidateRefForInput<VersionType>(ref, VersionType::Impl::EXPECTED_TYPE);
  }
 
  template<class MutableDerivedAssetHandleType, class ConfigType>
  MutableDerivedAssetHandleType Make( const std::string &ref_,
                                      AssetDefs::Type type_,
                                      const khMetaData &meta,
                                      const ConfigType &config)
  {
    using Impl = typename MutableDerivedAssetHandleType::Impl;
    using AssetStorageType = typename Impl::Base;
    return MutableDerivedAssetHandleType(std::make_shared<Impl>
                                            (AssetStorageType::MakeStorage(
                                        ref_, type_, 
                                        Impl::EXPECTED_SUBTYPE,
                                        std::vector<SharedString>(), meta),
                                        config));
  }

  template<class MutableDerivedAssetHandleType, class ConfigType>
  MutableDerivedAssetHandleType Make( const std::string &ref_,
                                      AssetDefs::Type type_,
                                      const std::vector<SharedString>& inputs_,
                                      const khMetaData &meta,
                                      const ConfigType &config)
  {
    using Impl = typename MutableDerivedAssetHandleType::Impl;
    using AssetStorageType = typename Impl::Base;
    return MutableDerivedAssetHandleType(std::make_shared<Impl>
                                            (AssetStorageType::MakeStorage(
                                        ref_, type_,
                                        Impl::EXPECTED_SUBTYPE,
                                        inputs_, meta),
                                        config));
  }

  template<class MutableDerivedAssetHandleType, class ConfigType>
  MutableDerivedAssetHandleType Make( const std::string &ref_,
                                      const std::vector<SharedString>& inputs_,
                                      const khMetaData &meta,
                                      const ConfigType &config)
  {
    using Impl = typename MutableDerivedAssetHandleType::Impl;
    using AssetStorageType = typename Impl::Base;
    return MutableDerivedAssetHandleType(std::make_shared<Impl>
                                            (AssetStorageType::MakeStorage(
                                        ref_, Impl::EXPECTED_TYPE,
                                        Impl::EXPECTED_SUBTYPE,
                                        inputs_, meta),
                                        config));
  }

  template<class MutableDerivedAssetHandleType, class ConfigType>
  MutableDerivedAssetHandleType FindMake(const std::string& ref_,
                                          AssetDefs::Type type_,
                                          const khMetaData& meta_,
                                          const ConfigType& config_)
  {
      MutableDerivedAssetHandleType asset = Find<MutableDerivedAssetHandleType>(ref_, type_);
      if (asset)
      {
          asset->Modify(meta_, config_);
          return asset;
      }
      return Make<MutableDerivedAssetHandleType, ConfigType>(ref_, type_, meta_, config_);
  }

  template<class MutableDerivedAssetHandleType, class ConfigType>
  MutableDerivedAssetHandleType FindMake(const std::string& ref_,
                                          AssetDefs::Type type_,
                                          const std::vector<SharedString>& inputs_,
                                          const khMetaData& meta_,
                                          const ConfigType& config_)
  {
      MutableDerivedAssetHandleType asset = Find<MutableDerivedAssetHandleType>(ref_, type_);
      if (asset)
      {
          asset->Modify(inputs_, meta_, config_);
          return asset;
      }
      return Make<MutableDerivedAssetHandleType, ConfigType>(ref_, type_, inputs_, meta_, config_);
  }

  template <class MutableDerivedAssetHandleType, class ConfigType>
  MutableDerivedAssetHandleType FindMake(const std::string& ref_,
                                          const std::vector<SharedString>& inputs_,
                                          const khMetaData& meta_,
                                          const ConfigType& config_)
  {
     MutableDerivedAssetHandleType asset = Find<MutableDerivedAssetHandleType>(ref_);
     if (asset)
     {
         asset->Modify(inputs_, meta_, config_);
         return asset;
     }
     return Make<MutableDerivedAssetHandleType, ConfigType> (ref_, inputs_, meta_, config_);
  }

  template <class MutableDerivedVersionHandleType, class Version, class ConfigType>
  MutableDerivedVersionHandleType FindMakeAndUpdate(const std::string& ref_,
                                                  const AssetDefs::Type type_,
                                                  const std::vector<SharedString>& inputs_,
                                                  const khMetaData& meta_,
                                                  const ConfigType& config_,
                                                  const std::vector<Version>& cachedinputs_)
  {
      using AssetHandleType = typename MutableDerivedVersionHandleType::Impl::MutableAssetType;
      auto asset = FindMake<AssetHandleType>
              (ref_, type_, inputs_, meta_, config_);
      bool needed = false;
      return asset->MyUpdate(needed, cachedinputs_);
  }

  template <class MutableDerivedVersionHandleType, class Version, class ConfigType>
  MutableDerivedVersionHandleType FindMakeAndUpdate(const std::string& ref_,
                                                  const std::vector<SharedString>& inputs_,
                                                  const khMetaData& meta_,
                                                  const ConfigType& config_,
                                                  const std::vector<Version>& cachedinputs_ )
  {
      using AssetHandleType = typename MutableDerivedVersionHandleType::Impl::MutableAssetType;
      auto asset = FindMake<AssetHandleType>
              (ref_, inputs_, meta_, config_);
      bool needed = false;
      return asset->MyUpdate(needed, cachedinputs_);
  }

  template <class MutableDerivedVersionHandleType, class Version, class ConfigType>
  MutableDerivedVersionHandleType FindMakeAndUpdate(const std::string& ref_,
                                                  AssetDefs::Type type_,
                                                  const khMetaData& meta_,
                                                  const ConfigType& config_)
  {
      using AssetHandleType = typename MutableDerivedVersionHandleType::Impl::MutableAssetType;
      auto asset = FindMake<AssetHandleType>
              (ref_, type_, meta_, config_);
      bool needed = false;
      return asset->MyUpdate(needed);
  }

  // function defn1
  // e.g. NATIVE/DatabaseAssetD.cpp/fusion/autoingest/sysman/plugins
  //    - PacketLevelAssetD.cpp
  //    - CombinedRPAssetD.cpp          XXX
  //    - RasterDBRootAssetD.cpp        XXX
  //    - RasterGEIndexAssetD.cpp
  //    - PacketGenAssetD.cpp
  //    - RasterProductAssetD.cpp
  //    - KRMPAssetD.cpp
  //    - KRPAssetD.cpp
  //    - MaskgenAssetD.cpp
  //    - MosaicAssetD.cpp
  //    - SourceAssetD.cpp
  //    - RasterKHDBAssetD.cpp
  //    - BlendAssetD.cpp
  //    - ReprojectAssetD.cpp
  //
  //    Call Locations:
  //        - MercatorRastorProject.src
  //            => RasterDBRoot
  //        - MercatorRasterProduct.src
  //            => CombinedRPAsset
  //        - RasterProduct.src
  //            => CombinedRPAsset
  //
  // SEVEM PARAMS
  //
  // assetRef = GetAssetRef() = parentRef?
  // type
  // k* [string]
  // inputs
  // khMetaData()
  // Config()
  // vector<AssetVersion>
  //
  template <class MutableDerivedVersionHandleType, class Version, class ConfigType>
  MutableDerivedVersionHandleType FindMakeAndUpdateSubAsset(const std::string& parentAssetRef,
                                                            AssetDefs::Type type_,
                                                            const std::string& basename,
                                                            const std::vector<SharedString>& inputs_,
                                                            const khMetaData &meta_,
                                                            const ConfigType& config_,
                                                            const std::vector<Version>& cachedinputs_)
  {
      using Impl = typename MutableDerivedVersionHandleType::Impl;
      auto ref_ = AssetDefs::SubAssetName(parentAssetRef,
                                          basename,
                                          type_,
                                          Impl::EXPECTED_SUBTYPE);
      return FindMakeAndUpdate<MutableDerivedVersionHandleType, Version, ConfigType>
              (ref_,
               type_,
               inputs_,
               meta_,
               config_,
               cachedinputs_);
  }

  // function defn2
  // e.g. NATIVE/DatabaseAssetD.cpp/fusion/autoingest/sysman/plugins:
  //    - DatabaseAssetD.cpp
  //    - TmeshAssetD.cpp
  //    - copyrightAssetD.cpp
  //    - ImeshKHDBAssetD.cpp
  //    - ProjectedMapLayerAssetD.cpp       XXX     ???
  //    - VectorKHDBAssetD.cpp
  //    - MapGEIndexAssetD.cpp
  //    - MapPOIAssetD.cpp                  XXX
  //    - VectorPOIAssetD                   XXX
  //    - MapLayerJSAssetD.cpp
  //    - MercatorMapDatabaseAssetD.cpp
  //    - GEDBAssetD.cpp
  //    - UnifiedIndexAssetD.cpp
  //    - CombinedTerrainAssetD.cpp
  //    - VectorGEIndexAssetD.cpp
  //    - KMLProjectAssetD.cpp
  //    - MapDatabaseAssetD.cpp
  //    - MapLayerLevel.cpp                 XXX     ???
  //    - MapDBAssetD.cpp
  //    - MapLayerAssetVersionD.cpp
  //    - ProjectedMapProject.cpp
  //    - MapProjectAssetD.cpp
  //    - DatabaseAssetD.cpp
  //    - QTPacketAssetD.cpp
  //    - VectorDBRootAssetD.cpp
  //    - VectorFuseAssetD.cpp              XXX
  //    - VectorQueryAssetD.cpp             XXX
  //    - VectorLayerAssetD.cpp             XXX
  //    - VectorProjectAssetD.cpp
  //    - VectorLayerXAssetD.cpp            XXX     ???
  //    - VectorProductAssetD.cpp
  //    - MercatorRasterProductAssetD.cpp
  //
  //    Call locations:
  //      - ProjectedMapLayer.src
  //        => ProjectedMapLayer
  //        => MapLayerLevel
  //        => VectorLayerXAssetD
  //      - VectorLayer.src
  //        => VectorQueryAsset
  //        => VectorFuseAsset
  //        => VectorPOI
  //      - MapLayer.src
  //        => MapPOIAssetAsset
  //        => ??? line 161 ??? Also VectorQueryAsset
  //      - VectorProject.src
  //        => VectorLayer
  //        =>
  //  SIX params
  template <class MutableDerivedVersionHandleType, class Version, class ConfigType>
  MutableDerivedVersionHandleType FindMakeAndUpdateSubAsset(const std::string& parentAssetRef,
                                                            const std::string& basename,
                                                            const std::vector<Version>& inputs_,
                                                            const khMetaData& meta_,
                                                            const ConfigType& config,
                                                            const std::vector<Version>& cachedinputs_)
  {
      using Impl = typename MutableDerivedVersionHandleType::Impl;
      auto ref_ = AssetDefs::SubAssetName(parentAssetRef,
                                          basename,
                                          Impl::EXPECTED_TYPE,
                                          Impl::EXPECTED_SUBTYPE);

      return FindMakeAndUpdate<MutableDerivedVersionHandleType, Version, ConfigType>
              (ref_,
               inputs_,
               meta_,
               config,
               cachedinputs_);
  }


  template <class MutableDerivedAssetHandleType, class ConfigType>
  MutableDerivedAssetHandleType FindAndModify(const std::string& ref_,
                                              const std::vector<SharedString>& inputs_,
                                              const khMetaData& meta_,
                                              const ConfigType& config_)
  {
      using Impl = typename MutableDerivedAssetHandleType::Impl;
      MutableDerivedAssetHandleType asset = Find<MutableDerivedAssetHandleType>(ref_);
      if (asset)
      {
          asset->Modify(inputs_, meta_, config_);
          return asset;
      }
      throw khException(kh::tr("%1 '%2' does not exist")
                        .arg(Impl::EXPECTED_SUBTYPE).arg(ref_));
  }

  template <class MutableDerivedAssetHandleType, class ConfigType>
  MutableDerivedAssetHandleType FindAndModify(const std::string& ref_,
                                              AssetDefs::Type type_,
                                              const std::vector<SharedString>& inputs_,
                                              const khMetaData& meta_,
                                              const ConfigType& config_)
  {
      using Impl = typename MutableDerivedAssetHandleType::Impl;
      MutableDerivedAssetHandleType asset = Find<MutableDerivedAssetHandleType>(ref_, type_);
      if (asset)
      {
          asset->Modify(inputs_, meta_, config_);
          return asset;
      }
      throw khException(kh::tr("%1 '%2' does not exist")
                        .arg(Impl::EXPECTED_SUBTYPE).arg(ref_));
  }

  template <class MutableDerivedAssetHandleType, class ConfigType>
  MutableDerivedAssetHandleType FindAndModify(const std::string& ref_,
                                              AssetDefs::Type type_,
                                              const khMetaData& meta_,
                                              const ConfigType& config_)
  {
      using Impl = typename MutableDerivedAssetHandleType::Impl;
      MutableDerivedAssetHandleType asset = Find<MutableDerivedAssetHandleType>(ref_, type_);
      if (asset)
      {
          asset->Modify(meta_, config_);
          return asset;
      }
      throw khException(kh::tr("%1 '%2' does not exist")
                        .arg(Impl::EXPECTED_SUBTYPE).arg(ref_));
  }

  template<class MutableDerivedAssetHandleType, class ConfigType>
  MutableDerivedAssetHandleType MakeNew( const std::string &ref_,
                const std::vector<SharedString>& inputs_,
                const khMetaData &meta,
                const ConfigType &config)
  {
    using Impl = typename MutableDerivedAssetHandleType::Impl;
    MutableDerivedAssetHandleType asset = Find<MutableDerivedAssetHandleType>(ref_);
    if (asset) {
        throw khException(kh::tr("%1 '%2' already exists")
                          .arg(Impl::EXPECTED_SUBTYPE).arg(ref_));
    } else {
        return Make<MutableDerivedAssetHandleType, ConfigType>(ref_, inputs_, meta, config);
    }
  }
}
#endif

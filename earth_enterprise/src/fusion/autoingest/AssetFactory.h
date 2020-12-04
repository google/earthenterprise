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
#include "AssetVersionRef.h"
#include "AssetRegistry.h"
#include "autoingest/.idl/AssetStorage.h"

// The goal of this namspace is to have a single place in the code where all
// asset creation is handled.

// Except for CreateNewFromDOM and CreateNewInvalid, you should pass
// <Name>Type as the template parameter (e.g, PacketGenType). That type
// references all the necessary related types that the templates below may
// need to use.
namespace AssetFactory
{
  template<class AssetType>
  static std::shared_ptr<AssetType> CreateNewFromDOM(
    const std::string & tagName, 
    void *e)
  {
      typename AssetRegistry<AssetType>::AssetPluginInterface *plugin = AssetRegistry<AssetType>::GetPlugin(tagName);
      if (plugin && plugin->pNewFromDom)
        return (plugin->pNewFromDom)(e);
      return nullptr;
  }

  template<class AssetType>
  static std::shared_ptr<AssetType> CreateNewInvalid(
    std::string tagName, 
    const std::string & ref)
  {
      typename AssetRegistry<AssetType>::AssetPluginInterface *plugin = AssetRegistry<AssetType>::GetPlugin(tagName);
      if (plugin && plugin->pNewInvalid) {
        return (plugin->pNewInvalid)(ref);
      }
      return nullptr;
  }

  template<class FindType>
  static FindType Find(const std::string & ref, const AssetDefs::Type & type, const std::string & subtype)
  {
    assert(type != AssetDefs::Invalid);
    try {
      typename FindType::BBase asset(ref);
      if (asset &&
          (asset->type == type) &&
          (asset->subtype == subtype)) {
          return FindType(ref);
      }
    } catch (...) {
        // do nothing - don't even generate any warnings
    }
    return FindType();
  }

  template<class AssetType>
  inline typename AssetType::AssetD Find(const std::string & ref, const AssetDefs::Type & type) {
    return Find<typename AssetType::AssetD>(ref, type, AssetType::SUBTYPE);
  }

  template<class AssetType>
  inline typename AssetType::AssetD Find(const std::string & ref) {
    return Find<typename AssetType::AssetD>(ref, AssetType::TYPE, AssetType::SUBTYPE);
  }

  template<class AssetType>
  inline typename AssetType::VersionD FindVersion(const std::string & ref, const AssetDefs::Type & type) {
    return Find<typename AssetType::VersionD>(ref, type, AssetType::SUBTYPE);
  }

  template<class AssetType>
  void ValidateRefForInput(const std::string & ref, const AssetDefs::Type & type)
  {
    assert(type != AssetDefs::Invalid);
    if (AssetVersionRef(ref).Version() == "current") {
      typename AssetType::Asset asset = Find<AssetType>(ref, type);
      if (!asset) {
        throw std::invalid_argument(
            "No such " + ToString(type) + " " + AssetType::SUBTYPE + " asset: " + ref);
      }
    } else {
      typename AssetType::VersionD version = FindVersion<AssetType>(ref, type);
      if (!version) {
        throw std::invalid_argument(
            "No such " + ToString(type) + " " + AssetType::SUBTYPE + " asset version: " +
            ref);
      }
    }
  }

  template<class AssetType>
  inline void ValidateRefForInput(const std::string & ref) {
    return ValidateRefForInput<AssetType>(ref, AssetType::TYPE);
  }

  template<class AssetType>
  typename AssetType::MutableAssetD Make(const std::string &ref_,
                                      AssetDefs::Type type_,
                                      const khMetaData &meta,
                                      const typename AssetType::Config &config)
  {
    return typename AssetType::MutableAssetD(
        std::make_shared<typename AssetType::AssetImplD>
            (AssetStorage::MakeStorage(
                ref_, type_, 
                AssetType::SUBTYPE,
                std::vector<SharedString>(), meta),
                config));
  }

  template<class AssetType>
  typename AssetType::MutableAssetD Make(const std::string &ref_,
                                      AssetDefs::Type type_,
                                      const std::vector<SharedString>& inputs_,
                                      const khMetaData &meta,
                                      const typename AssetType::Config &config)
  {
    return typename AssetType::MutableAssetD(
        std::make_shared<typename AssetType::AssetImplD>
            (AssetStorage::MakeStorage(
                ref_, type_,
                AssetType::SUBTYPE,
                inputs_, meta),
                config));
  }

  template<class AssetType>
  typename AssetType::MutableAssetD Make(const std::string &ref_,
                                      const std::vector<SharedString>& inputs_,
                                      const khMetaData &meta,
                                      const typename AssetType::Config &config)
  {
    return typename AssetType::MutableAssetD(
        std::make_shared<typename AssetType::AssetImplD>
            (AssetStorage::MakeStorage(
                ref_, AssetType::TYPE,
                AssetType::SUBTYPE,
                inputs_, meta),
                config));
  }

  template<class AssetType>
  typename AssetType::MutableAssetD FindMake(const std::string& ref_,
                                          AssetDefs::Type type_,
                                          const std::vector<SharedString>& inputs_,
                                          const khMetaData& meta_,
                                          const typename AssetType::Config& config_)
  {
      typename AssetType::MutableAssetD asset = Find<AssetType>(ref_, type_);
      if (asset)
      {
          asset->Modify(inputs_, meta_, config_);
          return asset;
      }
      return Make<AssetType>(ref_, type_, inputs_, meta_, config_);
  }

  template<class AssetType>
  typename AssetType::MutableAssetD FindMake(const std::string& ref_,
                                          const std::vector<SharedString>& inputs_,
                                          const khMetaData& meta_,
                                          const typename AssetType::Config& config_)
  {
     typename AssetType::MutableAssetD asset = Find<AssetType>(ref_);
     if (asset)
     {
         asset->Modify(inputs_, meta_, config_);
         return asset;
     }
     return Make<AssetType> (ref_, inputs_, meta_, config_);
  }

  template <class AssetType>
  typename AssetType::MutableVersionD FindMakeAndUpdate(const std::string& ref_,
                                                  const AssetDefs::Type type_,
                                                  const std::vector<SharedString>& inputs_,
                                                  const khMetaData& meta_,
                                                  const typename AssetType::Config& config_,
                                                  const std::vector<typename AssetType::Version::Base>& cachedinputs_)
  {
      auto asset = FindMake<AssetType>
              (ref_, type_, inputs_, meta_, config_);
      bool needed = false;
      return asset->MyUpdate(needed, cachedinputs_);
  }

  template <class AssetType>
  typename AssetType::MutableVersionD FindMakeAndUpdate(const std::string& ref_,
                                                  const std::vector<SharedString>& inputs_,
                                                  const khMetaData& meta_,
                                                  const typename AssetType::Config& config_,
                                                  const std::vector<typename AssetType::Version::Base>& cachedinputs_ )
  {
      auto asset = FindMake<AssetType>
              (ref_, inputs_, meta_, config_);
      bool needed = false;
      return asset->MyUpdate(needed, cachedinputs_);
  }

  template <class AssetType>
  typename AssetType::MutableVersionD FindMakeAndUpdateSubAsset(const std::string& parentAssetRef,
                                                            AssetDefs::Type type_,
                                                            const std::string& basename,
                                                            const std::vector<SharedString>& inputs_,
                                                            const khMetaData &meta_,
                                                            const typename AssetType::Config& config_,
                                                            const std::vector<typename AssetType::Version::Base>& cachedinputs_)
  {
      auto ref_ = AssetDefs::SubAssetName(parentAssetRef,
                                          basename,
                                          type_,
                                          AssetType::SUBTYPE);
      return FindMakeAndUpdate<AssetType>
              (ref_,
               type_,
               inputs_,
               meta_,
               config_,
               cachedinputs_);
  }

  template <class AssetType>
  typename AssetType::MutableVersionD FindMakeAndUpdateSubAsset(const std::string& parentAssetRef,
                                                            const std::string& basename,
                                                            const std::vector<SharedString>& inputs_,
                                                            const khMetaData& meta_,
                                                            const typename AssetType::Config& config,
                                                            const std::vector<typename AssetType::Version::Base>& cachedinputs_)
  {
      auto ref_ = AssetDefs::SubAssetName(parentAssetRef, basename,
                                          AssetType::TYPE, AssetType::SUBTYPE);
      return FindMakeAndUpdate<AssetType>
             (ref_,
              inputs_,
              meta_,
              config,
              cachedinputs_);
  }

  template <class AssetType>
  typename AssetType::MutableVersionD ReuseOrMakeAndUpdate(
      const std::string &ref_,
      AssetDefs::Type type_,
      const khMetaData &meta_,
      const typename AssetType::Config& config_)
  {
    typename AssetType::MutableAssetD asset = Find<AssetType>(ref_, type_);
    if (asset) {
        for (const auto& v : asset->versions) {
            try {
              typename AssetType::VersionD version(v);
              version.LoadAsTemporary();
              if ((version->state != AssetDefs::Offline) &&
                  (version->inputs.empty()) &&
                  config_.IsUpToDate(version->config)) {
                version.MakePermanent();
                return version;
              }
           }
           catch (...) {
             notify(NFY_WARN, "ReuseOrMakeAndUpdate could not reuse a version." );
           }
        }
        asset->Modify( meta_, config_);
    } else {
        asset = Make<AssetType>(ref_, type_, meta_, config_);
    }
    bool needed = false;
    return asset->MyUpdate(needed);
  }

  template <class AssetType, class CachedInputType>
  typename AssetType::MutableVersionD ReuseOrMakeAndUpdate(
        const std::string &ref_,
        AssetDefs::Type type_,
        const std::vector<SharedString>& inputs_,
        const khMetaData &meta_,
        const typename AssetType::Config& config_,
        const std::vector<CachedInputType>& cachedinputs_)
  {
    // make a copy since actualinputarg is macro substituted, so begin() &
    // end() could be called on different temporary objects
    std::vector<SharedString> inputarg = inputs_;
    // bind my input versions refs
    std::vector<SharedString> boundInputs;
    boundInputs.reserve(inputarg.size());
    std::transform(inputarg.begin(), inputarg.end(), back_inserter(boundInputs),
                   ptr_fun(&AssetVersionRef::Bind));
    typename AssetType::MutableAssetD asset = Find<AssetType>(ref_, type_);
    if (asset) {
        for (const auto& v : asset->versions) {
            try {
              typename AssetType::VersionD version(v);
              version.LoadAsTemporary();
              if ((version->state != AssetDefs::Offline) &&
                  (version->inputs == boundInputs) &&
                  config_.IsUpToDate(version->config)) {
                version.MakePermanent();
                return version;
              }
           }
           catch (...) {
             notify(NFY_WARN, "ReuseOrMakeAndUpdate could not reuse a version." );
           }
        }
        asset->Modify(inputs_, meta_, config_);
    } else {
        asset = Make<AssetType>(ref_, type_, inputs_, meta_, config_);
    }
    bool needed = false;
    return asset->MyUpdate(needed, cachedinputs_);
  }

  template <class AssetType, class Extras>
  typename AssetType::MutableVersionD ReuseOrMakeAndUpdate(
        const std::string &ref_,
        AssetDefs::Type type_,
        const khMetaData &meta_,
        const typename AssetType::Config& config_,
        const Extras &extra)
  {
    typename AssetType::MutableAssetD asset = Find<AssetType>(ref_, type_);
    if (asset) {
        for (const auto& v : asset->versions) {
            try {
              typename AssetType::VersionD version(v);
              version.LoadAsTemporary();
              if ((version->state != AssetDefs::Offline) &&
                  (version->inputs.empty()) &&
                  config_.IsUpToDate(version->config)) {
                version.MakePermanent();
                return version;
              }
           }
           catch (...) {
             notify(NFY_WARN, "ReuseOrMakeAndUpdate could not reuse a version." );
           }
        }
        asset->Modify(meta_, config_);
    } else {
        asset = Make<AssetType>(ref_, type_, meta_, config_);
    }
    bool needed = false;
    return asset->MyUpdate(needed, extra);
  }

  template <class AssetType>
  typename AssetType::MutableVersionD ReuseOrMakeAndUpdateSubAsset(
        const std::string &parentAssetRef,
        AssetDefs::Type type_,
        const std::string &basename,
        const khMetaData &meta_,
        const typename AssetType::Config& config_)
  {
    auto ref_ =  AssetDefs::SubAssetName(
        parentAssetRef, basename, type_, AssetType::SUBTYPE);
    return ReuseOrMakeAndUpdate<AssetType>(ref_, type_, meta_, config_);
  }

  template <class AssetType, class CachedInputType>
  typename AssetType::MutableVersionD ReuseOrMakeAndUpdateSubAsset(
        const std::string &parentAssetRef,
        AssetDefs::Type type_,
        const std::string &basename,
        const std::vector<SharedString>& inputs_,
        const khMetaData &meta_,
        const typename AssetType::Config& config_,
        const std::vector<CachedInputType>& cachedinputs_)
  {
    auto ref_ = AssetDefs::SubAssetName(
        parentAssetRef, basename, type_, AssetType::SUBTYPE);
    return ReuseOrMakeAndUpdate<AssetType>(
        ref_, type_, inputs_, meta_, config_, cachedinputs_);
  }

  template <class AssetType>
  typename AssetType::MutableAssetD FindAndModify(const std::string& ref_,
                                              const std::vector<SharedString>& inputs_,
                                              const khMetaData& meta_,
                                              const typename AssetType::Config& config_)
  {
      typename AssetType::MutableAssetD asset = Find<AssetType>(ref_);
      if (asset)
      {
          asset->Modify(inputs_, meta_, config_);
          return asset;
      }
      throw khException(kh::tr("%1 '%2' does not exist")
                        .arg(AssetType::SUBTYPE.c_str()).arg(ref_.c_str()));
  }

  template<class AssetType>
  typename AssetType::MutableAssetD MakeNew( const std::string &ref_,
                const std::vector<SharedString>& inputs_,
                const khMetaData &meta,
                const typename AssetType::Config &config)
  {
    typename AssetType::MutableAssetD asset = Find<AssetType>(ref_);
    if (asset) {
        throw khException(kh::tr("%1 '%2' already exists")
                          .arg(AssetType::SUBTYPE.c_str()).arg(ref_.c_str()));
    } else {
        return Make<AssetType>
                   (ref_, inputs_, meta, config);
    }
  }
}

#endif

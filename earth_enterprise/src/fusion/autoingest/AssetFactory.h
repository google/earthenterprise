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
  template<class MutableDerivedAssetHandleType, class ConfigType>
  MutableDerivedAssetHandleType Make( const std::string &ref_,
                                      AssetDefs::Type type_,
                                      const khMetaData &meta,
                                      const ConfigType &config)
  {
    using Impl = typename MutableDerivedAssetHandleType::Impl;
    return MutableDerivedAssetHandleType(std::make_shared<Impl>
                                            (AssetStorage::MakeStorage(
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
    return MutableDerivedAssetHandleType(std::make_shared<Impl>
                                            (AssetStorage::MakeStorage(
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
    return MutableDerivedAssetHandleType(std::make_shared<Impl>
                                            (AssetStorage::MakeStorage(
                                        ref_, Impl::EXPECTED_TYPE, 
                                        Impl::EXPECTED_SUBTYPE,
                                        inputs_, meta),
                                        config));
  }

  template<class MutableDerivedAssetHandleType, class ConfigType>
  void MakeNew( const std::string &ref_,
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
        Make<MutableDerivedAssetHandleType, ConfigType>(ref_, inputs_, meta, config);
    }
  }
}
#endif
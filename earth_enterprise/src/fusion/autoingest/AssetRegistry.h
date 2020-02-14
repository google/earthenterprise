/*
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

#ifndef ASSET_REGISTRY_H
#define ASSET_REGISTRY_H

#include <functional>
#include <memory>
#include <unordered_map>

template <class AssetType>
class AssetRegistry{
public:
  AssetRegistry() = delete;

  class AssetPluginInterface {
    /*
    * Collection of plugin-specific versions of functions.
    */
    public:
      using new_from_dom_func = std::function<std::shared_ptr<AssetType>(void*)>;
      using new_invalid_func = std::function<std::shared_ptr<AssetType>(const std::string &ref)>;
      new_from_dom_func pNewFromDom;
      new_invalid_func pNewInvalid;

      explicit AssetPluginInterface(new_from_dom_func pNFD, new_invalid_func pNI):
        pNewFromDom(pNFD), pNewInvalid(pNI) {}
  };


  class PluginRegistrar
  {
    /*
    * PluginRegistrar objects are instantiated within a plugin's cpp file to
    * register the plugin.
    */
    public:
      explicit PluginRegistrar(
        std::string const & name, 
        std::unique_ptr<AssetPluginInterface> assetPlugin)
      {
          AssetRegistry<AssetType>::RegisterPlugin(name, std::move(assetPlugin));
      }
  };

  static void RegisterPlugin(
    std::string const & assetTypeName, 
    std::unique_ptr<AssetPluginInterface> assetPlugin)
  {
    PluginRegistry()[assetTypeName] = std::move(assetPlugin);
  }

  static AssetPluginInterface * GetPlugin(const std::string & assetTypeName) {
    auto it = PluginRegistry().find(assetTypeName);
    if (it != PluginRegistry().end())
      return it->second.get();
    return nullptr;
  }

  using PluginMap = std::unordered_map<std::string, std::unique_ptr<AssetRegistry<AssetType>::AssetPluginInterface>>;

protected:
  static PluginMap & PluginRegistry();
};

template <class AssetType>
typename AssetRegistry<AssetType>::PluginMap & AssetRegistry<AssetType>::PluginRegistry()
{
  static PluginMap reg;
  return reg;
}


#endif //ASSET_REGISTRY_H

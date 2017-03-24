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

#ifndef __AssetD_h
#define __AssetD_h

#include <Asset.h>
#include "AssetVersionD.h"
#include "AssetHandleD.h"

// ****************************************************************************
// ***  AssetImplD
// ****************************************************************************
class AssetImplD : public virtual AssetImpl
{
  friend class DerivedAssetHandle_<Asset, AssetImplD>;
  friend class MutableAssetHandleD_<DerivedAssetHandle_<Asset, AssetImplD> >;

  // private and unimplemented -- illegal to copy an AssetImplD
  AssetImplD(const AssetImplD&);
  AssetImplD& operator=(const AssetImplD&);
 protected:
  static khRefGuard<AssetImplD> Load(const std::string &boundref);
  virtual bool Save(const std::string &filename) const = 0;

  AssetImplD(void) : AssetImpl() { }
  AssetImplD(const AssetStorage &storage)
      : AssetImpl(storage) { }

  void AddVersionRef(const std::string &verref);

  bool InputsUpToDate(const AssetVersion &,
                      const std::vector<AssetVersion> &cachedInputs) const;
  void UpdateInputs(std::vector<AssetVersion> &inputvers) const;

 public:

  // const so can be called w/o a MutableHandle (it could already be
  // uptodate).  needed is set to true iff everything was not up to date
  // and so something happened as a result of the Update call.
  virtual AssetVersionD Update(bool &needed) const = 0;
  void Modify(const std::vector<std::string>& inputs_,
              const khMetaData &meta_);
};

typedef DerivedAssetHandle_<Asset, AssetImplD> AssetD;
typedef MutableAssetHandleD_<AssetD> MutableAssetD;


#endif /* __AssetD_h */

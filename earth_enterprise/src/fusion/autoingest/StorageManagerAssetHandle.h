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

#ifndef STORAGEMANAGERASSETHANDLE_H
#define STORAGEMANAGERASSETHANDLE_H

#include <functional>
#include <memory>

#include "autoingest/.idl/storage/AssetDefs.h"

// Objects outside the storage manager will access assets through AssetHandles.
// This allows the storage manager to properly clean up when the other object
// is done using the asset (release locks, update cache size, etc).
//
// This AssetHandle will eventually replace the AssetHandle_ definied in
// AssetHandle.h and its derivatives.
template<class AssetType>
class AssetHandle {
  private:
    AssetPointerType<AssetType> ptr;
    std::function<void(void)> onFinalize;
  public:
    AssetHandle(AssetPointerType<AssetType> ptr,
                std::function<void(void)> onFinalize)
        : ptr(ptr), onFinalize(onFinalize) {}
    AssetHandle() = default;
    ~AssetHandle() {
      if (onFinalize) onFinalize();
    }
    inline AssetType * operator->() const { return ptr.operator->(); }
    inline explicit operator bool() const { return ptr && (ptr->type != AssetDefs::Invalid); }
};

#endif // STORAGEMANAGERASSETHANDLE_CPP

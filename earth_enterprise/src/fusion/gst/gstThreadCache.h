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

#ifndef KHSRC_FUSION_GST_GSTTHREADCACHE_H__
#define KHSRC_FUSION_GST_GSTTHREADCACHE_H__

#include <gstCache.h>
#include <khThread.h>


// brief Thread safe version of gstCache.
// Lock protected access.
template < class Type, class TypeID = int >
class gstThreadCache : private gstCache< Type, TypeID > {
 public:
  typedef gstCache<Type, TypeID> BaseClass;
  typedef typename BaseClass::CacheItem CacheItem;

  gstThreadCache(int maxsize) : BaseClass(maxsize) {
  }

  ~gstThreadCache() {
  }

  Type fetchAndPin(TypeID id) {
    khLockGuard guard(cacheMutex);
    CacheItem* found = this->hash_table_->find(id);

    if (found != NULL) {
      gstCache<Type, TypeID>::makeHot(found);
      found->pin();
      return found->data();
    }
    return 0;
  }

  Type fetch(const TypeID& id) {
    khLockGuard guard(cacheMutex);
    CacheItem* found = this->hash_table_->find(id);

    if (found != 0) {
      gstCache<Type, TypeID>::makeHot(found);
      return found->data();
    }
    return 0;
  }

  //
  // return data that is replaced
  //
  Type insert(Type data, const TypeID& id) {
    // if data is NULL then don't actually insert into cache
    // (this matches behavior of using a fetch callback)
    if (data == NULL)
      return data;

    CacheItem* item = NULL;
    Type replace = 0;

    khLockGuard guard(cacheMutex);
    if (this->full() && (item = this->getColdest())) {
      replace = item->data();
      BaseClass::insert(item, data, id);
    } else {
      item = this->addItem();
      BaseClass::insert(item, data, id);
    }

    return replace;
  }

  int pin(TypeID id) {
    khLockGuard guard(cacheMutex);
    return BaseClass::pin(id);
  }

  int unpin(TypeID id) {
    khLockGuard guard(cacheMutex);
    return BaseClass::unpin(id);
  }

  inline void flush() {
    khLockGuard guard(cacheMutex);
    BaseClass::flush();
  }

 private:
  khMutex cacheMutex; // protect access to entire cache
};

#endif  // !KHSRC_FUSION_GST_GSTTHREADCACHE_H__

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


#include <khRefCounter.h>
#include <khCache.h>
#include <khException.h>
#include "khRasterProduct.h"
#include "khRasterProductLevel.h"

#ifndef KHSRC_FUSION_KHRASTER_PRODUCTLEVELREADERCACHE_H__
#define KHSRC_FUSION_KHRASTER_PRODUCTLEVELREADERCACHE_H__

// ****************************************************************************
// ***  Reader Cache for khRasterProductLevels
// ***
// ***  khRasterProductLevels can exist w/o opening their file or consuming
// ***  many resources. On large projects you can't keep them all open. And
// ***  it's expensive to keep opening and closing them. This class keeps a
// ***  number of them open using an LRU cache.
// ***
// ***  Because the ReadTile methods are templated instead of the entire class,
// ***  the ccache can be used to cache different type of products (e.g. data
// ***  and alpha at the same time)
// ****************************************************************************
class ProductLevelReaderCache
{
  class CachedReaderImpl : public khRefCounter {
   private:
    const khRasterProductLevel *prodLevel;

   public:
    /// determine amount of memory used by CachedReaderImpl
    std::uint64_t GetSize() {
      return sizeof(prodLevel);
    }
    inline CachedReaderImpl(const khRasterProductLevel *plev) :
        prodLevel(plev) {
      if (!prodLevel->OpenReader()) {
        throw khException(kh::tr("Unable to open product %1 level %2")
                          .arg(prodLevel->product()->name().c_str())
                          .arg(prodLevel->levelnum()));
      }
    }
    inline ~CachedReaderImpl(void) {
      prodLevel->CloseReader();
    }

    template <class DestTile>
    inline void
    ReadTile(std::uint32_t row, std::uint32_t col, DestTile &dest) const {
      if (!prodLevel->ReadTile(row, col, dest)) {
        throw khException
          (kh::tr("Unable to read tile (lrc) %1,%2,%3 %4")
           .arg(prodLevel->levelnum()).arg(row).arg(col)
           .arg(prodLevel->product()->name().c_str()));
      }
    }
  };
  typedef khRefGuard<CachedReaderImpl> CachedReader;


  mutable khCache<const khRasterProductLevel*, CachedReader> cache;

 public:
  inline ProductLevelReaderCache(unsigned int cacheSize) : cache(cacheSize, "product level reader") { }

  template <class DestTile>
  inline void
  ReadTile(const khRasterProductLevel* plev,
           std::uint32_t row, std::uint32_t col, DestTile &dest) const {
    CachedReader found;
    if (!cache.Find(plev, found)) {
      found = khRefGuardFromNew(new CachedReaderImpl(plev));
      cache.Add(plev, found);
    }
    found->ReadTile(row, col, dest);
  }
};

#endif  // !KHSRC_FUSION_KHRASTER_PRODUCTLEVELREADERCACHE_H__

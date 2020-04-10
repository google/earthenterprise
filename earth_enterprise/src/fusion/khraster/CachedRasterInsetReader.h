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


#ifndef __CachedRasterInsetReader_h
#define __CachedRasterInsetReader_h

#include <khraster/khRasterTile.h>
#include <khraster/khRasterProduct.h>
#include <khraster/khRasterProductLevel.h>
#include <khraster/MagnifyQuadrant.h>
#include <khRefCounter.h>
#include <khCache.h>
#include "ProductLevelReaderCache.h"


// ****************************************************************************
// ***  These classes assist in the efficent reading of Data and Alpha
// ***  products.
// ****************************************************************************

class ProdTileKey {
 private:
  const khRasterProductLevel *prodLevel;
  std::uint64_t                      tileid;

 public:
  inline bool operator< (const ProdTileKey &o) const {
    return ((prodLevel < o.prodLevel) ||
            ((prodLevel == o.prodLevel) && (tileid < o.tileid)));
  }

  inline ProdTileKey(const khRasterProductLevel *plev,
                     const khTileAddr &addr)
      : prodLevel(plev), tileid(addr.Id()) { }
};

template <class TileType>
class CachedProductTileImpl : public khRefCounter
{
 public:
  TileType tile;
};


template <class TileType_, class MagnifyWeighting>
class CachingProductTileReader_ImageryAlpha
{
 public:
  typedef TileType_ TileType;
 protected:
  typedef khRefGuard<CachedProductTileImpl<TileType> > CachedTile;
  typedef khCache<ProdTileKey, CachedTile> TileCache;

  // cache for lower res tiles loaded from products - full res tiles
  // aren't cached, since we should only need them once.
  TileCache  tileCache;
  ProductLevelReaderCache readerCache;

  TileType   tmpTile;
 public:
  void ReadTile(const khRasterProductLevel *prodLev,
                const khTileAddr &addr,
                TileType &dstTile,
                bool fillMissingWithZero = false);

  CachingProductTileReader_ImageryAlpha(unsigned int tileCacheSize,
                                        unsigned int readerCacheSize) :
      tileCache(tileCacheSize, "imagery alpha tile reader"),
      readerCache(readerCacheSize)
  { }
};

typedef CachingProductTileReader_ImageryAlpha
<ImageryProductTile, ImageryMagnifyWeighting> CachingProductTileReader_Imagery;

typedef CachingProductTileReader_ImageryAlpha
<AlphaProductTile, ImageryMagnifyWeighting> CachingProductTileReader_Alpha;


class CachingProductTileReader_Heightmap
{
 public:
  typedef HeightmapFloat32ProductTile TileType;
  typedef ExpandedHeightmapTile ExpandedTileType;
 protected:
  typedef khRefGuard<CachedProductTileImpl<TileType> > CachedTile;
  typedef khCache<ProdTileKey, CachedTile> TileCache;

  // cache for lower res tiles loaded from products - full res tiles
  // aren't cached, since we should only need them once.
  TileCache  tileCache;
  ProductLevelReaderCache readerCache;

  ExpandedTileType   tmpTile1;
  ExpandedTileType   tmpTile2;

  void FetchExtraPixels(const khRasterProductLevel *prodLev,
                        const khTileAddr &addr,
                        ExpandedTileType &dstTile,
                        const khOffset<std::uint32_t> &dstOffset,
                        const khExtents<std::uint32_t> &srcExtents,
                        const khExtents<std::uint32_t> &fallbackExtents);

  CachedTile FetchAndCacheProductTile(const khRasterProductLevel *prodLev,
                                      const khTileAddr &addr);
 public:
  void ReadTile(const khRasterProductLevel *prodLev,
                const khTileAddr &addr,
                TileType &dstTile, const bool is_mercator);

  CachingProductTileReader_Heightmap(unsigned int tileCacheSize,
                                     unsigned int readerCacheSize) :
      tileCache(tileCacheSize, "height map tile reader"),
      readerCache(readerCacheSize)
  { }
};



#endif /* __CachedRasterInsetReader_h */

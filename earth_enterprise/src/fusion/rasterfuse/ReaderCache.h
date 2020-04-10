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


#ifndef __ReaderCache_h
#define __ReaderCache_h

#include <khraster/ProductLevelReaderCache.h>
#include <khraster/ffioRasterReader.h>


// ****************************************************************************
// ***  Reader Cache for ffio::raster::Reader
// ***
// ***  ffio::raster::Readers can exist w/o opening their file or consuming
// ***  many resources. On large projects you can't keep them all open. And
// ***  it's expensive to keep opening and closing them. This class keeps a
// ***  number of them open using an LRU cache.
// ****************************************************************************
template <class TileType>
class FFIORasterReaderCache
{
  typedef ffio::raster::Reader<TileType> RawReader;

  class CachedReaderImpl : public khRefCounter {
   private:
    // we have no ownership claim on this pointer, it is managed by
    // someone above me and is guaranteed to exist for longer than this
    // object.
    const RawReader *reader;

   public:
    inline CachedReaderImpl(const RawReader *reader_) : reader(reader_) {
      // there is no explcit open
    }
    inline ~CachedReaderImpl(void) {
      reader->Close();
    }

    // determine amount of memory used by FFIORasterReaderCache
    std::uint64_t GetSize() {
      return sizeof(reader);
    }

    inline void
    ReadTile(const khTileAddr &addr, TileType &dest) const {
      if (!reader->FindReadTile(addr, dest)) {
        // more explicit message already emitted
        throw khException(kh::tr("Unable to read tile"));
      }
    }
  };
  typedef khRefGuard<CachedReaderImpl> CachedReader;


  mutable khCache<const RawReader*, CachedReader> cache;

 public:
  inline FFIORasterReaderCache(unsigned int cacheSize) : cache(cacheSize, "reader") { }

  inline void
  ReadTile(const RawReader *reader, const khTileAddr &addr,
           TileType &dest) const {
    CachedReader found;
    if (!cache.Find(reader, found)) {
      found = khRefGuardFromNew
              (new CachedReaderImpl(reader));
      cache.Add(reader, found);
    }
    found->ReadTile(addr, dest);
  }
};




#endif /* __ReaderCache_h */

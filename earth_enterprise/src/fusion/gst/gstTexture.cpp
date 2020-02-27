// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include "fusion/gst/gstTexture.h"

#include "common/kbf.h"
#include "fusion/khraster/khRasterProduct.h"
#include "fusion/khraster/CastTile.h"
#include "common/proto_streaming_imagery.h"
#include "common/khFileUtils.h"
#include "common/khGuard.h"
#include "fusion/gst/maprender/PreviewController.h"
#include "fusion/gst/gstProgress.h"
#include "common/khEndian.h"
#include "common/khTileAddr.h"
#include "fusion/gst/gstMisc.h"
#include "common/geindex/Reader.h"
#include "common/geindex/IndexBundle.h"
#include "common/compressor.h"
#include "fusion/dbgen/.idl/DBConfig.h"
#include "common/khConstants.h"


inline
khTileAddr
TranslateImageryToProductAddr(const khTileAddr &inTileAddr) {
  assert(ClientImageryTilespaceBase.tileSize <=
         RasterProductTilespaceBase.tileSize);
  std::uint32_t ratio = RasterProductTilespaceBase.tileSize /
                 ClientImageryTilespaceBase.tileSize;

  return khTileAddr(ImageryToProductLevel(inTileAddr.level),
                    inTileAddr.row / ratio,
                    inTileAddr.col / ratio);
}


// Flip image vertically
// NOTE: must be even number of lines (ie. not 255)
void FlipImageVertically(unsigned char* buff, int w, int h, int c) {
  const int line_sz = w * c;
  const int half_h = h >> 1;
  std::string tmp;
  tmp.reserve(line_sz);
  for (int y = 0; y < half_h; ++y) {
    memcpy(&tmp[0], buff + (y * line_sz), line_sz);
    memcpy(buff + (y * line_sz), buff + ((h - 1 - y) * line_sz), line_sz);
    memcpy(buff + ((h - 1 - y) * line_sz), &tmp[0], line_sz);
  }
}

// confirm buffer is a valid JPG image
bool IsValidJPG(const unsigned char* buff) {
  const unsigned char jfifhdrA[] = { 0xff, 0xd8, 0xff, 0xe0 };
  const unsigned char jfifhdrB[] = { 'J', 'F', 'I', 'F' };
  return memcmp(jfifhdrA, buff, 4) == 0 && memcmp(jfifhdrB, buff + 6, 4) == 0;
}

// confirm buffer is a valid PNG image
bool IsValidPNG(const unsigned char* buff) {
  const unsigned char pnghdr[] = { 'P', 'N', 'G'};
  return memcmp(pnghdr, buff + 1, 3) == 0;
}


// -----------------------------------------------------------------------------
// Keyhole Pyramid (PYR) texture
// -----------------------------------------------------------------------------

class gstPYRTexture : public gstTextureImpl {
 public:
  gstPYRTexture(const std::string& kippath, const std::string& kmppath);
  virtual ~gstPYRTexture();

  // inherited from gstTextureImpl
  virtual bool Load(std::uint64_t addr, unsigned char *obuff);
  virtual void Find(TileExistance *te, std::uint64_t addr);

  typedef ImageObj<char>* ImageObjPtr;

  static bool loadMaskFromDisk_cb(void* obj, ImageObjPtr& data,
                                  const std::uint64_t& id) {
    return (reinterpret_cast<gstPYRTexture*>(obj))->loadMaskFromDisk(data, id);
  }
  bool loadMaskFromDisk(ImageObjPtr& data, const std::uint64_t& id);

  static bool loadBaseFromDisk_cb(void* obj, ImageObjPtr& data,
                                  const std::uint64_t& id) {
    return (reinterpret_cast<gstPYRTexture*>(obj))->loadBaseFromDisk(data, id);
  }
  bool loadBaseFromDisk(ImageObjPtr& data, const std::uint64_t& id);

  static bool loadHeightmapBaseFromDisk_cb(void* obj, ImageObjPtr& data,
                                           const std::uint64_t& id) {
    return (reinterpret_cast<gstPYRTexture*>(obj))->loadHeightmapBaseFromDisk(
        data, id);
  }
  bool loadHeightmapBaseFromDisk(ImageObjPtr& data, const std::uint64_t& id);
  bool IsMercatorImagery() const { return kip_->IsMercator(); }

 private:
  typedef gstCache<ImageObjPtr, std::uint64_t> MemoryCache;
  MemoryCache* mask_memory_cache_;
  MemoryCache* base_memory_cache_;

  khDeleteGuard<khRasterProduct> kip_;
  khDeleteGuard<khRasterProduct> kmp_;

  gstImageLut<std::int16_t, unsigned char> contrast_lut_;
};

gstTextureGuard NewPYRTexture(
    const std::string& kippath, const std::string& kmppath) {
  try {
    return gstTextureGuard(
        khRefGuardFromNew(new gstPYRTexture(kippath, kmppath)));
  } catch(const std::exception &e) {
    notify(NFY_WARN, "%s", e.what());
  } catch(...) {
    notify(NFY_WARN, "INTERNAL ERROR: Unknown error creating PYR Texture");
  }
  return gstTextureGuard();
}

gstPYRTexture::gstPYRTexture(
  const std::string& kippath, const std::string& kmppath)
  : gstTextureImpl(),
    mask_memory_cache_(0),
    base_memory_cache_(0),
    kip_(),
    kmp_() {
  std::string kippath_local = kippath;
  if (khHasExtension(kippath, ".xml")) {
    kippath_local = khDirname(kippath);
  }
  kip_ = khRasterProduct::Open(kippath_local);

  bool heightmap = false;

  if (!kip_) {
    throw khException(kh::tr("Unable to open %1").arg(kippath.c_str()));
  } else if (kip_->type() == khRasterProduct::Imagery) {
    spec_ = gstGLTexSpec(GL_RGB, 256, 256, GL_RGB, GL_UNSIGNED_BYTE);

  } else if (kip_->type() == khRasterProduct::Heightmap) {
    spec_ = gstGLTexSpec(GL_RGB, 256, 256, GL_RGB, GL_UNSIGNED_BYTE);
    heightmap = true;
  } else {
    throw khException(kh::tr("%1 isn't Imagery or Heightmap").arg(kippath.c_str()));
  }

  std::string kmppath_local;
  if (!kmppath.empty()) {
    if (khHasExtension(kmppath, ".xml")) {
      kmppath_local = khDirname(kmppath);
    } else {
      kmppath_local = kmppath;
    }
  } else {
    kmppath_local = khReplaceExtension(kippath_local, ".kmp");
  }

  kmp_ = khRasterProduct::Open(kmppath);
  if (kmp_ && (kmp_->type() != khRasterProduct::AlphaMask)) {
    notify(NFY_NOTICE, "Unable to find alpha mask for %s", kmppath.c_str());
  }

  base_memory_cache_ = new MemoryCache(2);
  if (heightmap) {
    base_memory_cache_->addFetchCallback(
        loadHeightmapBaseFromDisk_cb, reinterpret_cast<void*>(this));
  } else {
    base_memory_cache_->addFetchCallback(
        loadBaseFromDisk_cb, reinterpret_cast<void*>(this));
  }

  mask_memory_cache_ = new MemoryCache(2);
  mask_memory_cache_->addFetchCallback(
      loadMaskFromDisk_cb, reinterpret_cast<void*>(this));

  //
  // assemble contrast stretch from base tile
  //
  if (heightmap) {
    // find level that has the most coverage, but is only 1 row x 1 col
    std::uint32_t lev = kip_->minLevel();
    for (std::uint32_t tlev = lev; tlev <= kip_->maxLevel(); ++tlev) {
      if (kip_->level(tlev).tileExtents().numRows() == 1 &&
          kip_->level(tlev).tileExtents().numCols() == 1) {
        lev = tlev;
      } else {
        break;
      }
    }
    khRasterProduct::Level &mylevel = kip_->level(lev);

    // read the tile
    HeightmapInt16ProductTile int16Tile;
    bool ok = false;
    switch (kip_->componentType()) {
      case khTypes::Int16:
        ok = mylevel.ReadTile(mylevel.tileExtents().beginRow(),
                              mylevel.tileExtents().beginCol(),
                              int16Tile);
        break;
      case khTypes::Float32:
        {
          // read float32 and cast it to std::int16_t
          HeightmapFloat32ProductTile float32Tile;
          ok = mylevel.ReadTile(mylevel.tileExtents().beginRow(),
                                mylevel.tileExtents().beginCol(),
                                float32Tile);
          if (ok) {
            CastTile(int16Tile, float32Tile);
          }
        }
        break;
      default:
        notify(NFY_WARN, "Unsupported heightmap type %s.",
               khTypes::StorageName(kip_->componentType()));
    }

    // generate the contrast LUT
    if (!ok) {
      notify(NFY_WARN,
             "Failed to read tile from heightmap. Preview will not be scaled");
    } else {
      const std::uint32_t maxbuf = HeightmapInt16ProductTile::BandPixelCount;
      gstImageStats<std::int16_t> stats(maxbuf, int16Tile.bufs[0]);
      contrast_lut_.setEqualize(stats);
    }
  }

  bbox_.init(kip_->degOrMeterWest(), kip_->degOrMeterEast(),
             kip_->degOrMeterSouth(), kip_->degOrMeterNorth());
}

gstPYRTexture::~gstPYRTexture() {
  delete base_memory_cache_;
  delete mask_memory_cache_;
}

void gstPYRTexture::Find(TileExistance* tile_existance, std::uint64_t addr) {
  TexTile tile(addr);

  // figure out if we're looking for an imagery or alpha tile
  khRasterProduct* rp = (tile.alpha() == 1) ? kmp_ : kip_;

  if (!rp) {
    tile_existance->bestAvailable = 0;
    return;
  }

  bool found = 0;
  while (!found) {
    // Check if the corresponding product tile exists
    // The texture engine uses the ImageryPacketTilespace internally.
    // Convert this tile address to RasterProductTilespace
    khTileAddr prodAddr(TranslateImageryToProductAddr
                        (khTileAddr(tile.lev,
                                    tile.row,
                                    tile.col)));
    if (!rp->validLevel(prodAddr.level) ||
        !rp->level(prodAddr.level).tileExtents().
        ContainsRowCol(prodAddr.row, prodAddr.col)) {
      if (tile.upLevel() == -1) {
        tile_existance->bestAvailable = 0;
        return;
      }
    } else {
      found = true;
    }
  }

  tile_existance->bestAvailable = tile.addr();
}


// Only called from gstTextureManager read thread
bool gstPYRTexture::Load(std::uint64_t addr, unsigned char* obuff) {
  TexTile tile(addr);

  // figure out if we're looking for an imagery or alpha tile
  khRasterProduct* rp = (tile.alpha() == 1) ? kmp_ : kip_;

  if (rp) {
    // convert tile address to rasterproduct space space
    khTileAddr prodAddr(TranslateImageryToProductAddr
                        (khTileAddr(tile.lev,
                                    tile.row,
                                    tile.col)));
    if (rp->validLevel(prodAddr.level) &&
        rp->level(prodAddr.level).tileExtents().
        ContainsRowCol(prodAddr.row, prodAddr.col)) {
      std::uint64_t naddr = TILEADDR(prodAddr.level,
                              prodAddr.row,
                              prodAddr.col,
                              tile.alpha(), tile.src);

      ImageObjPtr imgtile = tile.alpha() ? mask_memory_cache_->fetch(naddr) :
        base_memory_cache_->fetch(naddr);

      if (imgtile) {
        // find the offset of the imagery tile w/in the rasterproduct tile
        assert(RasterProductTileResolution % ImageryQuadnodeResolution == 0);
        assert(RasterProductTileResolution >= ImageryQuadnodeResolution);
        unsigned int ratio = RasterProductTileResolution / ImageryQuadnodeResolution;
        unsigned int y = tile.row % ratio;
        unsigned int x = tile.col % ratio;

        // finally copy the pixels we're looking for
        imgtile->getTile(x * ImageryQuadnodeResolution,
                         y * ImageryQuadnodeResolution,
                         ImageryQuadnodeResolution,
                         ImageryQuadnodeResolution,
                         reinterpret_cast<char*>(obuff),
                         Interleaved, LowerLeft);
        return true;
      }
    }
  }
  notify(NFY_WARN, "Internal Error: load requested for invalid tile,"
         " filling with black");
  memset(obuff, 0,
         ImageryQuadnodeResolution * ImageryQuadnodeResolution * 3);
  return true;
}

void draw(unsigned char* rgb[], unsigned int dim, unsigned int x, unsigned int y, unsigned int val[]) {
  if (x > dim - 1 || y > dim - 1)
    return;

  for (unsigned int c = 0; c < 3; ++c)
    rgb[c][(y * dim) + x] = val[c];
}

bool gstPYRTexture::loadHeightmapBaseFromDisk(ImageObjPtr& reuse,
                                              const std::uint64_t& id) {
  // load actual tile data into a temporary buf
  TexTile tile(id);
  if (!kip_->validLevel(tile.lev)) {
    notify(NFY_WARN, "Failed to read tile from heightmap (bad level).");
    return false;
  }
  khRasterProduct::Level &mylevel = kip_->level(tile.lev);
  HeightmapInt16ProductTile int16Tile;
  bool ok = false;
  switch (kip_->componentType()) {
    case khTypes::Int16:
      ok = mylevel.ReadTile(tile.row,
                            tile.col,
                            int16Tile);
      break;
    case khTypes::Float32:
      {
        // read float32 and cast it to std::int16_t
        HeightmapFloat32ProductTile float32Tile;
        ok = mylevel.ReadTile(tile.row,
                              tile.col,
                              float32Tile);
        if (ok) {
          CastTile(int16Tile, float32Tile);
        }
      }
      break;
    default:
      notify(NFY_WARN, "Unsupported heightmap type %s.",
             khTypes::StorageName(kip_->componentType()));
  }

  if (!ok) {
    notify(NFY_WARN, "Failed to read tile from heightmap.");
    return false;
  }

  // figure out where to put the RGB results
  if (!reuse) {
    ImgTile intile(RasterProductTileResolution, RasterProductTileResolution, 3);
    reuse = new ImageObj<char>(intile, Separate, LowerLeft);
  }
  unsigned char* red   = reinterpret_cast<unsigned char*>(reuse->getData(0));
  unsigned char* green = reinterpret_cast<unsigned char*>(reuse->getData(1));
  unsigned char* blue  = reinterpret_cast<unsigned char*>(reuse->getData(2));

  // assemble one channel
  const std::uint32_t maxbuf = HeightmapInt16ProductTile::BandPixelCount;
  for (unsigned int texel = 0; texel < maxbuf; ++texel) {
    red[texel] = contrast_lut_.getVal(int16Tile.bufs[0][texel]);
  }
  // and copy the rest
  memcpy(green, red, maxbuf);
  memcpy(blue, red, maxbuf);

  return true;
}


bool gstPYRTexture::loadBaseFromDisk(ImageObjPtr& reuse, const std::uint64_t& id) {
  if (!reuse) {
    ImgTile intile(RasterProductTileResolution, RasterProductTileResolution, 3);
    reuse = new ImageObj<char>(intile, Separate, LowerLeft);
  }
  unsigned char* destBufs[3] = {
    reinterpret_cast<unsigned char*>(reuse->getData(0)),
    reinterpret_cast<unsigned char*>(reuse->getData(1)),
    reinterpret_cast<unsigned char*>(reuse->getData(2)),
  };

  TexTile tile(id);

  // pull data from file
  bool is_monochromatic = kip_->level(tile.lev).IsMonoChromatic();
  khRasterTileBorrowBufs<ImageryProductTile> pyrTile(destBufs,
                                                     is_monochromatic);
  kip_->level(tile.lev).ReadTile(tile.row, tile.col, pyrTile);

  // We've already commited by changing 'reuse' above. Even if the ReadTile
  // fails we must now return true (and just supply garbage data)
  return true;
}

bool gstPYRTexture::loadMaskFromDisk(ImageObjPtr& reuse, const std::uint64_t& id) {
  if (!kmp_) {
    // since we return false, we don't care about reuse
    return false;
  }

  // TODO: - why do we always delete the old one?
  // What's wrong with reusing it?
  if (reuse)
    delete reuse;

  ImgTile intile(RasterProductTileResolution,  RasterProductTileResolution, 1);
  reuse = new ImageObj<char>(intile, Separate, LowerLeft);

  // read tile as band-separate
  unsigned char* destBufs[1] = {
    reinterpret_cast<unsigned char*>(reuse->getData(0))
  };
  TexTile tile(id);

  // pull data from file
  khRasterTileBorrowBufs<AlphaProductTile> pyrTile(destBufs, false);
  kmp_->level(tile.lev).ReadTile(tile.row, tile.col, pyrTile);

  // We've already commited by changing 'reuse' above. Even if the ReadTile
  // fails we must now return true (and just supply garbage data)
  return true;
}

//------------------------------------------------------------------------------
// 2D Maps Texture
//------------------------------------------------------------------------------

// class QString;
// class MapLayerConfig;

class gstMapTexture : public gstTextureImpl {
 public:
  gstMapTexture(const MapLayerConfig& cfg, gstMapTexture *prev,
                const bool is_mercator_preview);
  virtual ~gstMapTexture();

  // inherited from gstTextureImpl
  virtual bool Load(std::uint64_t addr, unsigned char* obuff);
  virtual void Find(TileExistance* te, std::uint64_t addr);

 private:
  khDeleteGuard<maprender::PreviewController> preview_controller_;
};

gstTextureGuard NewMapTexture(const MapLayerConfig& cfg,
                              gstTextureGuard prev,
                              QString &error,
                              const bool is_mercator_preview) {
  khRefGuard<gstMapTexture> map_texture;
  map_texture.dyncastassign(prev);

  try {
    return gstTextureGuard(khRefGuardFromNew(
        new gstMapTexture(cfg, &*map_texture, is_mercator_preview)));
  } catch(const khException &e) {
    error = kh::tr("Unable to preview:\n %1").arg(e.qwhat());
  } catch(const std::exception& e) {
    error = kh::tr("Unable to preview:\n %1").arg(e.what());
  } catch(...) {
    error = kh::tr("Unable to preview:\n %1").arg("Unknown error");
  }
  return gstTextureGuard();
}

gstMapTexture::gstMapTexture(const MapLayerConfig& layerConfig,
                             gstMapTexture *prev,
                             const bool is_mercator_preview) {
  if (is_mercator_preview) {
    // init to world coverage in Mercator meter.
    bbox_.init(-khGeomUtilsMercator::khEarthCircumference / 2.0,  // west
               khGeomUtilsMercator::khEarthCircumference / 2.0,   // east
               -khGeomUtilsMercator::khEarthCircumference / 2.0,  // south
               khGeomUtilsMercator::khEarthCircumference / 2.0);  // north
  } else {
    bbox_.init(-180, 180, -180, 180);  // init to world coverage in Deg
  }

  gstProgress progress;

  preview_controller_ = TransferOwnership(new maprender::PreviewController(
      is_mercator_preview ? FusionMapMercatorTilespace : FusionMapTilespace,
      0.005 /* oversizeFactor */,
      layerConfig, progress,
      prev ? &*prev->preview_controller_ : 0,
      false /* debug */));
}

gstMapTexture::~gstMapTexture() {
}

bool gstMapTexture::Load(std::uint64_t addr, unsigned char* obuff) {
  try {
    khDeleteGuard<unsigned char, ArrayDeleter> tmpBuf(TransferOwnership
                                              (new unsigned char[256 * 256 * 4]));

    preview_controller_->GetTile(addr, &tmpBuf[0]);

#if __BYTE_ORDER == __BIG_ENDIAN
    // Big Endian - ARGB
    if (SUBFROMADDR(addr)) {
      // alpha
      unsigned char *in = &tmpBuf[0];
      for (unsigned int i = 0; i < 256 * 256; ++i) {
        *obuff++ = in[0];
        in += 4;
      }
    } else {
      unsigned char *in = &tmpBuf[0];
      for (unsigned int i = 0; i < 256 * 256; ++i) {
        *obuff++ = in[1];
        *obuff++ = in[2];
        *obuff++ = in[3];
        in += 4;
      }
    }
#else
    // Little Endian - BGRA
    if (SUBFROMADDR(addr)) {
      // alpha
      unsigned char *in = &tmpBuf[0];
      for (unsigned int i = 0; i < 256 * 256; ++i) {
        *obuff++ = in[3];
        in += 4;
      }
    } else {
      unsigned char *in = &tmpBuf[0];
      for (unsigned int i = 0; i < 256 * 256; ++i) {
        *obuff++ = in[2];
        *obuff++ = in[1];
        *obuff++ = in[0];
        in += 4;
      }
    }
#endif
  } catch(const std::exception &e) {
    notify(NFY_WARN, "%s", e.what());
    return false;
  } catch(...) {
    notify(NFY_WARN, "INTERNAL ERROR: Unknown error creating Map Texture");
    return false;
  }
  return true;
}

void gstMapTexture::Find(TileExistance* te, std::uint64_t addr) {
  // tile always exists
  if (preview_controller_->HasLevel(LEVFROMADDR(addr))) {
    te->bestAvailable = addr;
  } else {
    te->bestAvailable = 0;
  }
}

//------------------------------------------------------------------------------
// HTTP Texture
//------------------------------------------------------------------------------

class gstHTTPTexture : public gstTextureImpl {
 public:
  explicit gstHTTPTexture(const std::string& server);
  virtual ~gstHTTPTexture();

  // inherited from gstTextureImpl
  virtual bool Load(std::uint64_t addr, unsigned char* obuff);
  virtual void Find(TileExistance* te, std::uint64_t addr);

 private:
  gstEarthStream earth_stream_;
};

gstTextureGuard NewHTTPTexture(const std::string& server) {
  try {
    return gstTextureGuard(khRefGuardFromNew(new gstHTTPTexture(server)));
  } catch(const std::exception& e) {
    notify(NFY_WARN, "%s", e.what());
  } catch(...) {
    notify(NFY_WARN, "INTERNAL ERROR: Unknown error creating HTTP Texture");
  }
  return gstTextureGuard();
}

gstHTTPTexture::gstHTTPTexture(const std::string& server)
  : earth_stream_(server) {
}

gstHTTPTexture::~gstHTTPTexture() {
}

void gstHTTPTexture::Find(TileExistance* tile_existance, std::uint64_t addr) {
  TexTile tex_tile(addr);

  // don't support alpha
  if (tex_tile.alpha() == 1) {
    tile_existance->bestAvailable = 0;
    return;
  }

  while (earth_stream_.GetImageVersion(gstQuadAddress(tex_tile.lev,
      tex_tile.row, tex_tile.col)) == gstEarthStream::invalid_version) {
    if (tex_tile.upLevel() < 0) {
      notify(NFY_WARN, "Unable to find a texture tile for requested addr:%llu",
             static_cast<long long unsigned>(addr));
      tile_existance->bestAvailable = 0;
      return;
    }
  }

  tile_existance->bestAvailable = tex_tile.addr();
}

bool gstHTTPTexture::Load(std::uint64_t addr, unsigned char* obuff) {
  TexTile tex_tile(addr);
  gstQuadAddress quad_addr(tex_tile.lev, tex_tile.row, tex_tile.col);
  return earth_stream_.GetImage(quad_addr, reinterpret_cast<char*>(obuff));
}

// -----------------------------------------------------------------------------
// GE Index Texture
// -----------------------------------------------------------------------------

class gstGEIndexTexture : public gstTextureImpl {
 public:
  explicit gstGEIndexTexture(const std::string& path);
  virtual ~gstGEIndexTexture() {}

  // inherited from gstTextureImpl
  virtual bool Load(std::uint64_t addr, unsigned char* obuff);
  virtual void Find(TileExistance* te, std::uint64_t addr);

 private:
  khDeleteGuard<geindex::BlendReader> reader_;
  LittleEndianReadBuffer find_buf_;
  LittleEndianReadBuffer read_buf_;
  khDeleteGuard<Compressor> compressor_;

  // used to extract imagery-packets in protobuf-format.
  geEarthImageryPacket packet_;
};

gstTextureGuard NewGEIndexTexture(const std::string& path) {
  try {
    return gstTextureGuard(khRefGuardFromNew(new gstGEIndexTexture(path)));
  } catch(const std::exception &e) {
    notify(NFY_WARN, "%s", e.what());
  } catch(...) {
    notify(NFY_WARN, "INTERNAL ERROR: Unknown error creating GE Index Texture");
  }
  return gstTextureGuard();
}

gstGEIndexTexture::gstGEIndexTexture(const std::string& path)
  : compressor_(TransferOwnership(new JPEGCompressor(256, 256, 3, 0))) {
  // if (geindex::IsGEIndexOfType(GlobalFilePool, path, "Imagery")) {
  if (geindex::IsGEIndex(path)) {
    reader_ = TransferOwnership(
                  new geindex::BlendReader(*GlobalFilePool, path,
                  2 /* num bucket levels cached */));
  } else {
    throw khException(
        kh::tr("Directory path \"%1\" is not an Imagery Index").arg(
            path.c_str()));
  }
}

void gstGEIndexTexture::Find(TileExistance* te, std::uint64_t addr) {
  TexTile tex_tile(addr);

  // GE Index doesn't support alpha
  if (tex_tile.alpha() == 1) {
    te->bestAvailable = 0;
    return;
  }

  while (true) {
    try {
      geindex::BlendEntry entry;
      reader_->GetEntry(QuadtreePath(tex_tile.lev, tex_tile.row, tex_tile.col),
          geindex::BlendEntry::ReadKey(),
          &entry, find_buf_);
      break;
    } catch(...) {
      if (tex_tile.upLevel() < 0) {
      notify(NFY_WARN, "Unable to find a texture tile for requested addr:%llu",
             static_cast<long long unsigned>(addr));
      te->bestAvailable = 0;
      return;
      }
    }
  }

  te->bestAvailable = tex_tile.addr();
}

bool gstGEIndexTexture::Load(std::uint64_t addr, unsigned char* obuff) {
  TexTile tile(addr);

  try {
    const bool size_only = false;
    reader_->GetData(QuadtreePath(static_cast<std::uint64_t>(tile.lev),
                                  static_cast<std::uint64_t>(tile.row),
                                  static_cast<std::uint64_t>(tile.col)),
                     geindex::BlendEntry::ReadKey(), read_buf_, size_only);

    // Do quick check for a valid 2d jpeg.
    if (IsValidJPG(reinterpret_cast<const unsigned char*>(&read_buf_[0]))) {
      compressor_->decompress(reinterpret_cast<char*>(&read_buf_[0]),
                              cellSize(),
                              reinterpret_cast<char*>(obuff));
    } else if (IsValidPNG(reinterpret_cast<const unsigned char*>(&read_buf_[0]))) {
      // Create blank (gray) tile.
      memset(obuff, 128, 256 * 256 * 3);
      notify(NFY_WARN, "Png decompression not yet supported, using blank.");
      return true;

    // Try to parse 3d proto imagery packet.
    } else if (packet_.ParseFromString(read_buf_)) {
      if (packet_.HasImageData()) {
        const std::string &image_data = packet_.ImageData();

        if (!IsValidJPG(reinterpret_cast<const unsigned char*>(&image_data[0]))) {
          notify(NFY_WARN,
                 "buffer read from GE Index doesn't contain a valid JPG image,"
                 " skipping!");
          packet_.Clear();
          return false;
        }
        compressor_->decompress(
            const_cast<char*>(&image_data[0]),
            cellSize(),
            reinterpret_cast<char*>(obuff));
      } else {
        notify(NFY_WARN,
               "buffer read from GE Index doesn't contain image data,"
               " skipping!");
        packet_.Clear();
        return false;
      }
    } else {
      notify(NFY_WARN, "Unrecognized packet data, skipping!");
      return false;
    }

    packet_.Clear();

    // jpg image is upper-left orientation and texture must be lower-left
    FlipImageVertically(obuff, 256, 256, 3);
    return true;
  } catch(const khException &e) {
    // message in e.qwhat()
    notify(NFY_WARN, "Failed reading tile (lev=%d row=%u col=%u)",
           tile.lev, tile.row, tile.col);
  } catch(...) {
    // unknown error
  }
  return false;
}

// -----------------------------------------------------------------------------
// GEDB Texture
// -----------------------------------------------------------------------------

class gstGEDBTexture : public gstTextureImpl {
 public:
  explicit gstGEDBTexture(const std::string& path);
  virtual ~gstGEDBTexture() {}

  // inherited from gstTextureImpl
  virtual bool Load(std::uint64_t addr, unsigned char* obuff);
  virtual void Find(TileExistance* te, std::uint64_t addr);

 private:
  khDeleteGuard<geindex::UnifiedReader> reader_;
  LittleEndianReadBuffer find_buf_;
  LittleEndianReadBuffer read_buf_;
  khDeleteGuard<Compressor> compressor_;
};

gstTextureGuard NewGEDBTexture(const std::string& path) {
  try {
    return gstTextureGuard(khRefGuardFromNew(new gstGEDBTexture(path)));
  } catch(const std::exception &e) {
    notify(NFY_WARN, "%s", e.what());
  } catch(...) {
    notify(NFY_WARN, "INTERNAL ERROR: Unknown error creating GE Index Texture");
  }
  return gstTextureGuard();
}

gstGEDBTexture::gstGEDBTexture(const std::string& path)
  : compressor_(TransferOwnership(new JPEGCompressor(256, 256, 3, 0))) {
  std::string headerpath = khComposePath(path, kHeaderXmlFile);
  if (!khExists(headerpath)) {
    throw khException(kh::tr("\"%1\" is not a GEDB").arg(path.c_str()));
  }
  std::string indexpath;
  {
    DbHeader header;
    if (!header.Load(headerpath)) {
      throw khException(kh::tr("Unable to load GEDB header \"%1\"")
                        .arg(headerpath.c_str()));
    }
    indexpath = header.index_path_;
  }
  if (!khIsAbspath(indexpath)) {
    indexpath = khComposePath(path, indexpath);
  }

  reader_ = TransferOwnership(
      new geindex::UnifiedReader(*GlobalFilePool, indexpath,
                                 2 /* num bucket levels cached */));
}

void gstGEDBTexture::Find(TileExistance* te, std::uint64_t addr) {
  TexTile tex_tile(addr);

  // GEDB doesn't support alpha
  if (tex_tile.alpha() == 1) {
    te->bestAvailable = 0;
    return;
  }

  while (true) {
    try {
      geindex::UnifiedEntry entry;
      reader_->GetEntry(QuadtreePath(tex_tile.lev, tex_tile.row, tex_tile.col),
                        geindex::TypedEntry::ReadKey(
                            0 /* version */,
                            kGEImageryChannelId,
                            geindex::TypedEntry::Imagery,
                            false /* version matters */),
                        &entry, find_buf_);
      break;
    } catch(...) {
      if (tex_tile.upLevel() < 0) {
        notify(NFY_WARN,
               "Unable to find a texture tile for requested addr:%llu",
               static_cast<long long unsigned>(addr));
        te->bestAvailable = 0;
        return;
      }
    }
  }

  te->bestAvailable = tex_tile.addr();
}

bool gstGEDBTexture::Load(std::uint64_t addr, unsigned char* obuff) {
  TexTile tile(addr);

  try {
    const bool size_only = false;
    reader_->GetData(QuadtreePath(static_cast<std::uint64_t>(tile.lev),
                                  static_cast<std::uint64_t>(tile.row),
                                  static_cast<std::uint64_t>(tile.col)),
                     geindex::TypedEntry::ReadKey(
                         0 /* version */,
                         kGEImageryChannelId,
                         geindex::TypedEntry::Imagery,
                         false /* version matters */),
                     read_buf_,
                     size_only);

    if (!IsValidJPG(reinterpret_cast<unsigned char*>(&read_buf_[0]))) {
      notify(NFY_WARN,
             "GEDB buffer doesn't contain a valid JPG image, skipping!");
      return false;
    }
    compressor_->decompress(reinterpret_cast<char*>(&read_buf_[0]), cellSize(),
                            reinterpret_cast<char*>(obuff));
    // jpg image is upper-left orientation and texture must be lower-left
    FlipImageVertically(obuff, 256, 256, 3);
    return true;
  } catch(const khException &e) {
    // message in e.qwhat()
    notify(NFY_WARN, "Failed reading tile (lev=%d row=%u col=%u)",
           tile.lev, tile.row, tile.col);
  } catch(...) {
    // unknown error
  }
  return false;
}

void xyToBlist(unsigned int xbits, unsigned int ybits, int blevel, unsigned char blist[32]) {
  std::uint32_t right, top;
  std::int32_t order[][2] = {{0, 3}, {1, 2}};

  for (int j = 0; j < blevel; ++j) {
    right = xbits & (0x1U << (blevel - j - 1));
    top   = ybits & (0x1U << (blevel - j - 1));

    blist[j] = order[right != 0][top != 0];
  }
}


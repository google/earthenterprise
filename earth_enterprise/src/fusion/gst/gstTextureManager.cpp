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


#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <alloca.h>
#include <assert.h>
#include <string.h>
#include <fnmatch.h>
#include <ctype.h>

#include <autoingest/Asset.h>
#include <autoingest/AssetVersion.h>
#include <autoingest/plugins/RasterProjectAsset.h>

#include <gstTextureManager.h>
#include <image.h>
#include <gstHistogram.h>
#include <gstGeode.h>
#include <gstFileUtils.h>
#include <khConstants.h>
#include <khFileUtils.h>
#include <khFunctor.h>
#include <khException.h>

void ReadThread::start() {
  khThreadBase::PrepareProfiling(&profile_timer);
  QThread::start();
}

void ReadThread::run(void) {
  khThreadBase::BeginProfiling(profile_timer);
  texman->readThreadFunc();
}

gstTextureManager *theTextureManager = nullptr;

gstTextureManager::gstTextureManager(int memoryCacheSize, int textureCacheSize)
    : base_texture_(),
      readThread(this),
      requested_level_(0)
{
  theTextureManager = this;

  texture_number_ = BASE_IMAGE_ID + 1;

  tile_existance_cache_ = new TileExistanceCache(5000);
  tile_existance_cache_->addFetchCallback(findTile_cb, (void*)this);

  base_texture_cache_ = new TextureCache(textureCacheSize);
  base_texture_cache_->addFetchCallback(bindTile_cb, (void*)this);

  overlay_RGB_texture_cache_ = new TextureCache(textureCacheSize);
  overlay_RGB_texture_cache_->addFetchCallback(bindTile_cb, (void*)this);

  overlay_alpha_texture_cache_ = new TextureCache(textureCacheSize);
  overlay_alpha_texture_cache_->addFetchCallback(bindTile_cb, (void*)this);

  texture_cache_histogram_ = new gstHistogram(textureCacheSize);

  memory_cache_ = new MemoryCache(memoryCacheSize);

  current_tex_id_ = 0;

  readThread.start();
}

gstTextureManager::~gstTextureManager() {
  // Shut down the read thread first. it may come back and try to access
  // the memory I'm about to release.
  readQueue.clear();
  readQueue.push(InvalidTileAddrHash);  // signal for read thread to exit
  readThread.wait();

  delete tile_existance_cache_;
  delete base_texture_cache_;
  delete overlay_RGB_texture_cache_;
  delete overlay_alpha_texture_cache_;

  // silence debug warning by unpinning base texture
  memory_cache_->unpin(0);
  delete memory_cache_;
}


//
// must be called inside a draw routine
// requires a valid graphics context
//
void gstTextureManager::glinit() {
  //
  // create a checkered texture for the invalid regions
  GLubyte checkImage[CHECK_IMAGE_SIZE][CHECK_IMAGE_SIZE][3];
  GLubyte c;
  for (int i = 0; i < CHECK_IMAGE_SIZE; i++) {
    for (int j = 0; j < CHECK_IMAGE_SIZE; j++) {
      c = ((((i & 0x8) == 0) ^ ((j & 0x8)) == 0)) * 100;  // + 100;
      checkImage[i][j][0] = c;
      checkImage[i][j][1] = c;
      checkImage[i][j][2] = c;
    }
  }

  glBindTexture(GL_TEXTURE_2D, CHECK_IMAGE_ID);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  // glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
               CHECK_IMAGE_SIZE, CHECK_IMAGE_SIZE, 0,
               GL_RGB, GL_UNSIGNED_BYTE, checkImage);


  // indicate no texture loads are pending
  resetFrame();
}

// -----------------------------------------------------------------------------

gstTEXFormat::gstTEXFormat(const char *n, bool is_mercator_imagery_asset)
    : gstFormat(n, is_mercator_imagery_asset),
      texture_() {
  nofileok = true;
}

gstTEXFormat::~gstTEXFormat() {
  CloseFile();
}

// loadfile version to support formatManager
gstStatus gstTEXFormat::OpenFile() {
  texture_ = theTextureManager->NewTextureFromPath(name(),
                                                   &is_mercator_imagery_);
  if (!texture_) {
    return GST_OPEN_FAIL;
  } else {
    theTextureManager->AddOverlayTexture(texture_);
  }

  // build bounding box geometry
  extents_ = texture_->bbox();

  AddLayer(gstPolyLine, 1, gstHeaderImpl::Create());

  return GST_OKAY;
}

gstStatus gstTEXFormat::CloseFile() {
  if (texture_) {
    theTextureManager->removeTexture(texture_);
    texture_.release();
  }

  return GST_OKAY;
}

gstGeodeHandle gstTEXFormat::GetFeatureImpl(std::uint32_t layer, std::uint32_t id) {
  assert(layer < NumLayers());
  assert(layer == 0);
  assert(id < NumFeatures(layer));

  return gstGeodeImpl::Create(extents_);
}

gstRecordHandle gstTEXFormat::GetAttributeImpl(std::uint32_t, std::uint32_t) {
  throw khException(kh::tr("No attributes available"));
}

void gstTEXFormat::setEnabled(bool s) {
  texture_->setEnabled(s);
  gstFormat::setEnabled(s);
}

// -----------------------------------------------------------------------------

gstTextureGuard gstTextureManager::NewTextureFromPath(
    const std::string& path, bool* is_mercator_imagery) {
  gstTextureGuard error_handle;
  if (path.empty()) {
    notify(NFY_WARN, "Bad texture path: %s", path.c_str());
    return error_handle;
  }

  // handle URL first since it is not a valid file system path
  if (QString(path.c_str()).startsWith("http://") ||
      QString(path.c_str()).startsWith("https://")) {
    return NewHTTPTexture(path);
  }

  QString base(khBasename(path).c_str());

  bool is_mercator_imagery_project =
      khHasExtension(path, kMercatorImageryProjectSuffix);
  bool is_flat_projection_imagery_project =
      khHasExtension(path, kImageryProjectSuffix);

  if (is_mercator_imagery_project || is_flat_projection_imagery_project) {
    *is_mercator_imagery = is_mercator_imagery_project;
    Asset asset(AssetDefs::FilenameToAssetPath(path));
    if (asset->type != AssetDefs::Imagery && asset->subtype !=
        (*is_mercator_imagery ? kMercatorProjectSubtype : kProjectSubtype)) {
      notify(NFY_WARN, "Asset %s is not a valid imagery project", path.c_str());
      return error_handle;
    }
    AssetVersion last_good_version(asset->GetLastGoodVersionRef());
    if (!last_good_version) {
      notify(NFY_WARN, "Asset %s doesn't have a valid current version",
             path.c_str());
      return error_handle;
    }
    std::string index_path = last_good_version->GetOutputFilename(0);
    if (khHasExtension(index_path, ".geindex")) {
      return NewGEIndexTexture(index_path);
    } else {
      return error_handle;
    }
  }
  bool is_flat_imagery_asset = base.contains(kImageryAssetSuffix.c_str());
  bool is_flat_terrain_asset = base.contains(kTerrainAssetSuffix.c_str());
  bool is_mercator_imagery_asset = base.contains(kMercatorImageryAssetSuffix.c_str());
  if (is_flat_imagery_asset ||
      is_mercator_imagery_asset ||
      is_flat_terrain_asset) {
    *is_mercator_imagery = is_mercator_imagery_asset;
    AssetVersion version(path);
    if (version &&
        (version->type == AssetDefs::Terrain ||
         version->type == AssetDefs::Imagery) &&
        version->state == AssetDefs::Succeeded) {
      std::string kippath = version->GetOutputFilename(0);
      std::string kmppath = version->GetOutputFilename(1);
      return NewPYRTexture(kippath, kmppath);
    }
  } else {
    if (!khExists(path)) {
      notify(NFY_WARN, "Bad texture path: %s", path.c_str());
      return error_handle;
    }
    if (base.endsWith(".gedb")) {
      *is_mercator_imagery = false;
      return NewGEDBTexture(path);
    } else if (base.endsWith(".kip") || base.endsWith(".ktp")) {
      std::string str = path;
      str += "/" + kHeaderXmlFile;
      gstTextureGuard texture = NewPYRTexture(str, std::string());
      if (texture) {
        *is_mercator_imagery = texture->IsMercatorImagery();
      }
      return texture;
    } else if (base.endsWith(".xml")) {
      gstTextureGuard texture = NewPYRTexture(path, std::string());
      if (texture) {
        *is_mercator_imagery = texture->IsMercatorImagery();
      }
      return texture;
    }
  }

  return error_handle;
}

bool gstTextureManager::AddBaseTexture(gstTextureGuard texture) {
  // lock out read thread from doing any real work.
  // It may grab another addr from the queue, but it won't
  // be able to do any work with it.
  khLockGuard guard(readWorkerMutex);

  texture->id(0);

  // Set new base texture, will release old one
  base_texture_ = texture;

  // prepare our base texture
  // XXX should try to free any existing base texture memory here!!!
  unsigned char* buf = new unsigned char[texture->cellSize()];
  if (texture->Load(TILEADDR(0, 0, 0, 0, 0), buf) == false) {
    notify(NFY_WARN, "Unable to load base texture tile!  "
           "Disabling background texture.");
    texture->setEnabled(false);
    delete [] buf;
    return false;
  } else {
    // clear out any previous memory
    memory_cache_->flush();
    base_texture_cache_->flush();
    tile_existance_cache_->flush();

    memory_cache_->insert(buf, 0L);
    // pin down so it can never fall out of the cache
    memory_cache_->pin(0);

    unsigned int texid = BASE_IMAGE_ID;

    // calls FindTexture which locks textureMapMutex
    if (!bindTile(texid, 0)) {
      notify(NFY_WARN, "Unable to load base texture tile!  "
             "Disabling background texture.");
      texture->setEnabled(false);
      return false;
    } else {
      texture->setEnabled(true);
    }
  }
  return true;
}

bool gstTextureManager::AddOverlayTexture(gstTextureGuard texture) {
  khLockGuard guard(textureMapMutex);
  texture->id(getNextTexID());
  texture_map_[texture->id()] = texture;
  return true;
}

void gstTextureManager::toggleBaseTexture() {
  if (base_texture_)
    base_texture_->setEnabled(!base_texture_->enabled());
}

void gstTextureManager::removeTexture(const gstTextureGuard &toRemove) {
  khLockGuard guard(textureMapMutex);
  texture_map_.erase(toRemove->id());
}

// search for a texture by ID, returning the base texture
// if the ID does not exist
// TODO: this seems wrong.  should return a failure
// code if the ID does not exist and let the caller default
// to the base texture if it wants to.
gstTextureGuard gstTextureManager::FindTexture(int id) {
  khLockGuard guard(textureMapMutex);
  if (id != 0) {
    std::map<unsigned int, gstTextureGuard>::iterator found = texture_map_.find(id);
    if (found != texture_map_.end())
      return found->second;
  }

  return base_texture_;
}


void gstTextureManager::resetFrame() {
  render_unfinished_ = 0;
  readQueue.clear();
}

// return -1 if texture not found, otherwise, return actual level number
// of texture
int gstTextureManager::prepareBaseTexture(TexTile& tile) {
  // base texture should always have id=0
  if (!base_texture_ || !base_texture_->enabled()) {
    glBindTexture(GL_TEXTURE_2D, CHECK_IMAGE_ID);
    tile.s1 = 2;
    tile.t1 = 2;
    return -1;
  }

  tile.src = 0;

  TileExistance* te = tile_existance_cache_->fetch(tile.addr());
  assert(te != nullptr);

  // reset address (if necessary)
  requested_level_ = LEVFROMADDR(te->bestAvailable);
  while (tile.lev > requested_level_)
    tile.upLevel();

  unsigned int texid;
  while ((texid = base_texture_cache_->fetchAndPin(tile.addr())) == 0) {
    if (tile.upLevel() < 0) {
      notify(NFY_WARN, "Base texture should always have level 0 tile!");
      glBindTexture(GL_TEXTURE_2D, CHECK_IMAGE_ID);
      return -1;
    }
  }

  // we found one, so let's load it up!
  // if the fetch forced a load, it will already be bound, but if
  // it was already in the cache, we need this.
  glBindTexture(GL_TEXTURE_2D, texid);

  assert(tile.lev >= 0);

  return tile.lev;
}


void gstTextureManager::GetOverlayList(
    const TexTile& tile,
    std::vector<gstTextureGuard> &draw_list, const bool is_mercator_preview) {
  khLockGuard guard(textureMapMutex);

  gstBBox tilebox(tile.xx, tile.xx + tile.grid,
                  tile.yy, tile.yy + tile.grid);
  if (is_mercator_preview) {
    // If Mercator projection, expect image bounding box in meter;
    // So convert viewable tile to meter for intersection test.
    tilebox.init(khTilespace::DeNormalizeMeter(tilebox.w),
                 khTilespace::DeNormalizeMeter(tilebox.e),
                 khTilespace::DeNormalizeMeter(tilebox.s),
                 khTilespace::DeNormalizeMeter(tilebox.n));
  } else {
    // If Flat projection, expect image bounding box in degree;
    // So convert viewable tile to degree for intersection test.
    tilebox.init(khTilespace::Denormalize(tilebox.w),
                 khTilespace::Denormalize(tilebox.e),
                 khTilespace::Denormalize(tilebox.s),
                 khTilespace::Denormalize(tilebox.n));
  }

    for (const auto& it : texture_map_) {
      gstTextureGuard tex = it.second;
      if (tex && tex->enabled() && tex != base_texture_) {
        if (tilebox.Intersect(tex->bbox()))
          draw_list.push_back(tex);
    }
  }
}


//
// only call this function immediately after GetOverlayList()
// which will ensure the list of texture id's are valid
//
int gstTextureManager::prepareTexture(TexTile& tile,
                                      const gstTextureGuard &tex) {
  // if no secondary textures have been loaded, return immediately
  if (getCurrTexID() == 0)
    return -1;

  tile.src = tex->id();

  // determine what is the best tile that exists in our db
  TileExistance* te = tile_existance_cache_->fetch(tile.addr());

  assert(te != nullptr);

  // no texture available
  if (te->bestAvailable == 0)
    return -1;


  // now try to get this best one, but take anything less as long
  // as we can get something immediately
  // this will fill our read queue with everything from this tile
  // all the way up the stack until we've found a tile

  requested_level_ = LEVFROMADDR(te->bestAvailable);
  while (tile.lev > requested_level_)
    tile.upLevel();

  unsigned int texid;
  TextureCache* textureCache = tile.alpha() == 0 ? overlay_RGB_texture_cache_ :
                               overlay_alpha_texture_cache_;
  while ((texid = textureCache->fetchAndPin(tile.addr())) == 0) {
    if (tile.upLevel() == -1)
      return -1;
  }

  // we found one, so let's load it up!
  glBindTexture(GL_TEXTURE_2D, texid);

  return tile.lev;
}

void gstTextureManager::recycleTexture(std::uint64_t addr) {
  TextureCache* texture_cache;
  if (SRCFROMADDR(addr) == 0) {
    texture_cache = base_texture_cache_;
  } else {
    if (SUBFROMADDR(addr) == 0) {
      texture_cache = overlay_RGB_texture_cache_;
    } else {
      texture_cache = overlay_alpha_texture_cache_;
    }
  }

  if (!texture_cache->unpin(addr))
    notify(NFY_WARN, "unpin of textureCache entry for %llu failed",
           static_cast<long long unsigned>(addr));
}

// this is a callback, triggered by the gstCache object
// it will get called when the requested object is not in the cache
bool gstTextureManager::bindTile(unsigned int& reuseID, const std::uint64_t& addr) {
  // has this tile been loaded yet by our read thread?
  unsigned char* buf = memory_cache_->fetchAndPin(addr);

  // Nope, queue it up to be loaded
  if (buf == nullptr) {
    if (requested_level_ == LEVFROMADDR(addr)) {
      readQueue.push(addr);
      render_unfinished_++;
    }
    return false;
  }

  // do we need a new texid, or just re-use an old one?
  unsigned int texid = (reuseID == 0) ? texture_number_++ : reuseID;
  texture_cache_histogram_->add(texid - BASE_IMAGE_ID);


  glBindTexture(GL_TEXTURE_2D, texid);

  gstTextureGuard tex = FindTexture(SRCFROMADDR(addr));
  assert(tex);
  const gstGLTexSpec& glspec = tex->glspec();

  if (reuseID) {
    // base texture handled here (has no alpha)
    if (SRCFROMADDR(addr) == 0) {
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                      glspec.Width(), glspec.Height(),
                      glspec.Format(), glspec.Type(),
                      static_cast<GLvoid*>(buf));

      // overlay RGB
    } else if (SUBFROMADDR(addr) == 0) {
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                      glspec.Width(), glspec.Height(),
                      glspec.Format(), glspec.Type(),
                      static_cast<GLvoid*>(buf));

      // overlay ALPHA
    } else {
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                      glspec.Width(), glspec.Height(),
                      GL_ALPHA, glspec.Type(),
                      static_cast<GLvoid*>(buf));
    }
  } else {
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    if (SRCFROMADDR(addr) == 0) {
      glTexImage2D(GL_TEXTURE_2D, 0, glspec.InternalFormat(),
                   glspec.Width(), glspec.Height(), 0,
                   glspec.Format(), glspec.Type(), static_cast<GLvoid*>(buf));

    } else if (SUBFROMADDR(addr) == 0) {
      glTexImage2D(GL_TEXTURE_2D, 0, glspec.InternalFormat(),
                   glspec.Width(), glspec.Height(), 0,
                   glspec.Format(), glspec.Type(), static_cast<GLvoid*>(buf));
    } else {
      // XXX
      // XXX figure out how to get another glspec for alpha
      // XXX
      glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
                   glspec.Width(), glspec.Height(), 0,
                   GL_ALPHA, glspec.Type(), static_cast<GLvoid*>(buf));
    }
  }

  // unpin once texture is paged since will be requested from
  // cache again when will be paged again
  if (!memory_cache_->unpin(addr))
    notify(NFY_WARN, "unpin of memoryCache entry for %llu failed",
           static_cast<long long unsigned>(addr));

  reuseID = texid;

  return true;
}

void gstTextureManager::readThreadFunc() {
  int idx = 0;
  int qsz = 0;
  unsigned char* next_buffer = nullptr;
  std::uint64_t addr;

  while (true) {
    addr = readQueue.pop();
    if (addr == InvalidTileAddrHash) {
      break;
    }

    khLockGuard guard(readWorkerMutex);
    if (addr && !memory_cache_->fetch(addr)) {
      gstTextureGuard tex = FindTexture(SRCFROMADDR(addr));
      assert(tex);

      if (next_buffer == nullptr) {
        next_buffer = new unsigned char[tex->cellSize()];
        notify(NFY_VERBOSE,
               "(%d) New tex buffer [%d] sz=%d c:%d r:%d l:%d s:%d",
               qsz, idx, tex->cellSize(), COLFROMADDR(addr),
               ROWFROMADDR(addr), LEVFROMADDR(addr), SRCFROMADDR(addr));
        ++idx;
      }

      if (tex->Load(addr, next_buffer) == true)
        next_buffer = memory_cache_->insert(next_buffer, addr);
    }
  }
}


// Find the best texture tile to use for requested address
//
// this is a callback, triggered by the tileExistanceCache
// when the requested tile is no longer in the cache
bool gstTextureManager::findTile(TileExistance* &reuse, const std::uint64_t& addr) {
  if (!reuse)
    reuse = new TileExistance;

  gstTextureGuard tex = FindTexture(SRCFROMADDR(addr));
  assert(tex);

  tex->Find(reuse, addr);
  return true;
}

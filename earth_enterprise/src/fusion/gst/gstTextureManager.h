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

#ifndef KHSRC_FUSION_GST_GSTTEXTUREMANAGER_H__
#define KHSRC_FUSION_GST_GSTTEXTUREMANAGER_H__

#include <semaphore.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <map>
#include <qthread.h>

#include <khThread.h>
#include <gstTypes.h>
#include <gstCache.h>
#include <gstThreadCache.h>
#include <notify.h>
#include <gstCallback.h>
#include <gstQueue.h>
#include <gstLimits.h>
#include <gstFormat.h>
#include <gstFileUtils.h>
#include <image.h>
#include <gstTexture.h>

class gstSource;
class gstHistogram;

#define CHECK_IMAGE_ID 2
#define CHECK_IMAGE_SIZE 64

#define BASE_IMAGE_ID 555

class gstTextureManager;
class ReadThread : public QThread {
  gstTextureManager *texman;
  struct itimerval profile_timer;
public:
  ReadThread(gstTextureManager *texman_) : texman(texman_) {}
  void start();
protected:
  virtual void run(void);
};

// Manage texture resources
// Manage all main memory and texture memory resources for texture usage.

class gstTextureManager {
  friend class ReadThread;
 public:
  gstTextureManager(int memoryCacheSize, int textureCacheSize);
  ~gstTextureManager();
  gstTextureManager(const gstTextureManager&) = delete;
  gstTextureManager(gstTextureManager&&) = delete;
  gstTextureManager& operator=(const gstTextureManager&) = delete;
  gstTextureManager& operator=(gstTextureManager&&) = delete;

  // flush all caches
  void flush();

  // Initialize the texture manager from a valid graphics context
  void glinit();

  gstTextureGuard NewTextureFromPath(const std::string& path,
                                     bool* is_mercator_imagery);
  bool AddBaseTexture(gstTextureGuard texture);
  bool AddOverlayTexture(gstTextureGuard texture);

  gstSource* loadfile(const char *path);

  // a base texture should always have a valid root tile
  // that stays pinned in cache.  worst case for drawing a frame will use
  // some or all of this base texture to complete tiles
  void loadBaseTextureRoot();

  void removeTexture(const gstTextureGuard &toRemove);

  // Bind the default texture
  // void prepareDefaultTexture(double *, double *);
  int prepareBaseTexture(TexTile& tile);
  void GetOverlayList(const TexTile& tile, std::vector<gstTextureGuard>&,
                      const bool is_mercator_preview);
  int prepareTexture(TexTile& tile, const gstTextureGuard &tex);
  void recycleTexture(std::uint64_t addr);

  int renderUnfinished() { return render_unfinished_; }
  void resetFrame();

  gstHistogram* textureCacheHist() { return texture_cache_histogram_; }

  void toggleBaseTexture();

 private:
  typedef gstCache< unsigned int, std::uint64_t > TextureCache;
  TextureCache* base_texture_cache_;
  TextureCache* overlay_RGB_texture_cache_;
  TextureCache* overlay_alpha_texture_cache_;

  static bool bindTile_cb(void *obj, unsigned int &data, const std::uint64_t &id) {
    return reinterpret_cast<gstTextureManager*>(obj)->bindTile(data, id);
  }
  bool bindTile(unsigned int& data, const std::uint64_t& id);

  typedef gstCache<TileExistance*, std::uint64_t> TileExistanceCache;
  TileExistanceCache* tile_existance_cache_;
  static bool findTile_cb(void *obj, TileExistance *&data, const std::uint64_t &id) {
    return reinterpret_cast<gstTextureManager*>(obj)->findTile(data, id);
  }
  bool findTile(TileExistance *&data, const std::uint64_t &id);

  unsigned int getNextTexID() { return ++current_tex_id_; }
  unsigned int getCurrTexID() { return current_tex_id_; }

  gstTextureGuard FindTexture(int id);

  gstTextureGuard base_texture_;

  int render_unfinished_;

  void readThreadFunc();
  ReadThread readThread;
  khMTQueue<std::uint64_t> readQueue;
  khMutex readWorkerMutex;

  int texture_number_;

  gstHistogram* texture_cache_histogram_;

  typedef gstThreadCache<unsigned char*, std::uint64_t> MemoryCache;
  MemoryCache* memory_cache_;

  std::map<unsigned int, gstTextureGuard> texture_map_;
  khMutex textureMapMutex;

  unsigned int current_tex_id_;  // internal ID of texture, always increasing, no re-use

  unsigned int requested_level_;
};

// -----------------------------------------------------------------------------

extern gstTextureManager *theTextureManager;

class gstTEXFormat : public gstFormat {
 public:
  gstTEXFormat(const char *n, bool is_mercator_imagery);
  ~gstTEXFormat();

  virtual gstStatus OpenFile();
  virtual gstStatus CloseFile();
  virtual void setEnabled(bool s);

 private:
  FORWARD_ALL_SEQUENTIAL_ACCESS_TO_BASE;

  virtual gstGeodeHandle GetFeatureImpl(std::uint32_t layer, std::uint32_t id);
  virtual gstRecordHandle GetAttributeImpl(std::uint32_t layer, std::uint32_t id);
  FORWARD_GETFEATUREBOX_TO_BASE;


  gstTextureGuard texture_;
  gstBBox extents_;
};

#endif  // !KHSRC_FUSION_GST_GSTTEXTUREMANAGER_H__

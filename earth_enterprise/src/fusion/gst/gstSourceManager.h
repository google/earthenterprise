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

#ifndef KHSRC_FUSION_GST_GSTSOURCEMANAGER_H__
#define KHSRC_FUSION_GST_GSTSOURCEMANAGER_H__

#include <gstMemory.h>
#include <gstTypes.h>
#include <khHashTable.h>
#include <gstThreadCache.h>
#include <gstFileUtils.h>
#include <gstBBox.h>
#include <khCache.h>
#include <gstGeode.h>
#include <gstGeometry.h>
#include <gstRecord.h>

#include <vector>

class gstSource;
class gstFormat;

// build the geode address as follows:
//   (source:16) + (layer:16) + (feature#:32)
//
#define GEODE_ADDR(src, lyr, ftr) \
    (  ((std::uint64_t)(ftr) & 0xffffffff)        | \
      (((std::uint64_t)(lyr) & 0x0000ffff) << 32) | \
      (((std::uint64_t)(src) & 0x0000ffff) << 48) )

#define ADDR2FTR(addr) int( 0xffffffff & (addr) )
#define ADDR2LYR(addr) int( 0x0000ffff & ((addr) >> 32) )
#define ADDR2SRC(addr) int( 0x0000ffff & ((addr) >> 48) )

class UniqueFeatureId {
 public:
  UniqueFeatureId(int s, std::uint32_t l, std::uint32_t f) :
      source_id(s), layer_num(l), feature_id(f) {}
  UniqueFeatureId(const UniqueFeatureId& b) {
    source_id = b.source_id;
    layer_num = b.layer_num;
    feature_id = b.feature_id;
  }
  bool operator==(const UniqueFeatureId& b) const {
    return source_id == b.source_id &&
           layer_num == b.layer_num &&
          feature_id == b.feature_id;
  }
  int source_id;
  std::uint32_t layer_num;
  std::uint32_t feature_id;
};

// ****************************************************************************
// ***  gstSharedSource
// ****************************************************************************
class gstSharedSourceImpl : public khRefCounter {
  int sourceId;
  gstSource *source;
  std::string path;
  std::string key;


 public:
  gstSharedSourceImpl(const std::string &path_, const std::string &key_);
  ~gstSharedSourceImpl(void);
  int Id(void) const;
  std::uint32_t NumFeatures(std::uint32_t layer) const;
  gstRecordHandle GetAttributeOrThrow(std::uint32_t layer, std::uint32_t id);
  const gstHeaderHandle& GetAttrDefs(std::uint32_t layer) const;
  const char* name(void) const;
  gstFormat* Format(void) const;
  gstSource* GetRawSource(void) const;
};
typedef khRefGuard<gstSharedSourceImpl> gstSharedSource;


// ****************************************************************************
// ***  gstSourceManager
// ****************************************************************************
class gstSourceManager : public gstMemory {
 public:
  // Since all vector-data cannot be kept in-memory and since there is a
  // localization of usage, we cache only a subset of vector-data. The size of
  // the cache depends on the following independent variables.
  static const unsigned int kGeodesCacheSize;
  static const unsigned int kRecordCacheSize;

  static void init(int);

  inline int AddSource(gstSource* s) {
    khLockGuard guard(mutex);
    return PrivateAddSource(s);
  }
  inline void RemoveSource(int id) {
    khLockGuard guard(mutex);
    return PrivateRemoveSource(id);
  }

  inline gstGeodeHandle GetFeatureOrThrow(const UniqueFeatureId& ufid,
                                          bool is_mercator_preview) {
    khLockGuard guard(mutex);
    return PrivateGetFeatureOrThrow(ufid, is_mercator_preview);
  }
  inline gstGeometryHandle GetGeometryOrThrow(const UniqueFeatureId& ufid,
                                              bool is_mercator_preview) {
    khLockGuard guard(mutex);
    return PrivateGetGeometryOrThrow(ufid, is_mercator_preview);
  }
  inline gstBBox GetFeatureBoxOrThrow(const UniqueFeatureId& ufid) {
    khLockGuard guard(mutex);
    return PrivateGetFeatureBoxOrThrow(ufid);
  }
  inline gstRecordHandle GetAttributeOrThrow(const UniqueFeatureId& ufid) {
    khLockGuard guard(mutex);
    return PrivateGetAttributeOrThrow(ufid);
  }


  inline gstSource* GetSource(int id) {
    khLockGuard guard(mutex);
    return PrivateGetSource(id);
  }

  // will catchown exception and return an empty handle
  gstSharedSource TryGetSharedAssetSource(const std::string &assetRef);
  gstSharedSource GetSharedAssetSource(const std::string &assetRef);

  // will catchown exception and return an empty handle
  gstSharedSource TryGetSharedSource(const std::string &path);
  gstSharedSource GetSharedSource(const std::string &path);

  // used only by ~gstSharedSource
  void RemoveSharedSource(int sourceId, const std::string &path);
 private:

  int PrivateAddSource(gstSource* s);
  void PrivateRemoveSource(int id);

  gstGeodeHandle PrivateGetFeatureOrThrow(const UniqueFeatureId& ufid,
                                          bool is_mercator_preview);
  gstGeometryHandle PrivateGetGeometryOrThrow(const UniqueFeatureId& ufid,
                                              bool is_mercator_preview);
  gstBBox PrivateGetFeatureBoxOrThrow(const UniqueFeatureId& ufid);
  gstRecordHandle PrivateGetAttributeOrThrow(const UniqueFeatureId& ufid);

  gstSource* PrivateGetSource(int id);

 private:
  khCache<std::uint64_t, gstGeodeHandle> geode_cache_;
  khCache<std::uint64_t, gstGeometryHandle> geometry_cache_;
  khCache<std::uint64_t, gstRecordHandle> record_cache_;

  khMutex mutex;
  std::vector<gstSource*> source_list_;

  std::map<std::string, gstSharedSourceImpl*> sharedSources;

  gstSourceManager(int);
  virtual ~gstSourceManager();
};

extern gstSourceManager *theSourceManager;




#endif  // !KHSRC_FUSION_GST_GSTSOURCEMANAGER_H__

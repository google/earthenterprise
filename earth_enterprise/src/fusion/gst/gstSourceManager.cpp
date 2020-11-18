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


#include <gstSourceManager.h>
#include <gstSource.h>
#include <gstFormat.h>
#include <khArray.h>
#include <khException.h>
#include <gstJobStats.h>
#include <autoingest/Asset.h>
#include <autoingest/AssetVersion.h>

#define MAX_OPEN_FILES 10

// Expect 10000 Geodes to consume maximum cache size of 5MB (avg 500 Byte).
const unsigned int gstSourceManager::kGeodesCacheSize = 10000;
// Record caches don't consume much and doesn't change much.
const unsigned int gstSourceManager::kRecordCacheSize = 20000;
gstSourceManager* theSourceManager = NULL;

void gstSourceManager::init(int sz) {
  assert(theSourceManager == NULL);
  theSourceManager = new gstSourceManager(sz);
}

gstSourceManager::gstSourceManager(int cacheSize)
    : geode_cache_(gstSourceManager::kGeodesCacheSize, "GST geode"),
      geometry_cache_(cacheSize, "GST geometry"),
      record_cache_(gstSourceManager::kRecordCacheSize, "GST record") {
}

gstSourceManager::~gstSourceManager() {
  for (std::vector<gstSource*>::iterator it = source_list_.begin();
       it != source_list_.end(); ++it) {
    if ((*it) != NULL)
      (*it)->unref();
  }
}

int gstSourceManager::PrivateAddSource(gstSource* src) {
  source_list_.push_back(src);
  src->ref();
  return source_list_.size() - 1;
}

void gstSourceManager::PrivateRemoveSource(int id) {
  gstSource* src = source_list_[id];

  assert(src != NULL);

  if (src->getRef() == 1) {
    source_list_[id] = NULL;

    geode_cache_.clear();
    geometry_cache_.clear();
    record_cache_.clear();
  }

  src->unref();
}

gstSource* gstSourceManager::PrivateGetSource(int id) {
  gstSource* src = source_list_[id];

  assert(src != NULL);

  if (!src->Opened()) {
    if (src->Open() != GST_OKAY) {
      return NULL;
    }
  }

  return src;
}

gstGeometryHandle gstSourceManager::PrivateGetGeometryOrThrow(
    const UniqueFeatureId& ufid, bool is_mercator_preview) {
  std::uint64_t addr = GEODE_ADDR(ufid.source_id, ufid.layer_num, ufid.feature_id);

  gstGeometryHandle geom;
  if (!geometry_cache_.Find(addr, geom)) {
    gstGeodeHandle geode = PrivateGetFeatureOrThrow(ufid, is_mercator_preview);
    geom = gstGeometryImpl::Create(geode);
    geometry_cache_.Add(addr, geom);
  }

  return geom;
}

#ifdef JOBSTATS_ENABLED
enum {JOBSTATS_GETFEATURE, JOBSTATS_GETBOX, JOBSTATS_GETATTR,
      JOBSTATS_HITCACHE, JOBSTATS_PULL, JOBSTATS_FREE};
static gstJobStats::JobName JobNames[] = {
  {JOBSTATS_GETFEATURE, "Get Feature     "},
  {JOBSTATS_GETBOX,     "Get Feature Box "},
  {JOBSTATS_GETATTR,    "Get Attribute   "},
  {JOBSTATS_HITCACHE,   "+ Hit Cache     "},
  {JOBSTATS_PULL,       "+ Pull from src "},
  {JOBSTATS_FREE,       "+ Free Previous "}
};
gstJobStats* get_stats = new gstJobStats("SRC MGR", JobNames, 6);
#endif

gstGeodeHandle gstSourceManager::PrivateGetFeatureOrThrow(
    const UniqueFeatureId& ufid, bool is_mercator_preview) {
  JOBSTATS_SCOPED(get_stats, JOBSTATS_GETFEATURE);
  std::uint64_t addr = GEODE_ADDR(ufid.source_id, ufid.layer_num, ufid.feature_id);

  gstGeodeHandle geode;
  if (!geode_cache_.Find(addr, geode)) {
    gstSource* src = PrivateGetSource(ufid.source_id);
    if (!src) {
      throw khException(kh::tr("Unable to find source with id %1")
                        .arg(ufid.source_id));
    }

    geode = src->GetFeatureOrThrow(ufid.layer_num, ufid.feature_id,
                                   is_mercator_preview);
    geode_cache_.Add(addr, geode);
  }
  return geode;
}

gstRecordHandle gstSourceManager::PrivateGetAttributeOrThrow(
    const UniqueFeatureId& ufid) {
  static gstRecordHandle last_record;
  static UniqueFeatureId last_ufid(-1, 0, 0);
  JOBSTATS_SCOPED(get_stats, JOBSTATS_GETATTR);

  if (ufid == last_ufid) {
    JOBSTATS_BEGIN(get_stats, JOBSTATS_HITCACHE);
    JOBSTATS_END(get_stats, JOBSTATS_HITCACHE);
    return last_record;
  }

  gstRecordHandle record;
  {
    JOBSTATS_SCOPED(get_stats, JOBSTATS_PULL);
    gstSource* src = PrivateGetSource(ufid.source_id);
    if (!src) {
      throw khException(kh::tr("Unable to find source with id %1")
                        .arg(ufid.source_id));
    }
    record = src->GetAttributeOrThrow(ufid.layer_num, ufid.feature_id);
  }

  JOBSTATS_BEGIN(get_stats, JOBSTATS_FREE);
  last_ufid = ufid;
  last_record = record;
  JOBSTATS_END(get_stats, JOBSTATS_FREE);

  return record;
}

gstBBox gstSourceManager::PrivateGetFeatureBoxOrThrow(
    const UniqueFeatureId& ufid) {
  JOBSTATS_SCOPED(get_stats, JOBSTATS_GETBOX);
  gstSource* src = PrivateGetSource(ufid.source_id);
  if (!src) {
    throw khException(kh::tr("Unable to find source with id %1")
                      .arg(ufid.source_id));
  }
  return src->GetFeatureBoxOrThrow(ufid.layer_num, ufid.feature_id);
}


gstSharedSource gstSourceManager::GetSharedAssetSource(
    const std::string &assetRef) {
  gstSharedSource found =
    AnotherRefGuardFromRaw(sharedSources[assetRef]);
  if (!found) {
    Asset asset(assetRef);
    if (!asset) {
      throw khException(kh::tr("No such asset: '%1'").arg(assetRef.c_str()));
    }
    AssetVersion ver(asset->GetLastGoodVersionRef());
    std::string productName;
    if (ver) {
      productName = ver->GetOutputFilename(0);
    } else {
      throw khException(kh::tr("Asset '%1' doesn't have a good version")
                        .arg(assetRef.c_str()));
    }

    found = khRefGuardFromNew(new gstSharedSourceImpl(productName, assetRef));
    // keep a handle to the raw, we don't want our map to keep it busy
    sharedSources[assetRef] = &*found;
  }
  return found;
}

gstSharedSource gstSourceManager::TryGetSharedAssetSource(
    const std::string &assetRef) {
  try {
    return GetSharedAssetSource(assetRef);
  } catch (const std::exception &e) {
    notify(NFY_WARN, "%s", e.what());
  } catch (...) {
  }
  return gstSharedSource();
}

gstSharedSource gstSourceManager::GetSharedSource(const std::string &path) {
  gstSharedSource found =
    AnotherRefGuardFromRaw(sharedSources[path]);
  if (!found) {
    found = khRefGuardFromNew(new gstSharedSourceImpl(path, path));
    // keep a handle to the raw, we don't want our map to keep it busy
    sharedSources[path] = &*found;
  }
  return found;
}

gstSharedSource gstSourceManager::TryGetSharedSource(const std::string &path) {
  try {
    return GetSharedSource(path);
  } catch (const std::exception &e) {
    notify(NFY_WARN, "%s", e.what());
  } catch (...) {
  }
  return gstSharedSource();
}

void gstSourceManager::RemoveSharedSource(int sourceId,
                                          const std::string &path) {
  sharedSources.erase(path);
  RemoveSource(sourceId);
}



// ****************************************************************************
// ***  gstSharedSourceImpl
// ****************************************************************************
gstSharedSourceImpl::gstSharedSourceImpl(const std::string &path_,
                                         const std::string &key_) :
    sourceId(), source(), path(path_), key(key_)
{
  source = new gstSource(path.c_str());
  if (source->Open() != GST_OKAY) {
    throw khException(kh::tr("Unable to open '%1'").arg(path.c_str()));
  }
  sourceId = theSourceManager->AddSource(source);
  source->unref(); // now owned by source manager
}
gstSharedSourceImpl::~gstSharedSourceImpl(void) {
  theSourceManager->RemoveSharedSource(sourceId, key);
}
int gstSharedSourceImpl::Id(void) const { return sourceId; }
 std::uint32_t gstSharedSourceImpl::NumFeatures(std::uint32_t layer) const {
  return source->NumFeatures(layer);
}
gstRecordHandle gstSharedSourceImpl::GetAttributeOrThrow(std::uint32_t layer,
                                                         std::uint32_t id) {
  return source->GetAttributeOrThrow(layer, id);
}
const gstHeaderHandle&
gstSharedSourceImpl::GetAttrDefs(std::uint32_t layer) const {
  return source->GetAttrDefs(layer);
}
const char* gstSharedSourceImpl::name(void) const {
  return source->name();
}
gstFormat* gstSharedSourceImpl::Format(void) const {
  return source->Format();
}
gstSource* gstSharedSourceImpl::GetRawSource(void) const {
  return source;
}

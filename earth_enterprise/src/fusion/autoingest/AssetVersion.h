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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_ASSETVERSION_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_ASSETVERSION_H_

#include <algorithm>
#include "fusion/autoingest/AssetVersionRef.h"
#include "fusion/autoingest/.idl/AssetStorage.h"
#include "fusion/autoingest/AssetHandle.h"
#include "fusion/autoingest/MiscConfig.h"
#include "common/khFileUtils.h"

/******************************************************************************
 ***  AssetVersionImpl
 ***
 ***  Implementation for the AssetVersion base class.
 ***
 ***  Code should NOT use this (or any other *Impl) class directly. Instead
 ***  it should use the AssetVersion and derived AssetVersion
 ***  (RasterBlendAssetVersion, MosaicAssetVersion) handle classes.
 ***
 ***  AssetVersion ver = asset->CurrVersionRef();
 ***  if (ver) {
 ***     ... = ver->config.layers.size();
 ***  }
 ******************************************************************************/
class AssetVersionImpl : public khRefCounter, public AssetVersionStorage {
  friend class AssetImpl;
  friend class AssetHandle_<AssetVersionImpl>;

  // private and unimplemented -- illegal to copy an AssetVersionImpl
  AssetVersionImpl(const AssetVersionImpl&);
  AssetVersionImpl& operator=(const AssetVersionImpl&);

 protected:
  // implemented in LoadAny.cpp
  static khRefGuard<AssetVersionImpl> Load(const std::string &boundref);

  std::string XMLFilename() const { return XMLFilename(GetRef()); }
  std::string WorkingDir(void) const { return WorkingDir(GetRef()); }
  std::string WorkingFileRef(const std::string &fname) const {
    return WorkingDir() + fname;
  }

 public:
  std::string WorkingFilename(const std::string &fname) const {
    return AssetDefs::AssetPathToFilename(WorkingFileRef(fname));
  }

 protected:
  // used by my intermediate derived classes since their calls to
  // my constructor will never actualy be used
  AssetVersionImpl(void) : timestamp(0), filesize(0) { }

  AssetVersionImpl(const AssetVersionStorage &storage)
      : AssetVersionStorage(storage), timestamp(0), filesize(0) { }

 public:
  // use by cache - maintained outside of constructors
  time_t timestamp;
  uint64 filesize;

  virtual std::string PluginName(void) const = 0;
  virtual ~AssetVersionImpl(void) { }
  virtual bool IsLeaf(void) const { return false; }
  virtual std::string Logfile(void) const {
    std::string logfile = LogFilename(GetRef());
    return (!AssetDefs::Finished(state) || khExists(logfile)) ?
      logfile : std::string();
  }
  static std::string LogFilename(const AssetVersionRef &ref) {
    return AssetDefs::AssetPathToFilename(WorkingDir(ref) + "logfile");
  }

  bool CanRebuild(void) const {
    return ((state & (AssetDefs::Failed | AssetDefs::Canceled)) &&
            (subtype != "Source"));
  }
  bool CanCancel(void) const {
    return ((state & (AssetDefs::Waiting | AssetDefs::Queued |
                      AssetDefs::InProgress | AssetDefs::Blocked)) &&
            (subtype != "Source"));
  }
  bool CanClean(void) const {
    return (AssetDefs::CanOffline(state) && (subtype != "Source"));
  }
  bool CanSetBad(void) const {
    return (state == AssetDefs::Succeeded);
  }
  bool CanClearBad(void) const {
    return (state == AssetDefs::Bad);
  }

  std::string GetRef(void) const { return name; }
  std::string GetAssetRef(void) const {
    AssetVersionRef verref(name);
    return verref.AssetRef();
  }

  template <class outIter>
  outIter GetInputs(outIter oi) const {
    for (std::vector<std::string>::const_iterator i = inputs.begin();
         i != inputs.end(); ++i) {
      oi++ = *i;
    }
    return oi;
  }

  void GetInputFilenames(std::vector<std::string> &out) const;
  virtual void GetOutputFilenames(std::vector<std::string> &out) const = 0;
  virtual std::string GetOutputFilename(uint i) const = 0;
  virtual void AfterLoad(void) { }

  // static helpers
  static std::string WorkingDir(const AssetVersionRef &ref);
  static std::string XMLFilename(const AssetVersionRef &ref) {
    return AssetDefs::AssetPathToFilename(WorkingDir(ref) +
                                          "khassetver.xml");
  }

  // Gets the database path, type and ref string for the given dbname.
  // This is useful for distinguishing specific types of databases and
  // dealing with short db names.
  // dbname: input database name. May be:
  //   1. short without version (e.g Databases/ExampleMap_Merc)
  //   2. short with version    (e.g Databases/ExampleMap_Merc?version=7)
  //   3. long (e.g /gevol/assets/Databases/
  //                ExampleMap_Merc.kmmdatabase/mapdb.kda/ver007/mapdb)
  // gedb_path: the output path of the gedb directory for the database.
  // db_type: the output string type of the db. One of:
  //          kMapDatabaseSubtype, kMercatorMapDatabaseSubtype, or
  //          kDatabaseSubtype
  // db_ref: the output asset version ref to the database
  // Returns whether it succeeded.
  // Warning: Use this only for Fusion side code and not for Server side code,
  //          because this depends on ASSET_ROOT which is set only on Fusion.
  static bool SimpleGetGedbPathAndType(const std::string& dbname,
                                       std::string* gedb_path,
                                       std::string* db_type,
                                       std::string* db_ref);

  static bool SimpleGetGedbPath(const std::string& dbname,
                                std::string* gedb_path) {
    std::string db_type;
    if (khIsAbspath(dbname)) {
      *gedb_path = khNormalizeDir(dbname, false);
      return khIsAbsoluteDbName(*gedb_path, &db_type);
    }
    std::string db_ref;
    return SimpleGetGedbPathAndType(dbname, gedb_path, &db_type, &db_ref);
  }

  // Like above but does additional checks to make sure db is suitable for
  // publishing. (e.g. it's not an old/unsupported format)
  static bool GetGedbPathAndType(const std::string& dbname,
                                 std::string* gedb_path,
                                 std::string* db_type,
                                 std::string* db_ref);

  static bool GetGedbPath(const std::string& dbname, std::string* gedb_path) {
    std::string db_type;
    if (khIsAbspath(dbname)) {
      *gedb_path = khNormalizeDir(dbname, false);
      return khIsAbsoluteDbName(*gedb_path, &db_type);
    }
    std::string db_ref;
    return GetGedbPathAndType(dbname, gedb_path, &db_type, &db_ref);
  }
};


// ****************************************************************************
// ***  AssetVersion & its specializations
// ****************************************************************************
typedef AssetHandle_<AssetVersionImpl> AssetVersion;


template <>
inline khCache<std::string, AssetVersion::HandleType>&
AssetVersion::cache(void) {
  static khCache<std::string, AssetVersion::HandleType>
    instance(MiscConfig::Instance().VersionCacheSize);
  return instance;
}

// DoBind() specialization with caching Version.
template <> template <>
inline void AssetVersion::DoBind<1>(const std::string &boundref,
                                    const AssetVersionRef &boundVerRef,
                                    bool checkFileExistenceFirst,
                                    Int2Type<1>) const {
  // Check in cache.
  HandleType entry = CacheFind(boundref);
  bool addToCache = false;

  // Try to load from XML.
  if (!entry) {
    if (checkFileExistenceFirst) {
      std::string filename = Impl::XMLFilename(boundVerRef);
      if (!khExists(Impl::XMLFilename(boundVerRef))) {
        // In this case DoBind is allowed not to throw even if
        // we configured to normally throw.
        return;
      }
    }

    // Will succeed, generate stub, or throw exception.
    entry = Load(boundref);
    addToCache = true;
  } else if (check_timestamps) {
    std::string filename = Impl::XMLFilename(boundVerRef);
    uint64 filesize = 0;
    time_t timestamp = 0;
    if (khGetFileInfo(filename, filesize, timestamp) &&
        ((timestamp != entry->timestamp) ||
         (filesize != entry->filesize))) {
      // The file has changed on disk.

      // Drop the current entry from the cache.
      cache().Remove(boundref, false);  // Don't prune, the Add() will.

      // Will succeed, generate stub, or throw exception.
      entry = Load(boundref);
      addToCache = true;
    }
  }

  // Set my handle.
  handle = entry;

  // Add it to the cache.
  if (addToCache)
    cache().Add(boundref, entry);

  // Used by derived class to mark dirty.
  OnBind(boundref);
}

// DoBind() specialization without caching Version.
template <> template <>
inline void AssetVersion::DoBind<0>(const std::string &boundref,
                        const AssetVersionRef &boundVerRef,
                        bool checkFileExistenceFirst,
                        Int2Type<0>) const {
  // Check in cache.
  HandleType entry = CacheFind(boundref);

  // Try to load from XML.
  if (!entry) {
    if (checkFileExistenceFirst) {
      std::string filename = Impl::XMLFilename(boundVerRef);
      if (!khExists(Impl::XMLFilename(boundVerRef))) {
        // In this case DoBind is allowed not to throw even if
        // we configured to normally throw.
        return;
      }
    }

    // Will succeed, generate stub, or throw exception.
    entry = Load(boundref);
  } else if (check_timestamps) {
    std::string filename = Impl::XMLFilename(boundVerRef);
    uint64 filesize = 0;
    time_t timestamp = 0;
    if (khGetFileInfo(filename, filesize, timestamp) &&
        ((timestamp != entry->timestamp) ||
         (filesize != entry->filesize))) {
      // The file has changed on disk.

      // Drop the current entry from the cache.
      cache().Remove(boundref, false);  // Don't prune, the Add() will.

      // Will succeed, generate stub, or throw exception.
      entry = Load(boundref);
    }
  }

  // Set my handle.
  handle = entry;

  // Used by derived class to mark dirty.
  OnBind(boundref);
}


template <>
inline void AssetVersion::DoBind(const std::string &boundRef,
                     const AssetVersionRef &boundVerRef,
                     bool checkFileExistenceFirst) const {
  DoBind(boundRef, boundVerRef, checkFileExistenceFirst, Int2Type<1>());
}


template <>
inline bool AssetVersion::Valid(void) const {
  if (handle) {
    return handle->type != AssetDefs::Invalid;
  } else {
    // deal quickly with an empty ref
    if (ref.empty())
      return false;

    // bind the ref
    std::string boundRef = AssetVersionRef::Bind(ref);
    AssetVersionRef boundVerRef(boundRef);

    // deal quickly with an invalid version
    if (boundVerRef.Version() == "0")
      return false;

    try {
      DoBind(boundRef, boundVerRef, true /* check file & maybe not throw */);
    } catch (...) {
      return false;
    }
    return handle && (handle->type != AssetDefs::Invalid);
  }
}


template <>
inline void AssetVersion::Bind(void) const {
  if (!handle) {
    std::string boundref = AssetVersionRef::Bind(ref);
    AssetVersionRef boundVerRef(boundref);
    DoBind(boundref, boundVerRef, false);
  }
}


template <>
inline void AssetVersion::BindNoCache() const {
  if (!handle) {
    std::string boundref = AssetVersionRef::Bind(ref);
    AssetVersionRef boundVerRef(boundref);
    DoBind(boundref, boundVerRef, false, Int2Type<false>());
  }
}



// ****************************************************************************
// ***  LeafAssetVersionImpl
// ****************************************************************************
class LeafAssetVersionImpl : public virtual AssetVersionImpl {
 protected:
  // Since I inherit virtually from AssetVersionImpl, all of my derived
  // classes will need to initialize it directly
  // This means that there's nothing for me to do
  LeafAssetVersionImpl(void) { }

 public:
  virtual bool IsLeaf(void) const { return true; }

  virtual void GetOutputFilenames(std::vector<std::string> &out) const {
    for (std::vector<std::string>::const_iterator o = outfiles.begin();
         o != outfiles.end(); ++o) {
      out.push_back(AssetDefs::AssetPathToFilename(*o));
    }
  }
  std::string GetOutputFilename(uint i) const {
    if (i < outfiles.size()) {
      return AssetDefs::AssetPathToFilename(outfiles[i]);
    } else {
      return std::string();
    }
  }
};


// ****************************************************************************
// ***  CompositeAssetVersionImpl
// ****************************************************************************
class CompositeAssetVersionImpl : public virtual AssetVersionImpl {
 protected:
  // Since I inherit virtually from AssetVersionImpl, all of my derived
  // classes will need to initialize it directly
  // This means that there's nothing for me to do
  CompositeAssetVersionImpl(void) { }

 public:
  virtual std::string Logfile(void) const {
    // composite assets can only have a logfile if the plugin threw an
    // exception during the DelayedBuildChildren. So don't bother
    // stating the filesystem to look for a logfile unless we're failed.
    std::string logfile = LogFilename(GetRef());
    return ((state == AssetDefs::Failed) && khExists(logfile)) ?
                    logfile : std::string();
  }

  virtual void GetOutputFilenames(std::vector<std::string> &out) const;
  std::string GetOutputFilename(uint i) const;
};


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_ASSETVERSION_H_

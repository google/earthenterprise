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
#include "StorageManager.h"

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
class AssetVersionImpl : public khRefCounter, public AssetVersionStorage, public StorageManaged {
  friend class AssetImpl;
  friend class AssetHandle_<AssetVersionImpl>;

  // Illegal to copy an AssetVersionImpl
  AssetVersionImpl(const AssetVersionImpl&) = delete;
  AssetVersionImpl& operator=(const AssetVersionImpl&) = delete;
  AssetVersionImpl(const AssetVersionImpl&&) = delete;
  AssetVersionImpl& operator=(const AssetVersionImpl&&) = delete;

 public:
  std::string XMLFilename() const { return XMLFilename(GetRef()); }
  std::string WorkingDir(void) const { return WorkingDir(GetRef()); }
  std::string WorkingFileRef(const std::string &fname) const {
    return WorkingDir() + fname;
  }

  // implemented in LoadAny.cpp
  static khRefGuard<AssetVersionImpl> Load(const std::string &boundref);

  virtual bool Save(const std::string &filename) const {
    assert(false); // Can only save from sub-classes
    return false;
  };

  std::string WorkingFilename(const std::string &fname) const {
    return AssetDefs::AssetPathToFilename(WorkingFileRef(fname));
  }

 protected:
  // used by my intermediate derived classes since their calls to
  // my constructor will never actualy be used
  AssetVersionImpl(void) = default;

  AssetVersionImpl(const AssetVersionStorage &storage)
      : AssetVersionStorage(storage) { }

 public:
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

  const SharedString & GetRef(void) const { return name; }
  std::string GetAssetRef(void) const {
    AssetVersionRef verref(name);
    return verref.AssetRef();
  }

  template <class outIter>
  outIter GetInputs(outIter oi) const {
    for (const auto &i : inputs) {
      oi++ = i;
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
inline StorageManager<AssetVersionImpl>&
AssetVersion::storageManager(void)
{
  static StorageManager<AssetVersionImpl> storageManager(
      MiscConfig::Instance().VersionCacheSize, "version");
  return storageManager;
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
    SharedString boundRef = AssetVersionRef::Bind(ref);
    AssetVersionRef boundVerRef(boundRef);

    // deal quickly with an invalid version
    if (boundVerRef.Version() == "0")
      return false;

    try {
      DoBind(true /* check file & maybe not throw */, true /* add to cache */);
    } catch (...) {
      return false;
    }
    return handle && (handle->type != AssetDefs::Invalid);
  }
}

template <>
inline std::string AssetVersion::Filename() const {
  std::string boundref = AssetVersionRef::Bind(ref);
  AssetVersionRef boundVerRef(boundref);
  return AssetVersionImpl::XMLFilename(boundVerRef);
}

template <>
inline const SharedString AssetVersion::Key() const {
  return AssetVersionRef::Bind(ref);
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

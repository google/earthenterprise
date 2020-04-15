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

#include "fusion/autoingest/AssetVersion.h"
#include <sstream>
#include <iomanip>
#include "fusion/autoingest/Asset.h"
#include "fusion/autoingest/plugins/DatabaseAsset.h"
#include "fusion/autoingest/plugins/MapDatabaseAsset.h"
#include "fusion/autoingest/plugins/MercatorMapDatabaseAsset.h"
#include "fusion/autoingest/plugins/GEDBAsset.h"

// ****************************************************************************
// ***  AssetVersionImpl
// ****************************************************************************
std::string
AssetVersionImpl::WorkingDir(const AssetVersionRef &ref)
{
  AssetVersionRef boundref = ref.Bind();

  // this routine assumes a bound version versionRef if it is not bound
  // (i.e no version= or version=current) the resulting workingDir will
  // point to '<path>/ver000/' which shouldn't exists
  unsigned int vernum = 0;
  FromString(boundref.Version(), vernum);
  std::ostringstream out;
  out << "ver" << std::setw(3) << std::setfill('0') << vernum;
  return AssetImpl::WorkingDir(boundref.AssetRef())+out.str()+"/";
}


void
AssetVersionImpl::GetInputFilenames(std::vector<std::string> &out) const
{
  for (const auto &i : inputs) {
    AssetVersion(i)->GetOutputFilenames(out);
  }
}



// ****************************************************************************
// ***  CompositeAssetVersionImpl
// ****************************************************************************
void
CompositeAssetVersionImpl::GetOutputFilenames(std::vector<std::string> &out) const
{
  // We only have outputs when we are succeeded, bad or offline
  if (state & (AssetDefs::Succeeded | AssetDefs::Bad | AssetDefs::Offline)) {
    for (const auto &c : children) {
      AssetVersion(c)->GetOutputFilenames(out);
    }
  }
}


std::string
CompositeAssetVersionImpl::GetOutputFilename(unsigned int i) const
{
  // We only have outputs when we are succeeded, bad or offline
  if (state & (AssetDefs::Succeeded | AssetDefs::Bad | AssetDefs::Offline)) {
    std::vector<std::string> outfiles;
    GetOutputFilenames(outfiles);
    if (i < outfiles.size()) {
      return outfiles[i];
    }
  }
  return std::string();
}

namespace {
// Construct a versioned asset path from a full gedb/mapdb path.
// dbname: full name of the form:
//         /gevol/assets/db/test.kdatabase/gedb.kda/ver074/gedb
//         /gevol/assets/db/test.kdatabase/mapdb.kda/ver074/mapdb
// return: the asset path of the form:
//         db/test.kdatabase?version=XXX
//         If cannot parse return string as it is.
std::string DbFilenameToAssetPath(const std::string& dbname) {
  std::string buff = khNormalizeDir(dbname, false);
  const std::string last_token = khBasename(buff);
  if (last_token != kGedbBase && last_token != kMapdbBase) {
    return dbname;
  }
  buff = khDirname(buff);
  const std::string version_token = khBasename(buff);
  if (!StartsWith(version_token, "ver") || version_token.length() != 6) {
    return dbname;
  }
  buff = khDirname(buff);
  const std::string key_word = khBasename(buff);
  if (key_word != (last_token + kDbKeySuffix)) {
    return dbname;
  }
  const std::string& asset_root = AssetDefs::AssetRoot();
  if (!StartsWith(buff, asset_root)) {
    return dbname;
  }
  buff = khDirname(buff);
  return buff.substr(asset_root.length() + 1) +
      "?version=" + version_token.substr(3);
}
}  // end namespace

bool AssetVersionImpl::SimpleGetGedbPathAndType(
    const std::string& dbname,
    std::string* gedb_path,
    std::string* db_type,
    std::string* db_ref) {
  // The dbname may be of the form.
  // db/test.kdatabase
  // db/test.kdatabase?version=74
  // /gevol/assets/db/test.kdatabase/gedb.kda/ver074/gedb
  AssetVersion db_asset_version;
  if (khIsAbspath(dbname)) {
    // Absolute path is full name
    std::string asset_name = DbFilenameToAssetPath(dbname);
    db_asset_version = asset_name;
  } else {
    // Otherwise, must extract the ref from the relative name.
    db_asset_version =
      AssetVersion(AssetDefs::GuessAssetVersionRef(dbname, std::string()));
  }
  if (!db_asset_version.Valid()) {
    notify(NFY_WARN, "Unable to load %s record", dbname.c_str());
    return false;
  }

  if (db_asset_version->state != AssetDefs::Succeeded) {
    notify(NFY_WARN, "Database %s has not been successfully built.",
           dbname.c_str());
    return false;
  }

  if (db_asset_version->type != AssetDefs::Database) {
    notify(NFY_WARN, "%s is not a database version.", dbname.c_str());
    return false;
  }

  *db_type = db_asset_version->subtype;
  *db_ref = db_asset_version->GetRef();

  AssetVersion gedb_version;
  if (*db_type == kMapDatabaseSubtype) {
    MapDatabaseAssetVersion mdav(db_asset_version);
    gedb_version = mdav->GetMapdbChild();
  } else if (*db_type == kMercatorMapDatabaseSubtype) {
    MercatorMapDatabaseAssetVersion mdav(db_asset_version);
    gedb_version = mdav->GetMapdbChild();
  } else {
    DatabaseAssetVersion dav(db_asset_version);
    gedb_version = dav->GetGedbChild();
  }

  if (!gedb_version) {
    notify(NFY_WARN, "Invalid GEDB version.");
    return false;
  }

  *gedb_path = gedb_version->GetOutputFilename(0);
  return true;
}


bool AssetVersionImpl::GetGedbPathAndType(
    const std::string& dbname,
    std::string* gedb_path,
    std::string* db_type,
    std::string* db_ref) {

  if (!SimpleGetGedbPathAndType(dbname, gedb_path, db_type, db_ref)) {
    return false;
  }

  if (*db_type == kDatabaseSubtype) {
    DatabaseAssetVersion dav(*db_ref);
    GEDBAssetVersion gedb_version(dav->GetGedbChild());
    // make sure that the gedb child has the newest config version
    if (!GEDBAssetVersion(gedb_version)->config._IsCurrentIdlVersion()) {
      notify(NFY_WARN, "Invalid Database version: Old Format");
      return false;
    }
  }

  return true;
}

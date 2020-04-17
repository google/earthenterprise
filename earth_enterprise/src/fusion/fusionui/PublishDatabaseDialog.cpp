// Copyright 2017 Google Inc.
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

#include "fusion/fusionui/PublishDatabaseDialog.h"
#include <qcombobox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include "fusion/autoingest/AssetVersion.h"
#include "fusion/autoingest/plugins/DatabaseAsset.h"
#include "fusion/autoingest/plugins/MapDatabaseAsset.h"
#include "fusion/autoingest/plugins/MercatorMapDatabaseAsset.h"
#include "fusion/autoingest/plugins/GEDBAsset.h"
#include "fusion/gst/gstAssetGroup.h"
#include "fusion/fusionui/Preferences.h"

PublishDatabaseDialog::PublishDatabaseDialog(
    QWidget* parent,
    const Asset& asset,
    const std::vector<QString>& nicknames)
    : PublishDatabaseDialogBase(parent, 0, false, 0) {

  std::string database_name = shortAssetName(asset->GetRef().toString().c_str());
  db_name_label->setText(database_name.c_str());

  std::vector<QString>::const_iterator nickname = nicknames.begin();
  for (; nickname != nicknames.end(); ++nickname) {
    nickname_combo->insertItem(*nickname);
  }

  AssetStorage::VersionList::const_iterator version = asset->versions.begin();
  for (; version != asset->versions.end(); ++version) {
    AssetVersion asset_version(*version);
    // If this version was not built successfully then we don't even list it.
    if (asset_version->state != AssetDefs::Succeeded)
      continue;
    if (asset_version->subtype == kMapDatabaseSubtype) {
      MapDatabaseAssetVersion mdav(asset_version);
      if (!mdav->GetMapdbChild())
        continue;
    } else if (asset_version->subtype == kMercatorMapDatabaseSubtype) {
      MercatorMapDatabaseAssetVersion mdav(asset_version);
      if (!mdav->GetMapdbChild())
        continue;
    } else {
      DatabaseAssetVersion dav(asset_version);
      // Make sure it's a new database with a gedb child
      GEDBAssetVersion gedb(dav->GetGedbChild());
      if (!gedb)
        continue;
      // Make sure that the gedb child has the newest config version
      if (!gedb->config._IsCurrentIdlVersion()) {
        continue;
      }
    }

    valid_versions_.push_back(*version);
    QString item = QString("%1:").arg(asset_version->version);
    item += "    " + asset_version->meta.GetValue("createdtime");
    item += "  " + QString(asset_version->PrettyState().c_str());
    version_combo->insertItem(item);
  }

  // Find the previously used published server for the specific database.
  std::string previous_server_name =
    Preferences::PublishServerName(database_name);

  int current_server_index = 0;
  for (int i = 0; i < static_cast<int>(nicknames.size()); ++i) {
    if (previous_server_name == nicknames[i].ascii()) {
      current_server_index = i;
      break;
    }
  }
  nickname_combo->setCurrentItem(current_server_index);

  if (valid_versions_.size() > 0) {
    version_combo->setCurrentItem(0);
  }
}

bool PublishDatabaseDialog::HasValidVersions() {
  return (!valid_versions_.empty());
}

QString PublishDatabaseDialog::GetSelectedNickname() {
  return nickname_combo->currentText();
}

int PublishDatabaseDialog::GetSelectedCombination() {
  return nickname_combo->currentItem();
}

std::string PublishDatabaseDialog::GetSelectedVersion() {
  return valid_versions_[version_combo->currentItem()];
}

std::string PublishDatabaseDialog::GetTargetPath() {
  return target_path_edit->text().toUtf8().constData();
}


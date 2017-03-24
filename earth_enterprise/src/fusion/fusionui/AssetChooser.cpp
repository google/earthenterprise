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


#include "fusion/fusionui/AssetChooser.h"

#include <list>

#include <qstring.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qmessagebox.h>
#include <qiconview.h>
#include <qinputdialog.h>
#include <qapplication.h>

#include "fusion/gst/gstAssetManager.h"

#include "fusion/fusionui/Preferences.h"
#include "fusion/fusionui/AssetManager.h"


namespace {

const int ChangeDirEventId  = static_cast<int>(QEvent::User);

class ChangeDirEvent : public QCustomEvent {
 public:
  explicit ChangeDirEvent(const gstAssetFolder &folder) :
      QCustomEvent(ChangeDirEventId),
      folder_(folder)
  { }

  gstAssetFolder folder_;
};

// -----------------------------------------------------------------------------

class FolderItem : public QIconViewItem {
 public:
  FolderItem(QIconView* parent, const gstAssetFolder& f);
  const gstAssetFolder getFolder() { return folder; }

 private:
  gstAssetFolder folder;
};

// -----------------------------------------------------------------------------

FolderItem::FolderItem(QIconView* parent, const gstAssetFolder& f)
    : QIconViewItem(parent), folder(f) {
  setText(shortAssetName(f.name()));
  AssetDisplayHelper a(AssetDefs::Invalid, std::string());
  setPixmap(a.GetPixmap());
  setKey(QString("0" + text()));
}

// -----------------------------------------------------------------------------

class AssetItem : public QIconViewItem {
 public:
  AssetItem(QIconView* parent, gstAssetHandle handle);
  gstAssetHandle getAssetHandle() { return assetHandle; }

 private:
  gstAssetHandle assetHandle;
};

// -----------------------------------------------------------------------------

AssetItem::AssetItem(QIconView* parent, gstAssetHandle handle)
    : QIconViewItem(parent), assetHandle(handle) {
  setText(shortAssetName(handle->getName()));

  Asset asset = handle->getAsset();
  AssetDisplayHelper a(asset->type, asset->subtype);
  setPixmap(a.GetPixmap());
  setKey(QString("%1").arg(a.GetSortKey()) + text());
}

}  // namespace

// -----------------------------------------------------------------------------

AssetChooser::AssetChooser(QWidget* parent, AssetChooser::Mode m,
                           AssetDefs::Type t, const std::string& st)
    : AssetChooserBase(parent, 0, false, 0), mode_(m),
      type_(t), subtype_(st),
      current_folder_(theAssetManager->getAssetRoot()) {
  const QString SaveBtnText = QObject::tr("Save");
  const QString OpenBtnText = QObject::tr("Open");

  iconView->setSorting(true);

  if (mode_ == Save || mode_ == SaveAs) {
    ok_btn->setText(SaveBtnText);
    setCaption(SaveBtnText);
  } else {
    ok_btn->setText(OpenBtnText);
    setCaption(OpenBtnText);
  }

  AssetDisplayHelper adh(type_, subtype_);
  compatible_asset_types_.push_back(adh);
  filterCombo->insertItem(adh.GetPixmap(), adh.PrettyName());
  filterCombo->insertItem("All");

  // This will draw the iconview contents.
  chooseFilter(adh.PrettyName());

  ok_btn->setEnabled(false);

  // Accept key presses.
  setFocusPolicy(QWidget::StrongFocus);

  // Restore previous directory for this type/subtype.
  RestorePreviousDir(adh);
}

AssetChooser::AssetChooser(
    QWidget* parent, AssetChooser::Mode m,
    const std::vector<AssetCategoryDef>& compatible_asset_defs,
    const std::string &all_compatible_assets_filter_text,
    const std::string &all_compatible_assets_icon_name)
    : AssetChooserBase(parent, 0, false, 0), mode_(m),
      all_compatible_assets_filter_text_(all_compatible_assets_filter_text),
      current_folder_(theAssetManager->getAssetRoot()) {
  assert(mode_ == Open);
  assert(!compatible_asset_defs.empty());

  {
    // Note: it is not required for Open Dialog, but just to have them
    // initialized.
    const AssetCategoryDef& asset_category_def = compatible_asset_defs[0];
    type_ = asset_category_def.type;
    subtype_ = asset_category_def.subtype;
  }

  const QString OpenBtnText = QObject::tr("Open");
  ok_btn->setText(OpenBtnText);
  setCaption(OpenBtnText);

  iconView->setSorting(true);

  // Insert 'all compatible assets' item in view filter combobox of OpenDialog.
  if (!all_compatible_assets_filter_text_.empty()) {
    if (!(all_compatible_assets_filter_text_.empty() ||
          all_compatible_assets_icon_name.empty())) {
      filterCombo->insertItem(
          AssetDisplayHelper::LoadPixmap(
              QObject::tr(all_compatible_assets_icon_name.c_str())),
          QObject::tr(all_compatible_assets_filter_text_.c_str()));
    } else {
      assert(!all_compatible_assets_filter_text_.empty());
      assert(all_compatible_assets_icon_name.empty());
      filterCombo->insertItem(
          QObject::tr(all_compatible_assets_filter_text_.c_str()));
    }
  }

  // Insert compatible asset items in view filter combobox of OpenDialog.
  for (size_t i = 0; i < compatible_asset_defs.size(); ++i) {
    const AssetCategoryDef& asset_category_def = compatible_asset_defs[i];
    AssetDisplayHelper adh(asset_category_def.type, asset_category_def.subtype);
    compatible_asset_types_.push_back(adh);
    filterCombo->insertItem(adh.GetPixmap(), adh.PrettyName());
  }
  filterCombo->insertItem("All");

  assert(compatible_asset_types_.size() != 0);

  // This will draw the iconview contents.
  if (!all_compatible_assets_filter_text_.empty()) {
    chooseFilter(QObject::tr(all_compatible_assets_filter_text_.c_str()));
  } else {
    const AssetDisplayHelper& adh = compatible_asset_types_[0];
    chooseFilter(adh.PrettyName());
  }

  ok_btn->setEnabled(false);

  // Accept key presses.
  setFocusPolicy(QWidget::StrongFocus);

  // Restore previous directory for this type/subtype.
  const AssetDisplayHelper& adh = compatible_asset_types_[0];
  RestorePreviousDir(adh);
}

void AssetChooser::RestorePreviousDir(const AssetDisplayHelper& adh) {
  if (khExists(Preferences::filepath("assetchooser.xml").latin1())) {
    if (chooser_history_.Load(
            Preferences::filepath("assetchooser.xml").latin1())) {
      QString dir = chooser_history_.FindDir(adh.GetSortKey());
      if (!dir.isEmpty()) {
        QString path = AssetDefs::AssetRoot() + "/" + dir;
        if (khDirExists(path.latin1())) {
          updateView(gstAssetFolder(path));
        } else {
          notify(NFY_WARN, "Invalid path in history: %s", path.latin1());
        }
      }
    }
  }
}

void AssetChooser::chooseFilter(const QString& str) {
  if (str == "All") {
    match_string_.clear();
  } else {
    match_string_ = str.latin1();
  }

  updateView(current_folder_);
}

bool AssetChooser::matchFilter(const gstAssetHandle handle) const {
  Asset asset = handle->getAsset();
  if (asset->meta.GetAs("hidden", false)) {
    return false;
  }

  if (match_string_.empty())
    return true;

  AssetDisplayHelper a(asset->type, asset->subtype);

  if (match_string_ == all_compatible_assets_filter_text_) {
    assert(!all_compatible_assets_filter_text_.empty());
    for (size_t i = 0; i < compatible_asset_types_.size(); ++i) {
      if (a.PrettyName() == compatible_asset_types_[i].PrettyName())
        return true;
    }
    return false;
  }

  return (a.PrettyName() == match_string_);
}

void AssetChooser::keyPressEvent(QKeyEvent* e) {
  if (e->key() == Qt::Key_Backspace) {
    cdtoparent();
    e->accept();
  } else if (e->key() == Qt::Key_Escape) {
    reject();
    e->accept();
  } else {
    e->ignore();
  }
}

void AssetChooser::customEvent(QCustomEvent *e) {
  // Make sure this is really an event that we sent.
  switch (static_cast<int>(e->type())) {
    case ChangeDirEventId: {
      ChangeDirEvent* cdEvent = dynamic_cast<ChangeDirEvent*>(e);
      iconView->clearSelection();
      updateView(cdEvent->folder_);
      break;
    }
  }
}

void AssetChooser::accept() {
  if (mode_ == Open) {
    // Check whether item has been selected;
    // Note: there is no textEdited-signal in Qt3, so we compare
    // name of selected/current item with nameEdit's text to make a decision
    // whether it has been edited.
    QIconViewItem* item = iconView->currentItem();
    if (item != NULL) {
      AssetItem* assetItem = dynamic_cast<AssetItem*>(item);
      // If name doesn't match with current item name, then reset item
      // pointer to initiate searching of item by name below.
      if (assetItem != NULL &&
          getName() != shortAssetName(assetItem->getAssetHandle()->getName())) {
        item = NULL;
      }
    }

    if (item == NULL) {
      // Note: means that name have been edited and we try to find item by name.
      item = iconView->findItem(getName(), Qt::ExactMatch);
    }

    AssetItem* assetItem = dynamic_cast<AssetItem*>(item);
    if (assetItem != NULL) {
      nameEdit->setText(shortAssetName(assetItem->getAssetHandle()->getName()));
      gstAssetHandle asset_handle = assetItem->getAssetHandle();
      Asset asset = asset_handle->getAsset();
      type_ = asset->type;
      subtype_ = asset->subtype;
    } else {
      QMessageBox::critical(
          this, "Error",
          tr("The specified asset \"") + getName() + tr("\" does not exist."),
          tr("OK"), QString::null, QString::null, 0);
      return;
    }
  }

  QString fullpath;
  if (!getFullPath(fullpath)) {
    QMessageBox::critical(this, tr("Error"),
        fullpath, tr("OK"), QString::null, QString::null, 0);
    return;
  }

  if (mode_ == Save || mode_ == SaveAs) {
    // validate this asset name is unique and prompt if not!
    if (Asset(fullpath.latin1())) {
      if (QMessageBox::warning(this, "Warning",
          fullpath + tr(" already exists.\nDo you want to replace it?"),
          tr("OK"), tr("Cancel"), 0, 1) == 1)
        return;
    }
  } else {
    // Validate whether this asset exists and has compatible asset type.
    if (!Asset(fullpath.latin1())) {
      QMessageBox::critical(
          this, "Error",
          tr("The specified asset \"") + getName() + tr("\" does not exist."),
          tr("OK"), QString::null, QString::null, 0);
      return;
    }

    if (!IsCompatibleAsset()) {
      QMessageBox::critical(this, "Error",
          tr("The selected asset \"") + getName() +
          tr("\" has an incompatible asset type."),
          tr("OK"), QString::null, QString::null, 0);
      return;
    }
  }

  // save directory for this type/subtype
  AssetDisplayHelper adh(type_, subtype_);
  chooser_history_.SetDir(adh.GetSortKey(), current_folder_.relativePath());
  chooser_history_.Save(Preferences::filepath("assetchooser.xml").latin1());

  AssetChooserBase::accept();
}

bool AssetChooser::getFullPath(QString& fullpath) const {
  QString path;
  if (current_folder_.relativePath().isEmpty()) {
    path = getName();
  } else {
    path = current_folder_.relativePath() + "/" + getName();
  }

  QString errormsg;
  std::string fullassetname;
  try {
    fullassetname = AssetDefs::NormalizeAssetName(path.latin1(), type_,
                                                  subtype_);
  } catch(const std::exception& e) {
    errormsg = QString::fromUtf8(e.what());
  } catch(...) {
    errormsg = "Unknown error";
  }

  if (!errormsg.isEmpty()) {
    fullpath = errormsg;
    return false;
  }

  fullpath = fullassetname.c_str();
  return true;
}

bool AssetChooser::IsCompatibleAsset() const {
  assert(mode_ == Open);
  const AssetDisplayHelper adh(type_, subtype_);
  std::vector<AssetDisplayHelper>::const_iterator it = std::find(
      compatible_asset_types_.begin(), compatible_asset_types_.end(), adh);
  return it != compatible_asset_types_.end();
}

QString AssetChooser::getName() const {
  return nameEdit->text();
}

const gstAssetFolder& AssetChooser::getFolder() const {
  return current_folder_;
}

void AssetChooser::selectItem(QIconViewItem* item) {
  AssetItem* assetItem = dynamic_cast<AssetItem*>(item);
  if (assetItem != NULL) {
    nameEdit->setText(shortAssetName(assetItem->getAssetHandle()->getName()));
  }
}

void AssetChooser::nameChanged(const QString& str) {
  if (str.isEmpty()) {
    ok_btn->setEnabled(false);
  } else {
    QIconViewItem* item = iconView->currentItem();
    if (item != NULL) {
       // If name doesn't match with current item, then clear selection.
      AssetItem* assetItem = dynamic_cast<AssetItem*>(item);
      if (assetItem != NULL &&
          getName() != shortAssetName(assetItem->getAssetHandle()->getName())) {
        iconView->clearSelection();
      }
    }
    ok_btn->setEnabled(true);
    ok_btn->setDefault(true);
  }
}

void AssetChooser::chooseItem(QIconViewItem* item) {
  FolderItem* folder = dynamic_cast<FolderItem*>(item);
  if (folder != NULL) {
    // we can't call updateView directly since it will replace the contents
    // of the list. And the Qt routine that called us will access it again
    // after we return. Post an event back to myself with the folder
    // and we'll call updateView when we get it
    QApplication::postEvent(this,
                            new ChangeDirEvent(folder->getFolder()));
    return;
  }

  AssetItem* asset = dynamic_cast<AssetItem*>(item);
  if (asset != NULL)
    accept();
}

void AssetChooser::cdtoparent() {
  if (current_folder_.name() == theAssetManager->getAssetRoot().name())
    return;
  updateView(current_folder_.getParentFolder());
}

void AssetChooser::NewDirectory() {
  bool ok;
  QString text = QInputDialog::getText("Make New Folder",
      tr("Enter new folder name:"),
      QLineEdit::Normal, QString::null, &ok, this);
  if (ok && !text.isEmpty()) {
    QString newdir(current_folder_.fullPath() + "/" + text);
    QString error;
    if (!theAssetManager->makeDir
        (AssetDefs::FilenameToAssetPath(newdir.latin1()), error)) {
      QMessageBox::critical(this, tr("Error"),
                            error, tr("OK"), QString::null, QString::null, 0);
    } else {
      updateView(current_folder_);
    }
  }
}

void AssetChooser::updateView(const gstAssetFolder& folder) {
  current_folder_ = folder;
  if (folder.name() == theAssetManager->getAssetRoot().name()) {
    cdtoparentBtn->setEnabled(false);
  } else {
    cdtoparentBtn->setEnabled(true);
  }

  iconView->clear();

  //
  // first add all folders
  //
  std::vector<gstAssetFolder> folders = folder.getAssetFolders();
  for (std::vector<gstAssetFolder>::iterator it = folders.begin();
       it != folders.end(); ++it) {
    (void)new FolderItem(iconView, *it);
  }

  //
  // now add all assets
  //
  std::vector<gstAssetHandle> items = folder.getAssetHandles();
  for (std::vector<gstAssetHandle>::iterator it = items.begin();
       it != items.end(); ++it) {
    if (matchFilter(*it))
      (void)new AssetItem(iconView, *it);
  }

  //
  // populate ancestry combobox
  //
  ancestryCombo->clear();
  gstAssetFolder f = folder;
  while (f.isValid() && f.name() != theAssetManager->getAssetRoot().name()) {
    ancestryCombo->insertItem(AssetManager::AssetRoot() + "/"
                              + f.relativePath(), 0);
    f = f.getParentFolder();
  }
  ancestryCombo->insertItem(AssetManager::AssetRoot() + "/", 0);

  ancestryCombo->setCurrentItem(ancestryCombo->count() - 1);
}


void AssetChooser::chooseAncestor(const QString& ancestor) {
  gstAssetFolder f = current_folder_;
  while (f.isValid()) {
    if (ancestor == QString(AssetManager::AssetRoot() + "/")
        + f.relativePath()) {
      updateView(f);
      return;
    }
    f = f.getParentFolder();
  }
}

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


#include "fusion/fusionui/AssetManager.h"
#include "Qt/qobjectdefs.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <Qt/qstring.h>
#include <Qt/qstringlist.h>
#include <Qt/qcombobox.h>
#include <Qt/qapplication.h>
#include <Qt/qmessagebox.h>
#include <Qt/qpixmap.h>
#include <Qt/q3header.h>
using QHeader = Q3Header;
#include <Qt/qprogressdialog.h>
#include <Qt/qlineedit.h>
#include <Qt/qtabwidget.h>
#include <Qt/qimage.h>
#include <Qt/qpushbutton.h>
#include <Qt/qinputdialog.h>
#include <Qt/qlayout.h>
#include <Qt/qpainter.h>
#include <Qt/qcheckbox.h>
#include <Qt/qsplitter.h>
#include <Qt/q3widgetstack.h>
using QWidgetStack = Q3WidgetStack;
#include <Qt/qcursor.h>
#include <Qt/qthread.h>
#include <Qt/q3dragobject.h>
using QImageDrag = Q3ImageDrag;
#include <Qt/q3mimefactory.h>
using QMimeSourceFactory = Q3MimeSourceFactory;

#include "fusion/autoingest/plugins/RasterProductAsset.h"
#include "fusion/autoingest/plugins/MercatorRasterProductAsset.h"
#include "fusion/autoingest/plugins/RasterProjectAsset.h"
#include "fusion/autoingest/plugins/MercatorRasterProjectAsset.h"
#include "fusion/autoingest/plugins/VectorProductAsset.h"
#include "fusion/autoingest/plugins/VectorProjectAsset.h"
#include "fusion/autoingest/plugins/VectorLayerXAsset.h"
#include "fusion/autoingest/plugins/MapLayerAsset.h"
#include "fusion/autoingest/plugins/MapProjectAsset.h"
#include "fusion/autoingest/plugins/MapDatabaseAsset.h"
#include "fusion/autoingest/plugins/MercatorMapDatabaseAsset.h"
#include "fusion/autoingest/plugins/SourceAsset.h"
#include "fusion/autoingest/plugins/DatabaseAsset.h"
#include "fusion/autoingest/plugins/KMLProjectAsset.h"
#include "fusion/autoingest/plugins/GEDBAsset.h"
#include "fusion/autoingest/.idl/ServerCombination.h"
#include "fusion/autoingest/.idl/gstProvider.h"
#include "fusion/fusionui/AssetBase.h"
#include "fusion/fusionui/AssetDisplayHelper.h"
#include "fusion/fusionui/Preferences.h"
#include "fusion/fusionui/AssetVersionProperties.h"
#include "fusion/fusionui/AssetIconView.h"
#include "fusion/fusionui/AssetTableView.h"
#include "fusion/fusionui/AssetProperties.h"
#include "fusion/fusionui/SourceFileDialog.h"
#include "fusion/fusionui/ImageryAsset.h"
#include "fusion/fusionui/MercatorImageryAsset.h"
#include "fusion/fusionui/TerrainAsset.h"
#include "fusion/fusionui/VectorAsset.h"
#include "fusion/fusionui/SystemListener.h"
#include "fusion/fusionui/MainWindow.h"
#include "fusion/fusionui/Database.h"
#include "fusion/fusionui/NewAsset.h"
#include "fusion/fusionui/MapLayer.h"
#include "fusion/fusionui/MapProject.h"
#include "fusion/fusionui/MapDatabase.h"
#include "fusion/fusionui/MercatorMapDatabase.h"
#include "fusion/fusionui/ImageryProject.h"
#include "fusion/fusionui/MercatorImageryProject.h"
#include "fusion/fusionui/KMLProject.h"
#include "fusion/fusionui/VectorLayer.h"
#include "fusion/fusionui/VectorProject.h"
#include "fusion/fusionui/TerrainProject.h"
#include "fusion/fusionui/PushDatabaseDialog.h"
#include "fusion/fusionui/PublishDatabaseDialog.h"
#include "fusion/fusionui/geGuiProgress.h"
#include "fusion/fusionui/geGuiAuth.h"
#include "fusion/gepublish/PublisherClient.h"
#include "fusion/gst/gstAssetManager.h"
#include "fusion/gst/gstRegistry.h"
#include "fusion/fusionversion.h"
#include "common/notify.h"
#include <array>

namespace {

const char* const folder_closed_xpm[]={
  "16 16 9 1",
  "g c #808080",
  "b c #c0c000",
  "e c #c0c0c0",
  "# c #000000",
  "c c #ffff00",
  ". c None",
  "a c #585858",
  "f c #a0a0a4",
  "d c #ffffff",
  "..###...........",
  ".#abc##.........",
  ".#daabc#####....",
  ".#ddeaabbccc#...",
  ".#dedeeabbbba...",
  ".#edeeeeaaaab#..",
  ".#deeeeeeefe#ba.",
  ".#eeeeeeefef#ba.",
  ".#eeeeeefeff#ba.",
  ".#eeeeefefff#ba.",
  ".##geefeffff#ba.",
  "...##gefffff#ba.",
  ".....##fffff#ba.",
  ".......##fff#b##",
  ".........##f#b##",
  "...........####." };

const char* const folder_open_xpm[]={
  "16 16 11 1",
  "# c #000000",
  "g c #c0c0c0",
  "e c #303030",
  "a c #ffa858",
  "b c #808080",
  "d c #a0a0a4",
  "f c #585858",
  "c c #ffdca8",
  "h c #dcdcdc",
  "i c #ffffff",
  ". c None",
  "....###.........",
  "....#ab##.......",
  "....#acab####...",
  "###.#acccccca#..",
  "#ddefaaaccccca#.",
  "#bdddbaaaacccab#",
  ".eddddbbaaaacab#",
  ".#bddggdbbaaaab#",
  "..edgdggggbbaab#",
  "..#bgggghghdaab#",
  "...ebhggghicfab#",
  "....#edhhiiidab#",
  "......#egiiicfb#",
  "........#egiibb#",
  "..........#egib#",
  "............#ee#" };

QPixmap* folderClosed = 0;
QPixmap* folderOpen = 0;

class AssetFolder : public QListViewItem {
 public:
  AssetFolder(QListViewItem* parent, const gstAssetFolder& f);
  AssetFolder(Q3ListView* parent, const gstAssetFolder& f);

  const gstAssetFolder& getFolder() const { return folder; }

  void setOpen(bool m);
  void populate();

  AssetFolder* newFolder(const QString& text, QString &error);

  int rtti() const { return type_; }

 private:
  gstAssetFolder folder;
  int type_;
};

// -----------------------------------------------------------------------------

AssetFolder::AssetFolder(Q3ListView* parent, const gstAssetFolder& f)
    : QListViewItem(parent, f.name()),
      folder(f),
      type_(ASSET_MANAGER) {
  setPixmap(0, *folderClosed);
  setExpandable(true);
}

AssetFolder::AssetFolder(QListViewItem* parent, const gstAssetFolder& f)
    : QListViewItem(parent, f.name()),
      folder(f),
      type_(ASSET_FOLDER) {
  setPixmap(0, *folderClosed);
  setExpandable(f.getAssetFolders().size() != 0);
  setRenameEnabled(0, false);
}

void AssetFolder::setOpen(bool o) {
  // always clear out all children
  QListViewItem* item = firstChild();
  while (item) {
    takeItem(item);
    delete item;
    item = firstChild();
  }

  if (o && folder.getAssetFolders().size() != 0) {
    setPixmap(0, *folderOpen);
    populate();
  } else {
    setPixmap(0, *folderClosed);
  }

  QListViewItem::setOpen(o);
}

void AssetFolder::populate() {
  std::vector<gstAssetFolder> folders = folder.getAssetFolders();

  for (const auto& it : folders) {
    (void)new AssetFolder(this, it);
  }
}

AssetFolder* AssetFolder::newFolder(const QString& text, QString &error) {
  if (!folder.newFolder(text, error))
    return NULL;

  if (!isExpandable())
    setExpandable(true);

  // if the folder was already opened, we need to create the
  // AssetFolder object
  if (isOpen())
    return new AssetFolder(this, folder.getChildFolder(text));

  // however, if it is closed, the setOpen() call will see this
  // new directory and build the AssetFolder object, so we need to find it
  setOpen(true);

  QListViewItem* child = firstChild();
  while (child) {
    if (child->text(0) == text)
      return dynamic_cast<AssetFolder*>(child);
    child = child->nextSibling();
  }

  return NULL;
}

// -----------------------------------------------------------------------------

static QPixmap uic_load_pixmap_AssetManager(const QString& name) {
  const QMimeSource* m = QMimeSourceFactory::defaultFactory()->data(name);
  if (!m)
    return QPixmap();
  QPixmap pix;
  QImageDrag::decode(m, pix);
  return pix;
}

bool DatabaseHasValidVersion(const Asset &asset) {

  for (const auto& version : asset->versions) {
    AssetVersion asset_version(version);
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

    return true;
  }
  return false;
}

}  // namespace

// -----------------------------------------------------------------------------

const std::array<AssetDefs::Type,6> TypeSet =
{{
  AssetDefs::Invalid,
  AssetDefs::Vector,
  AssetDefs::Imagery,
  AssetDefs::Terrain,
  AssetDefs::Map,
  AssetDefs::Database
}};

const std::array<std::string,5> SubTypeSet =
{{
  "",
  kResourceSubtype,
  kLayer,
  kProjectSubtype,
  kDatabaseSubtype
}};

// -----------------------------------------------------------------------------

QString AssetManager::AssetRoot() {
  PrefsConfig prefs = Preferences::getConfig();

  if (prefs.showFullAssetPath) {
    return QString(AssetDefs::AssetRoot().c_str());
  } else {
    return QString("ASSET_ROOT");
  }
}

// -----------------------------------------------------------------------------

std::vector<AssetAction*> AssetAction::all_actions;

AssetAction::AssetAction(QObject* parent, AssetBase* asset_window)
  : QAction(parent),
    asset_window_(asset_window) {
  setText(asset_window->Name());
  AssetDisplayHelper helper(asset_window->AssetType(),
                            asset_window->AssetSubtype());
  setIconSet(QIconSet(helper.GetPixmap()));
  connect(this, SIGNAL(activated()), asset_window, SLOT(showNormal()));
  connect(this, SIGNAL(activated()), asset_window, SLOT(raise()));
  connect(asset_window, SIGNAL(NameChanged(const QString&)),
          this, SLOT(NameChanged(const QString&)));
  connect(asset_window, SIGNAL(destroyed()), this, SLOT(Cleanup()));
  all_actions.push_back(this);
}

void AssetAction::NameChanged(const QString& text) {
  setText(text);
}

void AssetAction::Cleanup() {
  std::vector<AssetAction*>::iterator pos = std::find(all_actions.begin(),
                                                      all_actions.end(), this);
  if (pos != all_actions.end())
    all_actions.erase(pos);
  delete this;
}

QString AssetAction::Name() const {
  return asset_window_->Name();
}

AssetAction* AssetAction::FindAsset(const QString& txt) {
  for (const auto& it : all_actions) {
    if (it->Name() == txt)
      return it;
  }
  return NULL;
}

bool AssetAction::OkToCloseAll() {
  for (const auto& it : all_actions) {
    if (!it->asset_window_->OkToQuit())
      return false;
  }
  return true;
}

void AssetAction::CloseAll() {
  // make a copy of our asset window vector since deleting will
  // invalidate iterators on the original
  std::vector<AssetAction*> close_actions = all_actions;
  for (auto& it : close_actions) {
    if (it->asset_window_->OkToQuit()) {
      delete it->asset_window_;
    } else {
      // terminate at first failure
      return;
    }
  }
}

void AssetAction::Cascade(const QPoint& start_pos) {
  const int xoffset = 13;
  const int yoffset = 20;
  int x = start_pos.x() + xoffset;
  int y = start_pos.y() + yoffset;

  for (auto& it : all_actions) {
    it->asset_window_->move(x, y);
    it->asset_window_->showNormal();
    it->asset_window_->raise();
    x += xoffset;
    y += yoffset;
  }
}

//------------------------------------------------------------------------------
ServeThread::ServeThread(PublisherClient* publisher_client,
                         const std::string& gedb_path)
    : publisher_client_(publisher_client),
      gedb_path_(gedb_path),
      retval_(false) {
}

ServeThread::~ServeThread() {
}

void ServeThread::start() {
  QThread::start();
}

PushThread::PushThread(PublisherClient* publisher_client,
                       const std::string& gedb_path)
    : ServeThread(publisher_client, gedb_path) {
}

void PushThread::run(void) {
#if 1
  retval_ = publisher_client_->PushDatabase(gedb_path_);
#else
  // Note: can be used for testing.
  while (progress_->done() < progress_->total()) {
    notify(NFY_WARN,
           "PushThread: %zd/%zd", progress_->done(), progress_->total());
    progress_->incrementDone(1);
    sleep(1);
  }
  retval_ = true;
#endif
  emit sfinished();
}

PublishThread::PublishThread(PublisherClient* publisher_client,
                             const std::string& gedb_path,
                             const std::string& target_path)
    : ServeThread(publisher_client, gedb_path),
      target_path_(target_path) {
}

void PublishThread::run(void) {
  retval_ = publisher_client_->PublishDatabase(gedb_path_, target_path_);
  emit sfinished();
}

ServeAssistant::ServeAssistant(QProgressDialog* progress_dialog,
                               geGuiProgress* progress,
                               geGuiAuth* auth)
    : progress_dialog_(progress_dialog),
      progress_(progress),
      auth_(auth) {
  QObject::connect(&timer_, SIGNAL(timeout()), this, SLOT(Perform()));
}

ServeAssistant::~ServeAssistant() {
}

void ServeAssistant::Start() {
  timer_.start(100);
}

void ServeAssistant::Stop() {
  timer_.stop();
}

void ServeAssistant::SetCanceled() {
  progress_->setCanceled();
}

void ServeAssistant::Perform() {
  progress_dialog_->setMinimum(0);
  progress_dialog_->setMaximum(progress_->total());
  progress_dialog_->setValue(progress_->done());

  // Check to see if a request for user authentication has been made.
  // If so, pop up the dialog and signal when we are finished.
  // Note: IsRequestPending() has a mutex to prevent a race condition.
  if (auth_->IsRequestPending()) {
    auth_->ExecAndSignal();
  }
}

//------------------------------------------------------------------------------

AssetManager* AssetManager::self = NULL;

std::string AssetManager::GetProviderById(std::uint32_t id) {
  ProviderMap::const_iterator found = provider_map_.find(id);
  if (found != provider_map_.end()) {
    return found->second;
  }
  return std::string();
}

AssetManager::AssetManager(QWidget* parent)
    : AssetManagerBase(parent),
      icon_grid_(128),
      filter_type_(0),
      filter_subtype_(0),
      asset_manager_icon_choice_orig_(
          Preferences::getConfig().assetManagerIconChoice) {
  {
    gstProviderSet providers;
    if (providers.Load()) {
      for (const auto& it : providers.items) {
        provider_map_[it.id] = it.key;
      }
    }
  }

  connect(categories,
          SIGNAL(contextMenuRequested(Q3ListViewItem*, const QPoint&, int)),
          this, SLOT(rmbClicked(Q3ListViewItem*, const QPoint&, int)));

  connect(assetTabWidget, SIGNAL(currentChanged(QWidget*)),
          this, SLOT(selectFolder()));

  connect(assetTableView, SIGNAL(doubleClicked(int, int, int, const QPoint&)),
          this, SLOT(doubleClicked(int, int, int, const QPoint&)));

  connect(assetTableView, SIGNAL(contextMenuRequested(int, int, const QPoint&)),
          this, SLOT(tableAssetMenu(int, int, const QPoint&)));

  connect(assetIconView,
          SIGNAL(contextMenuRequested(Q3IconViewItem*, const QPoint&)),
          this, SLOT(iconAssetMenu(Q3IconViewItem*, const QPoint&)));

  connect(assetTableView, SIGNAL(currentChanged(int, int)),
          this, SLOT(CurrentAssetChanged(int, int)));

  if (folderOpen == 0) {
    folderOpen = new QPixmap(folder_open_xpm);
    folderClosed = new QPixmap(folder_closed_xpm);
  }

  categories->header()->setStretchEnabled(true);
  categories->header()->hide();

  if (khExists(Preferences::filepath("assetmanager.layout").toUtf8().constData())) {
    if (layout_persist_.Load(
            Preferences::filepath("assetmanager.layout").toUtf8().constData())) {
      // update filter combox
      filter_type_ = layout_persist_.filterType;
      filter_subtype_ = layout_persist_.filterSubType;
      typeCombo->setCurrentItem(filter_type_);
      subtypeCombo->setCurrentItem(filter_subtype_);

      // update position
      if(layout_persist_.width > 4000 || layout_persist_.width < 100) {
          layout_persist_.width = DEFAULT_WINDOW_WIDTH;
      }
      if(layout_persist_.height > 4000 || layout_persist_.height < 100) {
          layout_persist_.height = DEFAULT_WINDOW_HEIGHT;
      }

      resize(layout_persist_.width, layout_persist_.height);
      move(layout_persist_.xpos, layout_persist_.ypos);

      // update viewmode
      if (layout_persist_.viewMode == AssetManagerLayout::List) {
        assetTabWidget->setCurrentPage(0);
      } else if (layout_persist_.viewMode == AssetManagerLayout::Preview) {
        assetTabWidget->setCurrentPage(1);
      }

      // update visibility
      if (layout_persist_.showme)
        show();

      showHiddenAssets(layout_persist_.showHidden);
      showHiddenCheck->setChecked(layout_persist_.showHidden);

      if (layout_persist_.folderSplitter.front() != 0) {
        Q3ValueList<int> splitter_list = layout_persist_.folderSplitter;
        folder_splitter->setSizes(splitter_list);
      }
    }
  }

  setCaption(tr("Asset Manager") + Preferences::CaptionText());
  self = this;

  // disable some buttons in LT Mode
  if (GetFusionProductType() == FusionLT) {
    newImageryResourceAction->removeFrom(Toolbar);
    newTerrainResourceAction->removeFrom(Toolbar);
    newImageryProjectAction->removeFrom(Toolbar);
    newTerrainProjectAction->removeFrom(Toolbar);
  }
  toolbar_chooser_combo->setCurrentItem(asset_manager_icon_choice_orig_);
}

AssetManager::~AssetManager() {
  if (asset_manager_icon_choice_orig_ !=
      Preferences::getConfig().assetManagerIconChoice) {
    Preferences::getConfig().Save(Preferences::filepath("preferences.xml").toUtf8().constData());
  }
  layout_persist_.showme = isShown();
  if (assetTabWidget->currentPageIndex() == 0) {
    layout_persist_.viewMode = AssetManagerLayout::List;
  } else if (assetTabWidget->currentPageIndex() == 1) {
    layout_persist_.viewMode = AssetManagerLayout::Preview;
  }

  gstAssetFolder folder = GetSelectedFolder();
  if (folder.isValid()) {
    layout_persist_.selectedPath = folder.relativePath();
  } else {
    layout_persist_.selectedPath = QString::null;
  }

  layout_persist_.folderSplitter.clear();
  Q3ValueList<int> folder_splitter_list = folder_splitter->sizes();
  for (Q3ValueList<int>::Iterator it = folder_splitter_list.begin();
       it != folder_splitter_list.end(); ++it) {
    layout_persist_.folderSplitter.push_back(*it);
  }

  layout_persist_.filterType = filter_type_;
  layout_persist_.filterSubType = filter_subtype_;
  layout_persist_.Save(Preferences::filepath("assetmanager.layout").toUtf8().constData());
}

AssetManagerLayout::Size AssetManager::GetLayoutSize(const QString& name) {
  return self->layout_persist_.FindLayoutByName(name);
}

void AssetManager::SetLayoutSize(const QString& name, int width, int height) {
  self->layout_persist_.SetLayoutSize(name, width, height);
}

void AssetManager::HandleNewWindow(AssetBase* asset_window) {
  AssetDisplayHelper helper(asset_window->AssetType(),
                            asset_window->AssetSubtype());
  AssetAction* action = new AssetAction(this, asset_window);
  action->addTo(Window);
  asset_window->setIcon(helper.GetPixmap());
  asset_window->move(QCursor::pos());
  asset_window->show();
}

bool AssetManager::RestoreExisting(const std::string& asset_ref) {
  AssetAction* action = AssetAction::FindAsset(asset_ref.c_str());
  if (action == NULL) {
    return false;
  } else {
    action->activate(QAction::Trigger); // could also be QAction::Hover
    return true;
  }
}

bool AssetManager::OkToQuit() {
  return AssetAction::OkToCloseAll();
}

void AssetManager::CloseAllWindows() {
  AssetAction::CloseAll();
}

void AssetManager::CascadeWindows() {
  AssetAction::Cascade(pos());
}

void AssetManager::CreateNewAsset() {
  NewAsset new_asset(this);
  AssetDisplayHelper::AssetKey asset_type  = new_asset.ChooseAssetType();

  switch (asset_type) {
    case AssetDisplayHelper::Key_VectorProduct:
      HandleNewWindow(new VectorAsset(this));
      break;
    case AssetDisplayHelper::Key_VectorLayer:
      HandleNewWindow(new VectorLayer(this));
      break;
    case AssetDisplayHelper::Key_VectorProject:
      HandleNewWindow(new VectorProject(this));
      break;
    case AssetDisplayHelper::Key_ImageryProduct:
      HandleNewWindow(new ImageryAsset(this));
      break;
    case AssetDisplayHelper::Key_MercatorImageryProduct:
      HandleNewWindow(new MercatorImageryAsset(this));
      break;
    case AssetDisplayHelper::Key_ImageryProject:
      HandleNewWindow(new ImageryProject(this));
      break;
    case AssetDisplayHelper::Key_MercatorImageryProject:
      HandleNewWindow(new ImageryProject(this));
      break;
    case AssetDisplayHelper::Key_TerrainProduct:
      HandleNewWindow(new TerrainAsset(this));
      break;
    case AssetDisplayHelper::Key_TerrainProject:
      HandleNewWindow(new TerrainProject(this));
      break;
    case AssetDisplayHelper::Key_Database:
      HandleNewWindow(new Database(this));
      break;
    case AssetDisplayHelper::Key_MapLayer:
      HandleNewWindow(new MapLayer(this));
      break;
    case AssetDisplayHelper::Key_MapProject:
      HandleNewWindow(new MapProject(this));
      break;
    case AssetDisplayHelper::Key_MapDatabase:
      HandleNewWindow(new MapDatabase(this));
      break;
    case AssetDisplayHelper::Key_MercatorMapDatabase:
      HandleNewWindow(new MercatorMapDatabase(this));
      break;
    case AssetDisplayHelper::Key_KMLProject:
      HandleNewWindow(new KMLProject(this));
      break;
    case AssetDisplayHelper::Key_Invalid:
    case AssetDisplayHelper::Key_Unknown:
      // do nothing
      break;
  }
}

void AssetManager::NewVectorAsset() {
  HandleNewWindow(new VectorAsset(this));
}

void AssetManager::NewVectorLayer() {
  HandleNewWindow(new VectorLayer(this));
}

void AssetManager::NewVectorProject() {
  HandleNewWindow(new VectorProject(this));
}

void AssetManager::NewImageryAsset() {
  HandleNewWindow(new ImageryAsset(this));
}

void AssetManager::NewMercatorImageryAsset() {
  HandleNewWindow(new MercatorImageryAsset(this));
}

void AssetManager::NewImageryProject() {
  HandleNewWindow(new ImageryProject(this));
}

void AssetManager::NewMercatorImageryProject() {
  HandleNewWindow(new MercatorImageryProject(this));
}

void AssetManager::NewTerrainAsset() {
  HandleNewWindow(new TerrainAsset(this));
}

void AssetManager::NewTerrainProject() {
  HandleNewWindow(new TerrainProject(this));
}

void AssetManager::NewMapLayer() {
  HandleNewWindow(new MapLayer(this));
}

void AssetManager::NewMapProject() {
  HandleNewWindow(new MapProject(this));
}

void AssetManager::NewMapDatabase() {
  HandleNewWindow(new MapDatabase(this));
}

void AssetManager::ToolbarChooserComboActivated(int choice) {
  Preferences::getConfig().assetManagerIconChoice =
      static_cast<PrefsConfig::ShowAssetManagerIconType>(choice);
  HideIcons();
  ShowIcons();
}

void AssetManager::NewMercatorMapDatabase() {
  HandleNewWindow(new MercatorMapDatabase(this));
}

void AssetManager::NewKMLProject() {
  HandleNewWindow(new KMLProject(this));
}

void AssetManager::NewDatabase() {
  HandleNewWindow(new Database(this));
}

void AssetManager::moveEvent(QMoveEvent* event) {
  const QPoint& pt = event->pos();

  // XXX only update if this is a valid event
  // XXX this is clearly a QT bug!
  if (x() != 0 && y() != 0) {
    layout_persist_.xpos = pt.x();
    layout_persist_.ypos = pt.y();
  }
}

void AssetManager::resizeEvent(QResizeEvent* event) {
  const QSize& sz = event->size();
  layout_persist_.width = sz.width();
  layout_persist_.height = sz.height();
}

void AssetManager::RemoveToolBarIcons() {
  newVectorResourceAction->removeFrom(Toolbar);
  newVectorProjectAction->removeFrom(Toolbar);
  newImageryResourceAction->removeFrom(Toolbar);
  newImageryProjectAction->removeFrom(Toolbar);
  newTerrainResourceAction->removeFrom(Toolbar);
  newTerrainProjectAction->removeFrom(Toolbar);
  newDatabaseAction->removeFrom(Toolbar);
  newImageryResourceMercatorAction->removeFrom(Toolbar);
  newImageryProjectMercatorAction->removeFrom(Toolbar);
  newMapLayerAction->removeFrom(Toolbar);
  newMapProjectAction->removeFrom(Toolbar);
  newMapDatabaseMercatorAction->removeFrom(Toolbar);
  newMapDatabaseAction->removeFrom(Toolbar);
}

void AssetManager::ShowIcons() {
  switch (asset_manager_icon_choice_ =
          Preferences::getConfig().assetManagerIconChoice) {
    case PrefsConfig::ShowEarthIcons:
      newImageryResourceAction->addTo(Toolbar);
      newImageryProjectAction->addTo(Toolbar);
      newVectorResourceAction->addTo(Toolbar);
      newVectorProjectAction->addTo(Toolbar);
      newTerrainResourceAction->addTo(Toolbar);
      newTerrainProjectAction->addTo(Toolbar);
      newDatabaseAction->addTo(Toolbar);
      break;
    case PrefsConfig::ShowMercatorMapIcons:
      newImageryResourceMercatorAction->addTo(Toolbar);
      newImageryProjectMercatorAction->addTo(Toolbar);
      newVectorResourceAction->addTo(Toolbar);
      newMapLayerAction->addTo(Toolbar);
      newMapProjectAction->addTo(Toolbar);
      newMapDatabaseMercatorAction->addTo(Toolbar);
      break;
    case PrefsConfig::ShowWjs84MapIcons:
      newImageryResourceAction->addTo(Toolbar);
      newImageryProjectAction->addTo(Toolbar);
      newVectorResourceAction->addTo(Toolbar);
      newMapLayerAction->addTo(Toolbar);
      newMapProjectAction->addTo(Toolbar);
      newMapDatabaseAction->addTo(Toolbar);
      break;
    default:
      break;
  }
}

void AssetManager::show() {
  if (!isVisible()) {
    connect(SystemListener::instance,
            SIGNAL(assetsChanged(const AssetChanges&)),
            this,
            SLOT(assetsChanged(const AssetChanges&)));
    RedrawTree(true);
  }
  HideIcons();
  ShowIcons();
  AssetManagerBase::show();
}

void AssetManager::HideIcons() {
  switch (asset_manager_icon_choice_) {
    case PrefsConfig::ShowEarthIcons:
      newVectorResourceAction->removeFrom(Toolbar);
      newVectorProjectAction->removeFrom(Toolbar);
      newImageryResourceAction->removeFrom(Toolbar);
      newImageryProjectAction->removeFrom(Toolbar);
      newTerrainResourceAction->removeFrom(Toolbar);
      newTerrainProjectAction->removeFrom(Toolbar);
      newDatabaseAction->removeFrom(Toolbar);
      break;
    case PrefsConfig::ShowMercatorMapIcons:
      newVectorResourceAction->removeFrom(Toolbar);
      newImageryResourceMercatorAction->removeFrom(Toolbar);
      newImageryProjectMercatorAction->removeFrom(Toolbar);
      newMapLayerAction->removeFrom(Toolbar);
      newMapProjectAction->removeFrom(Toolbar);
      newMapDatabaseMercatorAction->removeFrom(Toolbar);
      break;
    case PrefsConfig::ShowWjs84MapIcons:
      newVectorResourceAction->removeFrom(Toolbar);
      newImageryResourceAction->removeFrom(Toolbar);
      newImageryProjectAction->removeFrom(Toolbar);
      newMapLayerAction->removeFrom(Toolbar);
      newMapProjectAction->removeFrom(Toolbar);
      newMapDatabaseAction->removeFrom(Toolbar);
      break;
    default:
      break;
  }
}

void AssetManager::hide() {
  disconnect(SystemListener::instance,
             SIGNAL(assetsChanged(const AssetChanges&)),
             this,
             SLOT(assetsChanged(const AssetChanges&)));
  HideIcons();
  AssetManagerBase::hide();
}

void AssetManager::RedrawTree(bool select_top) {
  categories->clear();

  if (theAssetManager == NULL) {
    QMessageBox::warning(this, "Warning",
                         trUtf8("Asset Manager is misconfigured!"),
                         tr("OK"), 0, 0, 1);
    return;
  }

  const gstAssetFolder& assetroot = theAssetManager->getAssetRoot();

  AssetFolder* parent = new AssetFolder(categories, assetroot);
  parent->setText(0, AssetRoot());
  parent->setOpen(true);

  // first time drawing the tree might need to restore
  // previous view.
  if (!layout_persist_.selectedPath.isEmpty()) {
    OpenFolder(layout_persist_.selectedPath);
    layout_persist_.selectedPath = QString::null;
  } else if (select_top && categories->firstChild()) {
    categories->setSelected(categories->firstChild(), true);
  }
}

void AssetManager::addAsset() {
}

void AssetManager::childCollapsed(QListViewItem* item) {
  AssetFolder* assetfolder = dynamic_cast<AssetFolder*>(item);
  if (assetfolder == NULL)
    return;

  gstAssetFolder folder = GetSelectedFolder();
  if (!folder.isValid())
    categories->setSelected(assetfolder, true);
}

void AssetManager::rmbClicked(QListViewItem* item, const QPoint& pos, int) {
  enum { REFRESH, NEW_FOLDER };

  QPopupMenu menu(this);

  menu.insertItem(trUtf8("Refresh"), REFRESH);
  menu.insertSeparator();

  QString folder_name;
  AssetFolder* asset_folder = dynamic_cast<AssetFolder*>(item);
  if (asset_folder != NULL) {
    folder_name = asset_folder->getFolder().fullPath();
  }

  menu.insertItem(trUtf8("New Subfolder..."), NEW_FOLDER);

  switch (menu.exec(pos)) {
    case REFRESH:
      refresh();
      break;

    case NEW_FOLDER:
      NewFolder(folder_name);
      break;
  }
}

void AssetManager::tableAssetMenu(int row, int col, const QPoint& mouse_pos) {
  gstAssetHandle handle = GetAssetByRow(row);
  if (!handle->isValid())
    return;
  ShowAssetMenu(handle, mouse_pos);
}

void AssetManager::iconAssetMenu(QIconViewItem* item, const QPoint& mouse_pos) {
  AssetIcon* icon = dynamic_cast<AssetIcon*>(item);
  if (icon == NULL)
    return;

  ShowAssetMenu(icon->getAssetHandle(), mouse_pos);
}

void AssetManager::ShowAssetMenu(const gstAssetHandle& asset_handle,
                                 const QPoint& mouse_pos) {
  enum { BUILD_ASSET, CANCEL_ASSET, MODIFY_ASSET,
         PUSH_DB, PUBLISH_DB, ASSET_PROPERTIES,
         CURR_VER_PROPERTIES };
  QPopupMenu menu(this);

  Asset current_asset = asset_handle->getAsset();
  AssetVersion current_version(current_asset->CurrVersionRef());

  // first item in menu should be the asset name since the table
  // might get redrawn after the menu has popped-up
  AssetDisplayHelper a(current_asset->type, current_asset->subtype);

  std::string shortName = shortAssetName(asset_handle->getName());

  menu.insertItem(a.GetPixmap(), shortName.c_str());

  menu.insertSeparator();
  menu.insertSeparator();

  if (current_version) {
    menu.insertItem(trUtf8("Current Version Properties"),
                    CURR_VER_PROPERTIES);
  }
  menu.insertItem(trUtf8("Asset Versions"), ASSET_PROPERTIES);
  if (current_version && current_version->CanCancel()) {
    menu.insertItem(trUtf8("Cancel Current Version"),
                    CANCEL_ASSET);
  }

  if (current_asset->type == AssetDefs::Database) {
    int push_item_id = menu.insertItem(trUtf8("Push"), PUSH_DB);
    if (DatabaseHasValidVersion(current_asset)) {
      menu.setItemEnabled(push_item_id, TRUE);
    } else {
      menu.setItemEnabled(push_item_id, FALSE);
    }

    // Note: Switches off Publish functionality.
    // The virtual host is removed from Server Combination,
    // So it contains only URL, while for publishing we need URL + virtual
    // host.
    // Thought is to encourage server UI for all publishing.
    // menu.insertItem(trUtf8("Publish"), PUBLISH_DB);
  }

  menu.insertItem(trUtf8("&Build"), BUILD_ASSET);
  menu.insertItem(trUtf8("Modify"), MODIFY_ASSET);


  switch (menu.exec(mouse_pos)) {
    case BUILD_ASSET:
      BuildAsset(asset_handle);
      refresh();
      break;

    case CANCEL_ASSET:
      CancelAsset(asset_handle);
      refresh();
      break;

    case MODIFY_ASSET:
      ModifyAsset(asset_handle);
      refresh();
      break;

    case ASSET_PROPERTIES:
      ShowAssetProperties(asset_handle);
      break;

    case CURR_VER_PROPERTIES:
      ShowCurrVerProperties(asset_handle);
      break;

    case PUSH_DB:
      PushDatabase(asset_handle);
      refresh();
      break;

    case PUBLISH_DB:
      PublishDatabase(asset_handle);
      refresh();
      break;
  }
}

void AssetManager::showHiddenAssets(bool checked) {
  layout_persist_.showHidden = checked;
  // force redraw of table view or icon view
  selectFolder();
}

void AssetManager::CurrentAssetChanged(int row, int col) {
#if 0
  if (row == -1 || col == -1) {
    detail_stack->raiseWidget(0);
    return;
  }

  gstAssetHandle handle = GetAssetByRow(row);
  Asset current_asset = handle->getAsset();
  AssetVersion current_version(current_asset->CurrVersionRef());
  if (current_asset->type == AssetDefs::Database) {
    detail_stack->raiseWidget(1);
  } else {
    detail_stack->raiseWidget(2);
  }
#endif
}

void AssetManager::doubleClicked(int row, int col, int button,
                                 const QPoint& mouse_pos) {
  // clicking on background deselects current asset
  if (row == -1 || col == -1)
    return;

  gstAssetHandle handle = GetAssetByRow(row);
  if (!handle->isValid())
    return;

  if (col == 0)
    ModifyAsset(handle);
  if (col == 3 || col == 4)
    ShowCurrVerProperties(handle);
}

void AssetManager::ShowAssetProperties(const gstAssetHandle& handle) {
  AssetProperties* properties = new AssetProperties(NULL, handle);
  properties->show();
}

void AssetManager::ShowCurrVerProperties(const gstAssetHandle& handle) {
  Asset currAsset = handle->getAsset();
  AssetVersionProperties::Open(currAsset->CurrVersionRef());
}

gstAssetHandle AssetManager::GetAssetByRow(int row) {
  if (row == -1)
    return gstAssetHandleImpl::Create(gstAssetFolder(QString::null),
                                      QString::null);
  return assetTableView->GetAssetHandle(row);
}

void AssetManager::BuildAsset(const gstAssetHandle& handle) {
  QString error;
  if (!theAssetManager->buildAsset(handle, error)) {
    if (error.isEmpty())
      error =  tr("Build has unknown problems.\n");
    QMessageBox::critical(this, tr("Error"),
                          error,
                          tr("OK"), 0, 0, 0);
  }
}

void AssetManager::CancelAsset(const gstAssetHandle& handle) {
  QString error;
  if (!theAssetManager->cancelAsset(handle, error)) {
    if (error.isEmpty())
      error =  tr("Cancel has unknown problems.\n");
    QMessageBox::critical(this, tr("Error"),
                          error,
                          tr("OK"), 0, 0, 0);
  }
}

class FixCursor {
 public:
  FixCursor(QWidget* p) : parent(p) {}
  ~FixCursor() {
    parent->setCursor(Qt::arrowCursor);
  }
 private:
  QWidget* parent;
};

void AssetManager::PushDatabase(const gstAssetHandle& handle) {
  Asset asset = handle->getAsset();
  ServerCombinationSet sc_set;
  sc_set.Load();

  if (asset->versions.size() == 0) {
    QMessageBox::critical(this, "Error",
            tr("Please build the database first."), 0, 0, 0);
    return;
  }

  if (sc_set.combinations.size() == 0) {
    QMessageBox::critical(this, "Error", tr("Please add at least one server"
      "association using the Publisher tool."), 0, 0, 0);
    return;
  }

  std::vector<QString> nicknames;

  for (const auto& it : sc_set.combinations) {
    nicknames.push_back(it.nickname);
  }

  PushDatabaseDialog push_db_dlg(this, asset, nicknames);
  if (!push_db_dlg.HasValidVersions()) {
    QMessageBox::critical(
        this, "Error",
        tr("This database does not have a single pushable version."
           " Try rebuilding the database."),
        0, 0, 0);
    return;
  }

  if (push_db_dlg.exec() != QDialog::Accepted)
    return;

  AssetVersion db_asset_version();

  std::string gedb_path;
  AssetVersion gedb_version;
  if (asset->subtype == kMapDatabaseSubtype) {
    MapDatabaseAssetVersion mdav(push_db_dlg.GetSelectedVersion());
    gedb_version = mdav->GetMapdbChild();
  } else if (asset->subtype == kMercatorMapDatabaseSubtype) {
    MercatorMapDatabaseAssetVersion mdav(push_db_dlg.GetSelectedVersion());
    gedb_version = mdav->GetMapdbChild();
  } else {
    DatabaseAssetVersion dav(push_db_dlg.GetSelectedVersion());
    gedb_version = dav->GetGedbChild();
  }

  gedb_path = gedb_version->GetOutputFilename(0);

  QString nickname = push_db_dlg.GetSelectedNickname();

  // Update the preferences with the user's choice. We want to remember these
  // choices so that we can automatically select this server next time they
  // push/publish.
  std::string database_name = shortAssetName(asset->GetRef().toString());
  Preferences::UpdatePublishServerDbMap(database_name, nickname.toUtf8().constData());

  ServerConfig stream_server, search_server;

  for (const auto& it : sc_set.combinations) {
    if (nickname == it.nickname) {
      stream_server = it.stream;
      search_server = stream_server;
      break;
    }
  }

  geGuiProgress progress(100);
  geGuiAuth auth(this);
  // Currently, the auth dialog will be executed in the current thread.
  auth.SetSynchronous(true);

  PublisherClient publisher_client(AssetDefs::MasterHostName(),
                                   stream_server, search_server,
                                   &progress, &auth);
  if (!publisher_client.AddDatabase(
          gedb_path, push_db_dlg.GetSelectedVersion())) {
    QMessageBox::critical(this, "Push Failed",
            tr("Error: %1 ").arg(publisher_client.ErrMsg().c_str()), 0, 0, 0);
    return;
  }

  FixCursor fix_cursor(this);
  QProgressDialog progress_dialog(tr("Pushing database..."),
                                  tr("Cancel"), 0, 100, this);
  progress_dialog.setCaption(tr("Pushing"));

  // PublisherClient will now be run in a separate thread,
  // so any calls to the auth dialog now need to be asynchronous.
  auth.SetSynchronous(false);
  PushThread push_thread(&publisher_client, gedb_path);

  {
    ServeAssistant push_assistant(&progress_dialog, &progress, &auth);
    QObject::connect(&progress_dialog, SIGNAL(canceled()),
                     &push_assistant, SLOT(SetCanceled()));
    QObject::connect(&push_thread, SIGNAL(sfinished()), &push_assistant, SLOT(Stop()));
    // TODO: may still need? QObject::connect(&push_thread, SIGNAL(sfinished()), &push_thread, SLOT(deleteLater()));

    push_thread.start();
    push_assistant.Start();
    progress_dialog.show();

    while (push_thread.isRunning()) {
        qApp->processEvents();
    }
  }

  if (progress_dialog.wasCanceled()) {
    QMessageBox::critical(this, "Push Interrupted",
                          tr("Push Interrupted"), 0, 0, 0);
    return;
  }

  // hide progress dialog
  progress_dialog.reset();

  if (!push_thread.retval()) {
    QMessageBox::critical(this, "Push Failed",
            tr("Error: %1").arg(publisher_client.ErrMsg().c_str()), 0, 0, 0);
    return;
  }

  QMessageBox::information(this, tr("Success"),
                          tr("Database Successfully Pushed"),
                          QMessageBox::Ok, 0, 0);
}

void AssetManager::PublishDatabase(const gstAssetHandle& handle) {
  Asset asset = handle->getAsset();
  ServerCombinationSet sc_set;
  sc_set.Load();

  if (asset->versions.size() == 0) {
    QMessageBox::critical(this, "Error",
            tr("Please build the database first."), 0, 0, 0);
    return;
  }

  if (sc_set.combinations.size() == 0) {
    QMessageBox::critical(this, "Error", tr("Please add at least one server"
      "association using the Publisher tool."), 0, 0, 0);
    return;
  }

  std::vector<QString> nicknames;

  for (const auto& it : sc_set.combinations) {
    nicknames.push_back(it.nickname);
  }


  PublishDatabaseDialog publish_db_dlg(this, asset, nicknames);
  if (!publish_db_dlg.HasValidVersions()) {
    QMessageBox::critical(this, "Error",
      tr("This database does not have a single publishable version."
         " Try rebuilding the database."),
      0, 0, 0);
    return;
  }

  if (publish_db_dlg.exec() != QDialog::Accepted)
    return;

  std::string target_path = publish_db_dlg.GetTargetPath();

  AssetVersion db_asset_version();

  std::string gedb_path;
  AssetVersion gedb_version;
  if (asset->subtype == kMapDatabaseSubtype) {
    MapDatabaseAssetVersion mdav(publish_db_dlg.GetSelectedVersion());
    gedb_version = mdav->GetMapdbChild();
  } else if (asset->subtype == kMercatorMapDatabaseSubtype) {
    MercatorMapDatabaseAssetVersion mdav(publish_db_dlg.GetSelectedVersion());
    gedb_version = mdav->GetMapdbChild();
  } else {
    DatabaseAssetVersion dav(publish_db_dlg.GetSelectedVersion());
    gedb_version = dav->GetGedbChild();
  }

  gedb_path = gedb_version->GetOutputFilename(0);

  QString nickname = publish_db_dlg.GetSelectedNickname();

  // Update the preferences with the user's choice. We want to remember these
  // choices so that we can automatically select this server next time they
  // push/publish.
  std::string database_name = shortAssetName(asset->GetRef().toString());
  Preferences::UpdatePublishServerDbMap(database_name, nickname.toUtf8().constData());

  ServerConfig stream_server, search_server;

  for (const auto& it : sc_set.combinations) {
    if (nickname == it.nickname) {
      stream_server = it.stream;
      search_server = stream_server;
      break;
    }
  }

  geGuiProgress progress(100);
  geGuiAuth auth(this);

  // Currently, the auth dialog will be executed in the current thread.
  auth.SetSynchronous(true);

  PublisherClient publisher_client(AssetDefs::MasterHostName(),
                                   stream_server, search_server,
                                   &progress, &auth);
  FixCursor fix_cursor(this);
  QProgressDialog progress_dialog(tr("Publishing database..."),
                                  tr("Cancel"), 0, 100, this);
  progress_dialog.setCaption(tr("Publishing"));

  // PublisherClient will now be run in a separate thread,
  // so any calls to the auth dialog now need to be asynchronous.
  auth.SetSynchronous(false);
  PublishThread publish_thread(&publisher_client, gedb_path, target_path);

  {
    ServeAssistant publish_assistant(&progress_dialog, &progress, &auth);
    QObject::connect(&progress_dialog, SIGNAL(canceled()),
                     &publish_assistant, SLOT(SetCanceled()));
    QObject::connect(
        &publish_thread, SIGNAL(sfinished()), &publish_assistant, SLOT(Stop()));
    // TODO: May still need? QObject::connect(&publish_thread, SIGNAL(sfinished()), &publish_thread, SLOT(deleteLater()));

    publish_thread.start();
    publish_assistant.Start();
    progress_dialog.show();

    while (publish_thread.isRunning()) {
        qApp->processEvents();
    }
  }

  if (progress_dialog.wasCanceled()) {
    QMessageBox::critical(this, "Publish Interrupted",
                          tr("Publish Interrupted"), 0, 0, 0);
    return;
  }

  // hide progress dialog
  progress_dialog.reset();

  if (!publish_thread.retval()) {
    QMessageBox::critical(this, "Publish Failed",
            tr("Error: %1").arg(publisher_client.ErrMsg().c_str()), 0, 0, 0);
    return;
  }

  QMessageBox::information(this, tr("Success"),
                          tr("Database Successfully Published"),
                          QMessageBox::Ok, 0, 0);
}


void AssetManager::ModifyAsset(const gstAssetHandle& handle) {
  Asset current_asset = handle->getAsset();
  std::string assetref = current_asset->GetRef();
  if (RestoreExisting(assetref))
    return;

  AssetBase* asset_window = NULL;
  if (current_asset->type == AssetDefs::Imagery &&
      current_asset->subtype == kProductSubtype) {
    RasterProductAsset prod(assetref);
    RasterProductImportRequest request(prod->type);
    request.assetname = assetref;
    request.config = prod->config;
    request.meta = prod->meta;
    SourceAssetVersion source(prod->inputs[0]);
    assert(source);
    request.sources = source->config;
    asset_window = new ImageryAsset(this, request);
  } else if (current_asset->type == AssetDefs::Imagery &&
      current_asset->subtype == kMercatorProductSubtype) {
    MercatorRasterProductAsset prod(assetref);
    RasterProductImportRequest request(prod->type);
    request.assetname = assetref;
    request.config = prod->config;
    request.meta = prod->meta;
    SourceAssetVersion source(prod->inputs[0]);
    assert(source);
    request.sources = source->config;
    asset_window = new MercatorImageryAsset(this, request);
  } else if (current_asset->type == AssetDefs::Terrain &&
      current_asset->subtype == kProductSubtype) {
    RasterProductAsset prod(assetref);
    RasterProductImportRequest request(prod->type);
    request.assetname = assetref;
    request.config = prod->config;
    request.meta = prod->meta;
    SourceAssetVersion source(prod->inputs[0]);
    assert(source);
    request.sources = source->config;
    asset_window = new TerrainAsset(this, request);
  } else if (current_asset->type == AssetDefs::Vector &&
             current_asset->subtype == kProductSubtype) {
    VectorProductAsset prod(assetref);
    VectorProductImportRequest request;
    request.assetname = assetref;
    request.config = prod->config;
    request.meta = prod->meta;
    SourceAssetVersion source(prod->inputs[0]);
    assert(source);
    request.sources = source->config;
    asset_window = new VectorAsset(this, request);
  } else if (current_asset->type == AssetDefs::Database &&
             current_asset->subtype == kDatabaseSubtype) {
    DatabaseAsset dbasset(assetref);
    DatabaseEditRequest request;
    request.assetname = assetref;
    request.config = dbasset->config;
    request.meta = dbasset->meta;
    asset_window = new Database(this, request);
  } else if (current_asset->type == AssetDefs::Database &&
             current_asset->subtype == kMapDatabaseSubtype) {
    MapDatabaseAsset dbasset(assetref);
    MapDatabaseEditRequest request;
    request.assetname = assetref;
    request.config = dbasset->config;
    request.meta = dbasset->meta;
    asset_window = new MapDatabase(this, request);
  } else if (current_asset->type == AssetDefs::Database &&
             current_asset->subtype == kMercatorMapDatabaseSubtype) {
    MercatorMapDatabaseAsset dbasset(assetref);
    MapDatabaseEditRequest request;
    request.assetname = assetref;
    request.config = dbasset->config;
    request.meta = dbasset->meta;
    asset_window = new MercatorMapDatabase(this, request);
  } else if (current_asset->type == AssetDefs::Map &&
             current_asset->subtype == kLayer) {
    MapLayerEditRequest request;
    request.assetname = assetref;
    MapLayerAsset layer(assetref);
    request.config = layer->config;
    request.meta = layer->meta;
    asset_window = new MapLayer(this, request);
  } else if (current_asset->type == AssetDefs::Map &&
             current_asset->subtype == kProjectSubtype) {
    MapProjectEditRequest request;
    request.assetname = assetref;
    MapProjectAsset project(assetref);
    request.config = project->config;
    request.meta = project->meta;
    asset_window = new MapProject(this, request);
  } else if (current_asset->type == AssetDefs::Vector &&
             current_asset->subtype == kLayer) {
    VectorLayerXEditRequest request;
    request.assetname = assetref;
    VectorLayerXAsset layer(assetref);
    request.config = layer->config;
    request.meta = layer->meta;
    asset_window = new VectorLayer(this, request);
  } else if (current_asset->type == AssetDefs::Vector &&
             current_asset->subtype == kProjectSubtype) {
    VectorProjectEditRequest request;
    request.assetname = assetref;
    VectorProjectAsset project(assetref);
    request.config = project->config;
    request.meta = project->meta;
    asset_window = new VectorProject(this, request);
  } else if (current_asset->type == AssetDefs::Imagery &&
             current_asset->subtype == kProjectSubtype) {
    RasterProjectEditRequest request(AssetDefs::Imagery);
    request.assetname = assetref;
    RasterProjectAsset project(assetref);
    request.config = project->config;
    request.meta = project->meta;
    asset_window = new ImageryProject(this, request);
  } else if (current_asset->type == AssetDefs::Imagery &&
             current_asset->subtype == kMercatorProjectSubtype) {
    RasterProjectEditRequest request(AssetDefs::Imagery);
    request.assetname = assetref;
    MercatorRasterProjectAsset project(assetref);
    request.config = project->config;
    request.meta = project->meta;
    asset_window = new MercatorImageryProject(this, request);
  } else if (current_asset->type == AssetDefs::Terrain &&
             current_asset->subtype == kProjectSubtype) {
    RasterProjectEditRequest request(AssetDefs::Terrain);
    request.assetname = assetref;
    RasterProjectAsset project(assetref);
    request.config = project->config;
    request.meta = project->meta;
    asset_window = new TerrainProject(this, request);
  } else if (current_asset->type == AssetDefs::KML &&
             current_asset->subtype == kProjectSubtype) {
    KMLProjectEditRequest request;
    request.assetname = assetref;
    KMLProjectAsset project(assetref);
    request.config = project->config;
    request.meta = project->meta;
    asset_window = new KMLProject(this, request);
  }

  if (asset_window) {
    HandleNewWindow(asset_window);
  }
}

void AssetManager::refresh() {
  // int row = assetTableView->currentRow();

  // remember which folder is selected
  QString save_path;
  AssetFolder* item = dynamic_cast<AssetFolder*>(categories->selectedItem());
  if (item != NULL) {
    save_path = item->getFolder().relativePath();
  }

  // now reselect the folder
  if (save_path.isEmpty()) {
    RedrawTree(true);
  } else {
    RedrawTree(false);
    OpenFolder(save_path);
  }
}

void AssetManager::assetsChanged(const AssetChanges& changes) {
  // figure out the currently selected directory
  QString currpath;
  AssetFolder* item = dynamic_cast<AssetFolder*>(categories->selectedItem());
  if (item != NULL) {
    currpath = item->getFolder().relativePath();
  }
  if (currpath.isEmpty()) {
    currpath = ".";
  }

  // convert it to std::string only once for speed
  std::string curr = currpath.toStdString();

  // check to see if any of the changes are in this directory
  std::set<std::string> changedHere;

  for(const auto& i : changes.items) {
    if (khDirname(i.ref) == curr) {
      changedHere.insert(AssetVersionRef(i.ref).AssetRef());
    }
  }

  if (changedHere.size()) {
    gstAssetFolder folder = GetSelectedFolder();
    if (assetTabWidget->currentPageIndex() == 0) {
      TrackChangesInTableView(folder, changedHere);
    } else {
      UpdateIconView(folder);
    }
  }
}


void AssetManager::NewFolder(const QString& folder_name) {
  assert(theAssetManager != NULL);

  bool ok;
  QString text = QInputDialog::getText(tr("New Subfolder"),
                                       tr("Name:"),
                                       QLineEdit::Normal, QString::null,
                                       &ok, this);
  if (!ok || text.isEmpty())
    return;

  if (text.contains(QRegExp("[\\W+]"))) {
      QMessageBox::warning(this, "Warning",
                           tr("Invalid directory name: \n") +
                           text + QString("\n\n") +
                           tr("Please use only letters, numbers and "
                              "underscores."),
                           tr("OK"), 0, 0, 1);
      return;
  }

  QListViewItem* item;

  if (folder_name.isEmpty()) {
    item = categories->firstChild();
  } else {
    item = OpenFolder(gstAssetFolder(folder_name).relativePath());
  }

  if (item == NULL) {
    QMessageBox::warning(this, "Warning",
                         tr("Unable to create new directory: \n") +
                         folder_name + "/" + text + QString("\n\n") +
                         tr("Please confirm directory path in shell."),
                         tr("OK"), 0, 0, 1);
    return;
  }

  AssetFolder* asset_folder = dynamic_cast<AssetFolder*>(item);
  if (!asset_folder)
    return;

  QString error;
  AssetFolder* new_folder = asset_folder->newFolder(text, error);

  if (!new_folder) {
    if (error.isEmpty()) {
      // This error is because we have a problem making the widgetry
      QMessageBox::warning(this, "Warning",
                           tr("Internal error while creating directory"),
                           tr("OK"), 0, 0, 1);
    } else {
      // This is where we will see permissions and other filesytem errors
      QMessageBox::warning(this, "Warning",
                           error, tr("OK"), 0, 0, 1);
    }
    return;
  }

  categories->setSelected(new_folder, true);
}


QListViewItem* AssetManager::OpenFolder(const QString& folder) {
  // this will always be the root item
  QListViewItem* item = categories->firstChild();

  QStringList list = QStringList::split('/', folder);

  for (QStringList::Iterator it = list.begin(); it != list.end(); ++it) {
    item = item->firstChild();
    if (item == 0)
      return NULL;
    while (item->text(0) != *it) {
      item = item->nextSibling();
      if (item == 0)
        return NULL;
    }
    item->setOpen(true);
  }

  categories->setSelected(item, true);
  return item;
}

gstAssetFolder AssetManager::GetSelectedFolder() const {
  AssetFolder* asset_folder =
      dynamic_cast<AssetFolder*>(categories->selectedItem());
  if (asset_folder)
    return asset_folder->getFolder();

  return gstAssetFolder(QString::null);
}

void AssetManager::selectFolder() {
  gstAssetFolder folder = GetSelectedFolder();
  if (!folder.isValid())
    return;

  locationLineEdit->setText(AssetRoot() + "/" + folder.relativePath());

  if (assetTabWidget->currentPageIndex() == 0) {
    UpdateTableView(folder);
  } else {
    UpdateIconView(folder);
  }
}

void AssetManager::UpdateTableItem(int row, gstAssetHandle handle,
                                   Asset asset) {
  // Only set the AssetTableItem if it doesn't exist yet (new items)
  // If we're doing an assetsChanged update, the AssetTableItem can stay
  // the same.
  QTableItem *prev = assetTableView->item(row, 0);

  if (!prev) {
    AssetTableItem* ati = new AssetTableItem(assetTableView, handle);
    assetTableView->setItem(row, 0, ati);
  }

  // now set the rest of the columns
  int col = 1;
  std::string category = asset->PrettySubtype();
  if (category == kMercatorProductSubtype) {
    category = kResourceSubtype;
  }
  assetTableView->setText(row, col++, category.c_str());

  {
    std::string providerstr;
    if ((asset->subtype == kProductSubtype) &&
        ((asset->type == AssetDefs::Imagery) ||
         (asset->type == AssetDefs::Terrain))) {
      RasterProductAsset prod = asset;
      providerstr = GetProviderById(prod->config.provider_id_);
    } else if ((asset->subtype == kMercatorProductSubtype) &&
               (asset->type == AssetDefs::Imagery)) {
      MercatorRasterProductAsset prod = asset;
      providerstr = GetProviderById(prod->config.provider_id_);
    } else if ((asset->subtype == kProductSubtype) &&
               (asset->type == AssetDefs::Vector)) {
      VectorProductAsset prod = asset;
      providerstr = GetProviderById(prod->config.provider_id_);
    } else {
      providerstr = asset->meta.GetValue("provider").toUtf8().constData();
    }
    assetTableView->setText(row, col++, providerstr.c_str());
  }

  AssetVersion version(asset->CurrVersionRef());
  if (version) {
    assetTableView->setText(row, col++,
                            version->meta.GetValue("createdtime"));
    assetTableView->setItem(row, col++,
                            new AssetStateItem(assetTableView,
                                               version->PrettyState().c_str()));
  } else {
    assetTableView->setText(row, col++, "None");
    assetTableView->setItem(row, col++,
                            new AssetStateItem(assetTableView, "None"));
  }
  assetTableView->adjustRow(row);

  for (int i = 0; i < assetTableView->numRows(); ++i) {
      std::string aname {
        assetTableView->GetItem(i)->GetAssetHandle()
        ->getAsset()->GetRef().toString().c_str() };

    int bpos = aname.rfind('/') + 1, epos = aname.rfind('.');
    aname = aname.substr(bpos,epos-bpos);

    if (aname != assetTableView->GetItem(i)->text().toStdString())
    {
        assetTableView->GetItem(i)->setText(aname.c_str());
    }
  }
}

void AssetManager::TrackChangesInTableView(
    const gstAssetFolder &folder, const std::set<std::string> &changed) {
  // process each changed assetRef

  // one
  for (const auto& ref : changed) {
    QString baseRef = khBasename(ref).c_str();
    bool found = false;
    // try to find a match in the existing items
    for (int row = 0; row < assetTableView->numRows(); ++row) {
      AssetTableItem* item = assetTableView->GetItem(row);
      gstAssetHandle handle = item->GetAssetHandle();
      if (handle->getName() == baseRef) {
        found = true;

        // Will refetch record from disk if it has changed
        // NOTE: Using NFS on linux can cause a race here. The system
        // manager writes the record and then sends me a message. I look at
        // the file on disk but it hasn't changed yet. :-(
        Asset asset = handle->getAsset();

        if (!IsAssetVisible(asset)) {
          assetTableView->removeRow(row);
        } else {
          UpdateTableItem(row, handle, asset);
        }
        break;
      }
    }

    if (!found) {
      // no match found. This is a new one
      gstAssetHandle handle = gstAssetHandleImpl::Create(folder, baseRef);
      Asset asset = handle->getAsset();

      if (!IsAssetVisible(asset))
        continue;

      int numRows = assetTableView->numRows();
      assetTableView->setNumRows(numRows + 1);
      UpdateTableItem(numRows, handle, asset);
    }
  }

  // resort in case we just changed a sort key
  // This mostly does the right thing by preserving scroll position, etc
  // But it can end up changing the selection
  assetTableView->sortColumn
    (assetTableView->horizontalHeader()->sortIndicatorSection(),
     assetTableView->horizontalHeader()->sortIndicatorOrder() == Qt::AscendingOrder,
     true /* whole rows */);
}

void AssetManager::UpdateTableView(const gstAssetFolder& folder) {
  // reset table
  assetTableView->setNumCols(0);
  assetTableView->setNumRows(0);

  if (!folder.isValid())
    return;

  assetTableView->setNumCols(5);
  QHeader* header = assetTableView->horizontalHeader();
  int col = 0;

  header->setLabel(col++, tr("Asset Name"));
  header->setLabel(col++, tr("Category"));
  header->setLabel(col++, tr("Provider"));
  header->setLabel(col++, tr("Current Version"));
  header->setLabel(col++, tr("Current State"));

  // put directories here
  // std::vector<gstAssetGroups>

  std::vector<gstAssetHandle> items = folder.getAssetHandles();

  if (items.empty()) {
    return;
  }

  assetTableView->setUpdatesEnabled(false);

  int rowcount = 0;

  for (const auto& item : items) {
    gstAssetHandle handle = item;
    Asset asset = handle->getAsset();
    bool visible = IsAssetVisible(asset);

    if (!visible)
      continue;

    assetTableView->setNumRows(rowcount + 1);
    UpdateTableItem(rowcount++, handle, asset);
  }

  assetTableView->sortColumn(1, true, true);
  assetTableView->horizontalHeader()->setSortIndicator(1, true);

  assetTableView->setUpdatesEnabled(true);

  for (int c = 0; c < assetTableView->numCols(); ++c)
    assetTableView->adjustColumn(c);

  repaint();
}

void AssetManager::filterType(int menuid) {
  if (filter_type_ != menuid) {
    filter_type_ = menuid;
    selectFolder();
  }
}

void AssetManager::filterSubType(int menuid) {
  if (filter_subtype_ != menuid) {
    filter_subtype_ = menuid;
    selectFolder();
  }
}

bool AssetManager::IsAssetVisible(const Asset& asset) const {
  // skip invalid assets
  if (asset->type == AssetDefs::Invalid)
    return false;

  std::string category = asset->PrettySubtype();
  if (category == kMercatorProductSubtype) {
    category = kResourceSubtype;
  }
  // check filters
  if (!MatchFilter(asset->type, category))
    return false;

  // if the "show hidden" check is off, skip any asset that
  // has the "hidden" meta field set
  if (!showHiddenCheck->isChecked()) {
    bool hidden = asset->meta.GetAs("hidden", false);
    if (hidden)
      return false;
  }

  return true;
}

bool AssetManager::MatchFilter(AssetDefs::Type type,
                               const std::string& subtype) const {
  if (GetFusionProductType() == FusionLT) {
    if (type == AssetDefs::Imagery || type == AssetDefs::Terrain)
      return false;
  }

  // quick exit if not filtering
  if (filter_type_ == 0 && filter_subtype_ == 0)
    return true;

  const std::string* subtype_ptr = &subtype;
  // special-case the new maps database
  if (type == AssetDefs::Database && (subtype == kMapDatabaseSubtype ||
                                      subtype == kMercatorMapDatabaseSubtype)) {
    type = AssetDefs::Map;
    subtype_ptr = &kDatabaseSubtype;
  }
  if (type == AssetDefs::Imagery) {
    if (subtype == kMercatorProductSubtype) {
      subtype_ptr = &kProductSubtype;
    } else if (subtype == kMercatorProjectSubtype) {
      subtype_ptr = &kProjectSubtype;
    }
  }

  // check major type
  if (filter_type_ != 0 && TypeSet[filter_type_] != type)
    return false;

  // now check minor type (subtype)
  if (filter_subtype_ != 0 && SubTypeSet[filter_subtype_] != *subtype_ptr)
    return false;

  return true;
}

void AssetManager::UpdateIconView(const gstAssetFolder& folder) {
  assetIconView->clear();

  if (!folder.isValid())
    return;

  std::vector<gstAssetHandle> items = folder.getAssetHandles();

  if (items.empty())
    return;

  for (const auto& item : items) {
    Asset asset = item->getAsset();

    if (IsAssetVisible(asset))
      new AssetIcon(assetIconView, item, icon_grid_);
  }

  resetIconView(icon_grid_);
}

void AssetManager::resetIconView(int grid_size) {
  assetIconView->setGridX(grid_size + 20);
  assetIconView->setGridY(grid_size + 20);
  assetIconView->arrangeItemsInGrid();
}

void AssetManager::resizeIconView(int sz) {
  if (icon_grid_ != sz) {
    icon_grid_ = sz;

    for (QIconViewItem* item = assetIconView->firstItem();
         item; item = item->nextItem()) {
      static_cast<AssetIcon*>(item)->resize(sz);
    }
    resetIconView(sz);
  }
}


QColorGroup AssetManager::GetStateDrawStyle(
    const std::string& txt, QPainter* p, const QColorGroup& cg) {
  bool bold = false;
  QColor clr = cg.text();

  if (txt == "Canceled" || txt == "Failed") {
    bold = true;
    clr = QColor(255,0,0);
  } else if (txt == "Blocked") {
    bold = true;
    clr = QColor(255, 162, 0);  // orange
  } else if (txt == "Succeeded") {
    bold = true;
  } else if (txt == "Queued" || txt == "InProgress" || txt == "Waiting") {
    bold = true;
    clr = QColor(0, 170, 0);    // green
  }

  QFont f = p->font();
  f.setBold(bold);
  p->setFont(f);

  QColorGroup ngrp = cg;
  ngrp.setColor(QColorGroup::Text, clr);

  return ngrp;
}

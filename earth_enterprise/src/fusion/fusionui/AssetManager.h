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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_ASSETMANAGER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_ASSETMANAGER_H_

#include <string>
#include <vector>
#include <set>
#include <map>
#include <Qt/qobjectdefs.h>
#include <Qt/q3iconview.h>
using QIconViewItem = Q3IconViewItem;
#include <Qt/qstringlist.h>
#include <Qt/qaction.h>
#include <Qt/qthread.h>
#include <Qt/qtimer.h>
#include <Qt/q3popupmenu.h>
using QPopupMenu = Q3PopupMenu;
#include <Qt/q3listview.h>
using QListViewItem = Q3ListViewItem;
#include <Qt/qobject.h>
#include "fusion/autoingest/.idl/storage/AssetDefs.h"
#include "fusion/fusionui/.idl/layoutpersist.h"
#include "fusion/fusionui/.ui/assetmanagerbase.h"

#include "fusion/fusionui/Preferences.h"
#include "fusion/gst/gstAssetGroup.h"

#define ASSET_MANAGER  0x00ff0001
#define ASSET_FOLDER   0x00ff0002

#define DEFAULT_WINDOW_HEIGHT 646
#define DEFAULT_WINDOW_WIDTH 816

class AssetBase;
class AssetChanges;
class geGuiProgress;
class geGuiAuth;
class PublisherClient;
class QProgressDialog;

class AssetAction : public QAction {
  Q_OBJECT

 public:
  AssetAction(QObject* parent, AssetBase* base);
  QString Name() const;

  static AssetAction* FindAsset(const QString& txt);
  static bool OkToCloseAll();
  static void CloseAll();
  static void Cascade(const QPoint& start_pos);

 public slots:
  void NameChanged(const QString& txt);
  void Cleanup();

 private:
  AssetBase* asset_window_;
  static std::vector<AssetAction*> all_actions;
};

// -----------------------------------------------------------------------------

class AssetManager : public AssetManagerBase {
  Q_OBJECT

 public:
  AssetManager(QWidget* parent);
  ~AssetManager();

  static AssetManager* self;

  bool OkToQuit();
  static QString AssetRoot();

  // layout management
  static AssetManagerLayout::Size GetLayoutSize(const QString& name);
  static void SetLayoutSize(const QString& name, int width, int height);

  // asset chooser
  void addFilter(const QString& f);

  QListViewItem* OpenFolder(const QString& folder);
  void resetIconView(int grid_size);

  static QColorGroup GetStateDrawStyle(const std::string& txt, QPainter* p,
                                       const QColorGroup& cg);
  // inherited from QWidget
  virtual void show();
  virtual void hide();
  void ShowIcons();
  void HideIcons();

  // inherited from assetmanagerbase
  virtual void addAsset();
  virtual void resizeIconView(int sz);
  virtual void selectFolder();
  virtual void refresh();
  virtual void filterType(int t);
  virtual void filterSubType(int t);
  virtual void childCollapsed(QListViewItem* i);
  virtual void showHiddenAssets(bool s);
  virtual void CreateNewAsset();
  virtual void CloseAllWindows();
  virtual void CascadeWindows();

  virtual void NewVectorAsset();
  virtual void NewVectorLayer();
  virtual void NewVectorProject();
  virtual void NewImageryAsset();
  virtual void NewMercatorImageryAsset();
  virtual void NewImageryProject();
  virtual void NewMercatorImageryProject();
  virtual void NewTerrainAsset();
  virtual void NewTerrainProject();
  virtual void NewMapLayer();
  virtual void NewMapProject();
  virtual void NewMapDatabase();
  virtual void NewMercatorMapDatabase();
  virtual void NewKMLProject();
  virtual void NewDatabase();
  virtual void ToolbarChooserComboActivated(int choice);

 public slots:
  void rmbClicked(Q3ListViewItem* item, const QPoint& pos, int);
  void tableAssetMenu(int row, int col, const QPoint& mouse_pos);
  void iconAssetMenu(Q3IconViewItem* item, const QPoint& mouse_pos);
  void doubleClicked(int row, int col, int btn, const QPoint& mouse_pos);
  void assetsChanged(const AssetChanges& a);
  void CurrentAssetChanged(int row, int col);

  // popup-menu items
  void BuildAsset(const gstAssetHandle& handle);
  void CancelAsset(const gstAssetHandle& handle);
  void ModifyAsset(const gstAssetHandle& handle);
  void ShowAssetProperties(const gstAssetHandle& handle);
  void ShowCurrVerProperties(const gstAssetHandle& handle);
  void PushDatabase(const gstAssetHandle& handle);
  void PublishDatabase(const gstAssetHandle& handle);

 private:
  // inherited from qwidget
  virtual void resizeEvent(QResizeEvent* e);
  virtual void moveEvent(QMoveEvent* e);

  gstAssetHandle GetAssetByRow(int row);
  gstAssetFolder GetSelectedFolder() const;
  void ShowAssetMenu(const gstAssetHandle& handle, const QPoint& mouse_pos);
  void RedrawTree(bool selectTop);

  // context menu selects these
  void NewFolder(const QString& f);

  void UpdateTableItem(int row, gstAssetHandle handle, Asset asset);
  void TrackChangesInTableView(const gstAssetFolder& folder,
                               const std::set<std::string> &changed);
  void UpdateTableView(const gstAssetFolder& f);
  void UpdateIconView(const gstAssetFolder& f);

  bool IsAssetVisible(const Asset& asset) const;
  bool MatchFilter(AssetDefs::Type, const std::string& s) const;

  void HandleNewWindow(AssetBase* asset_window);
  bool RestoreExisting(const std::string& asset_ref);

  std::string GetProviderById(std::uint32_t id);

  void RemoveToolBarIcons();

  AssetManagerLayout layout_persist_;
  int icon_grid_;
  int filter_type_;
  int filter_subtype_;
  typedef std::map<std::uint32_t, std::string> ProviderMap;
  ProviderMap provider_map_;
  PrefsConfig::ShowAssetManagerIconType asset_manager_icon_choice_;
  const PrefsConfig::ShowAssetManagerIconType asset_manager_icon_choice_orig_;
};

// Thread classes for serving pushing and publishing.
class ServeThread : public QThread {
  //Q_OBJECT already present in QThread
  Q_OBJECT

 public:
  virtual ~ServeThread();

  void start();

  bool retval() { return retval_; }

 signals:
  void sfinished();

 protected:
  ServeThread(PublisherClient* publisher_client,
              const std::string& gedb_path);

  PublisherClient* const publisher_client_;
  std::string gedb_path_;
  bool retval_;
};

class PushThread : public ServeThread {
 public:
  PushThread(PublisherClient* publisher_client,
             const std::string& gedb_path);

 protected:
  virtual void run(void);

 private:
  geGuiProgress* progress_;
};

class PublishThread : public ServeThread {
 public:
  PublishThread(PublisherClient* publisher_client,
                const std::string& gedb_path,
                const std::string& target_path);

 protected:
  virtual void run(void);

 private:
  std::string target_path_;
};

// Pushing/Publishing serving assistant.
// When serving Pushing/Publishing (long time operation), updates data in
// ProgressDialog-object and allows to handle authentication request from
// other thread.
class ServeAssistant : public QObject {
  Q_OBJECT

 public:
  ServeAssistant(QProgressDialog* progress_dialog,
                 geGuiProgress* progress,
                 geGuiAuth* auth);

  virtual ~ServeAssistant();

  // Starts timer triggering Perform() on timeout.
  void Start();

 public slots:
  // Handler for cancel pushing/publishing.
  void SetCanceled();
  // Stops timer.
  void Stop();

 private slots:
  // Do work - updates data in ProgressDialog object and allows to handle
  // authentication request from other thread.
  void Perform();

 private:
  QProgressDialog* const progress_dialog_;
  geGuiProgress* const progress_;
  geGuiAuth* const auth_;
  QTimer timer_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_ASSETMANAGER_H_

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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_MAINWINDOW_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_MAINWINDOW_H_
#include <Qt/qobjectdefs.h>
#include "common/khArray.h"
#include "common/notify.h"
#include "fusion/gst/gstTypes.h"
#include "fusion/gst/gstBBox.h"
#include <Qt/qglobal.h>
#include <Qt/qobject.h>
#include <Qt/qaction.h>

#include "mainwindowbase.h"

class QLabel;
class QProgressBar;

class ProjectManager;
class ProjectDocker;
class GfxView;
class PlacemarkManager;
class SelectionView;
class SelectionViewDocker;
class AssetManager;
class FeatureEditor;
class SystemManager;

class NameAction : public QAction {
  Q_OBJECT
 public:
  NameAction(QObject* parent);

 public slots:
  void ChangeText(const QString& text);
};

// -----------------------------------------------------------------------------

class MainWindow : public MainWindowBase {
  Q_OBJECT

 public:
  MainWindow(QWidget* parent = 0, const char* name = 0,
             Qt::WFlags fl = Qt::Window);
  ~MainWindow();

  void Init();

  bool OkToQuit();

  void setupSelectionView();

  void updatePlaceMarks();
  gstStatus updateImageLayers();

  static MainWindow *self;

 signals:
  void zoomToSource(const gstBBox &);
  void prefsChanged();

 public slots:
  // file menu
  virtual void fileOpen();
  virtual void fileExit();

  // Tools Menu
  virtual void ShowFeatureEditor();
  virtual void assetManager();
  virtual void manageIcons();
  virtual void manageProviders();
  virtual void manageLocales();
  virtual void openPublisher();
  virtual void systemManager();

  // view menu
  virtual void viewSelection();
  virtual void viewOpenFiles();
  virtual void viewProjectManager();

  virtual void addPlaceMark();
  virtual void managePlacemarks();
  virtual void selectPlaceMark(int idx);

  // edit menu
  virtual void editPrefs();

  // help menu
  virtual void helpAbout();
  virtual void launchHelpManual();
  // choice can be either 0 (flat projection) or 1 (mercator projection)
  virtual void previewProjectionActivated(int choice);

  void latlongUpdate(double lat, double lon);
  void drawstats(int frame, double tex, double geom);

  void busyProgress(int val);

  void gfxviewContextMenu(QWidget *parent, const QPoint &pos);

  void fileDragOpen(const QString &str);

  void show();

 private:
  void saveScreenLayout();
  QString getManualPath();

  PlacemarkManager* placemark_manager_;

  ProjectDocker* project_manager_docker_;
  ProjectManager* preview_project_manager_;

  AssetManager* asset_manager_;
  FeatureEditor* feature_editor_;

  SystemManager* system_manager_;

  SelectionViewDocker* selection_view_docker_;

  QLabel* lat_lon_;
  QLabel* draw_stats_;

  QProgressBar* busy_progress_bar_;
  int busy_progress_max_;

  // inherited from qwidget
  virtual void closeEvent(QCloseEvent* event);
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_MAINWINDOW_H_

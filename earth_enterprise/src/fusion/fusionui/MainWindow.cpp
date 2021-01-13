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


#include <stdio.h>
#include <stdlib.h>

#include <Qt/qpainter.h>
#include <Qt/qinputdialog.h>
#include <Qt/qcombobox.h>
#include <Qt/qstatusbar.h>
#include <Qt/qapplication.h>
#include <Qt/qlayout.h>
#include <Qt/qlcdnumber.h>
#include <Qt/qspinbox.h>
#include <Qt/q3vbox.h>
#include <Qt/q3dockwindow.h>
using QDockWindow = Q3DockWindow;
#include <Qt/qmessagebox.h>
#include <Qt/q3popupmenu.h>
using QPopupMenu = Q3PopupMenu;
#include <Qt/qmenubar.h>
#include <Qt/qlabel.h>
#include <Qt/qprogressbar.h>
#include <Qt/q3dragobject.h>
#include <Qt/qpushbutton.h>
#include <Qt/qfile.h>
#include <Qt/qtextstream.h>
#include <Qt/q3mimefactory.h>
using QMimeSourceFactory = Q3MimeSourceFactory;
#include <Qt/q3dragobject.h>
using QImageDrag = Q3ImageDrag;

#include <builddate.h>
#include "fusion/fusionversion.h"

#include "fusion/fusionui/Preferences.h"
#include "fusion/fusionui/ProjectDocker.h"
#include "fusion/fusionui/ProjectManager.h"
#include "fusion/fusionui/MainWindow.h"
#include "fusion/fusionui/GfxView.h"
#include "fusion/fusionui/PlacemarkManager.h"
#include "fusion/fusionui/SelectionView.h"
#include "fusion/fusionui/IconManager.h"
#include "fusion/fusionui/AssetManager.h"
#include "fusion/fusionui/FeatureEditor.h"
#include "fusion/fusionui/SystemManager.h"
#include "fusion/fusionui/ProviderManager.h"
#include "fusion/fusionui/Publisher.h"
#include "fusion/fusionui/LocaleManager.h"
#include "fusion/fusionui/AboutFusion.h"

#include "fusion/gst/gstTextureManager.h"
#include "fusion/autoingest/.idl/Systemrc.h"
#include "common/geInstallPaths.h"

static QPixmap LoadPixmap(const QString& name) {
  const QMimeSource* m = QMimeSourceFactory::defaultFactory()->data(name);
  if (!m)
    return QPixmap();
  QPixmap pix;
  QImageDrag::decode(m, pix);
  return pix;
}

class QPainter;

// -----------------------------------------------------------------------------

NameAction::NameAction(QObject* parent)
  : QAction(parent, 0) {
}

void NameAction::ChangeText(const QString& text) {
  if (text.isEmpty()) {
    setText(QString("whatever!"));
  } else {
    setText(text);
  }
}

// -----------------------------------------------------------------------------

// globally visible structures
MainWindow* MainWindow::self = 0;

MainWindow::MainWindow(QWidget* parent, const char* name, Qt::WFlags fl)
    : MainWindowBase(parent, name, fl),
      placemark_manager_(),
      project_manager_docker_(),
      preview_project_manager_(),
      asset_manager_(),
      feature_editor_(),
      system_manager_(),
      selection_view_docker_(),
      lat_lon_(),
      draw_stats_(),
      busy_progress_bar_(),
      busy_progress_max_() {
  self = this;
}

void MainWindow::Init() {
  //
  // set all global values here...
  //
  setCaption(QString(GetFusionProductName().c_str()) + Preferences::CaptionText());

  asset_manager_ = NULL;
  feature_editor_ = NULL;

  system_manager_ = NULL;

  statusBar()->setSizeGripEnabled(false);
  lat_lon_ = new QLabel(statusBar());
  lat_lon_->setFixedWidth(210);
  statusBar()->addWidget(lat_lon_, 0, true);

  busy_progress_bar_ = new QProgressBar(statusBar());
  busy_progress_bar_->setTextVisible(FALSE);
  busy_progress_bar_->setMinimum(0);
  busy_progress_bar_->setMaximum(10);
  busy_progress_bar_->setFixedWidth(150);
  statusBar()->addWidget(busy_progress_bar_, 0, true);
  busy_progress_max_ = 10;

  if (Preferences::GlobalEnableAll) {
    draw_stats_ = new QLabel(statusBar());
    statusBar()->addWidget(draw_stats_, 0, true);
  } else {
    draw_stats_ = NULL;
  }

  // All project managers are contained in this dockwidget
  project_manager_docker_ = new ProjectDocker(QDockWindow::InDock, this,
                                              "Preview List");
  addToolBar(project_manager_docker_, Qt::DockLeft);
  preview_project_manager_ = project_manager_docker_->Preview();

  setupSelectionView();

  placemark_manager_ = NULL;
  updatePlaceMarks();
  updateImageLayers();

  // enable the help menu item if docs are installed
  if (!getManualPath().isEmpty()) {
    helpmanual->setVisible(TRUE);
  }

  // provide a contrast for the level view lcd number since it seems to not
  // be setting properly in the ui file
  levelView->setStyleSheet("background-color: black");

  // handle drop events
  connect(gfxview, SIGNAL(dropFile(const QString&)),
          this, SLOT(fileDragOpen(const QString&)));
  connect(this, SIGNAL(zoomToSource(const gstBBox&)),
          gfxview, SLOT(zoomToBox(const gstBBox&)));

  connect(gfxview, SIGNAL(selectBox(const gstDrawState&, Qt::ButtonState)),
      project_manager_docker_,
      SLOT(selectBox(const gstDrawState&, Qt::ButtonState)));

  // ---------------------------------------------------------------------------

  if (Preferences::GlobalEnableAll) {
    connect(preview_project_manager_, SIGNAL(statusMessage(const QString&)),
            statusBar(), SLOT(message(const QString&)));
  }

  // ---------------------------------------------------------------------------

  connect(editEnableAllAction, SIGNAL(activated()),
          project_manager_docker_, SLOT(enableAllLayers()));
  connect(editDisableAllAction, SIGNAL(activated()),
          project_manager_docker_, SLOT(disableAllLayers()));

  connect(editExpandAllAction, SIGNAL(activated()),
          project_manager_docker_, SLOT(expandAllLayers()));
  connect(editCollapseAllAction, SIGNAL(activated()),
          project_manager_docker_, SLOT(collapseAllLayers()));

  // ---------------------------------------------------------------------------

  if (Preferences::GlobalEnableAll) {
    connect(gfxview, SIGNAL(drawstats(int, double, double)),
            this, SLOT(drawstats(int, double, double)));
  }

  connect(gfxview, SIGNAL(renderbusy(int)),
          this, SLOT(busyProgress(int)));

  connect(gfxview, SIGNAL(latlonChanged(double, double)),
          this, SLOT(latlongUpdate(double, double)));

  connect(this, SIGNAL(prefsChanged()),
          gfxview, SLOT(ReloadPrefs()));

  connect(gfxview, SIGNAL(contextMenu(QWidget*, const QPoint &)),
          this, SLOT(gfxviewContextMenu(QWidget*, const QPoint&)));

  // ---------------------------------------------------------------------------

  connect(preview_project_manager_, SIGNAL(selectionView(gstSelector*)),
        selection_view_docker_->selectionView(), SLOT(configure(gstSelector*)));

  // ---------------------------------------------------------------------------

  if (Preferences::GlobalEnableAll) {
    QAction* toggleDebugAction = new QAction(this, "toggleDebugAction");
    toggleDebugAction->setText(tr("Toggle Debug"));
    toggleDebugAction->setAccel(tr("Ctrl+Shift+D"));
    toggleDebugAction->addTo(view_menu);
    connect(toggleDebugAction, SIGNAL(activated()),
            gfxview, SLOT(toggleDebug()));
  }

  if (!Preferences::ExperimentalMode) {
    delete edit_tool;
    edit_tool = NULL;
    delete featureEditorAction;
    featureEditorAction = NULL;
  }

  /*
  assetManagerAction
  placemarkManagerAction
  featureEditorAction
  iconAction
  localeManagerAction
  providerManagerAction
  toolsPublisherAction
  systemManagerAction
  */

  if (Preferences::ExperimentalMode) {
    toolsPublisherAction->removeFrom(tools_menu);

    QPopupMenu* admin_menu = new QPopupMenu(this);
    toolsPublisherAction->addTo(admin_menu);
    menubar->insertItem(tr("&Admin"), admin_menu, 7, 3);
  }
  previewProjection->setCurrentItem(
      Preferences::getConfig().isMercatorPreview ? 1 : 0);
}
MainWindow::~MainWindow() {
  delete project_manager_docker_;
  delete selection_view_docker_;

  delete asset_manager_;
  delete feature_editor_;

  delete system_manager_;
}

void MainWindow::previewProjectionActivated(int choice) {
  bool is_mercator_preview = (choice != 0);
  // Whenever preview projection is changed, update the preference and also save
  // it in file so as to remember last preview projection.
  if (is_mercator_preview != Preferences::getConfig().isMercatorPreview) {
    Preferences::getConfig().isMercatorPreview = is_mercator_preview;
    GfxView::SetIsMercatorPreview(Preferences::getConfig().isMercatorPreview);
    Preferences::getConfig().Save(Preferences::filepath("preferences.xml").toUtf8().constData());
    updateImageLayers();  // Read the background texture (for this projection).
    {  // Adjust the frustum so that the center of map is same lat-lon
      gstBBox& frust = gfxview->state()->frust;
      double center_latitude = (frust.n + frust.s) / 2.0;
      double new_center_latitude = Preferences::getConfig().isMercatorPreview ?
        MercatorProjection::
            FromNormalizedLatitudeToMercatorNormalizedLatitude(center_latitude):
        MercatorProjection::
            FromMercatorNormalizedLatitudeToNormalizedLatitude(center_latitude);
      double adjust_center = new_center_latitude - center_latitude;
      frust.n += adjust_center;
      frust.s += adjust_center;
    }
    emit prefsChanged();
    gfxview->updateGL();  // Redraw the background.
  }
}

QString MainWindow::getManualPath() {
  QString doc(khComposePath(
      kGESharePath, "doc/manual/index.html").c_str());
  if (!khExists(doc.toUtf8().constData())) {
    return QString();
  }
  else {
    return doc;
  }
}

void MainWindow::launchHelpManual() {
  // find manual
  QString doc = getManualPath();
  if (doc.isEmpty()) {
    QMessageBox::critical(
        this, "Error",
        tr("Unable to find manual."),
        tr("OK"), 0, 0, 0);
    return;
  }

  // find browser
  QString browser;
  QStringList mpath;
  mpath << "/usr/bin/firefox";
  mpath << "/usr/bin/google-chrome";

  for (QStringList::Iterator it = mpath.begin(); it != mpath.end(); ++it) {
    if (khExists(it->toUtf8().constData())) {
      browser = *it;
      break;
    }
  }

  if (browser.isEmpty()) {
    QMessageBox::critical(this, "Error",
                          tr("Unable to find a suitable browser.  "
                             "Please install Firefox or Chrome."),
                          tr("OK"), 0, 0, 0);
    return;
  }

  // ensure shell is bourne so we can redirect stderr
  setenv("SHELL", "/bin/sh", 1);
  QString cmd = browser + " " + doc + " 2> /dev/null &";
  system(cmd.latin1());
}

void MainWindow::helpAbout() {
  AboutFusion about(this);
  about.exec();
}

void MainWindow::gfxviewContextMenu(QWidget* parent, const QPoint& pos) {
  QPopupMenu menu(parent);

  select_tool->addTo(&menu);
  zoombox_tool->addTo(&menu);
  zoomdrag_tool->addTo(&menu);
  pan_tool->addTo(&menu);

  if (Preferences::ExperimentalMode) {
    edit_tool->addTo(&menu);
  }

  menu.exec(pos);
}


void MainWindow::show() {
  QFile file(Preferences::filepath("screen.layout"));
  if (file.open(IO_ReadOnly)) {
    QTextStream stream(&file);
    stream >> *this;
    file.close();
  }

  if (khExists(Preferences::filepath("mainwindow.layout").latin1())) {
    LayoutPersist lp;
    if (lp.Load(Preferences::filepath("mainwindow.layout").latin1())) {
      resize(lp.width, lp.height);
      move(lp.xpos, lp.ypos);
    }
  }

  MainWindowBase::show();

  // create these tools now so that they can be made visible
  // based on the preferences
  if (asset_manager_ == NULL)
    asset_manager_ = new AssetManager(0);

  if (Preferences::ExperimentalMode) {
    if (feature_editor_ == NULL)
      feature_editor_ = new FeatureEditor(0);
  }

  static bool validateGfx = false;
  if (!validateGfx) {
    gfxview->ValidateGfxMode();
    validateGfx = true;
  }
}

void MainWindow::setupSelectionView() {
  selection_view_docker_ = new SelectionViewDocker(QDockWindow::InDock, this,
                                                   "Attribute Table", 0, true);
  addToolBar(selection_view_docker_, Qt::DockBottom);
}


void MainWindow::updatePlaceMarks() {
  if (placemark_manager_ == NULL)
    placemark_manager_ = new PlacemarkManager();

  placeMarkList->clear();
  placeMarkList->insertItem(tr("Favorites..."));
  placeMarkList->insertStringList(placemark_manager_->GetList());
}

class TextureWarning : public QMessageBox {
 public:
  TextureWarning(const QString& message, QWidget* parent)
      : QMessageBox("Warning", message, QMessageBox::Warning,
                    QMessageBox::Ok, QMessageBox::NoButton,
                    QMessageBox::NoButton, parent, 0, false) {
    // must set this flag here because the base QWidget will
    // not honor it if modal is not true
    setWindowFlags(static_cast<Qt::WindowFlags>(Qt::WA_DeleteOnClose));
  }
};

gstStatus MainWindow::updateImageLayers() {
  QString texpath = Preferences::getConfig().GetBackgroundPath();

  // ensure opengl has been initialized before doing any texture work
  gfxview->updateGL();

  gstTextureManager* txmgr = gfxview->textureManager();

  bool is_mercator_imagery;
  gstTextureGuard texture =
      txmgr->NewTextureFromPath(std::string(texpath.latin1()),
                                &is_mercator_imagery);
  QString proj_msg(
      (is_mercator_imagery && !Preferences::getConfig().isMercatorPreview) ?
         "The image is not in Flat Projection.\n" :
      ((!is_mercator_imagery && Preferences::getConfig().isMercatorPreview) ?
         "The image is not in Mercator Projection.\n" : ""));

  if (!texture || !txmgr->AddBaseTexture(texture) || !proj_msg.isEmpty()) {
    TextureWarning* warning = new TextureWarning(
        proj_msg +
        trUtf8("Unable to load background imagery: ") + "\n"
        + texpath + "\n"
        + trUtf8("Reverting to default background."), this);
    warning->show();

    texture = txmgr->NewTextureFromPath(std::string(
        Preferences::getConfig().GetDefaultBackgroundPath().latin1()),
        &is_mercator_imagery);

    if (!texture || !txmgr->AddBaseTexture(texture) ||
        is_mercator_imagery != Preferences::getConfig().isMercatorPreview) {
      TextureWarning* warning = new TextureWarning(
          trUtf8("Unable to load default imagery!\n"
                 "Your Fusion installation is probably corrupted\n"
                 "and might require re-installation."), this);
      warning->show();
      return GST_OPEN_FAIL;
    }
  }

  return GST_OKAY;
}

void MainWindow::fileOpen() {
  preview_project_manager_->FileOpen();
}

void MainWindow::fileDragOpen(const QString& str) {
  QString modstr = ProjectManager::CleanupDropText(str);

  preview_project_manager_->addLayers(modstr.latin1(), NULL);
}

void MainWindow::closeEvent(QCloseEvent* event) {
  event->ignore();
  fileExit();
}

bool MainWindow::OkToQuit() {
  if (asset_manager_ && !asset_manager_->OkToQuit())
    return false;

  if (feature_editor_ != NULL) {
    if (feature_editor_->Close() != true)
      return false;
  }

  return true;
}


void MainWindow::fileExit() {
  if (!OkToQuit())
    return;
  saveScreenLayout();
  QApplication::exit(0);
}

void MainWindow::saveScreenLayout() {
  QFile file(Preferences::filepath("screen.layout"));
  if (file.open(IO_WriteOnly)) {
    QTextStream stream(&file);
    stream << *this;
    file.close();
  }

  LayoutPersist lp;
  lp.width = width();
  lp.height = height();
  lp.xpos = x();
  lp.ypos = y();
  lp.showme = true;
  lp.currentProject = 0;        // obsolete
  lp.Save(Preferences::filepath("mainwindow.layout").latin1());
}

//------------------------------------------------------------------------------

void MainWindow::addPlaceMark() {
  bool ok = FALSE;
  QString text = QInputDialog::getText(tr("New Favorite"),
                                       tr("Favorite name:"),
                                       QLineEdit::Normal,
                                       QString::null, &ok, this);
  if (ok && !text.isEmpty()) {
    placeMarkList->insertItem(text);
    placemark_manager_->Add(gstPlacemark(text, gfxview->CenterLat(),
                                       gfxview->CenterLon(), gfxview->level()));
  }
}

void MainWindow::managePlacemarks() {
  if (placemark_manager_->Manage() == QDialog::Accepted)
    updatePlaceMarks();
}


void MainWindow::selectPlaceMark(int idx) {
  // ignore the first entry, as it is just the label 'Placemarks...'
  if (idx != 0) {
    --idx;
    gstPlacemark pm = placemark_manager_->Get(idx);
    gfxview->setPosition(pm.latitude, pm.longitude, pm.level);
  }
}

void MainWindow::editPrefs() {
  Preferences prefs(this);

  if (prefs.exec() != QDialog::Accepted)
    return;

  updateImageLayers();

  emit prefsChanged();
}

void MainWindow::manageIcons() {
  (new IconManager(this, false, Qt::WDestructiveClose))->show();
}

void MainWindow::manageProviders() {
  (new ProviderManager(this, true, Qt::WDestructiveClose))->exec();
}

void MainWindow::openPublisher() {
  (new Publisher(this, true, Qt::WDestructiveClose))->exec();
}

void MainWindow::manageLocales() {
  (new LocaleManager(this, true, Qt::WDestructiveClose))->exec();
}

void MainWindow::assetManager() {
  asset_manager_->showNormal();
  asset_manager_->raise();
  asset_manager_->refresh();
}

void MainWindow::ShowFeatureEditor() {
  if (Preferences::ExperimentalMode) {
    feature_editor_->showNormal();
    feature_editor_->raise();
  }
}

void MainWindow::systemManager() {
  if (system_manager_ == NULL)
    system_manager_ = new SystemManager();
  system_manager_->showNormal();
  system_manager_->raise();
}

void MainWindow::viewSelection() {
  selection_view_docker_->show();
}

void MainWindow::viewOpenFiles() {
  // _ofDocker->show();
}

void MainWindow::viewProjectManager() {
  project_manager_docker_->show();
}

void MainWindow::latlongUpdate(double lat, double lon) {
  QString ns = (lat >= 0.0) ? "N" : "S";
  QString ew = (lon >= 0.0) ? "E" : "W";
  lat_lon_->setText(QString("%1%2, %3%4")
                    .arg(fabs(lat), 10, 'f', 6).arg(ns)
                    .arg(fabs(lon), 10, 'f', 6).arg(ew));
}

void MainWindow::drawstats(int frame, double tex, double geom) {
  if (draw_stats_) {
    draw_stats_->setText(QString("f:%1 t:%2 g:%3").arg(frame).
                         arg(tex, 0, 'f', 4).arg(geom, 0, 'f', 4));
  }
}

void MainWindow::busyProgress(int val) {
  if (val == 0) {
    //busy_progress_bar_->setProgress(0);
    busy_progress_bar_->setValue(0);
    busy_progress_max_ = 10;
    return;
  }

  if (val > busy_progress_max_)
    busy_progress_max_ = val;

  int percent = static_cast<int>(static_cast<float>(busy_progress_max_ - val) /
                                 static_cast<float>(busy_progress_max_) * 11.0);

  if (percent < 1)  // always draw something!
    percent = 1;

  //busy_progress_bar_->setProgress(percent);
  busy_progress_bar_->setValue(0);
}

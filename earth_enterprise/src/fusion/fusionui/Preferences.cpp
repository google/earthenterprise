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


#include <Qt/qlineedit.h>
#include <Qt/qlabel.h>
#include <Qt/qpushbutton.h>
#include <Qt/qcheckbox.h>
#include <Qt/qcombobox.h>
#include <Qt/qtextcodec.h>
#include <Qt/qfileinfo.h>
#include <Qt/qcolordialog.h>
#include <Qt/qspinbox.h>
#include <Qt/q3widgetstack.h>
#include <Qt/qobject.h>
#include <Qt/qfiledialog.h>
#include <notify.h>
#include <autoingest/.idl/Systemrc.h>
#include <khFileUtils.h>
#include "SourceFileDialog.h"
#include "Preferences.h"
#include "AssetChooser.h"
#include <RuntimeOptions.h>
#include "common/khConstants.h"
#include <fusion/fusionui/GfxView.h>
#include <cstdlib>

// -----------------------------------------------------------------------------
// static access
// -----------------------------------------------------------------------------

bool Preferences::initOnce = false;

bool Preferences::GlobalEnableAll = false;
bool Preferences::ExpertMode = false;
bool Preferences::ExperimentalMode = false;
bool Preferences::GoogleInternal = false;

std::string Preferences::prefsDir;

QString Preferences::filepath(const QString& fname) {
  if (prefsDir.empty())
    fprintf(stderr, "prefsDir is empty!\n");
  std::string fpath { prefsDir };
  fpath.push_back('/');
  fpath += fname.toUtf8().constData();
  return QString(fpath.c_str());
}

PrefsConfig& Preferences::getConfig() {
  static bool loadonce = true;
  static PrefsConfig config;

  if (loadonce) {
    QString prefsPath = Preferences::filepath("preferences.xml");
    if (khExists(prefsPath.toUtf8().constData())) {
      config.Load(prefsPath.toUtf8().constData());
    } else {
      config = getDefaultConfig();
      config.Save(prefsPath.toUtf8().constData());
    }
    loadonce = false;

    GfxView::SetIsMercatorPreview(Preferences::getConfig().isMercatorPreview);
  }
  return config;
}

PrefsConfig Preferences::getDefaultConfig() {
  // start with a default config
  PrefsConfig cfg;

  return cfg;
}

// -----------------------------------------------------------------------------

void Preferences::init() {
  if (initOnce)
    return;

  initOnce = true;

  //
  // establish local user's preference directory
  //
  prefsDir = std::string(getenv("HOME")) + "/.fusion";
  if (!khDirExists(prefsDir)) {
    if (!khMakeDir(prefsDir, 0750)) {
      notify(NFY_WARN, "Unsuccessful creating Fusion resource directory: %s",
             prefsDir.c_str());
      notify(NFY_WARN, "Maybe $HOME is not set correctly? ($HOME = %s)",
             getenv("HOME"));
    }
  }

  GlobalEnableAll  = RuntimeOptions::EnableAll();
  ExpertMode       = RuntimeOptions::ExpertMode();
  ExperimentalMode = RuntimeOptions::ExperimentalMode();
  GoogleInternal   = RuntimeOptions::GoogleInternal();
}

QString Preferences::empty_path("<i>" + kh::tr("empty") + "</i>");

Preferences::Preferences(QWidget* parent)
    : PreferencesBase(parent, 0, false, 0),
      selectOutline(4, 0),
      selectFill(4, 0) {
  // restore saved preferences
  const PrefsConfig& prefsConfig = getConfig();

  // gather all valid codecs from qt and insert in drop-down list
  codecCombo->insertItem(kh::tr("<none>"));
  int count = 0;
  QTextCodec* codec;
  for (; (codec = QTextCodec::codecForIndex(count)); ++count)
    codecCombo->insertItem(codec->name(), 1);

  // find saved codec in list and make it current
  for (int index = 0; index < count; ++index) {
    if (codecCombo->text(index) == prefsConfig.characterEncoding) {
      codecCombo->setCurrentItem(index);
      break;
    }
  }

  // update widgets from config
  default_imagery_index->setText(PrefsConfig::kDefaultImageryIndex);
  imagery_project->setText(prefsConfig.backgroundImageryProject);
  imagery_index->setText(prefsConfig.backgroundTexturePath);
  stream_server->setText(prefsConfig.backgroundStreamServer);

  default_imagery_index_mercator->setText(
      PrefsConfig::kDefaultImageryIndexMercator);
  imagery_project_mercator->setText(
      prefsConfig.backgroundImageryProjectMercator);
  imagery_index_mercator->setText(prefsConfig.backgroundTexturePathMercator);
  if (prefsConfig.backgroundType == PrefsConfig::BuiltinIndex) {
    background_stack->raiseWidget(0);
    background_type->setCurrentItem(0);
  } else if (prefsConfig.backgroundType == PrefsConfig::ImageryProject) {
    background_stack->raiseWidget(1);
    background_type->setCurrentItem(1);
  } else if (prefsConfig.backgroundType == PrefsConfig::ImageryIndex) {
    background_stack->raiseWidget(2);
    background_type->setCurrentItem(2);
  } else if (prefsConfig.backgroundType == PrefsConfig::StreamServer) {
    background_stack->raiseWidget(3);
    background_type->setCurrentItem(3);
  }
  if (prefsConfig.backgroundTypeMercator == PrefsConfig::BuiltinIndex) {
    background_stack_mercator->raiseWidget(0);
    background_type_mercator->setCurrentItem(0);
  } else if (prefsConfig.backgroundTypeMercator ==
             PrefsConfig::ImageryProject) {
    background_stack_mercator->raiseWidget(1);
    background_type_mercator->setCurrentItem(1);
  } else if (prefsConfig.backgroundTypeMercator == PrefsConfig::ImageryIndex) {
    background_stack_mercator->raiseWidget(2);
    background_type_mercator->setCurrentItem(2);
  }

  showFIDCheck->setChecked(prefsConfig.dataViewShowFID);
  autoRaiseCheck->setChecked(prefsConfig.dataViewAutoRaise);
  assetRootCheck->setChecked(prefsConfig.showFullAssetPath);

  selectOutline = prefsConfig.selectOutlineColor;

  QRgb rgba = qRgba(selectOutline[0],
                    selectOutline[1],
                    selectOutline[2],
                    selectOutline[3]);

  QPalette outlinepalette;
  outlinepalette.setColor(QPalette::Button, QColor(rgba));

  outline_color_btn->setAutoFillBackground(true);
  outline_color_btn->setPalette(outlinepalette);
  outline_color_btn->setFlat(true);
  outline_color_btn->update();

  selectFill = prefsConfig.selectFillColor;

  rgba = qRgba(selectFill[0],
               selectFill[1],
               selectFill[2],
               selectFill[3]);

  QPalette fillpalette;
  fillpalette.setColor(QPalette::Button, QColor(rgba));

  fill_color_btn->setAutoFillBackground(true);
  fill_color_btn->setPalette(fillpalette);
  fill_color_btn->setFlat(true);
  fill_color_btn->update();

  selectOutlineWidthSpin->setValue(prefsConfig.selectOutlineLineWidth);

  if (prefsConfig.defaultVectorPath.isEmpty()) {
    vector_path_label->setText(empty_path);
  } else {
    vector_path_label->setText(prefsConfig.defaultVectorPath);
  }

  if (prefsConfig.defaultImageryPath.isEmpty()) {
    imagery_path_label->setText(empty_path);
  } else {
    imagery_path_label->setText(prefsConfig.defaultImageryPath);
  }

  if (prefsConfig.defaultTerrainPath.isEmpty()) {
    terrain_path_label->setText(empty_path);
  } else {
    terrain_path_label->setText(prefsConfig.defaultTerrainPath);
  }
}

void Preferences::ChooseSelectOutline() {
  QRgb init = qRgba(selectOutline[0],
                    selectOutline[1],
                    selectOutline[2],
                    selectOutline[3]);

  QRgb rgba = QColorDialog::getRgba(init);

  QPalette outlinepalette;
  outlinepalette.setColor(QPalette::Button, QColor(rgba));

  outline_color_btn->setAutoFillBackground(true);
  outline_color_btn->setPalette(outlinepalette);
  outline_color_btn->setFlat(true);
  outline_color_btn->update();


  selectOutline[0] = qRed(rgba);
  selectOutline[1] = qGreen(rgba);
  selectOutline[2] = qBlue(rgba);
  selectOutline[3] = qAlpha(rgba);
}

void Preferences::ChooseSelectFill() {
  QRgb init = qRgba(selectFill[0],
                    selectFill[1],
                    selectFill[2],
                    selectFill[3]);

  QRgb rgba = QColorDialog::getRgba(init);

  QPalette fillpalette;
  fillpalette.setColor(QPalette::Button, QColor(rgba));

  fill_color_btn->setAutoFillBackground(true);
  fill_color_btn->setPalette(fillpalette);
  fill_color_btn->setFlat(true);
  fill_color_btn->update();


  selectFill[0] = qRed(rgba);
  selectFill[1] = qGreen(rgba);
  selectFill[2] = qBlue(rgba);
  selectFill[3] = qAlpha(rgba);
}

void Preferences::ChooseImageryIndex() {
  DatabaseIndexDialog fileDialog;

  fileDialog.addFilter("Google Earth Database (*gedb)");
  fileDialog.setSelectedFilter(0);
  QFileInfo finfo(imagery_index->text());
  fileDialog.setDir(finfo.dirPath());

  if (fileDialog.exec() == QDialog::Accepted) {
    imagery_index->setText(fileDialog.selectedFile());
  }
}

void Preferences::ChooseImageryProject() {
  AssetChooser chooser(this, AssetChooser::Open, AssetDefs::Imagery,
                       kProjectSubtype);
  if (chooser.exec() != QDialog::Accepted)
    return;

  QString newpath;
  if (!chooser.getFullPath(newpath)) {
    notify(NFY_WARN, "Problems with this asset!");
    return;
  }

  imagery_project->setText(newpath);
}

void Preferences::ChooseImageryIndexMercator() {
  DatabaseIndexDialog fileDialog;

  fileDialog.addFilter("Prebuilt Mercator Raster (*kip, *.ktp)");
  fileDialog.setSelectedFilter(0);
  QFileInfo finfo(imagery_index->text());
  fileDialog.setDir(finfo.dirPath());

  if (fileDialog.exec() == QDialog::Accepted) {
    imagery_index_mercator->setText(fileDialog.selectedFile());
  }
}

void Preferences::ChooseImageryProjectMercator() {
  AssetChooser chooser(this, AssetChooser::Open, AssetDefs::Imagery,
                       kMercatorProjectSubtype);
  if (chooser.exec() != QDialog::Accepted)
    return;

  QString newpath;
  if (!chooser.getFullPath(newpath)) {
    notify(NFY_WARN, "Problems with this asset!");
    return;
  }

  imagery_project_mercator->setText(newpath);
}

void Preferences::accept() {
  PrefsConfig& config = getConfig();
  config.backgroundImageryProject = imagery_project->text();
  config.backgroundTexturePath = imagery_index->text();
  config.backgroundStreamServer = stream_server->text();

  config.backgroundImageryProjectMercator = imagery_project_mercator->text();
  config.backgroundTexturePathMercator = imagery_index_mercator->text();

  int id = background_stack->id(background_stack->visibleWidget());
  switch (id) {
    case 0:
      config.backgroundType = PrefsConfig::BuiltinIndex;
      break;
    case 1:
      config.backgroundType = PrefsConfig::ImageryProject;
      break;
    case 2:
      config.backgroundType = PrefsConfig::ImageryIndex;
      break;
    case 3:
      config.backgroundType = PrefsConfig::StreamServer;
      break;
  }

  id = background_stack_mercator->id(
      background_stack_mercator->visibleWidget());
  switch (id) {
    case 0:
      config.backgroundTypeMercator = PrefsConfig::BuiltinIndex;
      break;
    case 1:
      config.backgroundTypeMercator = PrefsConfig::ImageryProject;
      break;
    case 2:
      config.backgroundTypeMercator = PrefsConfig::ImageryIndex;
      break;
  }

  config.characterEncoding = codecCombo->currentText();
  config.dataViewShowFID = showFIDCheck->isChecked();
  config.dataViewAutoRaise = autoRaiseCheck->isChecked();
  config.showFullAssetPath = assetRootCheck->isChecked();
  config.selectOutlineColor = selectOutline;
  config.selectFillColor = selectFill;
  config.selectOutlineLineWidth = selectOutlineWidthSpin->value();

  config.defaultVectorPath = EmptyCheck(vector_path_label->text());
  config.defaultImageryPath = EmptyCheck(imagery_path_label->text());
  config.defaultTerrainPath = EmptyCheck(terrain_path_label->text());

  config.Save(Preferences::filepath("preferences.xml").toUtf8().constData());

  PreferencesBase::accept();
}

QString Preferences::EmptyCheck(const QString& txt) {
  if (txt == empty_path) {
    return QString("");
  } else {
    return txt;
  }
}

QString Preferences::CaptionText() {
  // Allow user to override hostname for display as CaptionText.
  // This is useful for our docs team for example, but would also
  // allow users to use an easy to read nickname.
  char* fusion_host = getenv("FUSION_HOSTNAME");
  std::string host;
  if (fusion_host) {
    host = fusion_host;
  } else {
    extern std::string khHostname(void);
    host = khHostname();
  }
  QString caption(QString("  host=") + host.c_str());
  QString runtimeDesc = RuntimeOptions::DescString();
  if (!runtimeDesc.isEmpty()) {
    caption += " | ";
    caption += runtimeDesc;
  }
  return caption;
}

QString Preferences::DefaultVectorPath() {
  PrefsConfig cfg = getConfig();
  return cfg.defaultVectorPath;
}

QString Preferences::DefaultImageryPath() {
  PrefsConfig cfg = getConfig();
  return cfg.defaultImageryPath;
}

QString Preferences::DefaultTerrainPath() {
  PrefsConfig cfg = getConfig();
  return cfg.defaultTerrainPath;
}

void Preferences::ChooseDefaultVectorPath() {
  QString path = QFileDialog::getExistingDirectory(vector_path_label->text(),
                     this, 0, kh::tr("Select Folder"), true);
  if (path != QString::null)
    vector_path_label->setText(path);
}

void Preferences::ChooseDefaultImageryPath() {
  QString path = QFileDialog::getExistingDirectory(imagery_path_label->text(),
                     this, 0, kh::tr("Select Folder"), true);
  if (path != QString::null)
    imagery_path_label->setText(path);
}

void Preferences::ChooseDefaultTerrainPath() {
  QString path = QFileDialog::getExistingDirectory(terrain_path_label->text(),
                     this, 0, kh::tr("Select Folder"), true);
  if (path != QString::null)
    terrain_path_label->setText(path);
}

std::string Preferences::PublishServerName(const std::string& database_name) {
  std::map<std::string, std::string>::const_iterator iter =
    getConfig().publisher_db_map.find(database_name);
  if (iter == getConfig().publisher_db_map.end()) {
    return kEmptyString;
  }
  return iter->second;
}

void Preferences::UpdatePublishServerDbMap(const std::string& database_name,
                                           const std::string& server_name) {
  PrefsConfig& config = getConfig();
  config.publisher_db_map[database_name] = server_name;
  config.Save(Preferences::filepath("preferences.xml").toUtf8().constData());
}

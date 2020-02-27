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

#include <Qt/qobjectdefs.h>
#include <Qt/qdatetimeedit.h>
#include <Qt/qspinbox.h>
#include <Qt/qtextcodec.h>
#include <Qt/q3listbox.h>
#include <Qt/qcheckbox.h>
#include <Qt/qmessagebox.h>
#include <Qt/qlabel.h>
#include <Qt/qinputdialog.h>
#include<Qt/q3combobox.h>
#include "common/khFileUtils.h"
#include "common/khException.h"
#include "common/khConstants.h"
#include "autoingest/.idl/gstProvider.h"
#include "autoingest/khFusionURI.h"
#include "autoingest/.idl/storage/VectorProductConfig.h"
#include "fusion/fusionui/QDateWrapper.h"

#include "fusion/fusionui/VectorAssetWidget.h"
#include "fusion/fusionui/Preferences.h"

class AssetBase;

namespace {

struct StdConversion {
  QString name;
  double  scale;
} StdConversions[] = {
#ifndef NDEBUG
  { kh::tr("Centimeters"),
    0.01 },
  { kh::tr("Decimeters"),
    0.1 },
#endif
  { kh::tr("Feet (International)"),
    khIntlFoot },
  { kh::tr("Feet (US Survey)"),
    khUsSurveyFoot },
  { kh::tr("Meters"),
    1.0 }
#ifndef NDEBUG
  ,
  { kh::tr("Rods"),
    5.0292101 },
  { kh::tr("Furlongs"),
    201.1684 },
  { kh::tr("Miles"),
    1609.344 },
  { kh::tr("Light years"),
    9.4605284e+15 },
  { kh::tr("Parsecs"),
    3.0856776e+16 }
#endif
};

const int NumStdConversions =
    static_cast<int>(sizeof(StdConversions) / sizeof(StdConversions[0]));

const QString custom_conversion(kh::tr("Other..."));


// Force feature type conversion enum.
// Note: it should be synced with ForceFeatureTypeConversions-array.
enum ForceFeatureType {
  kForceFeatureNone = 0,  // no conversion
  kForceFeature2D   = 1,  // force 2D
  kForceFeature25D  = 2,  // force 2.5D
  kForceFeature3D   = 3   // force 3D
};

// Force feature type conversion strings.
// Note: it should be synced with ForceFeatureType-enum.
// Note: "force 3D" should be last element. We use such constraint
// to enable/disable this element. see GetNumForceFeatureTypeConversions();
struct ForceFeatureTypeConversion {
  QString name;
} ForceFeatureTypeConversions[] = {
  {kh::tr("<none>")},
  {kh::tr("force 2D")},
  {kh::tr("force 2.5D")},
  {kh::tr("force 3D")}
};

int GetNumForceFeatureTypeConversions() {
  static const int NumForceFeatureTypeConversions =
      static_cast<int>(sizeof(ForceFeatureTypeConversions) /
                       sizeof(ForceFeatureTypeConversions[0]));

  // "-1"- Hide last element ("force 3D").
  return (Preferences::GoogleInternal ?
          NumForceFeatureTypeConversions :
          (NumForceFeatureTypeConversions - 1));
}

}  // namespace


VectorAssetWidget::VectorAssetWidget(QWidget* parent, AssetBase* base)
  : VectorAssetWidgetBase(parent),
    AssetWidgetBase(base),
    file_dialog_(NULL) {
  int count = 0;
  for (QTextCodec* codec; (codec = QTextCodec::codecForIndex(count)); ++count)
    codec_combo->insertItem(codec->name(), 1);

  PrefsConfig prefs = Preferences::getConfig();
  for (int index = 0; index < count; ++index) {
    if (codec_combo->text(index) == prefs.characterEncoding) {
      codec_combo->setCurrentItem(index);
      break;
    }
  }

  {
    gstProviderSet providers;
    if (!providers.Load()) {
      QMessageBox::critical(this, kh::tr("Error"),
                            kh::tr("Unable to load providers\n") +
                            kh::tr("Check console for more information"),
                            kh::tr("OK"), 0, 0, 0);
    } else {
      provider_combo->insertStringList(providers.GetNames());
      // grab ids now in case the user changes them before
      // closing this dialog
      provider_id_list_ = providers.GetIds();
    }
  }

  // clear the combobox to be certain nothing has been
  // inserted from the designer tool that we don't know about
  elev_units_combo->clear();

  // add the standard conversions to the conversion combo
  for (int i = 0; i < NumStdConversions; ++i) {
    elev_units_combo->insertItem(StdConversions[i].name, i);
  }

  // finally add the custom unit option
  elev_units_combo->insertItem(custom_conversion);

  last_conv_index_ = 0;
  source_changed_label->hide();

  // clear the combobox to be certain nothing has been
  // inserted from the designer tool that we don't know about
  force_feature_type_combo->clear();

  // add the force feature type conversions to the
  // "force feature type"-combo.
  for (int i = 0; i < GetNumForceFeatureTypeConversions(); ++i) {
    force_feature_type_combo->insertItem(
        ForceFeatureTypeConversions[i].name, i);
  }

  // Create a wrapper for accessing the date from the GUI.
  acquisition_date_wrapper_ =
      new qt_fusion::QDateWrapper(this,
                                  acquisition_date_year,
                                  acquisition_date_month,
                                  acquisition_date_day);
}

Q3FileDialog* VectorAssetWidget::FileDialog() {
  if (!file_dialog_) {
    file_dialog_ = new Q3FileDialog(this);
    file_dialog_->setMode(Q3FileDialog::ExistingFiles);
    file_dialog_->setCaption(kh::tr("Open Source"));
    if (khDirExists(Preferences::DefaultVectorPath().toUtf8().constData())) {
      file_dialog_->setDir(Preferences::DefaultVectorPath());
    } else {
      QMessageBox::critical(
          this, kh::tr("Error"),
          kh::tr("The default vector source path is not valid.\n"
             "Please update your preferences."),
          kh::tr("OK"), 0, 0, 0);
    }

    file_dialog_->addFilter(
      "Supported files ( *.rt1 *.RT1 *.gml *.GML *.txt *.csv *.TXT *.CSV *.dgn"
        " *.DGN *.tab *.TAB *.shp *.SHP *.kml *.KML *.kmz *.KMZ )");
    file_dialog_->addFilter("US Census Tiger Line Files ( *.rt1 *.RT1 )");
    file_dialog_->addFilter("OpenGIS GML ( *.gml *.GML )");
    file_dialog_->addFilter("Generic Text ( *.txt *.csv *.TXT *.CSV )");
    file_dialog_->addFilter("MicroStation Design ( *.dgn *.DGN )");
    file_dialog_->addFilter("MapInfo Table ( *.tab *.TAB )");
    file_dialog_->addFilter("ESRI Shapefile ( *.shp *.SHP )");
    file_dialog_->addFilter(
        "Keyhole Markup Language ( *.kml *.KML *.kmz *.KMZ )");

    // Make the second filter current, which is the 'Support files' one.
    // The first filter is the default "All Files (*)".
    file_dialog_->setSelectedFilter(1);
  }

  return file_dialog_;
}

void VectorAssetWidget::AddSource() {
  if (FileDialog()->exec() != QDialog::Accepted)
    return;

  QStringList files = FileDialog()->selectedFiles();

  for (QStringList::Iterator it = files.begin(); it != files.end(); ++it) {
    source_list->insertItem(*it);
  }
}

void VectorAssetWidget::DeleteSource() {
  int row = source_list->currentItem();
  if (row != -1)
    source_list->removeItem(row);
}

void VectorAssetWidget::CustomConversion(const QString& str) {
  // need to track the selected item in order to reset it
  // if the user cancels the custom dialog below
  if (str != custom_conversion) {
    last_conv_index_ = elev_units_combo->currentItem();
    return;
  }

  bool ok;
  double conv = QInputDialog::getDouble(
                  kh::tr("Custom Unit Conversion Factor"),
                  kh::tr("Enter multiplier to convert source values to meters:"),
                  1.0, -2147483647, 2147483647, 16, &ok, this);

  if (ok) {
    UpdateElevUnits(conv);
  } else {
    elev_units_combo->setCurrentItem(last_conv_index_);
  }
}


void VectorAssetWidget::UpdateElevUnits(double conv) {
  int i = 0;
  for (; i < NumStdConversions; ++i) {
    if (conv == StdConversions[i].scale) {
      elev_units_combo->setCurrentItem(i);
      break;
    }
  }
  if (i == NumStdConversions) {
    // non-standard conversion

    // has a custom entry already been added yet to the combobox?
    if (elev_units_combo->count() == NumStdConversions + 1) {
      // no, insert it now
      elev_units_combo->insertItem(QString::number(conv, 'g', 16),
                                 NumStdConversions);
    } else {
      // yes, replace the previous custom entry with the new one
      elev_units_combo->changeItem(QString::number(conv, 'g', 16),
                                 NumStdConversions);
    }
    elev_units_combo->setCurrentItem(NumStdConversions);
  }

  // make sure the combobox now has the new custom entry selected
  last_conv_index_ = elev_units_combo->currentItem();
}

double VectorAssetWidget::GetElevUnits() const {
  int currItem = elev_units_combo->currentItem();
  if (currItem < NumStdConversions) {
    return StdConversions[currItem].scale;
  }

  assert(elev_units_combo->currentItem() == NumStdConversions);

  return elev_units_combo->currentText().toDouble();
}

void VectorAssetWidget::SetForceFeatureType(const int val) {
  assert(val < GetNumForceFeatureTypeConversions());
  force_feature_type_combo->setCurrentItem(val);
}

int VectorAssetWidget::GetForceFeatureType() const {
  int currItem = force_feature_type_combo->currentItem();
  assert(currItem < GetNumForceFeatureTypeConversions());
  return currItem;
}

void VectorAssetWidget::SetIgnoreBadFeaturesCheck(const bool val) {
  ignore_bad_features_check->setChecked(val);
}

bool VectorAssetWidget::GetIgnoreBadFeaturesCheck() const {
  return ignore_bad_features_check->isChecked();
}

void VectorAssetWidget::SetDoNotFixInvalidGeometriesCheck(const bool val) {
  do_not_fix_invalid_geometries_check->setChecked(val);
}

bool VectorAssetWidget::GetDoNotFixInvalidGeometriesCheck() const {
  return do_not_fix_invalid_geometries_check->isChecked();
}

void VectorAssetWidget::Prefill(const VectorProductImportRequest& request) {
  acquisition_date_wrapper_->SetDate(0, 0, 0);
  std::string date = request.meta.GetValue("sourcedate").toUtf8().constData();
  if (!date.empty()) {
    acquisition_date_wrapper_->SetDate(date);
  }

  if (request.config.provider_id_ != 0) {
    for (int c = 0; c < static_cast<int>(provider_id_list_.size()); ++c) {
      if (request.config.provider_id_== provider_id_list_[c]) {
        // +1 since we have a "<none>" at the beginning of the combo box
        provider_combo->setCurrentItem(c+1);
        break;
      }
    }
  }

  for (int index = 0; index < codec_combo->count(); ++index) {
    if (codec_combo->text(index) == request.config.encoding.c_str()) {
      codec_combo->setCurrentItem(index);
      break;
    }
  }

  layer_spin->setValue(request.config.layer);

  UpdateElevUnits(request.config.scale);

  SetIgnoreBadFeaturesCheck(request.config.ignore_bad_features_);

  SetDoNotFixInvalidGeometriesCheck(
      request.config.do_not_fix_invalid_geometries_);

  // Set item in "force feature type"-combo.
  if (request.config.force2D) {
    SetForceFeatureType(kForceFeature2D);
  } else if (request.config.force25D) {
    SetForceFeatureType(kForceFeature25D);
  } else if (request.config.force3D) {
    SetForceFeatureType(kForceFeature3D);
  } else {
    SetForceFeatureType(kForceFeatureNone);
  }

  for (std::vector<SourceConfig::FileInfo>::const_iterator src =
       request.sources.sources.begin();
       src != request.sources.sources.end(); ++src) {
    khFusionURI furi(src->uri);
    if (furi.Valid()) {
      source_list->insertItem(furi.NetworkPath().c_str());
    } else {
      source_list->insertItem(src->uri.c_str());
    }
  }

  north_boundary->setText(QString::number(request.config.north_boundary));
  south_boundary->setText(QString::number(request.config.south_boundary));
  east_boundary->setText(QString::number(request.config.east_boundary));
  west_boundary->setText(QString::number(request.config.west_boundary));

  // notify user if sources on disk have changed
  VectorProductImportRequest request_modified = request;
  try {
    AssembleEditRequest(&request_modified);
  } catch(...) {
    // don't display any errors here, our only puyrpose is to enable the
    // "sources changed" message. popping up messages just leads to
    // obnoxious multiple reporting issues. If they try to save, they will
    // get the more specific message then.
  }
  if (request.sources != request_modified.sources) {
    source_changed_label->show();
  }
}

void VectorAssetWidget::AssembleEditRequest(
    VectorProductImportRequest* request) {
  request->config.encoding = codec_combo->currentText().toUtf8().constData();
  if (request->config.encoding == "<none>")
    request->config.encoding = "";

  request->config.layer = layer_spin->value();

  // we have "<none>" at the beginning of the combo box
  if (provider_combo->currentItem() != 0) {
    request->config.provider_id_ =
      provider_id_list_[provider_combo->currentItem() - 1];
  } else {
    request->config.provider_id_ = 0;
  }

  request->meta.SetValue("sourcedate", acquisition_date_wrapper_->GetDate());

  request->config.scale = GetElevUnits();

  request->config.ignore_bad_features_ = GetIgnoreBadFeaturesCheck();

  request->config.do_not_fix_invalid_geometries_ =
      GetDoNotFixInvalidGeometriesCheck();

  // Set force feature type.
  int force_feature_type = GetForceFeatureType();
  request->config.force2D = false;
  request->config.force25D = false;
  request->config.force3D = false;
  switch (force_feature_type) {
    case kForceFeatureNone:
      break;
    case kForceFeature2D:
      request->config.force2D = true;
      break;
    case kForceFeature25D:
      request->config.force25D = true;
      break;
    case kForceFeature3D:
      request->config.force3D = true;
      break;
    default:
      assert(!"Undefined force feature type.");
      break;
  }

  request->sources.clear();
  std::string msg;
  for (int row = 0; row < source_list->numRows(); row++) {
    std::string filename = source_list->text(row).toUtf8().constData();
    SourceConfig::AddResult result = request->sources.AddFile(filename);
    switch (result) {
      case SourceConfig::NonVolume:
          msg.clear();
          msg = filename;
          msg += kh::tr(" doesn't reside on a known volume.\n").toUtf8().constData();
          msg += " You can move your asset files to a known volume, or create a new volume\n";
          msg += " that contains their current location using geconfigureassetroot command.";
          throw khException(msg);
      case SourceConfig::FileOK:
        break;
      case SourceConfig::CantStat:
        msg.clear();
        msg = kh::tr("Unable to get file information for ").toUtf8().constData();
        msg += filename + "\n";
        msg += khErrnoException::errorString(errno).toUtf8().constData();
        throw khException(msg);
    }
  }
  bool ok_north = false;
  bool ok_south = false;
  bool ok_east  = false;
  bool ok_west = false;
  request->config.north_boundary = north_boundary->text().toDouble(&ok_north);
  request->config.south_boundary = south_boundary->text().toDouble(&ok_south);
  request->config.east_boundary = east_boundary->text().toDouble(&ok_east);
  request->config.west_boundary = west_boundary->text().toDouble(&ok_west);
  std::string boundary_error;
  if (!ok_north) {
    boundary_error += "Bad double value for north '";
    boundary_error += north_boundary->text().toUtf8().constData();
    boundary_error += "'\n";
  } else if (request->config.north_boundary < -90 ||
             request->config.north_boundary > 90    ) {
    boundary_error += "north should be in range [-90, 90].\n";
  }
  if (!ok_south) {
    boundary_error += "Bad double value for south '";
    boundary_error += south_boundary->text().toUtf8().constData();
    boundary_error += "'\n";
  } else if (request->config.south_boundary < -90 ||
             request->config.south_boundary > 90    ) {
    boundary_error += "south should be in range [-90, 90].\n";
  }
  if (boundary_error.empty() &&
      request->config.north_boundary <= request->config.south_boundary) {
    boundary_error += "north should be greater than south.\n";
  }
  const std::string::size_type boundary_error_length = boundary_error.length();
  if (!ok_east) {
    boundary_error += "Bad double value for east '";
    boundary_error += east_boundary->text().toUtf8().constData();
    boundary_error += "'\n";
  } else if (request->config.east_boundary < -180 ||
             request->config.east_boundary > 180    ) {
    boundary_error += "east should be in range [-180, 180].\n";
  }
  if (!ok_west) {
    boundary_error += "Bad double value for west '";
    boundary_error += west_boundary->text().toUtf8().constData();
    boundary_error += "'\n";
  } else if (request->config.west_boundary < -180 ||
             request->config.west_boundary > 180    ) {
    boundary_error += "west should be in range [-180, 180].\n";
  }
  // If no new error messages have been added then east and west are good
  const bool is_good_east_and_west =
      boundary_error.length() == boundary_error_length;
  if (is_good_east_and_west &&
      request->config.east_boundary <= request->config.west_boundary) {
    boundary_error += "east should be greater than west.\n";
  }
  if (!boundary_error.empty()) {
    throw khException(boundary_error);
  }
}

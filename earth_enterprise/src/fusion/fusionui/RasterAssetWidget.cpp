// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
// Copyright 2020, Open GEE Contributors
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
//

#include "fusion/fusionui/RasterAssetWidget.h"
#include "cpl_port.h"

#include <algorithm>
#include <gdal.h>
#include "fusion/khgdal/khgdal.h"
#include <Qt/qcheckbox.h>
#include <Qt/qcolordialog.h>
#include <Qt/qcombobox.h>
#include <Qt/q3filedialog.h>
#include <Qt/qgroupbox.h>
#include <qinputdialog.h>
#include <Qt/qinputdialog.h>
#include <Qt/qlabel.h>
#include <Qt/q3listbox.h>
#include <Qt/qmessagebox.h>
#include <Qt/qpushbutton.h>
#include <qspinbox.h>
#include <Qt/qspinbox.h>
#include <Qt/q3listview.h>

#include "autoingest/.idl/gstProvider.h"
#include "autoingest/.idl/storage/RasterProductConfig.h"
#include "fusion/autoingest/khFusionURI.h"
#include "fusion/fusionui/AssetBase.h"
#include "fusion/fusionui/QDateWrapper.h"
#include "fusion/fusionui/Preferences.h"
#include "common/khStringUtils.h"
#include "common/khConstants.h"
#include "common/khException.h"

static struct StdConversion {
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


static unsigned int NumStdConversions =
    sizeof(StdConversions)/sizeof(StdConversions[0]);

namespace {
const QString custom_conversion(QObject::tr("Other..."));
}

RasterAssetWidget::RasterAssetWidget(QWidget* parent, AssetDefs::Type type)
  : RasterAssetWidgetBase(parent),
    asset_type_(type),
    file_dialog_(NULL) {
  last_conv_index_ = 0;
  last_mosaic_fill_index_ = 0;

  // clear the combobox to be certain nothing has been
  // inserted from the designer tool that we don't know about
  elev_units_combo->clear();

  // add the standard conversions to the conversion combo
  for (unsigned int i = 0; i < NumStdConversions; ++i) {
    elev_units_combo->insertItem(StdConversions[i].name, i);
  }

  // finally add the custom unit option
  elev_units_combo->insertItem(custom_conversion);

  if (AssetType() == AssetDefs::Imagery) {
    terrain_group_box->hide();
    mosaic_fill_combo->insertItem(tr("Black"), 1);
    mosaic_fill_combo->insertItem(tr("White"), 2);

    //  set year to fixed width so that layout does not auto adjust it
    //  and sometimes shows only two digits of the year. So we want to make sure
    //  year text gets at least the space required to display four digits and
    //  rest of the space can be taken by layout to auto adjust.
  } else {
    mask_band_label->hide();
    mask_band_combo->hide();
    mask_tolerance_label->hide();
    mask_tolerance_spin->hide();
    mask_fillwhite_check->hide();
    // time is not applicable for Terrain assets.
    AdjustAcquisitionTimeEnabled(false);
  }

  {
    gstProviderSet providers;
    if (!providers.Load()) {
      QMessageBox::critical(this, tr("Error"),
                            tr("Unable to load providers\n") +
                            tr("Check console for more information"),
                            tr("OK"), 0, 0, 0);
    } else {
      provider_combo->insertStringList(providers.GetNames());
      // grab ids now in case the user changes them before
      // closing this dialog
      provider_id_list_ = providers.GetIds();
    }
  }

  // lutfile is only available in expert mode
  // and only for imagery assets
  // this is an un-published feature, and the files
  // can only be generated with an unpublished tool "scroll"
  if (AssetType() == AssetDefs::Terrain || !Preferences::ExperimentalMode) {
    lutfile_label->hide();
    lutfile_name_label->hide();
    lutfile_delete_btn->hide();
    lutfile_chooser_btn->hide();
  }

  lutfile_name_label->setText("<none>");
  source_changed_label->hide();

  // Create a wrapper for accessing the date and time from the GUI.
  acquisition_date_wrapper_ =
      new qt_fusion::QDateWrapper(this,
                                  acquisition_date_year,
                                  acquisition_date_month,
                                  acquisition_date_day,
                                  acquisition_date_hours,
                                  acquisition_date_minutes,
                                  acquisition_date_seconds);
}

Q3FileDialog* RasterAssetWidget::FileDialog() {
  if (!file_dialog_) {
    // will be automatically deleted with this
    file_dialog_ = new Q3FileDialog(this);
    file_dialog_->setMode(Q3FileDialog::ExistingFiles);
    file_dialog_->setCaption(tr("Open Source"));
    bool add_dem_file_filter = false;

    if (AssetType() == AssetDefs::Imagery) {
      if (khDirExists(Preferences::DefaultImageryPath().latin1())) {
        file_dialog_->setDir(Preferences::DefaultImageryPath());
      } else {
        QMessageBox::critical(
                this, tr("Error"),
                tr("The default imagery source path is not valid.\n"
                   "Please update your preferences."),
                tr("OK"), 0, 0, 0);
      }
    } else {
      if (khDirExists(Preferences::DefaultTerrainPath().latin1())) {
        file_dialog_->setDir(Preferences::DefaultTerrainPath());
      } else {
        QMessageBox::critical(
                this, tr("Error"),
                tr("The default terrain source path is not valid.\n"
                   "Please update your preferences."),
                tr("OK"), 0, 0, 0);
      }
      add_dem_file_filter = true;
    }
    // For JPEG2000 and MrSID, check if GDAL is built
    // with specific format support.
    bool is_jpeg_found = false;
    bool is_mrsid_found = false;
    for (int iDr = 0; iDr < GDALGetDriverCount(); iDr++) {
      if (is_jpeg_found && is_mrsid_found) break;

      GDALDriverH hDriver = GDALGetDriver(iDr);
      //Check for raster formats only.
      char** papszMD = GDALGetMetadata(hDriver, NULL);
      // If not Raster format do not check
      if(!CPLFetchBool((const char**)papszMD, GDAL_DCAP_RASTER, false))
        continue;

      std::string driverShortName = GDALGetDriverShortName(hDriver);

      if (!is_jpeg_found) {
        std::size_t jp2 = driverShortName.find("JP2");
        std::size_t jpeg2000 = driverShortName.find("JPEG2000");
        if (jp2 != std::string::npos || jpeg2000 != std::string::npos) {
          is_jpeg_found = true;
          continue;
        }
      }
      if (!is_mrsid_found) {
        std::size_t mrsid = driverShortName.find("MrSID");
        if (mrsid != std::string::npos) {
          is_mrsid_found = true;
          continue;
        }
      }
    }
    // Add file filters:
    QString supported_files_filter = "Supported images (";
    if (add_dem_file_filter)
      supported_files_filter += " *.dem *.DEM";
    if (is_jpeg_found)
      supported_files_filter += " *.jp2 *.JP2";
    if (is_mrsid_found)
      supported_files_filter += " *.sid *.SID";
    supported_files_filter += " *.img *.IMG *.tif *.TIF *.tiff *.TIFF )";
    file_dialog_->addFilter(supported_files_filter);

    if (add_dem_file_filter)
      file_dialog_->addFilter("USGS ASCII DEM ( *.dem *.DEM )");
    if (is_jpeg_found)
      file_dialog_->addFilter("JPEG2000 ( *.jp2 *.JP2 )");
    if (is_mrsid_found)
      file_dialog_->addFilter("MrSID ( *.sid *.SID )");
    file_dialog_->addFilter("Erdas Imagine ( *.img *.IMG )");
    file_dialog_->addFilter("TIFF/GeoTIFF ( *.tif *.TIF *.tiff *.TIFF )");

    // Make the second filter current, which is the "Supported images" one.
    // The first one is the default "All Files (*)".
    file_dialog_->setSelectedFilter(1);
  }
  return file_dialog_;
}

void RasterAssetWidget::AdjustMosaicEnabled(void) {
  bool enabled = (source_list->numRows() > 1);
  mosaic_group_box->setEnabled(enabled);
}

void RasterAssetWidget::AddSource() {
  if (FileDialog()->exec() != QDialog::Accepted)
    return;

  bool modified = false;
  QStringList files = FileDialog()->selectedFiles();
  for (QStringList::Iterator it = files.begin(); it != files.end(); ++it) {
    QString string = *it;
    std::size_t idx = string.find("/header.xml");
    if (idx != std::string::npos)
    { 
      string.remove( idx, strlen("/header.xml"));
    }
    // was Qt::ExactMatch
    if (source_list->findItem(string, Q3ListView::ExactMatch)) {
      QMessageBox::warning(
          this, kh::tr("Warning : duplicate source.") ,
          kh::tr("Source '%1' already exists in this resource. Ignoring duplicates.")
          .arg(string),
          kh::tr("OK"), 0, 0, 0);
      continue;
    } else {
      source_list->insertItem(string);
      modified = true;
    }
  }

  if (modified) {
	this->GetAssetBase()->SetSaveError(false);
  }

  // adding a source may enable mosaic config
  AdjustMosaicEnabled();

  // disable "Have Mask" option if more than one source is chosen
  AdjustMaskType();
}

void RasterAssetWidget::DeleteSource() {
  int row = source_list->currentItem();

  if (row == -1)
    return;

  source_list->removeItem(row);
  this->GetAssetBase()->SetSaveError(false);

  // deleting a source may disable mosaic config
  AdjustMosaicEnabled();

  // enable "Have Mask" option if source list is reduced to one or less
  AdjustMaskType();
}

void RasterAssetWidget::MosaicFillActivated(const QString& str) {
  // need to track the selected item in order to reset it
  // if the user cancels the custom dialog below
  if (str != tr("Other...")) {
    last_mosaic_fill_index_ = mosaic_fill_combo->currentItem();
    SyncMosaicTolerance();
    return;
  }

  bool ok = false;
  std::string fill;

  if (AssetType() == AssetDefs::Terrain) {
    double conv = QInputDialog::getDouble(
        tr("Custom Mosaic Fill Color"),
        tr("Enter heightmap value that represents fill:"),
        0.0, -2147483647, 2147483647, 16, &ok, this);
    fill = ToString(conv);
  } else {
    QColor choice = QColorDialog::getColor(Qt::black, this);
    if (choice.isValid()) {
      ok = true;
      fill.reserve(15);
      fill = ToString(choice.red());
      fill.append(",");
      fill.append(ToString(choice.green()));
      fill.append(",");
      fill.append(ToString(choice.blue()));
    }
  }

  if (ok) {
    UpdateMosaicFill(fill);
  } else {
    mosaic_fill_combo->setCurrentItem(last_mosaic_fill_index_);
  }
}

void RasterAssetWidget::UpdateMosaicFill(const std::string& fill) {
  if (fill.empty()) {
    mosaic_fill_combo->setCurrentItem(0);
  } else if (AssetType() == AssetDefs::Terrain) {
    if (mosaic_fill_combo->count() == 2) {
      // need to add the custom fill value to the combo
      mosaic_fill_combo->insertItem(fill.c_str(), 1);
    } else {
      // need to undate the custom fill value in the combo
      mosaic_fill_combo->changeItem(fill.c_str(), 1);
    }
    mosaic_fill_combo->setCurrentItem(1);
  } else /* AssetType() == AssetDefs::Imagery */ {
    if (fill == "0,0,0") {
      mosaic_fill_combo->setCurrentItem(1);
    } else if (fill == "255,255,255") {
      mosaic_fill_combo->setCurrentItem(2);
    } else {
      if (mosaic_fill_combo->count() == 4) {
        // need to add the custom fill value to the combo
        mosaic_fill_combo->insertItem(fill.c_str(), 3);
      } else {
        // need to undate the custom fill value in the combo
        mosaic_fill_combo->changeItem(fill.c_str(), 3);
      }
      mosaic_fill_combo->setCurrentItem(3);
    }
  }

  // make sure the combobox now has the new custom entry selected
  last_mosaic_fill_index_ = mosaic_fill_combo->currentItem();
  SyncMosaicTolerance();
}

void RasterAssetWidget::SyncMosaicTolerance(void) {
  bool enable_tolerance = (last_mosaic_fill_index_ != 0);
  mosaic_tolerance_label->setEnabled(enable_tolerance);
  mosaic_tolerance_spin->setEnabled(enable_tolerance);
}


std::string RasterAssetWidget::GetMosaicFill() const {
  int curr_item = mosaic_fill_combo->currentItem();
  if (curr_item == 0)
    return std::string();

  if (AssetType() == AssetDefs::Imagery) {
    switch (curr_item) {
      case 1:
        return "0,0,0";
      case 2:
        return "255,255,255";
    }
  }

  return std::string(mosaic_fill_combo->currentText().toUtf8().constData());
}

void RasterAssetWidget::ChangeMaskType(int mode) {
  bool enabled = (mode == 0);
  mask_feather_label->setEnabled(enabled);
  mask_feather_spin->setEnabled(enabled);
  mask_tolerance_label->setEnabled(enabled);
  mask_tolerance_spin->setEnabled(enabled);
  mask_holesize_label->setEnabled(enabled);
  mask_holesize_spin->setEnabled(enabled);
  mask_band_label->setEnabled(enabled);
  mask_band_combo->setEnabled(enabled);
  mask_fillwhite_check->setEnabled(enabled);
  mask_nodata_label->setEnabled(enabled);
  mask_nodata_lineedit->setEnabled(enabled);
#if 0
  if (mode == 0) {
    automaskGroupBox->setEnabled(true);
  } else {
    automaskGroupBox->setEnabled(false);
  }
#endif
}

void RasterAssetWidget::CustomConversion(const QString& str) {
  // need to track the selected item in order to reset it
  // if the user cancels the custom dialog below
  if (str != custom_conversion) {
    last_conv_index_ = elev_units_combo->currentItem();
    return;
  }

  bool ok;
  double conv = QInputDialog::getDouble(
                  tr("Custom Unit Conversion Factor"),
                  tr("Enter multiplier to convert source values to meters:"),
                  1.0, -2147483647, 2147483647, 16, &ok, this);

  if (ok) {
    UpdateElevUnits(conv);
  } else {
    elev_units_combo->setCurrentItem(last_conv_index_);
  }
}

void RasterAssetWidget::UpdateElevUnits(double conv) {
  unsigned int i = 0;
  for (; i < NumStdConversions; ++i) {
    if (conv == StdConversions[i].scale) {
      elev_units_combo->setCurrentItem(i);
      break;
    }
  }
  if (i == NumStdConversions) {
    // non-standard conversion

    // has a custom entry already been added yet to the combobox?
    if (elev_units_combo->count() == static_cast<int>(NumStdConversions) + 1) {
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

double RasterAssetWidget::GetElevUnits() const {
  int curr_item = elev_units_combo->currentItem();
  if (curr_item < static_cast<int>(NumStdConversions)) {
    return StdConversions[curr_item].scale;
  }

  assert(elev_units_combo->currentItem() ==
         static_cast<int>(NumStdConversions));

  return elev_units_combo->currentText().toDouble();
}

void RasterAssetWidget::ChooseLutfile() {
  Q3FileDialog lutDialog(this, "Choose a Keyhole LUT file", true);
  lutDialog.addFilter("Keyhole LUT file ( *.lut )");

  if (lutDialog.exec() == QDialog::Accepted)
    lutfile_name_label->setText(lutDialog.selectedFile());
}

void RasterAssetWidget::DeleteLutfile() {
  lutfile_name_label->setText("<none>");
}


void RasterAssetWidget::Prefill(const RasterProductImportRequest& request) {
  // always fill in lutfile so that it is available for final import request
  // however, the widget might be hidden if not in expert mode
  if (!request.config.lutfile.empty())
    lutfile_name_label->setText(request.config.lutfile.c_str());

  acquisition_date_wrapper_->SetDate(0, 0, 0, 0, 0, 0);
  std::string date = request.meta.GetValue("sourcedate").toUtf8().constData();

  if (!date.empty()) {
    acquisition_date_wrapper_->SetDate(date);
  }

  if (request.config.provider_id_ != 0) {
    for (int c = 0; c < static_cast<int>(provider_id_list_.size()); ++c) {
      if (request.config.provider_id_== provider_id_list_[c]) {
        // +1 since we have a "<none>" at the beginning of the combo box
        provider_combo->setCurrentItem(c + 1);
        break;
      }
    }
  }

  source_list->clear();
  std::vector<SourceConfig::FileInfo>::const_iterator src =
    request.sources.sources.begin();
  for (; src != request.sources.sources.end(); ++src) {
    khFusionURI furi(src->uri);
    if (furi.Valid()) {
      source_list->insertItem(furi.NetworkPath().c_str());
    } else {
      source_list->insertItem(src->uri.c_str());
    }
  }

  // fill Mask widgets
  RestoreAllMaskOptions();
  if (request.config.havemask) {
    mask_type_combo->setCurrentItem(1);
  } else if (request.config.nomask) {
    mask_type_combo->setCurrentItem(2);
  } else {
    mask_type_combo->setCurrentItem(0);
  }
  mask_feather_spin->setValue(request.config.maskgenConfig.feather);
  mask_band_combo->setCurrentItem(request.config.maskgenConfig.band);
  mask_tolerance_spin->setValue(request.config.maskgenConfig.threshold);
  mask_holesize_spin->setValue(request.config.maskgenConfig.holesize);
  mask_fillwhite_check->setChecked(request.config.maskgenConfig.whitefill);
  // for now the GUI doesn't expose/maintain these two, so leave them alone
  //    req.config.maskgenConfig.mode
  //    req.config.maskgenConfig.fillvalue

  // fill Mosaic widgets
  UpdateMosaicFill(request.config.fill);
  mosaic_tolerance_spin->setValue(request.config.mosaicFillTolerance);

  // fill terrain widgets
  if (AssetType() == AssetDefs::Terrain)
    UpdateElevUnits(request.config.scale);
  clamp_elev_check->setChecked(request.config.clampNonnegative);
  mask_nodata_lineedit->setText(request.config.maskgenConfig.nodata.c_str());

  AdjustMaskType();
  AdjustMosaicEnabled();
  north_boundary->setText(QString::number(request.config.north_boundary));
  south_boundary->setText(QString::number(request.config.south_boundary));
  east_boundary->setText(QString::number(request.config.east_boundary));
  west_boundary->setText(QString::number(request.config.west_boundary));

  // notify user if sources on disk have changed
  RasterProductImportRequest request_modified = request;
  try {
    AssembleEditRequest(&request_modified);
  } catch (...) {
    // don't display any errors here, our only puyrpose is to enable the
    // "sources changed" message. popping up messages just leads to
    // obnoxious multiple reporting issues. If they try to save, they will
    // get the more specific message then.
  }
  if (request.sources != request_modified.sources) {
    source_changed_label->show();
  }
}

void RasterAssetWidget::AssignMaskType(RasterProductImportRequest* request,
                                       MaskType masktype) {
  switch (masktype) {
    case AutoMask:
      request->config.havemask = false;
      request->config.nomask = false;
      break;

    case HaveMask:
      request->config.havemask = true;
      request->config.nomask = false;
      break;

    case NoMask:
      request->config.nomask = true;
      request->config.havemask = false;
      break;
  }
}

void RasterAssetWidget::AssembleEditRequest(
    RasterProductImportRequest* request) {
  // Terrain fields
  if (AssetType() == AssetDefs::Terrain)
    request->config.scale = GetElevUnits();
  request->config.clampNonnegative = clamp_elev_check->isChecked();

  // Mask fields
  AssignMaskType(request, GetMaskType());
  request->config.maskgenConfig.feather = mask_feather_spin->value();
  request->config.maskgenConfig.band = mask_band_combo->currentItem();
  request->config.maskgenConfig.threshold = mask_tolerance_spin->value();
  request->config.maskgenConfig.holesize = mask_holesize_spin->value();
  request->config.maskgenConfig.whitefill = mask_fillwhite_check->isChecked();
  // Get Terrain NoData text and remove any spaces before storing.
  std::string nodata_tmp = mask_nodata_lineedit->text().toUtf8().constData();
  CleanString(&nodata_tmp, " ");
  if (AssetType() == AssetDefs::Imagery) {
    std::string::size_type comma_pos = nodata_tmp.find(",");
    if (comma_pos != std::string::npos) {
      QMessageBox::warning(this, "Warning: Ignoring Extra No Data Values",
        tr("Imagery Resources may only have a single \"No Data\" value or "
             "range.\n"
             "The values/ranges following the first value/range have been "
             "removed."),
        tr("OK"), 0, 0, 1);
      nodata_tmp = nodata_tmp.substr(0, comma_pos);
      mask_nodata_lineedit->setText(nodata_tmp.c_str());
    }
  }

  request->config.maskgenConfig.nodata = nodata_tmp;
  // for now the GUI doesn't expose/maintain these two, so leave them alone
  //    request->config.maskgenConfig.mode
  //    request->config.maskgenConfig.fillvalue

  // Mosaic fields
  request->config.fill = GetMosaicFill();
  request->config.mosaicFillTolerance = mosaic_tolerance_spin->value();


  // only take lutfile if valid
  if (!lutfile_name_label->text().isEmpty() && lutfile_name_label->text()
      != QString("<none>"))
    request->config.lutfile = lutfile_name_label->text().latin1();

  // we have "<none>" at the beginning of the combo box
  if (provider_combo->currentItem() != 0) {
    request->config.provider_id_ =
      provider_id_list_[provider_combo->currentItem() - 1];
  } else {
    request->config.provider_id_ = 0;
  }

  if (!acquisition_date_wrapper_->IsValidDate()) {
    throw khException(tr("Invalid date."));
  }
  request->meta.SetValue("sourcedate", acquisition_date_wrapper_->GetDate());

  request->sources.clear();
  if (source_list->numRows() == 0){
    throw khException(tr("No source files specified \n") +
                      khErrnoException::errorString(errno));
  }
  for (int row = 0; row < source_list->numRows(); ++row) {
    std::string filename = source_list->text(row).latin1();
    SourceConfig::AddResult result = request->sources.AddFile(filename);
    switch (result) {
      case SourceConfig::NonVolume:
        throw khException(filename + kh::tr(" doesn't reside on a known volume.\n").toUtf8().constData()
                        + " You can move your asset files to a known volume, or create a new volume\n"
                        + " that contains their current location using geconfigureassetroot command.");
        break;
      case SourceConfig::FileOK:
        break;
      case SourceConfig::CantStat:
        std::string msg { kh::tr("Unable to get file information for ").toUtf8().constData() };
        msg += filename + "\n";
        throw khException(msg + khErrnoException::errorString(errno).toUtf8().constData());
        break;

    }
  }

  // set the sourceIsProduct flag appropriately
  request->config.sourceIsProduct = false;
  if (request->sources.sources.size() == 1) {
    std::string fname =
      khFusionURI(request->sources.sources[0].uri).NetworkPath();
    if (!khIsURI(fname) &&
        (khHasExtension(fname, ".kip") ||
         khHasExtension(fname, ".ktp"))) {
      request->config.sourceIsProduct = true;
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
    boundary_error += north_boundary->text().latin1();
    boundary_error += "'\n";
  } else if (request->config.north_boundary < -90 ||
             request->config.north_boundary > 90    ) {
    boundary_error += "north should be in range [-90, 90].\n";
  }
  if (!ok_south) {
    boundary_error += "Bad double value for south '";
    boundary_error += south_boundary->text().latin1();
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
    boundary_error += east_boundary->text().latin1();
    boundary_error += "'\n";
  } else if (request->config.east_boundary < -180 ||
             request->config.east_boundary > 180    ) {
    boundary_error += "east should be in range [-180, 180].\n";
  }
  if (!ok_west) {
    boundary_error += "Bad double value for west '";
    boundary_error += west_boundary->text().latin1();
    boundary_error += "'\n";
  } else if (request->config.west_boundary < -180 ||
             request->config.west_boundary > 180    ) {
    boundary_error += "west should be in range [-180, 180].\n";
  }
  if (boundary_error.length() == boundary_error_length &&
             request->config.east_boundary <= request->config.west_boundary) {
    boundary_error += "east should be greater than west.\n";
  }
  if (!boundary_error.empty()) {
    throw khException(boundary_error);
  }
}

RasterAssetWidget::MaskType RasterAssetWidget::GetMaskType() const {
  if (mask_type_combo->currentItem() == 0) {
    return AutoMask;
  } else if (mask_type_combo->currentItem() == 2) {
    return NoMask;
  }

  return (mask_type_combo->count() == 3) ? HaveMask : NoMask;
}

void RasterAssetWidget::AdjustMaskType() {
  if (source_list->numRows() > 1 && mask_type_combo->count() == 3) {
    if (GetMaskType() == HaveMask) {
      QMessageBox::warning(this, "Warning",
        tr("Mask Type cannot be \"Have Mask\" when more than one"
           "source is selected.") +
        QString("\n\n") +
        tr("Changing Mask Type to \"Auto Mask\""),
        tr("OK"), 0, 0, 1);

      mask_type_combo->setCurrentItem(0);
    }

    mask_type_combo->removeItem(1);

  } else if (source_list->numRows() <= 1 && mask_type_combo->count() < 3) {
    RestoreAllMaskOptions();
  }
  ChangeMaskType(mask_type_combo->currentItem());
}

void RasterAssetWidget::RestoreAllMaskOptions() {
  if (mask_type_combo->count() < 3) {
    int currMask = mask_type_combo->currentItem();

    mask_type_combo->insertItem(tr("Have Mask"), 1);

    // if mask was set to 'no mask' this is the 3rd item
    if (currMask == 1)
      mask_type_combo->setCurrentItem(2);
  }
}

void RasterAssetWidget::AdjustAcquisitionTimeEnabled(bool enabled) {
    acquisition_date_hours->setEnabled(enabled);
    acquisition_date_minutes->setEnabled(enabled);
    acquisition_date_seconds->setEnabled(enabled);
    acquisition_time_label->setEnabled(enabled);
    acquisition_time_seperator_label1->setEnabled(enabled);
    acquisition_time_seperator_label2->setEnabled(enabled);
}

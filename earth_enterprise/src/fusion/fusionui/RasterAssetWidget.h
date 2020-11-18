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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_RASTERASSETWIDGET_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_RASTERASSETWIDGET_H_

#include <vector>
#include "autoingest/.idl/storage/AssetDefs.h"
#include "rasterassetwidgetbase.h"
#include "fusion/fusionui/AssetWidgetBase.h"

namespace qt_fusion {
  class QDateWrapper;
}
class AssetBase;
class Q3FileDialog;
class QStringList;
class RasterProductImportRequest;

class RasterAssetWidget : public RasterAssetWidgetBase {
 protected:
  RasterAssetWidget(QWidget* parent, AssetDefs::Type type);

 public:
  void Prefill(const RasterProductImportRequest& request);
  void AssembleEditRequest(RasterProductImportRequest* request);

  // inherited from RasterAssetWidgetBase
  virtual void AddSource();
  virtual void DeleteSource();
  virtual void ChooseLutfile();
  virtual void DeleteLutfile();
  virtual void MosaicFillActivated(const QString& str);
  virtual void ChangeMaskType(int mode);
  virtual void CustomConversion(const QString& str);

 private:
  Q3FileDialog* FileDialog();
  enum MaskType { AutoMask, HaveMask, NoMask };
  MaskType GetMaskType() const;
  void AdjustMaskType();
  void AssignMaskType(RasterProductImportRequest* request,
                      MaskType masktype);
  void AdjustMosaicEnabled();
  void AdjustAcquisitionTimeEnabled(bool);
  void SyncMosaicTolerance();
  void UpdateElevUnits(double conv);
  double GetElevUnits() const;
  void UpdateMosaicFill(const std::string& fill);
  std::string GetMosaicFill() const;
  void RestoreAllMaskOptions();
  AssetDefs::Type AssetType() const { return asset_type_; }
  virtual AssetBase* GetAssetBase() const = 0;

  AssetDefs::Type asset_type_;
  Q3FileDialog* file_dialog_;
  std::vector<std::uint32_t> provider_id_list_;
  int last_conv_index_;
  int last_mosaic_fill_index_;
  qt_fusion::QDateWrapper* acquisition_date_wrapper_;
};

class ImageryAssetWidget : public RasterAssetWidget,
                           public AssetWidgetBase {
 public:
  inline ImageryAssetWidget(QWidget* parent, AssetBase* base) :
      RasterAssetWidget(parent, AssetDefs::Imagery), AssetWidgetBase(base) { }
  AssetBase* GetAssetBase() const { return AssetWidgetBase::GetAssetBase(); }
};

class TerrainAssetWidget : public RasterAssetWidget,
                           public AssetWidgetBase {
 public:
  inline TerrainAssetWidget(QWidget* parent, AssetBase* base) :
      RasterAssetWidget(parent, AssetDefs::Terrain), AssetWidgetBase(base) { }
  AssetBase* GetAssetBase() const { return AssetWidgetBase::GetAssetBase(); }
};


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_RASTERASSETWIDGET_H_

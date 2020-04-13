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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_VECTORASSETWIDGET_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_VECTORASSETWIDGET_H_

#include "fusionui/.ui/vectorassetwidgetbase.h"
#include "fusion/fusionui/AssetWidgetBase.h"
#include <Qt/q3filedialog.h>
#include "common/khTypes.h"


namespace qt_fusion {
  class QDateWrapper;
}
class AssetBase;
class QStringList;
class VectorProductImportRequest;

class VectorAssetWidget : public VectorAssetWidgetBase,
                          public AssetWidgetBase {
 public:
  VectorAssetWidget(QWidget* parent, AssetBase* base);

  void Prefill(const VectorProductImportRequest& request);
  void AssembleEditRequest(VectorProductImportRequest* request);

  // inherited from VectorAssetWidgetBase
  virtual void AddSource();
  virtual void DeleteSource();
  virtual void CustomConversion(const QString&);

 private:
  void UpdateElevUnits(double conv);
  double GetElevUnits() const;

  void SetForceFeatureType(const int val);
  int GetForceFeatureType() const;

  void SetIgnoreBadFeaturesCheck(const bool val);
  bool GetIgnoreBadFeaturesCheck() const;

  void SetDoNotFixInvalidGeometriesCheck(const bool val);
  bool GetDoNotFixInvalidGeometriesCheck() const;

  Q3FileDialog* FileDialog();

  Q3FileDialog* file_dialog_;
  std::vector<std::uint32_t> provider_id_list_;
  int last_conv_index_;
  qt_fusion::QDateWrapper* acquisition_date_wrapper_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_VECTORASSETWIDGET_H_

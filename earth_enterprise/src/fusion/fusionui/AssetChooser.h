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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_ASSETCHOOSER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_ASSETCHOOSER_H_

#include <autoingest/.idl/storage/AssetDefs.h>
#include <fusionui/.idl/filehistory.h>

#include "fusion/gst/gstAssetGroup.h"
#include "assetchooserbase.h"
#include "fusion/fusionui/AssetDisplayHelper.h"
#include <Qt/qwidget.h>
#include <Qt/q3iconview.h>
#include <Qt/qevent.h>
using QIconViewItem = Q3IconViewItem;

class AssetChooser : public AssetChooserBase {
 public:
  enum Mode { Open, Save, SaveAs };

  struct AssetCategoryDef {
    AssetCategoryDef(AssetDefs::Type _type,
                     const std::string& _subtype)
        : type(_type),
          subtype(_subtype) {
    }
    AssetDefs::Type type;
    std::string subtype;
  };

  // Constructor creates Asset Chooser Dialog with view filter
  // for single acceptable type of input asset.
  // e.g. The Open Dialog filter items: 'type:subtype asset' and 'All'.
  AssetChooser(QWidget* parent, Mode m, AssetDefs::Type type,
               const std::string& subtype);

  // Constructor creates Asset Chooser Dialog with view filter
  // for more than one acceptable type of input asset.
  // e.g. Mercator Map Database asset can be created using either Flat or
  // Mercator Imagery Project asset. So, the Open Dialog filter items may be:
  // 'Mercator Imagery Project', 'Flat Imagery Project', 'All'.
  // The compatible_asset_defs specifies compatible types for input asset.
  // The all_compatible_assets_filter_text and all_compatible_assets_icon_name
  // are a text and an icon name for an additional item of Open Dialog filter
  // which displays assets of all compatible types.
  // Note:  The all_compatible_assets_filter_text- value is used as matching
  // string while filtering. So, the all_compatible_assets_icon_name can't be
  // specified with empty string of all_compatible_assets_filter_text parameter.
  // In such case, the 'all compatible assets item' won't be added to view
  // filter of OpenDialog.
  AssetChooser(
      QWidget* parent, Mode m,
      const std::vector<AssetCategoryDef> &compatible_asset_defs,
      const std::string &all_compatible_assets_filter_text,
      const std::string &all_compatible_assets_icon_name);

  // from QWidget
  virtual void keyPressEvent(QKeyEvent* e);
  virtual void customEvent(QEvent *e) final;

  // from QDialog
  virtual void accept();

  // from AssetChooserBase
  virtual void selectItem(QIconViewItem* i);
  virtual void chooseItem(QIconViewItem* i);
  virtual void cdtoparent();
  virtual void chooseAncestor(const QString& ancestor);
  virtual void nameChanged(const QString& str);
  virtual void chooseFilter(const QString& str);
  virtual void NewDirectory();

  QString getName() const;
  const gstAssetFolder& getFolder() const;
  bool getFullPath(QString& p) const;

 private:
  // Checks whether selected input asset has a compatible asset type.
  bool IsCompatibleAsset() const;

  // Restores the previous directory for corresponding type/subtype.
  void RestorePreviousDir(const AssetDisplayHelper& adh);

  void updateView(const gstAssetFolder& f);
  bool matchFilter(const gstAssetHandle item) const;

  Mode mode_;
  AssetDefs::Type type_;
  std::string subtype_;
  std::string match_string_;
  std::string all_compatible_assets_filter_text_;
  std::vector<AssetDisplayHelper> compatible_asset_types_;

  gstAssetFolder current_folder_;
  AssetChooserHistory chooser_history_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_ASSETCHOOSER_H_

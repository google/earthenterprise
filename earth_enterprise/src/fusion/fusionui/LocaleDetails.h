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


#ifndef KHSRC_FUSION_FUSIONUI_LOCALEDETAILS_H__
#define KHSRC_FUSION_FUSIONUI_LOCALEDETAILS_H__

#include <Qt/qobjectdefs.h>
#include <Qt/q3table.h>
#include <Qt/qcheckbox.h>
#include <Qt/q3scrollview.h>
#include <map>
#include "WidgetControllers.h"
#include <autoingest/.idl/storage/LayerConfig.h>


class LocaleColumn;
class LayerLegendWidget;

class LocaleDetails : public WidgetController {
  Q_OBJECT

  friend class LocaleColumn;
 public:
  typedef std::map<QString, LocaleConfig> LocaleMap;
  typedef LocaleMap::const_iterator       LocaleIterator;
  enum EditMode { LayerMode, FolderMode, KMLLayerMode,
                  ImageryLayerMode, TerrainLayerMode, MapLayerMode };

  static void Create(WidgetControllerManager &manager,
                     LayerLegendWidget *layer_legend,
                     EditMode edit_mode,
                     LocaleConfig *default_locale_config,
                     LocaleMap    *locale_map_config);
  ~LocaleDetails(void);
  void SyncToConfig(void);

 protected:
  virtual void SyncToWidgetsImpl(void) {
    MySyncToWidgetsImpl();
  }

private slots:
  void CellPressed(int row, int col, int btn, const QPoint& pos);
  void ContextMenu(int row, int col, const QPoint& pos);
  void FilterToggled(bool on);

 private:
  enum FieldEnum {
    FIELD_ICON,
    FIELD_NAME,
    FIELD_INIT_STATE,
    FIELD_DESC,
    FIELD_LOOKAT,
    FIELD_REQUIRED_VERSION,
    FIELD_KMLURL,
    FIELD_REQUIRED_VRAM,
    FIELD_PROBABILITY,
    FIELD_SAVE_LOCKED,
    FIELD_REQUIRED_USER_AGENT,
    FIELD_REQUIRED_CLIENT_CAPABILITIES,
    FIELD_CLIENT_CONFIG_SCRIPT_NAME,
    FIELD_3D_DATA_CHANNEL_BASE,
    FIELD_REPLICA_DATA_CHANNEL_BASE
  };


  LocaleDetails(WidgetControllerManager &manager,
                LayerLegendWidget *layer_legend,
                EditMode edit_mode,
                LocaleConfig *default_locale_config,
                LocaleMap *locale_map_config);
  void MySyncToWidgetsImpl(void);
  void AddLocale(const QString& name,
                 const LocaleConfig& config,
                 bool is_defaultable = true);

  Q3Table *table_;

  LocaleConfig *default_locale_config_;
  LocaleMap    *locale_map_config_;
  std::vector<FieldEnum> field_list_;
  khDeletingVector<LocaleColumn> locale_cols_;
  bool filtering_;
};

#endif  // !KHSRC_FUSION_FUSIONUI_LOCALEDETAILS_H__

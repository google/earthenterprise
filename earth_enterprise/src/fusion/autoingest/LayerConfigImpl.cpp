// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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



#include <autoingest/.idl/storage/LayerConfig.h>
#include <autoingest/.idl/storage/IconReference.h>
#include <third_party/rfc_uuid/uuid.h>
#include <khFileUtils.h>


void
LayerConfig::ApplyTemplate(const LayerConfig &templateConfig,
                           bool applyDisplayRules,
                           bool applyLegend)
{
  // Some things we never take from the template
  // channelId
  // assetRef
  // legend

  if (applyDisplayRules) {
    std::deque<std::uint32_t> oldStyleIds;
    SaveOldStyleIds(oldStyleIds);
    lodForce                = templateConfig.lodForce;
    displayRules            = templateConfig.displayRules;
    preserveTextLevel       = templateConfig.preserveTextLevel;
    allowFeatureDuplication = templateConfig.allowFeatureDuplication;
    layerContextScript      = templateConfig.layerContextScript;
    allowEmptyLayer         = templateConfig.allowEmptyLayer;
    TryRestoreOldStyleIds(oldStyleIds);
  }

  if (applyLegend) {
    isVisible     = templateConfig.isVisible;
    isEnabled     = templateConfig.isEnabled;
    isExpandable  = templateConfig.isExpandable;
    defaultLocale = templateConfig.defaultLocale;
    locales       = templateConfig.locales;
    skipLayer     = templateConfig.skipLayer;
    searchFields  = templateConfig.searchFields;
    meta          = templateConfig.meta;
    searchStyle   = templateConfig.searchStyle;
  }
}

bool LayerConfig::ValidateIconPresence(QString* message) const
{
  unsigned int orig_msg_length = message->length();
  for (const auto& disp : displayRules) {
    // only check external icon paths
    if (disp.site.style.icon.type == IconReference::External) {
      std::string path = khEnsureExtension(
          khComposePath(IconReference::CustomIconPath(),
                        disp.site.style.icon.href.toUtf8().constData()),
          ".png");
      if (!khExists(path)) {
        *message += "- Label icon missing: \"" + disp.site.style.icon.href + "\"\n";
      }
    }
  }

  if (defaultLocale.icon_.GetValue().type == IconReference::External) {
    std::string path = khEnsureExtension(
        khComposePath(IconReference::CustomIconPath(),
                      defaultLocale.icon_.GetValue().href.toUtf8().constData()),
        ".png");
    if (!khExists(path)) {
      *message += "- Layer icon missing: \"" + defaultLocale.icon_.GetValue().href + "\"\n";
    }
  }

  for (const auto& it : locales) {
    if (it.second.icon_.GetValue().type == IconReference::External) {
      std::string path = khEnsureExtension(
          khComposePath(IconReference::CustomIconPath(),
                        it.second.icon_.GetValue().href.toUtf8().constData()),
          ".png");
      if (!khExists(path)) {
        *message += "- Layer icon missing: \"" + it.second.icon_.GetValue().href + "\"\n";
      }
    }
  }

  return message->length() == static_cast<int>(orig_msg_length);
}

std::string
LayerConfig::MapName(std::string oldName)
{
  std::map<std::string, std::string> conversionMap;
  conversionMap["dot"] = "773";
  conversionMap["american-flag"] = "541";
  conversionMap["asian-flag"] = "539";
  conversionMap["dining"] = "536";
  conversionMap["fast-food"] = "542";
  conversionMap["french-flag"] = "538";
  conversionMap["italian-flag"] = "540";
  conversionMap["mexican-flag"] = "537";
  conversionMap["misc-dining"] = "519";
  conversionMap["bang"] = "771";
  conversionMap["bars"] = "518";
  conversionMap["building"] = "768";
  conversionMap["auto"] = "543";
  conversionMap["note"] = "774";
  conversionMap["parks"] = "769";
  conversionMap["recreation"] = "513";
  conversionMap["transportation"] = "512";
  conversionMap["one-dollar"] = "514";
  conversionMap["three-dollars"] = "516";
  conversionMap["two-dollars"] = "515";
  conversionMap["four-dollars"] = "517";

  return conversionMap[oldName];
}


void
LayerConfig::AfterLoad(LayerConfig::DeprecatedMembers &depmembers)
{
  // upgrade the old "named" icons to the newer ones
  if (depmembers.iconType == IconReference::Internal) {
    std::string newName = MapName(depmembers.icon);
    if ( newName != "" ) {
      depmembers.icon = newName;
    }
  }

  if (!legend.isEmpty()) {
    // strip old '!' and new '#' from front of legend
    if ((legend[0] == '!') || (legend[0] == '#')) {
      legend = legend.right(legend.length() - 1);
    }
  }


  // Fusion 2.4 or earlier - before locale stuff
  // copy over the default layer name from the layer
  // config to the default locale config
  if (defaultLocale.name_.GetValue().isEmpty()) {
    defaultLocale.name_ = depmembers.name;
    defaultLocale.icon_ = IconReference(depmembers.iconType,
                                        QString(depmembers.icon.c_str()));
    defaultLocale.is_checked_ = depmembers.initialShow;
    // desc, lookat & kmlUrl didn't exist before so defaults are good
    // The default for labels is a set with 'All Labels' pre-inserted
    // this is exactly what we want.
  }

  // Undo the full path in layerName that existed for a few weeks.
  // The legend already contains the path, no need to replicate it
  // "folder1|folder2|layera" ==> "layera"
  defaultLocale.name_ = defaultLocale.name_.GetValue().section('|',-1);

  // The defaultLocale should never have its default flags set.
  // As we add new fields (like desc) the defaults will default to true
  // this will clear them for the defaultLocale
  defaultLocale.ClearDefaultFlags();

  // A note on default visibility of folders:
  //
  // In principle the Google Earth client should completely ignore the default
  // visibility of folders since the visibility should be determined entirely
  // by the children's visibilities. However the GE client up to 4.2 does not
  // do that correctly and depends on the visibility to be set correctly in
  // the dbRoot generated by fusion.
  //
  // Fusion 3.0 introduced a new logic that would force the default visibility
  // for folders to false.  This breaks the 4.2 client (should work for 4.3
  // though). To support clients before 4.3 this logic was removed again
  // starting with fusion 3.0.3. The current setup will work with both
  // 4.2 and 4.3 clients. For reference below is the commented out code
  // that was removed.

  // REMOVED CODE:
  //
  // // Folders must always have their initial state set to false.
  // // The client should ignore this, but apparently there are
  // // cases when it doesn't. So, we will always make them false.
  // if (IsFolder()) {
  //   defaultLocale.is_checked_ = false;
  //   for (LocaleMapIterator it = locales.begin();
  //        it != locales.end(); ++it) {
  //     it->second.is_checked_.SetUseDefault();
  //   }
  // }
}

bool LayerConfig::IsKMLLayer(void) const {
  return !defaultLocale.kml_layer_url_.GetValue().isEmpty();
}


QString LayerConfig::DefaultNameWithPath(void) const {
  return legend.isEmpty() ?
    defaultLocale.name_.GetValue() :
    legend + "|" + defaultLocale.name_.GetValue();
}

QString LayerConfig::DefaultShortName(void) const {
  return defaultLocale.name_.GetValue();
}

LocaleConfig
LayerConfig::GetLocale(const QString& localeName) const
{
  std::map<QString, LocaleConfig>::const_iterator it = locales.find(localeName);
  // return the default locale config this specific is not set
  if (it == locales.end()) return defaultLocale;
  LocaleConfig newLocale = it->second;
  // set the individual fields from the default locale if required
  newLocale.BindDefaults(defaultLocale);

  return newLocale;
}

// Create a UUID for the LayerConfig.
// Should be called on initialization of a layer and should never change without
// care.  The UUID should remain constant for the life of the asset.
// Only assigns a UUID if the UUID is empty.
void
LayerConfig::AssignUuidIfEmpty(void)
{
  if (asset_uuid_ == "") {
    asset_uuid_ = create_uuid_string();
  }
}

void LayerConfig::ResetIds(void)
{
  channelId = 0;
  asset_uuid_.clear();
}

void
LayerConfig::AssignStyleIds(AvailId &avail)
{
  // called immediately after applying a template
  // assign ids for enabled features & sites and set disable ones to 0
  for (auto& disp : displayRules) {
    if (disp.feature.enabled()) {
      disp.feature.style.id = avail.next();
    } else {
      disp.feature.style.id = 0;
    }
    if (disp.site.enabled) {
      disp.site.style.id = avail.next();
    } else {
      disp.site.style.id = 0;
    }
  }
}

void
LayerConfig::SaveOldStyleIds(std::deque<std::uint32_t> &old)
{
  for (auto& disp : displayRules) {
    if (disp.feature.enabled() && (disp.feature.style.id != 0)) {
      old.push_back(disp.feature.style.id);
    }
    if (disp.site.enabled && (disp.site.style.id != 0)) {
      old.push_back(disp.site.style.id);
    }
  }
}

void
LayerConfig::TryRestoreOldStyleIds(std::deque<std::uint32_t> &old)
{

  for (auto& disp : displayRules) {
    if (disp.feature.enabled() && old.size()) {
      disp.feature.style.id = old.front();
      old.pop_front();
    } else {
      disp.feature.style.id = 0;
    }
    if (disp.site.enabled && old.size()) {
      disp.site.style.id = old.front();
      old.pop_front();
    } else {
      disp.site.style.id = 0;
    }
  }
}

bool
LayerConfig::HasFilterJS(void) const
{
  for (const auto& disp : displayRules) {
    if (disp.HasFilterJS())
      return true;
  }
  return false;
}

bool
LayerConfig::HasJS(void) const
{
  for (const auto& disp : displayRules) {
    if (disp.HasJS())
      return true;
  }
  return false;
}

bool
LayerConfig::HasSearchFields(void) const
{
  return (searchFields.size() > 0);
}

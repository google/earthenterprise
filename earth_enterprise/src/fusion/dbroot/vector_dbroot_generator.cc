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


#include "fusion/dbroot/vector_dbroot_generator.h"
#include "common/khStringUtils.h"
#include "common/khSimpleException.h"
#include "fusion/dbroot/vector_dbroot_context.h"

namespace {
const QString kLayerPathDelimiter("|");
}

VectorDbrootGenerator::VectorDbrootGenerator(
    VectorDbrootContext *vector_context,
    const QString &locale,
    const std::string &outfile) :
    ProtoDbrootGenerator(*vector_context, locale, outfile,
                         true /* map_strings_to_ids */),
    vector_context_(vector_context) {
}

QString VectorDbrootGenerator::GetUniqueName(const QString& layerPath) {
  if (layerPath.isEmpty())
    return QString();
  std::map<QString, QString>::iterator it = uniqueNameMap.find(layerPath);
  if (it != uniqueNameMap.end())
    return it->second;

  // insert the new layername in the layerName map
  // first check in the list of used names to see if it exists
  QString baseName = layerPath.section(kLayerPathDelimiter, -1);

  // add spaces to the end of the layer name to make it unique
  // this is to maintain nicer looking layernames for older
  // clients
  while (usedNames.find(baseName) != usedNames.end()) {
    baseName += " ";
  }
  uniqueNameMap[layerPath] = baseName;
  usedNames.insert(baseName);

  return baseName;
}

void VectorDbrootGenerator::Emit(geProtoDbroot::FileFormat output_format) {
  EmitNestedLayers();
  EmitStyleAttrs();
  EmitStyleMaps();
  EmitLODs();
  WriteProtobuf(output_format);
}

namespace {
 std::uint32_t ColorVecToABGR(const std::vector< unsigned int>  &vec) {
  // color vecs built internally by other fusion tools
  // even though stored as unsigned int, they are really unsigned char
  return (((vec[3] & 0xff) << 24) |
          ((vec[2] & 0xff) << 16) |
          ((vec[1] & 0xff) <<  8) |
          (vec[0] & 0xff));
}
}  // anonymous namespace

keyhole::dbroot::NestedFeatureProto* VectorDbrootGenerator::MakeProtoLayer(
    std::int32_t channel_id,
    const std::string &layer_name,
    const std::string &parent_name) {

  keyhole::dbroot::NestedFeatureProto* new_layer = NULL;

  if (!parent_name.empty()) {
    // find this layer's parent and make a new child there
    NamedLayerMap::iterator it = name_layer_map_.find(parent_name);
    if (it == name_layer_map_.end()) {
      throw khSimpleException("INTERNAL ERROR: Unable to find parent layer '")
          << parent_name << "' for layer '" << layer_name << "'";
    }
    new_layer = it->second->add_children();
  } else {
    // this layer has no parent, so just make it at the top level
    new_layer = outproto_.add_nested_feature();
  }

  // set the other fields in the new layer proto
  new_layer->set_channel_id(channel_id);

  // Add new layer to our lookup maps
  name_layer_map_.insert(std::make_pair(layer_name, new_layer));
  id_layer_map_.insert(std::make_pair(channel_id, new_layer));

  return new_layer;
}


void VectorDbrootGenerator::EmitNestedLayers(void) {
  // emit each layer
  for (std::set< unsigned int> ::const_iterator layerIdx =
           vector_context_->used_layer_ids_.begin();
       layerIdx != vector_context_->used_layer_ids_.end(); ++layerIdx) {
    const LayerConfig *layer = &vector_context_->config_.layers[*layerIdx];

    LocaleConfig localecfg = layer->GetLocale(locale_);

    // make some unique names we're going to use to map our flat structure to
    // the new nested structure
    std::string unique_layer_name(
        (const char*)GetUniqueName(layer->DefaultNameWithPath()).utf8());
    std::string unique_parent_name(
        (const char*)GetUniqueName(layer->legend).utf8());

    // get the localized strings for this locale
    std::string display_name((const char*)localecfg.name_.GetValue().utf8());
    std::string description(
        (const char*)localecfg.desc_.GetValue().section('\n', 0, 0).utf8());
    std::string look_at((const char*)localecfg.look_at_.GetValue().utf8());
    std::string kml_layer_url(
        (const char*)localecfg.kml_layer_url_.GetValue().utf8());
    std::string required_client_vram(
        (const char*)localecfg.required_client_vram_.GetValue().utf8());
    std::string required_client_version(
        (const char*)localecfg.required_client_version_.GetValue().utf8());
    std::string probability(
        (const char*)localecfg.probability_.GetValue().utf8());
    std::string required_user_agent(
        (const char*)localecfg.required_user_agent_.GetValue().utf8());
    std::string required_client_capabilities(
        (const char*)localecfg.required_client_capabilities_.GetValue().utf8());
    std::string client_config_script_name(
        (const char*)localecfg.client_config_script_name_.GetValue().utf8());
    std::int32_t diorama_data_channel_base =
        localecfg.diorama_data_channel_base_.GetValue();
    std::int32_t diorama_replica_data_channel_base =
        localecfg.diorama_replica_data_channel_base_.GetValue();

    // keep track of which icons and layers are used
    vector_context_->AddLegendIcon(localecfg.icon_.GetValue());
    std::string icon_path("icons/");
    icon_path += localecfg.icon_.GetValue().LegendHref();


    // Emit the new Protobuf version
    keyhole::dbroot::NestedFeatureProto *nested =
        MakeProtoLayer(layer->channelId,
                       unique_layer_name,
                       unique_parent_name);

    nested->set_asset_uuid(layer->asset_uuid_.c_str());
    SetProtoStringId(nested->mutable_display_name(), display_name);
    nested->set_is_visible(layer->isVisible);
    nested->set_is_enabled(layer->isEnabled);
    nested->set_is_checked(localecfg.is_checked_.GetValue());
    if (layer->IsFolder()) {
      keyhole::dbroot::FolderProto *folder = nested->mutable_folder();
      folder->set_is_expandable(layer->isExpandable);
    }
    {
      keyhole::dbroot::LayerProto *nested_layer = nested->mutable_layer();
      nested_layer->set_preserve_text_level(layer->preserveTextLevel);
    }
    nested->set_layer_menu_icon_path(icon_path);
    if (!description.empty()) {
      SetProtoStringId(nested->mutable_description(), description);
    }
    if (!SetLookAtString(nested, look_at)) {
      throw khSimpleException("Invalid LookAt string for layer '")
          << display_name << "'";
    }
    if (!kml_layer_url.empty()) {
      SetProtoStringId(nested->mutable_kml_url(), kml_layer_url);
    }
    nested->set_is_save_locked(localecfg.save_locked_);
    if (!required_client_vram.empty()) {
      nested->mutable_requirement()->set_required_vram(required_client_vram);
    }
    if (!required_client_version.empty()) {
      nested->mutable_requirement()->set_required_client_ver(
          required_client_version);
    }
    if (!probability.empty()) {
      nested->mutable_requirement()->set_probability(probability);
    }
    if (!required_user_agent.empty()) {
      nested->mutable_requirement()->set_required_user_agent(
          required_user_agent);
    }
    if (!required_client_capabilities.empty()) {
      nested->mutable_requirement()->set_required_client_capabilities(
          required_client_capabilities);
    }
    if (!client_config_script_name.empty()) {
      nested->set_client_config_script_name(client_config_script_name);
    }
    nested->set_diorama_data_channel_base(diorama_data_channel_base);
    nested->set_replica_data_channel_base(diorama_replica_data_channel_base);
  }
}

QString CompactStyleName(const QString& name) {
  static std::map<QString, QString> allocated_names;
  static int current_id = 0;

  std::map<QString, QString>::iterator found = allocated_names.find(name);
  if (found != allocated_names.end())
    return found->second;

  QString val = IntToBase62(current_id++).c_str();
  allocated_names[name] = val;

  return val;
}

void VectorDbrootGenerator::EmitStyleAttrs(void) {
  for (std::set< unsigned int> ::const_iterator usedIndex =
           vector_context_->used_layer_ids_.begin();
       usedIndex != vector_context_->used_layer_ids_.end(); ++usedIndex) {
    const LayerConfig *layer = &vector_context_->config_.layers[*usedIndex];

    // only emit styles for stream layers (KMLLayers and folders will
    // never have their styles used)
    if (!layer->IsStreamedLayer())
      continue;

    // Figure out the provider for this layer
    std::uint32_t providerId = vector_context_->config_.layerProviderIds[*usedIndex];
    if (!vector_context_->GetProvider(providerId)) {
      providerId = 0;
    } else {
      AddProvider(providerId);
    }

    // Process each display rule - each will have its own style
    for (std::vector<DisplayRuleConfig>::const_iterator rule =
           layer->displayRules.begin();
        rule != layer->displayRules.end(); ++rule) {
      // ----- NON-POINTS (e.g lines & polygons) -----
      if (rule->feature.featureType != VectorDefs::PointZ) {
        const FeatureConfig &feature = rule->feature;
        const StyleConfig &style = feature.style;
        QString feature_style_name = QString::number(style.id) + ":" +
            rule->name + QString(" (feature)");

        std::string iconName;
        int icon_width = 32;
        std::vector< unsigned int>  polyColor = style.polyColor;
        if ( feature.featureType == VectorDefs::LineZ &&
            feature.drawAsRoads == true ) {
          if ( feature.roadLabelType == FeatureConfig::Shield ) {
            // set iconName only for roads with shields
            IconReference icon_ref(style.icon.type, style.icon.href);
            iconName = "icons/" + icon_ref.NormalHref();
            vector_context_->AddNormalIcon(icon_ref);
            icon_width = vector_context_->GetIconWidth(icon_ref);
          }
          // the client gets the road geometry color from polyColor
          // not lineColor. The fusion config dialogs use the line color
          // for styling roads. Tell the client what it wants to hear.
          polyColor = style.lineColor;
        }


        // Protobuf version
        {
          {
            // remember where we put these new styles so we can emit the
            // new stylemap later
            std::int32_t new_pos = outproto_.style_attribute_size();
            // we only emit one style for lines/polys
            style_id_maps_.push_back(StyleIdMap(style.id,
                                                new_pos /* normal */,
                                                new_pos /* highlight */));
          }
          keyhole::dbroot::StyleAttributeProto* style_attribute =
              outproto_.add_style_attribute();
          style_attribute->set_style_id(
              CompactStyleName(feature_style_name).latin1());
          style_attribute->set_provider_id(providerId);
          style_attribute->set_poly_color_abgr(ColorVecToABGR(polyColor));
          style_attribute->set_line_color_abgr(ColorVecToABGR(style.lineColor));
          style_attribute->set_placemark_icon_color_abgr(
              ColorVecToABGR(style.icon.normalColor));
          style_attribute->set_label_color_abgr(
              ColorVecToABGR(style.label.normalColor));
          style_attribute->set_line_width(style.lineWidth);
          style_attribute->set_placemark_icon_scale(style.icon.normalScale);
          style_attribute->set_label_scale(style.label.normalScale);
          if (feature.polygonDrawMode == VectorDefs::FillAndOutline) {
            keyhole::dbroot::DrawFlagProto* draw_flag =
                style_attribute->add_draw_flag();
            draw_flag->set_draw_flag_type(
                keyhole::dbroot::DrawFlagProto::TYPE_FILL_AND_OUTLINE);
          } else if (feature.polygonDrawMode == VectorDefs::FillOnly) {
            keyhole::dbroot::DrawFlagProto* draw_flag =
                style_attribute->add_draw_flag();
            draw_flag->set_draw_flag_type(
                keyhole::dbroot::DrawFlagProto::TYPE_FILL_ONLY);
          } else if (feature.polygonDrawMode == VectorDefs::OutlineOnly) {
            keyhole::dbroot::DrawFlagProto* draw_flag =
                style_attribute->add_draw_flag();
            draw_flag->set_draw_flag_type(
                keyhole::dbroot::DrawFlagProto::TYPE_OUTLINE_ONLY);
          }
          SetProtoStringId(style_attribute->mutable_placemark_icon_path(),
                           iconName);
          style_attribute->set_placemark_icon_x(0);
          style_attribute->set_placemark_icon_y(0);
          style_attribute->set_placemark_icon_width(icon_width);
          style_attribute->set_placemark_icon_height(icon_width);  // sq icons
          // balloon stuff not used by features
        }
      }  /* if (rule->feature.featureType != VectorDefs::PointZ) */

      if (rule->site.enabled) {
        const SiteConfig &site = rule->site;
        const StyleConfig &style = site.style;
        QString site_style_name_normal = QString::number(style.id) + ":"
            + rule->name + QString(" (site-normal)");
        QString site_style_name_highlight = QString::number(style.id) + ":"
            + rule->name + QString(" (site-highlight)");

        QString normalIcon, highlightIcon;
        int icon_width = 32;
        if (site.enablePopup) {
          IconReference icon_ref(style.icon.type, style.icon.href);
          icon_width = vector_context_->GetIconWidth(icon_ref);
          normalIcon = "icons/";
          normalIcon += icon_ref.NormalHighlightHref().c_str();
          highlightIcon = normalIcon;
          vector_context_->AddNormalHighlightIcon(icon_ref);
          // even when we emit the stacked icons, we still want to generate
          // the separate normal and highlight so the client can
          // reference them in saved placemarks
          vector_context_->AddNormalIcon(icon_ref);
          vector_context_->AddHighlightIcon(icon_ref);
        } else {
          normalIcon = QString("");
          highlightIcon = QString("");
        }


        std::string balloon_style;
        std::string balloon_text;
        {
          if (site.balloonStyleMode == SiteConfig::Basic) {
            if (site.balloonInsertHeader) {
              balloon_text += "<h3>$[name]</h3>";
            }
            balloon_text += "$[description]<br>";
            if (site.balloonInsertDirections) {
              balloon_text += "<br>$[geDirections]";
            }
          } else {
            balloon_text += site.balloonText.toUtf8().constData();
          }

          balloon_style = "\"" + balloon_text + "\"";

          // add bgColor & textColor
          char color_buf[100];
          snprintf(color_buf, sizeof(color_buf),
                   " 0x%02x%02x%02x%02x 0x%02x%02x%02x%02x",
                   site.balloonBgColor[3], site.balloonBgColor[2],
                   site.balloonBgColor[1], site.balloonBgColor[0],
                   site.balloonFgColor[3], site.balloonFgColor[2],
                   site.balloonFgColor[1], site.balloonFgColor[0]);
          balloon_style += color_buf;
        }

        // Protobuf versions
        {
          {
            // remember where we put these new styles so we can emit the
            // new stylemap later
            std::int32_t new_pos = outproto_.style_attribute_size();
            style_id_maps_.push_back(StyleIdMap(style.id,
                                                new_pos   /* normal */,
                                                new_pos+1 /* highlight */));
          }

          // Populate StyleAttributeProto for normal placemarks
          keyhole::dbroot::StyleAttributeProto* normal_style =
              outproto_.add_style_attribute();
          normal_style->set_style_id(
              CompactStyleName(site_style_name_normal).latin1());
          normal_style->set_provider_id(providerId);
          // no poly color for sites
          // no line color for sites
          normal_style->set_placemark_icon_color_abgr(
              ColorVecToABGR(style.icon.normalColor));
          normal_style->set_label_color_abgr(
              ColorVecToABGR(style.label.normalColor));
          // no line width for sites
          normal_style->set_placemark_icon_scale(style.icon.normalScale);
          normal_style->set_label_scale(style.label.normalScale);
          SetProtoStringId(normal_style->mutable_placemark_icon_path(),
                           std::string((const char *)normalIcon.ascii()));
          normal_style->set_placemark_icon_x(0);
          normal_style->set_placemark_icon_y(0);
          normal_style->set_placemark_icon_width(icon_width);
          normal_style->set_placemark_icon_height(icon_width);  // sq icons
          keyhole::dbroot::PopUpProto* pop_up = normal_style->mutable_pop_up();
          pop_up->set_is_balloon_style(!balloon_text.empty());
          SetProtoStringId(pop_up->mutable_text(), balloon_text);
          pop_up->set_background_color_abgr(
              ColorVecToABGR(site.balloonBgColor));
          pop_up->set_text_color_abgr(
              ColorVecToABGR(site.balloonFgColor));

          // Make the placemark highlight style by copying the normal style and
          // making changes as necessary
          keyhole::dbroot::StyleAttributeProto* highlight_style =
              outproto_.add_style_attribute();
          highlight_style->CopyFrom(*normal_style);
          highlight_style->set_style_id(
              CompactStyleName(site_style_name_highlight).latin1());
          highlight_style->set_placemark_icon_color_abgr(
              ColorVecToABGR(style.icon.highlightColor));
          highlight_style->set_label_color_abgr(
              ColorVecToABGR(style.label.highlightColor));
          highlight_style->set_placemark_icon_scale(style.icon.highlightScale);
          highlight_style->set_label_scale(style.label.highlightScale);
          SetProtoStringId(highlight_style->mutable_placemark_icon_path(),
                           std::string((const char *)highlightIcon.ascii()));
          highlight_style->set_placemark_icon_y(icon_width);
        }
      }  /* if (rule->site.enabled) */
    }  /* foreach display rule */
  }  /* foreach used layer */
}

void VectorDbrootGenerator::EmitStyleMaps(void) {
  // Protobuf version
  for (std::vector<StyleIdMap>::const_iterator map = style_id_maps_.begin();
      map != style_id_maps_.end(); ++map) {
    keyhole::dbroot::StyleMapProto* style_map = outproto_.add_style_map();
    style_map->set_style_map_id(map->map_id_);
    style_map->set_normal_style_attribute(map->normal_style_id_);
    style_map->set_highlight_style_attribute(map->highlight_style_id_);
  }
}

void VectorDbrootGenerator::EmitLODs(void) {
  for (std::set< unsigned int> ::const_iterator usedIndex =
           vector_context_->used_layer_ids_.begin();
       usedIndex != vector_context_->used_layer_ids_.end(); ++usedIndex) {
    const LayerConfig *layer = &vector_context_->config_.layers[*usedIndex];

    // only emit LODs for stream layers (KMLLayers and folders don't have them)
    if (!layer->IsStreamedLayer())
      continue;

    // Get the LOD information
    std::string lodfile  = vector_context_->config_.layerLODFiles[*usedIndex];
    if (lodfile.empty()) {
      continue;
    }
    FILE *LODFILE = fopen(lodfile.c_str(), "r");
    if (LODFILE) {
      // set autoLOD flag to be on for polygon features
      // this helps the client draw polygons with a reasonable
      // frame-rate
      // also set the transition mode to pop off
      // this will help the client to avoid z-fighting
      std::uint32_t lodFlags = 0;
      bool isPolygonZ = false;
      for (std::vector<DisplayRuleConfig>::const_iterator rule =
             layer->displayRules.begin();
           rule != layer->displayRules.end(); ++rule) {
        if ( rule->feature.featureType == VectorDefs::PolygonZ ) {
          lodFlags |= 0x1;  // autoLOD = 1
          lodFlags |= 0x2;  // begin transition mode = 1
          lodFlags |= 0x4;  // end transition mode = 1
          isPolygonZ = true;
        }
      }
      LOD lod(GetUniqueName(layer->DefaultNameWithPath()).toUtf8().constData(),
              layer->channelId, lodFlags);
      char line[50];
      while (fgets(line, sizeof(line), LODFILE)) {
        lod.states.push_back(atoi(line));  // NOLINT
      }
      fclose(LODFILE);

      // New Protobuf version
      // lookup previously generated NestedFeatureProto
      keyhole::dbroot::NestedFeatureProto *nested = 0;
      {
        IdLayerMap::iterator found = id_layer_map_.find(layer->channelId);
        if (found == id_layer_map_.end()) {
          throw khSimpleException("INTERNAL ERROR: Unable to find protolayer "
                                  "for channel ") << layer->channelId;
        }
        nested = found->second;
      }
      if (isPolygonZ) {
        nested->set_feature_type(
            keyhole::dbroot::NestedFeatureProto::TYPE_POLYGON_Z);
      }
      for (unsigned int i = 0; i < lod.states.size(); ++i) {
        std::int32_t lod_level = lod.states[i];
        if ((lod_level > 0) && (lod_level != 30)) {
          keyhole::dbroot::ZoomRangeProto* zoom_range =
              nested->mutable_layer()->add_zoom_range();
          zoom_range->set_min_zoom(i);
          zoom_range->set_max_zoom(lod_level);
        }
      }
    }  /* if (LODFILE) */
  }
}

// Copyright 2017 Google Inc. Copyright 2018 the Open GEE Contributors.
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


#include "fusion/autoingest/JsonUtils.h"
#include <math.h>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include "common/khConstants.h"
#include "common/khStringUtils.h"
#include "common/projection.h"
#include "fusion/dbgen/.idl/DBConfig.h"
#include "fusion/autoingest/.idl/storage/MapLayerJSConfig.h"
#include "fusion/searchtabs/.idl/SearchTabs.h"

namespace {

// The request type constants for Map layers.
const char kImageryMapsRequest[] = "ImageryMaps";
const char kImageryMapsMercatorRequest[] = "ImageryMapsMercator";
const char kVectorRequest[] = "Vector";
const char kVectorMapsRasterRequest[] = "VectorMapsRaster";

// Utility to wrap the input string in quotes.
std::string Quoted(const std::string& input) {
  return "\"" + input + "\"";
}

// Utility to wrap the input QString in quotes.
std::string Quoted(const QString& input) {
  std::string temp(input.utf8());
  return Quoted(temp);
}

// Determines the request type for a map layer based on projection of 2D Map
// and a layer description (type, projection).
// Flat map:
//   Imagery layer:
//     ImageryMaps
//   VectorRaster layer:
//     -> VectorMapsRaster;
//
// Mercator map:
//   Imagery layer:
//     layer projection is Mercator -> ImageryMaps;
//     layer projection is Flat -> ImageryMapsMercator;
//   VectorRaster layer:
//     -> VectorMapsRaster;
std::string MapLayerRequestType(bool is_map_mercator,
                                const MapLayerJSConfig::Layer& layer) {
  std::string request_type;
  switch (layer.type_) {
    case MapLayerJSConfig::Layer::Imagery:
      if (layer.projection_ == MapLayerJSConfig::Layer::UndefinedProjection) {
        // 2D Flat/Mercator databases created with GEE-4.x, GEE-5.0.0-2.
        // Since GEE-4.x, GEE-5.0.0-2 do not allow to create a Mercator database
        // with flat imagery layer, it means that all layers in such databases
        // have the same projection as a database. So, reprojection is not
        // needed and we set request type returning original tiles.
        request_type = kImageryMapsRequest;
      } else {
        request_type = (
            is_map_mercator &&
            layer.projection_ == MapLayerJSConfig::Layer::FlatProjection) ?
            kImageryMapsMercatorRequest : kImageryMapsRequest;
      }
      break;
    case MapLayerJSConfig::Layer::Vector:
      request_type = kVectorRequest;
      throw khException(kh::tr("Vector layers not yet implemented"));
    case MapLayerJSConfig::Layer::VectorRaster:
      request_type = kVectorMapsRasterRequest;
      break;
    case MapLayerJSConfig::Layer::UndefinedType:
      throw khException(kh::tr("Undefined map layer type."));
    default:
      throw khException(kh::tr("Invalid map layer type."));
  }

  return request_type;
}

}  // namespace


std::string JsonUtils::MapsJsonBuffer(
  const std::string& database_url,
  const std::string& host_domain,
  const std::string& protocol,
  const std::vector<MapLayerJSConfig::Layer>& layers,
  const DbHeader& db_header,
  const std::string& locale) {
  std::string layers_json = JsonUtils::MapLayersJson(layers, db_header, locale);

  // Create the map of field name-value pairs for the output JSON.
  std::map<std::string, std::string> field_map;
  field_map["dbType"] = Quoted(std::string("gemap"));
  field_map["serverUrl"] = Quoted(database_url);
  field_map["useGoogleLayers"] =
      JsonUtils::JsonBool(db_header.use_google_imagery_);
  field_map["isAuthenticated"] =
      JsonUtils::JsonBool(database_url.find("https") == 0);
  field_map["projection"] =
      Quoted(std::string(db_header.is_mercator_ ? "mercator" : "flat"));
  field_map["layers"] = layers_json;
  return JsonObject(field_map);
}

std::string JsonUtils::GEJsonBuffer(
  const std::string& database_url,
  const std::string& host_domain,
  const std::string& protocol,
  const std::vector<RasterDBRootGenConfig::Layer>& raster_layers,
  const std::vector<LayerConfig>& vector_layers,
  const std::string& locale) {
  std::string layers_json =
      JsonUtils::GELayersJson(raster_layers, vector_layers, locale);

  // Create the map of field name-value pairs for the output JSON.
  std::map<std::string, std::string> field_map;
  field_map["dbType"] = Quoted(std::string("gedb"));
  field_map["serverUrl"] = Quoted(database_url);
  field_map["isAuthenticated"] =
      JsonUtils::JsonBool(protocol.compare("https") == 0);
  field_map["layers"] = layers_json;
  return JsonObject(field_map);
}

std::string JsonUtils::SearchTabsJson(
  const std::vector<SearchTabDefinition>& search_tabs) {
  std::vector<std::string> search_tab_json_objects;
  // Create the JSON entries for each of the search tabs.
  std::vector<SearchTabDefinition>::const_iterator iter = search_tabs.begin();
  for (; iter != search_tabs.end(); ++iter) {
    search_tab_json_objects.push_back(SearchTabJson(*iter));
  }
  return JsonArray(search_tab_json_objects);
}

std::string JsonUtils::SearchTabJson(const SearchTabDefinition& search_tab) {
  // The JSON publishes the MapsSearchAdapter for all search url's, since the
  // JS processing code is the same for all JSON based clients.
  std::string url = search_tab.url.utf8().data();
  url = ReplaceString(url, kEarthClientSearchAdapter, kMapsSearchAdapter);

  // Collect the arg fields into a vector.
  std::vector<std::string> arg_array_entries;
  for (int i = 0; i < static_cast<int>(search_tab.fields.size()); ++i) {
    const SearchTabDefinition::Field& field = search_tab.fields[i];
    std::map<std::string, std::string> arg_field_map;
    arg_field_map["screenLabel"] = Quoted(field.label);
    arg_field_map["urlTerm"] = Quoted(field.key);

    std::string arg_entry = JsonObject(arg_field_map);
    arg_array_entries.push_back(arg_entry);
  }
  std::string args_array = JsonArray(arg_array_entries);

  // Create the map of field name-value pairs for the output JSON.
  std::map<std::string, std::string> field_map;
  field_map["tabLabel"] = Quoted(search_tab.label);
  field_map["url"] = Quoted(url);
  field_map["args"] = args_array;
  return JsonObject(field_map);
}

std::string JsonUtils::GELayersJson(
  const std::vector<RasterDBRootGenConfig::Layer>& raster_layers,
  const std::vector<LayerConfig>& vector_layers,
  const std::string& locale) {
  std::vector<std::string> layer_json_entries;
  // Create the JSON entries for each of the search tabs, starting with
  // raster layers.
  std::vector<RasterDBRootGenConfig::Layer>::const_iterator raster_iter =
    raster_layers.begin();
  for (; raster_iter != raster_layers.end(); ++raster_iter) {
    layer_json_entries.push_back(GERasterLayerJson(*raster_iter, locale));
  }
  // Now the vector layers.
  std::vector<LayerConfig>::const_iterator iter = vector_layers.begin();
  for (; iter != vector_layers.end(); ++iter) {
    const LayerConfig& layer = *iter;
    if (!layer.skipLayer) {
      layer_json_entries.push_back(GELayerJson(layer, locale));
    }
  }
  return JsonArray(layer_json_entries);
}

std::string JsonUtils::GELayerJson(const LayerConfig& layer,
                                   const std::string& locale) {
  LocaleConfig locale_config = layer.GetLocale(locale.c_str());

  // Create the map of field name-value pairs for the output JSON.
  std::map<std::string, std::string> field_map;
  field_map["id"] = Quoted(layer.asset_uuid_);
  field_map["label"] = Quoted(locale_config.name_.GetValue());
  field_map["description"] = Quoted(locale_config.desc_.GetValue());
  field_map["isInitiallyOn"] =
    JsonUtils::JsonBool(locale_config.is_checked_.GetValue());
  field_map["isExpandable"] = JsonUtils::JsonBool(layer.isExpandable);
  field_map["isEnabled"] = JsonUtils::JsonBool(layer.isEnabled);
  field_map["isVisible"] = JsonUtils::JsonBool(layer.isVisible);
  std::string lookAt(locale_config.look_at_.GetValue().utf8());
  field_map["lookAt"] = JsonUtils::LookAtJson(lookAt);
  return JsonObject(field_map);
}

std::string JsonUtils::GERasterLayerJson(
  const RasterDBRootGenConfig::Layer& layer,
  const std::string& locale) {
  LegendLocale locale_config = layer.legend_.GetLegendLocale(locale.c_str());

  // Create the map of field name-value pairs for the output JSON.
  std::map<std::string, std::string> field_map;
  field_map["id"] = Quoted(layer.asset_uuid_);
  field_map["label"] = Quoted(locale_config.name.GetValue());
  field_map["description"] = Quoted(locale_config.desc.GetValue());
  field_map["isInitiallyOn"] =
    JsonUtils::JsonBool(locale_config.isChecked.GetValue());
  field_map["isExpandable"] = JsonUtils::JsonBool(false);
  field_map["isEnabled"] = JsonUtils::JsonBool(true);
  field_map["isVisible"] = JsonUtils::JsonBool(true);
  std::string lookAt(locale_config.lookAt.GetValue().utf8());
  field_map["lookAt"] = JsonUtils::LookAtJson(lookAt);
  return JsonObject(field_map);
}

std::string JsonUtils::MapLayersJson(
  const std::vector<MapLayerJSConfig::Layer>& layers,
  const DbHeader& db_header,
  const std::string& locale) {
  // Create the JSON entries for each of the layers.
  std::vector<std::string> layer_json_entries;
  for (int i = 0; i < static_cast<int>(layers.size()); ++i) {
    const MapLayerJSConfig::Layer& layer = layers[i];
    layer_json_entries.push_back(
        JsonUtils::MapLayerJson(layer, db_header, locale));
  }
  return JsonArray(layer_json_entries);
}

std::string JsonUtils::MapLayerJson(const MapLayerJSConfig::Layer& layer,
                                    const DbHeader& db_header,
                                    const std::string& locale) {
  // Get the legend for the specified locale.
  const LegendLocale& legend_locale = layer.legend_.GetLegendLocale(locale.c_str());
  // Create the map of field name-value pairs for the output JSON.
  std::string icon_name =
      "icons/" + legend_locale.icon.GetValue().LegendHref();
  bool is_imagery = (layer.type_ == MapLayerJSConfig::Layer::Imagery);
  double opacity = 1.0;
  std::map<std::string, std::string> field_map;
  field_map["id"] = Itoa(layer.channel_id_);
  field_map["version"] = Itoa(layer.index_version_);
  field_map["label"] = Quoted(legend_locale.name.GetValue());
  field_map["icon"] = Quoted(icon_name);
  field_map["initialState"] =
    JsonUtils::JsonBool(legend_locale.isChecked.GetValue());
  field_map["isPng"] = JsonUtils::JsonBool(!is_imagery);
  field_map["opacity"] = DoubleToString(opacity);

  field_map["requestType"] = Quoted(
      MapLayerRequestType(db_header.is_mercator_, layer));

  // TODO: add new locale fields
  // opacity (needs locale entry and ui)
  // look_at (needs ui)
  std::string lookAt(legend_locale.lookAt.GetValue().utf8());
  field_map["lookAt"] = JsonUtils::LookAtJson(lookAt);
  return JsonObject(field_map);
}

std::string JsonUtils::LookAtJson(const std::string& lookAtSpec) {
  if (lookAtSpec.empty())
    return Quoted(std::string("none"));
  // LookAtSpec format is :
  // longitude|latitude|altitude|...
  // we only care for these 3.
  std::vector<std::string> tokens;
  TokenizeString(lookAtSpec, tokens, "|");

  std::map<std::string, std::string> field_map;

  if (tokens.size() >= 3) {
    double altitude = strtod(tokens[2].c_str(), NULL);
    int zoomLevel = AltitudeToZoomLevel(altitude);

    // Create the map of field name-value pairs for the output JSON.
    field_map["lat"] = tokens[1];
    field_map["lng"] = tokens[0];
    field_map["zoom"] = Itoa(zoomLevel);
    field_map["altitude"] = DoubleToString(altitude);
  } else {
    int zoomLevel = AltitudeToZoomLevel(0);
    field_map["lat"] = "0";
    field_map["lng"] = "0";
    field_map["zoom"] = Itoa(zoomLevel);
    field_map["altitude"] = "0";
  }
  return JsonObject(field_map);
}

std::string JsonUtils::JsonObject(
  const std::map<std::string, std::string>& field_map) {
  std::ostringstream buffer;
  buffer << "{\n";
  std::map<std::string, std::string>::const_iterator iter = field_map.begin();
  for (; iter != field_map.end(); ++iter) {
    if (iter != field_map.begin()) {
      buffer << ",\n";  // Note: IE6 doesn't like trailing commas.
    }
    buffer << iter->first << " : " << iter->second;
  }
  buffer << "\n}\n";
  return buffer.str();
}

std::string JsonUtils::JsonArray(
  const std::vector<std::string>& array_entries) {
  std::ostringstream buffer;
  buffer << "\n[\n";
  for (int i = 0; i < static_cast<int> (array_entries.size()); ++i) {
    if (i != 0) {
      buffer << ",\n";  // Note: IE6 doesn't like trailing commas.
    }
    buffer << array_entries[i];
  }
  buffer << "\n]\n";
  return buffer.str();
}

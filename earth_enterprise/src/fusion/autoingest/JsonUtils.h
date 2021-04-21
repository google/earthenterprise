/*
 * Copyright 2017 Google Inc. Copyright 2018 the Open GEE Contributors.
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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_JSONUTILS_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_JSONUTILS_H_

#include <string>
#include <vector>
#include <map>
#include "fusion/autoingest/.idl/storage/MapLayerJSConfig.h"
#include "fusion/dbgen/.idl/DBConfig.h"
#include "fusion/autoingest/.idl/storage/LayerConfig.h"
#include "fusion/autoingest/.idl/storage/RasterDBRootConfig.h"

class SearchTabDefinition;
class QString;

// JsonUtils is a static utility class for formatting JSON text for
// Earth and Maps databases.
class JsonUtils {

 public:
  // Create the JSON buffer for a Google Earth Database.
  // database_url: the url of the GE server
  //               (i.e., http://mysite.com/default_ge/)
  // host_url: the url of the server http host (i.e., http://mysite.com/)
  // protocol: the protocol for the site (i.e., "http" or "https")
  // raster_layers: the list of raster layer definitions
  // vector_layers: the list of vector layer definitions
  // locale: the locale to use for the output JSON
  // return a string with the fully constructed JSON object.
  static std::string GEJsonBuffer(
    const std::string& database_url,
    const std::string& host_url,
    const std::string& protocol,
    const std::vector<RasterDBRootGenConfig::Layer>& raster_layers,
    const std::vector<LayerConfig>& vector_layers,
    const std::string& locale);

  // Create the JSON buffer for a Google Maps Database.
  // database_url: the url of the Maps server
  //               (i.e., http://mysite.com/default_map)
  // host_domain: the domain of the server (i.e.,mysite.com)
  // protocol: the protocol for the site (i.e., "http" or "https")
  // layers: the list of layer definitions
  // db_header: the database information
  // locale: the locale to use for the output JSON
  // return a string with the fully constructed JSON object.
  static std::string MapsJsonBuffer(
    const std::string& database_url,
    const std::string& host_domain,
    const std::string& protocol,
    const std::vector<MapLayerJSConfig::Layer>& layers,
    const DbHeader& db_header,
    const std::string& locale);

 protected:
  //this was private because the class was intended to be 
  //"static" but made protected to allow for unit testing
  JsonUtils() {}  

  // Create a JSON string for a set of search tabs.
  // search_tabs: the list of search tab definitions
  // return: the JSON string for the search tabs
  static std::string SearchTabsJson(
     const std::vector<SearchTabDefinition>& search_tabs);

  // Create a JSON string for a single search tab.
  // search_tab: the search tab definition
  // return: the JSON string for the search tab
  static std::string SearchTabJson(const SearchTabDefinition& search_tab);

  // Create a JSON string for a set of Google Earth layers.
  // raster_layers: the list of raster layer definitions
  // vector_layers: the list of vector layer definitions
  // return: the JSON string for the layer
  static std::string GELayersJson(
    const std::vector<RasterDBRootGenConfig::Layer>& raster_layers,
    const std::vector<LayerConfig>& vector_layers,
    const std::string& locale);

  // Create a JSON string for a single Google Earth layer.
  // layer: the layer definition
  // locale: the locale to use for the output JSON
  // return: the JSON string for the layer
  static std::string GELayerJson(const LayerConfig& layer,
                                 const std::string& locale);

  // Create a JSON string for a single Google Earth raster layer.
  // layer: the raster layer definition
  // locale: the locale to use for the output JSON
  // return: the JSON string for the layer
  static std::string GERasterLayerJson(
    const RasterDBRootGenConfig::Layer& layer,
    const std::string& locale);

  // Create a JSON string for a set of Map layers.
  // layers: the list of layer definitions
  // db_header: the database information
  // locale: the locale to use for the output JSON
  // return: the JSON string for the layer
  static std::string MapLayersJson(
    const std::vector<MapLayerJSConfig::Layer>& layers,
    const DbHeader& db_header,
    const std::string& locale);

  // Create a JSON string for a single Map layer.
  // layer: the layer definition
  // db_header: the database information
  // locale: the locale to use for the output JSON
  // return: the JSON string for the layer
  static std::string MapLayerJson(const MapLayerJSConfig::Layer& layer,
                                  const DbHeader& db_header,
                                  const std::string& locale);

  // Utility to output a JSON object of JSON field/value pairs.
  // field_map: the collection of field (name --> values). An empty map is
  //            valid.
  // returns: the string format of the JSON object.
  static std::string JsonObject(
    const std::map<std::string, std::string>& field_map);

  // Utility to output a JSON array of JSON string values.
  // array_entries: the vector of array entries. An empty vector is valid.
  // returns: the string format of the JSON array.
  static std::string JsonArray(const std::vector<std::string>& array_entries);

  // Utility to return the "true" or "false" string for the bool value.
  // value: the boolean value to test
  // return: "true" if value is true, otherwise "false"
  static std::string JsonBool(bool value) {
    return value ? "true" : "false";
  }

  // Utilitiy for converting a look-at point spec to JSON object.
  // lookAtSpec: a string with the following format
  //   longitude|latitude|altitude|...
  //   we only care for these 3.
  // return: the string JSON object for the look-at point.
  static std::string LookAtJson(const std::string& lookAtSpec);
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_JSONUTILS_H_

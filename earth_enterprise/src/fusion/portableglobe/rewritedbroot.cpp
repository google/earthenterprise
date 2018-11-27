// Copyright 2018 the Open GEE Contributors
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

// Functions that rewrite various aspects of the dbroot.

#include "rewritedbroot.h"
#include <khGetopt.h>
#include <fstream>   // NOLINT(readability/streams)
#include <set>
#include <string>
#include "common/khConstants.h"
#include "common/khFileUtils.h"
#include "common/khStringUtils.h"
#include "common/gedbroot/proto_dbroot.h"
#include "fusion/portableglobe/shared/serverreader.h"

void AddStringToProto(const std::string& str,
                      keyhole::dbroot::StringIdOrValueProto* proto) {
  proto->clear_string_id();
  proto->set_value(str);
}

/**
 * Saves all icons it discovers in the dbroot to the given directory. Original
 * file names are preserved.
 *
 * @param path Path to directory where icons are to be stored.
 * @param server Address of server where icons are to come from.
 * @param dbroot geProtoDbroot containing icon references.
 */
void SaveIcons(const std::string &path,
               const std::string &server,
               geProtoDbroot *dbroot) {
  std::set<std::string> icon_paths;

  khEnsureParentDir(path + "/icon.png");

  // Icons are referenced from two places in the dbroot
  //   - style_attribute[].placemark_icon_path
  //   - nested_feature[].layer_menu_icon_path

  // Loop through all of the style attributes gathering icon_paths
  for (int i = 0; i < dbroot->style_attribute_size(); ++i) {
    const keyhole::dbroot::StyleAttributeProto &style =
        dbroot->style_attribute(i);
    if (style.has_placemark_icon_path()) {
      // This search tab has an icon path to process

      // let's see if we have the icon path directly in the placemark_icon_path
      // field or if it is redirected to the translation_entry table
      const keyhole::dbroot::StringIdOrValueProto &string_id_or_value =
          style.placemark_icon_path();
      if (string_id_or_value.has_value()) {
        // the icon path is stored directly in the placemark_icon_path field
        std::string icon_path = string_id_or_value.value();
        icon_paths.insert(icon_path);
      } else {
        // the icon path is in the translation_entry table
        // GetTraslationEntryById will throw if not found
        keyhole::dbroot::StringEntryProto *string_entry =
            dbroot->GetTranslationEntryById(string_id_or_value.string_id());
        std::string icon_path = string_entry->string_value();
        icon_paths.insert(icon_path);
      }
    }
  }

  // Loop through all of the nested features (layers) gathering icon_paths
  for (int i = 0; i < dbroot->nested_feature_size(); ++i) {
    const keyhole::dbroot::NestedFeatureProto &nested =
        dbroot->nested_feature(i);
    if (nested.has_layer_menu_icon_path()) {
      // This nested feature has an icon path to process
      // layer_menu_icon_path is stored as a simple string. There is no
      // possible redirection to the translation_entry table
      std::string icon_path = nested.layer_menu_icon_path();
      icon_paths.insert(icon_path);
    }
  }

  // Now that we have all the icons in a set, loop through the set.
  // Get each one from the server and save it to the given icon directory.
  std::string prefix = "icons/";
  for (std::set<std::string>::iterator iter = icon_paths.begin();
       iter != icon_paths.end(); ++iter) {
    // icon paths were added to set exactly as they were in the dbroot
    // this means they have an "icons/" prefix. We want to strip that prefix.
    if (!iter->empty()) {
      if (!StartsWith(*iter, prefix)) {
        throw khSimpleException("Icon path missing ")
            << prefix << " prefix: " << *iter;
      }
      std::string relpath = iter->substr(prefix.size());
      ServerReader::SaveServerData(path + "/" + relpath,
                                   server,
                                   "/flatfile?lf-0-icons/" + relpath);
    }
  }
}

/**
 * Removes references to layers of data packet types that are not included
 * in the glb.
 * @param data_type Type of data included (ALL, IMAGERY, TERRAIN, or VECTOR).
 * @param dbroot geProtoDbroot to be modified
 */
void RemoveUnusedLayers(const std::string &data_type,
                        geProtoDbroot *dbroot) {
  if (data_type != kCutAllDataFlag) {
    if (data_type != kCutImageryDataFlag) {
      dbroot->set_imagery_present(false);
    }
    if (data_type != kCutTerrainDataFlag) {
      dbroot->set_terrain_present(false);
    }
    if (data_type != kCutVectorDataFlag) {
      dbroot->clear_nested_feature();
    }
  }
}

/**
 * Replaces server and port in search tabs with the given server and port.
 * @param server Server to which search queries should be addressed.
 * @param port Port of server on which search queries should be addressed.
 * @param dbroot geProtoDbroot to be modified
 */
void ReplaceSearchServer(const std::string &search_service,
                         geProtoDbroot *dbroot) {
  assert(dbroot);
  // Add search to end snippet.
  keyhole::dbroot::EndSnippetProto* end_snippet =
      dbroot->mutable_end_snippet();
  // Ensure deprecated search tabs are clear.
  end_snippet->clear_search_tab();
  keyhole::dbroot::EndSnippetProto::SearchConfigProto* search_config =
      end_snippet->mutable_search_config();
  search_config->clear_search_server();
  keyhole::dbroot::EndSnippetProto::SearchConfigProto::SearchServer*
      search_server = search_config->add_search_server();
  // No supplemental UI allowed in Portable.
  search_server->clear_supplemental_ui();
  // Only allow the one-box search for Portable.
  std::string url;
  // If no service provided, use the default one relative to the dbRoot.
  if (search_service.empty()) {
    url = kPortableSearchUrl;
  } else {
    url = search_service;
  }
  AddStringToProto(url, search_server->mutable_url());
}

/**
 * Removes the reference to the server that the dbroot references for 
 * historical imagery.
 * @param dbroot geProtoDbroot to be modified
 */
void DisableTimeMachine(geProtoDbroot *dbroot) {
  assert(dbroot); 
  keyhole::dbroot::EndSnippetProto* end_snippet =
      dbroot->mutable_end_snippet();
  keyhole::dbroot::TimeMachineOptionsProto* time_machine_options =
      end_snippet->mutable_time_machine_options(); 
  time_machine_options->set_server_url("");
}

/**
 * Replaces server and port in kml references with the given server and port.
 * It also saves the old kml references so that these can be pulled into
 * local files.
 *
 * The kml_url_to_file_map_file format is the source url followed by the new
 * local file name:
 *   <url_to_original_kml_file1> <relative_file1_name_in_portable_globe>\n
 *   ...
 *   <url_to_original_kml_fileN> <relative_fileN_name_in_portable_globe>\n
 *
 * @param new_kml_base_url Base url to which kml references will be addressed.
 * @param kml_url_to_file_map_file File storing pairs of kml source urls and
 *                                 corresponding local kml file name.
 * @param preserve_kml_filenames Flag to keep the original filenames of kml 
 *                                references (true) or to use a sequence of
 *                                generated filenames.
 * @param dbroot geProtoDbroot to be modified
 */
void ReplaceReferencedKml(const std::string &new_kml_base_url,
                          const std::string &kml_url_to_file_map_file,
                          const bool preserve_kml_filenames,
                          geProtoDbroot *dbroot) {
  std::ofstream fout;

  // Open file for storing kml urls and local files.
  if ( !kml_url_to_file_map_file.empty() )
  {
    khEnsureParentDir(kml_url_to_file_map_file);
    fout.open(kml_url_to_file_map_file.c_str(), std::ios::out);
    if (!fout) {
      notify(NFY_FATAL, "Unable to write kml map file: %s",
           kml_url_to_file_map_file.c_str());
    }
  }
  std::string kml_url;
  std::string kml_file_name;

  // Loop through all of the nested layer entries.
  int num_kml_files = 0;
  for (int i = 0; i < dbroot->nested_feature_size(); ++i) {
    keyhole::dbroot::NestedFeatureProto *nested =
        dbroot->mutable_nested_feature(i);
    if (nested->has_kml_url()) {
      // This layer has a kml_url. Make a local filename for it.
      char str[20];
      snprintf(str, sizeof(str),
               kDbrootKmlFileNameTemplate.c_str(),
               num_kml_files);
      num_kml_files += 1;
      kml_file_name = str;

      // let's see if we have the url directly in the kml_url field or if it
      // is redirected to the translation_entry table
      keyhole::dbroot::StringIdOrValueProto *string_id_or_value =
          nested->mutable_kml_url();
      if (string_id_or_value->has_value()) {
        // the url is stored directly in the kml_url field
        std::string kml_url = string_id_or_value->value();
        if ( preserve_kml_filenames ) {
            kml_file_name = kml_url.substr(kml_url.find_last_of("/") + 1);
        }
        string_id_or_value->set_value(new_kml_base_url + "/" + kml_file_name);
      } else {
        // the url is in the translation_entry table
        // GetTraslationEntryById will throw if not found
        keyhole::dbroot::StringEntryProto *string_entry =
            dbroot->GetTranslationEntryById(string_id_or_value->string_id());
        kml_url = string_entry->string_value();
        if ( preserve_kml_filenames ) {
            kml_file_name = kml_url.substr(kml_url.find_last_of("/") + 1);
        }
        string_entry->set_string_value(new_kml_base_url + "/" + kml_file_name);
      }
    }
    // Write out map entry to file if one is specified.   
    if ( fout ) 
      fout << kml_url << " " << kml_file_name << std::endl;
  }
  if ( fout ) fout.close();
}

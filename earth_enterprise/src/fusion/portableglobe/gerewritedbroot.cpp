// Copyright 2017 Google Inc.
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
// Rewrites the dbroot for the portable server.
// Changes the search server and port in the dbroot
// and copies all icons  referenced in the dbroot
// into a given directory.

#include <khGetopt.h>
#include <fstream>   // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <set>
#include <string>
#include "common/khConstants.h"
#include "common/khFileUtils.h"
#include "common/khStringUtils.h"
#include "common/gedbroot/proto_dbroot.h"
#include "fusion/gst/gstEarthStream.h"
#include "fusion/portableglobe/shared/serverreader.h"

// TODO: Move code to separate file with tests.

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
 * @param dbroot geProtoDbroot to be modified
 */
void ReplaceReferencedKml(const std::string &new_kml_base_url,
                          const std::string &kml_url_to_file_map_file,
                          geProtoDbroot *dbroot) {
  std::ofstream fout;

  // Open file for storing kml urls and local files.
  khEnsureParentDir(kml_url_to_file_map_file);
  fout.open(kml_url_to_file_map_file.c_str(), std::ios::out);
  if (!fout) {
    notify(NFY_FATAL, "Unable to write kml map file: %s",
           kml_url_to_file_map_file.c_str());
  }

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
      std::string kml_file_name = str;

      // let's see if we have the url directly in the kml_url field or if it
      // is redirected to the translation_entry table
      keyhole::dbroot::StringIdOrValueProto *string_id_or_value =
          nested->mutable_kml_url();
      if (string_id_or_value->has_value()) {
        // the url is stored directly in the kml_url field
        std::string kml_url = string_id_or_value->value();
        fout << kml_url << " " << kml_file_name << std::endl;
        string_id_or_value->set_value(new_kml_base_url + "/" + kml_file_name);
      } else {
        // the url is in the translation_entry table
        // GetTraslationEntryById will throw if not found
        keyhole::dbroot::StringEntryProto *string_entry =
            dbroot->GetTranslationEntryById(string_id_or_value->string_id());
        std::string kml_url = string_entry->string_value();
        fout << kml_url << " " << kml_file_name << std::endl;
        string_entry->set_string_value(new_kml_base_url + "/" + kml_file_name);
      }
    }
  }

  fout.close();
}


void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "E.g. gerewritedbroot --source=http://bplinux "
          "--icon_directory=/tmp/icons --dbroot_file=/tmp/dbroot "
          "--kml_map_file=/tmp/kml_map\n"
          "\nusage: %s \\\n"
          "           --source=<source_string> \\\n"
          "           --icon_directory=<icon_directory_string> \\\n"
          "           --dbroot_file=<dbroot_file_string> \\\n"
          "           --search_service=<search_service_url_string> \\\n"
          "           --kml_map_file=<kml_map_file_string> \\\n"
          "           --kml_server=<kml_server_string> \\\n"
          "           --kml_port=<kml_port_string> \\\n"
          "           --kml_url_path=<kml_url_path_string> \\\n"
          "           --use_ssl_for_kml=<use_ssl_for_kml_bool> \\\n"
          "\n"
          " Reads dbroot and rewrites the search tabs so that they point\n"
          " at the given search server and port. Creates a directory\n"
          " of all of the icons referred to by the dbroot. Creates a file.\n"
          " listing all kml files that are referenced in the dbroot and.\n"
          " gives their new name on the kml server.\n"
          "\n"
          " Required:\n"
          "   --source:        Server whose dbroot should be rewritten.\n"
          "   --icon_directory: Directory where icons should be stored.\n"
          "   --dbroot_file:   File where new dbroot should be stored.\n"
          "   --kml_map_file:  File where kml map of source urls to local\n"
          "                    files should be stored.\n"
          " Options:\n"
          "   --data_type:     Type of data packet to cut (ALL, IMAGERY,\n"
          "                    TERRAIN, or VECTOR).\n"
          "                    Default is ALL.\n"
          "   --search_service: Url to search service. If none is provided\n"
          "                    then uses relative url for standard Portable\n"
          "                    search.\n"
          "   --kml_server:    Server to be used for kml files in the\n"
          "                    dbroot.\n"
          "                    Default: localhost\n"
          "   --kml_port:      Port to be used for kml files in the\n"
          "                    dbroot.\n"
          "                    Default: 8888\n"
          "   --kml_url_path:  Path in new url to prefix kml file name.\n"
          "                    Default: kml\n"
          "   --use_ssl_for_kml:  Use https:// instead of http:// for\n"
          "                    accessing kml files.\n"
          "                    Default: false\n",
          progn.c_str());
  exit(1);
}


int main(int argc, char *argv[]) {
  std::string progname = argv[0];
  bool help;
  bool use_ssl_for_kml = false;
  std::string source;
  std::string icon_directory;
  std::string dbroot_file;
  std::string kml_map_file;
  std::string data_type = kCutAllDataFlag;
  std::string search_service = "";
  std::string kml_server = "";
  std::string kml_port = "";
  std::string kml_url_path = "kml";

  khGetopt options;
  options.flagOpt("help", help);
  options.flagOpt("?", help);
  options.flagOpt("use_ssl_for_kml", use_ssl_for_kml);
  options.opt("source", source);
  options.opt("icon_directory", icon_directory);
  options.opt("dbroot_file", dbroot_file);
  options.opt("kml_map_file", kml_map_file);
  options.opt("data_type", data_type);
  options.opt("search_service", search_service);
  options.opt("kml_server", kml_server);
  options.opt("kml_port", kml_port);
  options.opt("kml_url_path", kml_url_path);
  options.setRequired("source", "icon_directory",
                      "dbroot_file", "kml_map_file");

  int argn;
  if (!options.processAll(argc, argv, argn)
      || help
      || argn != argc) {
    usage(progname);
  }

  // Make sure data_type is legal.
  data_type = UpperCaseString(data_type);
  if ((data_type != kCutAllDataFlag) &&
      (data_type != kCutImageryDataFlag) &&
      (data_type != kCutTerrainDataFlag) &&
      (data_type != kCutVectorDataFlag)) {
    usage(progname, "** ERROR ** Bad data_type flag.");
  }

  // Create temp directory for doing the work
  std::string temp_dir = khDirname(dbroot_file) + "/rewrite_dbroot";
  std::string dbroot_path = temp_dir + "/" + kBinaryDbrootPrefix;
  khEnsureParentDir(dbroot_path);

  // Fetch the live dbroot from the real server
  if (!ServerReader::SaveServerData(dbroot_path,
                                    source,
                                    "/dbRoot.v5?output=proto&hl=en&gl=us")) {
    notify(NFY_FATAL, "Unable to get dbroot from source.");
  }

  try {
    // load the dbroot we just fetched from the server
    geProtoDbroot dbroot(dbroot_path, geProtoDbroot::kEncodedFormat);

    // Remove layers of unused data types.
    RemoveUnusedLayers(data_type, &dbroot);

    // modify base_url fields in search tabs
    ReplaceSearchServer(search_service, &dbroot);

    // modify kml_url fields in layers
    std::string new_kml_base_url;
    if (kml_server.empty()) {
      new_kml_base_url = kml_url_path;
    } else {
      new_kml_base_url = ComposeUrl(use_ssl_for_kml, kml_server,
                                    kml_port, kml_url_path);
    }
    ReplaceReferencedKml(new_kml_base_url, kml_map_file, &dbroot);

    // fetch all icons from real server and save them locally
    SaveIcons(icon_directory, source, &dbroot);

    // write a local copy of our modified dbroot
    dbroot.Write(dbroot_file, geProtoDbroot::kEncodedFormat);
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}

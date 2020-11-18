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


#include <string>
#include <vector>

#include "fusion/autoingest/.idl/Locale.h"
#include "fusion/autoingest/.idl/storage/AssetDefs.h"
#include "fusion/autoingest/.idl/storage/MapLayerJSConfig.h"
#include "fusion/dbgen/.idl/DBConfig.h"
#include "fusion/dbmanifest/dbmanifest.h"
#include "common/khGetopt.h"
#include "common/khFileUtils.h"
#include "common/khConstants.h"
#include "common/khStringUtils.h"
#include "common/khException.h"
#include "common/geFilePool.h"

namespace {
void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\n"
          "usage: %s --config <config_file_path> --output <output_path>\n"
          "\n",
          progn.c_str());
  exit(1);
}


std::string LocalizedLayerDefs(const std::string& dir_prefix,
                               const std::string& locale) {
  std::string output_file = dir_prefix + "/" + kLayerDefsPrefix;
  if (!locale.empty())
    output_file += "." + locale;
  return output_file;
}

// Note: for output layer definitions (/mapdb) Fusion uses
// 'postamble'-suffix in the filename since the file
// (layers.js.[postamble].LOCALE) is then updated while publishing.
std::string LocalizedOutputLayerDefs(const std::string& dir_prefix,
                                     const std::string& locale) {
  std::string output_file = dir_prefix + "/" + kLayerDefsPostamblePrefix;
  if (!locale.empty())
    output_file += "." + locale;
  return output_file;
}



// Identify the vector and raster layer config files for the given
// maps database if any.
// config: the map db config.
// vector_config_file: on output contains the filename of the vector layer
//                     config file for the database. If no such file exists,
//                     return "".
// imagery_config_file: on output contains the filename of the imagery layer
//                      config file for the database. If no such file exists,
//                      return "".
void GetMapLayerConfigFiles(const MapDBGenConfig& config,
                            std::string* vector_config_file,
                            std::string* imagery_config_file) {
  *vector_config_file = "";
  *imagery_config_file = "";

  // Get the imagery layer defs if they exist.
  if (!config.imagery_layerdefs_path_.empty()) {
    *imagery_config_file = ReplaceSuffix(config.imagery_layerdefs_path_,
                                         "layerjs", "config.xml");
  }
  // Get the map vector layer defs if they exist.
  if (!config.map_layerdefs_path_.empty()) {
    *vector_config_file = ReplaceSuffix(config.map_layerdefs_path_,
                                        "layerjs", "config.xml");
  }

  // By default, return "" for files that don't exist.
  if (!(vector_config_file->empty()) && !khExists(*vector_config_file)) {
    *vector_config_file = "";  // Return empty filename if it doesn't exist.
  }
  if (!(imagery_config_file->empty()) && !khExists(*imagery_config_file)) {
    *imagery_config_file = "";  // Return empty filename if it doesn't exist.
  }
}

// Updates the raster layer config (mapdb/rasterlayerconfig.xml) setting
// projection property in case it is undefined. The projection property is
// undefined in the imagery projects built with versions of Fusion GEE-5.0.0-2
// or updated from GEE-4.x to 5.0.0-2.

// Note: Setting projection properties we then may properly set a requestType
// in maps.json. It allows us to create 2D Mercator Map using imagery projects
// built with Fusion versions GEE-5.0.0-2 or updated from GEE-4.x to 5.0.0-2.
//
// Function extracts imagery project path from imagery config path, then, based
// on imagery project suffix, determines the projection of imagery layer and
// sets the layer projection property in rasterlayerconfig.xml.
//
// The source_config_path is a path to an imagery layer config. It is used
// to extract imagery project path.
// The dest_config_path is a raster layer config path (
// mapdb/rasterlayerconfig.xml). We update projection property in the raster
// layer config and do not change source imagery layer config.
void UpdateRasterLayerConfig(const std::string& source_config_path,
                             const std::string& dest_config_path) {
  // Load raster layer config.
  MapLayerJSConfig config;
  if (!config.Load(dest_config_path)) {
    notify(NFY_FATAL,
           "Unable to load raster layer config file %s",
           dest_config_path.c_str());
  }

  if (!config.layers_.empty()) {
    if (config.layers_.size() != 1) {
      notify(NFY_FATAL,
             "More than one layer in raster layer config file %s",
             dest_config_path.c_str());
    }

    MapLayerJSConfig::Layer &layer = config.layers_[0];
    assert(layer.type_ == MapLayerJSConfig::Layer::Imagery);
    if (layer.projection_ == MapLayerJSConfig::Layer::UndefinedProjection) {
      // Get layer projection based on imagery project suffix.
      // Build the subasset name which is 'layerjs.kva'. The 'layerjs' is a base
      // name for the MapLayerJS asset.
      // Note: added '/' since otherwise we need to omit it below.
      // TODO: look for other way to get imagery project path!?
      std::string sub_asset_name = "/layerjs" +
          AssetDefs::FileExtension(AssetDefs::Vector, "MapLayerJS");
      std::string::size_type subasset_name_pos =
          source_config_path.find(sub_asset_name);
      if (std::string::npos == subasset_name_pos) {
        notify(NFY_FATAL,
               "Invalid config file path %s", source_config_path.c_str());
      }
      std::string imagery_project_path =
          source_config_path.substr(0, subasset_name_pos);
      bool is_mercator_imagery_project =
          khHasExtension(imagery_project_path,
                         kMercatorImageryProjectSuffix);
      layer.projection_ = is_mercator_imagery_project ?
          MapLayerJSConfig::Layer::MercatorProjection:
          MapLayerJSConfig::Layer::FlatProjection;

      // Rewrite raster layer config.
      config.Save(dest_config_path);
    }
  }
}
}  // namespace.


int main(int argc, char* argv[]) {
  try {
    std::string progname = argv[0];

    khGetopt options;
    std::string config_file_path, output_path;

    options.opt("config", config_file_path);
    options.opt("output", output_path);

    int argn;
    if (!options.processAll(argc, argv, argn))
      usage(progname);

    if (config_file_path.empty() || output_path.empty())
      usage(progname);

    MapDBGenConfig config;
    DbHeader header;

    // Load the GEDBGenConfig and copy over the index path, search tabs and
    if (!config.Load(config_file_path)) {
      // specific message already emitted
      notify(NFY_FATAL, "Unable to load config file %s",
                        config_file_path.c_str());
    }

    // Print the input file sizes for diagnostic log file info.
    std::vector<std::string> input_files;
    input_files.push_back(config_file_path);
    khPrintFileSizes("Input File Sizes", input_files);

    header.db_type_ = DbHeader::TYPE_MAPSDB;
    header.index_path_ = config.index_path_;
    header.database_version_ = config.database_version_;
    header.use_google_imagery_ = config.use_google_imagery_;
    header.is_mercator_ = config.is_mercator_;

    if (config.map_layerdefs_path_.empty() &&
        config.imagery_layerdefs_path_.empty()) {
      throw khException("Missing layer definitions.");
    }

    header.search_tabs_ = config.search_tabs_;
    header.poi_file_paths_ = config.poi_file_paths_;

    // There is an "icons" directory under every layerdefs directory.
    if (!config.imagery_layerdefs_path_.empty())
      header.icons_dirs_.push_back(config.imagery_layerdefs_path_ + "/icons");
    if (!config.map_layerdefs_path_.empty())
      header.icons_dirs_.push_back(config.map_layerdefs_path_ + "/icons");

    // Create the output directory.
    khMakeDir(output_path);

    // Read supported locales.
    LocaleSet locale_set;
    if (!locale_set.Load())
      throw khException(kh::tr("Unable to load locales."));

    for (int i = -1;
         i < static_cast<int>(locale_set.supportedLocales.size()); ++i) {
      std::string locale;
      std::string layerdefs_buf;

      if (i < 0)
        locale = kDefaultLocaleSuffix;
      else
        locale = locale_set.supportedLocales[i].toUtf8().constData();

      if (!config.imagery_layerdefs_path_.empty()) {
        khReadStringFromFile(LocalizedLayerDefs(config.imagery_layerdefs_path_,
                                                locale), layerdefs_buf);
      }
      if (!config.map_layerdefs_path_.empty()) {
        khReadStringFromFile(LocalizedLayerDefs(config.map_layerdefs_path_,
                                                locale), layerdefs_buf);
      }
      std::string output_layerdefs_path =
          LocalizedOutputLayerDefs(output_path + "/layers", locale);
      khWriteStringToFile(output_layerdefs_path, layerdefs_buf);
      header.toc_paths_.push_back(output_layerdefs_path);
    }

    header.fusion_host_ = AssetDefs::MasterHostName();
    header.Save(output_path + "/" + kHeaderXmlFile);


    // We also want to copy the current locale information (if it exists)
    // as it is needed during publish.
    // The destination locale file is used by the DbManifest class.
    typedef std::pair<std::string, std::string> FilePair;
    std::vector<FilePair> json_dependency_files;
    json_dependency_files.push_back(
      FilePair(LocaleSet::AssetRootLocalesFilename(),
               LocaleSet::LocalesFilename()));

    // These files also include layer config information for vector and imagery
    // layers.  These are only needed if they exist (i.e., we have imagery
    // and/or vector layers).
    // The destinations for the vector and imagery layer files are used
    // by the DbManifest class.
    std::string vector_config_file;
    std::string imagery_config_file;
    GetMapLayerConfigFiles(config, &vector_config_file, &imagery_config_file);
    json_dependency_files.push_back(
      FilePair(vector_config_file, kDbVectorLayerConfigFile));
    json_dependency_files.push_back(
      FilePair(imagery_config_file, kDbImageryLayerConfigFile));

    for (unsigned int i = 0; i < json_dependency_files.size(); ++i) {
      std::string source = json_dependency_files[i].first;
      std::string dest = khComposePath(output_path,
                                       json_dependency_files[i].second);
      // Copy the source only if it exists.
      if ((!source.empty()) && khExists(source)) {
        if (!khCopyFile(source, dest)) {
          notify(NFY_FATAL, "Unable to proceed: failed to create \"%s\".",
                 dest.c_str());
        }

        // Update a raster layer config (rasterlayerconfig.xml) setting the
        // projection property if it is undefined. The projection property is
        // undefined for GEE 5.0.0 - 5.0.2 imagery projects and GEE-4.x imagery
        // projects that have been updated to 5.0.0 - 5.0.2.
        if (json_dependency_files[i].second == kDbImageryLayerConfigFile) {
          UpdateRasterLayerConfig(source, dest);
        }
      }
    }

    // Create db manifest files list and write to .filelist in
    // mapdb.kda/verxxx/mapdb_aux directory.
    std::string db_name = output_path;
    // Note: returns asset specific database name.
    DbManifest db_manifest(&db_name);
    std::vector<std::string> file_list;
    geFilePool file_pool;
    db_manifest.GetDbFilesList(file_pool, &file_list);

    // Write db manifest files list to file.
    std::string aux_output_path = khComposePath(khDirname(output_path),
                                                kMapdbAuxBase);
    std::string fileslist_filepath = khComposePath(aux_output_path,
                                                   kDbManifestFilesListFile);
    khWriteStringVectorToFile(fileslist_filepath, file_list);

    // On successful completion, print out the output file sizes.
    std::vector<std::string> output_files;
    output_files.push_back(output_path);
    khPrintFileSizes("Output File Sizes", output_files);

    // Print auxiliary output files: database manifest files list.
    output_files.clear();
    output_files.push_back(aux_output_path);
    khPrintFileSizes("Aux. Output File Sizes", output_files);
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }

  return 0;
}

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


#include <set>
#include <string>
#include <vector>

#include "fusion/autoingest/.idl/Locale.h"
#include "fusion/autoingest/.idl/storage/AssetDefs.h"
#include "fusion/dbgen/.idl/DBConfig.h"
#include "fusion/dbmanifest/dbmanifest.h"
#include "fusion/dbroot/proto_dbroot_combiner.h"
#include "common/khGetopt.h"
#include "common/khFileUtils.h"
#include "common/khConstants.h"
#include "common/khStringUtils.h"
#include "common/geFilePool.h"
#include "common/gedbroot/proto_dbroot_util.h"


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


std::string LocalizedDbrootFilename(const std::string& dir_prefix,
                                    const std::string& locale) {
  assert(!locale.empty());
  return dir_prefix + "/" + kPostamblePrefix + "." + locale;
}

std::string FindLocalizedDbroot(const std::string& dir_prefix,
                                const std::string& locale) {
  assert(!locale.empty());
  std::string file_name = LocalizedDbrootFilename(dir_prefix, locale);
  if (!khExists(file_name)) {
    file_name = LocalizedDbrootFilename(dir_prefix, kDefaultLocaleSuffix);
    if (!khExists(file_name)) {
      throw khSimpleException("dbroot does not exist: ") << file_name;
    }
  }
  return file_name;
}

void FindNeededLocales(const std::string &dbroot_dir,
                       std::set<std::string> *needed) {
  if (dbroot_dir.empty()) return;

  std::vector<std::string> files;
  khGetBasenamesMatchingPattern(dbroot_dir,
                                kPostamblePrefix + "." /* prefix */,
                                "" /* suffix */,
                                &files);
  for (std::vector<std::string>::const_iterator file = files.begin();
       file != files.end(); ++file) {
    needed->insert(khExtension(*file, false /* include_dot */));
  }
}


// Identify the vector and raster layer config files for the given
// ge database if any.
// config: the ge db config.
// vector_config_file: on output contains the filename of the vector layer
//                     config file for the database. If no such file exists,
//                     return "".
// imagery_config_file: on output contains the filename of the imagery layer
//                      config file for the database. If no such file exists,
//                      return "".
// We need to load the layer information for the raster and vector layers.
void GetGeLayerConfigFiles(const GEDBGenConfig& config,
                           std::string* vector_config_file,
                           std::string* imagery_config_file) {
  std::string dbroot_file = config.vector_dbroot_path_;
  std::string raster_dbroot_file = config.imagery_dbroot_path_;

  // The DBRootGenConfig has all the vector layer information that we
  // need.
  // The config is in the same directory as the dbroot
  // e.g., /gevol/assets/.../dbroot.kva/ver###/dbroot
  // we need /gevol/assets/.../dbroot.kva/ver###/dbrootconfig.xml
  *vector_config_file = ReplaceSuffix(dbroot_file,
                                      "dbroot", "dbrootconfig.xml");

  // The RasterDBRootGenConfig file is found similarly to the DBRootGenConfig.
  // Minor difference is that the raster config file is just "config.xml".
  *imagery_config_file = ReplaceSuffix(raster_dbroot_file,
                                       "dbroot", "config.xml");

  // By default, return "" for files that don't exist.
  if (!(vector_config_file->empty()) && !khExists(*vector_config_file)) {
    *vector_config_file = "";  // Return empty filename if it doesn't exist.
  }
  if (!(imagery_config_file->empty()) && !khExists(*imagery_config_file)) {
    *imagery_config_file = "";  // Return empty filename if it doesn't exist.
  }
}

}  // namespace


int main(int argc, char* argv[]) {
  using gedbroot::CreateTimemachineDbroot;
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

    GEDBGenConfig config;
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

    header.db_type_ = DbHeader::TYPE_GEDB;
    header.index_path_ = config.index_path_;
    header.database_version_ = config.database_version_;

    header.search_tabs_ = config.search_tabs_;
    header.poi_file_paths_ = config.poi_file_paths_;

    // There is an "icons" directory under every dbroot directory.
    if (!config.vector_dbroot_path_.empty())
      header.icons_dirs_.push_back(config.vector_dbroot_path_ + "/icons");
    if (!config.imagery_dbroot_path_.empty())
      header.icons_dirs_.push_back(config.imagery_dbroot_path_ + "/icons");
    if (!config.terrain_dbroot_path_.empty())
      header.icons_dirs_.push_back(config.terrain_dbroot_path_ + "/icons");

    // Create the output directory.
    khMakeDir(output_path);

    // figure out which locales we need to generate
    std::set<std::string> needed_locales;
    FindNeededLocales(config.vector_dbroot_path_,  &needed_locales);
    FindNeededLocales(config.imagery_dbroot_path_, &needed_locales);
    FindNeededLocales(config.terrain_dbroot_path_, &needed_locales);

    // combine the low level dbroots for each locale
    for (std::set<std::string>::const_iterator locale = needed_locales.begin();
         locale != needed_locales.end(); ++locale) {
      ProtoDbrootCombiner combiner;

      // add global config that everybody needs
      combiner.ProtoDbroot()->
          AddUniversalFusionConfig(config.database_version_);

      // Order doesn't matter between these three since imagery and terrain
      // don't have their own layers. Maybe someday they will? Lets go ahead
      // with imagery, vector, terrain since that is currently the order
      // that the client displays them in.
      if (!config.imagery_dbroot_path_.empty()) {
        combiner.AddDbroot(
            FindLocalizedDbroot(config.imagery_dbroot_path_, *locale),
            geProtoDbroot::kProtoFormat);
        combiner.ProtoDbroot()->set_imagery_present(true);
      } else {
        combiner.ProtoDbroot()->set_imagery_present(false);
      }
      if (!config.vector_dbroot_path_.empty()) {
        combiner.AddDbroot(
            FindLocalizedDbroot(config.vector_dbroot_path_, *locale),
            geProtoDbroot::kProtoFormat);
      }
      if (!config.terrain_dbroot_path_.empty()) {
        combiner.AddDbroot(
            FindLocalizedDbroot(config.terrain_dbroot_path_, *locale),
            geProtoDbroot::kProtoFormat);
        combiner.ProtoDbroot()->set_terrain_present(true);
      } else {
        combiner.ProtoDbroot()->set_terrain_present(false);
      }

      std::string output_dbroot_path =
          LocalizedDbrootFilename(output_path + "/dbroots", *locale);
      combiner.Write(output_dbroot_path, geProtoDbroot::kProtoFormat);
      header.toc_paths_.push_back(output_dbroot_path);
    }

    // Create the timemachine dbroot.
    if (!config.imagery_dbroot_path_.empty() &&
        config.imagery_has_timemachine_) {
      // Create a new timemachine dbroot based on imagery postamble.DEFAULT.
      std::string template_name = LocalizedDbrootFilename(
          config.imagery_dbroot_path_, kDefaultLocaleSuffix);
      std::string output_dbroot_path = output_path + "/dbroots/" +
          kBinaryTimeMachineDbroot;
      CreateTimemachineDbroot(output_dbroot_path, config.database_version_,
                              template_name);

      // Add it to the list of dbroots.
      header.toc_paths_.push_back(output_dbroot_path);
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
    GetGeLayerConfigFiles(config, &vector_config_file, &imagery_config_file);
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
      }
    }

    // Create db manifest files list and write to .filelist in
    // mapdb.kda/verxxx/gedb_aux directory.
    std::string db_name = output_path;
    // Note: returns asset specific database name.
    DbManifest db_manifest(&db_name);
    std::vector<std::string> file_list;
    geFilePool file_pool;
    db_manifest.GetDbFilesList(file_pool, &file_list);

    // Write db manifest files list to file.
    std::string aux_output_path = khComposePath(khDirname(output_path),
                                                kGedbAuxBase);
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

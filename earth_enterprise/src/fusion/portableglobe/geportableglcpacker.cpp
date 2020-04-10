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


// Packs up globe files into a standalone ".glc" file
// or an addendum ".gla" file, which can be appended
// to an existing glc file to add content to it.

#include <notify.h>
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <map>
#include <sstream>  // NOLINT(readability/streams)
#include <string>
#include <vector>
#include "common/khAbortedException.h"
#include "common/khFileUtils.h"
#include "common/khGetopt.h"
#include "common/khSimpleException.h"
//#include "common/khTypes.h"
#include <cstdint>
#include "fusion/portableglobe/shared/file_packer.h"
#include "fusion/portableglobe/shared/file_unpacker.h"

/**
 * Read in paths from given layers info file. For an addendum file,
 * it should only specify files to be added as new or replacements
 * to existing files in the original glc. The file name is typically
 * "earth/addendum_layer_info.txt". The file "earth/layer_info.txt"
 * might be one of the files being updated, for example if a
 * new layer is being added.
 */

void ReadAssetPaths(std::vector<std::string>* asset_paths,
                    const std::string& file_path) {
  std::string name;
  std::string path;
  std::string type;
  int index;
  int parent;

  std::ifstream fin(file_path.c_str());
  while (!fin.eof()) {
    // TODO: Support spaces in names?
    fin >> name;
    if (!fin.eof()) {
      if (!name.empty()) {
        fin >> path;
        if (path.empty()) {
          notify(NFY_WARN, "Ignoring bad layer format. Name: %s Path: %s",
                 name.c_str(), path.c_str());
        } else {
          // These are not yet used here, but may be in the future.
          // They are used for serving the glc on the Portable Server.
          fin >> type;
          fin >> index;
          fin >> parent;

          if (!khExists(path)) {
            notify(NFY_FATAL, "Unable to find file: %s", path.c_str());
          }
          asset_paths->push_back(path);
        }
      }
    }
  }
}


/**
 * Packs a glc based on the information in the layer_info_file. The layer_info
 * file is usually the layers_info.txt file that Portable Server uses to
 * identify the glbs in the glc.
 */
void BuildGlc(const std::string& layer_info_file,
              const std::string& output,
              bool make_copy) {
  if (!khExists(layer_info_file)) {
    notify(NFY_FATAL, "Unable to find file: %s", layer_info_file.c_str());
  }

  if (khExists(output)) {
    notify(NFY_FATAL, "%s already exists.", output.c_str());
  }

  std::vector<std::string> asset_paths;

  std::string build_file;
  {
    ReadAssetPaths(&asset_paths, layer_info_file);
    if (asset_paths.size() > 0) {
      if (make_copy) {
        build_file = output;
      } else {
        build_file = asset_paths[0];
      }

      // prefix_len is not used here. We are saving the whole path.
      // Change if we use spec directory as base directory.
      size_t prefix_len = -1;
      fusion_portableglobe::FilePacker packer(
          build_file, asset_paths[0], prefix_len);
      for (size_t i = 1; i < asset_paths.size(); ++i) {
        packer.AddFile(asset_paths[i], prefix_len);
      }
    } else {
      notify(NFY_WARN, "No files specified.");
    }
  }

  if (!make_copy) {
    khRename(build_file, output);
  }
}

/**
 * If an addendum num is being added, then make sure the number is
 * one more than the one in the original glc. If there isn't one in the
 * original glc, then the addendum num should be 1.
 */

void CheckAddendumNum(const std::vector<std::string>& asset_paths,
                      const std::string& glc_file) {
  const char* addendum_num_path = "earth/addendum_num.txt";
  std::vector<std::string>::const_iterator it;
  // If an addendum num is given, check it against what is in the
  // glc.
  for (it = asset_paths.begin(); it != asset_paths.end(); ++it) {
    if (*it == addendum_num_path) {
      int addendum_num;
      std::ifstream fin(addendum_num_path);
      fin >> addendum_num;

      fusion_portableglobe::FileUnpacker unpacker(glc_file.c_str());
      std::string file_data;
      if (unpacker.FindFile(addendum_num_path)) {
        std::ifstream fp(glc_file.c_str(), std::ios::in | std::ios::binary);
        file_data.resize(unpacker.Size());
        fp.seekg(unpacker.Offset());
        fp.read(&file_data[0], unpacker.Size());
        fp.close();
        int previous_addendum_num = atoi(file_data.substr(5).c_str());
        if (addendum_num == previous_addendum_num + 1) {
          notify(NFY_INFO, "Appears to be addendum %d to glc.", addendum_num);
        } else {
          notify(NFY_FATAL, "Expected addend num %d but found %d.",
                 previous_addendum_num + 1, addendum_num);
        }
        return;
      } else {
        if (addendum_num == 1) {
          notify(NFY_INFO, "Appears to be first addendum to glc.");
          return;
        }
        notify(NFY_FATAL, "Checking is minimal if not addendum num specified.");
      }
    }
  }

  notify(NFY_WARN, "Checking is minimal if addendum num specified.");
}

/**
 * Packs a glc addendum based on the information in the layer_info_file for
 * the original glc and for the addendum. The layer_info file is usually
 * the layers_info.txt file that Portable Server uses to
 * identify the glbs in the glc.
 */
void BuildAddendum(const std::string& layer_info_file,
                   const std::string& glc_file,
                   const std::string& output) {
  if (!khExists(layer_info_file)) {
    notify(NFY_FATAL, "Unable to find layer info file: %s",
           layer_info_file.c_str());
  }

  if (glc_file.empty() || !khExists(glc_file)) {
    notify(NFY_FATAL, "Unable to find glc file: %s", glc_file.c_str());
  }

  if (khExists(output)) {
    notify(NFY_FATAL, "%s already exists.", output.c_str());
  }

  std::vector<std::string> asset_paths;
  ReadAssetPaths(&asset_paths, layer_info_file);
  CheckAddendumNum(asset_paths, glc_file);

  // Pack the files from addendum_file_info.txt.
  fusion_portableglobe::FilePacker packer(output, glc_file);
  for (size_t i = 0; i < asset_paths.size(); ++i) {
    packer.AddFile(asset_paths[i], -1);
  }
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
          "usage: %s \\\n"
          "           --layer_info=<glc_layer_info_file> \\\n"
          "           --output=<destination_file>\n"
          "\n"
          "\nOptions:\n"
          "   --make_copy:     Make a copy of all files so that the\n"
          "                    first glb is not destroyed.\n"
          "                    Default is false.\n"
          "   --addendum:      Whether to create an addendum to an existing\n"
          "                    glc file.\n"
          "                    Default is false.\n"
          "   --glc_file:      If creating an addendum, this is the\n"
          "                    glc file that the addendum will be added to.\n"
          "                    Required if addendum is true.\n"
          "\n"
          "E.g.\n"
          "  geportableglcpacker --layer_info earth/layer_info --output my_globe.glc\n"
          "\n"
          " Packs up glb files into a single glc file. If make_copy is set\n"
          " to True, the glbs are undisturbed. If not, then the first\n"
          " glb is destroyed as it becomes the base of the glc, but\n"
          " the command can run considerably faster.\n"
          "\n"
          " If an addendum is being created, then the target glc is\n"
          " specified, and the layer_info file should specify the files\n"
          " to be added. The addendum can be concatenated with the glc\n"
          " file to create a new glc with extended content.\n"
          "\n"
          " The output naming conventions are that it should end in '.glc'\n"
          " if a standalone glc is being created and in '.gla' if an\n"
          " addendum is being created to add to an existing glc. When\n"
          " a glc and an addendum are concatenated, the resulting file\n"
          " should also have a name ending in '.glc'.\n"
          "\n"
          " The layer info file is a text file in the following format:\n"
          "   name1 path_to_glb1 type1 index1 parent1\n"
          "    ...\n"
          "   nameN path_to_glbN typeN indexN parentN\n",
          progn.c_str());
  exit(1);
}

int main(int argc, char **argv) {
  try {
    std::string progname = argv[0];

    // Process commandline options
    int argn;
    bool help = false;
    bool make_copy = false;
    bool addendum = false;
    std::string layer_info_file;
    std::string glc_file;
    std::string output;

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.flagOpt("addendum", addendum);
    options.flagOpt("make_copy", make_copy);
    options.opt("layer_info", layer_info_file);
    options.opt("output", output);
    options.opt("glc_file", glc_file);
    options.setRequired("output", "layer_info");

    if (!options.processAll(argc, argv, argn)
        || help
        || argn != argc) {
      usage(progname);
      return 1;
    }

    if (addendum) {
      BuildAddendum(layer_info_file, glc_file, output);
    } else {
      BuildGlc(layer_info_file, output, make_copy);
    }
  } catch(const khAbortedException &e) {
    notify(NFY_FATAL, "Unable to proceed: See previous warnings");
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }
}

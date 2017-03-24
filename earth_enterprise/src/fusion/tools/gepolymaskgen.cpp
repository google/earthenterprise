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
// Generate poly mask command line tool. Lets you build a mask by combining
// existing polygon and tiff masks. For polygon files, if we are OR-ing, the
// polygon is assumed to be made up of 0xff values, if we are AND-ing, the
// polygon is assumed to be made up of 0x00 values. In general, AND-ing is
// done with the negative image of the given mask (shape or tiff).
//
// Threshold is used to convert pixels that are neither 0x00 or 0xff
// to either 0x00 (if pixel value is at or below threshold) or 0xff (if pixel
// value is above threshold). This may be used to prevent AND-ing or OR-ing of
// two feathered regions, which is not recommended because you can get spurious
// results. It can also be used with feathering to expand or retract the
// extent of a polygon.
//
// Command forms:
//   -- get help --
//   gepolymaskgen [--help | --? | ?]

//   -- build mask using base image (start with empty mask) --
//   gepolymaskgen [--feather <feather_int>]
//                 -base_image </path/input_image.tif>
//                 [
//                   [--feather <feather_int>]
//                   [--invert]
//                   [--threshold threshold_byte]
//                   [--or_mask </path/file.[tif|shp|kml]>]
//                   [--and_neg_mask </path/file.[tif|shp|kml]>]
//                 ]*  any combination zero or more times
//                 --output_mask </path/output_mask.tif>

//   -- build mask using base mask --
//   gepolymaskgen [--feather <feather_int>]
//                 -base_mask </path/alpha_mask.tif>
//                 [
//                   [--feather <feather_int>]
//                   [--invert]
//                   [--threshold threshold_byte]
//                   [--or_mask </path/file.[tif|shp|kml]>]
//                   [--and_neg_mask </path/file.[tif|shp|kml]>]
//                 ]*  any combination zero or more times
//                 --output_mask </path/output_mask.tif>
//  Examples:
//     -- simple polygon mask with no feathering
//     gepolymaskgen --base_image /path/input_image.tif
//                   --or_mask /path/polygon.kml
//                   --output_mask /path/result_mask.tif
//
//     -- subtract polygon mask from base with no feathering
//     gepolymaskgen --base_mask /path/base_mask.tif
//                   --and_neg_mask /path/polygon.kml
//                   --output_mask /path/result_mask.tif
//
//     -- add and subtract polygon and tiff masks with different feathers
//     gepolymaskgen --feather 30
//                   --base_mask /path/base_mask.tif
//                   --feather 20
//                   --or_mask /path/SF.kml
//                   --or_mask /path/daly_city.kml
//                   --feather 15
//                   --or_mask /path/circle_mask.tif
//                   --feather 5
//                   --and_neg_mask /path/gg_park.kml
//                   --and_neg_mask /path/northbeach.kml
//                   --feather 40
//                   --and_neg_mask /path/circle_mask2.tif
//                   --output_mask /path/mask.tif

#include <fstream>
#include <getopt.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include "common/khFileUtils.h"
#include "common/notify.h"
#include "fusion/tools/polymask.h"

#define KHPOLYMASKGENVERSION "0.5"

typedef std::map<std::string, std::string> OptionsMap;

// Commands given as command line arguments
enum Command {
  NO_MORE_COMMANDS = -1,
  UNKNOWN_COMMAND = 0,
  BASE_MASK = 1,
  BASE_IMAGE,
  FEATHER,
  FEATHER_BORDER,
  INVERT,
  AND_MASK,
  OR_MASK,
  THRESHOLD,
  DIFF_MASK,
  OUTPUT_MASK,
  HELP,
};

// States for processing the command line arguments
enum State {
  START,      // Get the base image or mask.
  NEXT_MASK,  // Process adding and subtracting of masks.
  DONE        // Build and save the new mask.
  };

// Convert string to an integer. Return whether the argument was a legal
// integer value.
bool GetIntArg(std::string str, int* arg) {
  *arg = atoi(str.c_str());

  // If we got 0 but it wasn't the string for zero
  // then the int was not properly formed.
  if ((*arg == 0) && (str != "0")) {
    return false;
  }

  return true;
}


// Check that the given file is readable. Allow any type
// and later see if gdal info can derive the needed info from it.
// Namely, it must have the geo extent and the pixel dimensions.
void CheckInputFile(std::string file_name) {
  if (!file_name.empty() && !khIsFileReadable(file_name)) {
    notify(NFY_FATAL, "Cannot read file: %s",
           file_name.c_str());
  }
}


// Check that the given file is a readable kml, shp, or tiff file.
void CheckMaskFile(std::string file_name) {
  if (!file_name.empty() && !khIsFileReadable(file_name)) {
    notify(NFY_FATAL, "Cannot read file: %s",
           file_name.c_str());
  }

  std::string suffix = file_name.substr(file_name.size() - 4);
  if ((suffix != ".tif") && (suffix != ".kml") && (suffix != ".shp")) {
    notify(NFY_FATAL, "Bad file format for %s. Expecting .tif, .kml, or .shp.",
           file_name.c_str());
  }
}


// Shows legal arguments for the program.
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
          "usage: %s --help | -?\n"
          "       %s [--feather <int_feather>] --base_mask <geotiff_mask_file>"
          " [options] --output_mask <geotiff_mask_file>\n"
          "       %s [--feather <int_feather>] --base_image"
          " <geotiff_image_file> [options] --output_mask <geotiff_mask_file>\n"
          "\n Options are applied in order given.\n"
          " Valid options:\n"
          "       --feather <int_feather>:     Feather to apply to all \n"
          "                                    subsequent masks until a\n"
          "                                    different feather is given.\n"
          "                                    Feather can be 0, positive,\n"
          "                                    or negative. Default is 0.\n"
          "       --feather_border <int_flag>: If flag is non-zero, border\n"
          "                                    is feathered. Otherwise,\n"
          "                                    border is left in tact.\n"
          "                                    Flag remains in effect\n"
          "                                    until it is modified.\n"
          "                                    Default is border is\n"
          "                                    feathered.\n"
          "       --and_neg_mask <mask_file>:  Bitwise AND of negative image"
          "                                    of polygon or raster mask\n"
          "                                    with the current mask.\n"
          "                                    Polygons can be given in\n"
          "                                    .kml or .shp files and are\n"
          "                                    assumed to be filled with\n"
          "                                    0x00. Raster masks should be\n"
          "                                    .tif files with the same pixel "
          "                                    dimensions as the base mask.\n"
          "                                    Care should be taken not to"
          "                                    overlap feathered regions.\n"
          "       --or_mask <mask_file>:       Bitwise OR polygon or raster\n"
          "                                    mask to the current mask.\n"
          "                                    Polygons can be given in\n"
          "                                    .kml or .shp files and are\n"
          "                                    assumed to be filled with\n"
          "                                    0xff. Raster masks should be\n"
          "                                    .tif files with the same pixel\n"
          "                                    dimensions as the base mask.\n"
          "                                    Care should be taken not to"
          "                                    overlap feathered regions.\n"
          "       --threshold <thresh_byte>:   All pixels at or below the\n"
          "                                    threshold byte are set to\n"
          "                                    0x00; all other pixels are set\n"
          "                                    to 0xff.\n"
          "\n"
          " Please note that ordering of arguments is important; they"
          " are applied in the order that they are given. The feather"
          " argument implies that you are setting the feather to be"
          " used for any subsequent masks. E.g. to feather the base mask,"
          " you must set the feather before the base mask is given."
          " Positive feathers erode the mask (white area retracts); negative"
          " feathers expand the mask (white area grows).\n"
          "\n"
          " Examples:\n"
          " -- simple polygon mask with no feathering\n"
          "   gepolymaskgen --base_image /path/input_image.tif \\\n"
          "                 --or_mask /path/polygon.kml \\\n"
          "                 --output_mask /path/result_mask.tif\n"
          "\n"
          " -- negative polygon mask with some feathering\n"
          "   gepolymaskgen --base_image /path/input_image.tif \\\n"
          "                 --feather 30 \\\n"
          "                 --feather_border 0 \\\n"
          "                 --and_neg_mask /path/polygon.kml \\\n"
          "                 --invert \\\n"
          "                 --output_mask /path/result_mask.tif\n"
          "\n"
          " -- OR and AND polygon and tiff masks with "
          "different feathers \\\n"
          "   gepolymaskgen --feather 30 \\\n"
          "                 --base_mask /path/base_mask.tif \\\n"
          "                 --feather 20 \\\n"
          "                 --feather_border 0 \\\n"
          "                 --or_mask /path/SF.kml \\\n"
          "                 --or_mask /path/daly_city.kml \\\n"
          "                 --feather 15 \\\n"
          "                 --feather_border 1 \\\n"
          "                 --or_mask /path/circle_mask.tif \\\n"
          "                 --feather 5 \\\n"
          "                 --and_neg_mask /path/gg_park.kml \\\n"
          "                 --and_neg_mask /path/northbeach.kml \\\n"
          "                 --feather 40 \\\n"
          "                 --and_neg_mask /path/circle_mask2.tif \\\n"
          "                 --output_mask /path/mask.tif\n",
          progn.c_str(), progn.c_str(), progn.c_str());
  exit(1);
}


// Process initialization commmands to create a base mask.
State ProcessStartCommands(fusion_tools::PolyMask* poly_mask,
                           const Command command,
                           const std::string file_name,
                           const int feather,
                           const std::string progname) {
  State state = START;
  switch (command) {
    // Set feather for initial mask.
    case FEATHER:
      notify(NFY_NOTICE, "Feather %d", feather);
      poly_mask->AddCommandSetFeather(feather);
      break;

    // Set feather border flag.
    case FEATHER_BORDER:
      notify(NFY_NOTICE, "Feather border %d", feather);
      poly_mask->AddCommandSetFeatherBorder(feather);
      break;


    // Base image provides geo-extent and resolution of the mask.
    case BASE_IMAGE:
      CheckInputFile(file_name);
      notify(NFY_NOTICE, "Base image: %s", file_name.c_str());
      poly_mask->AddCommandSetBaseImage(file_name);
      state = NEXT_MASK;
      break;

    // Base mask provides geo-extent and resolution of the mask
    // as well as the initial mask itself.
    case BASE_MASK:
      CheckInputFile(file_name);
      notify(NFY_NOTICE, "Base mask: %s", file_name.c_str());
      poly_mask->AddCommandSetBaseMask(file_name);
      state = NEXT_MASK;
      break;

    case HELP:
      usage(progname);
      break;

    default:
       notify(NFY_NOTICE, "Must specify an input image or a base mask");
       usage(progname);
       break;
  }

  return state;
}


// Process "next mask" commmands to add or subtract from the base mask.
State ProcessNextMaskCommands(fusion_tools::PolyMask* poly_mask,
                              const Command command,
                              const std::string file_name,
                              const int int_arg,
                              const std::string progname) {
  State state = NEXT_MASK;
  switch (command) {
    // Set feather for next set of masks.
    case FEATHER:
      notify(NFY_NOTICE, "Feather %d", int_arg);
      poly_mask->AddCommandSetFeather(int_arg);
      break;

    // Set feather border flag.
    case FEATHER_BORDER:
      notify(NFY_NOTICE, "Feather border %d", int_arg);
      poly_mask->AddCommandSetFeatherBorder(int_arg);
      break;

    // Invert the mask created up to this point.
    case INVERT:
      notify(NFY_NOTICE, "Invert");
      poly_mask->AddCommandInvert();
      break;

    // Threshold the mask created up to this point.
    case THRESHOLD:
      notify(NFY_NOTICE, "Threshold %d", int_arg);
      poly_mask->AddCommandThreshold(int_arg);
      break;

    // AND mask polygons or image with current mask.
    case AND_MASK:
      CheckMaskFile(file_name);
      notify(NFY_NOTICE, "AND mask: %s", file_name.c_str());
      poly_mask->AddCommandAndMask(file_name);
      break;

    // OR mask polygons or image with current mask.
    case OR_MASK:
      CheckMaskFile(file_name);
      notify(NFY_NOTICE, "OR mask: %s", file_name.c_str());
      poly_mask->AddCommandOrMask(file_name);
      break;

     // Show differences between two masks. (For testing only)
    case DIFF_MASK:
      CheckMaskFile(file_name);
      notify(NFY_NOTICE, "Diff mask: %s", file_name.c_str());
      poly_mask->AddCommandDiffMask(file_name);
      break;

    // Set output file for the final mask.
    case OUTPUT_MASK:
      notify(NFY_NOTICE, "Output mask: %s", file_name.c_str());
      poly_mask->AddCommandSetOutputMask(file_name);
      state = DONE;
      break;

    default:
      notify(NFY_NOTICE, "Illegal argument: %s", optarg);
      usage(progname);
  }

  return state;
}


int main(int argc, char *argv[]) {
  // const int MAX_NUM_PASSES = 6;

  try {
    std::string progname = argv[0];
    std::vector<std::vector<std::string> > polygon_files;
    std::string file_name("");
    int int_arg = 0;

    // Command line arguments
    static struct option longopts[] = {
        { "help",           no_argument,            NULL,   HELP },
        { "?",              no_argument,            NULL,   HELP },
        { "invert",         no_argument,            NULL,   INVERT },
        { "feather_border", required_argument,      NULL,   FEATHER_BORDER },
        { "base_mask",      required_argument,      NULL,   BASE_MASK },
        { "base_image",     required_argument,      NULL,   BASE_IMAGE },
        { "feather",        required_argument,      NULL,   FEATHER },
        { "and_neg_mask",   required_argument,      NULL,   AND_MASK },
        { "or_mask",        required_argument,      NULL,   OR_MASK },
        { "threshold",      required_argument,      NULL,   THRESHOLD },
        { "diff_mask",      required_argument,      NULL,   DIFF_MASK },
        { "output_mask",    required_argument,      NULL,   OUTPUT_MASK },
        { NULL,             0,                      NULL,   }
    };

    State state = START;
    fusion_tools::PolyMask poly_mask;

    // Process each embedded command from the command line.
    Command next_command;
    while ((next_command =
            (Command) getopt_long_only(argc, argv, "", longopts, NULL))
           != NO_MORE_COMMANDS) {

      // If there was ambiguity, then help character is returned.
      if ((char) next_command == '?') {
        next_command = HELP;
      }

      // Get arguments for the commands, if we are expecting any.
      if ((next_command != INVERT) && (next_command != HELP)) {
        if (next_command == FEATHER) {
          if (!GetIntArg(optarg, &int_arg)) {
            notify(NFY_NOTICE, "Expected an integer argument for feather.");
            usage(progname);
          }
        } else if (next_command == FEATHER_BORDER) {
          if (!GetIntArg(optarg, &int_arg)) {
            notify(NFY_NOTICE,
                   "Expected an integer argument for feather_border.");
            usage(progname);
          }
        } else if (next_command == THRESHOLD) {
          if (!GetIntArg(optarg, &int_arg)) {
            notify(NFY_NOTICE,
                   "Expected an integer argument for threshold.");
            usage(progname);
          }
        } else {
          file_name = optarg;
        }
      }

      switch (state) {
        // Initialize the base mask.
        case START:
          state = ProcessStartCommands(&poly_mask,
                                       next_command,
                                       file_name,
                                       int_arg,
                                       progname);
          break;

        // Keep adding or subtracting masks as they are given.
        case NEXT_MASK:
          state = ProcessNextMaskCommands(&poly_mask,
                                          next_command,
                                          file_name,
                                          int_arg,
                                          progname);
          break;

        // We should have already quit.
        case DONE:
          notify(NFY_NOTICE, "Output mask should be last argument.");
          usage(progname);
          break;
      }
    }

    if (state != DONE) {
      if (state == START) {
        notify(NFY_NOTICE, "You must specify an input_image OR a base_mask");
      } else {
        notify(NFY_NOTICE, "You must specify an output_mask");
      }
      usage(progname);
    }

    // Build the mask.
    poly_mask.BuildMask();
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}




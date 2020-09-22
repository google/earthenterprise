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


#include <unistd.h>
#include <algorithm>
#include <vector>
#include "fusion/config/gefConfigUtil.h"
#include "fusion/gemaptilegen/.idl/Config.h"
#include "fusion/gemaptilegen/Generator.h"
#include "fusion/gst/gstMisc.h"
#include "common/khGetopt.h"
#include "common/notify.h"
#include "common/geFilePool.h"
#include "common/khAbortedException.h"
#include "common/khFileUtils.h"


unsigned int kDefaultNumCPUs = 1;

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(
      stderr,
      "\nusage: %s [options] --config <configfile> --output <out.kmpacket>\n"
      "   Map tiles are generated and written to out.kmpacket\n"
      "   If the output kmpacket already exists, its contents are over written.\n"
      "   Supported options are:\n"
      "      --help | -?:  Display this usage message\n"
      "      --pnglevel    PNG Compression level (0=none through 9=max)\n"
      "      --numcpus     Number of CPUs to use (default %u)\n",
      progn.c_str(), CommandlineNumCPUsDefault());
  exit(1);
}


int main(int argc, char **argv) {
  try {
    std::string progname = argv[0];

    // Process commandline options
    int argn;
    bool help = false;
    std::string outdir;
    std::string configfile;
    int pnglevel = -1;
    std::uint32_t numcpus = CommandlineNumCPUsDefault();
    //    unsigned int num_writer_threads = 2;

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("output", outdir);
    options.opt("config", configfile, &khGetopt::FileExists);
    options.opt("pnglevel", pnglevel);
    options.opt("numcpus", numcpus);

    if (!options.processAll(argc, argv, argn))
      usage(progname);
    if (help)
      usage(progname);
    if (argn != argc)
      usage(progname);

    numcpus = std::min(numcpus, CommandlineNumCPUsDefault());

    // Validate commandline options
    if (!outdir.size()) {
      usage(progname, "No --output specified");
    }
    if (!configfile.size()) {
      usage(progname, "No --config specified");
    }

    // -1 means use the ZLIB default
    if (pnglevel < -1 || pnglevel > 9) {
      usage(progname, "PNG Level must be 0 through 9");
    }

    if (numcpus < 1) {
      usage(progname, "Number of CPUs should not be less than 1");
    }
    // Print the input file sizes for diagnostic log file info.
    std::vector<std::string> input_files;
    input_files.push_back(configfile);
    khPrintFileSizes("Input File Sizes", input_files);

    gemaptilegen::Config config;
    if (!config.Load(configfile)) {
      usage(progname, "Unable to load %s", configfile.c_str());
    }

    geFilePool file_pool;

    gstInit();

    // default values are fine for now. Maybe someday we'll expose the
    // ability to override some of them (num render threads, etc.)
    const khTilespace *fusion_tilespace = 0;
    switch (config.projection_) {
      case AssetDefs::FlatProjection:
        fusion_tilespace = &FusionMapTilespace;
        break;
      case AssetDefs::MercatorProjection:
        fusion_tilespace = &FusionMapMercatorTilespace;
        break;
    }

    gemaptilegen::TaskConfig task_config(*fusion_tilespace, pnglevel, numcpus);

    gemaptilegen::Generator generator(file_pool, outdir, config, task_config,
        config.projection_ == AssetDefs::MercatorProjection);
    generator.Run();

    // On successful completion, print out the output file sizes.
    std::vector<std::string> output_files;
    output_files.push_back(outdir);
    khPrintFileSizes("Output File Sizes", output_files);
  } catch (const khAbortedException &e) {
    notify(NFY_FATAL, "Unable to proceed: See previous warnings");
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }

  return 0;
}

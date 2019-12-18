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


#include <stdio.h>
#include <vector>

#include "common/notify.h"
#include "common/khFileUtils.h"
#include "common/khProgressMeter.h"
#include "fusion/gst/gstSelector.h"
#include "fusion/gst/gstSource.h"


char* cmd_name;

void usage() {
  fprintf(stderr, "usage: %s --output <outfile> --product <product> "
          "--config <config file>\n\n", cmd_name);
  exit(EXIT_FAILURE);
}

#define ARG_IS(a, b) (strcmp(a, b) == 0)

int main(int argc, char** argv) {
  try {
    cmd_name = strrchr(argv[0], '/');
    cmd_name = cmd_name == NULL ? argv[0] : cmd_name + 1;

    char* output = NULL;
    char* product = NULL;
    char* config = NULL;

    if (argc == 1)
      usage();

    int argn = 1;
    for (; argn < argc; argn++) {
      char* arg = argv[argn];

      if (ARG_IS(arg, "--output") || ARG_IS(arg, "-o")) {
        argn++;
        if (argn >= argc)
          usage();
        size_t outputlen = strlen(argv[argn]);
        output = reinterpret_cast<char*>(alloca(outputlen + 1));
        strncpy(output, argv[argn], outputlen + 1);
        // strip trailing slash if exists
        if (output[strlen(output) - 1] == '/')
          output[strlen(output) - 1] = '\0';
      } else if (ARG_IS(arg, "--product") || ARG_IS(arg, "-p")) {
        argn++;
        if (argn >= argc)
          usage();
        product = strdup(argv[argn]);
      } else if (ARG_IS(arg, "--config") || ARG_IS(arg, "-c")) {
        argn++;
        if ( argn >= argc )
          usage();
        config = strdup(argv[argn]);
      }
    }

    if (output == NULL || product == NULL || config == NULL) {
      fprintf(stderr, "Missing required args!\n");
      usage();
    }

    //
    // here we will fabricate a vector project and stuff in all the
    // pieces by hand.  the project is necessary to get a source manager
    // which is needed by the selector when he wants to grab the features
    // out of the source
    //
    // the selector is the guy who owns the query since he must iterate
    // through all the source features and compare them to the filters
    // one at a time until a match is found
    //

    gstInit();

    if (khEnsureParentDir(output) != true)
      notify(NFY_FATAL, "Unable to create output path %s", output);

    QueryConfig querycfg;
    if (!querycfg.Load(config))
      notify(NFY_FATAL, "Unable to read query config file %s", config);

    gstSource src(product);
    if (src.Open() != GST_OKAY)
      notify(NFY_FATAL, "Unable to open source product %s", product);

    // Print the input file sizes for diagnostic log file info.
    std::vector<std::string> input_files;
    input_files.push_back(config);
    input_files.push_back(product);
    khPrintFileSizes("Input File Sizes", input_files);

    gstSelector selector(&src, querycfg);

    // In 3.1.1, we changed this implementation to create the selection files
    // in a single pass. The prior implementation relied on loading all of the
    // features first, then computing and writing the selection list.
    // This would often overflow memory.
    {
      khProgressMeter progress(selector.NumSrcFeatures(), "Features");
      std::string output_name_prefix(output);
      selector.CreateSelectionListFilesBatch(output_name_prefix, &progress);
    }

    // On successful completion, print out the output file sizes.
    std::vector<std::string> output_files;
    khGetNumericFilenames(output, output_files);
    khPrintFileSizes("Output File Sizes", output_files);
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "Query error: %s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Query error: %s", "Unknown error");
  }

  return EXIT_SUCCESS;
}

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
#include <notify.h>
#include <vector>
#include <gstVectorProject.h>
#include <gstLayer.h>
#include <gstSource.h>
#include <khFileUtils.h>
#include <gstProgress.h>
#include <khstl.h>
#include <geFilePool.h>
#include <autoingest/.idl/storage/VectorFuseAssetConfig.h>

char* command_name;

void usage() {
  fprintf(stderr, "usage: %s --output <outfile> --product <product> "
          "--config <config file>\n",
          command_name);
  fprintf(stderr, "\n");
  exit(EXIT_FAILURE);
}

#define ARG_IS(a, b) (strcmp(a, b) == 0)

int main(int argc, char** argv) {
  command_name = strrchr(argv[0], '/');
  command_name = command_name == NULL ? argv[0] : command_name + 1;

  std::string output;
  std::string product;
  std::string config;

  if (argc == 1)
    usage();

  for (int argn = 1; argn < argc; ++argn) {
    char* arg = argv[argn];

    if (ARG_IS(arg, "--output") || ARG_IS(arg, "-o")) {
      ++argn;
      if (argn >= argc)
        usage();
      output = argv[argn];
      // strip trailing slash if exists
      if (EndsWith(output, "/"))
        output.resize(output.size() - 1);
    } else if (ARG_IS(arg, "--product") || ARG_IS(arg, "-p")) {
      ++argn;
      if (argn >= argc)
        usage();
      product = argv[argn];
    } else if (ARG_IS(arg, "--config") || ARG_IS(arg, "-c")) {
      argn++;
      if (argn >= argc)
        usage();
      config = argv[argn];
    }
  }

  if (output.empty()) {
    fprintf(stderr, "Missing --output\n");
    usage();
  }

  if (product.empty()) {
    fprintf(stderr, "Missing --product\n");
    usage();
  }

  if (config.empty()) {
    fprintf(stderr, "Missing --config\n");
    usage();
  }
  // Print the input file sizes for diagnostic log file info.
  std::vector<std::string> input_files;
  input_files.push_back(config);
  input_files.push_back(product);
  khPrintFileSizes("Input File Sizes", input_files);

  gstInit();

  FuseConfig fusecfg;
  if (!fusecfg.Load(config.c_str()))
    notify(NFY_FATAL, "Unable to read fuse config file %s", config.c_str());

  gstSource src(product.c_str());
  if (src.Open() != GST_OKAY)
    notify(NFY_FATAL, "Unable to open source product %s", product.c_str());

  gstVectorProject project("vectorfuse");

  gstLayer layer(&project, &src, fusecfg);

  geFilePool file_pool;
  if (!layer.ExportStreamPackets(file_pool, output.c_str()))
    notify(NFY_FATAL, "Failed to create fused packet files");


  // On successful completion, print out the output file sizes.
  std::vector<std::string> output_files;
  output_files.push_back(output);
  khPrintFileSizes("Output File Sizes", output_files);
}

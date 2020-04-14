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
#include <notify.h>
#include <vector>

#include "common/geFilePool.h"
#include "common/khAbortedException.h"
#include "common/khstl.h"
#include "common/khFileUtils.h"

#include "fusion/geindexgen/Todo.h"
#include "fusion/geindexgen/Generator.h"

// must be last header so ToString(geindex::TypedEntry::TypeEnum) will
// pick the right function
#include "common/khGetopt.h"

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s [options] --type=<type> --stack <stackfile>"
          " --output <out.geindex>\n"
          "   Packets from the files specified in <stackfile> are indexed\n"
          "   into the output geindex. If the output geindex already exists,\n"
          "   its contents are over written.\n"
          "   Supported values for <type> are:\n"
          "      Imagery\n"
          "      Terrain\n"
          "      VectorGE\n"
          "   Supported options are:\n"
          "      --help | -?:  Display this usage message\n"
          "      --delta <prev.geindex>: Perform a delta index\n"
          "      --batchsize <num>: Processing batch size (default %u)\n",
          progn.c_str(), geindexgen::kDefaultMergeSessionSize);
  exit(1);
}


int main(int argc, char **argv) {
  try {
    std::string progname = argv[0];

    // Process commandline options
    int argn;
    bool help = false;
    std::string outdir;
    std::string stackfilename;
    std::string delta;
    unsigned int batch_size = geindexgen::kDefaultMergeSessionSize;
    unsigned int queue_size = 500;
    unsigned int num_writer_threads = 2;

    // QTPacket is invalid sentinal
    geindex::TypedEntry::TypeEnum type =
      geindex::TypedEntry::QTPacket;

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("output", outdir);
    options.opt("stack", stackfilename, &khGetopt::FileExists);
    options.opt("delta", delta, &khGetopt::DirExists);
    options.opt("batchsize", batch_size);
    options.choiceOpt("type", type,
                      makevec3(geindex::TypedEntry::Imagery,
                               geindex::TypedEntry::Terrain,
                               geindex::TypedEntry::VectorGE));

    if (!options.processAll(argc, argv, argn))
      usage(progname);
    if (help)
      usage(progname);
    if (argn != argc)
      usage(progname);

    // Validate commandline options
    if (type == geindex::TypedEntry::QTPacket) {
      usage(progname, "--type must be specified");
    }
    if (!outdir.size()) {
      usage(progname, "No output specified");
    }
    if (!stackfilename.size()) {
      usage(progname, "No --stack specified");
    }
    if (delta.size()) {
      notify(NFY_FATAL, "--delta not supported yet.");
    }
    if (batch_size == 0) {
      usage(progname, "--batchsize must be greater than 0.");
    }


    geFilePool file_pool;
    // Print the input file sizes for diagnostic log file info.
    std::vector<std::string> input_files;
    input_files.push_back(stackfilename);

    if ((type == geindex::TypedEntry::Imagery) ||
        (type == geindex::TypedEntry::Terrain)) {
      // handle raster indexing
      geindexgen::BlendStack stack;
      if (!stack.Load(stackfilename)) {
        usage(progname, "Unable to load %s", stackfilename.c_str());
      }
      if (stack.insets_.empty()) {
        notify(NFY_FATAL, "No raster packets for indexing");
      }

      // pull out the sizes of the packetfiles used for indexing from the
      // stack of insets.
      std::vector<geindexgen::BlendStack::Inset>::const_iterator inset_iter =
        stack.insets_.begin();
      for (; inset_iter != stack.insets_.end(); ++inset_iter) {
        std::vector<geindexgen::BlendStack::Level>::const_iterator level_iter =
          inset_iter->levels_.begin();
        for (; level_iter != inset_iter->levels_.end(); ++level_iter) {
          input_files.push_back(level_iter->packetfile_);
        }
      }
      khPrintFileSizes("Input File Sizes", input_files);

      geindexgen::BlendGenerator gen(file_pool, outdir,
                                     batch_size, queue_size, stack,
                                     type, num_writer_threads);
      gen.DoIndexing();
    } else {
      // handle vector indexing
      geindexgen::VectorStack stack;
      if (!stack.Load(stackfilename)) {
        usage(progname, "Unable to load %s", stackfilename.c_str());
      }
      if (stack.layers_.empty()) {
        notify(NFY_FATAL, "No vector packets for indexing");
      }

      // pull out the sizes of the packetfiles used for indexing from the
      // stack of insets.
      std::vector<geindexgen::VectorStack::Layer>::const_iterator layer_iter =
        stack.layers_.begin();
      for (; layer_iter != stack.layers_.end(); ++layer_iter) {
        input_files.push_back(layer_iter->packetfile_);
      }
      khPrintFileSizes("Input File Sizes", input_files);


      geindexgen::VectorGenerator gen(file_pool, outdir,
                                      batch_size, queue_size, stack,
                                      type, num_writer_threads);
      gen.DoIndexing();
    }

    // On successful completion, print out the output file sizes.
    std::vector<std::string> output_files;
    output_files.push_back(outdir);
    khPrintFileSizes("Output File Sizes", output_files);
  } catch(const khAbortedException &e) {
    notify(NFY_FATAL, "Unable to proceed: See previous warnings");
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }

  return 0;
}

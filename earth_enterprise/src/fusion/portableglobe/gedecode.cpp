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


// Tool for checking 3d imagery packets.
// This tool should always be built locally and
// never installed.

#include <khGetopt.h>
#include <notify.h>
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <string>
#include "common/khFileUtils.h"
#include "common/etencoder.h"

// Converts an encoded file to an unencoded file (or vice versa).
// Encoding is used for Google Earth packets.
void Decode(std::string input_file,
            std::string output_file) {
  std::ifstream fp_in(input_file.c_str(), std::ios::in | std::ios::binary);
  std::ofstream fp_out(output_file.c_str(), std::ios::out | std::ios::binary);

  std::uint64_t buffer_size = khGetFileSizeOrThrow(input_file);
  std::string buffer;
  buffer.resize(buffer_size);

  // Get data to be encoded.
  fp_in.read(&buffer[0], buffer_size);

  etEncoder::EncodeWithDefaultKey(&buffer[0], buffer_size);

  fp_out.write(&buffer[0], buffer_size);

  fp_in.close();
  fp_out.close();
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
          "E.g. gedecode \\\n"
          "        --input test.jpg \\\n"
          "        --output test.enc\n"
          "\nusage: %s\n"
          "   --input <input_file_path>: The file to be decoded.\n"
          "   --output <output_file_path>: Where decoded data should go.\n"
          "\n"
          " Tool for decoding a file.\n"
          " The encoding is symmetric so it can also be used for encoding\n"
          " a file.\n",
          progn.c_str());
  exit(1);
}

int main(int argc, char **argv) {
  try {
    std::string progname = argv[0];

    // Process commandline options
    int argn;
    bool help = false;
    std::string input_file;
    std::string output_file;

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("input", input_file);
    options.opt("output", output_file);
    options.setRequired("input", "output");

    if (!options.processAll(argc, argv, argn)
        || help
        || argn != argc) {
      usage(progname);
      return 1;
    }

    Decode(input_file, output_file);
    std::cout << "Decoded file: " << output_file << std::endl;
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }
}

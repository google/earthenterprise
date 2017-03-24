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


#include <map>
#include <stdio.h>
#include <khGetopt.h>
#include <notify.h>
#include <khFileUtils.h>
#include <packetfile/packetfilereader.h>
#include <packetfile/packetfilereaderpool.h>

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s [options] <bundlefile>\n"
          "   Supported options are:\n"
          "      --help | -?:             Display this usage message\n"
          "      --path <quadtree path>   Only dump file with this path\n"
          "      --count                  Terminate after this many entries\n"
          "      --verbose                Display everything\n"
          "      --dump                   Save objects to disk\n"
          "      --histogram              Create a histogram of all entries\n"
          "      --histogram10            Same as --histogram, but display\n"
          "                                  entries that have counts >= 10\n"
          "",
          progn.c_str());
  exit(1);
}

int main(int argc, char** argv) {
  try {
    std::string progname = argv[0];

    // ********** process commandline options **********
    int argn;
    khGetopt options;
    bool help = false;
    std::string restrict_path;
    int restrict_count = -1;
    bool verbose = false;
    bool dump_packets = false;
    bool dump_histogram = false;
    bool dump_histogram_10 = false;

    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("path", restrict_path);
    options.opt("count", restrict_count);
    options.flagOpt("verbose", verbose);
    options.flagOpt("dump", dump_packets);
    options.flagOpt("histogram", dump_histogram);
    options.flagOpt("histogram10", dump_histogram_10);

    if (!options.processAll(argc, argv, argn) || help)
      usage(progname);

    const char* bundle = GetNextArg();
    if (!bundle)
      usage(argv[0], "<bundlefile> not specified");

    geFilePool file_pool;
    PacketFileReader reader(file_pool, bundle);
    std::string buffer;
    QuadtreePath read_path;
    size_t read_size;
    size_t min_size = 0;
    size_t max_size = 0;
    std::map<size_t, int> histogram;

    int read_counter = 0;
    while (true) {
      read_size = reader.ReadNextCRC(&read_path, buffer);
      if (read_size == 0)
        break;

      if (read_counter == 0) {
        min_size = max_size = read_size;
      } else {
        if (read_size < min_size)
          min_size = read_size;
        if (read_size > max_size)
          max_size = read_size;
      }

      histogram[read_size]++;

      ++read_counter;
      std::string path_string = read_path.AsString();

      if (!restrict_path.empty()) {
        if (path_string != restrict_path)
          continue;
      }

      if (verbose)
        printf("path=%s size=%lld\n", path_string.c_str(),
               (long long int)read_size);

      if (dump_packets)
        khWriteSimpleFile(path_string + ".png", buffer.c_str(), read_size);

      if (!restrict_path.empty())
        break;  // we must be done if path was specified

      if (restrict_count == read_counter)
        break;
    }

    printf("min packet size: %d\n", (int)min_size);
    printf("max packet size: %d\n", (int)max_size);
    printf("total packets: %d\n", read_counter);

    for (std::map<size_t, int>::iterator it = histogram.begin();
         it != histogram.end(); ++it) {
      if (dump_histogram)
        printf("[%llu] %d\n", (long long unsigned)it->first, it->second);
      if (dump_histogram_10 && it->second >= 10)
        printf("[%llu] %d\n", (long long unsigned)it->first, it->second);
    }

  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }

  return 0;
}


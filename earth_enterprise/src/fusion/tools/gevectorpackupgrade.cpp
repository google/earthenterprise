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


// gevectorpackupgrade kbf_directory packetfile_directory
//
// Tool to convert old style vector kbf pack file set to new
// PacketFile format.
//
// The kbf file set consists of (potentially) multiple files, pack.00,
// pack.01, pack.02, ...  Each record has a header (see kbf.h)
// containing the quadtree location of the packet.
//
// The new PacketFile format has a small sidecar index which stores
// packet locations.  The actual data packets have a CRC32
// appended. The kbf header is not present.
//
// In the conversion process, the original pack.nn files are read, the
// packets are compressed, and then written to a new packetfile with
// sidecar index.

#include <string>
#include <iostream>
#include <string.h>

#include <kbf.h>
#include <khGetopt.h>
#include <khFileUtils.h>
#include <khSimpleException.h>
#include <geFilePool.h>
#include <quadtreepath.h>
#include <packetcompress.h>
#include <packetfile/packetfilewriter.h>
#include <packetfile/packetfilereader.h>

static void ReadbackOutput(geFilePool &file_pool,
                           const std::string path,
                           std::uint64_t record_count) {
  PacketFileReader reader(file_pool, path);

  QuadtreePath last_qt_path;
  QuadtreePath qt_path;
  std::string buffer;
  size_t read_size;
  std::uint64_t read_count = 0;

  while (0 != (read_size = reader.ReadNextCRC(&qt_path, buffer))) {
    if (qt_path < last_qt_path) {
      notify(NFY_FATAL, "ReadbackOutput: records out of order at %s",
             qt_path.AsString().c_str());
    }
    last_qt_path = qt_path;
    ++read_count;
  }

  if (read_count != record_count) {
    notify(NFY_FATAL, "ReadbackOutput: expected %llu records, found %llu",
           static_cast<unsigned long long>(record_count),
           static_cast<unsigned long long>(read_count));
  }

  notify(NFY_DEBUG, "ReadbackOutput: read %llu records",
           static_cast<unsigned long long>(read_count));
}

int main(int argc, char *argv[]) {
  int argn;
  bool debug = false;
  bool verbose = false;
  bool readback = false;
  bool help = false;
  khGetopt options;
  options.flagOpt("debug", debug);      // set notify level to DEBUG
  options.flagOpt("verbose", verbose);  // set notify level to VERBOSE
  options.flagOpt("readback", readback);  // read and check output file
  options.flagOpt("help", help);
  options.flagOpt("?", help);

  if (!options.processAll(argc, argv, argn)  ||  help
      ||  argc < 3  ||  argn != argc - 2) {
    std::cerr << "usage: " << std::string(argv[0])
              << " [-debug] [-verbose] [-readback] [-help] input_dir output_dir"
              << std::endl
              << "  Converts old Fusion kbf format vector file set to"
              << std::endl
              << "  new Fusion packet file."
              << std::endl;
    exit(1);
  }

  if (verbose) {
    setNotifyLevel(NFY_VERBOSE);  
  } else if (debug) {
    setNotifyLevel(NFY_DEBUG);
  }

  geFilePool file_pool;

  // Open the vector pack file set for conversion.
  std::string oldpackdir = argv[argc-2];
  FileBundlePackImport kbf_reader(file_pool, oldpackdir);
  std::uint64_t kbf_size = kbf_reader.data_size();
  std::uint64_t kbf_pos = 0;
  notify(NFY_INFO, "gevectorpackupgrade: reading %llu bytes from %s",
         static_cast<unsigned long long>(kbf_size),
         kbf_reader.abs_path().c_str());

  // Create the output packet file
  std::string newpackdir = argv[argc-1];
  PacketFileWriter writer(file_pool, argv[argc-1]);
  notify(NFY_INFO, "gevectorpackupgrade: writing to %s",
         writer.abs_path().c_str());

  // Read, compress, and write each record in the kbf file set
  std::uint64_t record_count = 0;
  std::vector<char> compressed_buffer;

  while (kbf_pos < kbf_size) {
    // Read record header
    kbfFileHeader kbf_header;
    kbf_reader.ReadAt(kbf_reader.LinearToBundlePosition(kbf_pos),
                      &kbf_header,
                      sizeof(kbf_header));
    if (__BYTE_ORDER == __BIG_ENDIAN) {
      kbf_header.byteSwap();
    }
    if (kbf_header.magicId[0] != MAGIC_1
        ||  kbf_header.magicId[1] != MAGIC_2
        ||  kbf_header.magicId[2] != MAGIC_3) {
      notify(NFY_FATAL,
             "gevectorpackupgrade: invalid kbf header at offset %llu",
             static_cast<unsigned long long>(kbf_pos));
      exit(2);
    }
    kbf_pos += sizeof(kbf_header);

    // Extract path and data size
    QuadtreePath qt_path(kbf_header.level, kbf_header.branchlist);
    size_t src_data_size = kbf_header.size;

    if (verbose) {
      notify(NFY_VERBOSE,
             "gevectorpackupgrade: converting record %llu at %s, %lu bytes",
             static_cast<unsigned long long>(record_count),
             qt_path.AsString().c_str(),
             static_cast<unsigned long>(src_data_size));
    }

    // Read data from vector file
    char src_buffer[src_data_size];
    kbf_reader.ReadAt(kbf_reader.LinearToBundlePosition(kbf_pos),
                      src_buffer,
                      src_data_size);
    kbf_pos += src_data_size;

    // Compress vector packet
    size_t est_compressed_size = KhPktGetCompressBufferSize(src_data_size);
    if (est_compressed_size + FileBundle::kCRCsize > compressed_buffer.size()) {
      compressed_buffer.reserve(est_compressed_size + FileBundle::kCRCsize);
    }
    size_t compressed_size = est_compressed_size;
    if (KhPktCompressWithBuffer(src_buffer,
                                src_data_size,
                                &compressed_buffer[0],
                                &compressed_size)) {
      // Write data to output packet file
      writer.WriteAppendCRC(qt_path,
                            &compressed_buffer[0],
                            compressed_size + FileBundle::kCRCsize);
    } else {
      notify(NFY_FATAL,
             "gevectorpackupgrade: KhPktCompressWithBuffer failed"
             " at record %llu",
             static_cast<unsigned long long>(record_count));
    }
    ++record_count;
  }

  kbf_reader.CloseNoUpdate();
  writer.Close();


  // copy the lodtable too
  if (khExists(khComposePath(oldpackdir, "lodtable"))) {
    if (!khLinkOrCopyFile(khComposePath(oldpackdir, "lodtable"),
                          khComposePath(newpackdir, "lodtable"))) {
      notify(NFY_FATAL, "Unable to proceed");
    }
  }


  if (readback) {
    ReadbackOutput(file_pool, argv[argc-1], record_count);
  }

  notify(NFY_INFO, "gevectorpackupgrade: converted %llu records",
         static_cast<unsigned long long>(record_count));

  return 0;
}

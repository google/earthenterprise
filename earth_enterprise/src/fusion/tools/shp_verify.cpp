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


// File: shp_verify.cpp
// Purpose: verify contents of ESRI Shape file
//          currently only looks at shp & shx files

#include <stdio.h>
#include <khGetopt.h>
#include <notify.h>
#include <khFileUtils.h>
#include <khEndian.h>

bool verbose = false;
std::string progname;

void usage(const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s [options] <shp file>\n"
          "   Supported options are:\n"
          "      --help | -?:             Display this usage message\n"
          "      --verbose                Show more info\n"
          "",
          progname.c_str());
  exit(1);
}

std::string kInvalidType("Invalid");
std::string ShapeType(std::uint32_t type) {
  switch (type) {
    case  0: return "Null Shape";
    case  1: return "Point";
    case  3: return "PolyLine";
    case  5: return "Polygon";
    case  8: return "MultiPoint";
    case 11: return "PointZ";
    case 13: return "PolyLineZ";
    case 15: return "PolygonZ";
    case 18: return "MultiPointZ";
    case 21: return "PointM";
    case 23: return "PolyLineM";
    case 25: return "PolygonM";
    case 28: return "MultiPointM";
    case 31: return "MultiPatch";
    default: return kInvalidType;
  }
}

const int kBoundsStringCount = 8;
struct MainFileHeader {
  std::uint32_t file_code;
  std::uint32_t file_length;
  std::uint32_t version;
  std::uint32_t shape_type;
  double bounds[kBoundsStringCount];
};

const char* BoundsStrings[] = {
  "Xmin", "Ymin", "Xmax", "Ymax",
  "Zmin", "Zmax", "Mmin", "Mmax"
};


MainFileHeader VerifyMainFileHeader(char* buf, const std::string& path,
                                    const std::string& type) {
  // first 28 bytes are big endian
  BigEndianReadBuffer be_buf(buf, 28);

  MainFileHeader header;

  // verify file code is 9994
  be_buf >> header.file_code;
  if (verbose)
    printf("%s File Code: %u\n", type.c_str(), header.file_code);
  if (header.file_code != 9994)
    printf("ERROR! Expected File Code to be 9994, but found %u\n",
           header.file_code);

  // verify unused bytes are zero
  std::uint32_t unused;
  for (int i = 0; i < 5; ++i) {
    be_buf >> unused;
    if (unused != 0)
      printf("ERROR! Expected Unused byte # %d to be zero, but found %u\n",
             i, unused);
  }

  // verify file length matches actual
  be_buf >> header.file_length;
  header.file_length <<= 1;  // length is in 16-bit words, convert to bytes
  if (verbose)
    printf("%s File Length: %u\n", type.c_str(), header.file_length);
  std::uint64_t actual_size;
  time_t mtime;
  if (!khGetFileInfo(path, actual_size, mtime))
    usage("Unable to stat file: %s", path.c_str());
  if (header.file_length != actual_size)
    printf("ERROR! Header says file length is %u, but the actual size is %llu\n",
           header.file_length, (long long unsigned)actual_size);

  // remaining 100 bytes are little endian
  LittleEndianReadBuffer le_buf(&buf[28], 72);
  le_buf >> header.version;
  if (verbose)
    printf("%s Version: %u\n", type.c_str(), header.version);
  if (header.version != 1000)
    printf("ERROR! Expected Version to be 1000, but found %u\n", header.version);

  le_buf >> header.shape_type;
  if (verbose)
    printf("%s Shape Type: %u (%s)\n", type.c_str(), header.shape_type,
           ShapeType(header.shape_type).c_str());
  if (ShapeType(header.shape_type) == kInvalidType)
    printf("ERROR! Invalid Shape Type %u\n", header.shape_type);

  for (int b = 0; b < kBoundsStringCount; ++b) {
    le_buf >> header.bounds[b];
    if (verbose)
      printf("%s Bounding Box %s: %lf\n", type.c_str(), BoundsStrings[b],
             header.bounds[b]);
  }

  return header;
}


int main(int argc, char** argv) {
  progname = argv[0];

  // ********** process commandline options **********
  int argn;
  khGetopt options;
  bool help = false;

  options.flagOpt("help", help);
  options.flagOpt("?", help);
  options.flagOpt("verbose", verbose);

  if (!options.processAll(argc, argv, argn) || help)
    usage();

  const char* next_arg = GetNextArg();
  if (!next_arg)
    usage("<shp file> not specified");

  // ensure extension of primary 'shp' file is valid
  std::string shp_path(next_arg);
  bool lower_case_ext = true;
  if (khHasExtension(shp_path, ".SHP")) {
    lower_case_ext = false;
  } else if (!khHasExtension(shp_path, ".shp")) {
    usage("Invalid SHP file specified");
  }

  std::string shx_path = khReplaceExtension(shp_path,
                                            lower_case_ext ? ".shx" : ".SHX");

  if (!khExists(shp_path) || !khExists(shx_path))
    usage("Can't find specified SHP or SHX file");

  if (verbose) {
    printf("Opening SHP: %s\n", shp_path.c_str());
    printf("Opening SHX: %s\n", shx_path.c_str());
  }

  int shp_fd = khOpenForRead(shp_path);
  int shx_fd = khOpenForRead(shx_path);
  if (shp_fd == -1 || shx_fd == -1)
    usage("Unable to open SHP or SHX for reading");

  char shp_buf[100];
  char shx_buf[100];
  if (!khReadAll(shp_fd, &shp_buf[0], 100) || !khReadAll(shx_fd, &shx_buf[0], 100))
    usage("Unable to read 100 byte file header of SHP or SHX");

  MainFileHeader shp_hdr = VerifyMainFileHeader(shp_buf, shp_path, "SHP");
  MainFileHeader shx_hdr = VerifyMainFileHeader(shx_buf, shx_path, "SHX");

  // validate shp & shx headers match
  if (shp_hdr.shape_type != shx_hdr.shape_type)
    printf("ERROR! SHP type %u does not match SHX type %u\n",
           shp_hdr.shape_type, shx_hdr.shape_type);

  for (int b = 0; b < kBoundsStringCount; ++b) {
    if (shp_hdr.bounds[b] != shx_hdr.bounds[b])
      printf("ERROR! Bounding Box %s mismatch.  SHP = %lf, SHX = %lf\n",
             BoundsStrings[b], shp_hdr.bounds[b], shx_hdr.bounds[b]);
  }

  // scan all shape records, skipping geometry
  char rec_hdr[8];
  std::string rec_buf;
  char shx_rec[8];
  std::uint32_t expected_rec_id = 1;
  std::uint32_t shp_rec_pos = 100;
  while (khReadAll(shp_fd, &rec_hdr, 8)) {
    BigEndianReadBuffer be_buf(rec_hdr, 8);
    std::uint32_t rec_num;
    be_buf >> rec_num;
    if (rec_num != expected_rec_id) {
      printf("ERROR! Expecting record # %d, but got # %d\n",
             expected_rec_id, rec_num);
    }

    std::uint32_t content_length;
    be_buf >> content_length;
    content_length <<= 1;  // measured in 16-bit words, convert to bytes

    rec_buf.resize(0);
    rec_buf.resize(content_length);
    if (!khReadAll(shp_fd, &rec_buf[0], content_length)) {
      printf("ERROR! Unable to read complete record of length %u\n", content_length);
      exit(-1);
    }

    std::uint32_t type = *((const std::uint32_t*)rec_buf.data());

    if (verbose) {
      printf("Record Number: %d, Content Length: %u, Type: %u (%s)\n",
             rec_num, content_length, type, ShapeType(type).c_str());
    }

    if (type != shp_hdr.shape_type) {
      printf("ERROR! Main File Header Shape Type is %d (%s), but record # %d is %d (%s)\n",
             shp_hdr.shape_type, ShapeType(shp_hdr.shape_type).c_str(),
             rec_num, type, ShapeType(type).c_str());
    }


    // read the associated record from the index file (shx) and compare
    if (!khReadAll(shx_fd, &shx_rec[0], 8)) {
      printf("ERROR! Unable to read record # %d from index file", expected_rec_id);
    }

    BigEndianReadBuffer shx_be_buf(shx_rec, 8);
    std::uint32_t shx_offset;
    shx_be_buf >> shx_offset;
    shx_offset <<= 1;
    if (shx_offset != shp_rec_pos) {
      printf("ERROR! Expected SHX Offset %u, but got %u\n",
             shp_rec_pos, shx_offset);
    }

    std::uint32_t shx_content_length;
    shx_be_buf >> shx_content_length;
    shx_content_length <<= 1;
    if (shx_content_length != content_length) {
      printf("ERROR! Exected SHX Content Length %d, but got %d\n",
             content_length, shx_content_length);
    }

    ++expected_rec_id;
    shp_rec_pos += (8 + content_length);
  }
  khClose(shp_fd);
  khClose(shx_fd);
}

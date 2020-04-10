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


#include <string.h>
#include <fstream>
#include <notify.h>
#include <khGetopt.h>
#include <khgdal/khgdal.h>

#include <khgdal/.idl/khVirtualRaster.h>
#include "khLUT.h"

void
usage(const std::string &progn, const char *msg = 0, ...)
{
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s [options] --lut <lutfile> <.khvr> \n"
          "   Supported options are:\n"
          "      --help | -?:      Display this usage message\n"
          "      --validate:       Validate the inputs and exit\n",
          progn.c_str());
  exit(1);
}


int
main(int argc, char *argv[])
{
  std::string progname = argv[0];

  // process commandline options
  int argn;
  bool help = false;
  bool validate = false;
  std::string output;
  std::string lutfilename;
  khGetopt options;
  options.flagOpt("help", help);
  options.flagOpt("?", help);
  options.flagOpt("validate", validate);
  options.opt("lut", lutfilename, &khGetopt::FileExists);
  if (!options.processAll(argc, argv, argn))
    usage(progname);
  if (help)
    usage(progname);

  output = GetNextArgAsString();
  // validate, post-process options
  if (output.empty())
    usage(progname, "No .khvr specified");
  if (lutfilename.empty()) {
    usage(progname, "No --lut specified");
  }

  try {
    // load existing virtualraster
    khVirtualRaster virtraster;
    if (!virtraster.Load(output)) {
      notify(NFY_FATAL, "Unable to load %s",
             output.c_str());
    }

    // process / validate LUT
    std::ifstream in(lutfilename.c_str());
    if (!in) {
      notify(NFY_FATAL, "Unable to open lut file %s",
             lutfilename.c_str());
    }
    unsigned int numbands;
    in >> numbands;
    if (!in) {
      notify(NFY_FATAL, "Unable to interpret lut file %s",
             lutfilename.c_str());
    }
    if (numbands != virtraster.outputBands.size()) {
      notify(NFY_FATAL, "Incompatible LUT has %u bands. Imagery has %u.",
             numbands, static_cast<unsigned>(virtraster.outputBands.size()));
    }
    for (unsigned int b = 0; b < numbands; ++b) {
      khVirtualRaster::OutputBand &band = virtraster.outputBands[b];
      GDALDataType intype  = GDTFromName(band.inDatatype);
      // for now the presence of a LUT implies an output type of
      // GDT_Byte
      GDALDataType outtype = GDT_Byte;
      band.outDatatype = GDALGetDataTypeName(outtype);
      switch (intype) {
        case GDT_Byte:
          ParseAndStoreLUT<unsigned char>(in, outtype, band.defaultLut);
          break;
        case GDT_UInt16:
          ParseAndStoreLUT<std::uint16_t>(in, outtype, band.defaultLut);
          break;
        case GDT_Int16:
          ParseAndStoreLUT<std::int16_t>(in, outtype, band.defaultLut);
          break;
        default:
          notify(NFY_FATAL,
                 "Band %d has type %s which cannot be used as input to a LUT",
                 b+1, band.inDatatype.c_str());
      }
    }

    if (!validate) {
      // save it out to XML
      if (!virtraster.Save(output)) {
        notify(NFY_FATAL, "Unable to write %s", output.c_str());
      }
    }

  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Caught unknown exception");
  }


  return 0;
}

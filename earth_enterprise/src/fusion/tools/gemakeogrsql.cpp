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

/******************************************************************************
File:        gemakeogrsql.cpp
Description:

-------------------------------------------------------------------------------
For history see CVS log (cvs log makeogrsql.cpp -or- Emacs Ctrl-xvl).
******************************************************************************/


#include <ogr_spatialref.h>
#include <cpl_conv.h>
#include <cpl_string.h>
#include <ogrsf_frmts.h>

#include <notify.h>
#include <khgdal/khgdal.h>
#include <khGetopt.h>
#include <khGuard.h>
#include <khFileUtils.h>
#include <khstl.h>
#include <cctype>
#include <algorithm>

#include <gst/.idl/gstDBSource.h>

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
          "\nusage: %s [options] --output <output.ogrsql> --ogrsrc <OGRSourceStr>\n"
          "   Supported options are:\n"
          "      --help | -?:          Display this usage message\n"
          "      --srs <override srs>: Use given SRS\n"
          "      --sql <sql string>:   Run query, instead of default layers\n"
          "      --ocitable <tablename>:  OCI Table to use for fetching SRS\n",
          progn.c_str());
  exit(1);
}


int main(int argc, char *argv[]) {
  std::string progname = argv[0];

  // process commandline options
  int argn;
  bool help = false;
  std::string output;
  std::string overridesrs;
  std::string sqlstr;
  std::string ogrsrcstr;
  std::string ocitablestr;
  khGetopt options;
  options.flagOpt("help", help);
  options.flagOpt("?", help);
  options.opt("output", output);
  options.opt("srs", overridesrs);
  options.opt("sql", sqlstr);
  options.opt("ogrsrc", ogrsrcstr);
  options.opt("ocitable", ocitablestr);
  if (!options.processAll(argc, argv, argn))
    usage(progname);
  if (help)
    usage(progname);


  // validate, post-process options
  if (output.empty())
    usage(progname, "No output specified");
  if (ogrsrcstr.empty())
    usage(progname, "No ogrsrc specified");
  if (!khHasExtension(output, ".ogrsql"))
    usage(progname, "Output file must end in .ogrsql");

  khGDALInit();
  OGRRegisterAll();


  try {
    // ***** validate the ogr connection string *****
    khDeleteGuard<GDALDataset> dataSource;
    dataSource =
      TransferOwnership((GDALDataset *) GDALOpenEx(ogrsrcstr.c_str(),
                                                   GDAL_OF_READONLY,
                                                   NULL, NULL, NULL));
    if (!dataSource) {
      notify(NFY_FATAL, "Unable to create SQL reader for (%s)",
             ogrsrcstr.c_str());
    }

    // ***** check/fetch SRS *****
    if (!overridesrs.empty()) {
      // validate supplied SRS
      // will throw if unable to interpret the SRS
      OGRSpatialReference srs;
      InterpretSRSString(overridesrs, srs);
    } else if (!ocitablestr.empty()) {
      std::transform(ocitablestr.begin(), ocitablestr.end(),
                     ocitablestr.begin(), ::toupper);

      // we don't have an override srs, but we do have an OCI table name
      if (!StartsWith(ogrsrcstr, "OCI:")) {
        usage(progname, "--ocitable only valid with 'OCI:' ogrsrc");
      }

      // split the ogrstr into the pieces we need
      std::vector<std::string> parts;
      split(ogrsrcstr, ":", std::back_inserter(parts));
      if (parts.size() >= 2) {
        // build a new ogrsrc with our table name
        std::string newogrsrc = parts[0] + ":" + parts[1] + ":" + ocitablestr;
        // open the data source
        khDeleteGuard<GDALDataset> dataSource;
        dataSource =
          TransferOwnership((GDALDataset *) GDALOpenEx(ogrsrcstr.c_str(),
                                                       GDAL_OF_READONLY,
                                                       NULL, NULL, NULL));

        if (!dataSource) {
          notify(NFY_FATAL, "Unable to create SQL reader for (%s) to get SRS from table",
                 newogrsrc.c_str());
        }
        // fetch our only layer
        if (dataSource->GetLayerCount() < 1) {
          notify(NFY_FATAL, "No layers in SQL reader (%s)",
                 newogrsrc.c_str());
        }
        OGRLayer *poLayer = dataSource->GetLayer(0);
        if (!poLayer) {
          notify(NFY_FATAL, "Unable to access layer 0 in SQL reader (%s)",
                 newogrsrc.c_str());
        }
        // get its SRS
        OGRSpatialReference* srs = poLayer->GetSpatialRef();
        if (!srs) {
          notify(NFY_FATAL, "Unable to get SRS from table '%s'",
                 ocitablestr.c_str());
        }

        // make it into a WKT string
        overridesrs = GetWKT(*srs);
      } else {
        usage(progname, "Malformed ogrsrc for ");
      }
    }

    // ***** check sql *****
    if (!sqlstr.empty()) {
      OGRLayer *poLayer =
        dataSource->ExecuteSQL(sqlstr.c_str(),
                               0, /* spatialFilter */
                               0  /* sqlDialect */);
      if (!poLayer) {
        notify(NFY_FATAL, "Unable to execute SQL (%s)", sqlstr.c_str());
      }
      dataSource->ReleaseResultSet(poLayer);
    }

    // save it out to XML
    gstDBSource dbSource;
    dbSource.OGRDataSource = ogrsrcstr.c_str();
    dbSource.srsOverride = overridesrs.c_str();
    dbSource.sql = sqlstr.c_str();
    if (!dbSource.Save(output)) {
      notify(NFY_FATAL, "Unable to write %s", output.c_str());
    }

  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Caught unknown exception");
  }

  return 0;
}

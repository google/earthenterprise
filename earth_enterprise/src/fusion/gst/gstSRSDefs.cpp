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


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <gstSRSDefs.h>
#include <sstream>
#include <string>
#include <gstTXTTable.h>

#include <ogr_spatialref.h>
#include <cpl_conv.h>
#include <cpl_string.h>
#include <geInstallPaths.h>
#include <khFileUtils.h>

// instantiate only one of these
gstSRSDefs instance;
gstSRSDefs& theSRSDefs(instance);

gstSRSDefs::gstSRSDefs() {
  init();
}

std::string utmzonename(int z) {
  int w = (z - 31) * 6;
  int e = w + 6;
  std::ostringstream oss;
  oss << "UTM Zone " << z << " (range " << abs(w);
  if (w < 0) {
    oss << "W";
  } else if (w > 0) {
    oss << "E";
  }
  oss << " - " << abs(e);
  if (e < 0) {
    oss << "W";
  } else if (e > 0) {
    oss << "E";
  }
  oss << ")";

  return oss.str();
}

void gstSRSDefs::init() {
  //
  // configure UTM projections
  //
  {
    srsDefs.push_back(Category("UTM / WGS84 / Northern Hemisphere"));
    Category& cat = srsDefs[srsDefs.size() - 1];
    for (int z = 1; z <= 60; ++z)
      cat.addProj(ProjFromEPSG(utmzonename(z), 32600 + z));
  }

  {
    srsDefs.push_back(Category("UTM / WGS84 / Southern Hemisphere"));
    Category& cat = srsDefs[srsDefs.size() - 1];
    for (int z = 1; z <= 60; ++z)
      cat.addProj(ProjFromEPSG(utmzonename(z), 32700 + z));
  }

  {
    srsDefs.push_back(Category("UTM / WGS72 / Northern Hemisphere"));
    Category& cat = srsDefs[srsDefs.size() - 1];
    for (int z = 1; z <= 60; ++z)
      cat.addProj(ProjFromEPSG(utmzonename(z), 32200 + z));
  }

  {
    srsDefs.push_back(Category("UTM / WGS72 / Southern Hemisphere"));
    Category& cat = srsDefs[srsDefs.size() - 1];
    for (int z = 1; z <= 60; ++z)
      cat.addProj(ProjFromEPSG(utmzonename(z), 32300 + z));
  }

  //
  // configure US State Plane projections
  //
  gstHeaderHandle spcsHeader = gstHeaderImpl::Create();
  spcsHeader->addSpec("ID", gstTagInt);
  spcsHeader->addSpec("STATE", gstTagString);
  spcsHeader->addSpec("ZONE", gstTagString);
  spcsHeader->addSpec("PROJ_METHOD", gstTagInt);
  spcsHeader->addSpec("DATUM", gstTagString);
  spcsHeader->addSpec("USGS_CODE", gstTagInt);
  spcsHeader->addSpec("EPSG_PCS_CODE", gstTagInt);

  gstTXTTable spcsTable(khComposePath(kGESharePath,
                                      "gdal/stateplane.csv").c_str());
  spcsTable.SetHeader(spcsHeader);
  spcsTable.SetFileType(gstTXTTable::Delimited);
  spcsTable.SetDelimiter(',');
  spcsTable.SetSkipRows(1);

  if (spcsTable.Open(GST_READONLY) == GST_OKAY) {
    srsDefs.push_back(Category("US State Plane / NAD27"));
    Category& spcs_nad27 = srsDefs[srsDefs.size() - 1];

    srsDefs.push_back(Category("US State Plane / NAD83"));
    Category& spcs_nad83 = srsDefs[srsDefs.size() - 1];

    char name[100];
    for (unsigned int r = 0; r < spcsTable.NumRows(); ++r) {
      gstRecordHandle rec = spcsTable.Row(r);
      snprintf(name, 100, "%s %s (%d)", rec->Field(1)->ValueAsString().c_str(),
              rec->Field(2)->ValueAsString().c_str(),
              rec->Field(0)->ValueAsInt());
      if (*(rec->Field(4)) == gstValue("NAD83")) {
        spcs_nad83.addProj(ProjFromEPSG(name, rec->Field(6)->ValueAsInt()));
      } else if (*(rec->Field(4)) == gstValue("NAD27")) {
        spcs_nad27.addProj(ProjFromEPSG(name, rec->Field(6)->ValueAsInt()));
      }
    }
  }
}

// --------------------------------------------------------------------


int gstSRSDefs::getCategoryCount() const {
  return srsDefs.size();
}

std::string gstSRSDefs::getCategoryName(int cat) const {
  return srsDefs[cat].name;
}

std::string gstSRSDefs::getProjectionName(int cat, int proj) const {
  return srsDefs[cat].proj[proj].name;
}

int gstSRSDefs::getProjectionCode(int cat, int proj) const {
  return srsDefs[cat].proj[proj].code;
}

int gstSRSDefs::getProjectionCount(int cat) const {
  return srsDefs[cat].proj.size();
}

std::string gstSRSDefs::getWKT(int cat, int proj) const {
  std::string wktstring;

  OGRSpatialReference srs;
  if (srs.importFromEPSG(getProjectionCode(cat, proj)) == OGRERR_NONE) {
    char* wkt = NULL;
    srs.exportToWkt(&wkt);
    wktstring = wkt;
    CPLFree(wkt);
  }

  return wktstring;
}

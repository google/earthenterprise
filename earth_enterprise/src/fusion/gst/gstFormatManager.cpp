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


#include <string>
#include <dlfcn.h>
#include <ctype.h>
#include <qstringlist.h>
#include <qregexp.h>

#include <khFileUtils.h>
#include <khConstants.h>
#include <gstFormatManager.h>
#include <notify.h>
#include <gstRegistry.h>
#include <gstFileUtils.h>

#include <gstOGRFormat.h>
#include <gstTXTFormat.h>
#include <gstKMLFormat.h>
#include <gstTextureManager.h>
#include <gstGDALFormat.h>
#include <gstGDTFormat.h>
#include <gstKVPFile.h>
#include <gstKHMFormat.h>


gstFormatManager* theFormatManager(void) {
  static gstFormatManager manager;
  return &manager;
}

gstFormatManager::gstFormatManager() {
  RegisterFormat<gstOGRFormat>("MapInfo", "MapInfo",
                               "*.tab *.TAB");
  RegisterFormat<gstOGRFormat>("MicroStation", "MicroStation",
                               "*.dgn *.DGN");
  RegisterFormat<gstOGRFormat>("ESRI Shape", "Shape",
                               "*.shp *.SHP");
  RegisterFormat<gstOGRFormat>("US Census Tiger Line Files", "Tiger",
                               "*.rt1 *.RT1");
  RegisterFormat<gstOGRFormat>("VPF File", "VPF",
                               "gltp:/*");
  RegisterFormat<gstOGRFormat>("OpenGIS GML", "GML",
                               "*.gml *.GML");
#if 0
  RegisterFormat<gstOGRSQLFormat>("OGR SQL", "OGRSQL",
                                  "*.ogrsql");
#endif
  RegisterFormat<gstTXTFormat>("Generic Text", "ASCII",
                               "*.txt *.csv *.TXT *.CSV");
  RegisterFormat<gstKMLFormat>("Keyhole Markup Language", "ASCII",
                               "*.kml *.kmz *.KML *.KMZ");
  RegisterFormatAsset<gstTEXFormat, false>(
      "Keyhole Image Product", "Keyhole Image Product", "*.kip");
  RegisterFormat<gstKVPFormat>("Keyhole Vector Product",
                               "Keyhole Vector Product", "*.kvp ");
  RegisterFormat<gstGDALFormat>("Supported Raster", "GDAL",
                                "*.tif *.TIF *.tiff *.TIFF *.img *.IMG *.jp2 *.JP2 *.rpf"
                                "*.RPF *.fit *.FIT *.dem *.DEM *.sid *.SID");
  RegisterFormat<gstKVGeomFormat>("Keyhole Vector Geometry",
                                  "Keyhole Vector Geometry", "*.kvgeom");
  RegisterFormat<gstGDTFormat>("Geographic Data Technology", "GDT",
                               "*cyb.t?? *cyi.t?? *pci.t?? *pcb.t?? *pli.t??"
                               "*plb.t?? *.tx1 *.TX1 *.tx7 *.TX7");
  RegisterFormat<gstKHMFormat>("Keyhole Mosaic", "Keyhole Mosaic",
                               "*.khm");
  RegisterFormat<gstKHMFormat>("Keyhole Virtual Raster",
                               "Keyhole Virtual Raster", "*.khvr");
  RegisterFormatAsset<gstTEXFormat, false>(
      "Keyhole Image Asset", "Keyhole Image Asset",
      (std::string("*") + kImageryAssetSuffix + std::string("*")).c_str());
  RegisterFormatAsset<gstTEXFormat, true>(
      "Keyhole Mercator Image Asset", "Keyhole Mercator Image Asset",
      (std::string("*") + kMercatorImageryAssetSuffix +
       std::string("*")).c_str());
  RegisterFormatAsset<gstTEXFormat, false>(
      "Keyhole Terrain Asset", "Keyhole Terrain Asset", "*.ktasset*");
}

gstFormatManager::~gstFormatManager() {
  for (std::vector<MetaFormat*>::iterator it = formats_.begin();
       it != formats_.end(); ++it) {
    delete (*it);
  }
}

gstFormat* gstFormatManager::FindFormat(const char* fname) {
  // search our format db to find a matching extension
  for (std::vector<MetaFormat*>::iterator it = formats_.begin();
       it != formats_.end(); ++it) {
    gstFormat* format = (*it)->Match(fname);
    if (format != NULL) {
      notify(NFY_DEBUG, "Format %s matches %s", (*it)->ShortDesc(), fname);
      return format;
    }
  }

  notify(NFY_DEBUG, "Unable to find a format that matches %s", fname);

  return NULL;
}

// -----------------------------------------------------------------------------

MetaFormat::MetaFormat(gstFormatNewFmtFunc format_func,
                       const char* desc,
                       const char* short_desc,
                       const char* filter)
    : new_format_func_(format_func),
      description_(desc),
      short_description_(short_desc),
      filter_(filter) {
  QStringList filters = QStringList::split(' ', filter_);
  for (QStringList::Iterator it = filters.begin();
       it != filters.end(); ++it) {
    QRegExp* rx = new QRegExp(*it);
    rx->setWildcard(true);
    patterns_.push_back(rx);
  }
}

MetaFormat::~MetaFormat() {
  for (std::vector<QRegExp*>::iterator it = patterns_.begin();
       it != patterns_.end(); ++it) {
    delete (*it);
  }
}

QString MetaFormat::FileDialogFilterString() const {
  return QString("%1 (%2)").arg(description_).arg(filter_);
}

gstFormat* MetaFormat::Match(const char* fname) {
  for (std::vector<QRegExp*>::iterator it = patterns_.begin();
       it != patterns_.end(); ++it) {
    QRegExp* rx = (*it);
    if (rx->exactMatch(fname)) {
      if (new_format_func_) {
        return (*new_format_func_)(fname);
      } else {
        break;
      }
    }
  }

  return NULL;
}

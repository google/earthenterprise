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


#include "fusion/gst/gstGDTFormat.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>

#include "fusion/gst/gstRecord.h"
#include "fusion/gst/gstTXTTable.h"
#include "fusion/gst/gstFileUtils.h"
#include "common/khException.h"
#include "common/notify.h"
#include "common/khHashTable.h"
#include "common/khGuard.h"
#include "common/khFileUtils.h"


gstGDTFormat::gstGDTFormat(const char* n)
    : gstFormat(n) {
}

gstGDTFormat::~gstGDTFormat() {
  CloseFile();
}

gstStatus gstGDTFormat::OpenFile() {
  //
  // determine what type of file this is
  //
  if (!khExists(name())) {
    notify(NFY_WARN, "Unable to access file: %s", name());
    return GST_OPEN_FAIL;
  }

  if (khHasExtension(name(), "st.tx1") || khHasExtension(name(), "ST.TX1")) {
    // looks like a road record, check the first few bytes to confirm
    //   the first line is a GDT copyright notice
    //   record type is contained in the first character of 2nd line
    //   version information is contained in characters 2-5 of 2nd line
    char version_buf[1024];
    int fd = ::open(name(), O_RDONLY);
    if (fd == -1) {
      notify(NFY_WARN, "Unable to open file: %s", name());
      return GST_OPEN_FAIL;
    }
    if (read(fd, version_buf, 1024) != 1024) {
      notify(NFY_WARN, "Unable to read file: %s", name());
      ::close(fd);
      return GST_READ_FAIL;
    } else {
      ::close(fd);
    }


    // skip copyright line
    char *nl = static_cast<char*>(memchr(&version_buf[0], '\n', 1024));
    if (nl == NULL) {
      notify(NFY_WARN,
             "Unable to skip copyright notice in first line of file: %s",
             name());
      return GST_READ_FAIL;
    }

    // move past new line
    ++nl;
    if (nl >= &version_buf[0] + 1019 || *nl != '1') {
      notify(NFY_WARN, "Unknown GDT road record type.");
      return GST_UNKNOWN;
    }

    // move past record type
    ++nl;
    int version_num = atoi(nl);

    GDTType primary, secondary;

    if (version_num == 5051 || version_num == 5077) {
      primary = TransType1_v3;
      secondary = TransType2_v3;
    } else {
      // versionNum == 3050
      // Dynamap Transportation v5.1, version 5020
      // Dynamap Transportation v5.3, version 0308
      primary = TransType1_v5;
      secondary = TransType2_v3;
    }

    Configure(primary_def_, primary, std::string(name()));
    gstFileInfo fi2(name());
    if (khExists(khReplaceExtension(name(), ".tx2"))) {
      Configure(secondary_def_, secondary, khReplaceExtension(name(), ".tx2"));
    } else if (khExists(khReplaceExtension(name(), ".TX2"))) {
      Configure(secondary_def_, secondary, khReplaceExtension(name(), ".TX2"));
    }
  } else if (khHasExtension(name(), "hy.tx1") || khHasExtension(name(), "HY.TX1")) {
    Configure(primary_def_, TransType1Hwy, name());
    if (khExists(khReplaceExtension(name(), ".tx2"))) {
      Configure(secondary_def_, TransType2_v3, khReplaceExtension(name(), ".tx2"));
    } else if (khExists(khReplaceExtension(name(), ".TX2"))) {
      Configure(secondary_def_, TransType2_v3, khReplaceExtension(name(), ".TX2"));
    }
  } else if (khHasExtension(name(), "tx1") || khHasExtension(name(), "TX1")) {
    Configure(primary_def_, DynamapPrimary, name());
    if (khExists(khReplaceExtension(name(), ".tx2"))) {
      Configure(secondary_def_, TransType2_v3, khReplaceExtension(name(), ".tx2"));
    } else if (khExists(khReplaceExtension(name(), ".TX2"))) {
      Configure(secondary_def_, TransType2_v3, khReplaceExtension(name(), ".TX2"));
    }
  } else if (khHasExtension(name(), "tx7")) {
    Configure(primary_def_, TransType7, name());
  } else if (khHasExtension(name(), "pci")) {
    Configure(primary_def_, PostalCodeInventory, name());
  } else if (khHasExtension(name(), "pcb")) {
    Configure(primary_def_, PostalCodeBoundary, name());
  } else if (khHasExtension(name(), "cyi")) {
    Configure(primary_def_, CountyInventory, name());
  } else if (khHasExtension(name(), "cyb")) {
    Configure(primary_def_, CountyBoundary, name());
  } else if (khHasExtension(name(), "pli")) {
    Configure(primary_def_, PlaceInventory, name());
  } else if (khHasExtension(name(), "plb")) {
    Configure(primary_def_, PlaceBoundary, name());
  } else {
    notify(NFY_WARN, "Unsupported type: %s", name());
    return GST_UNKNOWN;
  }

  if (primary_def_.table->Open(GST_READONLY) != GST_OKAY) {
    delete primary_def_.table;
    primary_def_.table = NULL;
    notify(NFY_WARN, "Unable to open file: %s", name());
    return GST_OPEN_FAIL;
  }

  if (secondary_def_.table) {
    if (secondary_def_.table->Open(GST_READONLY) != GST_OKAY) {
      notify(NFY_WARN, "Found secondary file, but unable to open it: %s",
             secondary_def_.table->name());
      delete secondary_def_.table;
      secondary_def_.table = NULL;
    }
  }

  if (!Parse())
    return GST_UNKNOWN;

  notify(NFY_DEBUG, "GDT file %s has %lu features", name(),
         static_cast<long unsigned>(raw_geometry_.size()));

  AddLayer(raw_geometry_.size(), primary_def_.table->GetHeader());

  return GST_OKAY;
}

gstStatus gstGDTFormat::CloseFile() {
  if (primary_def_.table) {
    primary_def_.table->Close();
    delete primary_def_.table;
    primary_def_.table = NULL;
  }

  if (secondary_def_.table) {
    secondary_def_.table->Close();
    delete secondary_def_.table;
    secondary_def_.table = NULL;
  }

  raw_geometry_.clear();
  attributes_.clear();

  return GST_OKAY;
}

gstGeodeHandle gstGDTFormat::GetFeatureImpl(std::uint32_t layer, std::uint32_t fidx) {
  // should be checked by gstSource before calling me
  assert(layer < NumLayers());
  assert(layer == 0);
  assert(fidx < NumFeatures(layer));

  gstGeodeHandle new_geode = raw_geometry_[fidx]->FromRaw();
  if (!new_geode) {
    throw khSoftException(kh::tr("Unable to interpret raw feature"));
  }
  return new_geode;
}

gstRecordHandle gstGDTFormat::GetAttributeImpl(std::uint32_t layer, std::uint32_t fidx) {
  // should be checked by gstSource before calling me
  assert(layer < NumLayers());
  assert(layer == 0);
  assert(fidx < NumFeatures(layer));

  return attributes_[fidx];
}

bool gstGDTFormat::Parse() {
  notify(NFY_INFO, "Parsing primary %s : %s", FormatName(), name());

  gstRecordHandle attrib;

  double fx = 0, fy = 0;

  khHashTable<gstGeodeHandle> hash_table(secondary_def_.table ? 100000 : 0);

  //
  // read entire contents of secondary file into memory, creating new
  // geodes for every unique gdt ID, and inserting into a hash table
  // by GDT ID for later correlation with primary file
  //
  if (secondary_def_.table) {
    for (unsigned int r = 0; r < secondary_def_.table->NumRows(); ++r) {
      attrib = secondary_def_.table->Row(r);
      if (!attrib) {
        notify(NFY_WARN, "Problems reading row %d from GDT file: %s",
               r, secondary_def_.table->name());
        return false;
      }

      gstGeodeHandle geodeh = hash_table.find(attrib->Field(2)->ValueAsInt());

      switch (secondary_def_.primType) {
        case gstUnknown:
          break;

          // TODO: below for all types we extract only
          // x,y-coordinates. Explore if we can get 2.5D geometry
          // from GDT-format.
        case gstPoint:
        case gstPolyLine:
        case gstStreet:
        case gstPolygon:
        case gstPoint25D:
        case gstPolyLine25D:
        case gstStreet25D:
        case gstPolygon25D:
        case gstPolygon3D: {
          // NOTE: looks like commmon part and should be before switch.
          // But it is placed here because not all branches are supported.
          if (!geodeh) {
            geodeh = gstGeodeImpl::Create(secondary_def_.primType);
            hash_table.insertBlind(attrib->Field(2)->ValueAsInt(), geodeh);
          }

          gstGeode* geode = static_cast<gstGeode*>(&(*geodeh));
          for (unsigned int ii = 0; ii < secondary_def_.geoFields.size(); ++ii) {
            GeoField gf = secondary_def_.geoFields[ii];
            fx = attrib->Field(gf.x)->ValueAsDouble();
            fy = attrib->Field(gf.y)->ValueAsDouble();
            if (fx == 0.0 || fy == 0.0)
              continue;
            geode->AddVertex(gstVertex(fx, fy));
          }
        }
          break;

        case gstMultiPolygon:
        case gstMultiPolygon25D:
        case gstMultiPolygon3D:
          // TODO: support multi-polygon features??!
          notify(NFY_WARN,
                 "%s: multi-polygon features are not supported(skipped).",
                 __func__);
          break;
      }
    }
  }

  // now read the primary file and if a secondary file exists look up the
  // feature id in the hash table to see if we need to add a new feature
  // or augment the existing feature
  for (unsigned int r = 0; r < primary_def_.table->NumRows(); ++r) {
    attrib = primary_def_.table->Row(r);
    if (!attrib) {
      notify(NFY_WARN, "Problems reading row %d from GDT file", r);
      return false;
    }

    gstGeodeHandle geodeh;

    switch (primary_def_.primType) {
      case gstUnknown:
        break;

        // TODO: below for all types we extract only
        // x,y-coordinates. Explore if we can have 2.5D geometry
        // in GDT-format.
      case gstPoint:
      case gstPolyLine:
      case gstStreet:
      case gstPolygon:
      case gstPoint25D:
      case gstPolyLine25D:
      case gstStreet25D:
      case gstPolygon25D:
      case gstPolygon3D: {
        // NOTE: looks like commmon part and should be before switch.
        // But it is placed here because not all branches are supported.
        if (!secondary_def_.table) {
          geodeh = gstGeodeImpl::Create(primary_def_.primType);
        } else {
          geodeh = hash_table.find(attrib->Field(2)->ValueAsInt());
          if (!geodeh)
            geodeh = gstGeodeImpl::Create(primary_def_.primType);
        }

        gstGeode* geode = static_cast<gstGeode*>(&(*geodeh));
        for (unsigned int ii = 0; ii < primary_def_.geoFields.size(); ++ii) {
          GeoField gf = primary_def_.geoFields[ii];
          fx = attrib->Field(gf.x)->ValueAsDouble();
          fy = attrib->Field(gf.y)->ValueAsDouble();
          if (fx == 0.0 || fy == 0.0)
            continue;
          if (ii == 0 && geode->NumParts() != 0) {
            geode->InsertVertex(0, 0, gstVertex(fx, fy));
          } else {
            geode->AddVertex(gstVertex(fx, fy));
          }
        }
      }
        break;

      case gstMultiPolygon:
      case gstMultiPolygon25D:
      case gstMultiPolygon3D:
        // TODO: support multi-polygon features??!
        notify(NFY_WARN,
               "%s: multi-polygon features are not supported.", __func__);
        break;
    }

    if (geodeh) {
      // create a raw version of this geode to minimize memory
      // since we will hold all geodes in memory
      raw_geometry_.push_back(gstRawGeodeImpl::Create(geodeh->ToRaw(NULL)));

      attributes_.push_back(attrib);
    }
  }

  return true;
}   // End gstGDTFormat::Parse()

void gstGDTFormat::Configure(FileDef& file_def, GDTType type,
                             const std::string& file_name) {
  //
  // *st.txx
  // Dynamap/Transportation version 5.0 2003
  //
  static gstHeaderImpl::FieldSpec transType1_v5[] = {
    { "RT", gstTagString, 1 },                 // Record Type (value "1")
    { "VERSION", gstTagInt, 4 },               // Version Number
    { "DYNAMAP_ID", gstTagInt, 10 },           // GDT Record Number
    { "FEDIRP", gstTagString, 2 },             // Feature Direction, Prefix
    { "FENAME", gstTagString, 30 },            // Feature Name
    { "FETYP", gstTagString, 6 },              // Feature Type
    { "FEDIRS", gstTagString, 2 },             // Feature Direction Suffix
    { "FCC", gstTagString, 3 },                // Feature Class Code
    { "FRADDL", gstTagString, 11 },            // From Address Left
    { "TOADDL", gstTagString, 11 },            // To Address Left
    { "FRADDR", gstTagString, 11 },            // From Address Right
    { "TOADDR", gstTagString, 11 },            // To Address Right
    { "POSTAL_L", gstTagInt, 5 },              // Postal Code (ZIP or FSA) Left
    { "POSTAL_R", gstTagInt, 5 },              // Postal Code (ZIP or FSA) Right
    { "FRLONG", gstTagDouble, 10, 0.000001 },  // Longitude From (leading -, implied 6 dec. places)
    { "FRLAT", gstTagDouble, 9, 0.000001 },    // Latitude From (leading +, implied 6 dec. places)
    { "TOLONG", gstTagDouble, 10, 0.000001 },  // Longitude To (leading -, implied 6 dec. places)
    { "TOLAT", gstTagDouble, 9, 0.000001 },    // Latitude To (leading +, implied 6 dec. places)
    { "ACC", gstTagInt, 1 },                   // Artery Classification Code ("1", "2", or "3")
    { "NAME_TYPE", gstTagString, 1 },          // "R" (always PRN for this product)
    { "SHIELD", gstTagString, 1 },             // "I", "U", "S", or blank
    { "HWY_NUM", gstTagString, 5 },            // #, # with letter, or blank (if SHIELD is filled)
    { "LENGTH", gstTagDouble, 8 },             // seg length in miles, (implied 4 decimal places)
    { "SPEED", gstTagInt, 3 },                 // speed in mph (US)
    { "ONE_WAY", gstTagString, 2 },            // "FT", "TF", or ""
    { "F_ZLEV", gstTagInt, 2 },                // functional From segment-end elevation
    { "T_ZLEV", gstTagInt, 2 },                // functional To segment-end elevation
    { "FT_COST", gstTagDouble, 8 },            // from-to travel time (minutes, implied 5 dec. places)
    { "TF_COST", gstTagDouble, 8 },            // to-from travel time (minutes, implied 5 dec. places)
    { "FT_DIR", gstTagString, 2 },             // From-to direction
    { "TF_DIR", gstTagString, 2 },             // To-from direction
    { "NAME_FLAG", gstTagString, 3 },          // Name metadata flag
    { "DELIMITER", gstTagString, 2 }           // Carriage return/line feed
  };

  static gstHeaderImpl::FieldSpec transType1_v3[] = {
    { "RT", gstTagString, 1 },                 // Record Type (value "1")
    { "VERSION", gstTagInt, 4 },               // Version Number
    { "DYNAMAP_ID", gstTagInt, 10 },           // GDT Record Number
    { "FEDIRP", gstTagString, 2 },             // Feature Direction, Prefix
    { "FENAME", gstTagString, 30 },            // Feature Name
    { "FETYP", gstTagString, 4 },              // Feature Type
    { "FEDIRS", gstTagString, 2 },             // Feature Direction Suffix
    { "FCC", gstTagString, 3 },                // Feature Class Code
    { "FRADDL", gstTagString, 11 },            // From Address Left
    { "TOADDL", gstTagString, 11 },            // To Address Left
    { "FRADDR", gstTagString, 11 },            // From Address Right
    { "TOADDR", gstTagString, 11 },            // To Address Right
    { "POSTAL_L", gstTagInt, 5 },              // Postal Code (ZIP or FSA) Left
    { "POSTAL_R", gstTagInt, 5 },              // Postal Code (ZIP or FSA) Right
    { "FRLONG", gstTagDouble, 10, 0.000001 },  // Longitude From (leading -, implied 6 dec. places)
    { "FRLAT", gstTagDouble, 9, 0.000001 },    // Latitude From (leading +, implied 6 dec. places)
    { "TOLONG", gstTagDouble, 10, 0.000001 },  // Longitude To (leading -, implied 6 dec. places)
    { "TOLAT", gstTagDouble, 9, 0.000001 },    // Latitude To (leading +, implied 6 dec. places)
    { "ACC", gstTagInt, 1 },                   // Artery Classification Code ("1", "2", or "3")
    { "NAME_TYPE", gstTagString, 1 },          // "R" (always PRN for this product)
    { "SHIELD", gstTagString, 1 },             // "I", "U", "S", or blank
    { "HWY_NUM", gstTagString, 5 },            // #, # with letter, or blank (if SHIELD is filled)
    { "LENGTH", gstTagDouble, 8 },             // seg length in miles, (implied 4 decimal places)
    { "SPEED", gstTagInt, 3 },                 // speed in mph (US)
    { "ONE_WAY", gstTagString, 2 },            // "FT", "TF", or ""
    { "F_ZLEV", gstTagInt, 2 },                // functional From segment-end elevation
    { "T_ZLEV", gstTagInt, 2 },                // functional To segment-end elevation
    { "FT_COST", gstTagDouble, 8 },            // from-to travel time (minutes, implied 5 dec. places)
    { "TF_COST", gstTagDouble, 8 },            // to-from travel time (minutes, implied 5 dec. places)
    { "DELIMITER", gstTagString, 2 }           // Carriage return/line feed
  };

  // *pci.txx
  static gstHeaderImpl::FieldSpec postalCodeInv[] = {
    { "State", gstTagString,  2 },
    { "Postal", gstTagString, 5 },              // Postal Code 5/3 digit ZIP/FSA
    { "EncPostal", gstTagString, 5 },           // Enclosing ZIP or FSA
    { "AreaMiles", gstTagDouble, 9 },           // Area in square miles
    { "GeoLat", gstTagDouble, 8, 0.000001 },    // Geometry-based Latitude
    { "GeoLong", gstTagDouble, 9, -0.000001 },  // Geometry-based Longitide
    { "DelLat", gstTagDouble, 8, 0.000001 },    // Delivery-based Latitude
    { "DelLong", gstTagDouble, 9, -0.000001 },  // Delivery-based Longitude
    { "Name", gstTagString, 28 },               // ZIP or FSA name
    { "PCType", gstTagString, 1 },              // Postal code type
    { "NameType", gstTagString, 1 },            // Name Type
    { "County1FIPS", gstTagString, 3 },         // County 1 FIPS
    { "County2FIPS", gstTagString, 3 },         // County 2 FIPS
    { "County3FIPS", gstTagString, 3 },         // County 3 FIPS
    { "RPOFlag", gstTagString, 1 },             // RPO Flag ("R" or blank)
    { "LastLineFlag", gstTagString, 1 },        // Lastline Flag
    { "PointLoc", gstTagString, 1 },            // Point Location ("A"=actual)
    { "Blank0", gstTagString, 3 },              // Blanks
    { "Delimiter", gstTagString, 2 }
  };

  // *pcb.txx
  static gstHeaderImpl::FieldSpec postalCodeBnds[] = {
    { "LeftState", gstTagString, 2 },
    { "LeftCounty", gstTagString, 3 },
    { "LeftPostalCode", gstTagString, 5 },
    { "Blank0", gstTagString, 3 },
    { "RightState", gstTagString, 2 },
    { "RightCounty", gstTagString, 3 },
    { "RightPostalCode", gstTagString, 5 },
    { "Blank1", gstTagString, 3 },
    { "FromLat", gstTagDouble, 8, 0.000001 },
    { "FromLong", gstTagDouble, 9, -0.000001 },
    { "ToLat", gstTagDouble, 8, 0.000001 },
    { "ToLong", gstTagDouble, 9, -0.000001 },
    { "Delimiter", gstTagString, 2 }
  };

  // *cyi.t37
  static gstHeaderImpl::FieldSpec countyInventory[] = {
    { "State", gstTagString, 2 },                   // State/Province FIPS code
    { "County", gstTagString, 3 },                  // County/CD FIPS code
    { "Blank0", gstTagString, 13 },
    { "StateAbrv", gstTagString, 2 },               // State/Prov alpha abbrv
    { "CountyName", gstTagString, 28 },             // Full County/CD Name
    { "Area", gstTagString, 11 },                   // Decimal point in pos 56
    { "Blank1", gstTagString, 1 },
    { "Latitude", gstTagDouble, 9, 0.000001 },      // Decimal point in pos 63
    { "Longitude", gstTagDouble, 11, -0.0000001 },  // Signed, decimal in pos 74
    { "Delimiter", gstTagDouble, 2 }                // Carriage return/line feed
  };


  // *cyb.t37
  static gstHeaderImpl::FieldSpec countyBoundary[] = {
    { "LeftState", gstTagString, 2 },            // State/Prov FIPS code left
    { "LeftCounty", gstTagString, 3 },           // County/CD FIPS code left
    { "Blank0", gstTagString, 10 },
    { "RightState", gstTagString, 2 },           // State/Prov FIPS code right
    { "RightCounty", gstTagString, 3 },          // County/CD FIPS code right
    { "Blank1", gstTagString, 10 },
    { "FromLat", gstTagDouble, 8, 0.000001 },
    { "FromLong", gstTagDouble, 9, -0.000001 },
    { "ToLat", gstTagDouble, 8, 0.000001 },
    { "ToLong", gstTagDouble, 9, -0.000001 },
    { "Delimiter", gstTagString, 2 }
  };

  // *pli.t31
  static gstHeaderImpl::FieldSpec placeInventory[] = {
    { "Name", gstTagString, 40 },                 // Cleaned name
    { "Key", gstTagString, 10 },                  // State=2, County=3, Place=5
    { "Capital", gstTagString, 1 },               // State capitol: Y=yes N=no
    { "Population", gstTagUInt64, 10 },
    { "Longitude", gstTagDouble, 10, 0.000001 },  // Implied 6 decimal places
    { "Latitude", gstTagDouble, 9, 0.000001 },    // Implied 6 decimal places
    { "Delimiter", gstTagDouble, 2 }
  };

  // *plb.t37
  static gstHeaderImpl::FieldSpec placeBoundary[] = {
    { "LeftState", gstTagString, 2 },
    { "LeftCounty", gstTagString, 3 },
    { "LeftPlace", gstTagString, 5 },
    { "Blank0", gstTagString, 5 },
    { "RightState", gstTagString, 2 },
    { "RightCounty", gstTagString, 3 },
    { "RightPlace", gstTagString, 5 },
    { "Blank1", gstTagString, 5 },
    { "FromLat", gstTagDouble, 8, 0.000001 },
    { "FromLong", gstTagDouble, 9, -0.000001 },
    { "ToLat", gstTagDouble, 8, 0.000001 },
    { "ToLong", gstTagDouble, 9, -0.000001 },
    { "Delimiter", gstTagString, 2 }
  };


#if 0
  static gstHeaderImpl::FieldSpec placeInventory[] = {
    { "State", gstTagString, 2 },
    { "County", gstTagString, 3 },
    { "Place", gstTagString, 5 },
    { "Blank0", gstTagString, 1 },
    { "StateAbrv", gstTagString, 2 },
    { "CountyName", gstTagString, 28 },
    { "PlaceName", gstTagString, 40 },
    { "Area", gstTagDouble, 11 },
    { "Blank1", gstTagString, 1 },
    { "Latitude", gstTagDouble, 9, 0.000001 },
    { "Longitude", gstTagDouble, 11, 0.000001 },
    { "Delimiter", gstTagString, 2 }
  };
#endif


  static gstHeaderImpl::FieldSpec transType2_v3[] = {
    { "RT", gstTagString, 1 },              // Record Type (value "2")
    { "VERSION", gstTagString, 4 },         // Version Number
    { "DYNAMAP_ID", gstTagString, 10 },     // GDT Record Number
    { "RTSQ", gstTagString, 3 },            // Record Sequence Number
    { "Lon0", gstTagDouble, 10, .000001 },
    { "Lat0", gstTagDouble, 9, .000001 },
    { "Lon1", gstTagDouble, 10, .000001 },
    { "Lat1", gstTagDouble, 9, .000001 },
    { "Lon2", gstTagDouble, 10, .000001 },
    { "Lat2", gstTagDouble, 9, .000001 },
    { "Lon3", gstTagDouble, 10, .000001 },
    { "Lat3", gstTagDouble, 9, .000001 },
    { "Lon4", gstTagDouble, 10, .000001 },
    { "Lat4", gstTagDouble, 9, .000001 },
    { "Lon5", gstTagDouble, 10, .000001 },
    { "Lat5", gstTagDouble, 9, .000001 },
    { "Lon6", gstTagDouble, 10, .000001 },
    { "Lat6", gstTagDouble, 9, .000001 },
    { "Lon7", gstTagDouble, 10, .000001 },
    { "Lat7", gstTagDouble, 9, .000001 },
    { "Lon8", gstTagDouble, 10, .000001 },
    { "Lat8", gstTagDouble, 9, .000001 },
    { "Lon9", gstTagDouble, 10, .000001 },
    { "Lat9", gstTagDouble, 9, .000001 },
    { "DELIMITER", gstTagString, 2 }       // Carriage return/line feed
  };

  static gstHeaderImpl::FieldSpec transType7[] = {
    { "RT", gstTagString, 1 },               // Record Type
    { "VERSION", gstTagInt, 4 },             // Version Number
    { "STATE", gstTagString, 2 },            // FIPS State Code for File
    { "COUNTY", gstTagString, 3 },           // FIPS County Code for File
    { "LAND", gstTagInt, 10 },               // Landmark Identification Number
    { "SOURCE", gstTagString, 1 },           // Source Code
    { "FCC", gstTagString, 3 },              // Feature Class Code
    { "LANAME", gstTagString, 30 },          // Landmark Feature Name
    { "LONG", gstTagDouble, 10, 0.000001 },  // Longitude (point only)
    { "LAT", gstTagDouble, 9, 0.000001 },    // Latitude (point only)
    { "FILLER", gstTagString, 1 },           // Filler (make even char count)
    { "DELIMITER", gstTagString, 2 }         // Carriage Return/Line Feed
  };

  static gstHeaderImpl::FieldSpec transType1Hwy[] = {
    { "RT", gstTagString, 1 },         // Record Type (value "1")
    { "VERSION", gstTagInt, 4 },
    { "SEGMENT_ID", gstTagInt, 10 },
    { "FEDIRP", gstTagString, 2 },
    { "FENAME", gstTagString, 30 },
    { "FETYP", gstTagString, 6 },
    { "FEDIRS", gstTagString, 2 },
    { "FCC", gstTagString, 3 },
    { "FRLONG", gstTagDouble, 10, 0.000001 },
    { "FRLAT", gstTagDouble, 9, 0.000001 },
    { "TOLONG", gstTagDouble, 10, 0.000001 },
    { "TOLAT", gstTagDouble, 9, 0.000001 },
    { "ACC", gstTagInt, 1 },
    { "SHIELD", gstTagString, 1 },
    { "HWY_NUM", gstTagString, 5 },
    { "LENGTH", gstTagDouble, 8 },
    { "SPEED", gstTagInt, 3 },
    { "ONE_WAY", gstTagString, 2 },
    { "F_ZLEV", gstTagInt, 2 },
    { "T_ZLEV", gstTagInt, 2 },
    { "FT_COST", gstTagDouble, 8 },
    { "TF_COST", gstTagDouble, 8 },
    { "FT_DIR", gstTagString, 2 },
    { "TF_DIR", gstTagString, 2 },
    { "NAME_FLAG", gstTagString, 3 },
    { "DELIMITER", gstTagString, 2 },
  };

#if 0
  static gstHeaderImpl::FieldSpec transType8[] = {
    { "RT", gstTagString, 1 },    // Record Type (value "8")
    { "VERSION", gstTagInt, 4 },
    { "STATE", gstTagInt, 2 },
    { "COUNTY", gstTagInt, 3 },
    { "FILE_ID", gstTagInt, 5 },
    { "POLY_ID", gstTagInt, 10 },
    { "LOCATION_ID", gstTagInt, 10 },
    { "FILLER", gstTagString, 1 },
    { "DELIMITER", gstTagString, 2 },
  };

  static gstHeaderImpl::FieldSpec transTypeA[] = {
    { "RT", gstTagString, 1 },
    { "VERSION", gstTagInt, 4 },
    { "STATE", gstTagInt, 2 },
    { "COUNTY", gstTagInt, 3 },
    { "CENID", gstTagInt, 5 },
    { "POLYID", gstTagInt, 10 },
    { "FILLER", gstTagString, 73 },
    { "DELIMITER", gstTagString, 2 },
  };

  static gstHeaderImpl::FieldSpec transTypeI[] = {
    { "RT", gstTagString, 1 },           // Record Type (value "I")
    { "VERSION", gstTagInt, 4 },         // Version Number
    { "DYNAMAP_ID", gstTagInt, 10 },     // Permanent Dynamap ID
    { "STATE", gstTagInt, 2 },           // State FIPS code
    { "COUNTY", gstTagInt, 3 },          // County FIPS code
    { "RTLINK", gstTagString, 1 },       // Set to blank
    { "FILE_ID_LEFT", gstTagInt, 5 },    // Left file ID
    { "POLY_ID_LEFT", gstTagInt, 10 },   // Left side polygon ID
    { "FILE_ID_RIGHT", gstTagInt, 5 },   // Right file ID
    { "POLY_ID_RIGHT", gstTagInt, 10 },  // Right side polygon ID
    { "FILLER", gstTagString, 1 },       // to even the record length
    { "DELIMITER", gstTagString, 2 }
  };
#endif

  // Dynamap/2000 11.0
  static gstHeaderImpl::FieldSpec dynamap2000Type1[] = {
    { "RT", gstTagString, 1 },       // 0: Record Type (Value "1")
    { "VERSION", gstTagInt, 4 },     // 1: Version Number
    { "RECNUM", gstTagInt, 10 },     // 2: GDT Record Number
    { "SIDE1", gstTagInt, 1 },       // 3: Single Side Setment Code
    { "SOURCE", gstTagString, 1 },   // 4: Source Code
    { "FEDIRP", gstTagString, 2 },   // 5: Feature Direction, Prefix
    { "FENAME", gstTagString, 30 },  // 6: Feature Name
    { "FETYPE", gstTagString, 4 },   // 7: Feature Type
    { "FEDIRS", gstTagString, 2 },   // 8: Feature Direction Suffix
    { "FCC", gstTagString, 3 },      // 9: Feature Class Code
    { "FRADDL", gstTagString, 11 },  // 10: From Address Left
    { "TOADDL", gstTagString, 11 },  // 11: To Address Left
    { "FRADDR", gstTagString, 11 },  // 12: From Address Right
    { "TOADDR", gstTagString, 11 },  // 13: To Address Right
    { "FRIADDFL", gstTagInt, 1 },    // 14: From Imputed Address Flag Left
    { "TOIADDFL", gstTagInt, 1 },    // 15: To Imputed Address Flag Left
    { "FRIADDFR", gstTagInt, 1 },    // 16: From Imputed Address Flag Right
    { "TOIADDFR", gstTagInt, 1 },    // 17: To Imputed Address Flag Right
    { "ZIPL", gstTagInt, 5 },        // 18: ZIP Code Left
    { "ZIPR", gstTagInt, 5 },        // 19: ZIP Code Right
    { "FAIRL", gstTagString, 5 },    // 20:
    { "FAIRR", gstTagString, 5 },    // 21:
    { "ANRCL", gstTagString, 2 },    // 22:
    { "ANRCR", gstTagString, 2 },    // 23:
    { "STATEL", gstTagString, 2 },   // 24:
    { "STATER", gstTagString, 2 },   // 25:
    { "COUNTYL", gstTagString, 3 },  // 26:
    { "COUNTYR", gstTagString, 3 },  // 27:
    { "FMCDL", gstTagString, 5 },    // 28:
    { "FMCDR", gstTagString, 5 },    // 29:
    { "FSMCDL", gstTagString, 5 },   // 30:
    { "FSMCDR", gstTagString, 5 },   // 31:
    { "FPLL", gstTagString, 5 },     // 32:
    { "FPLR", gstTagString, 5 },     // 33:
    { "CTBNAL", gstTagString, 6 },   // 34:
    { "CTBNAR", gstTagString, 6 },   // 35:
    { "BLKL", gstTagString, 4 },     // 36:
    { "BLKR", gstTagString, 4 },     // 37:
    { "FRLONG", gstTagDouble, 10, 0.000001 },  // 38:
    { "FRLAT", gstTagDouble, 9, 0.000001 },    // 39:
    { "TOLONG", gstTagDouble, 10, 0.000001 },  // 40:
    { "TOLAT", gstTagDouble, 9, 0.000001 },    // 41:
    { "DELIMITER", gstTagString, 2 }           // 42:
  };


  static gstHeaderImpl::FieldSpec dynamap2000Type2[] = {
    { "RT", gstTagInt, 1 },                 // Record Type (value "2")
    { "VERSION", gstTagInt, 4 },            // Version Number
    { "RECNUM", gstTagInt, 10 },            // GDT Record Number
    { "RTSQ", gstTagInt, 3 },               // Record Sequence Number
    { "Lon0", gstTagDouble, 10, .000001 },
    { "Lat0", gstTagDouble, 9, .000001 },
    { "Lon1", gstTagDouble, 10, .000001 },
    { "Lat1", gstTagDouble, 9, .000001 },
    { "Lon2", gstTagDouble, 10, .000001 },
    { "Lat2", gstTagDouble, 9, .000001 },
    { "Lon3", gstTagDouble, 10, .000001 },
    { "Lat3", gstTagDouble, 9, .000001 },
    { "Lon4", gstTagDouble, 10, .000001 },
    { "Lat4", gstTagDouble, 9, .000001 },
    { "Lon5", gstTagDouble, 10, .000001 },
    { "Lat5", gstTagDouble, 9, .000001 },
    { "Lon6", gstTagDouble, 10, .000001 },
    { "Lat6", gstTagDouble, 9, .000001 },
    { "Lon7", gstTagDouble, 10, .000001 },
    { "Lat7", gstTagDouble, 9, .000001 },
    { "Lon8", gstTagDouble, 10, .000001 },
    { "Lat8", gstTagDouble, 9, .000001 },
    { "Lon9", gstTagDouble, 10, .000001 },
    { "Lat9", gstTagDouble, 9, .000001 },
    { "DELIMITER", gstTagString, 2 }       // Carriage return/line feed
  };


  int skip_rows = 1;
  int count = 0;
  gstHeaderImpl::FieldSpec* spec = 0;

  switch (type) {
    case TransType1_v3:
      count = sizeof transType1_v3 / sizeof(gstHeaderImpl::FieldSpec);
      spec = &transType1_v3[ 0 ];
      file_def.primType = gstPolyLine;
      file_def.geoFields.push_back(GeoField(14, 15));
      file_def.geoFields.push_back(GeoField(16, 17));
      break;

    case TransType1_v5:
      count = sizeof transType1_v5 / sizeof(gstHeaderImpl::FieldSpec);
      spec = &transType1_v5[ 0 ];
      file_def.primType = gstPolyLine;
      file_def.geoFields.push_back(GeoField(14, 15));
      file_def.geoFields.push_back(GeoField(16, 17));
      break;

    case TransType2_v3:
      count = sizeof transType2_v3 / sizeof(gstHeaderImpl::FieldSpec);
      spec = &transType2_v3[ 0 ];
      file_def.primType = gstPolyLine;
      file_def.geoFields.push_back(GeoField(4, 5));
      file_def.geoFields.push_back(GeoField(6, 7));
      file_def.geoFields.push_back(GeoField(8, 9));
      file_def.geoFields.push_back(GeoField(10, 11));
      file_def.geoFields.push_back(GeoField(12, 13));
      file_def.geoFields.push_back(GeoField(14, 15));
      file_def.geoFields.push_back(GeoField(16, 17));
      file_def.geoFields.push_back(GeoField(18, 19));
      file_def.geoFields.push_back(GeoField(20, 21));
      file_def.geoFields.push_back(GeoField(22, 23));
      skip_rows = 0;
      break;

    case DynamapPrimary:
      count = sizeof dynamap2000Type1 / sizeof(gstHeaderImpl::FieldSpec);
      spec = &dynamap2000Type1[ 0 ];
      file_def.primType = gstPolyLine;
      file_def.geoFields.push_back(GeoField(38, 39));
      file_def.geoFields.push_back(GeoField(40, 41));
      break;

    case TransType1Hwy:
      count = sizeof transType1Hwy / sizeof(gstHeaderImpl::FieldSpec);
      spec = &transType1Hwy[ 0 ];
      file_def.primType = gstPolyLine;
      file_def.geoFields.push_back(GeoField(8, 9));
      file_def.geoFields.push_back(GeoField(10, 11));
      break;

    case TransType7:
      count = sizeof transType7 / sizeof(gstHeaderImpl::FieldSpec);
      spec = &transType7[ 0 ];
      file_def.primType = gstPoint;
      file_def.geoFields.push_back(GeoField(8, 9));
      skip_rows = 0;
      break;

    case PostalCodeInventory:
      count = sizeof postalCodeInv / sizeof(gstHeaderImpl::FieldSpec);
      spec = &postalCodeInv[ 0 ];
      file_def.primType = gstPoint;
      file_def.geoFields.push_back(GeoField(5, 4));
      skip_rows = 12;
      break;

    case PostalCodeBoundary:
      count = sizeof postalCodeBnds / sizeof(gstHeaderImpl::FieldSpec);
      spec = &postalCodeBnds[0];
      file_def.primType = gstPolyLine;
      file_def.geoFields.push_back(GeoField(9, 8));
      file_def.geoFields.push_back(GeoField(11, 10));
      skip_rows = 12;
      break;

    case CountyInventory:
      count = sizeof countyInventory / sizeof(gstHeaderImpl::FieldSpec);
      spec = &countyInventory[0];
      file_def.primType = gstPoint;
      file_def.geoFields.push_back(GeoField(8, 7));
      skip_rows = 0;
      break;

    case CountyBoundary:
      count = sizeof countyBoundary / sizeof(gstHeaderImpl::FieldSpec);
      spec = &countyBoundary[0];
      file_def.primType = gstPolyLine;
      file_def.geoFields.push_back(GeoField(7, 6));
      file_def.geoFields.push_back(GeoField(9, 8));
      skip_rows = 0;
      break;

    case PlaceInventory:
      count = sizeof placeInventory / sizeof(gstHeaderImpl::FieldSpec);
      spec = &placeInventory[0];
      file_def.primType = gstPoint;
      file_def.geoFields.push_back(GeoField(4, 5));
      skip_rows = 0;
      break;

    case PlaceBoundary:
      count = sizeof placeBoundary / sizeof(gstHeaderImpl::FieldSpec);
      spec = &placeBoundary[0];
      file_def.primType = gstPolyLine;
      file_def.geoFields.push_back(GeoField(9, 8));
      file_def.geoFields.push_back(GeoField(11, 10));
      skip_rows = 0;
      break;

    case Unknown:
      count = 0;
      break;

    case TransType2_v5:
      break;
  }

  gstHeaderHandle header = gstHeaderImpl::Create();
  for (int ii = 0; ii < count; ++ii)
    header->addSpec(gstHeaderImpl::FieldSpec(spec[ii]));

  file_def.table = new gstTXTTable(file_name.c_str());
  file_def.table->SetHeader(header);
  file_def.table->SetFileType(gstTXTTable::FixedWidth);
  file_def.table->SetSkipRows(skip_rows);
}

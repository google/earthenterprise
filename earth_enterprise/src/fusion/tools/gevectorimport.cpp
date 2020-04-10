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


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>

#include <vector>
#include <string>
#include <map>

#include <qtextcodec.h>
#include <qdir.h>

#include "fusion/gst/gstGridUtils.h"
#include "fusion/gst/gstGeode.h"
#include "fusion/gst/gstSource.h"
#include "fusion/gst/gstFileUtils.h"
#include "fusion/gst/gstKVPFile.h"
#include "fusion/gst/gstKVPTable.h"
#include "fusion/gst/gstRecord.h"
#include "fusion/gst/gstGeometryChecker.h"
#include "fusion/gst/gstPolygonCleaner.h"
#include "fusion/gst/.idl/gstKVPAsset.h"
#include "fusion/gst/gstJobStats.h"

#include "common/khConstants.h"
#include "common/khGetopt.h"
#include "common/khFileUtils.h"
#include "common/khException.h"
#include "common/khTileAddr.h"
#include "common/RuntimeOptions.h"

#ifdef JOBSTATS_ENABLED
enum {VALIDATE, IMPORT, OPENSRC, OPENDST, GETGEODE, PUTGEODE,
      GETRECORD, PUTRECORD, SAVE};
static gstJobStats::JobName JobNames[] = {
  {VALIDATE,   "Validate All      "},
  {IMPORT,     "Import All        "},
  {OPENSRC,    "Open Source       "},
  {OPENDST,    "Open Destination  "},
  {GETGEODE,   "Retrieve Geode    "},
  {PUTGEODE,   "Store Geode       "},
  {GETRECORD,  "Retrieve Record   "},
  {PUTRECORD,  "Store Record      "},
  {SAVE,       "Save Asset        "}
};
gstJobStats* import_stats = new gstJobStats("IMPORT", JobNames, 9);
#endif

char* cmd_name;

void usage(const char* msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }
  std::string options("Options:\n");
  options.append(
      "  -e, --encoding <name>        specify text encoding of source file\n");
  options.append(
      "  -f, --filelist <file>        text file listing sources to import\n");
  options.append(
      "  -l, --layer <layer id>       source layer id to import\n");
  options.append(
      "  -q, --query_encodings        dump list of supported encodings\n");
  options.append(
      "  -v, --validate               validate command without doing any"
      " work\n");
  options.append(
      "  -s, --scale <to-meters>      convert to meters while importing\n");
  options.append(
      "      --force2D                enforce 2D coordinates\n");
  options.append(
      "      --force25D               enforce 2.5D coordinates\n");

  if (RuntimeOptions::GoogleInternal())
    options.append(
        "      --force3D                enforce 3D coordinates\n");

  options.append(
      "      --ignore_bad_features    ignore bad features in source\n");
  options.append(
      "      --do_not_fix_invalid_geometries    don't fix invalid geometries"
      " in source (coincident vertices, spikes)\n");
  options.append(
      "      --north_boundary <double> Crop to north_boundary"
      " (latitude in decimal deg)\n");
  options.append(
      "      --south_boundary <double> Crop to south_boundary"
      " (latitude in decimal deg)\n");
  options.append(
      "      --east_boundary <double> Crop to east_boundary"
      " (longitude in decimal deg)\n");
  options.append(
      "      --west_boundary <double> Crop to west_boundary"
      " (longitude in decimal deg)\n");

  fprintf(stderr,
          "\n"
          "usage: %s [Options] {-o, --output <outfile>} <source file>...\n\n"
          "%s"
          "\n", cmd_name, options.c_str());

  exit(EXIT_FAILURE);
}

bool validateSources(const std::vector<std::string>& srclist, unsigned int layer) {
  notify(NFY_NOTICE, "Begin source validation phase.");
  if (srclist.size() == 0) {
    fprintf(stderr, "At least one source is required.");
    return false;
  }

  int total_features = 0;
  bool have_header = false;
  gstHeaderHandle match_header;

  for (std::vector<std::string>::const_iterator it = srclist.begin();
       it != srclist.end(); ++it) {
    notify(NFY_NOTICE, "Opening source %s", (*it).c_str());

    gstSource src((*it).c_str());
    if (src.Open() != GST_OKAY) {
      notify(NFY_WARN, "Failed to open source %s", (*it).c_str());
      return false;
    }

    if (layer >= src.NumLayers()) {
      notify(NFY_WARN,
             "Layer (%u) out of range, "
             "must be less than %u for source: %s",
             layer,
             src.NumLayers(),
             (*it).c_str());
      return false;
    }

    if (!have_header) {
      match_header = src.GetAttrDefs(layer);
      have_header = true;
    } else {
      if (*src.GetAttrDefs(layer) != *match_header) {
        notify(NFY_WARN, "Headers do not match!");
        return false;
      }
    }

    notify(NFY_NOTICE, "Layer %d has %d Features", layer,
           src.NumFeatures(layer));
    total_features += src.NumFeatures(layer);
  }

  notify(NFY_NOTICE, "Total features: %d", total_features);
  return true;
}

// returns true if area of polygon greater then specified by max25d_area,
// otherwise it returns false.
bool VerifyPolygon25DByArea(const gstGeodeHandle &geode,
                            const int featurecount,
                            const double max25d_area) {
  assert(geode->PrimType() == gstPolygon25D);
  assert(!geode->IsDegenerate());

  double x, y, area;
  if (geode->Centroid(0, &x, &y, &area) == 0) {
    notify(
        NFY_DEBUG,
        "feature # %d : type=%s, parts=%d, vertexes=%d area=%.16lf",
        featurecount, PrettyPrimType(geode->PrimType()).c_str(),
        geode->NumParts(), geode->TotalVertexCount(), area);
  }

  return (fabs(area) > max25d_area);
}

// Class FeatureStatisticsCalculator calculates different statistics
// (min/max/average feature diameter) for source features.
// Note: It is used to handle features in product coordinates.
// The parameterization of the earth is within the unit square [0, 1]x[0, 1].
// So initial values for min/max diameter are 0.0/1.0.
class FeatureStatisticsCalculator {
 public:
  FeatureStatisticsCalculator()
      : count_(0),
        min_diameter_(1.),
        max_diameter_(.0),
        sum_diameters_(.0) {
  }

  void Reset() {
    count_ = 0;
    min_diameter_ = 1.;
    max_diameter_ = .0;
    sum_diameters_ = .0;
  }

  void Accept(const gstGeodeHandle &geodeh) {
    switch (geodeh->PrimType()) {
      case gstUnknown:
      case gstPoint:
      case gstPoint25D:
        break;

      case gstPolyLine:
      case gstPolyLine25D:
      case gstStreet:
      case gstStreet25D:
        // TODO: should be changed after support MultiPolyline
        // geometry type.
        for (unsigned int p = 0; p < geodeh->NumParts(); ++p) {
          double cur_diameter = geodeh->BoundingBoxOfPart(p).Diameter();
          AcceptDiameter(cur_diameter);
        }
        break;

      case gstPolygon:
      case gstPolygon25D:
        {
          double cur_diameter = geodeh->BoundingBox().Diameter();
          AcceptDiameter(cur_diameter);
        }
        break;

      case gstPolygon3D:
        // TODO: not sure if it is needed.
        break;

      case gstMultiPolygon:
      case gstMultiPolygon25D:
        {
          for (unsigned int p = 0; p < geodeh->NumParts(); ++p) {
            double cur_diameter = geodeh->BoundingBoxOfPart(p).Diameter();
            AcceptDiameter(cur_diameter);
          }
        }
        break;

      case gstMultiPolygon3D:
        // TODO: not sure if it is needed.
        break;

      default:
        notify(NFY_FATAL, "Invalid geometry type %d", geodeh->PrimType());
        break;
    }
  }

  double MinDiameter() const {
    return min_diameter_;
  }

  double MaxDiameter() const {
    return max_diameter_;
  }

  double AverageDiameter() const {
    return (count_ ? sum_diameters_ / count_ : .0);
  }

 private:
  void AcceptDiameter(const double diameter) {
    ++count_;
    if (diameter < min_diameter_) {
      min_diameter_ = diameter;
    }
    if (diameter > max_diameter_) {
      max_diameter_ = diameter;
    }
    sum_diameters_ += diameter;
  }

  int    count_;
  double min_diameter_;
  double max_diameter_;
  double sum_diameters_;
};


int main(int argc, char** argv) {
  try {
    const bool google_internal = RuntimeOptions::GoogleInternal();
    cmd_name = strrchr(argv[0], '/');
    cmd_name = cmd_name == NULL ? argv[0] : cmd_name + 1;

    int argn;
    std::string output;
    std::string file_list_name;
    QString codec_str;
    int layer = 0;
    double scale = 1.0;
    bool validate_only = false;
    bool force_2d = false;
    bool force_25d = false;
    bool force_3d = false;
    bool ignore_bad_features = false;
    bool do_not_fix_invalid_geometries = false;
    bool query_encodings = false;
    khFileList file_list;
    double north_boundary = 90.0;
    double south_boundary = -90.0;
    double east_boundary = 180.0;
    double west_boundary = -180.0;

    khGetopt options;
    options.opt("output", output);
    options.opt("filelist", file_list_name);
    options.opt("encoding", codec_str);
    options.opt("layer", layer);
    options.opt("scale", scale);
    options.flagOpt("validate", validate_only);
    options.flagOpt("force2D", force_2d);
    options.flagOpt("force25D", force_25d);

    if (google_internal)
      options.flagOpt("force3D", force_3d);

    options.flagOpt("ignore_bad_features", ignore_bad_features);
    options.flagOpt("do_not_fix_invalid_geometries",
                    do_not_fix_invalid_geometries);
    options.flagOpt("query_encodings", query_encodings);
    options.opt("north_boundary", north_boundary);
    options.opt("south_boundary", south_boundary);
    options.opt("east_boundary", east_boundary);
    options.opt("west_boundary", west_boundary);

    if (!options.processAll(argc, argv, argn)) {
      usage();
    }

    // all remaining args must be input files
    while (argn < argc) {
      try {
        file_list.AddFile(argv[argn++]);
      } catch(const std::exception &e) {
        notify(NFY_FATAL, "%s", e.what());
      } catch(...) {
        notify(NFY_FATAL, "Unknown error");
      }
    }

    // query must exit
    if (query_encodings) {
      QTextCodec* codec;
      for (int i = 0; (codec = QTextCodec::codecForIndex(i)); ++i)
        printf("%s\n", codec->name().constData());
      exit(EXIT_SUCCESS);
    }

    // strip trailing slash if exists
    while (output.length() && output[output.length() - 1] == '/')
      output.resize(output.length() - 1);
    if (output.empty()) {
      usage("Output is required");
    }

    // only accept one 'force' modifier
    if ((force_2d && force_25d) ||
        (force_2d && force_3d) ||
        (force_25d && force_3d)) {
      if (google_internal)
        usage("Can't use --force_2D/--force_25D/--force_3D at the same time");
      else
        usage("Can't use --force_2D/--force_25D at the same time");
    }

    // validate codec
    if (!codec_str.isEmpty()) {
      // Don't delete this codec when we're done with it.
      // It's owned by the Qt internals.
      QTextCodec* qcodec = QTextCodec::codecForName(codec_str.latin1(), 1);
      if (qcodec == 0) {
        fprintf(stderr, "Invalid codec supplied: %s\n", codec_str.latin1());
        fprintf(stderr, "Valid codec must be one of the following:\n");
        QTextCodec* codec;
        for (int i = 0; (codec = QTextCodec::codecForIndex(i)); ++i)
          printf("%s\n", codec->name().constData());
        exit(EXIT_FAILURE);
      }
    }

    if (north_boundary <= south_boundary) {
      notify(NFY_FATAL, "north_boundary <= south_boundary!.\n");
    }
    if (east_boundary <= west_boundary) {
      notify(NFY_FATAL, "east_boundary <= west_boundary!.\n");
    }
    khCutExtent::cut_extent = khExtents<double>(
        NSEWOrder,
        north_boundary, south_boundary, east_boundary, west_boundary);
    const bool to_cut = khCutExtent::cut_extent != khCutExtent::world_extent;
    gstBBox cut_box;
    if (to_cut) {
      khExtents<double> norm_cut_extent = khTilespaceBase::DegToNormExtents(
          khCutExtent::cut_extent);
      cut_box.init(norm_cut_extent.west(), norm_cut_extent.east(),
                   norm_cut_extent.south(), norm_cut_extent.north());
    }

    gstInit();

    if (!file_list_name.empty()) {
      try {
        file_list.AddFromFilelist(file_list_name);
      } catch(const std::exception &e) {
        notify(NFY_FATAL, "%s", e.what());
      } catch(...) {
        notify(NFY_FATAL, "Unknown error");
      }
    }

    if (file_list.size() == 0) {
      usage("Source list is empty, nothing to do!");
    }

    // Print the input file sizes for diagnostic log file info.
    std::vector<std::string> input_files;
    for (unsigned int i = 0; i < file_list.size(); ++i) {
      input_files.push_back(file_list[i]);
    }
    khPrintFileSizes("Input File Sizes", input_files);

    JOBSTATS_BEGIN(import_stats, VALIDATE);    // validate
    if (!validateSources(file_list, layer))
      exit(EXIT_FAILURE);
    JOBSTATS_END(import_stats, VALIDATE);

    if (validate_only)
      exit(EXIT_SUCCESS);

    std::string path = khEnsureExtension(output, ".kvp");

    if (!khMakeCleanDir(path)) {
      usage("Unable to create empty directory");
    }
    QDir outdir(path.c_str());

    if (force_2d) {
      notify(NFY_NOTICE, "*** Command run with --force2D");
      if (google_internal) {
        notify(NFY_NOTICE, "    All feature types of %s & %s will"
               " be converted to %s.",
               PrettyPrimType(gstPolygon25D).c_str(),
               PrettyPrimType(gstPolygon3D).c_str(),
               PrettyPrimType(gstPolygon).c_str());
      } else {
        notify(NFY_NOTICE, "    All feature types of %s will"
               " be converted to %s.",
               PrettyPrimType(gstPolygon25D).c_str(),
               PrettyPrimType(gstPolygon).c_str());
      }
    }

    if (force_25d) {
      notify(NFY_NOTICE, "*** Command run with --force25D");
      if (google_internal) {
        notify(NFY_NOTICE,
               "    All feature types of %s & %s will be converted to %s.",
               PrettyPrimType(gstPolygon).c_str(),
               PrettyPrimType(gstPolygon3D).c_str(),
               PrettyPrimType(gstPolygon25D).c_str());
      } else {
        notify(NFY_NOTICE,
               "    All feature types of %s will be converted to %s.",
               PrettyPrimType(gstPolygon).c_str(),
               PrettyPrimType(gstPolygon25D).c_str());
      }
    }

    if (force_3d) {
      notify(NFY_NOTICE, "*** Command run with --force3D");
      notify(NFY_NOTICE,
             "    All feature types of %s will be converted to %s.",
             PrettyPrimType(gstPolygon25D).c_str(),
             PrettyPrimType(gstPolygon3D).c_str());
    }

    // *************************************************************************
    // loop over entire collection now and convert each source
    // file to the Keyhole Vector Product format (KVP)

    gstKVPAsset kvpasset;
    kvpasset.name = outdir.path().latin1();

    // pick a grid size that would be a too big for a single building
    // for reference, all of San Francisco could fit in a grid at level 11
    const double max25d_area = Grid::Step(12) * Grid::Step(12);
    int show_max25d_message = 0;
    double max_feature_diameter_of_layer = .0;
    double min_feature_diameter_of_layer = 1.0;
    double average_feature_diameter_of_layer = .0;

    JOBSTATS_BEGIN(import_stats, IMPORT);
    notify(NFY_NOTICE, "Begin source import phase.");
    fusion_gst::GeometryChecker geometry_checker;
    fusion_gst::PolygonCleaner polygon_cleaner;
    bool have_header = false;
    bool have_attr = false;
    gstHeaderHandle hdr;
    int sub = 0;
    int width = static_cast<int>(log10(file_list.size())) + 1;
    std::map<gstPrimType, int> prim_type_counts;
    for (std::vector<std::string>::const_iterator it = file_list.begin();
         it != file_list.end(); ++it, ++sub) {
      notify(NFY_NOTICE, "Opening source %s", (*it).c_str());

      JOBSTATS_BEGIN(import_stats, OPENSRC);
      gstSource src((*it).c_str());
      if (src.Open() != GST_OKAY)
        notify(NFY_FATAL, "Failed to open source %s", (*it).c_str());
      JOBSTATS_END(import_stats, OPENSRC);

      if (!codec_str.isEmpty() && !src.SetCodec(codec_str)) {
        notify(NFY_FATAL, "Failed to apply codec %s", codec_str.latin1());
      }

      // take header from first source only
      if (!have_header) {
        have_header = true;
        hdr = src.GetAttrDefs(layer);
        if (hdr->numColumns() != 0) {
          have_attr = true;
          for (unsigned int r = 0; r < hdr->numColumns(); ++r) {
            kvpasset.header.push_back(gstKVPAsset::Record(hdr->Name(r),
                                                          hdr->ftype(r)));
          }
        }
        // and compare against every other sources header
      } else if (have_attr) {
        if (*src.GetAttrDefs(layer) != *hdr) {
          notify(NFY_FATAL, "Headers do not match!");
        }
      }
      if (to_cut && !cut_box.Intersect(src.BoundingBox(layer))) {
        continue;
      }

      // set the primType of the asset from the layer defs
      kvpasset.primType = src.GetPrimType(0);

      //
      // prepare output files
      //

      JOBSTATS_BEGIN(import_stats, OPENDST);

      char subname[width + 8];
      snprintf(subname, (width + 8), "subpart%0*d", width, sub);
      gstFileInfo dst(outdir.path().latin1(), subname, "kvgeom");

      if (dst.status() == GST_OKAY)
        notify(NFY_FATAL, "Output file already exists. Please remove: %s",
               dst.name());

      gstKVPFile kvp(dst.name());

      if (kvp.OpenForWrite() != GST_OKAY)
        notify(NFY_FATAL, "Unable to open output file for writing: %s",
               dst.name());

      gstFileInfo kdbinfo(dst);
      kdbinfo.SetExtension("kvattr");
      gstKVPTable kdb(kdbinfo.name());
      if (have_attr) {
        if (kdb.Open(GST_WRITEONLY) != GST_OKAY) {
          notify(NFY_FATAL, "Unable to open output file for writing: %s",
                 kdbinfo.name());
        }
        kdb.SetHeader(hdr);
      }

      JOBSTATS_END(import_stats, OPENDST);

      //
      // copy layer
      //
      int featurecount = 0;
      int vertexCount = 0;
      int collinearCount = 0;

      FeatureStatisticsCalculator statistic_calc;

      for (src.ResetReadingOrThrow(layer);
           !src.IsReadingDone();
           src.IncrementReadingOrThrow()) {
        gstGeodeHandle geode;
        gstRecordHandle rec;
        try {
          {
            JOBSTATS_SCOPED(import_stats, GETGEODE);
            geode = src.GetNormCurrFeatureOrThrow();
          }
          if (have_attr) {
            JOBSTATS_SCOPED(import_stats, GETRECORD);
            rec = src.GetCurrentAttributeOrThrow();
          }
        } catch(const khSoftException &e) {
          if (ignore_bad_features) {
            notify(NFY_WARN, "%s", e.what());
            continue;
          } else {
            throw;
          }
        }

        JOBSTATS_BEGIN(import_stats, PUTGEODE);
        if (force_2d) {
          if  (geode->PrimType() == gstPolygon25D ||
               geode->PrimType() == gstPolygon3D ||
               geode->PrimType() == gstMultiPolygon25D ||
               geode->PrimType() == gstMultiPolygon3D) {
            geode->ChangePrimType(gstPolygon);
          }
        } else if (force_25d) {
          if  (geode->PrimType() == gstPolygon ||
               geode->PrimType() == gstPolygon3D ||
               geode->PrimType() == gstMultiPolygon ||
               geode->PrimType() == gstMultiPolygon3D) {
            geode->ChangePrimType(gstPolygon25D);
          }
        } else if (force_3d) {
          if (geode->PrimType() == gstPolygon25D ||
              geode->PrimType() == gstMultiPolygon25D) {
            geode->ChangePrimType(gstPolygon3D);
          }
        }

        if (!do_not_fix_invalid_geometries) {
          // Detect and Fix invalid geometries in source: coincident points,
          // spikes.
          geometry_checker.Run(&geode);
          if (geode->IsEmpty()) {
            notify(NFY_WARN, "Found a degenerate geometry in src %d layer %d"
                   " feature %d (type %s): skipped.",
                   sub, layer, featurecount,
                   PrettyPrimType(geode->PrimType()).c_str());
            continue;
          }

          // Detect and remove completely overlapped edges, fix
          // cycle orientation (counterclockwise for outer cycles,
          // clockwise for inner cycles).
          polygon_cleaner.Run(&geode);
          if (geode->IsEmpty()) {
            notify(NFY_WARN, "Found a degenerate geometry in src %d layer %d"
                   " feature %d (type %s): skipped.",
                   sub, layer, featurecount,
                   PrettyPrimType(geode->PrimType()).c_str());
            continue;
          }
        }

        // Polygon25D should only be used for extruded buildings.
        // Do special pre-processing for gstPolygon25D.
        if (geode->PrimType() == gstPolygon25D ||
            geode->PrimType() == gstMultiPolygon25D ) {
          int cur_vertex_count = geode->TotalVertexCount();
          int cur_collinear_count = geode->RemoveCollinearVerts();

          if (geode->IsDegenerate()) {
            notify(NFY_WARN, "Found a degenerate geometry in src %d layer %d"
                   " feature %d (type %s): skipped.",
                   sub, layer, featurecount,
                   PrettyPrimType(geode->PrimType()).c_str());
            continue;
          }
          vertexCount += cur_vertex_count;
          collinearCount += cur_collinear_count;

          // Emit a warning if any Polygon25D feature is unreasonably large
          // this would suggest that the data has been classified incorrectly.
          if (geode->PrimType() == gstPolygon25D) {
            if (VerifyPolygon25DByArea(geode, featurecount, max25d_area)) {
              ++show_max25d_message;
            }
          } else {  // gstMultiPolygon25D
            assert(geode->PrimType() == gstMultiPolygon25D);
            const gstGeodeCollection *multi_geode =
                static_cast<const gstGeodeCollection*>(&(*geode));
            for (unsigned int p = 0; p < multi_geode->NumParts(); ++p) {
              const gstGeodeHandle geode_parth = multi_geode->GetGeode(p);
              if (VerifyPolygon25DByArea(geode_parth,
                                         featurecount, max25d_area)) {
                ++show_max25d_message;
                break;  // it's enough to get one incorrect polygon in
                // collection to take a decision about incorrectness
                // of feature.
              }
            }
          }
        }

        // apply necessary scaling to convert to meters
        if (scale != 1)
          geode->ApplyScaleFactor(scale);

        if (to_cut && !cut_box.Intersect(geode->BoundingBox())) {
          continue;
        }

        if (kvp.AddGeode(geode) != GST_OKAY)
          notify(NFY_FATAL, "Unable to add geode %d", featurecount);

        JOBSTATS_END(import_stats, PUTGEODE);

        if (have_attr) {
          if (codec_str.isEmpty()) {
            // only check if no codec was provided.  this makes sure that bit 8
            // is never set in any character of string fields
            if (!rec->ValidateEncoding()) {
              notify(NFY_FATAL, "Found an invalid character in src %d layer "
                     "%d feature %d. Please specify a character encoding.",
                     sub, layer, featurecount);
            }
          }

          JOBSTATS_BEGIN(import_stats, PUTRECORD);
          if (kdb.AddRecord(rec) != GST_OKAY) {
            notify(NFY_FATAL, "Invalid characters found in record.\n"
                   "Possibly the wrong character encoding was specified.");
          }
          JOBSTATS_END(import_stats, PUTRECORD);
        }

        prim_type_counts[geode->PrimType()]++;

        ++featurecount;

        // Gather features statistics for the source.
        statistic_calc.Accept(geode);
      }

      if (collinearCount)
        notify(NFY_NOTICE, "%d / %d collinear vertices eliminated",
               collinearCount, vertexCount);


      //
      // add source descriptor
      //

      gstKVPAsset::Source sa;
      sa.file = dst.fileName();
      //  sa.featureCount = src.numFeatures(layer);
      sa.featureCount = featurecount;     // use actual feature count
      sa.xmin = src.BoundingBox(layer).w;
      sa.xmax = src.BoundingBox(layer).e;
      sa.ymin = src.BoundingBox(layer).s;
      sa.ymax = src.BoundingBox(layer).n;
      sa.averageFeatureDiameter = statistic_calc.AverageDiameter();

      kvpasset.sources.push_back(sa);

      JOBSTATS_BEGIN(import_stats, SAVE);
      // Save after every source is added just in case something fails.
      kvpasset.Save(outdir.filePath(kHeaderXmlFile.c_str()).latin1());
      JOBSTATS_END(import_stats, SAVE);

      // Compute features statistics for layer.
      if (max_feature_diameter_of_layer < statistic_calc.MaxDiameter()) {
        max_feature_diameter_of_layer = statistic_calc.MaxDiameter();
      }
      if (min_feature_diameter_of_layer > statistic_calc.MinDiameter()) {
        min_feature_diameter_of_layer = statistic_calc.MinDiameter();
      }
      average_feature_diameter_of_layer += statistic_calc.AverageDiameter();
    }

    for (std::map<gstPrimType, int>::iterator it = prim_type_counts.begin();
         it != prim_type_counts.end(); ++it) {
      notify(NFY_NOTICE, "Imported %d features with type %s",
             it->second, PrettyPrimType(it->first).c_str());
    }

    average_feature_diameter_of_layer /= file_list.size();

    // Display feature statistics. For some feature types (e.g. gstPoint)
    // the statistic data are not computed. So we check
    // average_feature_diameter for 0 to skip feature statistics displaying.
    if (.0 != average_feature_diameter_of_layer) {
      notify(NFY_VERBOSE, "Min feature diameter (pixels at level 0): %.10f",
             (min_feature_diameter_of_layer *
              ClientVectorTilespace.pixelsAtLevel0));
      notify(NFY_VERBOSE, "Max feature diameter (pixels at level 0): %.10f",
             (max_feature_diameter_of_layer *
              ClientVectorTilespace.pixelsAtLevel0));
      notify(NFY_VERBOSE, "Average feature diameter (pixels at level 0): %.10f",
             (average_feature_diameter_of_layer *
              ClientVectorTilespace.pixelsAtLevel0));

      // Calculate Min/Efficient/Max Resolution Levels.
      // At Min Resolution Level the max feature of layer has diameter 1/8 of
      // tile size.
      kvpasset.minResolutionLevel =
          EfficientLOD(max_feature_diameter_of_layer,
                       ClientVectorTilespace,
                       ClientVectorTilespace.pixelsAtLevel0 / 8.0);

      // At Max Resolution Level the min feature of layer has diameter 1/8 of
      // tile size.
      kvpasset.maxResolutionLevel =
          EfficientLOD(min_feature_diameter_of_layer,
                       ClientVectorTilespace,
                       ClientVectorTilespace.pixelsAtLevel0 / 8.0);

      // At Efficient Resolution Level the average feature of layer has
      // diameter 1/8 of tile size.
      kvpasset.efficientResolutionLevel =
          EfficientLOD(average_feature_diameter_of_layer,
                       ClientVectorTilespace,
                       ClientVectorTilespace.pixelsAtLevel0 / 8.0);

      notify(NFY_NOTICE,
             "Recommended levels for vector project build:");

      notify(NFY_NOTICE, "Min Resolution Level"
             " (max feature has diameter 1/8 of tile): %u",
             kvpasset.minResolutionLevel);

      notify(NFY_NOTICE, "Max Resolution Level"
             " (min feature has diameter 1/8 of tile): %u",
             kvpasset.maxResolutionLevel);

      notify(NFY_NOTICE, "Efficient Resolution Level"
             " (average feature has diameter 1/8 of tile): %u",
             kvpasset.efficientResolutionLevel);

      notify(NFY_NOTICE, "Efficient Resolution Level can be used as Min/Max"
             " Resolution Level for data sets with big variance of feature "
             " diameters distribution.");
    }

    JOBSTATS_BEGIN(import_stats, SAVE);
    // Save after setting min/max/efficient resolution levels.
    kvpasset.Save(outdir.filePath(kHeaderXmlFile.c_str()).latin1());
    JOBSTATS_END(import_stats, SAVE);

    if (show_max25d_message) {
      notify(NFY_WARN, "Found %d very large features classified as %s",
             show_max25d_message,
             PrettyPrimType(gstPolygon25D).c_str());
      notify(NFY_WARN, "  Possibly the source data is classified incorrectly");
      notify(NFY_WARN, "  Consider importing with --force2D");
    }

    JOBSTATS_END(import_stats, IMPORT);
    JOBSTATS_DUMPALL();

    // On successful completion, print out the output file sizes.
    std::vector<std::string> output_files;
    output_files.push_back(output);
    khPrintFileSizes("Output File Sizes", output_files);
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "Error during import:\n%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Error during import:\n%s", "Unknown error");
  }

  return 0;
}

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


#include <sys/stat.h>
#include <khGetopt.h>
#include <notify.h>
#include <autoingest/.idl/storage/VectorPOIAssetConfig.h>
#include <gstSource.h>
#include <merge/merge.h>
#include <khException.h>
#include <khStringUtils.h>
#include <sstream>

// 10 million records, about 1 gigabyte
const std::uint32_t MAX_RECORDS_PER_POI_SEGMENT = 10 * 1000 * 1000;

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr, "\n"
      "usage: %s [options]\n"
      " Supported options are:\n"
      "   --output  <filename>   File to write [Required]\n"
      "   --product <filename>   Vector product to use [Required]\n"
      "   --config <filename>    Configuration file to use [Required]\n",
      progn.c_str());
  exit(1);
}

// ****************************************************************************
// ***  Forward declarations
// ****************************************************************************
void EmitSearchFileData(FILE *search_file_, const std::string& fileName);
void EmitSearchFileHeader(FILE *search_file_, const POIConfig &config);
void EmitSearchFileFooter(FILE *search_file_, const POIConfig &config);
void EmitSearchRecord(gstSource &src,
                      std::int32_t fid,
                      FILE *search_file_,
                      const std::vector<SearchField> &search_fields);

// ****************************************************************************
// ***  MergeSource to pull from Query file
// ****************************************************************************
class QueryMergeSource: public MergeSource<std::int32_t> {
  typedef std::deque<std::int32_t> Container;
  typedef Container::const_iterator Iterator;
 public:
  QueryMergeSource(const std::string &debug_desc, const std::string &queryfile)
      : MergeSource<std::int32_t>(debug_desc) {
    // open the query file
    FILE* select_fp = ::fopen(queryfile.c_str(), "r");
    if (select_fp == NULL) {
      throw khErrnoException(
          kh::tr("Unable to open query results file %1").arg(queryfile.c_str()));
    }
    khFILECloser closer(select_fp);

    // skip header (if it exists)
    {
      double xmin, xmax, ymin, ymax;
      (void) fscanf(select_fp, "EXTENTS: %lf, %lf, %lf, %lf\n", &xmin, &xmax,
                    &ymin, &ymax);
    }

    // slurp the feature ids
    int val;
    while (!feof(select_fp)) {
      fscanf(select_fp, "%d\n", &val);
      ids.push_back(val);
    }

    // make sure we're not empty
    if (ids.size() == 0) {
      throw khException(
          kh::tr("Query results file %1 is empty").arg(queryfile.c_str()));
    }

    // initialize the iterator
    current = ids.begin();
  }
  virtual const std::int32_t &Current(void) const {
    if (current != ids.end()) {
      return *current;
    } else {
      throw khException(
          kh::tr("No current element for merge source %1").arg(this->name().c_str()));
    }
  }
  virtual bool Advance(void) {
    if (current != ids.end()) {
      ++current;
      return current != ids.end();
    }
    return false;
  }
  virtual void Close(void) {
    // nothing to do
  }

 private:
  Container ids;
  Iterator current;

  DISALLOW_COPY_AND_ASSIGN(QueryMergeSource);
};

int main(int argc, char *argv[]) {
  try {
    std::string progname = argv[0];

    // process commandline options
    int argn;
    bool help = false;
    std::vector<std::string> output;
    std::vector<std::string> productfile;
    std::vector<std::string> configfile;
    khGetopt options;
    options.helpOpt(help);
    options.vecOpt("output", output);
    options.vecOpt("product", productfile, &khGetopt::FileExists);
    options.vecOpt("config", configfile, &khGetopt::FileExists);
    options.setRequired("output", "product", "config");
    if (!options.processAll(argc, argv, argn) || help) {
      usage(progname);
    }
    if (!(output.size() == productfile.size() &&
        output.size() == configfile.size())) {
      usage(progname);
    }

    gstInit();

    for (std::uint32_t i = 0; i < output.size(); ++i) {

      // load and validate the POIConfig
      POIConfig poi_config;
      if (!poi_config.Load(configfile[i])) {
        // more specific message already emitted
        notify(NFY_FATAL, "Unable to proceed\n");
      }
      if (poi_config.search_fields_.empty()) {
        notify(NFY_FATAL, "No search fields defined in config");
      }
      if (poi_config.query_files_.empty()) {
        notify(NFY_FATAL, "No query files found in config");
      }

      // load the VectorProduct
      gstSource src(productfile[i].c_str());
      if (src.Open() != GST_OKAY) {
        notify(NFY_FATAL, "Unable to open source product %s",
               productfile[i].c_str());
      }

      // create the output file
      FILE *search_file_ = fopen(output[i].c_str(), "w");
      if (!search_file_) {
        throw khErrnoException(
            kh::tr("Unable to open %1 for writing").arg(output[i].c_str()));
      }
      EmitSearchFileHeader(search_file_, poi_config);

      // make a merger with all of the query files
      Merge <std::int32_t> merger("Query Merger");
      for (unsigned int f = 0; f < poi_config.query_files_.size(); ++f) {
        merger.AddSource(
            TransferOwnership(new QueryMergeSource(
                "Query Source: " + poi_config.query_files_[f],
                poi_config.query_files_[f])));
      }

      // traverse and process them all
      merger.Start();
      std::uint32_t record_count = MAX_RECORDS_PER_POI_SEGMENT;
      std::int32_t prev_id = -1;
      std::uint32_t data_file_num = 0;
      std::string data_file_name = output[i] + Itoa(data_file_num);
      FILE * data_file_ = fopen(data_file_name.c_str(), "w");
      if (!data_file_) {
        throw khErrnoException(
            kh::tr("Unable to open %1 for writing").arg(
                   data_file_name.c_str()));
      }
      do {
        std::int32_t cur_id = merger.Current();
        if (cur_id != prev_id) {
          prev_id = cur_id;

          // emit this record
          EmitSearchRecord(src, cur_id, data_file_, poi_config.search_fields_);

          // if max records is reached, output the name of the data file to the
          // search file and create a new data file for emitting.
          if (!--record_count) {
            if (fclose(data_file_) != 0) {
              throw khErrnoException(
                kh::tr("Error closing data file %1").arg(data_file_name.c_str()));
            }

            EmitSearchFileData(search_file_, data_file_name);

            // create next point data file for output
            data_file_name = output[i] + Itoa(++data_file_num);
            data_file_ = fopen(data_file_name.c_str(), "w");
            if (!data_file_) {
              throw khErrnoException(
                  kh::tr("Unable to open %1 for writing").arg(
                         data_file_name.c_str()));
            }
            record_count = MAX_RECORDS_PER_POI_SEGMENT;
          }
        }
      } while (merger.Advance());
      merger.Close();
      // close the output file
      if (fclose(data_file_) != 0) {
        throw khErrnoException(
          kh::tr("Error closing data file %1").arg(data_file_name.c_str()));
      }
      // if there are records in the file, emit file name.
      if (record_count < MAX_RECORDS_PER_POI_SEGMENT) {
        EmitSearchFileData(search_file_, data_file_name);
      } else {
        remove(data_file_name.c_str());
      }

      EmitSearchFileFooter(search_file_, poi_config);
      if (fclose(search_file_) != 0) {
        throw khErrnoException(kh::tr("Error closing %1").arg(output[i].c_str()));
      }
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}

void EmitSearchFileData(FILE *search_file_, const std::string& fileName) {
  fprintf(search_file_, "<SearchDataFile>");
  fprintf(search_file_, "%s", fileName.c_str());
  fprintf(search_file_, "</SearchDataFile>\n");
}

void EmitSearchFileHeader(FILE *search_file_, const POIConfig &config) {
  fprintf(search_file_, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  fprintf(search_file_, "<POISearchFile>\n");
  if (0 && !config.search_style_.empty()) {
    fprintf(search_file_,
        "<Style><BalloonStyle><![CDATA[%s]]></BalloonStyle></Style>\n",
        config.search_style_.c_str());
  } else {
    fprintf(search_file_, "<Style></Style>\n");
  }
  fprintf(search_file_, "<SearchTableSchema>\n");
  fprintf(search_file_,
      "<field name=\"lon\" displayname=\"lon\" type=\"???\" use=\"???\"/>\n");
  fprintf(search_file_,
      "<field name=\"lat\" displayname=\"lat\" type=\"???\" use=\"???\"/>\n");
  for (unsigned int i = 0; i < config.search_fields_.size(); ++i) {
    fprintf(search_file_,
        "<field name=\"%s\" displayname=\"%s\" type=\"VARCHAR\" use=\"%s\"/>\n",
        QString("field%1").arg(i).latin1(),
        (const char*) config.search_fields_[i].name.utf8(),
        SearchField::UseTypeToSearchString(config.search_fields_[i].use_type));
  }
  fprintf(search_file_, "</SearchTableSchema>\n");
  fprintf(search_file_, "<SearchTableValues>\n");
}

void EmitSearchFileFooter(FILE *search_file_, const POIConfig &config) {
  fprintf(search_file_, "</SearchTableValues>\n");
  fprintf(search_file_, "</POISearchFile>\n");
}

// adapted from python source
char * BinasciiHexlify(double val, char * buffer) {
  char* argbuf = reinterpret_cast<char *>(&val);
  /* make hex version of string, taken from shamodule.c */
  int i, j;
  char c;
  for (i = j = 0; i < static_cast<int>(sizeof(double)); ++i) {
    c = (argbuf[i] >> 4) & 0xf;
    c = (c > 9) ? c + 'A' - 10 : c + '0';
    buffer[j++] = c;
    c = argbuf[i] & 0xf;
    c = (c > 9) ? c + 'A' - 10 : c + '0';
    buffer[j++] = c;
  }
  buffer[j] = 0;
  return buffer;
}

void EmitSearchRecord(gstSource &src, std::int32_t fid, FILE *search_file_,
                      const std::vector<SearchField> &search_fields) {
  // fetch the geometry and the record
  gstRecordHandle rec = src.GetAttributeOrThrow(0, fid);
  gstGeodeHandle geode = src.GetFeatureOrThrow(0, fid, false);

  // get the "location" of the feature
  if (geode->IsEmpty()) {
    throw khException(kh::tr("Feature %1 has no geometry").arg(fid));
  }
  gstVertex loc = geode->Center();
  // apparently Center can return an error as an invalid vertex
  if (loc == gstVertex()) {
    loc = gstVertex(geode->BoundingBox().CenterX(),
                    geode->BoundingBox().CenterY());
  }

  for (std::uint32_t i = 0; i < search_fields.size(); ++i) {
    QString fname = QString("field%1").arg(i);

    gstValue *val = rec->FieldByName(search_fields[i].name);
    if (!val) {
      throw khException(kh::tr("No field named %1").arg(search_fields[i].name));
    }
    fprintf(search_file_, "%s\t", (const char *) val->ValueAsUnicode().utf8());
  }

  char lon_buffer[static_cast<int>(sizeof(double) * 2 + 1)];
  char lat_buffer[static_cast<int>(sizeof(double) * 2 + 1)];
  // Convert coordinates to point data.
  // TODO(???): Might use geos lib for this.
  // Create geometry data in canonical form (Hex EWKB using SRID 4326)
  // 01 = geos type point
  // 01 = network order?
  // 000020E6100000 = SRID 4326 code
  // hex_lon_str
  // hex_lat_str
  // The formula below is from the original code.
  // Converts normalized versions to degrees.
  // Assumes:  0 <= loc.x < 1
  //        0.25 <= loc.y < 0.75
  fprintf(search_file_, "0101000020E6100000%s%s\n",
          BinasciiHexlify(loc.x * 360.0 - 180.0, lon_buffer),
          BinasciiHexlify(loc.y * 360.0 - 180.0, lat_buffer));
}

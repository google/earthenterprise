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

//
// Rewrites the dbroot for the portable server.
// Changes the search server and port in the dbroot
// and copies all icons  referenced in the dbroot
// into a given directory.

#include <khGetopt.h>
#include <string>
#include "rewritedbroot.h"
#include "common/khConstants.h"
#include "common/khFileUtils.h"
#include "common/khStringUtils.h"
#include "fusion/portableglobe/shared/serverreader.h"


void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "E.g. gerewritedbroot --source=http://bplinux "
          "--icon_directory=/tmp/icons --dbroot_file=/tmp/dbroot "
          "--kml_map_file=/tmp/kml_map\n"
          "\nusage: %s \\\n"
          "           --source=<source_string> \\\n"
          "           --icon_directory=<icon_directory_string> \\\n"
          "           --dbroot_file=<dbroot_file_string> \\\n"
          "           --search_service=<search_service_url_string> \\\n"
          "           --preserve_search_service \\\n"
          "           --kml_map_file=<kml_map_file_string> \\\n"
          "           --kml_server=<kml_server_string> \\\n"
          "           --kml_port=<kml_port_string> \\\n"
          "           --kml_url_path=<kml_url_path_string> \\\n"
          "           --use_ssl_for_kml=<use_ssl_for_kml_bool> \\\n"
          "           --preserve_kml_filenames \\\n"
          "           --disable_historical \\\n"
          "\n"
          " Reads dbroot and rewrites the search tabs so that they point\n"
          " to the given search server and port. Optionally creates a directory\n"
          " of all of the icons referred to by the dbroot. \n"
          " Optionally updates the host, path and/or filename to the \n"
          " KML Layer URLs in the dbRoot. \n"
          "\n"
          " Required:\n"
          "   --source:        Server URL or local dbroot file that should be rewritten.\n"
          "   --dbroot_file:   File where new dbroot should be stored.\n"
          " Options:\n"
          "   --data_type:     Type of data packet to cut (ALL, IMAGERY,\n"
          "                    TERRAIN, or VECTOR).\n"
          "                    Default is ALL.\n"
          "   --search_service: Url to search service. If none is provided\n"
          "                    then uses relative url for standard Portable\n"
          "                    search.\n"
          "   --preserve_search_service:\n" 
          "                    Keep original search service url.\n"
          "   --icon_directory: Directory where icons should be stored.\n"
          "   --kml_map_file:  Output file where map of source kml urls to local\n"
          "                    files will be stored.\n"
          "   --kml_server:    Server to be used for kml files in the\n"
          "                    dbroot.\n"
          "                    Default: localhost\n"
          "   --kml_port:      Port to be used for kml files in the\n"
          "                    dbroot.\n"
          "                    Default: 8888\n"
          "   --kml_url_path:  Path in new url to prefix kml file name.\n"
          "                    Default: kml\n"
          "   --use_ssl_for_kml:  \n"
          "                    Use https:// instead of http:// for\n"
          "                    accessing kml files.\n"
          "                    Default: false\n"
          "   --preserve_kml_filenames:\n"   
          "                    Keep the original filenames when\n"
          "                    replacing the host and path in KML layer URLs\n"
          "                    in the dbroot.  When unset, files are renamed\n"
          "                    sequentially from kml_dbroot_000.kml upwards.\n"
          "                    Default: false\n"
          "   --disable_historical:\n"    
          "                    Removes the reference to the server for  \n"
          "                    historical imagery.\n",
          
          progn.c_str());
  exit(1);
}


int main(int argc, char *argv[]) {
  std::string progname = argv[0];
  bool help = false;
  bool use_ssl_for_kml = false;
  bool preserve_kml_filenames = false;
  bool preserve_search_service = false;
  bool disable_historical = false;
  std::string source;
  std::string icon_directory;
  std::string dbroot_file;
  std::string kml_map_file;
  std::string data_type = kCutAllDataFlag;
  std::string search_service = "";
  std::string kml_server = "";
  std::string kml_port = "";
  std::string kml_url_path = "kml";

  khGetopt options;
  options.flagOpt("help", help);
  options.flagOpt("?", help);
  options.flagOpt("use_ssl_for_kml", use_ssl_for_kml);
  options.flagOpt("preserve_kml_filenames", preserve_kml_filenames);
  options.flagOpt("preserve_search_service", preserve_search_service);
  options.flagOpt("disable_historical", disable_historical);
  options.opt("source", source);
  options.opt("icon_directory", icon_directory);
  options.opt("dbroot_file", dbroot_file);
  options.opt("kml_map_file", kml_map_file);
  options.opt("data_type", data_type);
  options.opt("search_service", search_service);
  options.opt("kml_server", kml_server);
  options.opt("kml_port", kml_port);
  options.opt("kml_url_path", kml_url_path);
  options.setRequired( "source",
                      "dbroot_file" );

  
  int argn;
  if (!options.processAll(argc, argv, argn)
      || help
      || argn != argc) {
    usage(progname);
  }

  // Make sure data_type is legal.
  data_type = UpperCaseString(data_type);
  if ((data_type != kCutAllDataFlag) &&
      (data_type != kCutImageryDataFlag) &&
      (data_type != kCutTerrainDataFlag) &&
      (data_type != kCutVectorDataFlag)) {
    usage(progname, "** ERROR ** Bad data_type flag.");
  }

  // Create temp directory for doing the work
  std::string temp_dir = khDirname(dbroot_file) + "/rewrite_dbroot";
  std::string dbroot_path;
  
  // Parameters for URL Parsing, to determine if source is a URL or a File. 
  std::string protocol;
  std::string host;
  std::string path_query;
  bool success = RelaxedUrlSplitter(source, &protocol, &host, &path_query);

  if ( success && ( protocol == "http" || protocol == "https")) {
    dbroot_path = temp_dir + "/" + kBinaryDbrootPrefix;
    khEnsureParentDir(dbroot_path);

    // Fetch the live dbroot from the real server
    if ( !ServerReader::SaveServerData(dbroot_path,
                                    source,
                                    "/dbRoot.v5?output=proto&hl=en&gl=us")) {
      notify(NFY_FATAL, "Unable to get dbroot from source.");
    }
  } else {
    dbroot_path = source;
  } 

  try {
    // load the dbroot we just fetched from the server
    geProtoDbroot dbroot(dbroot_path, geProtoDbroot::kEncodedFormat);

    // Remove reference to historical imagery server
    if (disable_historical) {
      DisableTimeMachine(&dbroot);
    }

    // Remove layers of unused data types.
    RemoveUnusedLayers(data_type, &dbroot);

    // modify base_url fields in search tabs
    if (!preserve_search_service) {
      ReplaceSearchServer(search_service, &dbroot);
    }

    // modify kml_url fields in layers
    std::string new_kml_base_url;
    if (kml_server.empty()) {
      new_kml_base_url = kml_url_path;
    } else {
      new_kml_base_url = ComposeUrl(use_ssl_for_kml, kml_server,
                                    kml_port, kml_url_path);
    }
    ReplaceReferencedKml(new_kml_base_url, kml_map_file, preserve_kml_filenames, &dbroot);

    // fetch all icons from real server and save them locally
    if (!icon_directory.empty()) {
      SaveIcons(icon_directory, source, &dbroot);
    }

    // write a local copy of our modified dbroot
    dbroot.Write(dbroot_file, geProtoDbroot::kEncodedFormat);
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}

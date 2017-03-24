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
// Takes a map file of source urls to local files. Grabs the kml using
// the source url and puts it into the local file. If the kml contains
// links to other kml files, these can be added to the files to be
// processed. This allows all of the kml files and references to them
// to recursively be converted to local files.

#include <khGetopt.h>
#include <fstream>   // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <map>
#include <set>
#include <string>
#include "common/khConstants.h"
#include "common/khFileUtils.h"
#include "common/khStringUtils.h"
#include "fusion/gst/gstEarthStream.h"
#include "fusion/portableglobe/shared/serverreader.h"


// TODO: Move code to separate file with tests.

/**
 * Read file from source url and store it as a local kml file.
 *
 * @param src_url Source url of the file.
 * @param kml_file Path to local file where content should be stored.
 */
void CopyToLocalFile(const std::string& src_url,
                     const std::string& local_file) {
  if (!ServerReader::SaveServerData(local_file, src_url)) {
    notify(NFY_FATAL, "Unable to load file: %s", src_url.c_str());
  }
}

/**
 * Read in local kml file and see if it has any links to other kml files.
 * If it does, add those files to list of kml source urls to be processed.
 * Also add urls and unique local file names to the map.
 *
 * @param src_url Source url of the kml file.
 * @param kml_url_to_file_map Map of source url to local file path.
 * @param kml_to_load Set of source urls remaining to be loaded.
 * @param num_linked_kml_files Number of linked kml files found so far.
 * @param kml_base_url New base url for accessing the local kml file.
 * @param kml_directory Directory where local kml files are stored.
 * @param ignore_absolute_urls Flag indicating absolute urls should not
 *                             be altered.
 */
void ProcessKmlFile(const std::string src_url,
                    std::map<std::string, std::string>* kml_url_to_file_map,
                    std::set<std::string>* kml_to_load,
                    int* num_linked_files,
                    std::string kml_base_url,
                    std::string kml_directory,
                    bool ignore_absolute_urls) {
  std::string kml_file = (*kml_url_to_file_map)[src_url];
  // Read kml file into string.
  std::string kml_data;
  khReadStringFromFile(kml_file, kml_data, 0);

  // Get the base url, server, and path from the source url.
  // These are needed if links to other kml files are
  // relative paths.
  const std::string old_base_url = khDirname(src_url);
  std::string old_base_server;
  std::string old_base_url_path;
  ServerReader::ExtractServerAndPath(old_base_url,
                                     &old_base_server,
                                     &old_base_url_path);

  // Start file to rewrite the local kml with new urls for links to
  // other kml files.
  std::ofstream fout;
  fout.open(kml_file.c_str(), std::ios::out);
  if (!fout) {
    notify(NFY_FATAL, "Unable to write local kml file: %s",
           kml_file.c_str());
  }

  // Replace href references with links to local files.
  int start_idx = 0;
  for (int end_idx = kml_data.find("<href>");
       end_idx >= 0;
       end_idx = kml_data.find("<href>", end_idx)) {
    end_idx += 6;
    int new_end_idx = kml_data.find("</href>", end_idx);
    if (new_end_idx < 0) {
      notify(NFY_FATAL, "Did not find closing href tag.");
    }

    std::string file_path = kml_data.substr(end_idx,
                                           new_end_idx - end_idx);
    // Check for "http://server.com/..." or "https://server.com/...".
    int http_idx = file_path.find("://");
    std::string new_src_url = "";
    // Check for absolute url (includes server address).
    if (http_idx >= 0) {
      if (!ignore_absolute_urls) {
        new_src_url = file_path;
      }
      notify(NFY_DEBUG, "Found NetworkLink absolute url: %s -> %s",
             file_path.c_str(), new_src_url.c_str());
    // Check for absolute path.
    } else  if (file_path[0] == '/') {
      new_src_url = old_base_server + file_path;
      notify(NFY_DEBUG, "Found NetworkLink absolute path: %s -> %s",
             file_path.c_str(), new_src_url.c_str());
    // Assume relative path.
    } else {
      new_src_url = old_base_server + old_base_url_path + "/" + file_path;
      notify(NFY_DEBUG, "Found NetworkLink relative path: %s -> %s",
             file_path.c_str(), new_src_url.c_str());
    }

    // If we found a kml url to change, change it here and add to list
    // of files to process.
    if (!new_src_url.empty()) {
      std::string new_file_url;
      std::string suffix = new_src_url.substr(new_src_url.size() - 3);

      // If new kml file, add it to list of files needing processing.
      if (kml_url_to_file_map->find(new_src_url) ==
          kml_url_to_file_map->end()) {
        char str[20];
        snprintf(str, sizeof(str),
                 kLinkedFileNameTemplate.c_str(),
                 *num_linked_files, suffix.c_str());
        *num_linked_files += 1;
        // If no base url, make sure that it remains relative with
        // no leading slash.
        if (kml_base_url.empty()) {
          new_file_url = str;
        } else {
          new_file_url = kml_base_url + "/" + str;
        }
        std::string local_file_path = kml_directory + "/" + str;

        (*kml_url_to_file_map)[new_src_url] = local_file_path;
        kml_to_load->insert(new_src_url);

      // If not new, then we have a cycle in the kml links.
      } else {
        if (suffix == "kml") {
          notify(NFY_WARN, "Cycle detected in kml references.");
        }

        std::string local_file_path = (*kml_url_to_file_map)[new_src_url];
        // If no base url, make sure that it remains relative with
        // no leading slash.
        if (kml_base_url.empty()) {
          new_file_url = local_file_path.substr(kml_directory.size() + 1);
        } else {
          new_file_url = kml_base_url + "/" +
                         local_file_path.substr(kml_directory.size() + 1);
        }
      }

      // Write the new kml url into the <Link> element.
      fout.write(kml_data.substr(start_idx, end_idx - start_idx).c_str(),
                 end_idx - start_idx);
      fout.write(new_file_url.c_str(), new_file_url.size());

      start_idx = new_end_idx;
    }

    end_idx = new_end_idx;
  }

  fout.write(kml_data.substr(start_idx).c_str(), kml_data.size() - start_idx);
  fout.close();
}

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "E.g. gekmlgrabber --kml_map_file=/tmp/kml_map"
          "\nusage: %s \\\n"
          "           --kml_map_file=<kml_map_file_string> \\\n"
          "           --output_directory=<output_directory_string> \\\n"
          "           --no_recurse \\\n"
          "\n"
          " Reads kml references and retrieves them from network\n"
          " and puts them into local files. If the no_recurse flag\n"
          " is not set, it does the same for any additional kml files\n"
          " referenced within the given kml files.\n"
          "\n"
          " If kml_server, kml_port, and kml_url_path are left unset,\n"
          " then paths are relative to referencing kml, which is\n"
          " generally preferred.\n"
          "\n"
          " Required:\n"
          "   --kml_map_file:  File where map of kml source urls to local\n"
          "                    files are stored.\n"
          "   --output_directory: Directory where local kml files are to\n"
          "                    be stored.\n"
          " Options:\n"
          "   --source:        Source for kml files.\n"
          "   --kml_server:    Server to be referenced in the\n"
          "                    dbroot for kml files.\n"
          "   --kml_port:      Port to be referenced in the\n"
          "                    dbroot for kml files.\n"
          "   --kml_url_path:  Path in new url to prefix kml file name.\n"
          "   --use_ssl_for_kml:  Require https:// instead of http:// for\n"
          "                    accessing kml files.\n"
          "                    Default: false\n"
          "   --no_recurse:    Do NOT make all kml files linked\n"
          "                    within the kml files local files as well.\n"
          "                    Default: false\n"
          "   --ignore_absolute_urls: If kml is linked with a full\n"
          "                    url address ('http://server/...'), leave\n"
          "                    it as it is.\n"
          "                    Default: false\n",
          progn.c_str());
  exit(1);
}

int main(int argc, char *argv[]) {
  std::string progname = argv[0];
  bool help = false;
  bool no_recurse = false;
  bool ignore_absolute_urls = false;
  bool use_ssl_for_kml = false;
  std::string kml_map_file;
  std::string kml_directory;
  std::string kml_server="";
  std::string source="localhost";
  std::string kml_port="";
  // Use empty string to preserve paths relative to referencing
  // kml. For example, if root kml is in /kml/root.kml and contains
  // a reference to /kml/link.kml, that reference should simply
  // be link.kml. This ensures that it works for Portable, where
  // the root url might be http://localhost:9335/kml/root.kml but
  // also in Earth Server where the root url might be
  // http://server.gov/target/kml/root.kml.
  std::string kml_url_path="";

  khGetopt options;
  options.flagOpt("help", help);
  options.flagOpt("?", help);
  options.flagOpt("no_recurse", no_recurse);
  options.flagOpt("ignore_absolute_urls", ignore_absolute_urls);
  options.flagOpt("use_ssl_for_kml", use_ssl_for_kml);
  options.opt("kml_map_file", kml_map_file);
  options.opt("output_directory", kml_directory);
  options.opt("source", source);
  options.opt("kml_server", kml_server);
  options.opt("kml_port", kml_port);
  options.opt("kml_url_path", kml_url_path);
  options.setRequired("kml_map_file", "output_directory");

  int argn;
  if (!options.processAll(argc, argv, argn)
      || help
      || argn != argc) {
    usage(progname);
    return 1;
  }

  // TODO: Replace with simple wrapper around curl.
  // Hack to use server reader for other servers.
  ServerReader::InitServer(source);

  // Create kml directory for putting local kml files in.
  khEnsureParentDir(kml_directory + "/dummy");
  std::ifstream fin;

  // Add each of the kml files from the map file, to the set
  // to be loaded and to the map mapping the source url
  // to the local file path.
  char line[256];
  std::set<std::string> kml_to_load;
  std::map<std::string, std::string> kml_url_to_file_map;
  fin.open(kml_map_file.c_str(), std::ios::in);
  if (!fin) {
    notify(NFY_FATAL, "Unable to open kml map file: %s",
           kml_map_file.c_str());
  }
  std::string kml_protocol;
  std::string kml_host;
  std::string kml_path;
  while (!fin.eof()) {
    fin.getline(line, sizeof(line));
    // If line isn't blank, process it.
    if (line[0] && (line[0] != '\r') && (line[0] != '\n')) {
      std::string line_str = line;
      int space_idx = line_str.find(" ");
      std::string kml_url = line_str.substr(0, space_idx);

      // kml_url may be a relative url, if so, prefix it with source.
      if (!UrlSplitter(kml_url, &kml_protocol, &kml_host, &kml_path)) {
        if (kml_url[0] == '/') {
          kml_url = source + kml_url;
        } else {
          kml_url = source + '/' + kml_url;
        }
      }

      std::string kml_file = kml_directory + "/" +
                             line_str.substr(space_idx + 1,
                                             line_str.size() - space_idx - 1);
      kml_to_load.insert(kml_url);
      kml_url_to_file_map[kml_url] = kml_file;
    }
  }

  // Process all files in the kml_to_load set.
  // If processing is recursive, then new kml files linked within the kml files
  // being processed can be added to the set.
  int num_linked_files = 0;
  std::string new_kml_base_url;
  if (kml_server.empty()) {
    new_kml_base_url = kml_url_path;
  } else {
    new_kml_base_url = ComposeUrl(use_ssl_for_kml, kml_server,
                                  kml_port, kml_url_path);
  }

  // We keep getting the iterator in the loop because items are being
  // added to and removed from the set. We are done when the set
  // is empty.
  for (std::set<std::string>::iterator iter = kml_to_load.begin();
       iter != kml_to_load.end();
       kml_to_load.erase(iter), iter = kml_to_load.begin()) {
    std::string local_file = kml_url_to_file_map[*iter];
    std::string suffix = local_file.substr(local_file.size() - 3);
    CopyToLocalFile(*iter, local_file);
    if (!no_recurse && (suffix == "kml")) {
      ProcessKmlFile(*iter, &kml_url_to_file_map, &kml_to_load,
                     &num_linked_files, new_kml_base_url, kml_directory,
                     ignore_absolute_urls);
    }
  }

  fin.close();
  return 0;
}

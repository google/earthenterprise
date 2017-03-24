/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Saves data from a server to the local file system.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_SERVERREADER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_SERVERREADER_H_

#include <string>
#include "fusion/gst/gstSimpleEarthStream.h"

class ServerReader {
 public:
  static gstSimpleEarthStream* gst_server;

  /**
   * Saves data read at url to the given path.
   *
   * @param path Path to file where data is to be stored.
   * @param server Address of server where data is to come from.
   * @param url Url to use on server to get the data.
   * @return the size of the data written to the file.
   */
  static size_t SaveServerData(std::string path,
                               std::string server,
                               std::string url);

  /**
   * Saves data read at url to the given path.
   *
   * @param file_path Path to file where data is to be stored.
   * @param full_url Full url with server to get the data.
   * @return the size of the data written to the file.
   */
  static size_t SaveServerData(std::string path,
                               std::string full_url);

  /**
   * Extract server and path from full url.
   *
   * @param full_url Full url with server to get the data.
   * @param server Server and port in the url.
   *                (includes prefix; e.g. "http://")
   * @param path Path after server and port including initial slash.
   */
  static void ExtractServerAndPath(std::string full_url,
                                   std::string* server,
                                   std::string* path);

  /**
   * Initialize the server before making other calls. Lets you grab
   * raw packets from other servers.
   *
   * @param server Address of fusion server.
   */
  static void InitServer(std::string server);
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_SERVERREADER_H_

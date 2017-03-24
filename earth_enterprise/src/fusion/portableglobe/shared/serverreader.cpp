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

// Saves data from a server to the local file system.

// TODO: Add unit tests here (and elsewhere) after
// TODO: creating gstSimpleEarthStream mock.

#include <fstream>   // NOLINT(readability/streams)
#include <string>
#include "common/khFileUtils.h"
#include "fusion/gst/gstSimpleEarthStream.h"
#include "fusion/portableglobe/shared/serverreader.h"
#include "common/gedbroot/eta_dbroot.h"
#include "common/gedbroot/eta2proto_dbroot.h"

gstSimpleEarthStream* ServerReader::gst_server = NULL;

/**
 * Saves data read at url to the given path.
 *
 * @param file_path Path to file where data is to be stored.
 * @param server Address of server where data is to come from.
 * @param url Url to use on server to get the data.
 * @return the size of the data written to the file.
 */
size_t ServerReader::SaveServerData(std::string file_path,
                                    std::string server, std::string url) {
  std::string raw_data;
  if (gst_server == NULL) {
    gst_server = new gstSimpleEarthStream(server);
  }

  std::string server_url = server + url;
  gst_server->GetRawPacket(server_url, &raw_data, false);

  // check whether it is an ETA dbroot
  if (EtaDbroot::IsEtaDbroot(raw_data, EtaDbroot::kExpectBinary)) {
    // This is a binary ETA dbroot, convert it to proto dbroot.
    geProtoDbroot proto_dbroot;
    gedbroot::geEta2ProtoDbroot eta2proto(&proto_dbroot);
    eta2proto.ConvertFromBinary(raw_data);

    // check whether the result is valid or not
    const bool EXPECT_EPOCH = true;
    if (!proto_dbroot.IsValid(EXPECT_EPOCH)) {
      return 0;
    }
    proto_dbroot.Write(file_path, geProtoDbroot::kEncodedFormat);

    return static_cast<size_t>(khGetFileSizeOrThrow(file_path));
  }

  // It is a proto dbroot, save data to given file.
  // TODO: Catch and rethrow IOErrors.
  std::ofstream fout;
  fout.open(file_path.c_str(), std::ios::out|std::ios::binary);
  fout.write(raw_data.c_str(), raw_data.size());
  fout.close();

  return raw_data.size();
}

/**
 * Saves data read at url to the given path.
 *
 * @param file_path Path to file where data is to be stored.
 * @param full_url Full url with server to get the data.
 * @return the size of the data written to the file.
 */
size_t ServerReader::SaveServerData(std::string file_path,
                                    std::string full_url) {
  std::string server;
  std::string path;
  ExtractServerAndPath(full_url, &server, &path);
  return SaveServerData(file_path, server, path);
}

/**
 * Extract server and path from full url.
 *
 * @param full_url Full url with server to get the data.
 * @param server Server and port in the url (including prefix; e.g. "http://").
 * @param path Path after server and port including initial slash.
 */
void ServerReader::ExtractServerAndPath(std::string full_url,
                                        std::string* server,
                                        std::string* path) {
  int idx = full_url.find("://");
  if (idx >= 0) {
    idx = full_url.find("/", idx + 3);
  } else {
    idx = full_url.find("/");
  }

  if (idx < 0) {
    *server = full_url;
    *path = "";
  } else {
    *server = full_url.substr(0, idx);
    *path = full_url.substr(idx);
  }
}

/**
 * Initialize the server before making other calls. Lets you grab
 * raw packets from other servers.
 *
 * @param server Address of fusion server.
 */
void ServerReader::InitServer(std::string server) {
  if (gst_server != NULL) {
    delete gst_server;
  }

  gst_server = new gstSimpleEarthStream(server);
}

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


#include "IndexManifest.h"
#include <khGuard.h>
#include <khFileUtils.h>
#include <khSimpleException.h>
#include <khEndian.h>
#include <third_party/rsa_md5/crc32.h>
#include <common/khConstants.h>

// IndexManifest
//
// Provide a complete listing of all files referenced through an index.
// Provide a method to obtain modified versions of header files with
// server path information inserted into headers.

namespace geindex {

void IndexManifest::GetManifest(geFilePool &filePool,
                                const std::string &index_path,
                                std::vector<ManifestEntry> &manifest,
                                const std::string& tmp_dir,
                                const std::string& disconnect_prefix,
                                const std::string& publish_prefix) {
  if (!khIsAbspath(index_path)) {
    throw khSimpleException("IndexManifest::GetManifest: '")
      << index_path << "' is not an absolute path.";
  }

  manifest.clear();

  // If it is a delta manifest, check that the index file exists.
  // Note: as for packet files below, we are trying to get index file
  // (unified.geindex) from either a disconnected db directory or
  // the publishroot.
  std::string actual_index_path = index_path;
  std::string actual_index_path_prefix = disconnect_prefix;
  if (!publish_prefix.empty() && !khExists(actual_index_path)) {
    // Note: If index file is in the publishroot for a delta
    // manifest, it means that index has not been changed from previous version.
    // In this case all packet files should also be in publishroot.
    // TODO: consider to implementing a special case below to get
    // all packetfiles from the publishroot when the index file is in
    // the publishroot - no need to check for packetfile existence in
    // a disconnected db directory!?

    if (publish_prefix != ".") {
      actual_index_path_prefix = publish_prefix;
      actual_index_path = actual_index_path_prefix +
          index_path.substr(disconnect_prefix.size());
    }

    if (!khExists(actual_index_path)) {
      throw khSimpleException(
          "IndexManifest::GetManifest: Insufficient delta manifest."
          " For example neither " )
          << index_path << ", nor " << actual_index_path
          << " exists. Try either using correct havepath/havepathfile in"
          " gedisconnectedsend or use a complete manifest by avoiding"
          " havepath/havepathfile.";
    }
  }

  // Note: the prefix is only used if we have hdr_orig_abs_path in index header
  // file, and it is an absolute path. See FileBundle::LoadHeaderFile().
  // Note: Considering that index bundle files should be located in
  // the same directory where index header file is, I believe we have to pass
  // an actual index path prefix to the reader.
  IndexBundleReader index_reader(
      filePool, actual_index_path, actual_index_path_prefix);
  // Get index.hdr and bundle.hdr for index file.
  index_reader.AppendManifest(manifest, tmp_dir);

  // Append manifests of all packetfile bundles.
  for (unsigned int p = 0 ; p < index_reader.PacketFileCount(); ++p) {
    std::string packetfile = index_reader.GetPacketFile(p);
    if (packetfile.empty()) {
      continue;
    }
    // If it is a delta manifest, check that the packetfile exists.
    std::string actual_packetfile_prefix = disconnect_prefix;
    if (!publish_prefix.empty() && !khExists(packetfile)) {
      const std::string orig_packet_file = packetfile;
      packetfile = packetfile.substr(disconnect_prefix.length());
      if (publish_prefix != ".") {
        actual_packetfile_prefix = publish_prefix;
        packetfile = publish_prefix + packetfile;
      }

      if (!khExists(packetfile)) {
        throw khSimpleException(
            "IndexManifest::GetManifest: Insufficient delta manifest."
            " For example neither " )
            << orig_packet_file << ", nor " << packetfile
            << " exists. Try either using correct havepath/havepathfile in"
            " gedisconnectedsend or use a complete manifest by avoiding"
            " havepath/havepathfile.";
      }
    }

    // Note: the prefix is only used if we have an
    // hdr_orig_abs_path in a packet header file, and it is an absolute
    // path. See FileBundle::LoadHeaderFile().
    // Note: Considering that packet bundle files should be located
    // in the same directory where a packet header file is, I believe we
    // have to pass an actual packetfile prefix to the reader.
    PacketFileReaderBase pf_reader(
        filePool, packetfile, actual_packetfile_prefix);
    pf_reader.AppendManifest(manifest, tmp_dir);
  }

  // As of 3.2 the Index manifest also may include the dated_imagery_channels
  // file for identifying historical imagery channels.
  std::string dated_imagery_channels_filename =
    khComposePath(index_reader.abs_path(), kDatedImageryChannelsFileName);
  if (khExists(dated_imagery_channels_filename)) {
    manifest.push_back(
      ManifestEntry(dated_imagery_channels_filename));
  }

  if (manifest.empty()) {
    throw khSimpleException("IndexManifest::GetAssetManifest: "
                            "empty manifest: ")
      << index_path;
  }
}

} // namespace geindex

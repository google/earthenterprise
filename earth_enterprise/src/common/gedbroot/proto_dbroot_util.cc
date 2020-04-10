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

#include "common/gedbroot/proto_dbroot_util.h"

#include <cassert>
#include "common/khGuard.h"
#include "common/khFileUtils.h"
#include "common/gedbroot/proto_dbroot.h"
#include "common/gedbroot/eta_dbroot.h"
#include "common/gedbroot/eta2proto_dbroot.h"
#include "common/khEndian.h"
#include "common/packetcompress.h"
#include "common/etencoder.h"

namespace gedbroot {

void CreateTimemachineDbroot(const std::string& dbroot_filename,
                             const unsigned int database_version,
                             const std::string& template_filename) {
  khDeleteGuard<geProtoDbroot> timemachine_dbroot;
  if (template_filename.length() > 0 && khExists(template_filename)) {
    // Copy from the template dbroot.
    timemachine_dbroot = TransferOwnership(new geProtoDbroot(
        template_filename, geProtoDbroot::kProtoFormat));
  } else {
    // Create a blank dbroot.
    timemachine_dbroot = TransferOwnership(new geProtoDbroot);

    // It must have proto imagery.
    timemachine_dbroot->set_proto_imagery(true);
  }

  // Add global config that everybody needs.
  timemachine_dbroot->AddUniversalFusionConfig(database_version);

  // Add timemachine specific settings.
  timemachine_dbroot->mutable_end_snippet()->mutable_time_machine_options()
      ->set_is_timemachine(true);
  timemachine_dbroot->mutable_end_snippet()->mutable_client_options()
      ->set_use_protobuf_quadtree_packets(true);

  // Write the new timemachine dbroot to disk.
  timemachine_dbroot->Write(dbroot_filename, geProtoDbroot::kEncodedFormat);
}

bool GetDbrootInfo(const std::string& content, bool *has_imagery,
                   bool *has_terrain, bool *is_proto_imagery) {
  geProtoDbroot dbroot;
  if (EtaDbroot::IsEtaDbroot(content, EtaDbroot::kExpectBinary)) {
    // Parse ETA dbroot.
    gedbroot::geEta2ProtoDbroot eta2proto(&dbroot);
    if (!eta2proto.ConvertFromBinary(content)) {
      return false;
    }
    assert(!dbroot.proto_imagery());
  } else {
    // Parse encrypted proto dbroot.
    keyhole::dbroot::EncryptedDbRootProto encrypted;
    if (!encrypted.ParseFromString(content)) {
      return false;
    }

    // Decode in place in the proto buffer.
    etEncoder::Decode(&(*encrypted.mutable_dbroot_data())[0],
                      encrypted.dbroot_data().size(),
                      encrypted.encryption_data().data(),
                      encrypted.encryption_data().size());

    // Uncompress.
    LittleEndianReadBuffer uncompressed;
    if (!KhPktDecompress(encrypted.dbroot_data().data(),
                         encrypted.dbroot_data().size(),
                         &uncompressed)) {
      return false;
    }

    // Parse actual dbroot_v2 proto.
    if (!dbroot.ParseFromArray(uncompressed.data(), uncompressed.size())) {
      return false;
    }
  }

  *has_terrain = dbroot.terrain_present();
  *has_imagery = dbroot.imagery_present();
  *is_proto_imagery = dbroot.proto_imagery();

  return true;
}

}  // namespace gedbroot

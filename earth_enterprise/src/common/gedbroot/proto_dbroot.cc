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

#include "common/gedbroot/proto_dbroot.h"
#include "google/protobuf/text_format.h"    // for google::protobuf::TextFormat
#include "google/protobuf/io/zero_copy_stream_impl.h"  // FileInputStream
#include "common/khFileUtils.h"
#include "common/khSimpleException.h"
#include "common/khEndian.h"
#include "common/packetcompress.h"
#include "common/etencoder.h"

geProtoDbroot::geProtoDbroot(void) {
  // define in .cc to reduce linker dependencies
}

geProtoDbroot::geProtoDbroot(const std::string &filename,
                             FileFormat input_format) {
  // open the filename and setup a protobuf input stream
  khReadFileCloser closer(::open(filename.c_str(), O_LARGEFILE | O_RDONLY));
  if (!closer.valid()) {
    throw khSimpleErrnoException("Unable to open ") << filename;
  }
  google::protobuf::io::FileInputStream stream(closer.fd());

  // slurp the contents
  switch (input_format) {
    case kTextFormat: {
      if (!google::protobuf::TextFormat::Parse(&stream, this)) {
        throw khSimpleException("Unable to read text dbroot from ")
            << filename;
      }
      break;
    }
    case kProtoFormat:
      if (!ParseFromZeroCopyStream(&stream)) {
        throw khSimpleException("Unable to read binary dbroot from ")
            << filename;
      }
      break;
    case kEncodedFormat: {
      // read encrypted proto from disk
      keyhole::dbroot::EncryptedDbRootProto encrypted;
      if (!encrypted.ParseFromZeroCopyStream(&stream)) {
        throw khSimpleException("Unable to read encoded dbroot from ")
            << filename;
      }

      // decode in place in the proto buffer
      try {
        etEncoder::Decode(&(*encrypted.mutable_dbroot_data())[0],
                          encrypted.dbroot_data().size(),
                          encrypted.encryption_data().data(),
                          encrypted.encryption_data().size());
      }
      catch (khSimpleException e) {
        throw khSimpleException("Error decoding dbroot: ") << e.what();
      }

      // uncompress
      LittleEndianReadBuffer uncompressed;
      if (!KhPktDecompress(encrypted.dbroot_data().data(),
                           encrypted.dbroot_data().size(),
                           &uncompressed)) {
        throw khSimpleException("Unable to uncompress encoded dbroot from ")
            << filename;
      }

      // parse actual dbroot_v2 proto
      if (!ParseFromArray(uncompressed.data(), uncompressed.size())) {
        throw khSimpleException("Unable to parse uncompressed encoded dbroot"
                                " from ")
            << filename;
      }
      break;
    }
  }
}

geProtoDbroot::~geProtoDbroot(void) {
  // define in .cc to reduce linker dependencies
}

void geProtoDbroot::Write(const std::string &filename,
                          FileFormat output_format) const {
  // open the filename and setup a protobuf output stream
  if (!khEnsureParentDir(filename)) {
    // warning already emitted
    throw khSimpleException("Can't make parent directory for ") << filename;
  }
  khWriteFileCloser closer(::open(filename.c_str(),
                                  O_LARGEFILE | O_WRONLY |
                                  O_CREAT | O_TRUNC,
                                  0666));
  if (!closer.valid()) {
    throw khSimpleErrnoException("Unable to open ")
        << filename << " for writing";
  }
  google::protobuf::io::FileOutputStream stream(closer.fd());

  // dump the contents
  switch (output_format) {
    case kTextFormat:
      if (!google::protobuf::TextFormat::Print(*this, &stream)) {
        throw khSimpleException("Unable to write text dbroot to ") << filename;
      }
      break;
    case kProtoFormat:
      if (!SerializeToZeroCopyStream(&stream)) {
        throw khSimpleException("Unable to write binary dbroot to ")
            << filename;
      }
      break;
    case kEncodedFormat: {
      // put binary representation of dbroot proto into string
      std::string text_output = this->SerializeAsString();

      // compress it using the old keyhole compression logic
      std::string compressed;
      compressed.resize(KhPktGetCompressBufferSize(text_output.size()));
      size_t compressed_size = compressed.size();
      if (!KhPktCompressWithBuffer(text_output.data(), text_output.size(),
                                   &compressed[0], &compressed_size)) {
        throw khSimpleException("Unable to compress dbroot");
      }

      // now encode/"crypt" it using the old keyhole encoding logic
      etEncoder::EncodeWithDefaultKey(&compressed[0], compressed_size);

      // build a new encrypted dbroot proto
      keyhole::dbroot::EncryptedDbRootProto encrypted;
      encrypted.set_encryption_type
          (keyhole::dbroot::EncryptedDbRootProto::ENCRYPTION_XOR);
      encrypted.set_encryption_data(etEncoder::kDefaultKey);
      encrypted.mutable_dbroot_data()->assign(compressed.data(),
                                              compressed_size);

      // now stream the new proto to disk
      if (!encrypted.SerializeToZeroCopyStream(&stream)) {
        throw khSimpleException("Unable to write encoded dbroot to ")
            << filename;
      }
      break;
    }
  }
}

void geProtoDbroot::Combine(const std::string &filename,
                            FileFormat input_format) {
  MergeFrom(geProtoDbroot(filename, input_format));
}

std::string geProtoDbroot::ToTextString(void) const {
  std::string output;
  if (!google::protobuf::TextFormat::PrintToString(*this, &output)) {
    throw khSimpleException("Unable to write text dbroot to string");
  }
  return output;
}

std::string geProtoDbroot::ToEncodedString(void) const {
  // put binary representation of dbroot proto into string
  std::string text_output = this->SerializeAsString();

  // compress it using the old keyhole compression logic
  std::string compressed;
  compressed.resize(KhPktGetCompressBufferSize(text_output.size()));
  size_t compressed_size = compressed.size();
  if (!KhPktCompressWithBuffer(text_output.data(), text_output.size(),
                               &compressed[0], &compressed_size)) {
    throw khSimpleException("Unable to compress dbroot");
  }

  // now encode/"crypt" it using the old keyhole encoding logic
  etEncoder::EncodeWithDefaultKey(&compressed[0], compressed_size);

  // build a new encrypted dbroot proto
  keyhole::dbroot::EncryptedDbRootProto encrypted;
  encrypted.set_encryption_type
      (keyhole::dbroot::EncryptedDbRootProto::ENCRYPTION_XOR);
  encrypted.set_encryption_data(etEncoder::kDefaultKey);
  encrypted.mutable_dbroot_data()->assign(compressed.data(),
                                          compressed_size);

  // now serialize the encrypted proto to a string and return it
  return encrypted.SerializeAsString();
}

void geProtoDbroot::AddUniversalFusionConfig(std::uint32_t epoch) {
  // setup some basic stuff all fusion dbroots need
  mutable_database_version()->set_quadtree_version(epoch);
  keyhole::dbroot::EndSnippetProto *end_snippet = mutable_end_snippet();
  keyhole::dbroot::ClientOptionsProto *client_options =
      end_snippet->mutable_client_options();
  end_snippet->set_disable_authentication(true);
  end_snippet->set_sky_database_is_available(false);
  client_options->set_use_extended_copyright_ids(false);
  client_options->set_show_2d_maps_icon(false);
}


keyhole::dbroot::StringEntryProto* geProtoDbroot::GetTranslationEntryById(
    std::int32_t id) {
  if (cached_string_table_.size() !=
      static_cast<size_t>(translation_entry_size())) {
    cached_string_table_.clear();
    for (int i = 0; i < translation_entry_size(); ++i) {
      keyhole::dbroot::StringEntryProto *string_entry =
          mutable_translation_entry(i);
      cached_string_table_[string_entry->string_id()] = string_entry;
    }
  }
  StringTable::const_iterator entry = cached_string_table_.find(id);
  if (entry == cached_string_table_.end()) {
    throw khSimpleException("Invalid string id: ") << id;
  }
  return entry->second;
}

bool geProtoDbroot::HasContents() const {
  // for timemachine dbroot, no need to check anything
  if (has_end_snippet() &&
      end_snippet().time_machine_options().is_timemachine()) {
    return true;
  }

  // for main dbroot, need to check the existence of raster, terrain,
  // or vector
  return  imagery_present() ||        // raster exist
          terrain_present() ||        // terrain exist
          nested_feature_size() > 0;  // vector exist
}

bool geProtoDbroot::IsValid(bool expect_epoch) const {
  if (!HasContents()) {
    notify(NFY_WARN, "Proto dbroot has no content.");
    return false;
  }

  if (expect_epoch && database_version().quadtree_version() == 0) {
    notify(NFY_WARN, "Database version (epoch) of \"0\" is invalid.");
    return false;
  }

  return true;
}

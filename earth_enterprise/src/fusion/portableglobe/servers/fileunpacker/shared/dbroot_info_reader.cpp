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

// TODO: High-level file comment.
#include "./dbroot_info_reader.h"
#include "common/gedbroot/proto_dbroot_util.h"

/**
 * Constructor.
 * Initialize reader/unpacker and default attributes.
 */
DbRootInfoReader::DbRootInfoReader(GlcReader *glc_reader,
                             GlcUnpacker *glc_unpacker)
  : glc_reader_(glc_reader)
  , glc_unpacker_(glc_unpacker)
  , has_terrain_(false)
  , has_imagery_(false)
  , is_proto_imagery_(false)
  , is_read_(false) {}

void DbRootInfoReader::Read() {
  if (!is_read_) {
    is_read_ = true;
    PackageFileLoc loc;
    if (glc_unpacker_->FindDbRoot(&loc)) {
      std::string contents;
      contents.resize(static_cast<size_t>(loc.Size()));
      if (glc_reader_->ReadData(&contents[0], loc.Offset(), loc.Size())) {
        gedbroot::GetDbrootInfo(contents, &has_imagery_, &has_terrain_,
                                &is_proto_imagery_);
      }
    }
  }
}

bool DbRootInfoReader::HasTerrain() {
  Read();
  return has_terrain_;
}

bool DbRootInfoReader::HasImagery() {
  Read();
  return has_imagery_;
}

bool DbRootInfoReader::IsProtoImagery() {
  Read();
  return is_proto_imagery_;
}

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

/* TODO: High-level file comment. */
#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_DBROOT_INFO_READER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_DBROOT_INFO_READER_H_

#include "./glc_unpacker.h"
#include "./glc_reader.h"

/**
 * Class for reading dbroot attributes (has imagery/terrain, is proto-imagery)
 * from globe file.
 */
class DbRootInfoReader {
 public:
  DbRootInfoReader(GlcReader *glc_reader, GlcUnpacker *glc_unpacker);

  /**
   * Return whether a 3d file contains terrain data, return false for 2d file.
   */
  bool HasTerrain();

  /**
   * Return whether 3d file contains imagery data, return false for 2d file.
   */
  bool HasImagery();

  /**
   * Return whether 3d file contains protobuffer imagery, return false for 2d
   * file.
   */
  bool IsProtoImagery();

 private:
  // Get dbroot attributes (has terrain, imagery, proto imagery, or not).
  void Read();

  GlcReader *glc_reader_;
  GlcUnpacker *glc_unpacker_;

  bool has_terrain_;
  bool has_imagery_;
  bool is_proto_imagery_;
  bool is_read_;
};


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_DBROOT_INFO_READER_H_

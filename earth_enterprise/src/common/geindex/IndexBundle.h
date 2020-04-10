/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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

//

#ifndef COMMON_GEINDEX_INDEXBUNDLE_H__
#define COMMON_GEINDEX_INDEXBUNDLE_H__

#include <khGuard.h>
#include <string>
#include "shared.h"

class FileBundle;
class EndianReadBuffer;
class geFilePool;
class ExternalDataAddress;

namespace geindex {

bool IsGEIndex(const std::string &path);
bool IsGEIndexOfType(geFilePool &file_pool,
                     const std::string &path,
                     const std::string &type);


// ****************************************************************************
// ***  Header (corresponds to index.hdr)
// ***
// ****************************************************************************
class Header {
 public:
  static const std::string kHeaderFilename;
  static const std::string kMagic;
  static const std::uint16_t      kCurrFileFormatVersion = 2;

  std::uint16_t          fileFormatVersion;
  ChildBucketAddr rootChildAddr;
  EntryBucketAddr rootEntryAddr;
  bool            slotsAreSingle;
  std::uint64_t          wastedSpace;
  std::string     contentDesc;

  Header(const std::string &desc, const std::string &index_path,
         bool slotsAreSingle_ = true);

  Header(geFilePool &file_pool, const std::string &index_path);

  Header(geFilePool &file_pool, const std::string &index_path,
         const std::string &prefix);

  void Push(EndianWriteBuffer &buf) const;

  std::uint32_t AddPacketFile(const std::string &packetfile);

  void   RemovePacketFile(const std::string &packetfile);

  // NOTE: The packetfile list can be sparse when working with a delta
  // index. If GetPacketFile() returns an empty string for a supplied
  // index, that packetfile has been removed from the index.
  inline std::uint32_t PacketFileCount(void) const { return packetfiles.size(); }

  std::string GetPacketFile(std::uint32_t packetfile_num) const;

  inline void SetPacketExtra(std::uint32_t packetfile_num, std::uint32_t extra) {
    assert(packetfile_num < packetfile_extras.size());
    packetfile_extras[packetfile_num] = extra;
  }

  inline std::uint32_t GetPacketExtra(std::uint32_t packetfile_num) const {
    assert(packetfile_num < packetfile_extras.size());
    return packetfile_extras[packetfile_num];
  }

  static std::string HeaderPath(const std::string &indexPath);

  // not intended for general use
  inline std::vector<std::string>& PacketfileVectorRef(void) {
    return packetfiles;
  }

  void AppendManifest(geFilePool& file_pool,
                      std::vector<ManifestEntry> &manifest,
                      const std::string& tmp_dir) const;
  bool WasRelativePaths() const { return was_relative_paths_; }

  static std::string ContentDesc(geFilePool& file_pool,
                                 const std::string &index_path);
 protected:
  bool was_relative_paths_;
 private:
  // The file names are always absolute without a trailing "/" for directories
  std::vector<std::string> packetfiles;
  std::vector<std::uint32_t>      packetfile_extras;
  const std::string        index_path_;
  std::uint32_t                   header_size_;

  void Pull(EndianReadBuffer &buf, const std::string &prefix);
};


// ****************************************************************************
// ***  Simple wrapper around FileBundle, adds higher level semantics
// ***
// ***  Designed to be a base class to IndexBundleReader & IndexBundleWriter
// ***  Derived classes will create a new FileBundle derived class and transfer
// ***  ownership of it to this base class when constructing it
// ****************************************************************************
class IndexBundle {
 public:
  Header header;
  const std::string &abs_path() const { return fileBundle->abs_path(); }

  // NOTE: The packetfile list can be sparse when working with a delta
  // index. If GetPacketFile() returns an empty string for a supplied
  // index, that packetfile has been removed fromt he index.
  inline std::uint32_t PacketFileCount(void) const { return header.PacketFileCount();}
  std::string GetPacketFile(std::uint32_t packetfile_num) const;
  void AppendManifest(std::vector<ManifestEntry> &manifest,
                      const std::string& tmp_dir) const;

 protected:
  khDeleteGuard<FileBundle> fileBundle;

  // used by reader - filePool and fname will be used to load header
  IndexBundle(geFilePool &filePool,
              const khTransferGuard<FileBundle> &fileBundle_,
              const std::string &prefix);

  IndexBundle(geFilePool &filePool,
              const khTransferGuard<FileBundle> &fileBundle_);

  // used by writer
  IndexBundle(const khTransferGuard<FileBundle> &fileBundle_,
              const std::string &desc, bool slotsAreSingle);

  ~IndexBundle(void);
 public:
  void LoadWithCRC(const BundleAddr &addr, EndianReadBuffer &buf);
  void LoadWithoutCRC(const BundleAddr &addr, EndianReadBuffer &buf);
};


} // namespace geindex

#endif // COMMON_GEINDEX_INDEXBUNDLE_H__

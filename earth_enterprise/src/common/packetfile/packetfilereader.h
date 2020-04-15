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


#ifndef COMMON_PACKETFILE_PACKETFILEREADER_H__
#define COMMON_PACKETFILE_PACKETFILEREADER_H__

#include <geFilePool.h>
#include <quadtreepath.h>
#include <filebundle/filebundlereader.h>
#include <packetfile/packetindex.h>
#include <common/ManifestEntry.h>


// PacketFileReaderBase is a wrapper over FileBundleReader. It keeps information
// about if data has crc and makes the correct call to FileBundleReader.
class PacketFileReaderBase : protected FileBundleReader {
 public:
  PacketFileReaderBase(geFilePool &file_pool, const std::string &path);
  PacketFileReaderBase(geFilePool &file_pool, const std::string &path,
                       const std::string &prefix);
  PacketFileReaderBase(geFilePool &file_pool, const std::string &path,
                       bool data_has_crc);

  virtual ~PacketFileReaderBase();

  const std::string& abs_path() const { return FileBundleReader::abs_path(); }

  // Get manifest for packet file (includes packetindex, if crc is not used)
  void AppendManifest(std::vector<ManifestEntry> &manifest,
                      const std::string &tmp_dir) const {
    if (IsCRCEnabled() == false) {
      const std::string index = IndexFilename();
      manifest.push_back(ManifestEntry(index, index));
    }
    FileBundleReader::AppendManifest(manifest, tmp_dir);
  }

  void GetManifest(std::vector<ManifestEntry> &manifest,
                   const std::string &tmp_dir) const {
    manifest.clear();
    AppendManifest(manifest, tmp_dir);
  }

  // Enable cached reading, with "max_blocks" number of cache blocks each
  // of size "block_size".
  // Suggested usage is max_blocks=2, block_size=30MB for a linear read through
  // a FileBundle.
  // For a quadtree traversal, perhaps use max_blocks=10, block_size=5MB.
  // max_blocks must be >= 2.
  void EnableReadCache(std::uint32_t max_blocks, std::uint32_t block_size);

  // Read a specific block, check CRC if it exists.  Resulting buffer
  // will have CRC striped off.  The second form reads the data into a
  // string buffer without modifying the buffer size (buffer must be
  // large enough to receive data).
  size_t ReadAtCRC(std::uint64_t read_pos, std::string &buffer,
                   size_t read_len) const {
    if (IsCRCEnabled()) {
      return FileBundleReader::ReadAtCRC(read_pos, &buffer, read_len);
    } else {
      FileBundleReader::ReadAt(read_pos, &buffer, read_len);
      return read_len;
    }
  }

  size_t ReadAtCRC(std::uint64_t read_pos,
                   std::string &buffer,
                   size_t offset,
                   size_t read_len) const {
    if (IsCRCEnabled()) {
      return FileBundleReader::ReadAtCRC(read_pos, &buffer, offset, read_len);
    } else {
      FileBundleReader::ReadAt(read_pos, &buffer, offset, read_len);
      return read_len;
    }
  }

  bool IsCRCEnabled() const {
    return data_has_crc_;
  }

  std::string IndexFilename() const {
    return khComposePath(abs_path(), kIndexBase);
  }

  static const std::string kIndexBase;  // The basename for packetindex

 protected:
  void SetDataHasCrc() {
    *const_cast<bool *>(&data_has_crc_) = true;
  }
 private:
  const bool data_has_crc_;
  friend class PacketFileUnitTest;
  DISALLOW_COPY_AND_ASSIGN(PacketFileReaderBase);
};


// PacketFileReader - read from packet file in preorder by quadtree path
class PacketFileReader : public PacketFileReaderBase {
 public:
  PacketFileReader(geFilePool &file_pool, const std::string &path);
  virtual ~PacketFileReader();

  // ReadNextCRC - read next data record and return data and quadtree
  // path.  The buffer size must be large enough for the data
  // (including CRC, if any).  The return value is the size without
  // CRC.  Returns 0 if end of data.
  //
  // Will throw an exception on error (including bad CRC or
  // buffer too short).  If the source file data does not have CRCs
  // (usually an old, converted pack file), then data CRC checking is
  // skipped.
  size_t ReadNextCRC(QuadtreePath *qt_path, void *buffer, size_t buf_size);
  size_t ReadNextCRC(QuadtreePath *qt_path, std::string &buffer);

 private:
  PacketIndexReader index_reader_;
  friend class PacketFileUnitTest;
  DISALLOW_COPY_AND_ASSIGN(PacketFileReader);
};

#endif  // COMMON_PACKETFILE_PACKETFILEREADER_H__

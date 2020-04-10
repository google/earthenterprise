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

//

#include "IndexBundle.h"
#include <third_party/rsa_md5/crc32.h>
#include "Entries.h"
#include <khEndian.h>
#include <geFilePool.h>
#include <filebundle/filebundle.h>
#include <khFileUtils.h>

namespace geindex {

void IndexBundle::LoadWithCRC(const BundleAddr &addr, EndianReadBuffer &buf) {
  buf.Seek(0);
  buf.resize(addr.Size());
  fileBundle->ReadAtCRC(addr.Offset(), &buf[0], addr.Size());
  buf.resize(addr.Size()-FileBundle::kCRCsize);
}

void IndexBundle::LoadWithoutCRC(const BundleAddr &addr,
                                 EndianReadBuffer &buf) {
  buf.Seek(0);
  buf.resize(addr.Size());
  fileBundle->ReadAt(addr.Offset(), &buf[0], addr.Size());
}

bool IsGEIndex(const std::string &path) {
  std::string header = Header::HeaderPath(path);
  if (khExists(header)) {
    std::string signature;
    if (khReadStringFromFile(header, signature, Header::kMagic.size()) &&
        (signature == Header::kMagic)) {
      return true;
    }
  }
  return false;
}

bool IsGEIndexOfType(geFilePool &file_pool,
                     const std::string &path,
                     const std::string &type)
{
  if (!IsGEIndex(path)) {
    return false;
  }
  return (geindex::Header::ContentDesc(file_pool, path) == type);
}

// ****************************************************************************
// ***  Header
// ****************************************************************************
const std::string Header::kHeaderFilename = "index.hdr";
const std::string Header::kMagic = "GEIndexHeader";

Header::Header(const std::string &desc, const std::string &index_path,
               bool slotsAreSingle_) :
    fileFormatVersion(kCurrFileFormatVersion),
    rootChildAddr(), rootEntryAddr(),
    slotsAreSingle(slotsAreSingle_),
    wastedSpace(0),
    contentDesc(desc),
    was_relative_paths_(true),
    packetfiles(), packetfile_extras(),
    index_path_(index_path) {
}

Header::Header(geFilePool &file_pool, const std::string &index_path,
               const std::string &prefix) :
    fileFormatVersion(kCurrFileFormatVersion),
    rootChildAddr(), rootEntryAddr(),
    slotsAreSingle(),
    wastedSpace(0),
    contentDesc(),
    was_relative_paths_(true),
    packetfiles(), packetfile_extras(),
    index_path_(index_path) {
  LittleEndianReadBuffer buf;
  file_pool.ReadSimpleFileWithCRC(HeaderPath(index_path), buf);
  Pull(buf, prefix);
}

Header::Header(geFilePool &file_pool, const std::string &index_path) :
    fileFormatVersion(kCurrFileFormatVersion),
    rootChildAddr(), rootEntryAddr(),
    slotsAreSingle(),
    wastedSpace(0),
    contentDesc(),
    was_relative_paths_(true),
    packetfiles(), packetfile_extras(),
    index_path_(index_path) {
  LittleEndianReadBuffer buf;
  file_pool.ReadSimpleFileWithCRC(HeaderPath(index_path), buf);
  Pull(buf, "");
}


void Header::Push(EndianWriteBuffer &buf) const {
  std::vector<std::string> tmp_packet_files(packetfiles);
  size_t num_packet_files = tmp_packet_files.size();
  for (size_t i = 0; i < num_packet_files; ++i) {
    std::string* packet_file = &tmp_packet_files[i];
    *packet_file = khRelativePath(index_path_, *packet_file);
  }

  buf << FixedLengthString(kMagic, kMagic.size())
      << fileFormatVersion
      << rootChildAddr
      << rootEntryAddr
      << slotsAreSingle
      << wastedSpace
      << tmp_packet_files
      << packetfile_extras
      << contentDesc;
}

std::string Header::ContentDesc(
    geFilePool& file_pool, const std::string &index_path) {
  LittleEndianReadBuffer buf;
  file_pool.ReadSimpleFileWithCRC(HeaderPath(index_path), buf);

  std::string skip_magic;
  std::uint16_t fileFormatVersion;
  buf >> FixedLengthString(skip_magic, kMagic.size())
      >> fileFormatVersion;

  std::string     contentDesc;
  if (fileFormatVersion >= 2) {
    ChildBucketAddr rootChildAddr;
    EntryBucketAddr rootEntryAddr;
    bool            slotsAreSingle;
    std::uint64_t          wastedSpace;
    std::vector<std::string> packetfiles;
    std::vector<std::uint32_t>      packetfile_extras;
    buf >> rootChildAddr
        >> rootEntryAddr
        >> slotsAreSingle
        >> wastedSpace
        >> packetfiles
        >> packetfile_extras
        >> contentDesc;
  }
  return contentDesc;
}

// Note: after getting list of packetfile paths from an index file,
// the function converts them to an absolute paths preparing for
// GetPacketFile().
// If an original packetfile path is an absolute path, a resulting packet path
// is a prefix + original packetfile path, otherwise it is
// an index path + original packetfile path.
void Header::Pull(EndianReadBuffer &buf, const std::string &prefix) {
  header_size_ = buf.size() + kCRC32Size;
  std::string magic;
  buf >> FixedLengthString(magic, kMagic.size());
  if (magic != kMagic) {
    throw khSimpleException("Bad header magic");
  }
  buf >> fileFormatVersion;
  if (fileFormatVersion > kCurrFileFormatVersion) {
    throw khSimpleException("Unsupported file format version");
  }
  buf >> rootChildAddr
      >> rootEntryAddr
      >> slotsAreSingle
      >> wastedSpace
      >> packetfiles;

  size_t num_packet_files = packetfiles.size();
  for (size_t i = 0; i < num_packet_files; ++i) {
    std::string& packet_file = packetfiles[i];
    if (khIsAbspath(packet_file)) {  // This check is required to accomodate old
      if (!prefix.empty()) {
        packet_file = prefix + packet_file;
      }
      was_relative_paths_ = false;
    } else {
      packet_file = khNormalizeDir(
          khComposePath(index_path_, packet_file), false);
    }
#if 0
    // Note: for regular db manifest (db asset), all packetfiles
    // should exist, while it may be false for delta manifest. So, commenting
    // this assert out.
    // Note: All packetfiles are checked for existence when
    // collecting them into db manifest in IndexManifest.
    assert(khExists(packet_file));
#endif
  }

  if (fileFormatVersion >= 1) {
    buf >> packetfile_extras;
  } else {
    // will prefill with 0
    packetfile_extras.resize(packetfiles.size());
  }
  if (fileFormatVersion >= 2) {
    buf >> contentDesc;
  } else {
    contentDesc = std::string();
  }
}

std::string Header::HeaderPath(const std::string &indexPath) {
  return khComposePath(indexPath, kHeaderFilename);
}

 std::uint32_t Header::AddPacketFile(const std::string &packetfile) {
  if (khIsAbspath(packetfile)) {
    packetfiles.push_back(packetfile);
  } else {
    std::string abs_packet_file = khNormalizeDir(
        khComposePath(index_path_, packetfile), false);
    packetfiles.push_back(abs_packet_file);
  }
  packetfile_extras.push_back(0);
  return packetfiles.size() - 1;
}

void Header::RemovePacketFile(const std::string &packetfile) {
  for (unsigned int i = 0; i < packetfiles.size(); ++i) {
    if (packetfiles[i] == packetfile) {
      packetfiles[i] = std::string();
      packetfile_extras[i] = 0;
      return;
    }
  }
}

std::string Header::GetPacketFile(std::uint32_t packetfile_num) const {
  std::string packetfile = packetfiles[packetfile_num];

  if (packetfile.empty()) {
    return std::string();
  }
  assert(khIsAbspath(packetfile));
  return packetfile;
}

void Header::AppendManifest(geFilePool& file_pool,
                            std::vector<ManifestEntry> &manifest,
                            const std::string& tmp_dir) const {
  // Index header is first entry (required by IndexManifest::GetServerManifest)

  // Index header file (always use current path for original path,
  // since the header always changes during incremental update).
  const std::string orig_header = HeaderPath(index_path_);

  manifest.resize(manifest.size() + 1);  // Add one uninitialized element
  ManifestEntry* entry = &manifest.back();
  entry->orig_path = orig_header;
  std::string* curr_header_path = &entry->current_path;
  std::uint64_t* curr_size = &entry->data_size;
  if (was_relative_paths_ == false) {
    LittleEndianWriteBuffer buf;
    buf << (*this);
    *curr_size = buf.size() + kCRC32Size;
    if (!tmp_dir.empty()) {
      *curr_header_path = khComposePath(tmp_dir, orig_header);
      khEnsureParentDir(*curr_header_path);
      file_pool.WriteSimpleFileWithCRC(*curr_header_path, buf);
      assert(*curr_size == khGetFileSizeOrThrow(*curr_header_path));
    } else {
      *curr_header_path = orig_header;
    }
  } else {
    *curr_size = header_size_;
    *curr_header_path = orig_header;
    assert(*curr_size == khGetFileSizeOrThrow(*curr_header_path));
  }
}

// ****************************************************************************
// ***  IndexBundle
// ****************************************************************************
IndexBundle::IndexBundle(geFilePool &filePool,
                         const khTransferGuard<FileBundle> &fileBundle_) :
    header(filePool, fileBundle_->abs_path()),
    fileBundle(fileBundle_) {
}

IndexBundle::IndexBundle(geFilePool &filePool,
                         const khTransferGuard<FileBundle> &fileBundle_,
                         const std::string &prefix) :
    header(filePool, fileBundle_->abs_path(), prefix),
    fileBundle(fileBundle_) {
}

IndexBundle::IndexBundle(const khTransferGuard<FileBundle> &fileBundle_,
                         const std::string &desc, bool slotsAreSingle) :
    header(desc, fileBundle_->abs_path(), slotsAreSingle),
    fileBundle(fileBundle_) {
}

// implemented in .cpp so the instantiation of the destructor for
// fileBundle will happen here (where we have #included the appropriate header)
IndexBundle::~IndexBundle(void) {
}

std::string IndexBundle::GetPacketFile(std::uint32_t idx) const {
  return header.GetPacketFile(idx);
}

void IndexBundle::AppendManifest(std::vector<ManifestEntry> &manifest,
                                 const std::string& tmp_dir) const {
  header.AppendManifest(fileBundle->file_pool(), manifest, tmp_dir);
  fileBundle->AppendManifest(manifest, tmp_dir);
}


} // namespace geindex

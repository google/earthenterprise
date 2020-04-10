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


#ifndef FUSION_GECRAWLER_ARCHIVESOURCE_H__
#define FUSION_GECRAWLER_ARCHIVESOURCE_H__

#include <third_party/rsa_md5/crc32.h>
#include <common/base/macros.h>
//#include "common/khTypes.h"
#include <cstdint>
#include "common/khEndian.h"
#include "common/geFilePool.h"
#include "common/quadtreepath.h"
#include "common/khFileUtils.h"
#include "common/filebundle/filebundlereader.h"
#include "fusion/gecrawler/crawlersource.h"

namespace gecrawler {

// ArchiveSource - traverse server database from archived copy

class ArchiveBundleReader;

template <class T> class ArchiveSource : public CrawlerSource<T> {
 public:
  // If the jpeg_date is uninitialized, we skip all historical imagery,
  // otherwise, it will get the latest imagery <= the given date, unless
  // the jpeg_date is marked as MatchAllDates() in which case it will get
  // all imagery refs.
  ArchiveSource(geFilePool* file_pool,
                const std::string &archive_name,
                const QuadtreePath &start_path,
                const keyhole::JpegCommentDate& jpeg_date);
  ~ArchiveSource();
  virtual bool GetPacket(const CrawlerSourceBase::PacketType packet_type,
                         const qtpacket::KhQuadtreeDataReference &img_ref,
                         std::string *buffer);
  virtual void Close();
 private:
  khDeleteGuard<ArchiveBundleReader> qtp_reader_;
  khDeleteGuard<ArchiveBundleReader> qtp2_reader_;
  khDeleteGuard<ArchiveBundleReader> img_reader_;
  khDeleteGuard<ArchiveBundleReader> ter_reader_;
  khDeleteGuard<ArchiveBundleReader> vec_reader_;
  DISALLOW_COPY_AND_ASSIGN(ArchiveSource);
};

//---------------------------------------------------------------------------

// ArchiveBundleReader - read data sequentially from archive

class ArchiveBundleReader : public FileBundleReader {
 public:
  ArchiveBundleReader(geFilePool &file_pool, const std::string &path)
      : FileBundleReader(file_pool, path),
        linear_position_(0) {
  }

  void ReadNext(qtpacket::KhQuadtreeDataReference *qt_ref,
                std::string *packet_buffer) {
    packet_buffer->clear();
    if (IsFinished())
      return;

    std::uint32_t packet_length = ReadPacketHeader(qt_ref);

    // Read the rest of the packet

    size_t read_size = packet_length + kCRC32Size;
    packet_buffer->resize(read_size);
    ReadAtCRC(LinearToBundlePosition(linear_position_),
              &(*packet_buffer)[0],
              packet_buffer->size());
    linear_position_ += read_size;
    packet_buffer->resize(packet_length); // discard CRC
  }

  inline bool IsFinished() const { return linear_position_ >= data_size(); }

  inline std::uint64_t Tell() const { return linear_position_; }
  inline void Seek(std::uint64_t new_position) { linear_position_ = new_position; }

 private:
  std::uint64_t linear_position_;
  LittleEndianReadBuffer le_buffer_;

  std::uint32_t ReadPacketHeader(qtpacket::KhQuadtreeDataReference *qt_ref) {
    std::uint32_t packet_length;
    size_t header_size = sizeof(packet_length)
        + qtpacket::KhQuadtreeDataReference::kSerialSize
        + kCRC32Size;
    le_buffer_.resize(header_size);
    le_buffer_.Seek(0);

    ReadAtCRC(LinearToBundlePosition(linear_position_),
              &le_buffer_[0],
              header_size);
    linear_position_ += header_size;

    le_buffer_ >> packet_length;
    le_buffer_ >> *qt_ref;
    return packet_length;
  }
};


//---------------------------------------------------------------------------
template <class T>
ArchiveSource<T>::ArchiveSource(geFilePool* file_pool,
                             const std::string &archive_name,
                             const QuadtreePath &start_path,
                             const keyhole::JpegCommentDate& jpeg_date)
    : CrawlerSource<T>(archive_name, start_path, jpeg_date) {
  std::string bundle_name("/bundle.0000");

  std::string qtp_archive_name = khComposePath(archive_name, "qtp");
  if (khExists(qtp_archive_name + bundle_name)) {
    qtp_reader_ = TransferOwnership(
        new ArchiveBundleReader(*file_pool, qtp_archive_name));
  }

  std::string qtp2_archive_name = khComposePath(archive_name, "qtp2");
  if (khExists(qtp2_archive_name + bundle_name)) {
    qtp2_reader_ = TransferOwnership(
        new ArchiveBundleReader(*file_pool, qtp2_archive_name));
  }

  std::string img_archive_name = khComposePath(archive_name, "img");
  if (khExists(img_archive_name + bundle_name)) {
    img_reader_ = TransferOwnership(
        new ArchiveBundleReader(*file_pool, img_archive_name));
  }

  std::string ter_archive_name = khComposePath(archive_name, "ter");
  if (khExists(ter_archive_name + bundle_name)) {
    ter_reader_ = TransferOwnership(
        new ArchiveBundleReader(*file_pool, ter_archive_name));
  }

  std::string vec_archive_name = khComposePath(archive_name, "vec");
  if (khExists(vec_archive_name + bundle_name)) {
    vec_reader_ = TransferOwnership(
        new ArchiveBundleReader(*file_pool, vec_archive_name));
  }
}

template <class T>
ArchiveSource<T>::~ArchiveSource() {
}

template <class T>
bool ArchiveSource<T>::GetPacket(
    const CrawlerSourceBase::PacketType packet_type,
    const qtpacket::KhQuadtreeDataReference &qt_ref,
    std::string *buffer) {
  assert(buffer);

  if (getNotifyLevel() >= NFY_DEBUG) {
    notify(NFY_DEBUG, "GetPacket %s \"%s\" ch %u ver %u",
           CrawlerSourceBase::packet_type_name(packet_type).c_str(),
           qt_ref.qt_path().AsString().c_str(),
           qt_ref.channel(),
           qt_ref.version());
  }

  khDeleteGuard<ArchiveBundleReader> *reader =
      (packet_type == CrawlerSourceBase::kQuadtree) ? &qtp_reader_
      : (packet_type == CrawlerSourceBase::kQuadtreeFormat2) ? &qtp2_reader_
      : (packet_type == CrawlerSourceBase::kImage) ? &img_reader_
      : (packet_type == CrawlerSourceBase::kTerrain) ? &ter_reader_
      : (packet_type == CrawlerSourceBase::kVector) ? &vec_reader_
      : NULL;
  assert(reader);
  if (!*reader)
    return false;

  while (!(*reader)->IsFinished()) {
    std::uint64_t saved_position = (*reader)->Tell();  // Save the current position.

    qtpacket::KhQuadtreeDataReference archive_ref;
    (*reader)->ReadNext(&archive_ref, buffer);

    if (archive_ref.qt_path() == qt_ref.qt_path()) {
      return true;
    }

    if (archive_ref.qt_path() > qt_ref.qt_path()) {
      notify(NFY_WARN, "Requested %s packet \"%s\" not in archive",
             CrawlerSourceBase::packet_type_name(packet_type).c_str(),
             qt_ref.qt_path().AsString().c_str());
      (*reader)->Seek(saved_position);  // Restore the position.
      return false;
    }

    notify(NFY_VERBOSE, "Skipping %s packet \"%s\" searching for \"%s\"",
           CrawlerSourceBase::packet_type_name(packet_type).c_str(),
           archive_ref.qt_path().AsString().c_str(),
           qt_ref.qt_path().AsString().c_str());
  }

  notify(NFY_WARN, "Requested %s packet \"%s\" after end of archive",
         CrawlerSourceBase::packet_type_name(packet_type).c_str(),
         qt_ref.qt_path().AsString().c_str());
  return false;
}

template <class T>
void ArchiveSource<T>::Close() {
  if (qtp_reader_) {
    qtp_reader_->Close();
    qtp_reader_.clear();
  }
  if (qtp2_reader_) {
    qtp2_reader_->Close();
    qtp2_reader_.clear();
  }
  if (img_reader_) {
    img_reader_->Close();
    img_reader_.clear();
  }
  if (ter_reader_) {
    ter_reader_->Close();
    ter_reader_.clear();
  }
  if (vec_reader_) {
    vec_reader_->Close();
    vec_reader_.clear();
  }
}

}  // namespace gecrawler

#endif  // FUSION_GECRAWLER_ARCHIVESOURCE_H__

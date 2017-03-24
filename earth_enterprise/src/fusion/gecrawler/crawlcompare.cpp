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


#include <fusion/gecrawler/crawlcompare.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <khEndian.h>

namespace gecrawler {

void DumpPacket(const std::string &buffer,
                const std::string &src_name,
                const std::string &packet_type_name,
                const std::string &qt_path_str,
                std::ostream &dump_log) {
  dump_log << std::string(75, '-') << std::endl;
  dump_log << "Path: \"" << qt_path_str << "\", type: " << packet_type_name
           << ", size " << std::right << std::dec << buffer.size()
           << ", source " << src_name << std::endl;
  const size_t kRowSize = 16;
  for (size_t i = 0; i < buffer.size(); i += kRowSize) {
    size_t row_end = std::min(i + kRowSize, buffer.size());
    dump_log << "  " << std::dec << std::setw(5) << std::setfill(' ')
             << i << " :";
    for (size_t j = i; j < row_end; ++j) {
      dump_log << " " << std::hex << std::setw(2) << std::setfill('0')
               << (0xff & static_cast<unsigned int>(buffer[j]));
    }
    dump_log << std::endl;
  }
}

// Compare selected reference data (qt_path, channel)

bool CrawlCompareBase::MostlyEqual(const qtpacket::KhQuadtreeDataReference
                                   *ref1,
                                   const qtpacket::KhQuadtreeDataReference
                                   *ref2,
                                   const bool check_provider) {
  if (!ref1 || !ref2)
    return false;
  return ref1->qt_path() == ref2->qt_path()
      && ref1->channel() == ref2->channel()
      && (!check_provider || ref1->provider() == ref2->provider());
}

bool CrawlCompareBase::SortLessThan(
  const qtpacket::KhQuadtreeDataReference &ref1,
  const qtpacket::KhQuadtreeDataReference &ref2) {
  return ref1.qt_path() < ref2.qt_path()
      || (ref1.qt_path() == ref2.qt_path()
          && ref1.channel() < ref2.channel());
}

bool CrawlCompareBase::HasNoDataReferences(
    const qtpacket::KhQuadtreeDataReferenceGroup &refs) {
  return refs.img_refs->empty()
      && refs.ter_refs->empty()
      && refs.vec_refs->empty();
}

// Compare generated quadtree packet references.

bool CrawlCompareBase::CompareRefs(
    const std::vector<qtpacket::KhQuadtreeDataReference> &refs1,
    const std::vector<qtpacket::KhQuadtreeDataReference> &refs2,
    const CrawlerSourceBase::PacketType packet_type,
    const std::string &description,
    std::string *diffs,
    KmlFolder *folder) {
  size_t i1 = 0;
  size_t i2 = 0;

  bool matched = true;
  std::ostringstream new_diffs;

  while (i1 < refs1.size() || i2 < refs2.size()) {
    const qtpacket::KhQuadtreeDataReference *ref1 =
        (i1 < refs1.size()) ? &refs1.at(i1) : NULL;
    const qtpacket::KhQuadtreeDataReference *ref2 =
        (i2 < refs2.size()) ? &refs2.at(i2) : NULL;

    if (MostlyEqual(ref1, ref2, options_.check_providers.test(packet_type))) {
      if (options_.compare_types.test(packet_type)) {
        if (!ComparePacketContents(*ref1,
                                   *ref2,
                                   packet_type,
                                   &new_diffs,
                                   folder)) {
          matched = false;
        }
      }
      ++i1;
      ++i2;
    } else if (!ref2  ||  (ref1 && ref1->qt_path() < ref2->qt_path())) {
      matched = false;
      std::ostringstream folder_snippet;
      folder_snippet << " Missing in source 2, "
                     << " channel=" << ref1->channel()
                     << " provider=" << ref1->provider();
      new_diffs << "\n    " << ref1->qt_path().AsString()
                << folder_snippet.str();
      if (folder) {
        folder->AddBox(ref1->qt_path(),
                       KmlLog::kSource2Str,
                       description + folder_snippet.str());
      }
      ++i1;
    } else if (!ref1  ||  ref1->qt_path() > ref2->qt_path()) {
      matched = false;
      std::ostringstream folder_snippet;
      folder_snippet << " Missing in source 1, "
                     << " channel=" << ref2->channel()
                     << " provider=" << ref2->provider();
      new_diffs << "\n    " << ref2->qt_path().AsString()
                << folder_snippet.str();
      if (folder) {
        folder->AddBox(ref2->qt_path(),
                       KmlLog::kSource1Str,
                       description + folder_snippet.str());
      }
      ++i2;
    } else {
      matched = false;
      std::ostringstream folder_snippet;
      folder_snippet << " Mismatch: channels=("
                     << ref1->channel() << "," << ref2->channel()
                     << ") providers=("
                     << ref1->provider() << "," << ref2->provider()
                     << ")";
      new_diffs << "\n    " << ref1->qt_path().AsString()
                << folder_snippet.str();
      if (folder) {
        folder->AddBox(ref1->qt_path(),
                       KmlLog::kBothStr,
                       description + folder_snippet.str());
      }
      ++i1;
      ++i2;
    }
  }

  if (matched) {
    return true;
  } else {
    *diffs += "\n  " + description + ":" + new_diffs.str();
    return false;
  }
}

bool CrawlCompareBase::CompareRefGroups(
    const qtpacket::KhQuadtreeDataReferenceGroup &group1,
    const qtpacket::KhQuadtreeDataReferenceGroup &group2,
    std::string *diffs,
    KmlFolder *folder) {
  // Comare data refs. Differences in quadtree refs will be found when
  // we try to read the quadtree packets.
  bool same_img = CompareRefs(*group1.img_refs,
                              *group2.img_refs,
                              CrawlerSourceBase::kImage,
                              "img_refs",
                              diffs,
                              folder);
  bool same_ter = CompareRefs(*group1.ter_refs,
                              *group2.ter_refs,
                              CrawlerSourceBase::kTerrain,
                              "ter_refs",
                              diffs,
                              folder);
  bool same_vec = CompareRefs(*group1.vec_refs,
                              *group2.vec_refs,
                              CrawlerSourceBase::kVector,
                              "vec_refs",
                              diffs,
                              folder);

  return same_img && same_ter && same_vec;
}


std::string CrawlCompareBase::FormatCounters() const {
  std::ostringstream strm;
  FormatCounters(&strm);
  return strm.str();
}

void CrawlCompareBase::FormatCounters(std::ostream *strm) const {
  *strm << std::dec << std::left
        << "Src 1 read:         "
        << " qtp=" << std::setw(10)
        << count_packets_read1_.Get(CrawlerSourceBase::kQuadtree)
        << " img=" << std::setw(10)
        << count_packets_read1_.Get(CrawlerSourceBase::kImage)
        << " ter=" << std::setw(10)
        << count_packets_read1_.Get(CrawlerSourceBase::kTerrain)
        << " vec=" << std::setw(10)
        << count_packets_read1_.Get(CrawlerSourceBase::kVector)
        << std::endl
        << "Src 1 failed:       "
        << " qtp=" << std::setw(10)
        << count_packets_failed1_.Get(CrawlerSourceBase::kQuadtree)
        << " img=" << std::setw(10)
        << count_packets_failed1_.Get(CrawlerSourceBase::kImage)
        << " ter=" << std::setw(10)
        << count_packets_failed1_.Get(CrawlerSourceBase::kTerrain)
        << " vec=" << std::setw(10)
        << count_packets_failed1_.Get(CrawlerSourceBase::kVector)
        << std::endl
        << "Src 2 read:         "
        << " qtp=" << std::setw(10)
        << count_packets_read2_.Get(CrawlerSourceBase::kQuadtree)
        << " img=" << std::setw(10)
        << count_packets_read2_.Get(CrawlerSourceBase::kImage)
        << " ter=" << std::setw(10)
        << count_packets_read2_.Get(CrawlerSourceBase::kTerrain)
        << " vec=" << std::setw(10)
        << count_packets_read2_.Get(CrawlerSourceBase::kVector)
        << std::endl
        << "Src 2 failed:       "
        << " qtp=" << std::setw(10)
        << count_packets_failed2_.Get(CrawlerSourceBase::kQuadtree)
        << " img=" << std::setw(10)
        << count_packets_failed2_.Get(CrawlerSourceBase::kImage)
        << " ter=" << std::setw(10)
        << count_packets_failed2_.Get(CrawlerSourceBase::kTerrain)
        << " vec=" << std::setw(10)
        << count_packets_failed2_.Get(CrawlerSourceBase::kVector)
        << std::endl
        << "Packets matched:    "
        << " qtp=" << std::setw(10)
        << count_packets_matched_.Get(CrawlerSourceBase::kQuadtree)
        << " img=" << std::setw(10)
        << count_packets_matched_.Get(CrawlerSourceBase::kImage)
        << " ter=" << std::setw(10)
        << count_packets_matched_.Get(CrawlerSourceBase::kTerrain)
        << " vec=" << std::setw(10)
        << count_packets_matched_.Get(CrawlerSourceBase::kVector)
        << std::endl
        << "Packets mismatched: "
        << " qtp=" << std::setw(10)
        << count_packets_mismatched_.Get(CrawlerSourceBase::kQuadtree)
        << " img=" << std::setw(10)
        << count_packets_mismatched_.Get(CrawlerSourceBase::kImage)
        << " ter=" << std::setw(10)
        << count_packets_mismatched_.Get(CrawlerSourceBase::kTerrain)
        << " vec=" << std::setw(10)
        << count_packets_mismatched_.Get(CrawlerSourceBase::kVector)
        << std::endl;
}

// Here's the implementations of some of the templated methods.
// G++ requires this to be in the header.
CrawlCompare::CrawlCompare(khTransferGuard<CrawlerSourceBase> source1,
                           khTransferGuard<CrawlerSourceBase> source2,
                           const CrawlCompareOptions &options,
                           khTransferGuard<KmlLog> kml_log)
    : CrawlCompareBase(options, kml_log),
      source1_(source1),
      source2_(source2) {
}

bool CrawlCompare::Compare() {
  assert(source1_);
  assert(source2_);

  std::vector<qtpacket::KhQuadtreeDataReference> qtp_refs1;
  std::vector<qtpacket::KhQuadtreeDataReference> qtp2_refs1;
  std::vector<qtpacket::KhQuadtreeDataReference> img_refs1;
  std::vector<qtpacket::KhQuadtreeDataReference> ter_refs1;
  std::vector<qtpacket::KhQuadtreeDataReference> vec_refs1;
  qtpacket::KhQuadtreeDataReferenceGroup refs1(&qtp_refs1,
                                               &qtp2_refs1,
                                               &img_refs1,
                                               &ter_refs1,
                                               &vec_refs1);
  std::vector<qtpacket::KhQuadtreeDataReference> qtp_refs2;
  std::vector<qtpacket::KhQuadtreeDataReference> qtp2_refs2;
  std::vector<qtpacket::KhQuadtreeDataReference> img_refs2;
  std::vector<qtpacket::KhQuadtreeDataReference> ter_refs2;
  std::vector<qtpacket::KhQuadtreeDataReference> vec_refs2;
  qtpacket::KhQuadtreeDataReferenceGroup refs2(&qtp_refs2,
                                               &qtp2_refs2,
                                               &img_refs2,
                                               &ter_refs2,
                                               &vec_refs2);
  std::string qt_packet1_as_string;
  std::string qt_packet2_as_string;

  KmlFolder folder;

  bool matched = true;
  while (!source1_->IsFinished() && !source2_->IsFinished()) {
    if (source1_->Next().qt_path() < source2_->Next().qt_path()) {
      if (LogMissingQTP("Source 2",
                        KmlLog::kSource2Str,
                        &refs1,
                        &source1_)) {
        count_packets_failed2_.Increment(CrawlerSourceBase::kQuadtree);
        matched = false;
      }
    } else if (source1_->Next().qt_path() > source2_->Next().qt_path()) {
      if (LogMissingQTP("Source 1",
                        KmlLog::kSource1Str,
                        &refs2,
                        &source2_)) {
        count_packets_failed1_.Increment(CrawlerSourceBase::kQuadtree);
        matched = false;
      }
    } else {
      refs1.Reset();
      qtpacket::KhQuadtreeDataReference qtp_read1;
      source1_->GetQtPacketRefs(&qtp_read1, &refs1,
                                false,
                                kml_log_ ? &qt_packet1_as_string : NULL);
      QuadtreePath qt_path1 = qtp_read1.qt_path();
      CrawlerSourceBase::RemoveOddLevelRefs(refs1.ter_refs);
      std::sort(vec_refs1.begin(), vec_refs1.end(), SortLessThan);
      count_packets_read1_.Increment(CrawlerSourceBase::kQuadtree);

      refs2.Reset();
      qtpacket::KhQuadtreeDataReference qtp_read2;
      source2_->GetQtPacketRefs(&qtp_read2, &refs2,
                                false,
                                kml_log_ ? &qt_packet2_as_string : NULL);
      QuadtreePath qt_path2 = qtp_read2.qt_path();
      CrawlerSourceBase::RemoveOddLevelRefs(refs2.ter_refs);
      std::sort(vec_refs2.begin(), vec_refs2.end(), SortLessThan);
      count_packets_read2_.Increment(CrawlerSourceBase::kQuadtree);

      assert(qt_path1 == qt_path2);
      if (kml_log_)
        folder.Reset(qt_path1, "mismatch");

      std::string diffs;
      if (CompareRefGroups(refs1, refs2, &diffs, kml_log_ ? &folder : NULL)) {
        count_packets_matched_.Increment(CrawlerSourceBase::kQuadtree);
      } else {
        count_packets_mismatched_.Increment(CrawlerSourceBase::kQuadtree);
        matched = false;
        notify(NFY_WARN, "Compare mismatch at \"%s\"%s",
               qt_path1.AsString().c_str(),
               diffs.c_str());
        if (kml_log_) {
          folder.AddPoint(qt_path1,
                          KmlLog::kBothStr,
                          "Compare mismatch:" + diffs +
                          "\n<table>\n"
                          "  <tr>\n"
                          "  <th align=center><font color=\"darkblue\">"
                          "Source 1</font></th>\n"
                          "  <th align=center><font color=\"darkred\">"
                          "Source 2</font></th>\n"
                          "  </tr>\n"
                          "  <tr>\n"
                          "    <td><font color=\"darkblue\"><pre>"
                          + qt_packet1_as_string +
                          "</font></pre></td>\n"
                          "    <td><font color=\"darkred\"><pre>"
                          + qt_packet2_as_string +
                          "</font></pre></td>\n"
                          "  </tr>\n"
                          "</table>\n");
          folder.AddBox(qt_path1, KmlLog::kBothStr, "Compare mismatch:");
          kml_log_->LogFolder(folder);
        }
      }
    }
  }

  source1_->Close();
  source1_.clear();
  source2_->Close();
  source2_.clear();
  if (kml_log_) {
    kml_log_->Close();
    kml_log_.clear();
  }

  return matched;
}


bool CrawlCompare::GetPacketContents(
    const qtpacket::KhQuadtreeDataReference &ref,
    const CrawlerSourceBase::PacketType packet_type,
    const std::string src_string,
    khDeleteGuard<CrawlerSourceBase> *source,
    std::ostream *snippet,
    EndianReadBuffer *buffer) {
  bool success = false;
  if (packet_type == CrawlerSourceBase::kImage) {
    // Imagery is JPEG compressed - don't decompress
    success = (*source)->GetPacket(packet_type, ref, buffer);
  } else {
    // Read and decompress terrain or vector
    std::string compressed_buffer;
    success = (*source)->GetPacket(packet_type, ref, &compressed_buffer);
    if (success) {
      success = KhPktDecompress(compressed_buffer.data(),
                                compressed_buffer.size(),
                                buffer);
    }
  }

  if (success) {
    return true;
  } else {
    *snippet << " " << src_string << ": "
             << CrawlerSourceBase::packet_type_name(packet_type)
             << " packet read failed for \"" << ref.qt_path().AsString()
             << "\"";
    if (packet_type == CrawlerSourceBase::kVector)
      *snippet << ", channel " << ref.channel();
    *snippet << ", version " << ref.version();
    return false;
  }
}

bool CrawlCompare::ComparePacketContents(
    const qtpacket::KhQuadtreeDataReference &ref1,
    const qtpacket::KhQuadtreeDataReference &ref2,
    const CrawlerSourceBase::PacketType packet_type,
    std::ostream *diffs,
    KmlFolder *folder) {
  assert(ref1.qt_path() == ref2.qt_path());
  if (packet_type == CrawlerSourceBase::kVector) {
    assert(ref1.channel() == ref2.channel());
  }

  bool success = true;
  std::ostringstream snippet;
  std::ostringstream snippet_suffix;
  std::string style;

  LittleEndianReadBuffer buffer1;
  if (!GetPacketContents(ref1,
                         packet_type,
                         KmlLog::kSource1Str,
                         &source1_,
                         &snippet,
                         &buffer1)) {
    style = KmlLog::kSource1Str;
    count_packets_failed1_.Increment(packet_type);
    success = false;
  } else {
    count_packets_read1_.Increment(packet_type);
  }

  LittleEndianReadBuffer buffer2;
  if (!GetPacketContents(ref2,
                         packet_type,
                         KmlLog::kSource2Str,
                         &source2_,
                         &snippet,
                         &buffer2)) {
    if (style.empty()) {
      style = KmlLog::kSource2Str;
    } else {
      style = KmlLog::kBothStr;
    }
    count_packets_failed2_.Increment(packet_type);
    success = false;
  } else {
    count_packets_read2_.Increment(packet_type);
  }

  if (success) {
    if (packet_type == CrawlerSourceBase::kImage) {
      success = (buffer1 == buffer2);
    } else if (packet_type == CrawlerSourceBase::kVector) {
      success = CompareVectorPackets(&buffer1, &buffer2, &snippet_suffix);
    } else if (packet_type == CrawlerSourceBase::kTerrain) {
      success = CompareTerrainPackets(&buffer1,
                                      &buffer2,
                                      options_.ignore_mesh,
                                      &snippet_suffix);
    } else {
      throw khSimpleException("Internal error, unknown packet type:")
          << packet_type;
    }

    if (success) {
      count_packets_matched_.Increment(packet_type);
    }else {
      count_packets_mismatched_.Increment(packet_type);
      snippet << "    Packet " <<
              CrawlerSourceBase::packet_type_name(packet_type)
              << " contents mismatch at \"" << ref1.qt_path().AsString()
              << "\"";
      if (packet_type == CrawlerSourceBase::kVector) {
        snippet << ", channel " << ref1.channel();
      }
      snippet << ", versions = (" << ref1.version()
              << "," << ref2.version() << ")"
              << ", lengths = (" << buffer1.size()
              << "," << buffer2.size() << ")";
      snippet << snippet_suffix.str();
      style = KmlLog::kBothStr;

      if (options_.dump_packets) {
        std::ostringstream dump;
        DumpPacket(buffer1,
                   source1_->name(),
                   CrawlerSourceBase::packet_type_name(packet_type),
                   ref1.qt_path().AsString(),
                   dump);
        DumpPacket(buffer2,
                   source2_->name(),
                   CrawlerSourceBase::packet_type_name(packet_type),
                   ref2.qt_path().AsString(),
                   dump);
        notify(NFY_INFO, "%s", dump.str().c_str());
      }
    }
  }

  if (success) {
    return true;
  } else {
    *diffs << "\n" << snippet.str();
    if (folder) {
      folder->AddBox(ref1.qt_path(), style, snippet.str());
    }
    return false;
  }
}

bool CrawlCompare::LogMissingQTP(
    const std::string &src_name,
    const std::string &style,
    qtpacket::KhQuadtreeDataReferenceGroup *refs,
    khDeleteGuard<CrawlerSourceBase> *source) {

  // Get the references for this packet
  QuadtreePath qt_path = (*source)->Next().qt_path();
  refs->Reset();
  qtpacket::KhQuadtreeDataReference qtp_read;
  std::string qt_packet_string;
  (*source)->GetQtPacketRefs(&qtp_read, refs,
                             false,
                             kml_log_ ? &qt_packet_string : NULL);
  CrawlerSourceBase::RemoveOddLevelRefs(refs->ter_refs);

  // If no data references, ignore the missing packet
  if (HasNoDataReferences(*refs))
    return false;

  notify(NFY_WARN, "%s: missing quadtree packet \"%s\"",
         src_name.c_str(),
         (*source)->Next().qt_path().AsString().c_str());
  if (kml_log_) {
    kml_log_->LogQtpDiff(qt_path,
                         style,
                         src_name
                         + " missing quadtree packet, other source has:\n"
                         + qt_packet_string);
  }
  return true;
}
} // namespace gecrawler

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


#ifndef FUSION_GECRAWLER_CRAWLCOMPARE_H__
#define FUSION_GECRAWLER_CRAWLCOMPARE_H__

#include <bitset>
#include <iostream>
#include "fusion/gecrawler/crawlersource.h"
#include "fusion/gecrawler/comparevector.h"
#include "fusion/gecrawler/compareterrain.h"
#include "fusion/gecrawler/kmllog.h"
#include "common/packetcompress.h"

class EndianReadBuffer;

namespace gecrawler {

// Output the packet information to the dump_log.
void DumpPacket(const std::string &buffer,
                const std::string &src_name,
                const std::string &packet_type_name,
                const std::string &qt_path_str,
                std::ostream &dump_log);

// CrawlCompare - compare two crawled databases

struct CrawlCompareOptions {
  typedef std::bitset<CrawlerSourceBase::kTypeCount> DataTypeSet;

  CrawlCompareOptions(const DataTypeSet &compare_types_,
                      const DataTypeSet &check_providers_,
                      const bool ignore_mesh_,
                      const bool dump_packets_)
      : compare_types(compare_types_),
        check_providers(check_providers_),
        ignore_mesh(ignore_mesh_),
        dump_packets(dump_packets_) {
  }

  const DataTypeSet compare_types;
  const DataTypeSet check_providers;

  // Ignore terrain mesh connectivity, check vertex locations only
  const bool ignore_mesh;

  // Dump all packets
  const bool dump_packets;
};

class CounterSet {
 public:
  CounterSet()
      : counters_(CrawlerSourceBase::kTypeCount),
        total_(0) {
  }
  ~CounterSet() {}
  inline void Increment(CrawlerSourceBase::PacketType packet_type) {
    ++counters_.at(packet_type);
    ++total_;
  }
  inline std::uint64_t Get(CrawlerSourceBase::PacketType packet_type) const {
    return counters_.at(packet_type);
  }
  inline std::uint64_t GetTotal() const { return total_; }
 private:
  std::vector<std::uint64_t> counters_;
  std::uint64_t total_;
  DISALLOW_COPY_AND_ASSIGN(CounterSet);
};

// CrawlCompareBase has non-templated methods and members to simplify the
// header definition.
class CrawlCompareBase {
 public:
  CrawlCompareBase(const CrawlCompareOptions &options,
                   khTransferGuard<KmlLog> kml_log)
    : options_(options),
      kml_log_(kml_log) {}
  virtual ~CrawlCompareBase() {}
  void FormatCounters(std::ostream *strm) const;
  std::string FormatCounters() const;
 protected:
  const CrawlCompareOptions options_;
  khDeleteGuard<KmlLog> kml_log_;

  CounterSet count_packets_read1_;
  CounterSet count_packets_failed1_;
  CounterSet count_packets_read2_;
  CounterSet count_packets_failed2_;
  CounterSet count_packets_matched_;
  CounterSet count_packets_mismatched_;

  static bool MostlyEqual(const qtpacket::KhQuadtreeDataReference *ref1,
                          const qtpacket::KhQuadtreeDataReference *ref2,
                          const bool check_provider);
  static bool SortLessThan(const qtpacket::KhQuadtreeDataReference &ref1,
                           const qtpacket::KhQuadtreeDataReference &ref2);
  static bool HasNoDataReferences(
      const qtpacket::KhQuadtreeDataReferenceGroup &refs);
  bool CompareRefs(
      const std::vector<qtpacket::KhQuadtreeDataReference> &refs1,
      const std::vector<qtpacket::KhQuadtreeDataReference> &refs2,
      const CrawlerSourceBase::PacketType packet_type,
      const std::string &description,
      std::string *diffs,
      KmlFolder *folder);
  bool CompareRefGroups(
      const qtpacket::KhQuadtreeDataReferenceGroup &group1,
      const qtpacket::KhQuadtreeDataReferenceGroup &group2,
      std::string *diffs,
      KmlFolder *folder);
  virtual bool ComparePacketContents(
      const qtpacket::KhQuadtreeDataReference &ref1,
      const qtpacket::KhQuadtreeDataReference &ref2,
      const CrawlerSourceBase::PacketType packet_type,
      std::ostream *diffs,
      KmlFolder *folder) = 0;
  DISALLOW_COPY_AND_ASSIGN(CrawlCompareBase);
};

// CrawlCompare is a templated class which will crawl templated quadtree
// packets.
class CrawlCompare : public CrawlCompareBase {
 public:
  CrawlCompare(khTransferGuard<CrawlerSourceBase> source1,
               khTransferGuard<CrawlerSourceBase> source2,
               const CrawlCompareOptions &options,
               khTransferGuard<KmlLog> kml_log);
  virtual ~CrawlCompare() {}
  bool Compare();
 private:
  khDeleteGuard<CrawlerSourceBase> source1_;
  khDeleteGuard<CrawlerSourceBase> source2_;

  bool ComparePacketContents(
      const qtpacket::KhQuadtreeDataReference &ref1,
      const qtpacket::KhQuadtreeDataReference &ref2,
      const CrawlerSourceBase::PacketType packet_type,
      std::ostream *diffs,
      KmlFolder *folder);
  bool GetPacketContents(const qtpacket::KhQuadtreeDataReference &ref,
                         const CrawlerSourceBase::PacketType packet_type,
                         const std::string src_string,
                         khDeleteGuard<CrawlerSourceBase> *source,
                         std::ostream *snippet,
                         EndianReadBuffer *buffer);
  bool LogMissingQTP(const std::string &src_name,
                     const std::string &style,
                     qtpacket::KhQuadtreeDataReferenceGroup *refs,
                     khDeleteGuard<CrawlerSourceBase> *source);
  DISALLOW_COPY_AND_ASSIGN(CrawlCompare);
};




} // namespace gecrawler

#endif  // FUSION_GECRAWLER_CRAWLCOMPARE_H__

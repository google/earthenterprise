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

#include "PacketFileAdaptingTraverser.h"
#include <khstl.h>
#include <khException.h>
#include <packetfile/packetfile.h>
#include <packetfile/packetindex.h>

namespace geindex {

template <class TypedBucket>
PacketFileAdaptingTraverserBase<TypedBucket>::PacketFileAdaptingTraverserBase(
    geFilePool &file_pool,
    const std::string &merge_source_name,
    TypedEntry::TypeEnum type,
    std::uint32_t version,
    std::uint32_t channel,
    const std::string packetfile) :
    AdaptingTraverserBase<TypedBucket>(merge_source_name, type),
    packet_index_reader_(
        TransferOwnership(
            new PacketIndexReader(file_pool,
                                  PacketFile::IndexFilename(packetfile)))),
    version_(version),
    channel_(channel),
    have_current_(false)
{
  ReadNext();
  if (!have_current_) {
    QString warn(kh::tr("%1 is empty").arg(packetfile.c_str()));
    notify(NFY_WARN, "%s", warn.latin1());
  }
}

template <class TypedBucket>
PacketFileAdaptingTraverserBase<TypedBucket>::~PacketFileAdaptingTraverserBase(void) {
  // here in cpp so packetfile.h doesn't have to be included in the header
}


template <class TypedBucket>
bool PacketFileAdaptingTraverserBase<TypedBucket>::Advance() {
  ReadNext();
  return have_current_;
}

template <class TypedBucket>
void PacketFileAdaptingTraverserBase<TypedBucket>::Close() {
  have_current_ = false;
  packet_index_reader_.clear();
}

// specialization for AllInfoBucket
template <>
void PacketFileAdaptingTraverserBase<AllInfoBucket>::ReadNext(void) {
  PacketIndexEntry entry;
  have_current_ = packet_index_reader_->ReadNext(&entry);
  if (have_current_) {
    curr_merge_entry_ =
      MergeEntry(entry.qt_path(),
                 makevec1(EntryType(ExternalDataAddress(entry.position(),
                                                        0 /* file num */,
                                                        entry.record_size()),
                                    version_, channel_,
                                    entry.extra(),
                                    type_)));
  }
}

// specialization for UnifiedBucket
template <>
void PacketFileAdaptingTraverserBase<UnifiedBucket>::ReadNext(void) {
  PacketIndexEntry entry;
  have_current_ = packet_index_reader_->ReadNext(&entry);
  if (have_current_) {
    curr_merge_entry_ =
      MergeEntry(entry.qt_path(),
                 makevec1(EntryType(ExternalDataAddress(entry.position(),
                                                        0 /* file num */,
                                                        entry.record_size()),
                                    version_, channel_,
                                    type_)));
  }
}


// ****************************************************************************
// ***  Explicit instantiations
// ****************************************************************************
template class PacketFileAdaptingTraverserBase<UnifiedBucket>;
template class PacketFileAdaptingTraverserBase<AllInfoBucket>;


} // namespace geindex

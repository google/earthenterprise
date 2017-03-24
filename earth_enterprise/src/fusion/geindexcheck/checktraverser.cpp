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


// CheckTraverser - index traverser to check index.

#include <notify.h>
#include "checktraverser.h"

namespace geindexcheck {

void CheckTraverser::OpenPacketFiles(geFilePool &file_pool) {
  const geindex::Header &header = GetIndexBundle().header;
  packet_readers.resize(header.PacketFileCount());

  for (size_t i = 0; i < packet_readers.size(); ++i) {
    std::string file_path = header.GetPacketFile(i);
    if (file_path.size() != 0) {
      packet_readers.Assign(i,
                            new PacketFileReaderBase(file_pool, file_path));
    } else {
      packet_readers.Assign(i, 0);
    }
  }
}

// Read the data for all the entries in the current bucket

template <class BucketType>
void  SpecificCheckTraverser<BucketType>::ReadCurrentEntryData() {
  size_t entry_count = CurrentEntryCount();
  const typename BucketType::SlotStorageType &entries =
      index_traverser_.Current();

  for (size_t i = 0; i < entry_count; ++i) {
    const geindex::ExternalDataAddress &addr = entries[i].dataAddress;
    notify(NFY_VERBOSE, "  Reading entry: file=%lu offset=%llu size=%lu",
           static_cast<unsigned long>(addr.fileNum),
           static_cast<unsigned long long>(addr.fileOffset),
           static_cast<unsigned long>(addr.size));
    if (addr.fileNum >= packet_readers.size()
        ||  packet_readers.at(addr.fileNum) == 0) {
      notify(NFY_FATAL,
             "Location \"%s\" entry %lu "
             "refers to invalid packet file number %lu",
             CurrentPath().AsString().c_str(),
             static_cast<unsigned long>(i),
             static_cast<unsigned long>(addr.fileNum));
      return;
    }

    std::string buffer;
    packet_readers.at(addr.fileNum)->
        ReadAtCRC(addr.fileOffset, buffer, addr.size);
  }
}

// SpecificCheckTraverser<BucketType> explicit instantiations

template class SpecificCheckTraverser<geindex::UnifiedBucket>;
template class SpecificCheckTraverser<geindex::BlendBucket>;
template class SpecificCheckTraverser<geindex::VectorBucket>;

} // namespace geindexcheck

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

#ifndef COMMON_GEINDEX_ENTRIESTMPL_H__
#define COMMON_GEINDEX_ENTRIESTMPL_H__

#include "IndexBundle.h"
#include <third_party/rsa_md5/crc32.h>

namespace geindex {


// ****************************************************************************
// ***  LoadedSingleEntryBucket
// ****************************************************************************
template <class Entry>
LoadedSingleEntryBucket<Entry>::LoadedSingleEntryBucket
(const EntryBucketAddr &addr, IndexBundle &bundle, EndianReadBuffer &tmpBuf) {
  if (addr) {
    bundle.LoadWithCRC(addr, tmpBuf);
    VersionedPull(tmpBuf, bundle.header.fileFormatVersion);
  }
}

template <class Entry>
void LoadedSingleEntryBucket<Entry>::Put(BucketEntrySlotType slot,
                                         const Entry &newEntry) {
  bool isDelete = !bool(newEntry.dataAddress);
  entry_slots[slot] = (isDelete) ? Entry() : newEntry;
}

template <class Entry>
 std::uint16_t LoadedSingleEntryBucket<Entry>::CountSlots(void) const {
  std::uint16_t numSlots = 0;
  for (std::uint16_t i = 0; i < kEntrySlotsPerBucket; ++i) {
    if (entry_slots[i].dataAddress) {
      ++numSlots;
    }
  }
  return numSlots;
}

template <class Entry>
void LoadedSingleEntryBucket<Entry>::Push(EndianWriteBuffer &buf) const {
  EndianWriteBuffer::size_type start = buf.CurrPos();

  std::uint16_t numSlots = this->CountSlots();
  if (numSlots > 0) {
    if (ShouldStoreSinglesSparsely(Entry::VersionedSize
                                   (kCurrentFileFormatVersion), numSlots)) {
      PushSparse(buf, numSlots);
    } else {
      PushFull(buf, numSlots);
    }
    buf << Crc32(&buf[start], buf.CurrPos());
  }
}

template <class Entry>
void LoadedSingleEntryBucket<Entry>::PushSparse(EndianWriteBuffer &buf,
                                                std::uint16_t numSlots) const
{
  // -- sparsely populated --
  // std::uint16_t numSlots;
  // BucketChildSlotType [numSlots]
  // { Entry for first  slot}
  // { Entry for second slot}
  // { Entry for third  slot}

  // leading count
  buf << numSlots;

  // "index" portion
  for (std::uint16_t i = 0; i < kEntrySlotsPerBucket; ++i) {
    if (this->entry_slots[i].dataAddress) {
      buf << EncodeAs<BucketChildSlotType>(i);
    }
  }

  // "entry" portion
  for (std::uint16_t i = 0; i < kEntrySlotsPerBucket; ++i) {
    if (this->entry_slots[i].dataAddress) {
      buf << this->entry_slots[i];
    }
  }
}

template <class Entry>
void LoadedSingleEntryBucket<Entry>::PushFull(EndianWriteBuffer &buf,
                                              std::uint16_t numSlots) const
{
  // -- fully populated --
  // std::uint16_t numSlots;
  // Entry[kEntrySlotsPerBucket]

  // leading count
  buf << numSlots;

  // "entry" portion
  for (std::uint16_t i = 0; i < kEntrySlotsPerBucket; ++i) {
    buf << this->entry_slots[i];
  }
}

template <class Entry>
void LoadedSingleEntryBucket<Entry>::VersionedPull(EndianReadBuffer &buf,
                                                   unsigned int fileFormatVersion) {
  std::uint16_t numSlots;
  buf >> numSlots;
  if (ShouldStoreSinglesSparsely(Entry::VersionedSize(fileFormatVersion),
                                 numSlots)) {
    PullSparse(buf, numSlots, fileFormatVersion);
  } else {
    PullFull(buf, fileFormatVersion);
  }
}

template <class Entry>
void LoadedSingleEntryBucket<Entry>::PullSparse(EndianReadBuffer &buf,
                                                std::uint16_t numSlots,
                                                unsigned int fileFormatVersion) {
  // -- sparsely populated --
  // std::uint16_t numSlots;
  // BucketChildSlotType [numSlots]
  // { Entry for first  slot}
  // { Entry for second slot}
  // { Entry for third  slot}

  std::vector<bool> haveEntry(kEntrySlotsPerBucket, false);

  // numSlots was already pulled by my caller, and passed to me as an argument

  // "index" portion
  for (std::uint16_t i = 0; i < numSlots; ++i) {
    BucketChildSlotType slot;
    buf >> slot;
    haveEntry[slot] = true;
  }

  // "entry" portion
  for (std::uint16_t i = 0; i < kEntrySlotsPerBucket; ++i) {
    if (haveEntry[i]) {
      this->entry_slots[i].VersionedPull(buf, fileFormatVersion);
    }
  }
}

template <class Entry>
void LoadedSingleEntryBucket<Entry>::PullFull(EndianReadBuffer &buf,
                                              unsigned int fileFormatVersion) {
  // -- fully populated --
  // std::uint16_t numSlots;
  // Entry[kEntrySlotsPerBucket]

  // numSlots was already pulled by my caller

  // "entry" portion
  for (std::uint16_t i = 0; i < kEntrySlotsPerBucket; ++i) {
    this->entry_slots[i].VersionedPull(buf, fileFormatVersion);
  }
}




// ****************************************************************************
// ***  LoadedMultipleEntryBucket
// ****************************************************************************
template <class Entry>
LoadedMultipleEntryBucket<Entry>::LoadedMultipleEntryBucket(
    const EntryBucketAddr &addr, IndexBundle &bundle,
    EndianReadBuffer &tmpBuf) {
  if (addr) {
    bundle.LoadWithCRC(addr, tmpBuf);
    VersionedPull(tmpBuf, bundle.header.fileFormatVersion);
  }
}

template <class Entry>
void LoadedMultipleEntryBucket<Entry>::Put(BucketEntrySlotType slot,
                                           const Entry &newEntry) {
  bool isDelete = ! bool(newEntry.dataAddress);
  std::vector<Entry> &entries = entry_slots[slot];

  for (typename std::vector<Entry>::iterator entry = entries.begin();
       entry != entries.end(); ++entry) {
    if (entry->WriteMatches(newEntry)) {
      if (isDelete) {
        entries.erase(entry);
      } else {
        *entry = newEntry;
      }
      return;
    }
  }
  if (!isDelete) {
    entries.push_back(newEntry);
  }
}

template <class Entry>
 std::uint16_t LoadedMultipleEntryBucket<Entry>::CountSlots(void) const {
  std::uint16_t numSlots = 0;
  for (std::uint16_t i = 0; i < kEntrySlotsPerBucket; ++i) {
    if (entry_slots[i].size()) {
      ++numSlots;
    }
  }
  return numSlots;
}

template <class Entry>
void LoadedMultipleEntryBucket<Entry>::Push(EndianWriteBuffer &buf) const {
  EndianWriteBuffer::size_type start = buf.CurrPos();
  std::uint16_t numSlots = this->CountSlots();
  if (numSlots > 0) {
    // leading count
    buf << numSlots;

    // "index" portion
    for (std::uint16_t i = 0; i < kEntrySlotsPerBucket; ++i) {
      if (this->entry_slots[i].size()) {
        buf << EncodeAs<BucketChildSlotType>(i)
            << EncodeAs<std::uint16_t>(this->entry_slots[i].size());
      }
    }

    // "entry" portion
    for (std::uint16_t i = 0; i < kEntrySlotsPerBucket; ++i) {
      for (unsigned int j = 0; j < this->entry_slots[i].size(); ++j) {
        buf << this->entry_slots[i][j];
      }
    }

    buf << Crc32(&buf[start], buf.CurrPos());
  }
}

template <class Entry>
void LoadedMultipleEntryBucket<Entry>::VersionedPull(EndianReadBuffer &buf,
                                                     unsigned int fileFormatVersion) {
  std::vector<std::uint16_t> slotCount(kEntrySlotsPerBucket, 0);

  std::uint16_t numSlots = 0;
  buf >> numSlots;

  // "index" portion
  for (std::uint16_t i = 0; i < numSlots; ++i) {
    BucketChildSlotType slot;
    std::uint16_t count;
    buf >> slot >> count;
    slotCount[slot] = count;
  }

  // "entry" portion
  for (std::uint16_t i = 0; i < kEntrySlotsPerBucket; ++i) {
    if (slotCount[i] > 0) {
      this->entry_slots[i].reserve(slotCount[i]);
      for (unsigned int j = 0; j < slotCount[i]; ++j) {
        Entry tmp;
        tmp.VersionedPull(buf, fileFormatVersion);
        this->entry_slots[i].push_back(tmp);
      }
    }
  }
}


} // namespace geindex


#endif // COMMON_GEINDEX_ENTRIESTMPL_H__

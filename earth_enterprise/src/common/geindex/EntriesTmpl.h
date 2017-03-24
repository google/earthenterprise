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
uint16 LoadedSingleEntryBucket<Entry>::CountSlots(void) const {
  uint16 numSlots = 0;
  for (uint16 i = 0; i < kEntrySlotsPerBucket; ++i) {
    if (entry_slots[i].dataAddress) {
      ++numSlots;
    }
  }
  return numSlots;
}

template <class Entry>
void LoadedSingleEntryBucket<Entry>::Push(EndianWriteBuffer &buf) const {
  EndianWriteBuffer::size_type start = buf.CurrPos();

  uint16 numSlots = this->CountSlots();
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
                                                uint16 numSlots) const
{
  // -- sparsely populated --
  // uint16 numSlots;
  // BucketChildSlotType [numSlots]
  // { Entry for first  slot}
  // { Entry for second slot}
  // { Entry for third  slot}

  // leading count
  buf << numSlots;

  // "index" portion
  for (uint16 i = 0; i < kEntrySlotsPerBucket; ++i) {
    if (this->entry_slots[i].dataAddress) {
      buf << EncodeAs<BucketChildSlotType>(i);
    }
  }

  // "entry" portion
  for (uint16 i = 0; i < kEntrySlotsPerBucket; ++i) {
    if (this->entry_slots[i].dataAddress) {
      buf << this->entry_slots[i];
    }
  }
}

template <class Entry>
void LoadedSingleEntryBucket<Entry>::PushFull(EndianWriteBuffer &buf,
                                              uint16 numSlots) const
{
  // -- fully populated --
  // uint16 numSlots;
  // Entry[kEntrySlotsPerBucket]

  // leading count
  buf << numSlots;

  // "entry" portion
  for (uint16 i = 0; i < kEntrySlotsPerBucket; ++i) {
    buf << this->entry_slots[i];
  }
}

template <class Entry>
void LoadedSingleEntryBucket<Entry>::VersionedPull(EndianReadBuffer &buf,
                                                   uint fileFormatVersion) {
  uint16 numSlots;
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
                                                uint16 numSlots,
                                                uint fileFormatVersion) {
  // -- sparsely populated --
  // uint16 numSlots;
  // BucketChildSlotType [numSlots]
  // { Entry for first  slot}
  // { Entry for second slot}
  // { Entry for third  slot}

  std::vector<bool> haveEntry(kEntrySlotsPerBucket, false);

  // numSlots was already pulled by my caller, and passed to me as an argument

  // "index" portion
  for (uint16 i = 0; i < numSlots; ++i) {
    BucketChildSlotType slot;
    buf >> slot;
    haveEntry[slot] = true;
  }

  // "entry" portion
  for (uint16 i = 0; i < kEntrySlotsPerBucket; ++i) {
    if (haveEntry[i]) {
      this->entry_slots[i].VersionedPull(buf, fileFormatVersion);
    }
  }
}

template <class Entry>
void LoadedSingleEntryBucket<Entry>::PullFull(EndianReadBuffer &buf,
                                              uint fileFormatVersion) {
  // -- fully populated --
  // uint16 numSlots;
  // Entry[kEntrySlotsPerBucket]

  // numSlots was already pulled by my caller

  // "entry" portion
  for (uint16 i = 0; i < kEntrySlotsPerBucket; ++i) {
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
uint16 LoadedMultipleEntryBucket<Entry>::CountSlots(void) const {
  uint16 numSlots = 0;
  for (uint16 i = 0; i < kEntrySlotsPerBucket; ++i) {
    if (entry_slots[i].size()) {
      ++numSlots;
    }
  }
  return numSlots;
}

template <class Entry>
void LoadedMultipleEntryBucket<Entry>::Push(EndianWriteBuffer &buf) const {
  EndianWriteBuffer::size_type start = buf.CurrPos();
  uint16 numSlots = this->CountSlots();
  if (numSlots > 0) {
    // leading count
    buf << numSlots;

    // "index" portion
    for (uint16 i = 0; i < kEntrySlotsPerBucket; ++i) {
      if (this->entry_slots[i].size()) {
        buf << EncodeAs<BucketChildSlotType>(i)
            << EncodeAs<uint16>(this->entry_slots[i].size());
      }
    }

    // "entry" portion
    for (uint16 i = 0; i < kEntrySlotsPerBucket; ++i) {
      for (uint j = 0; j < this->entry_slots[i].size(); ++j) {
        buf << this->entry_slots[i][j];
      }
    }

    buf << Crc32(&buf[start], buf.CurrPos());
  }
}

template <class Entry>
void LoadedMultipleEntryBucket<Entry>::VersionedPull(EndianReadBuffer &buf,
                                                     uint fileFormatVersion) {
  std::vector<uint16> slotCount(kEntrySlotsPerBucket, 0);

  uint16 numSlots = 0;
  buf >> numSlots;

  // "index" portion
  for (uint16 i = 0; i < numSlots; ++i) {
    BucketChildSlotType slot;
    uint16 count;
    buf >> slot >> count;
    slotCount[slot] = count;
  }

  // "entry" portion
  for (uint16 i = 0; i < kEntrySlotsPerBucket; ++i) {
    if (slotCount[i] > 0) {
      this->entry_slots[i].reserve(slotCount[i]);
      for (uint j = 0; j < slotCount[i]; ++j) {
        Entry tmp;
        tmp.VersionedPull(buf, fileFormatVersion);
        this->entry_slots[i].push_back(tmp);
      }
    }
  }
}


} // namespace geindex


#endif // COMMON_GEINDEX_ENTRIESTMPL_H__

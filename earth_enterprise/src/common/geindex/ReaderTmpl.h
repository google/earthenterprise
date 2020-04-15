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

#ifndef COMMON_GEINDEX_READERTMPL_H__
#define COMMON_GEINDEX_READERTMPL_H__

#include "IndexBundleReader.h"
#include <khEndian.h>

namespace geindex {


// ****************************************************************************
// ***  Template function implementations for geindex::Reader<Entry>
// ***
// ***  These are in a separate header so we don't have to polute the public
// ***  header with all the type information necessary for the implementation.
// ***
// ***  To create a Reader<> for your own Entry type you will need to include
// ***  this header in one of your cpp files and explicitily instantiate
// ***  the Reader<> specialization. See the end of Reader.cpp for an example.
// ****************************************************************************
template <class Entry>
void Reader<Entry>::GetEntry(const QuadtreePath &targetEntryPath,
                             const ReadKey &key,
                             Entry *entryReturn,
                             ReadBuffer &tmpBuf) {
  const BucketPath entryBucketPath(targetEntryPath);

  // Get the disk address of the EntryBucket
  EntryBucketAddr entryBucketAddr =
    GetEntryBucketAddr(entryBucketPath, tmpBuf);
  if (!entryBucketAddr) {
    // we don't have an address, nothing exists at this path
    throw khSimpleNotFoundException("No entries at '")
      << targetEntryPath.AsString()
      << "'";
  }

  // load the raw bits from disk
  bundleReader->LoadWithCRC(entryBucketAddr, tmpBuf);

  // buckets that can only store one entry per slot store their buckets
  // differently
  const BucketEntrySlotType slot = entryBucketPath.SubAddrAsEntrySlot(targetEntryPath);
  if (bundleReader->header.slotsAreSingle) {
    if (GetEntrySingle(slot, key, entryReturn, tmpBuf)) {
      return;
    }
  } else if (GetEntryMultiple(slot, key, entryReturn, tmpBuf)) {
    return;
  }

  throw khSimpleNotFoundException("No matching entry found at '")
    << targetEntryPath.AsString()
    << "'";
}

template <class Entry>
void Reader<Entry>::GetData(const QuadtreePath &targetEntryPath,
                            const ReadKey &key,
                            ReadBuffer &buf) {
  Entry entry;
  GetEntry(targetEntryPath, key, &entry, buf);

  LoadExternalData(entry.dataAddress, buf);
}


template <class Entry>
bool Reader<Entry>::GetNearestAncestorEntry(const QuadtreePath &targetEntryPath,
                                            const ReadKey &key,
                                            Entry *entryReturn,
                                            ReadBuffer &buffer,
                                            QuadtreePath* pathReturned) {
  if (!targetEntryPath.Level()) {
    return false;  // We've reached the top of the quadtree.
  }

  BucketPath entryBucketPath(targetEntryPath);
  BucketEntrySlotType slot = entryBucketPath.SubAddrAsEntrySlot(targetEntryPath);

  // Get the disk address of the EntryBucket
  EntryBucketAddr entryBucketAddr =
    GetEntryBucketAddr(entryBucketPath, buffer);
  if (entryBucketAddr) {
    // load the raw bits from disk
    bundleReader->LoadWithCRC(entryBucketAddr, buffer);

    // buckets that can only store one entry per slot store their buckets
    // differently
    if (bundleReader->header.slotsAreSingle) {
      if (GetEntrySingle(slot, key, entryReturn, buffer)) {
        *pathReturned = targetEntryPath;
        return true;
      }
    } else if (GetEntryMultiple(slot, key, entryReturn, buffer)) {
      *pathReturned = targetEntryPath;
      return true;
    }
  }

  // If we reach here, we have not found the entry in the index.
  // Search for the parent of the target path with a recursive call.
  QuadtreePath parentPath = targetEntryPath.Parent();
  return GetNearestAncestorEntry(parentPath, key, entryReturn,
                                 buffer, pathReturned);
}

template <class Entry>
void Reader<Entry>::GetNearestAncestorData(const QuadtreePath &targetEntryPath,
                                           const ReadKey &key,
                                           ReadBuffer &buf,
                                           QuadtreePath* pathReturned) {
  Entry entry;
  if (!GetNearestAncestorEntry(targetEntryPath, key, &entry, buf, pathReturned)) {
    // We've recursed to level 0 without finding an entry...throw a not found.
    throw khSimpleNotFoundException("No entries at '")
      << targetEntryPath.AsString() << "'";
  }

  LoadExternalData(entry.dataAddress, buf);
}

template <class Entry>
bool Reader<Entry>::GetEntrySingle(BucketEntrySlotType slot,
                                   const ReadKey &key,
                                   Entry *entryReturn,
                                   ReadBuffer &buf) {

  std::uint32_t entrySize = Entry::VersionedSize(fileFormatVersion);
  std::uint16_t numSlots;
  buf >> numSlots;

  // Single entries are stored in one of two ways
  if (ShouldStoreSinglesSparsely(entrySize, numSlots)) {
    // -- sparsely populated --
    // std::uint16_t numSlots;
    // BucketChildSlotType [numSlots]
    // { Entry for first  slot}
    // { Entry for second slot}
    // { Entry for third  slot}

    std::uint16_t skipEntryCount = 0;
    for (std::uint16_t i = 0; i < numSlots; ++i) {
      BucketChildSlotType currSlot;
      buf >> currSlot;
      if (currSlot != slot) {
        // not the slot we want
        ++skipEntryCount;
      } else {
        // We found the slot we want: skip past the rest of the slot
        // ids and the entries for the slots in front of me
        buf.Skip((numSlots - 1 - i) * sizeof(currSlot)
                 + skipEntryCount * entrySize);

        entryReturn->VersionedPull(buf, fileFormatVersion);
        return entryReturn->ReadMatches(key);
      }
    }
  } else {
    // -- fully populated --
    // std::uint16_t numSlots;
    // Entry[kEntrySlotsPerBucket]

    // skip the entries for the slots before me
    buf.Skip(slot * entrySize);

    // pull my entry
    entryReturn->VersionedPull(buf, fileFormatVersion);

    // check for a valid address, in fulled populted mode, there may be some
    // slots that aren't actually valid
    // check to make sure our key matches too
    return entryReturn->dataAddress && entryReturn->ReadMatches(key);
  }
  return false;
}

template <class Entry>
bool Reader<Entry>::GetEntryMultiple(BucketEntrySlotType slot,
                                     const ReadKey &key,
                                     Entry *entryReturn,
                                     ReadBuffer &buf) {

  std::uint32_t entrySize = Entry::VersionedSize(fileFormatVersion);

  // Stored as
  // std::uint16_t numSlots;
  // { BucketChildSlotType slot; std::uint16_t count; } [numSlots]
  // { entries for first  slot }
  // { entries for second slot }
  // { entries for third  slot }

  std::uint16_t numSlots;
  buf >> numSlots;
  std::uint32_t skipEntryCount = 0;
  for (std::uint16_t i = 0; i < numSlots; ++i) {
    BucketChildSlotType currSlot;
    std::uint16_t count;
    buf >> currSlot >> count;
    if (currSlot != slot) {
      // not the slot we want
      skipEntryCount += count;
    } else {
      // we found the slot we want
      // skip past the rest of the slot counts
      buf.Skip((numSlots - 1 - i) * (sizeof(currSlot)+sizeof(count)));

      // skip past the entries for the slots in front of me
      buf.Skip(skipEntryCount * entrySize);

      return FindMatchingEntryInMultiSlot(key, entryReturn, fileFormatVersion,
                                          buf, count, this);
    }
  }
  return false;
}


} // namespace geindex


#endif // COMMON_GEINDEX_READERTMPL_H__

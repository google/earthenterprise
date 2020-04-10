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

#ifndef COMMON_GEINDEX_READER_H__
#define COMMON_GEINDEX_READER_H__


#include "common/khGuard.h"
#include "common/khEndian.h"
#include "common/khThread.h"
#include "common/geindex/Entries.h"
#include "common/geindex/IndexBundleReader.h"
#include "common/geindex/shared.h"

class geFilePool;
class EndianReadBuffer;
class PacketFileReaderBase;
class LittleEndianReadBuffer;

namespace geindex {

class IndexBundleReader;
class ExternalDataAddress;

namespace readerimpl {

// -1 -> unknown, 0 -> no such bucket, 1 -> index into bucketIds below
typedef std::int32_t ChildBucketId;
static const ChildBucketId kUnknownBucket = -1;
static const ChildBucketId kNonExistentBucket = 0;
static const std::uint32_t kInvalidAddrIndex = 0;

class ChildBucketCache;

class ReaderBase {
 public:
  ~ReaderBase(void);

  std::uint32_t GetPacketExtra(std::uint32_t packetfile_num);
 protected:
  ReaderBase(geFilePool &filepool_, const std::string &filename,
             unsigned int numBucketLevelsCached);

  ChildBucketId
  FetchCachedChildBucketId(const BucketPath &targetChildBucketPath,
                           EndianReadBuffer &tmpBuf);

  EntryBucketAddr
  GetEntryBucketAddr(const BucketPath &targetEntryBucketPath,
                     EndianReadBuffer &tmpBuf);

  void LoadExternalData(const ExternalDataAddress &addr,
                        EndianReadBuffer &buf);

  const unsigned int NumBucketLevelsCached;
  khDeleteGuard<IndexBundleReader> bundleReader;
  const unsigned int fileFormatVersion;

 private:
  ChildBucketAddr LoadChildBucketToGetChildAddr
  (ChildBucketAddr addr, const BucketPath &subpath, EndianReadBuffer &tmpBuf);

  EntryBucketAddr LoadChildBucketToGetEntryAddr
  (ChildBucketAddr addr, const BucketPath &subpath, EndianReadBuffer &tmpBuf);

  PacketFileReaderBase* GetPacketFileReader(std::uint32_t fileNum);


  geFilePool &filePool;
  khMutex cacheMutex;
  khDeleteGuard<ChildBucketCache> cache;
  khMutex packetfileMutex;
  khDeletingVector<PacketFileReaderBase> packetFileReaders;
};


}  // namespace geindex::readerimpl


extern const std::uint32_t kMaxLevelsCached;

// ****************************************************************************
// ***  Reader<Entry>
// ***
// ***  Entry can be any class that follows the pattern for entries as
// ***  specified in Entries.h
// ***
// ***  Some member functions are defined in ReaderTmpl.h. See the
// ***  instructions at the top of that header for how to explicitly
// ***  instantiate this class for your 'Entry'
// ****************************************************************************
template <class Entry>
class Reader : public readerimpl::ReaderBase
{
 public:
  typedef Entry EntryType;
  typedef typename Entry::ReadKey ReadKey;
  typedef LittleEndianReadBuffer ReadBuffer;

  Reader(geFilePool &filepool, const std::string &filename,
         unsigned int numBucketLevelsCached)
      : ReaderBase(filepool, filename, numBucketLevelsCached) {}
  ~Reader(void) {}

  // Will throw if unable to satisfy request
  // defined in ReaderTmpl.h
  //
  // @param targetEntryPath quad-tree-path to the packet requested.
  // @param ReadKey details of what to read (version, channel(layer_id fixed for
  // imagery/terrain), request_type (imagery/terrain/vector...)).
  // @param entryReturn Entry ({filenum, offset, size}, version, channel)
  // to the bundle file for the data, returned by entryReturn.
  // @param tmpBuf ReadBuffer for reuse, it is set and used only inside the
  // method internally and nothing is returned by this, only a param to avoid
  // statics as this needs to be thread-safe.
  void GetEntry(const QuadtreePath &targetEntryPath, const ReadKey &key,
                Entry *entryReturn, ReadBuffer &tmpBuf);
  // This method reuses buf to call to GetEntry. But finally loads the data in
  // buf and returns by that (pass by reference) variable.
  void GetData(const QuadtreePath &targetEntryPath, const ReadKey &key,
               ReadBuffer &buf, const bool size_only);

  // defined in ReaderTmpl.h
  //
  // Get the data entry of the nearest ancestor. Return false if none found.
  // targetEntryPath: the quadnode path that we're looking for.
  // key: the read key for the entry.
  // entryReturn: the returned entry.
  // @param buffer ReadBuffer for reuse, it is set and used only inside the
  // method internally and nothing is returned by this, only a param to avoid
  // statics as this needs to be thread-safe.
  // pathReturned: the quadtree path of the entry that is found. This may be
  //               equal to targetEntryPath but may be an ancestor if the entry
  //               does not exist.
  // returns true if an entry is found, false otherwise.
  bool GetNearestAncestorEntry(const QuadtreePath &targetEntryPath,
                               const ReadKey &key,
                               Entry *entryReturn,
                               ReadBuffer &buffer,
                               QuadtreePath* pathReturned);

  // Get the data packet of the nearest ancestor. Throw if none found.
  // targetEntryPath: the quadnode path that we're looking for.
  // key: the read key for the entry.
  // buf: the output buffer for the data being read.
  // pathReturned: the quadtree path of the entry that is found. This may be
  //               equal to targetEntryPath but may be an ancestor if the entry
  //               does not exist.
  void GetNearestAncestorData(const QuadtreePath &targetEntryPath, const ReadKey &key,
                              ReadBuffer &buf, QuadtreePath* pathReturned);

 private:
  bool GetEntrySingle(BucketEntrySlotType slot, const ReadKey &key,
                      Entry *entryReturn, ReadBuffer &buf);
  bool GetEntryMultiple(BucketEntrySlotType slot, const ReadKey &key,
                        Entry *entryReturn, ReadBuffer &buf);
};

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
  const BucketEntrySlotType slot =
      entryBucketPath.SubAddrAsEntrySlot(targetEntryPath);
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
                            ReadBuffer &buf,
                            const bool size_only) {
  Entry entry;
  GetEntry(targetEntryPath, key, &entry, buf);
  if (size_only) {
    std::ostringstream stream;
    stream << (entry.dataAddress.size > FileBundle::kCRCsize ?
           (entry.dataAddress.size - FileBundle::kCRCsize) : 0);
    buf.SetValue(stream.str());
  } else {
    LoadExternalData(entry.dataAddress, buf);
  }
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
  BucketEntrySlotType slot =
      entryBucketPath.SubAddrAsEntrySlot(targetEntryPath);
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
  if (!GetNearestAncestorEntry(
      targetEntryPath, key, &entry, buf, pathReturned)) {
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

typedef Reader<TypedEntry> UnifiedReader;
typedef Reader<BlendEntry> BlendReader;

} // namespace geindex



#endif // COMMON_GEINDEX_READER_H__

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

#ifndef COMMON_GEINDEX_ENTRIES_H__
#define COMMON_GEINDEX_ENTRIES_H__

#include <cstdint>

#include "shared.h"

class EndianReadBuffer;
class EndianWriteBuffer;


// ****************************************************************************
// ***  Entries reference external data packets
// ***
// ***  Every entry must contain at least the following:
// ***  - ExternalDataAddress dataAddress - This actually references the
// ***    external data packet on disk.
// ***  - std::uint32_t version
// ***  - ReadKey class and a ReadMatches() function
// ***  - If more than one entry can exist in a Quadnode, a WriteMatches()
// ***    function must also exist.
// ****************************************************************************


namespace geindex {


class IndexBundle;


bool ShouldStoreSinglesSparsely(std::uint32_t entrySize, std::uint16_t numSlots);


class ExternalDataAddress {
 public:
  inline ExternalDataAddress(void) : fileOffset(0), fileNum(0), size(0) { }
  inline ExternalDataAddress(std::uint64_t fileOffset_, std::uint32_t fileNum_,
                             std::uint32_t size_) :
      fileOffset(fileOffset_), fileNum(fileNum_), size(size_) { }
  inline operator bool(void) const { return size != 0; }
  inline bool operator==(const ExternalDataAddress &o) const {
    return ((fileOffset == o.fileOffset) &&
            (fileNum == o.fileNum) &&
            (size == o.size));
  }
  inline bool operator!=(const ExternalDataAddress &o) const {
    return !operator==(o);
  }

  void Push(EndianWriteBuffer &buf) const;
  void Pull(EndianReadBuffer &buf);
  static inline std::uint32_t Size(void) {
    return (sizeof(std::uint64_t) + sizeof(std::uint32_t) + sizeof(std::uint32_t));
  }

  std::uint64_t fileOffset; // offset w/in the packetfile
  std::uint32_t fileNum;    // which packetfile in IndexBundle::Header::packetfiles
  std::uint32_t size;       // size of the block
};



// ****************************************************************************
// ***  SimpleInsetEntry
// ***
// ***  Like SimpleEntry but also holds the insetId (for later use by the
// ***  QTPacket generator)
// ***  NOTE: insetId is not part of the read key, only version is.
// ***  NOTE: This doesn't derive from SimpleEntry because we don't want the
// ***  extra padding bytes added to the structure
// ***
// ***  Useful for combined Tmesh packets
// ****************************************************************************
class SimpleInsetEntry {
 public:
  class ReadKey {
   public:
    std::uint32_t version;
    ReadKey(std::uint32_t version_) : version(version_) { }
  };
  inline bool ReadMatches(const ReadKey &key) {
    return (version == key.version);
  }
  inline bool WriteMatches(const SimpleInsetEntry &key) { return true; }

  SimpleInsetEntry(void) : dataAddress(), version(), insetId() { }
  SimpleInsetEntry(ExternalDataAddress dataAddress_, std::uint32_t version_,
                      std::uint32_t insetId_) :
      dataAddress(dataAddress_), version(version_),
      insetId(insetId_)
  { }
  void Push(EndianWriteBuffer &buf) const;
  void VersionedPull(EndianReadBuffer &buf, std::uint16_t fileFormatVersion);
  static inline std::uint32_t VersionedSize(std::uint16_t) {
    return (ExternalDataAddress::Size() + sizeof(std::uint32_t) + sizeof(std::uint32_t));
  }

  ExternalDataAddress dataAddress;
  std::uint32_t version;
  std::uint32_t insetId;
};

// ****************************************************************************
// ***  BlendEntry
// ***
// ***  Used only for intermediate indexes for imagery and terrain
// ***
// ***  Multiple blend tiles can exist at each quad path. But only one from
// ***  each packetfile. The packetfile number will later be used to look up
// ***  the order in the blend stack. This order will then be used to chose
// ***  which of the many to export to the next level index.
// ***
// ***  This is only used for random acces by the GUI, but that method doesn't
// ***  use a ReadKey.
// ****************************************************************************
class BlendEntry {
 public:
  class ReadKey { };  // not used, but needs to be defined in order to compile
  inline bool ReadMatches(const ReadKey &key) { return true; }


  inline bool WriteMatches(const BlendEntry &key) {
    return (dataAddress.fileNum == key.dataAddress.fileNum);
  }

  BlendEntry(void) : dataAddress(), version(), insetId() { }
  BlendEntry(ExternalDataAddress dataAddress_, std::uint32_t version_,
             std::uint32_t insetId_) :
      dataAddress(dataAddress_), version(version_),
      insetId(insetId_)
  { }
  void Push(EndianWriteBuffer &buf) const;
  void VersionedPull(EndianReadBuffer &buf, std::uint16_t fileFormatVersion);
  static inline std::uint32_t VersionedSize(std::uint16_t) {
    return (ExternalDataAddress::Size() + sizeof(std::uint32_t) + sizeof(std::uint32_t));
  }

  ExternalDataAddress dataAddress;
  std::uint32_t version;
  std::uint32_t insetId;
};

// ****************************************************************************
// ***  ChannelledEntry
// ***
// ***  Useful for existing Vector packets
// ***
// ***  As this is never used to random access reading, it has no ReadKey
// ****************************************************************************
class ChannelledEntry {
 public:
  inline bool WriteMatches(const ChannelledEntry &newEntry) {
    return (channel == newEntry.channel);
  }

  ChannelledEntry(void) : dataAddress(), version(),
                          channel() { }
  ChannelledEntry(ExternalDataAddress dataAddress_,
                  std::uint32_t version_, std::uint32_t channel_) :
      dataAddress(dataAddress_), version(version_),
      channel(channel_) { }
  void Push(EndianWriteBuffer &buf) const;
  void VersionedPull(EndianReadBuffer &buf, std::uint16_t fileFormatVersion);
  static inline std::uint32_t VersionedSize(std::uint16_t) {
    return (ExternalDataAddress::Size() + sizeof(std::uint32_t) + sizeof(std::uint32_t) +
            sizeof(std::uint32_t));
  }

  ExternalDataAddress dataAddress;
  std::uint32_t      version;
  std::uint32_t      channel;
};


// ****************************************************************************
// ***  TypedEntry
// ***
// ***  This is used in a combined index.
// ***  The provider is no longer needed since QtPackets will already have
// ***  been built.
// ***
// ***  Useful for the final unified index that indexes Imagery, Terrain,
// ***  Vector, and QTPackets
// ****************************************************************************
class TypedEntry {
 public:
  // NOTE: if you add to this enum, make sure you update ToString and
  // FromString in the cpp file
  enum TypeEnum { QTPacket, Imagery, Terrain,
                  VectorGE, VectorMaps, VectorMapsRaster,
                  Unified, DatedImagery, QTPacket2 };

  class ReadKey {
   public:
    std::uint32_t   version;
    std::uint32_t   channel;
    TypeEnum type;
    bool     version_matters;
    ReadKey(std::uint32_t version_, std::uint32_t channel_, TypeEnum type_,
            bool version_matters_ = true) :
        version(version_), channel(channel_), type(type_),
        version_matters(version_matters_)
    { }
  };
  inline bool ReadMatches(const ReadKey &key) {
    return ((channel == key.channel) && (type == key.type) &&
            (!key.version_matters || (version == key.version)));
  }
  inline bool WriteMatches(const TypedEntry &newEntry) {
    return ((type == newEntry.type) && (channel == newEntry.channel));
  }

  TypedEntry(void) : dataAddress(), version(), channel(), type() { }
  TypedEntry(ExternalDataAddress dataAddress_, std::uint32_t version_,
             std::uint32_t channel_, TypeEnum type_) :
      dataAddress(dataAddress_), version(version_),
      channel(channel_), type(type_) { }

  // ignore the provider
  TypedEntry(const SimpleInsetEntry &o, TypeEnum type_) :
      dataAddress(o.dataAddress), version(o.version), channel(),
      type(type_) { }
  TypedEntry(const BlendEntry &o, TypeEnum type_, std::uint32_t channel_ = 0) :
      dataAddress(o.dataAddress), version(o.version), channel(channel_),
      type(type_) { }
  TypedEntry(const ChannelledEntry &o, TypeEnum type_) :
      dataAddress(o.dataAddress), version(o.version), channel(o.channel),
      type(type_) { }

  void Push(EndianWriteBuffer &buf) const;
  void VersionedPull(EndianReadBuffer &buf, std::uint16_t fileFormatVersion);
  static inline std::uint32_t VersionedSize(std::uint16_t) {
    return (ExternalDataAddress::Size() + sizeof(std::uint32_t) + sizeof(std::uint32_t) +
            sizeof(std::uint8_t));
  }


  ExternalDataAddress dataAddress;
  std::uint32_t      version;
  std::uint32_t      channel;
  TypeEnum    type;
};

// ****************************************************************************
// ***  AllInfoEntry
// ***
// ***  Used by those places that want _all_ the information that may have
// ***  been stored in the more specific types.
// ***
// ***  Currently used by QTPacket creator. Should be used by debug/dump tools
// ***  as well. Who knows? Maybe it is by now. :-)
// ***
// ***  As this is never used to random access reading, it has no ReadKey
// ****************************************************************************
class AllInfoEntry {
 public:

  AllInfoEntry(void) : dataAddress(), version(), channel(),
                       insetId(), type() { }
  AllInfoEntry(ExternalDataAddress dataAddress_, std::uint32_t version_,
                     std::uint32_t channel_, std::uint32_t insetId_,
                     TypedEntry::TypeEnum type_) :
      dataAddress(dataAddress_), version(version_),
      channel(channel_), insetId(insetId_), type(type_) { }

  AllInfoEntry(const SimpleInsetEntry &o, TypedEntry::TypeEnum type_) :
      dataAddress(o.dataAddress), version(o.version), channel(),
      insetId(o.insetId), type(type_) { }
  AllInfoEntry(const BlendEntry &o, TypedEntry::TypeEnum type_,
             std::uint32_t channel_) :
      dataAddress(o.dataAddress), version(o.version), channel(channel_),
      insetId(o.insetId), type(type_) { }
  AllInfoEntry(const ChannelledEntry &o, TypedEntry::TypeEnum type_) :
      dataAddress(o.dataAddress), version(o.version), channel(o.channel),
      insetId(), type(type_) { }
  AllInfoEntry(const TypedEntry &o) :
      dataAddress(o.dataAddress), version(o.version), channel(o.channel),
      insetId(), type(o.type) { }

  operator TypedEntry(void) const {
    return TypedEntry(dataAddress, version, channel, type);
  }

  ExternalDataAddress  dataAddress;
  std::uint32_t               version;
  std::uint32_t               channel;
  std::uint32_t               insetId;
  TypedEntry::TypeEnum type;
};


typedef SimpleInsetEntry CombinedTmeshEntry;
// typedef SimpleInsetEntry TmeshEntry;
typedef ChannelledEntry     VectorEntry;
typedef TypedEntry          UnifiedEntry;



// ****************************************************************************
// ***  LoadedSingleEntryBucket
// ***
// ***  A SingleEntryBucket that has been loaded from disk. The Writer has
// ***  classes that derive from this to store it's write cache. The
// ***  MergeSource has classes that derive from this to store its read
// ***  cache.
// ****************************************************************************
template <class Entry>
class LoadedSingleEntryBucket {
 public:
  typedef Entry EntryType;
  typedef Entry SlotStorageType;
  static const bool isSingle = true;

  LoadedSingleEntryBucket(const EntryBucketAddr &addr, IndexBundle &bundle,
                          EndianReadBuffer &tmpBuf);

  std::uint16_t CountSlots(void) const;
  void Put(BucketEntrySlotType slot, const Entry &newEntry);
  void Push(EndianWriteBuffer &buf) const;
  void VersionedPull(EndianReadBuffer &buf, unsigned int fileFormatVersion);
  inline const SlotStorageType &EntrySlot(BucketEntrySlotType index) const {
    assert(index < kEntrySlotsPerBucket);
    return entry_slots[index];
  }
  inline bool IsSlotEmpty(BucketEntrySlotType index) const {
    assert(index < kEntrySlotsPerBucket);
    return !entry_slots[index].dataAddress;
  }

 protected:
  SlotStorageType entry_slots[kEntrySlotsPerBucket];

 private:
  void PushSparse(EndianWriteBuffer &buf, std::uint16_t numSlots) const;
  void PushFull(EndianWriteBuffer &buf, std::uint16_t numSlots) const;
  void PullSparse(EndianReadBuffer &buf, std::uint16_t numSlots,
                  unsigned int fileFormatVersion);
  void PullFull(EndianReadBuffer &buf, unsigned int fileFormatVersion);
};

// ****************************************************************************
// ***  LoadedMultipleEntryBucket
// ***
// ***  A MultipleEntryBucket that has been loaded from disk. The Writer has
// ***  classes that derive from this to store it's write cache. The
// ***  MergeSource has classes that derive from this to store its read
// ***  cache.
// ****************************************************************************
template <class Entry>
class LoadedMultipleEntryBucket {
 public:
  typedef Entry EntryType;
  typedef std::vector<Entry> SlotStorageType;
  static const bool isSingle = false;

  LoadedMultipleEntryBucket(const EntryBucketAddr &addr, IndexBundle &bundle,
                            EndianReadBuffer &tmpBuf);

  std::uint16_t CountSlots(void) const;
  void Put(BucketEntrySlotType slot, const Entry &newEntry);
  void Push(EndianWriteBuffer &buf) const;
  void VersionedPull(EndianReadBuffer &buf, unsigned int fileFormatVersion);
  inline const SlotStorageType &EntrySlot(BucketEntrySlotType index) const {
    assert(index < kEntrySlotsPerBucket);
    return entry_slots[index];
  }
  inline bool IsSlotEmpty(BucketEntrySlotType index) const {
    assert(index < kEntrySlotsPerBucket);
    return entry_slots[index].empty();
  }

 protected:
  SlotStorageType entry_slots[kEntrySlotsPerBucket];
};

template <class Entry>
class TypedefOnlyMultipleEntryBucket {
 public:
  typedef Entry EntryType;
  typedef std::vector<Entry> SlotStorageType;
  static const bool isSingle = false;
};



typedef LoadedSingleEntryBucket<CombinedTmeshEntry> CombinedTmeshBucket;
// typedef LoadedSingleEntryBucket<TmeshEntry>         TmeshBucket;
typedef LoadedMultipleEntryBucket<BlendEntry>       BlendBucket;
typedef LoadedMultipleEntryBucket<VectorEntry>      VectorBucket;
typedef LoadedMultipleEntryBucket<UnifiedEntry>     UnifiedBucket;
typedef TypedefOnlyMultipleEntryBucket<AllInfoEntry>     AllInfoBucket;




// ****************************************************************************
// ***  Conversion routines to convert from a slot full of one of the other
// ***  Entry types to a slot full of TypedEntries of TypedProviderEntries
// ****************************************************************************

// general case for singleton entry slots
template <class TypedSlot, class Entry, class Helper>
void PopulateTypedSlot(TypedSlot *typed_slot,
                       TypedEntry::TypeEnum type,
                       const Entry &entry, Helper *,
                       std::uint32_t /* ignored channelid */,
                       std::uint32_t /* ignored version */)
{
  typedef typename TypedSlot::value_type TypedEntryType;

  typed_slot->clear();
  typed_slot->push_back(TypedEntryType(entry, type));
}

// general case for multiple entry slots
template <class TypedSlot, class Entry, class Helper>
void PopulateTypedSlot(TypedSlot *typed_slot,
                       TypedEntry::TypeEnum type,
                       const std::vector<Entry> &entries, Helper *,
                       std::uint32_t /* ignored channelid */,
                       std::uint32_t /* ignored version */)
{
  typedef typename TypedSlot::value_type TypedEntryType;

  typed_slot->clear();
  for (unsigned int i = 0; i < entries.size(); ++i) {
    typed_slot->push_back(TypedEntryType(entries[i], type));
  }
}

// general case for TypedEntry slots
template <class TypedSlot, class Helper>
void PopulateTypedSlot(TypedSlot *typed_slot,
                       TypedEntry::TypeEnum type,
                       const std::vector<TypedEntry> &entries, Helper *,
                       std::uint32_t /* ignored channelid */,
                       std::uint32_t /* ignored version */)
{
  typedef typename TypedSlot::value_type TypedEntryType;

  typed_slot->clear();
  for (unsigned int i = 0; i < entries.size(); ++i) {
    typed_slot->push_back(TypedEntryType(entries[i]));
  }
}

// special case for slots full of BlendEntries
// Muliple source BelndEntries will map to a single Typed Entry
// The one with the highest stack number wins
template <class TypedSlot, class Helper>
void PopulateTypedSlot(TypedSlot *typed_slot,
                       TypedEntry::TypeEnum type,
                       const std::vector<BlendEntry> &entries, Helper *helper,
                       std::uint32_t channel_id,
                       std::uint32_t version)
{
  typedef typename TypedSlot::value_type TypedEntryType;

  typed_slot->clear();

  // figure out which one we want
  unsigned int want = 0;
  std::uint32_t pos = 0;
  for (unsigned int i = 0; i < entries.size(); ++i) {
    std::uint32_t newpos = helper->GetPacketExtra(entries[i].dataAddress.fileNum);
    if (newpos > pos) {
      pos = newpos;
      want = i;
    }
  }

  TypedEntryType entry = TypedEntryType(entries[want], type, channel_id);
  if (version != 0) {
    entry.version = version;
  }
  typed_slot->push_back(entry);
}



// ****************************************************************************
// ***  Helper routines to random access reader
// ***  Used to find matching entry from a multi-entry slot
// ***
// ***  NOTE: These routines need to know how multi-entry slots are stored
// ***  in the index (currently one after another)
// ****************************************************************************

// default implementation used for most multi-entry slots
template <class Entry, class Helper>
bool FindMatchingEntryInMultiSlot(const typename Entry::ReadKey &key,
                                  Entry *entryReturn,
                                  const unsigned int fileFormatVersion,
                                  EndianReadBuffer &buf,
                                  std::uint16_t entry_count,
                                  Helper *)
{
  // loop through entries in buffer looking for a match
  for (std::uint16_t j = 0; j < entry_count; ++j) {
    entryReturn->VersionedPull(buf, fileFormatVersion);
    if (entryReturn->ReadMatches(key)) {
      return true;
    }
  }
  return false;
}

// Specialized implementation used for most multi-entry Blend slots
template <class Helper>
bool FindMatchingEntryInMultiSlot(const typename BlendEntry::ReadKey &key,
                                  BlendEntry *entryReturn,
                                  const unsigned int fileFormatVersion,
                                  EndianReadBuffer &buf,
                                  std::uint16_t entry_count,
                                  Helper *helper)
{
  BlendEntry tmp;
  std::uint32_t pos = 0; // pos are stored+1 in the ExtraData, so 0 is guaranteed to
                  // be smaller than even the inset on the bottom of the stack

  // loop through all entries in buffer looking for the one with the largest
  // stack position (stored in the index ExtraData by file number)
  for (std::uint16_t j = 0; j < entry_count; ++j) {
    tmp.VersionedPull(buf, fileFormatVersion);
    std::uint32_t newpos = helper->GetPacketExtra(tmp.dataAddress.fileNum);
    if (newpos > pos) {
      pos = newpos;
      *entryReturn = tmp;
    }
  }
  return pos > 0;
}





}  // namespace geindex



extern std::string ToString(geindex::TypedEntry::TypeEnum e);
extern void FromString(const std::string &str,
                       geindex::TypedEntry::TypeEnum &e);



#endif // COMMON_GEINDEX_ENTRIES_H__

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

//

#include "Entries.h"
#include "shared.h"
#include <khConstants.h>
#include <khEndian.h>
#include "EntriesTmpl.h"

namespace geindex {

bool ShouldStoreSinglesSparsely(uint32 entrySize, uint16 numSlots) {
  // Buckets with single entries per slot can be stored in one of two ways
  // -- sparsely populated --
  // uint16 numSlots;
  // BucketChildSlotType [numSlots]
  // { Entry for first  slot}
  // { Entry for second slot}
  // { Entry for third  slot}
  // -- fully populated --
  // uint16 numSlots;
  // Entry[kEntrySlotsPerBucket]

  // we want to choose the one that costs us the least to store
  uint32 totalSparseCost = sizeof(uint16) +
                           numSlots * sizeof(BucketChildSlotType) +
                           numSlots * entrySize;
  uint32 totalFixedCost = sizeof(uint16) + kEntrySlotsPerBucket * entrySize;
  return totalSparseCost < totalFixedCost;
}

// ****************************************************************************
// ***  ExternalDataAddress
// ****************************************************************************
void ExternalDataAddress::Push(EndianWriteBuffer &buf) const {
  buf << fileOffset
      << fileNum
      << size;
}
void ExternalDataAddress::Pull(EndianReadBuffer &buf) {
  buf >> fileOffset
      >> fileNum
      >> size;
}


// ****************************************************************************
// ***  SimpleInsetEntry
// ****************************************************************************
void SimpleInsetEntry::Push(EndianWriteBuffer &buf) const {
  buf << dataAddress
      << version
      << insetId;
}
void SimpleInsetEntry::VersionedPull(EndianReadBuffer &buf,
                                        uint16 /* fileFormatVersion */) {
  buf >> dataAddress
      >> version
      >> insetId;
}

// ****************************************************************************
// ***  BlendEntry
// ****************************************************************************
void BlendEntry::Push(EndianWriteBuffer &buf) const {
  buf << dataAddress
      << version
      << insetId;
}
void BlendEntry::VersionedPull(EndianReadBuffer &buf,
                               uint16 /* fileFormatVersion */) {
  buf >> dataAddress
      >> version
      >> insetId;
}

// ****************************************************************************
// ***  ChannelledEntry
// ****************************************************************************
void ChannelledEntry::Push(EndianWriteBuffer &buf) const {
  buf << dataAddress
      << version
      << channel;
}
void ChannelledEntry::VersionedPull(EndianReadBuffer &buf,
                                    uint16 /* fileFormatVersion */) {
  buf >> dataAddress
      >> version
      >> channel;
}

// ****************************************************************************
// ***  TypedEntry
// ****************************************************************************
void TypedEntry::Push(EndianWriteBuffer &buf) const {
  buf << dataAddress
      << version
      << channel
      << EncodeAs<uint8>(type);
}
void TypedEntry::VersionedPull(EndianReadBuffer &buf,
                               uint16 /* fileFormatVersion */) {
  buf >> dataAddress
      >> version
      >> channel
      >> DecodeAs<uint8>(type);
}


// ****************************************************************************
// ***  Loaded*EntryBucket explicit instantiations
// ****************************************************************************
template class LoadedSingleEntryBucket<SimpleInsetEntry>;
template class LoadedMultipleEntryBucket<BlendEntry>;
template class LoadedMultipleEntryBucket<ChannelledEntry>;
template class LoadedMultipleEntryBucket<TypedEntry>;



} // namespace geindex


std::string ToString(geindex::TypedEntry::TypeEnum e) {
    // WARNING!!!
    // Do not change these strings, they are saved is various XML files
    switch (e) {
      case geindex::TypedEntry::QTPacket:
        return kQtPacketType;
      case geindex::TypedEntry::Imagery:
        return kImageryType;
      case geindex::TypedEntry::Terrain:
        return kTerrainType;
      case geindex::TypedEntry::VectorGE:
        return kVectorGeType;
      case geindex::TypedEntry::VectorMaps:
        return kVectorMapsType;
      case geindex::TypedEntry::VectorMapsRaster:
        return kVectorMapsRasterType;
    case geindex::TypedEntry::QTPacket2:
      return kQtPacketTwoType;
    case geindex::TypedEntry::Unified:
      return kUnifiedType;
    case geindex::TypedEntry::DatedImagery:
      return kDatedImageryType;
    }
    return std::string(); // silence compiler
}

void FromString(const std::string &str,
                geindex::TypedEntry::TypeEnum &e) {
    // WARNING!!!
    // Do not change these strings, they are saved is various XML files
    if (str == kQtPacketType) {
      e = geindex::TypedEntry::QTPacket;
    } else if (str == kImageryType) {
      e= geindex::TypedEntry::Imagery;
    } else if (str == kTerrainType) {
      e = geindex::TypedEntry::Terrain;
    } else if (str == kVectorGeType) {
      e = geindex::TypedEntry::VectorGE;
    } else if (str == kVectorMapsType) {
      e = geindex::TypedEntry::VectorMaps;
    } else if (str == kVectorMapsRasterType) {
      e = geindex::TypedEntry::VectorMapsRaster;
    } else if (str == kQtPacketTwoType) {
      e = geindex::TypedEntry::QTPacket2;
    } else if (str == kUnifiedType) {
      e = geindex::TypedEntry::Unified;
    } else if (str == kDatedImageryType) {
      e = geindex::TypedEntry::DatedImagery;
    } else {
      throw khSimpleException("Internal Error: Unrecognized "
                              "geindex::Entry::TypeEnum String: ")
                                << str;
    }
}

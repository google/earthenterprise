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

#include "Entries.h"
#include "shared.h"
#include <khConstants.h>
#include <khEndian.h>
#include "EntriesTmpl.h"

namespace geindex {

bool ShouldStoreSinglesSparsely(std::uint32_t entrySize, std::uint16_t numSlots) {
  // Buckets with single entries per slot can be stored in one of two ways
  // -- sparsely populated --
  // std::uint16_t numSlots;
  // BucketChildSlotType [numSlots]
  // { Entry for first  slot}
  // { Entry for second slot}
  // { Entry for third  slot}
  // -- fully populated --
  // std::uint16_t numSlots;
  // Entry[kEntrySlotsPerBucket]

  // we want to choose the one that costs us the least to store
  std::uint32_t totalSparseCost = sizeof(std::uint16_t) +
                           numSlots * sizeof(BucketChildSlotType) +
                           numSlots * entrySize;
  std::uint32_t totalFixedCost = sizeof(std::uint16_t) + kEntrySlotsPerBucket * entrySize;
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
                                        std::uint16_t /* fileFormatVersion */) {
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
                               std::uint16_t /* fileFormatVersion */) {
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
                                    std::uint16_t /* fileFormatVersion */) {
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
      << EncodeAs<std::uint8_t>(type);
}
void TypedEntry::VersionedPull(EndianReadBuffer &buf,
                               std::uint16_t /* fileFormatVersion */) {
  buf >> dataAddress
      >> version
      >> channel
      >> DecodeAs<std::uint8_t>(type);
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

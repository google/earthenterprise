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


#include <string>
#include <iomanip>
#include <cmath>
#include <packet.h>
#include "comparevector.h"

namespace {

const double kEpsilonXY = 1.0e-10;
const double kEpsilonZ = 1.0e-5;

const std::string kLinePrefix = "\n      ";

struct OffsetSize {
  std::int32_t offset;
  std::uint32_t size;
};

inline bool IsAlmostEqual(double a, double b, double epsilon) {
  return (fabs(a-b) < epsilon) ? true : false;
}

bool CompareDataHeaders(const etDataHeader& header_a,
                        const etDataHeader& header_b,
                        std::ostream *snippet) {
  if ( header_a.magicID != header_b.magicID ) {
    *snippet << kLinePrefix << "etDataHeader mismatcheded magicID: "
             << header_a.magicID << ", " << header_b.magicID;
    return false;
  }
  if ( header_a.dataTypeID != header_b.dataTypeID ) {
    *snippet << kLinePrefix << "etDataHeader mismatched dataTypeID: "
             << header_a.dataTypeID << ", " << header_b.dataTypeID;
    return false;
  }
  if ( header_a.version != header_b.version ) {
    *snippet << kLinePrefix << "etDataHeader mismatched version: "
             << header_a.version << ", " << header_b.version;
    return false;
  }
  if ( header_a.numInstances != header_b.numInstances ) {
    *snippet << kLinePrefix << "etDataHeader mismatched numInstances: "
             << header_a.numInstances << ", " << header_b.numInstances;
    return false;
  }
  if ( header_a.dataInstanceSize != header_b.dataInstanceSize ) {
    *snippet << kLinePrefix << "etDataHeader mismatched dataInstanceSize: "
             << header_a.dataInstanceSize << ", " << header_b.dataInstanceSize;
    return false;
  }
  if ( header_a.dataBufferOffset != header_b.dataBufferOffset ) {
    *snippet << kLinePrefix << "etDataHeader mismatched dataBufferOffset: "
             << header_a.dataBufferOffset << ", " << header_b.dataBufferOffset;
    return false;
  }
  if ( header_a.dataBufferSize != header_b.dataBufferSize ) {
    *snippet << kLinePrefix
             << "etDataHeader mismatched dataBufferSize (may be padding): "
             << header_a.dataBufferSize << ", " << header_b.dataBufferSize;
  }
  if ( header_a.metaBufferSize != header_b.metaBufferSize ) {
    *snippet << kLinePrefix << "etDataHeader mismatched metaBufferSize: "
             << header_a.metaBufferSize << ", " << header_b.metaBufferSize;
    return false;
  }
  return true;
}

bool ComparePolyLinePackets(etPolyLinePacketData* pak_a,
                            etPolyLinePacketData* pak_b,
                            std::ostream *snippet) {
  if ( pak_a->name.string && pak_b->name.string ) {
    if( strcmp(pak_a->name.string, pak_b->name.string) ) {
      *snippet << kLinePrefix << "etPolyLinePacketData name mismatch: \""
               << pak_a->name.string << "\", \""
               << pak_b->name.string << "\"";
      return false;
    }
  }
  if( pak_a->texId == pak_b->texId &&
      pak_a->numPt == pak_b->numPt &&
      pak_a->bitFlags == pak_b->bitFlags &&
      pak_a->style == pak_b->style ) {

    // compare individual verts
    for ( int v = 0; v < pak_a->numPt; ++v ) {
      if (!IsAlmostEqual(pak_a->localPt[v].elem[0],
                         pak_b->localPt[v].elem[0],
                         kEpsilonXY) ||
          !IsAlmostEqual(pak_a->localPt[v].elem[1],
                         pak_b->localPt[v].elem[1],
                         kEpsilonXY) ||
          !IsAlmostEqual(pak_a->localPt[v].elem[2],
                         pak_b->localPt[v].elem[2],
                         kEpsilonZ) ) {
        *snippet << kLinePrefix << "etPolyLinePacketData vertex mismatch, ("
                 << std::setprecision(12)
                 << pak_a->localPt[v].elem[0] << ","
                 << pak_a->localPt[v].elem[1] << ","
                 << pak_a->localPt[v].elem[2] << "), "
                 << pak_b->localPt[v].elem[0] << ","
                 << pak_b->localPt[v].elem[1] << ","
                 << pak_b->localPt[v].elem[2] << ")";
        return false;
      }
    }
    return true;
  }
  *snippet << kLinePrefix << "etPolyLinePacketData field mismatch, "
           << "texId=(" << pak_a->texId << "," << pak_b->texId
           << ") numPt =(" << pak_a->numPt << "," << pak_b->texId
           << ") bitFlags=(0x" << std::hex << pak_a->bitFlags
           << ",0x" << pak_b->bitFlags << std::dec
           << ") style=(" << pak_a->style << "," << pak_b->style << ")";
  return false;
}

bool CompareStreetPackets(etStreetPacketData* pak_a,
                          etStreetPacketData* pak_b,
                          std::ostream *snippet) {
  bool same_name = (pak_a->name.string == pak_b->name.string)  // both same ptr
      || (pak_a->name.string && pak_b->name.string &&          // contents same
          0 == strcmp(pak_a->name.string, pak_b->name.string));
  if (!same_name) {
    *snippet << kLinePrefix << "etStreetPacketData name mismatch: \""
             << pak_a->name.string << "\", \""
             << pak_b->name.string << "\"";
    return false;
  }
  if( pak_a->numPt == pak_b->numPt &&
      pak_a->bitFlags == pak_b->bitFlags &&
      pak_a->style == pak_b->style ) {

    // compare individual verts
    for ( int v = 0; v < pak_a->numPt; ++v ) {
      if (!IsAlmostEqual(pak_a->localPt[v].elem[0],
                         pak_b->localPt[v].elem[0],
                         kEpsilonXY) ||
          !IsAlmostEqual(pak_a->localPt[v].elem[1],
                         pak_b->localPt[v].elem[1],
                         kEpsilonXY) ||
          !IsAlmostEqual(pak_a->localPt[v].elem[2],
                         pak_b->localPt[v].elem[2],
                         kEpsilonZ) ) {
        *snippet << kLinePrefix << "etStreetPacketData vertex mismatch, ("
                 << std::setprecision(12)
                 << pak_a->localPt[v].elem[0] << ","
                 << pak_a->localPt[v].elem[1] << ","
                 << pak_a->localPt[v].elem[2] << "), "
                 << pak_b->localPt[v].elem[0] << ","
                 << pak_b->localPt[v].elem[1] << ","
                 << pak_b->localPt[v].elem[2] << ")";
        return false;
      }
    }
    return true;
  }
  *snippet << kLinePrefix << "etStreetPacketData field mismatch, "
           << "numPt =(" << pak_a->numPt << "," << pak_b->numPt
           << ") bitFlags=(0x" << std::hex << pak_a->bitFlags
           << ",0x" << pak_b->bitFlags << std::dec
           << ") style=(" << pak_a->style << "," << pak_b->style << ")";
  return false;
}

bool CompareLandmarkPackets(etLandmarkPacketData* pak_a,
                            etLandmarkPacketData* pak_b,
                            std::ostream *snippet) {
  if ( pak_a->name.string && pak_b->name.string ) {
    if( strcmp(pak_a->name.string, pak_b->name.string) ) {
      *snippet << kLinePrefix << "etLandmarkPacketData name mismatch: \""
               << pak_a->name.string << "\", \""
               << pak_b->name.string << "\"";
      return false;
    }
  }
  if ( pak_a->description && pak_b->description ) {
    if( strcmp((char*)pak_a->description, (char*)pak_b->description) ) {
      *snippet << kLinePrefix << "etLandmarkPacketData description mismatch: \""
               << (char*)pak_a->description << "\", \""
               << (char*)pak_b->description << "\"";
      return false;
    }
  }

  if( pak_a->numPt != pak_b->numPt ||
      pak_a->bitFlags != pak_b->bitFlags ||
      pak_a->style != pak_b->style ||
      pak_a->referenceType != pak_b->referenceType ) {
    *snippet << kLinePrefix << "etLandmarkPacketData field mismatch, "
             << "numPt =(" << pak_a->numPt << "," << pak_b->numPt
             << ") bitFlags=(0x" << std::hex << pak_a->bitFlags
             << ",0x" << pak_b->bitFlags << std::dec
             << ") style=(" << pak_a->style << "," << pak_b->style
             << "), referenceType=(" << pak_a->referenceType
             << "," << pak_b->referenceType << ")";
    return false;
  }

  if (!IsAlmostEqual(pak_a->localPt[0].elem[0],
                     pak_b->localPt[0].elem[0],
                     kEpsilonXY) ||
      !IsAlmostEqual(pak_a->localPt[0].elem[1],
                     pak_b->localPt[0].elem[1],
                     kEpsilonXY) ||
      !IsAlmostEqual(pak_a->localPt[0].elem[2],
                     pak_b->localPt[0].elem[2],
                     kEpsilonZ)) {
    *snippet << kLinePrefix << "etLandmarkPacketData vertex mismatch, ("
             << std::setprecision(12)
             << pak_a->localPt[0].elem[0] << ","
             << pak_a->localPt[0].elem[1] << ","
             << pak_a->localPt[0].elem[2] << "), "
             << pak_b->localPt[0].elem[0] << ","
             << pak_b->localPt[0].elem[1] << ","
             << pak_b->localPt[0].elem[2] << ")";
    return false;
  }
  return true;
}

bool ComparePolygonPackets(etPolygonPacketData* pak_a,
                           etPolygonPacketData* pak_b,
                           std::ostream *snippet) {
  if ( pak_a->name.string && pak_b->name.string ) {
    if( strcmp(pak_a->name.string, pak_b->name.string) ) {
      *snippet << kLinePrefix << "etPolygonPacketData name mismatch: \""
               << pak_a->name.string << "\", \""
               << pak_b->name.string << "\"";
      return false;
    }
  }
  if( pak_a->properties == pak_b->properties &&
      pak_a->numPt == pak_b->numPt &&
      pak_a->bitFlags == pak_b->bitFlags &&
      pak_a->numEdgeFlags == pak_b->numEdgeFlags &&
      pak_a->style == pak_b->style ) {

    // compare individual verts
    for ( int v = 0; v < pak_a->numPt; ++v ) {
      if (!IsAlmostEqual(pak_a->localPt[v].elem[0],
                         pak_b->localPt[v].elem[0],
                         kEpsilonXY) ||
          !IsAlmostEqual(pak_a->localPt[v].elem[1],
                         pak_b->localPt[v].elem[1],
                         kEpsilonXY) ||
          !IsAlmostEqual(pak_a->localPt[v].elem[2],
                         pak_b->localPt[v].elem[2],
                         kEpsilonZ) ) {
        *snippet << kLinePrefix << "etPolygonPacketData vertex mismatch, ("
                 << std::setprecision(12)
                 << pak_a->localPt[v].elem[0] << ","
                 << pak_a->localPt[v].elem[1] << ","
                 << pak_a->localPt[v].elem[2] << "), "
                 << pak_b->localPt[v].elem[0] << ","
                 << pak_b->localPt[v].elem[1] << ","
                 << pak_b->localPt[v].elem[2] << ")";
        return false;
      }
    }
    // compare individual edge flags
    for ( int f = 0; f < pak_a->numEdgeFlags; ++f ) {
      if (pak_a->edgeFlags[f] != pak_b->edgeFlags[f]) {
        *snippet << kLinePrefix << "etPolygonPacketData edgeFlags mismatch: "
                 << std::hex << "0x" << pak_a->edgeFlags[f]
                 << ", 0x" << pak_b->edgeFlags[f] << std::dec;
        return false;
      }
    }
    return true;
  }
  *snippet << kLinePrefix << "etPolygonPacketData field mismatch, "
           << "properties =(" << pak_a->properties << "," << pak_b->properties
           << ") numPt =(" << pak_a->numPt << "," << pak_b->numPt
           << ") bitFlags=(0x" << std::hex << pak_a->bitFlags
           << ",0x" << pak_b->bitFlags << std::dec
           << ") numEdgeFlags=(" << pak_a->numEdgeFlags
           << "," << pak_b->numEdgeFlags
           << "), style=(" << pak_a->style << "," << pak_b->style << ")";
  return false;
}


} // namespace

namespace gecrawler {

bool CompareVectorPackets(std::string *buffer_a,
                          std::string *buffer_b,
                          std::ostream *snippet) {
  const size_t size_a = buffer_a->size();
  const size_t size_b = buffer_b->size();

  if (size_a != size_b) {
    *snippet << kLinePrefix << "Packet size mismatch: " << size_a << ", " << size_b;
  }

  etDrawablePacket drawable_pak_a, drawable_pak_b;
  bool success_a = 0 == drawable_pak_a.load(&((*buffer_a)[0]), size_a);
  if (!success_a) {
    *snippet << kLinePrefix << "Bad drawable packet in source 1";
  }
  bool success_b = 0 == drawable_pak_b.load(&((*buffer_b)[0]), size_b);
  if (!success_b) {
    *snippet << kLinePrefix << "Bad drawable packet in source 2";
  }
  if (!success_a || !success_b)
    return false;

  if (drawable_pak_a.packetHeader.dataTypeID != TYPE_DRAWABLEPACKET) {
    *snippet << kLinePrefix << "Bad packet type in source 1, should be "
             << TYPE_DRAWABLEPACKET << ", is "
             << drawable_pak_a.packetHeader.dataTypeID;
    success_a = false;
  }
  if (drawable_pak_b.packetHeader.dataTypeID != TYPE_DRAWABLEPACKET) {
    *snippet << kLinePrefix << "Bad packet type in source 2, should be "
             << TYPE_DRAWABLEPACKET << ", is "
             << drawable_pak_b.packetHeader.dataTypeID;
    success_b = false;
  }
  if (!success_a || !success_b)
    return false;

  drawable_pak_a.offsetToPointer();
  drawable_pak_b.offsetToPointer();

  if ( !CompareDataHeaders(drawable_pak_a.packetHeader,
                           drawable_pak_b.packetHeader,
                           snippet) ) {
    return false;
  }

  bool success = true;
  id_size numInstances = std::min(drawable_pak_a.packetHeader.numInstances,
                                  drawable_pak_b.packetHeader.numInstances);

  for ( unsigned int jj = 0; jj < numInstances; ++jj ) {
    etDataPacket* pak_a = drawable_pak_a.getPtr( jj );
    etDataPacket* pak_b = drawable_pak_b.getPtr( jj );
    if ( !CompareDataHeaders(pak_a->packetHeader,
                             pak_b->packetHeader,
                             snippet) ) {
      success = false;
    } else {
      switch ( pak_a->packetHeader.dataTypeID ) {
        case TYPE_POLYLINEPACKET:
          ((etPolyLinePacket*)pak_a)->offsetToPointer();
          ((etPolyLinePacket*)pak_b)->offsetToPointer();
          for ( unsigned int ii = 0; ii < pak_a->packetHeader.numInstances; ++ii )
            if ( !ComparePolyLinePackets(((etPolyLinePacket*)pak_a)->getPtr(ii),
                                         ((etPolyLinePacket*)pak_b)->getPtr(ii),
                                         snippet)) {
              success = false;
            }
          break;
        case TYPE_STREETPACKET_UTF8:
          ((etStreetPacket*)pak_a)->offsetToPointer();
          ((etStreetPacket*)pak_b)->offsetToPointer();
          for ( unsigned int ii = 0; ii < pak_a->packetHeader.numInstances; ++ii )
            if ( !CompareStreetPackets(((etStreetPacket*)pak_a)->getPtr(ii),
                                       ((etStreetPacket*)pak_b)->getPtr(ii),
                                       snippet)) {
              success = false;
            }
          break;
        case TYPE_LANDMARK:
          ((etLandmarkPacket*)pak_a)->offsetToPointer();
          ((etLandmarkPacket*)pak_b)->offsetToPointer();
          for ( unsigned int ii = 0; ii < pak_a->packetHeader.numInstances; ++ii )
            if (!CompareLandmarkPackets(((etLandmarkPacket*)pak_a)->getPtr(ii),
                                        ((etLandmarkPacket*)pak_b)->getPtr(ii),
                                        snippet)) {
              success = false;
            }
          break;
        case TYPE_POLYGONPACKET:
          ((etPolygonPacket*)pak_a)->offsetToPointer();
          ((etPolygonPacket*)pak_b)->offsetToPointer();
          for ( unsigned int ii = 0; ii < pak_a->packetHeader.numInstances; ++ii )
            if (!ComparePolygonPackets(((etPolygonPacket*)pak_a)->getPtr(ii),
                                       ((etPolygonPacket*)pak_b)->getPtr(ii),
                                       snippet)) {
              success = false;
            }
          break;
        default:
          if (*buffer_a != *buffer_b) {
            *snippet << kLinePrefix << "Unknown data type "
                     << pak_a->packetHeader.dataTypeID
                     << " failed binary compare";
            success = false;
          }
      }
    }
  }
  return success;
}

} // namespace gecrawler

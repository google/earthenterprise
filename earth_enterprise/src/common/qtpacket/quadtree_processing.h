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
//
//
// Routines to construct a quadtree packet from index information.
//

#ifndef COMMON_QTPACKET_QUADTREE_PROCESSING_H__
#define COMMON_QTPACKET_QUADTREE_PROCESSING_H__

#include <cstdint>
#include <string>
#include <vector>
#include <common/base/macros.h>
#include <khGuard.h>
#include <khEndian.h>
#include <qtpacket/quadsetitem.h>
#include <qtpacket/quadtreepacketitem.h>

class KhQuadTreePacketProtoBuf;

namespace qtpacket {

// As of Google Earth Client 5.0, two types of quadtree packet formats are
// supported.
// Format1 is the original format, still used in 5.0.
// Format2 is the protocol buffer format, used in 5.0 for timemachine quadtree
// packets (imagery only), it will be used in future earth clients.
// Each of these takes in a group of QuadtreePacketItems which lie in
// the current quadset (a group of quadtree nodes that goes 4 levels deep).
//
// Fill in output packet with the in-memory quadtree packet structure
// built from the given input data:
//
// quadtree_packet_version gives the version number (also known as epoch number)
// to write into each newly constructed packet.  This version is simply a
// monotonic count of changes to the packets so that the earth client can
// identify updated packets and load them instead of relying on cache.
//
// The output of each is a quadtree packet of the respective format, which will
// be serialized to the quadtree database.
void ComputeQuadtreePacketFormat1(
    const QuadsetGroup<QuadtreePacketItem> &group,
    int quadtree_packet_version,
    KhQuadTreePacket16 &packet);

// Format2 computes the protocol buffer based quadtree packets.
// See ComputeQuadtreePacketFormat1 for a description of what this routine does.
void ComputeQuadtreePacketFormat2(
    const QuadsetGroup<QuadtreePacketItem> &group,
    int quadtree_packet_version,
    KhQuadTreePacketProtoBuf* packet);

// Helper routine for ComputeQuadtreePacket (defined here to allow access
// by tests)

void InsertChannel(std::uint16_t channel,
                   std::uint16_t version,
                   KhQuadTreeQuantum16 *node);

}  // namespace qtpacket

#endif  // COMMON_QTPACKET_QUADTREE_PROCESSING_H__

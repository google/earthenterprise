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

#ifndef GEO_SRC_FUSION_GST_VECTORPREP_POLYLINEJOINER_H_
#define GEO_SRC_FUSION_GST_VECTORPREP_POLYLINEJOINER_H_

#include <gstGeode.h>
#include <deque>

namespace vectorprep {

class SegmentEndExt;

// Joins polylines at degree two end points only. (We don't join at degree more
// than 2 as in those cases labeling each polyline separately is required, which
// is not the case for polylines at degree two). Joining two polylines sharing
// an endpoint is moving points from one polyline (say x) to the other (say y)
// and emptying the polyline "x". This class also removes duplicate polylines
// in the process. At the end we have some of segments in input polyline_vector_
// having more vertices from other segments and those other segments are empty.
// Also few segments in polyline_vector_ are empty because of duplicate removal.
//
// For ease of visualization we consider "polyline" as "segment" with first and
// last point as the two end points. So we may use both names to mean same.
//
// Algorithm:
// Collect end points avoiding empty polylines.
// Sort all end points.
// Find set of ends on same vertex.
// For each such set
//   Try to check if at that vertex degree is two. Exclude cycles and duplicate
//   polylines while counting degree. For each degree 2 vertex create a
//   partnership link between the two segments.
//
// For each non-cyclic end point (characterized by having no link at the end
// point but a link at other end point), find the longest connected chain of
// segments following degree 2 vertices and join those.
//
// For each cyclic end point (characterized by having a link, after taking out
// all non-cyclic chains in previous step), find the longest connected chain
// of segments (which will be a cycle) following degree 2 vertices and join
// those.
template<class T>
class PolylineJoiner {

 public:
  // This is the only public method available to callers. Returns number of
  // duplicate segments removed as well as number of segments joined.
  static void RemoveDuplicatesAndJoinNeighborsAtDegreeTwoVertices(
      const T& glist, std::uint64_t* num_duplicates, std::uint64_t* num_joined);

 private:
  // Returns how many more join opportunities are available. Used for validation
  // of JoinSegments result (in non-release modes only).
  static unsigned int JoinableVertexCount(const T& glist);


  PolylineJoiner(const T& glist);

  void Join();

  std::uint32_t InitializeSegmentEnds();

  // Remove duplicates and find merge partners at degree two vertices only
  void RemoveDuplicatesAndCyclesAndFindMergePartners();
  void FindSegmentEndSetsOnSameVertex();
  void SetOtherEndIndex();
  void RemoveCyclesSetEdgesAndInitializePartners(std::uint32_t i);
  void RemoveDuplicatesAndSetPartnersIfAnyAtThisVertex(
      std::uint32_t i, const std::uint32_t end_index_for_set_i);

  // Checks that the polyline_ and partners_index_ fields in SegmentEndExt are
  // set correctly.partners_index_partners_index_
  int AssertSanity();

  void MergeThisChainAtDegreeTwoVertices(SegmentEndExt* curr_end);
  void MergeAtDegreeTwoVerticesForNonCycles();
  void MergeAtDegreeTwoVerticesForCycles();

  const T& polyline_vector_;         // input vector of segments.
  // Some entries in polyline_vector_ may be empty edges to start with.
  const std::uint32_t num_edge_ends_;               // number of segments *2
  std::deque<SegmentEndExt> segment_ends_;  // the SegmentEndExt array for
  // sorting. SegmentEnd is simply an identifier representing a edge end (e.g
  // first and last points of i' th edge in polyline_vector_ are SegmentEnds
  // 2*i & 2*i +1. SegmentEndExt has additional data "vertex coordinates" for
  // avoiding the need of reaccessing polyline_vector_ and thus making sorting
  // faster. Later on after sorting is done this field is reused to keep
  // various other data needed in join process.
  const std::uint32_t segment_ends_size_;           // number of non-empty segments * 2
  // This buffer is used to pass "the list of edges to sort" to Join method.
  // To avoid memory fragmentation and for runtime purposes it is reserved to
  // the size of polyline_vector_.
  std::deque<std::pair<gstGeodeImpl*, bool> > segments_to_join_buffer_;
  std::uint64_t num_duplicates_;
  std::uint64_t num_joined_;
};

}  // end vectorprep

#endif  // GEO_SRC_FUSION_GST_VECTORPREP_POLYLINEJOINER_H_

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


#include "fusion/gst/vectorprep/PolylineJoiner.h"

#include <limits.h>
#include <map>
#include <deque>

#define NO_PARTNER UINT_MAX

namespace vectorprep {

// SegmentEnds of segment e (identified with segment_num) are 2 * segment_num
// and 2 * segment_num +1 respectively representing the first and last ends.
// As each segment is uniquely identified with segment_num each SegmentEnd is a
// unique representation by itself. Note that two different segments share same
// end point, but their SegmentEnds will be different. When segment_num's
// follow a continuous range [0, max_seg-1] the SegmentEnds also follow a
// continuous range of [0, 2*max_seg -1] and can be used for simple array data
// structures.
class SegmentEnd {
 public:
  // Empty constructor to be used for vector.
  SegmentEnd() : id_(UINT_MAX) {}

  // Create a SegmentEnd from SegmentEndId
  explicit SegmentEnd(std::uint32_t id) : id_(id) {}

  // Create a SegmentEnd from SegmentId and is_last (first/last) information.
  SegmentEnd(std::uint32_t seg_id, bool is_last) :
      id_((seg_id << 1) + (is_last ? 1 : 0)) {}

  const SegmentEnd& operator=(const SegmentEnd& other) {
    new(this) SegmentEnd(other.id_);
    return *this;
  }

  // Get other SegmentEnd, i.e if it is first end return last end and vice-versa
  SegmentEnd OtherEnd() const {
    return SegmentEnd(id_ ^ 1U);
  }

  // Get SegmentEndId
  std::uint32_t SegmentEndId() const { return id_; }

  // Get SegmentId
  const std::uint32_t SegmentId() const {
    return (id_ >> 1U);
  }

  // Return whether this SegmentEnd represents last point of Segment.
  bool IsLast() const {
    return ((id_ & 1U) != 0);
  }

  // Return whether this SegmentEnd represents first point of Segment.
  bool IsFirst() const {
    return ((id_ & 1U) == 0);
  }

  // Compare two segment ends.
  bool operator<(const SegmentEnd& other) const {
    return id_ < other.id_;
  }

 private:
  // Though defined correctly made private to avoid accidental mis-use.
  bool operator>(const SegmentEnd& other) const {
    return id_ > other.id_;
  }

  bool operator==(const SegmentEnd& other) const {
    return id_ == other.id_;
  }

  bool operator!=(const SegmentEnd& other) const {
    return id_ != other.id_;
  }

  const std::uint32_t id_;
};


// This wrapper is just to help sorting. Find the < operator with
// vertex class member an order faster than a wrapper taking glist and
// defining () operator.
class SegmentEndExt : public SegmentEnd {
 public:
  SegmentEndExt() {}

  explicit SegmentEndExt(std::uint32_t id, const gstVertex& v)
      : SegmentEnd(id), x_(v.x), y_(v.y) {
  }

  SegmentEndExt(std::uint32_t seg_id, bool is_last, const gstVertex& v)
      : SegmentEnd(seg_id, is_last), x_(v.x), y_(v.y) {
  }

  bool operator < (const SegmentEndExt& other) const {
    return x_ < other.x_ || (x_ == other.x_ && y_ < other.y_);
  }

  bool operator != (const SegmentEndExt& other) const {
    return (x_ != other.x_ || y_ != other.y_);
  }

  // This non-intuitive union has the following purpose
  // Since we will have an array of SegmentEndExt and segment ends may be in
  // Millions the memory requirment is of concern. So we use the memory in
  // various stages as follows:
  // stage 1: (during sorting) for storing coordinates of the point.
  // stage 2: for setting ends_at_index_. (overwriting x_ and y_)
  // stage 3: for storing tmp_other_ends_index_.
  // stage 4: for storing other_ends_index_
  // (Note: In stage 3 and 4 we have not overwritten ends_at_index_)
  // For each set of vertices on same point
  //   stage 5: for polyline_ (overwriting tmp_other_ends_index_) and
  //            initializing partners_index_ to NO_PARTNER
  //   stage 6: for resetting polyline_ and partners_index_ as required
  // So at the end only polyline_ and partners_index_ are used for merging and
  // resetting partners_index_ values (as required so that we avoid revisiting
  // the same vertex).
  union {
    struct {
      double x_;
      double y_;
    };

    struct {
      union {
        // polyline_ is the polyline for which SegmentEndExt is an end point.
        // We set this to NULL to imply that this SegmentEndExt can be ignored
        // due to being an end point of a removed polyline (cyclic/duplicate).
        gstGeodeImpl* polyline_;
        std::uint32_t tmp_other_ends_index_;
      };
      union {
        // partners_index_ is the index to the only other SegmentEndExt having
        // same geometric location. i.e partners_index_ is only for degree 2
        // vertices. Otherwise it is set to  NO_PARTNER.
        std::uint32_t partners_index_;   // set this to NO_PARTNER for no partner
        std::uint32_t ends_at_index_;    // this is used to point to the end point of
                                  // set of segment-ends on current vertex.
      };
      std::uint32_t other_ends_index_;
    };
  };
};


template<class T> PolylineJoiner<T>::PolylineJoiner(const T& glist)
    : polyline_vector_(glist), num_edge_ends_(glist.size() * 2),
      segment_ends_(num_edge_ends_, SegmentEndExt()),
      segment_ends_size_(InitializeSegmentEnds()), num_duplicates_(0),
      num_joined_(0) {
}


template<class T>
void PolylineJoiner<T>::RemoveDuplicatesAndJoinNeighborsAtDegreeTwoVertices(
    const T& glist, std::uint64_t* num_duplicates, std::uint64_t* num_joined) {
  PolylineJoiner joiner(glist);
  joiner.Join();
  assert(JoinableVertexCount(glist) == 0);
  *num_duplicates = joiner.num_duplicates_;
  *num_joined = joiner.num_joined_;
}


template<class T>
void PolylineJoiner<T>::Join() {
  if (segment_ends_size_ == 0) {
    return;
  }
  // Sort the edge ends so that those will be ordered on vertex
  std::sort(segment_ends_.begin(), segment_ends_.end());
  RemoveDuplicatesAndCyclesAndFindMergePartners();
  assert(AssertSanity());
  MergeAtDegreeTwoVerticesForNonCycles();
  MergeAtDegreeTwoVerticesForCycles();
}


template<class T>
 std::uint32_t PolylineJoiner<T>::InitializeSegmentEnds() {
  std::uint32_t num_ends = 0;
  const std::uint32_t g_end = polyline_vector_.size();
  for (std::uint32_t g = 0; g < g_end; ++g) {
    // NOTE: Here we check geode for correctness for PolylineJoiner.
    // Below we just cast it to gstGeode by static_cast without
    // checking the type.
    const gstGeodeHandle &segh = polyline_vector_[g];
    if (segh->FlatPrimType() != gstPolyLine &&
        segh->FlatPrimType() != gstStreet &&
        segh->NumParts() > 1) {
      notify(NFY_FATAL, "%s: Incorrect geode.", __func__);
    }

    const gstGeode *seg = static_cast<const gstGeode*>(&(*segh));
    if (seg->VertexCount(0) != 0) {
      new(&segment_ends_[num_ends])  // in place new using vector as membuf
          SegmentEndExt(g, false, seg->GetFirstVertex(0));
      ++num_ends;
      new(&segment_ends_[num_ends])  // in place new using vector as membuf
          SegmentEndExt(g, true, seg->GetLastVertex(0));
      ++num_ends;
    }
  }
  return num_ends;
}


// Input vertices are sorted on v(x_, y_)
template<class T>
void PolylineJoiner<T>::RemoveDuplicatesAndCyclesAndFindMergePartners() {
  // First find the set of vertices on same vertex
  FindSegmentEndSetsOnSameVertex();

  // For each SegmentEnd need to get position of other SegmentEnd in sorted
  // Set the other_ends_index_ field for all segment-ends.
  SetOtherEndIndex();

  // Find duplicates, cycles and partners for all degree 2 vertices
  // work on sets of segment ends on same vertex, as we reuse ends_at_index_
  // for partners_index_ (for this set) while working on a set.
  for (std::uint32_t i = 0, end_index_for_set_i; i < segment_ends_size_;
       i = end_index_for_set_i) {
    end_index_for_set_i = segment_ends_[i].ends_at_index_;
    RemoveCyclesSetEdgesAndInitializePartners(i);

    { // skip over leading cycles and continue if degree 1 vertex
      for (; i < end_index_for_set_i && segment_ends_[i].polyline_ == NULL;
             ++i) {
      }
      if ((end_index_for_set_i - i) <= 1) {  // => degree > 1
        continue;
      }
    }
    RemoveDuplicatesAndSetPartnersIfAnyAtThisVertex(i, end_index_for_set_i);
  }
}


// Assumes segment_ends_ is already sorted on geometric comparison of vertices.
// At end of this function the set of SegmentEnds on same vertex will have same
// ends_at_index_ (which is pointer to next set). Also after this the vertex
// information can no more be used, as it is being reused by other fields in
// the union.
template<class T>
void PolylineJoiner<T>::FindSegmentEndSetsOnSameVertex() {
  for (std::uint32_t i = 0, end_index_for_set_i; i < segment_ends_size_;) {
    const SegmentEndExt& this_end = segment_ends_[i];
    for (end_index_for_set_i = i + 1; end_index_for_set_i < segment_ends_size_;
         ++end_index_for_set_i) {
      if (this_end != segment_ends_[end_index_for_set_i]) {
        break;
      }
    }
    for (; i < end_index_for_set_i; ++i) {
      segment_ends_[i].ends_at_index_ = end_index_for_set_i;
    }
  }
}


// Set the other_ends_index_ field for all segment-ends. For each SegmentEnd
// gets position of other SegmentEnd in segment_ends_ (pre-sorted).
template<class T>
void PolylineJoiner<T>::SetOtherEndIndex() {
  // set the tmp_other_ends_index_ field, since we have used reserve we
  // can use segment_ends_ vector as an array indexed on SegmentEndId.
  for (std::uint32_t i = 0; i < segment_ends_size_; ++i) {
    segment_ends_[
        segment_ends_[i].OtherEnd().SegmentEndId()].tmp_other_ends_index_ = i;
  }

  // set the other_ends_index_ field so that we can get other ends index from
  // the current SegmentEndExt, without one extra array indexing.
  for (std::uint32_t i = 0; i < segment_ends_size_; ++i) {
    segment_ends_[i].other_ends_index_ = segment_ends_[
        segment_ends_[i].SegmentEndId()].tmp_other_ends_index_;
  }
}


// note that all SegmentEnd in same set are on same geometric vertex
// remove cycles, set polyline_ and initialize partners_ field. As a result the
// ends_at_index_ for this set will no longer be valid.
template<class T>
void PolylineJoiner<T>::RemoveCyclesSetEdgesAndInitializePartners(std::uint32_t i) {
  const std::uint32_t end_index_for_set_i = segment_ends_[i].ends_at_index_;
  for (; i < end_index_for_set_i; ++i) {
    SegmentEndExt& this_end = segment_ends_[i];
    if (i < this_end.other_ends_index_) {  // check for left ends only
      if (this_end.other_ends_index_ < end_index_for_set_i) {
        // right end in same set
        this_end.polyline_ = NULL;  // both ends are on the same set i.e cycle
                              // polyline_ == NULL => we will not merge cycles
      } else {
        this_end.polyline_ = &(*polyline_vector_[this_end.SegmentId()]);
      }
    } else {  // right end follows left end for edge
      this_end.polyline_ = segment_ends_[this_end.other_ends_index_].polyline_;
    }
    // initialize partners_index_ to invalid value
    this_end.partners_index_ = NO_PARTNER;
  }
}


// Exclude duplicates and then count degree of the vertex.
// If it is exactly 2 then set those two segment ends as partners of each
// other. Each such partner set represents a join oppertunity.
template<class T>
void PolylineJoiner<T>::RemoveDuplicatesAndSetPartnersIfAnyAtThisVertex(
    std::uint32_t i, const std::uint32_t end_index_for_set_i) {
  const std::uint32_t partner1 = i;
  // For finding degree-two vertices(i.e single partner segment ends),
  // initialize partner to 0. On first finding a partner, set that as partner,
  // on second and subsequent finding of partners set that to NO_PARTNER.
  std::uint32_t partner2 = 0;  // Note that 0 is not a good value for partner2
  // If there is a cycle on this vertex, then no merging at this vertex
  // because we need two labels for the two segments joining at vertex more than
  // degree two, so that labels are unambiguous.
  for (std::uint32_t j = i + 1; j < end_index_for_set_i; ++j) {
    // As duplicates have not yet been removed, for left ends, polyline_ == NULL
    // means cycle. (For right ends duplicates have been removed at
    // corresponding left ends and the right end copies left end polyline_,
    // and so NULL can be either due to duplicate removal or cycle).
    SegmentEndExt& curr_end = segment_ends_[j];
    if (j < curr_end.other_ends_index_  // Left End
        && curr_end.polyline_ == NULL) {
      partner2 = NO_PARTNER;  // partner2 will henceforth be only NO_PARTNER
      break;
    }
  }
  for ( ; ; partner2 = (partner2 == 0) ? i : NO_PARTNER) {
    const SegmentEndExt& this_end = segment_ends_[i];
    if (end_index_for_set_i <= this_end.other_ends_index_) {
      // duplicates are removed at left end only
      for (std::uint32_t n = i + 1; n < end_index_for_set_i; ++n) {
        SegmentEndExt& next_end = segment_ends_[n];
        // if other end is not to right it is to left and next_end is
        // right end and should be ignored. Else if NULL already deleted.
        if (end_index_for_set_i > next_end.other_ends_index_ ||
            next_end.polyline_ == NULL ||
            segment_ends_[this_end.other_ends_index_].ends_at_index_ !=
            segment_ends_[next_end.other_ends_index_].ends_at_index_) {
          continue;  // ignore right edges and non-same rt point
        }
        const std::uint32_t this_num_vertices =
            this_end.polyline_->TotalVertexCount();
        bool is_equal = (this_num_vertices ==
                         next_end.polyline_->TotalVertexCount());
        // if just 2 vertices those are already compared and equal
        if (is_equal && this_num_vertices != 2) {  // exhaustively check only
          // for polylines having more than 2 vertices.
          const bool is_both_left_first_or_left_last =
              (this_end.IsFirst() != next_end.IsFirst());
          is_equal = (static_cast<gstGeode*>(this_end.polyline_))->IsEqual(
              next_end.polyline_, is_both_left_first_or_left_last);
        }
        if (is_equal) {
          next_end.polyline_->Clear();  // delete duplicate
          ++num_duplicates_;
          next_end.polyline_ = NULL;
          // setting other end to NULL is not required as right end follows left
          // end for edges.
        }
      }
    }
    // skip over SegmentEndExt s which are no longer vald because of
    // cycle/duplicate removal.
    for ( ; ++i < end_index_for_set_i && segment_ends_[i].polyline_ == NULL; ) {
    }
    if (i == end_index_for_set_i) {
      break;
    }
  }
  if (partner2 != 0 && partner2 != NO_PARTNER) {
    // Found a valid set of degree 2 partners
    segment_ends_[partner1].partners_index_ = partner2;
    segment_ends_[partner2].partners_index_ = partner1;
  }
}


template<class T>
int PolylineJoiner<T>::AssertSanity() {
  for (std::uint32_t i = 0; i < segment_ends_size_; ++i) {
    assert(segment_ends_[segment_ends_[i].other_ends_index_].SegmentId() ==
           segment_ends_[i].SegmentId());
    if (segment_ends_[i].polyline_ == NULL) {
      assert(segment_ends_[i].partners_index_ == NO_PARTNER);
      assert(NO_PARTNER ==
             segment_ends_[segment_ends_[i].other_ends_index_].partners_index_);
    } else {
      if (segment_ends_[i].partners_index_ != NO_PARTNER) {
        std::uint32_t j = segment_ends_[i].partners_index_;
        assert(segment_ends_[j].partners_index_ == i);

        const gstGeode *geode_i = static_cast<const gstGeode*>(
            &(*polyline_vector_[segment_ends_[i].SegmentId()]));

        const gstGeode *geode_j = static_cast<const gstGeode*>(
            &(*polyline_vector_[segment_ends_[j].SegmentId()]));

        gstVertex v1 = segment_ends_[i].IsFirst()
            ? geode_i->GetFirstVertex(0)
            : geode_i->GetLastVertex(0);
        gstVertex v2 = segment_ends_[j].IsFirst()
            ? geode_j->GetFirstVertex(0)
            : geode_j->GetLastVertex(0);
        assert(fusion_gst::EqualsXY(v1, v2, .0));  // epsilon equals 0.
      }
    }
  }
  return 1;
}


template<class T>
void PolylineJoiner<T>::MergeAtDegreeTwoVerticesForNonCycles() {
  for (std::uint32_t i = 0; i < segment_ends_size_; ++i) {
    SegmentEndExt& curr_end = segment_ends_[i];
    // A non-cyclic chain starts from a degree 1 vertex having a degree 2
    // vertex on the other end of the edge.
    if ((NO_PARTNER != curr_end.partners_index_) || (NO_PARTNER ==
        segment_ends_[curr_end.other_ends_index_].partners_index_)) {
      continue;
    }
    MergeThisChainAtDegreeTwoVertices(&curr_end);
  }
}


template<class T>
void PolylineJoiner<T>::MergeAtDegreeTwoVerticesForCycles() {
  for (std::uint32_t i = 0; i < segment_ends_size_; ++i) {
    SegmentEndExt& curr_end = segment_ends_[i];
    // Once non-cyclic chains have been removed a cyclic chain starts from a
    // degree 2 vertex on the other end of the edge.
    if (NO_PARTNER == curr_end.partners_index_) {
      continue;
    }
    segment_ends_[curr_end.partners_index_].partners_index_ = NO_PARTNER;
    curr_end.partners_index_ = NO_PARTNER;
    MergeThisChainAtDegreeTwoVertices(&curr_end);
  }
}


template<class T>
void PolylineJoiner<T>::MergeThisChainAtDegreeTwoVertices(
    SegmentEndExt* curr_end) {
  std::uint32_t* other_ends_partner_index =
      &segment_ends_[curr_end->other_ends_index_].partners_index_;
  gstGeode* const seg = static_cast<gstGeode* const>(curr_end->polyline_);
  bool join_at_segs_last = curr_end->IsFirst();
  while (true) {
    curr_end = &segment_ends_[*other_ends_partner_index];
    curr_end->partners_index_ = NO_PARTNER;
    *other_ends_partner_index = NO_PARTNER;
    segments_to_join_buffer_.push_back(
        std::make_pair(curr_end->polyline_, curr_end->IsFirst()));
    other_ends_partner_index =
        &segment_ends_[curr_end->other_ends_index_].partners_index_;
    if (NO_PARTNER == *other_ends_partner_index) {
      break;
    }
  }

  seg->Join(segments_to_join_buffer_, join_at_segs_last);

  size_t to_join_size = segments_to_join_buffer_.size();
  for (size_t i = 0; i < to_join_size; ++i) {
    segments_to_join_buffer_[i].first->Clear();
    ++num_joined_;
  }
  segments_to_join_buffer_.clear();
}


// Returns how many more join opportunities are available. Used for validation
// of JoinSegments result (in non-release modes only).
template<class T>
unsigned int PolylineJoiner<T>::JoinableVertexCount(const T& glist) {
  unsigned int ret_val = 0;
  std::map<gstVertex, std::set<SegmentEnd>,
      fusion_gst::XYLexicographicLess<gstVertex> > endpoints;

  for (unsigned int g = 0; g < glist.size(); ++g) {
    const gstGeodeHandle &geodeh = glist.at(g);
    if (geodeh->FlatPrimType() != gstPolyLine &&
        geodeh->FlatPrimType() != gstStreet &&
        geodeh->NumParts() > 1) {
      notify(NFY_FATAL, "%s: Incorrect geode.", __func__);
    }

    if (!geodeh->IsEmpty()) {
      // For g'th segment we number its vertices as 2 * g and 2 * g +1
      const SegmentEnd first(g, false);
      const SegmentEnd last(g, true);

      const gstGeode *geode = static_cast<const gstGeode*>(&(*geodeh));
      const gstVertex& v1 = geode->GetFirstVertex(0);
      const gstVertex& v2 = geode->GetLastVertex(0);
      endpoints[v1].insert(first);
      endpoints[v2].insert(last);
    }
  }
  for (std::map<gstVertex, std::set<SegmentEnd> >::iterator endpoint =
       endpoints.begin(); endpoint != endpoints.end(); ++endpoint) {
    size_t num_segments = endpoint->second.size();
    if (num_segments == 2) {
      std::set<SegmentEnd>::iterator i = endpoint->second.begin();
      SegmentEnd one = *i;
      SegmentEnd other = *(++i);
      if (one.SegmentId() != other.SegmentId()) {
        ret_val++;
      }
    }
  }
  return ret_val;
}

// explicit instantiation
template class PolylineJoiner<GeodeList>;

}  // namespace vectorprep

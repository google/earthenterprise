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

#include "fusion/gst/gstPolygonBuilder.h"

#include "fusion/gst/gstGeomUtils.h"
#include "common/notify.h"
#include "common/khTileAddr.h"
//#include "common/khTypes.h"
#include <cstdint>


namespace {

// Gets previous halfedge in cycle.
template <int is_inner_cycle>
const fusion_gst::PcHalfedgeHandle& PrevInCycle(
    const fusion_gst::PcHalfedgeHandle &he, Int2Type<is_inner_cycle>);

// Gets previous halfedge in cycle. Specialization for inner cycle.
template <>
const fusion_gst::PcHalfedgeHandle& PrevInCycle<true>(
    const fusion_gst::PcHalfedgeHandle &he, Int2Type<true>) {
  assert(he->pred);
  return he->pred->pair;
}
// Gets previous halfedge in cycle. Specialization for outer cycle.
template <>
const fusion_gst::PcHalfedgeHandle& PrevInCycle<false>(
    const fusion_gst::PcHalfedgeHandle &he, Int2Type<false>) {
  assert(he->succ);
  return he->succ->pair;
}

// Gets next halfedge in cycle.
template <int is_inner_cycle>
const fusion_gst::PcHalfedgeHandle& NextInCycle(
    const fusion_gst::PcHalfedgeHandle &he, Int2Type<is_inner_cycle>);

// Gets next halfedge in cycle. Specialization for inner cycle.
template <>
const fusion_gst::PcHalfedgeHandle& NextInCycle<true>(
    const fusion_gst::PcHalfedgeHandle &he, Int2Type<true>) {
  assert(he->pair);
  return he->pair->succ;
}

// Gets next halfedge in cycle. Specialization for outer cycle.
template <>
const fusion_gst::PcHalfedgeHandle& NextInCycle<false>(
    const fusion_gst::PcHalfedgeHandle &he, Int2Type<false>) {
  assert(he->pair);
  return he->pair->pred;
}

// Converts Face's cycle to gstGeode's part;
// Int2Type<true> - convert inner cycle instantiation;
// Int2Type<false> - convert outer cycle instantiation;
template <int is_inner_cycle>
void ConvertCycleTo(const fusion_gst::PcCycle &cycle,
                    gstGeode *geode,
                    Int2Type<is_inner_cycle> is_inner) {
  notify(NFY_VERBOSE, "%s ...", __func__);

  fusion_gst::PcHalfedgeHandle cur_he = cycle.first_halfedge;

  if (!cur_he) {  // Cycle is empty;
    // It's a case when we get isolated halfedge (halfedge has no
    // successor/predecessor) in ComputeCycle(). See ComputeCycle()- we leave
    // cycle in Face but invalidate its halfedge-references;
    return;
  }

  if (!cycle.last_halfedge) {
    notify(NFY_FATAL, "Face has invalid cycle.");
  }

  geode->AddPart(8);  // TODO: number of vertexes;
  unsigned int vertex_counter = 0;
  while (cur_he != cycle.last_halfedge) {
    // Add vertex to geode part;
    const fusion_gst::PcVertex &v = cur_he->v;
    notify(NFY_VERBOSE, "Add vertex %d: (%f, %f, %f);",
           ++vertex_counter, v.x, v.y, v.z);

    geode->AddVertexAndEdgeFlag(gstVertex(v.x, v.y, v.z), cur_he->edge_type());

    // Get next halfedge in cycle.
    cur_he = NextInCycle(cur_he, is_inner);
  }

  // Add vertex from last halfedge.
  const fusion_gst::PcVertex &v_last = cycle.last_halfedge->v;
  geode->AddVertexAndEdgeFlag(gstVertex(v_last.x, v_last.y, v_last.z),
                              cycle.last_halfedge->edge_type());
  notify(NFY_VERBOSE, "Add vertex %d: (%f, %f, %f);",
         ++vertex_counter, v_last.x, v_last.y, v_last.z);

  // Add last vertex of cycle (last == first).
  // Note For last vertex that duplicates first one edge type value should be
  // taken from last_halfedge. See gstGeode::IsInternalVertex().
  const fusion_gst::PcVertex &v_first = cycle.first_halfedge->v;
  geode->AddVertexAndEdgeFlag(gstVertex(v_first.x, v_first.y, v_first.z),
                              cycle.last_halfedge->edge_type());
  notify(NFY_VERBOSE, "Add vertex %d: (%f, %f, %f);",
         ++vertex_counter, v_first.x, v_first.y, v_first.z);

  notify(NFY_VERBOSE, "%s done.", __func__);
}

// Computes cycle based on input halfedge.
// It uses primitive graph operation to work around cycle.
// he - first halfedge of cycle.
// cycle - computed cycle. The parameter can be NULL. In this case we visit all
// halfedges of cycle but do not create new cycle.
// Int2Type<true> - compute inner cycle instantiation;
// Int2Type<false> - compute outer cycle instantiation;
template <int is_inner_cycle>
void ComputeCycle(const fusion_gst::PcHalfedgeHandle &he,
                  fusion_gst::PcCycle *cycle,
                  Int2Type<is_inner_cycle> is_inner) {
  // TODO: support inner cycles that sharing a vertex.
  // Currently it is not required because we do holes cutting.

  notify(NFY_VERBOSE, "%s ...", __func__);

  fusion_gst::PcHalfedgeHandle first_he;
  fusion_gst::PcHalfedgeHandle cur_he;

  first_he = cur_he = he;

  if (cur_he->pred == cur_he && cur_he->succ == cur_he) {
    // TODO: this point should be not reachable.
    // It's temporary left for testing.
#ifndef NDEBUG
    notify(NFY_WARN,
           "ComputeCycle(): spike in cycle: (%.10f, %.10f)-(%.10f, %.10f)",
           khTilespace::Denormalize(cur_he->v.y),
           khTilespace::Denormalize(cur_he->v.x),
           khTilespace::Denormalize(cur_he->pair->v.y),
           khTilespace::Denormalize(cur_he->pair->v.x));
    notify(NFY_WARN, "normalized: (%.20f, %.20f)- (%.20f, %.20f)",
           cur_he->v.x, cur_he->v.y, cur_he->pair->v.x, cur_he->pair->v.y);
#endif

    cur_he->set_visited(true);
    if (cycle) {
      // invalidate current cycle by release references to first and
      // last halfedges.
      cycle->first_halfedge.release();
      cycle->last_halfedge.release();
    }
    return;
  }


  if (cycle) {
    // Set first halfedge of cycle.
    cycle->first_halfedge = first_he;
  }

  std::uint32_t count_verts = 0;
  do {
#if 0
    notify(NFY_NOTICE,
           "visited he: %p %p (%.10f, %.10f)-(%.10f, %.10f)",
           &(*cur_he), &(*cur_he->pair),
           khTilespace::Denormalize(cur_he->v.y),
           khTilespace::Denormalize(cur_he->v.x),
           khTilespace::Denormalize(cur_he->pair ? cur_he->pair->v.y : .0),
           khTilespace::Denormalize(cur_he->pair ? cur_he->pair->v.x : .0));
    notify(NFY_NOTICE, "visited he normalized: (%.20f, %.20f)-(%.20f, %.20f)",
           cur_he->v.x, cur_he->v.y,
           cur_he->pair ? cur_he->pair->v.x : .0,
           cur_he->pair ? cur_he->pair->v.y : .0);
#endif
    ++count_verts;

    // Set attributes for current halfedge.
    cur_he->set_visited(true);
    cur_he->SetFaceAttr(he->face_num(), he->cycle_num());

    // Get next halfedge of cycle.
    cur_he = NextInCycle(cur_he, is_inner);
  } while (/*!cur_he->visited() &&*/ (cur_he != first_he));

  ++count_verts;  // duplicate first vertex.
  if (cur_he == first_he && count_verts >= kMinCycleVertices) {
    if (cycle) {
      // Set last halfedge of cycle.
      cycle->last_halfedge = PrevInCycle(cur_he, is_inner);
    }
  } else {  // invalid cycle.
     // Invalidate current cycle by release references to first &
     // last halfedge.
    if (cycle) {
       cycle->first_halfedge.release();
       cycle->last_halfedge.release();
    }
  }

  notify(NFY_VERBOSE, "%s done.", __func__);
}

}  // namespace

namespace fusion_gst {

PcHalfedgeHandle PcHalfedge::CreatePair(const gstVertex &v0,
                                        const gstVertex &v1) {
  // Create halfedge based on ordered points and set attributes.
  PcHalfedgeHandle he = PcHalfedge::Create(v0);

  // Create opposite halfedge and set attributes.
  PcHalfedgeHandle opp_he = PcHalfedge::Create(v1);

  // Link created pair of halfedges.
  he->LinkPair(&opp_he);

  // Calculate edge attributes.
  he->CalculateDominatingPoint();
  he->CalculateInsideAreaLoc();

  return he;
}

// PcHalfedge
PcHalfedgeHandle PcHalfedge::CreatePair(const gstVertex &v0,
                                        const gstVertex &v1,
                                        const PcHalfedgeHandle &he) {
  assert(he->pair);

  // Create halfedge based on ordered points and set attributes.
  PcHalfedgeHandle new_he = PcHalfedge::Create(v0);

  // Create opposite halfedge and set attributes.
  PcHalfedgeHandle new_opp_he = PcHalfedge::Create(v1);

  // Link created pair of halfedges.
  new_he->LinkPair(&new_opp_he);

  // Calculate dominating point.
  new_he->CalculateDominatingPoint();

  // Initialize inside area location attribute.
  new_he->set_inside_area_loc(he->inside_area_loc());

  return new_he;
}

PcHalfedgeHandle PcHalfedge::CreateInternalPair(
    const gstVertex &v0,
    const gstVertex &v1,
    const PcInsideAreaLocationType inside_area_loc,
    const gstEdgeType edge_type) {
  // Create halfedge based on ordered point.
  PcHalfedgeHandle new_he =  PcHalfedge::Create(v0);

  // Create opposite halfedge and set attributes.
  PcHalfedgeHandle new_opp_he =  PcHalfedge::Create(v1);

  // Link created pair of halfedges.
  new_he->LinkPair(&new_opp_he);

  // Calculate dominating point.
  new_he->CalculateDominatingPoint();

  // Initialize inside area location attribute.
  new_he->set_inside_area_loc(inside_area_loc);

  // Initialize edge type.
  new_he->set_edge_type(edge_type);

  return new_he;
}

void PcHalfedge::CalculateDominatingPoint() {
  assert(pair);
  if (v.LessXY(pair->v)) {  // Left halfedge.
    dominating = kDominatingLeft;
    pair->dominating = kDominatingRight;
  } else {  // Right halfedge.
    dominating = kDominatingRight;
    pair->dominating = kDominatingLeft;
  }
}

void PcHalfedge::CalculateInsideAreaLoc() {
  assert(dominating != kDominatingNone);
  assert(pair);

  if (kDominatingLeft == dominating) {
    set_inside_area_loc(kInsideAreaLocAbove);
  } else {
    set_inside_area_loc(kInsideAreaLocBelow);
  }
}

// PcFace
void PcFace::ConvertTo(gstGeode *geode) const {
  if (CyclesSize() == 0) {
    notify(NFY_WARN, "Face without cycles.");
    return;
  }

  notify(NFY_VERBOSE, "%s: number of cycles: %u", __func__, CyclesSize());

  // Convert outer cycle;
  ConvertCycleTo(GetCycle(0), geode,
                 Int2Type<false>());  // is_inner_cycle is false;

  // Convert inner cycles;
  for (unsigned int part = 1; part < CyclesSize(); part++) {
    ConvertCycleTo(GetCycle(part), geode,
                   Int2Type<true>());  // is_inner_cycle is true;
  }
}

// CycleAcceptor
template<>
void CycleAcceptor<PcHalfedgeHandleList>::AddHalfedgePair(
    const gstVertex &v0, const gstVertex &v1) {
  PcHalfedgeHandle he = PcHalfedge::CreatePair(v0, v1);
  halfedges_->push_back(he);
  halfedges_->push_back(he->pair);
}

template<>
void CycleAcceptor<PcHalfedgeHandleMultiSet>::AddHalfedgePair(
    const gstVertex &v0, const gstVertex &v1) {
  PcHalfedgeHandle he = PcHalfedge::CreatePair(v0, v1);
  halfedges_->insert(he);
  halfedges_->insert(he->pair);
}

// PolygonBuilder

PolygonBuilderOptions PolygonBuilderOptions::NO_CUT_HOLES_NO_CLEAN() {
  return PolygonBuilderOptions();
}

PolygonBuilderOptions PolygonBuilderOptions::NO_CUT_HOLES_CLEAN() {
  PolygonBuilderOptions options;
  options.set_clean_overlapped_edges(true);
  return options;
}
PolygonBuilderOptions PolygonBuilderOptions::CUT_HOLES_NO_CLEAN() {
  PolygonBuilderOptions options;
  options.set_cut_holes(true);
  return options;
}

void PolygonBuilderOptions::set_cut_holes(const bool cut_holes) {
  cut_holes_ = cut_holes;
}

void PolygonBuilderOptions::set_clean_overlapped_edges(
    const bool clean_overlapped_edges) {
  clean_overlapped_edges_ = clean_overlapped_edges;
}

PolygonBuilder::PolygonBuilder(const PolygonBuilderOptions &options)
    : options_(options),
      out_halfedges_acceptor_(&out_halfedges_) {
}

PolygonBuilder::~PolygonBuilder() {
}

void PolygonBuilder::Reset() {
  // Clear output faces.
  out_faces_.clear();

  // Clear sweep status.
  sweep_status_.Clear();

  // Clear cut-halfedges;
  ClearHalfedgesList(&cut_halfedges_);

  // Clear output halfedges;
  ClearHalfedgesList(&out_halfedges_);
}

void PolygonBuilder::AcceptPolygon(const gstGeode *geode) {
  notify(NFY_VERBOSE, "%s ...", __func__);

  if (geode->IsDegenerate())
    return;

  // Accept data for further processing.
  for (unsigned int part = 0; part < geode->NumParts(); ++part) {
    notify(NFY_VERBOSE, "number vertexes in geode part %d: %d", part,
           geode->VertexCount(part));
    out_halfedges_acceptor_.Accept(geode, part);
  }
  notify(NFY_VERBOSE, "%s done.", __func__);
}

void PolygonBuilder::Run(GeodeList *pieces) {
  notify(NFY_VERBOSE, "%s ...", __func__);
  assert(pieces);

  if (Empty()) {
    return;    // no data for reconstruction.
  }

  // 1. Polygon Reconstruction step.
  if (options_.clean_overlapped_edges()) {
    // Link output halfedges around node and clean completely overlapped edges.
    LinkHalfedgesAndCleanOverlapped();

    // Reconstruct polygon from halfedges.
    ReconstructPolygon();
  } else {
    // Link output halfedges around node.
    LinkHalfedges();

    // Reconstruct polygon from halfedges.
    ReconstructPolygonAndSkipNotValidCycles();
  }

  // 2. Convert output faces to geodes.
  ReportPolygons(pieces);

  // 3. Reset internal intermediate structures.
  Reset();

  notify(NFY_VERBOSE, "%s done.", __func__);
}


void PolygonBuilder::LinkHalfedges() {
  notify(NFY_VERBOSE, "%s ...", __func__);

  PcVertex prev_dp = PcVertex::NaN();  // Not valid point.

  PcVertex cur_dp;
  PcHalfedgeHandle prev_he;
  for (PcHalfedgeHandleMultiSet::iterator it = out_halfedges_.begin();
       it != out_halfedges_.end(); ++it) {
    PcHalfedgeHandle cur_he = *it;
    assert(cur_he->pair);

    cur_dp = cur_he->v;

    if (!cur_dp.EqualsXY(prev_dp)) {
      // New or first dominating point reached.
      cur_he->pred = cur_he;
      cur_he->succ = cur_he;
    } else {
      assert(prev_he);
      // The same dominating point.
      cur_he->pred = prev_he;
      cur_he->succ = prev_he->succ;
      prev_he->succ = cur_he;
      cur_he->succ->pred = cur_he;
    }

    prev_dp = cur_dp;
    prev_he = cur_he;
  }

  notify(NFY_VERBOSE, "%s done.", __func__);
}

void PolygonBuilder::ReconstructPolygonAndSkipNotValidCycles() {
  notify(NFY_VERBOSE, "%s ...", __func__);

  // TODO: last_face_num, last_cycle_num[] instead of size();
  for (PcHalfedgeHandleMultiSet::iterator it = out_halfedges_.begin();
       it != out_halfedges_.end(); ++it) {
    PcHalfedgeHandle cur_he = *it;

    notify(NFY_VERBOSE,
           "current he: (%.20f, %.20f)-(%.20f, %.20f), inside_above: %d;",
           cur_he->v.x, cur_he->v.y, cur_he->pair->v.x, cur_he->pair->v.y,
           kInsideAreaLocAbove == cur_he->inside_area_loc());

    if ((kDominatingLeft == cur_he->dominating) && (!cur_he->visited())) {
      PcHalfedgeHandle outer_he;
      unsigned int existing_face_num = GetFaceNumber(cur_he, &outer_he);

      if (kInsideAreaLocAbove == cur_he->inside_area_loc()) {
        // New outer cycle.
        if (static_cast< unsigned int> (-1) != existing_face_num) {
#ifndef NDEBUG
          // Outer cycle inside another outer cycle.
          notify(NFY_WARN, "Invalid outer cycle (inside another one):"
                 " (%.20f, %.20f)-(%.20f, %.20f): skipped.",
                 khTilespace::Denormalize(cur_he->Left().y),
                 khTilespace::Denormalize(cur_he->Left().x),
                 khTilespace::Denormalize(cur_he->Right().y),
                 khTilespace::Denormalize(cur_he->Right().x));
          notify(NFY_WARN, "normalized:(%.20f, %.20f)-(%.20f, %.20f)",
                 cur_he->Left().x, cur_he->Left().y,
                 cur_he->Right().x, cur_he->Right().y);

          // outer he
          notify(NFY_WARN, "Invalid outer cycle (inside another one) outer_he:"
                 " (%.20f, %.20f)-(%.20f, %.20f): skipped.",
                 khTilespace::Denormalize(outer_he->Left().y),
                 khTilespace::Denormalize(outer_he->Left().x),
                 khTilespace::Denormalize(outer_he->Right().y),
                 khTilespace::Denormalize(outer_he->Right().x));

          notify(NFY_WARN, "outer_he normalized:(%.20f, %.20f)-(%.20f, %.20f)",
                 outer_he->Left().x, outer_he->Left().y,
                 outer_he->Right().x, outer_he->Right().y);
#endif

          // Skip invalid outer cycle.
          cur_he->SetFaceAttr(static_cast< unsigned int> (-1), static_cast< unsigned int> (-1));
          ComputeCycle(cur_he, NULL,        // NULL - do not create new cycle.
                       Int2Type<false>());  // is_inner_cycle is false.
        } else {
          notify(NFY_VERBOSE, "Add outer cycle");

          // Create new face and new cycle.
          out_faces_.push_back(PcFace());
          unsigned int new_face_num = out_faces_.size() - 1;
          PcFace& cur_face = out_faces_[new_face_num];
          cur_face.AddCycle();
          unsigned int new_cycle_num = cur_face.CyclesSize() - 1;
          cur_he->SetFaceAttr(new_face_num, new_cycle_num);
          ComputeCycle(cur_he,
                       &(cur_face.GetLastCycle()),
                       Int2Type<false>());  // is_inner_cycle is false.
        }
      } else {  // New inner cycle.
        // Get first halfedge of inner cycle.
        PcHalfedgeHandle &first_he = cur_he->succ;
        if (!first_he) {
          notify(NFY_FATAL, "Invalid halfedges set: not linked.");
        }

        notify(NFY_VERBOSE, "existing_face_num: %u", existing_face_num);

        if (static_cast< unsigned int> (-1) != existing_face_num) {  // Valid cycle.
          if ((existing_face_num >= out_faces_.size()) ||
              (0 == out_faces_[existing_face_num].CyclesSize())) {
            notify(NFY_FATAL, "Invalid face number.");
          }

          notify(NFY_VERBOSE, "Add inner cycle.");

          // TODO: make a policy CUT/NOCUT holes.
          if (options_.cut_holes()) {
            // add current inner cycle to existing face and make hole cut.
            AddHoleToOuterCycle(first_he, outer_he);
          } else {
            // Create new cycle in existing face - no holes cutting.
            PcFace& cur_face = out_faces_[existing_face_num];
            cur_face.AddCycle();
            unsigned int new_cycle_num = cur_face.CyclesSize() - 1;
            first_he->SetFaceAttr(existing_face_num, new_cycle_num);
            ComputeCycle(first_he,
                         &(cur_face.GetLastCycle()),
                         Int2Type<true>());  // is_inner_cycle is true.
          }
        } else if ((cur_he->pred == cur_he &&  // Spike before polygon.
                    cur_he->succ == cur_he) ||
                   // Spike after polygon.
                   (cur_he->pair->pred == cur_he->pair &&
                    cur_he->pair->succ == cur_he->pair)) {
          // It is the case when source halfedge lies on one of the quad's
          // edges and an incident polygon's part is completely outside
          // the quad.
          // TODO: This code should not be reachable. Currently
          // it's left for testing.
          cur_he->set_visited(true);

#ifndef NDEBUG
          notify(NFY_WARN, "ReconstructPolygon():"
                 " Spike in polygon cycle:(%.10f, %.10f)-(%.10f, %.10f)",
                 khTilespace::Denormalize(cur_he->Left().y),
                 khTilespace::Denormalize(cur_he->Left().x),
                 khTilespace::Denormalize(cur_he->Right().y),
                 khTilespace::Denormalize(cur_he->Right().x));
          notify(NFY_WARN, "normalized:(%.20f, %.20f)-(%.20f, %.20f)",
                 cur_he->Left().x, cur_he->Left().y,
                 cur_he->Right().x, cur_he->Right().y);
#endif

        } else {  // Invalid inner cycle.
#ifndef NDEBUG
          notify(NFY_WARN, "Invalid inner cycle in polygon"
                 " (edge of inner cycle:(%.10f, %.10f)-(%.10f, %.10f)"
                 " is located outside outer cycle of polygon): skipped.",
                 khTilespace::Denormalize(cur_he->Left().y),
                 khTilespace::Denormalize(cur_he->Left().x),
                 khTilespace::Denormalize(cur_he->Right().y),
                 khTilespace::Denormalize(cur_he->Right().x));
          notify(NFY_WARN, "normalized:(%.20f, %.20f)-(%.20f, %.20f)",
                 cur_he->Left().x, cur_he->Left().y,
                 cur_he->Right().x, cur_he->Right().y);
#endif

          // Skip invalid inner cycle.
          first_he->SetFaceAttr(static_cast< unsigned int> (-1), static_cast< unsigned int> (-1));
          ComputeCycle(first_he, NULL,     // NULL - do not create new cycle.
                       Int2Type<true>());  // is_inner_cycle is true.
        }
      }
    }

#ifndef NDEBUG
    if (!cur_he->visited()) {
      notify(NFY_WARN,
             "Invalid halfedges set (cur_he has status not visited):"
             " (%.10f, %.10f)-(%.10f, %.10f)",
             khTilespace::Denormalize(cur_he->Left().y),
             khTilespace::Denormalize(cur_he->Left().x),
             khTilespace::Denormalize(cur_he->Right().y),
             khTilespace::Denormalize(cur_he->Right().x));
      notify(NFY_WARN, "normalized: (%.20f, %.20f)-(%.20f, %.20f)",
             cur_he->v.x, cur_he->v.y, cur_he->pair->v.x, cur_he->pair->v.y);
    }
#endif

    // Update sweep line status.
    sweep_status_.Update(cur_he);
  }

  notify(NFY_VERBOSE, "%s done.", __func__);
}

 unsigned int PolygonBuilder::GetFaceNumber(const PcHalfedgeHandle &he,
                                   PcHalfedgeHandle *const outer_he) {
  notify(NFY_VERBOSE, "%s...", __func__);
  assert(outer_he != NULL);
  PcHalfedgeHandle prev = sweep_status_.PrevOfEdge(he);

  if (prev /*&& prev->visited()*/) {
    assert(prev->visited());
    notify(NFY_VERBOSE,
           "Prev: (%.10f, %.10f)-(%.10f, %.10f),"
           "Prev normalized: (%.20f, %.20f)-(%.20f, %.20f),"
           " inside area loc above: %d, face num: %u",
           khTilespace::Denormalize(prev->v.y),
           khTilespace::Denormalize(prev->v.x),
           khTilespace::Denormalize(prev->pair->v.y),
           khTilespace::Denormalize(prev->pair->v.x),
           prev->v.x, prev->v.y, prev->pair->v.x, prev->pair->v.y,
           (kInsideAreaLocAbove == prev->inside_area_loc()),
           prev->face_num());
#if 1
    if (options_.clean_overlapped_edges()) {
      // Cleaning, we do not use information about inside area location
      // from source geometry. We define it based on sweepline status and
      // use it to fix a cycle orientation.
      if (sweep_status_.CountEdges(prev) & 0x1) {
        *outer_he = prev;
        return (*outer_he)->face_num();
      }
    } else {
      if (kInsideAreaLocAbove == prev->inside_area_loc()) {
        *outer_he = prev;
        return (*outer_he)->face_num();
      }
    }
#else
    // TODO: switch to this branch when inside_area_loc
    // property is demoted.
    if (sweep_status_.CountEdges(prev) & 0x1) {
      *outer_he = prev;
      return (*outer_he)->face_num();
    }
#endif
  }

  return -1;
}

void PolygonBuilder::AddHoleToOuterCycle(PcHalfedgeHandle &he,
                                         PcHalfedgeHandle &outer_he) {
  notify(NFY_VERBOSE, "%s ...", __func__);
  notify(NFY_VERBOSE, "outer he: (%f, %f)-(%f, %f), face num %u,"
         " inside_above: %d.",
         outer_he->v.x, outer_he->v.y, outer_he->pair->v.x, outer_he->pair->v.y,
         outer_he->face_num(),
         kInsideAreaLocAbove == outer_he->inside_area_loc());

  // 1. Compute point on the outer boundary to connect inner cycle
  // (connection point).
  PcVertex connection(he->v);
  // Compute y-, z- coordinates of connection point.
  const PcVertex &pt1_line = outer_he->Left();
  const PcVertex &pt2_line = outer_he->Right();
  connection.y = GetLineYFromX(pt1_line, pt2_line, connection.x);
  GetLineZFromXY(pt1_line, pt2_line, &connection);

  notify(NFY_VERBOSE, "Connection point: (%f, %f, %f)",
         connection.x, connection.y, connection.z);

  // 2. Calculate previous and next halfedges relative to connection point.
  PcHalfedgeHandle new_opp_outer_he1;
  PcHalfedgeHandle new_opp_outer_he2;
  if (connection.EqualsXY(outer_he->Left())) {
    // Connection point coincides with vertex of outer cycle.
    new_opp_outer_he1 = outer_he->succ;
    new_opp_outer_he2 = outer_he;
  } else {  // Connection point does not coincide with vertex of outer cycle.
    // Split outer edge into two edges (outer halfedges pair into two pair of
    // halfedges).
    PcHalfedgeHandle opp_outer_he = outer_he->pair;

    // Update sweep line status: remove halfedge that will be split.
    sweep_status_.Update(opp_outer_he);

    // Create opposite halfedge and set attributes for first part of edge.
    new_opp_outer_he1 =
        PcHalfedge::Create(gstVertex(connection.x, connection.y, connection.z));
    new_opp_outer_he1->dominating = kDominatingRight;
    outer_he->LinkPair(&new_opp_outer_he1);
    new_opp_outer_he1->CopyAttr(outer_he);
    new_opp_outer_he1->set_visited(true);

    // Create opposite halfedge and set attributes for second part of edge.
    new_opp_outer_he2 =
        PcHalfedge::Create(gstVertex(connection.x, connection.y, connection.z));
    new_opp_outer_he2->dominating = kDominatingLeft;
    new_opp_outer_he2->LinkPair(&opp_outer_he);
    new_opp_outer_he2->CopyAttr(outer_he);
    new_opp_outer_he2->set_visited(true);

    // Store new halfedges created as result of splitting in temporary list
    // in order to properly release after run.
    cut_halfedges_.push_back(new_opp_outer_he1);
    cut_halfedges_.push_back(new_opp_outer_he2);

    // Update sweep line status: insert second part of split halfedge.
    sweep_status_.Update(new_opp_outer_he2);
  }

  // 3. Create cut-edges pair.
  PcHalfedgeHandle cut_he = PcHalfedge::CreateInternalPair(
      gstVertex(connection.x, connection.y, connection.z),
      gstVertex(he->v.x, he->v.y, he->v.z),
      outer_he->inside_area_loc(), kCutEdge);

  // Store cut-halfedges in temporary list in order to properly release
  // after run.
  cut_halfedges_.push_back(cut_he);
  cut_halfedges_.push_back(cut_he->pair);

  // 4. Link created halfedges around connection vertex.
  new_opp_outer_he1->pred = cut_he;
  new_opp_outer_he1->succ = new_opp_outer_he2;
  new_opp_outer_he2->pred = new_opp_outer_he1;
  new_opp_outer_he2->succ = cut_he;
  cut_he->pred = new_opp_outer_he2;
  cut_he->succ = new_opp_outer_he1;

  // 5. Link opposite cut-halfedge to inner cycle vertex.
  PcHalfedgeHandle tmp_he =  he->succ;
  PcHalfedgeHandle &opp_cut_he = cut_he->pair;
  he->succ = opp_cut_he;
  opp_cut_he->pred = he;
  opp_cut_he->succ = tmp_he;
  tmp_he->pred = opp_cut_he;

  // 6. Compute cut-up inner cycle as outer one.
  he->SetFaceAttr(outer_he->face_num(), outer_he->cycle_num());
  ComputeCycle(he, NULL,            // NULL - do not create new cycle.
               Int2Type<false>());  // is_inner_cycle is false.

  notify(NFY_VERBOSE, "%s done.", __func__);
}

void PolygonBuilder::ReportPolygons(GeodeList *pieces) {
  notify(NFY_VERBOSE, "%s ...", __func__);
  // Convert output faces to geodes.
  for (std::deque<PcFace>::const_iterator it = out_faces_.begin();
       it != out_faces_.end(); ++it) {
    if (it->IsValid()) {
      // TODO: compose all parts in one multi-polygon.
      gstGeodeHandle new_geodeh = gstGeodeImpl::Create(gtype_);
      gstGeode *new_geode = static_cast<gstGeode*>(&(*new_geodeh));
      it->ConvertTo(new_geode);
      pieces->push_back(new_geodeh);
    }
  }
  notify(NFY_VERBOSE, "%s done.", __func__);
}

// Note: after linking, the out_halfedges list contains
// inappropriate (not linked) halfedges, that should be skipped in further
// processing.
void PolygonBuilder::LinkHalfedgesAndCleanOverlapped() {
  notify(NFY_VERBOSE, "%s ...", __func__);

  PcVertex prev_dp = PcVertex::NaN();  // Not valid point.

  PcVertex cur_dp;
  PcHalfedgeHandle prev_he;
  PcHalfedgeHandle first_he;
  for (PcHalfedgeHandleMultiSet::iterator it = out_halfedges_.begin();
       it != out_halfedges_.end(); ++it) {
    PcHalfedgeHandle cur_he = *it;
    if (!cur_he->pair)
      continue;

    cur_dp = cur_he->v;
    if (!cur_dp.EqualsXY(prev_dp)) {
      // New dominating point is reached.
      if (!prev_dp.IsNaN() && first_he) {
        // Clean up equal halfedges for previous dominating point.
        PcHalfedgeHandle cur_he_link = first_he;
        first_he.release();

        PcHalfedgeHandle first_he_link = cur_he_link;
        PcHalfedgeHandle prev_he_link = first_he_link;
        cur_he_link = prev_he_link->succ;
        do {
          if (*cur_he_link == *prev_he_link) {
            // remove equal halfedges from dominating point linked list and
            // make them and their pair inappropriate.
            prev_he_link->pred->succ = cur_he_link->succ;
            cur_he_link->succ->pred = prev_he_link->pred;
            PcHalfedgeHandle tmp_he_link = prev_he_link->pred;
            cur_he_link->pair->Clean();
            cur_he_link->Clean();
            prev_he_link->pair->Clean();
            prev_he_link->Clean();
            if (prev_he_link == first_he_link) {
              first_he_link = tmp_he_link;
            }
            prev_he_link = tmp_he_link;
            cur_he_link = prev_he_link->succ;
          } else {
            prev_he_link = cur_he_link;
            cur_he_link = cur_he_link->succ;
          }
        } while (cur_he_link && (cur_he_link != first_he_link));
      }

      if (!cur_he->pair)
        continue;  // cur_he is inappropriate.

      first_he = cur_he;
      cur_he->pred = cur_he;
      cur_he->succ = cur_he;
    } else {
      // The same dominating point.
      cur_he->pred = prev_he;
      cur_he->succ = prev_he->succ;
      prev_he->succ = cur_he;
      cur_he->succ->pred = cur_he;
    }

    prev_dp = cur_dp;
    prev_he = cur_he;
  }

  notify(NFY_VERBOSE, "%s done.", __func__);
}

void PolygonBuilder::ReconstructPolygon() {
  notify(NFY_VERBOSE, "%s ...", __func__);

  // TODO: last_face_num, last_cycle_num[] instead of size();
  for (PcHalfedgeHandleMultiSet::iterator it = out_halfedges_.begin();
       it != out_halfedges_.end(); ++it) {
    PcHalfedgeHandle cur_he = *it;
    if (!cur_he->pair) {  // skip not linked halfedges.
      continue;
    }
    assert(cur_he->pair->pair);

    notify(NFY_VERBOSE,
           "current he: (%.20f, %.20f)-(%.20f, %.20f), inside_above: %d;",
           cur_he->v.x, cur_he->v.y, cur_he->pair->v.x, cur_he->pair->v.y,
           kInsideAreaLocAbove == cur_he->inside_area_loc());

    if ((kDominatingLeft == cur_he->dominating) && (!cur_he->visited())) {
      PcHalfedgeHandle outer_he;
      unsigned int existing_face_num = GetFaceNumber(cur_he, &outer_he);

      if (static_cast< unsigned int> (-1) == existing_face_num) {
        // New outer cycle.
        notify(NFY_VERBOSE, "Add outer cycle");

        // Create new face and new cycle.
        out_faces_.push_back(PcFace());
        unsigned int new_face_num = out_faces_.size() - 1;
        PcFace& cur_face = out_faces_[new_face_num];
        cur_face.AddCycle();
        unsigned int new_cycle_num = cur_face.CyclesSize() - 1;
        cur_he->SetFaceAttr(new_face_num, new_cycle_num);
        ComputeCycle(cur_he,
                     &(cur_face.GetLastCycle()),
                     Int2Type<false>());  // is_inner_cycle is false.
      } else {  // New inner cycle.
        // Get first halfedge of inner cycle.
        PcHalfedgeHandle &first_he = cur_he->succ;
        if (!first_he) {
          notify(NFY_FATAL, "Invalid halfedges set: not linked.");
        }

        notify(NFY_VERBOSE, "existing_face_num: %u", existing_face_num);

        if ((existing_face_num >= out_faces_.size()) ||
            (0 == out_faces_[existing_face_num].CyclesSize())) {
          notify(NFY_FATAL, "Invalid face number.");
        }

        notify(NFY_VERBOSE, "Add inner cycle.");

        if (options_.cut_holes()) {
          // add current inner cycle to existing face and make hole cut.
          AddHoleToOuterCycle(first_he, outer_he);
        } else {
          // Create new cycle in existing face - no holes cutting.
          PcFace& cur_face = out_faces_[existing_face_num];
          cur_face.AddCycle();
          unsigned int new_cycle_num = cur_face.CyclesSize() - 1;
          first_he->SetFaceAttr(existing_face_num, new_cycle_num);
          ComputeCycle(first_he,
                       &(cur_face.GetLastCycle()),
                       Int2Type<true>());  // is_inner_cycle is true.
        }
      }
    }

#ifndef NDEBUG
    if (!cur_he->visited()) {
      notify(NFY_WARN,
             "Invalid halfedges set (cur_he has status not visited):"
             " (%.10f, %.10f)-(%.10f, %.10f)",
             khTilespace::Denormalize(cur_he->Left().y),
             khTilespace::Denormalize(cur_he->Left().x),
             khTilespace::Denormalize(cur_he->Right().y),
             khTilespace::Denormalize(cur_he->Right().x));
      notify(NFY_WARN, "normalized: (%.20f, %.20f)-(%.20f, %.20f)",
             cur_he->v.x, cur_he->v.y, cur_he->pair->v.x, cur_he->pair->v.y);
    }
#endif

    // Update sweep line status.
    sweep_status_.Update(cur_he);
  }

  notify(NFY_VERBOSE, "%s done.", __func__);
}

}  // namespace fusion_gst

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


#include "fusion/gst/gstBoxCutter.h"

#include <deque>

#include "fusion/gst/gstGeomUtils.h"

// #define HYBRID_BOX_CUT_ON 1

namespace fusion_gst {

BoxCutter::BoxCutter(bool cut_holes)
    : polygon_clipper_(cut_holes) {
}

BoxCutter::BoxCutter(const gstBBox &_box, bool cut_holes)
    : box_(_box),
      polygon_clipper_(_box, cut_holes)  {
}

BoxCutter::~BoxCutter() {
}

void BoxCutter::SetClipRect(const double _wx1, const double _wx2,
                            const double _wy1, const double _wy2) {
  box_.init(_wx1, _wx2, _wy1, _wy2);
  polygon_clipper_.SetClipRect(box_);
}

void BoxCutter::SetClipRect(const gstBBox &_box) {
  box_ = _box;
  polygon_clipper_.SetClipRect(box_);
}

unsigned int BoxCutter::Run(const gstGeodeHandle &geodeh,
                    GeodeList &pieces,
                    bool *completely_covered) {
  pieces.clear();

  // Should be initialized each run because it can be overridden in
  // ClipPolygon3D();
  polygon_clipper_.SetClipRect(box_);

  // Check bounding boxes intersection.
  if (!box_.Intersect(geodeh->BoundingBox())) {
    return 0;  // no intersection.
  }

  // Compute intersection.
  switch (geodeh->PrimType()) {
    case gstUnknown:
    case gstPoint:
    case gstPoint25D:
      break;

    case gstPolyLine:
    case gstPolyLine25D:
    case gstStreet:
    case gstStreet25D:
      {
        const gstGeode *geode = static_cast<const gstGeode*>(&(*geodeh));
        ClipLines(geode, &pieces);
      }
      break;

    case gstPolygon:
    case gstPolygon25D:
#ifdef MULTI_POLYGON_ON
#  ifdef HYBRID_BOX_CUT_ON
      if (geodeh->NumParts() > 1) {
        polygon_clipper_.Run(geodeh, &pieces, completely_covered);
      } else {
        (static_cast<const gstGeode*>(&(*geodeh)))->BoxCut(box_, &pieces);
      }
#  else
      polygon_clipper_.Run(geodeh, &pieces, completely_covered);
#  endif
#else
      (static_cast<const gstGeode*>(&(*geodeh)))->BoxCut(box_, &pieces);
#endif
      break;

    case gstPolygon3D:
#ifdef MULTI_POLYGON_ON
#  ifdef HYBRID_BOX_CUT_ON
      if (geodeh->NumParts() > 1) {
        ClipPolygon3D(static_cast<const gstGeode*>(&(*geodeh)), &pieces,
                      completely_covered);
      } else {
        (static_cast<const gstGeode*>(&(*geodeh)))->BoxCut(box_, &pieces);
      }
#  else
      ClipPolygon3D(static_cast<const gstGeode*>(&(*geodeh)), &pieces,
                    completely_covered);
#  endif
#else
      (static_cast<const gstGeode*>(&(*geodeh)))->BoxCut(box_, &pieces);
#endif
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
#ifdef HYBRID_BOX_CUT_ON
      {
        const gstGeodeCollection *multi_geode =
            static_cast<const gstGeodeCollection*>(&(*geodeh));
        for (unsigned int p = 0; p < multi_geode->NumParts(); ++p) {
          bool _completely_covered = false;
          const gstGeodeHandle geode_parth = multi_geode->GetGeode(p);
          if (geode_parth->NumParts() > 1) {
            polygon_clipper_.Run(geode_parth, &pieces, &_completely_covered);
            *completely_covered = *completely_covered || _completely_covered;
          } else {
            const gstGeode *geode_part =
              static_cast<const gstGeode*>(&(*geode_parth));
            geode_part->BoxCut(box_, &pieces);
          }
        }
      }
#else
      polygon_clipper_.Run(geodeh, &pieces, completely_covered);
#endif
      break;

    case gstMultiPolygon3D:
      {
        const gstGeodeCollection *multi_geode =
            static_cast<const gstGeodeCollection*>(&(*geodeh));
        for (unsigned int p = 0; p < multi_geode->NumParts(); ++p) {
          const gstGeode *geode =
              static_cast<const gstGeode*>(&(*multi_geode->GetGeode(p)));
#ifdef HYBRID_BOX_CUT_ON
          bool _completely_covered = false;
          if (geode->NumParts() > 1) {
            ClipPolygon3D(geode, &pieces, &_completely_covered);
            *completely_covered = *completely_covered || _completely_covered;
          } else {
            geode->BoxCut(box_, &pieces);
          }
#else
          ClipPolygon3D(geode, &pieces, completely_covered);
#endif
        }
      }
      break;
  }
  notify(NFY_DEBUG, "number of pieces: %u", static_cast< unsigned int> (pieces.size()));

#ifndef NDEBUG
  for (size_t i = 0; i < pieces.size(); ++i) {
    notify(NFY_DEBUG, "piece num: %lu, type: %d, number of vertices: %u",
           i, pieces[i]->PrimType(), pieces[i]->TotalVertexCount());
  }
#endif

  return pieces.size();
}

void BoxCutter::ClipLines(const gstGeode *geode,
                          GeodeList *pieces) const {
  // JOBSTATS_SCOPED(boxcut_stats, JOBSTATS_POLYLINE);
  std::deque<Segment> segments;

  // Iterate across all segments, one at a time
  // and attempt to cut out a piece that fits into the supplied box.
  for (unsigned int p = 0; p < geode->NumParts(); ++p) {
    for (unsigned int v = 0; v < geode->VertexCount(p) - 1; ++v) {
      gstVertex v0 = geode->GetVertex(p, v);
      gstVertex v1 = geode->GetVertex(p, v + 1);

      gstVertex pt0 = v0;
      gstVertex pt1 = v1;

      int clipped = 0;
      if (clipped = box_.ClipLine(&pt0, &pt1)) {
        if (2 == clipped) {  // Segment was cut.
          // Calculate z-coordinate for clipping points.
          GetLineZFromXY(v0, v1, &pt0);
          GetLineZFromXY(v0, v1, &pt1);
        }
        segments.push_back(Segment(pt0, pt1));
      }
    }
  }

  if (!segments.empty()) {
    // Combine all segments for roads.
    if (geode->FlatPrimType() == gstStreet) {
      CombineOrderedSegments(segments, geode->PrimType(), pieces);
      // Polylines stay as segments so duplicates can be removed.
    } else {
      ConvertSegmentsToGeodes(segments, geode->PrimType(), pieces);
    }
  }
}

void BoxCutter::CombineOrderedSegments(
    const std::deque<Segment>& segments,
    const gstPrimType geode_prim_type,
    GeodeList *pieces) const {
  gstGeodeHandle new_geodeh = gstGeodeImpl::Create(geode_prim_type);
  pieces->push_back(new_geodeh);

  std::deque<Segment>::const_iterator seg = segments.begin();
  gstVertex prev = seg->v0;

  gstGeode *new_geode = static_cast<gstGeode*>(&(*new_geodeh));

  new_geode->AddVertexToPart0(gstVertex(seg->v0));

  for (; seg != segments.end(); ++seg) {
    // start a new subpart?
    if (seg->v0 != prev) {
      new_geodeh = gstGeodeImpl::Create(geode_prim_type);
      pieces->push_back(new_geodeh);

      new_geode = static_cast<gstGeode*>(&(*new_geodeh));
      new_geode->AddVertexToPart0(seg->v0);
    }

    new_geode->AddVertexToPart0(seg->v1);

    prev = seg->v1;
  }
}

void BoxCutter::ConvertSegmentsToGeodes(
    const std::deque<Segment>& segments,
    const gstPrimType geode_prim_type,
    GeodeList *pieces) const {
  for (std::deque<Segment>::const_iterator seg = segments.begin();
       seg != segments.end(); ++seg) {
    gstGeodeHandle new_geodeh = gstGeodeImpl::Create(geode_prim_type);
    pieces->push_back(new_geodeh);

    gstGeode *new_geode = static_cast<gstGeode*>(&(*new_geodeh));

    new_geode->AddPart(2);  // To optimize with vector reserve.
    new_geode->AddVertexToPart0(seg->v0);
    new_geode->AddVertexToPart0(seg->v1);
  }
}

void BoxCutter::ClipPolygon3D(const gstGeode *geode,
                              GeodeList *pieces, bool *completely_covered) {
  // for 3D polygons we perform clipping in 2D and compute z-values for clipped
  // coords from the plane equation.
  Vert3<double> normal;
  double distance;
  double north = box_.n;
  double south = box_.s;
  double west = box_.w;
  double east = box_.e;

  if (!geode->ComputePlaneEquation(&normal, &distance)) {
    return;  // Plane equation couldn't be computed, so geode is invalid.
    // Such geodes should be filtered on stage of vector-import
    // so just return without notification.
  }

  bool swapX = true;
  // In case of a vertical polygon we will swap either the x with z or y with z
  // depending upon the corresponding component of the normal and
  // then clip against the rotated.
  if (!normal.z && !normal.x) {
    swapX = false;
  }

  gstGeodeHandle geode2Dh = gstGeodeImpl::Create(geode->PrimType());

  gstGeode *geode2D = static_cast<gstGeode*>(&(*geode2Dh));

  for (unsigned int part = 0; part < geode->NumParts(); ++part) {
    geode2D->AddPart(geode->VertexCount(part));
    for (unsigned int v = 0; v < geode->VertexCount(part); ++v) {
      const gstVertex &vert = geode->GetVertex(part, v);
      if (!normal.z && swapX) {  // TODO: optimize.
        geode2D->AddVertex(gstVertex(vert.z, vert.y, .0));
        west = geode->MinZ(0);
        east = geode->MaxZ(0);
      } else if (!normal.z && !swapX) {
        geode2D->AddVertex(gstVertex(vert.x, vert.z, .0));
        south = geode->MinZ(0);
        north = geode->MaxZ(0);
      } else {
        geode2D->AddVertex(gstVertex(vert.x, vert.y, .0));
      }
    }
  }

  GeodeList new_pieces;
  polygon_clipper_.SetClipRect(west, east, south, north);
  polygon_clipper_.Run(geode2Dh, &new_pieces, completely_covered);

  // Compute the 3rd component from the plane equation.
  for (unsigned int p = 0; p < new_pieces.size(); ++p) {
    gstGeodeHandle &new_geodeh = new_pieces[p];

    gstGeode *new_geode = static_cast<gstGeode*>(&(*new_geodeh));

    assert(new_geode->NumParts() == 1);

    for (unsigned int i = 0; i < new_geode->VertexCount(0); ++i) {
      const gstVertex &vert = new_geode->GetVertex(0, i);

      if (!normal.z && swapX) {  // TODO: optimize
        double new_x = (distance - normal.y * vert.y) / normal.x;
        new_geode->ModifyVertex(0, i, gstVertex(new_x, vert.y, vert.x));
      } else if (!normal.z && !swapX) {
        double new_y = (distance - normal.x * vert.x) / normal.y;
        new_geode->ModifyVertex(0, i, gstVertex(vert.x, new_y, vert.y));
      } else {
        double new_z =
            (distance - (normal.x * vert.x) - (normal.y * vert.y)) / normal.z;
        new_geode->ModifyVertex(0, i, gstVertex(vert.x, vert.y, new_z));
      }
    }
  }

  pieces->insert(pieces->end(), new_pieces.begin(), new_pieces.end());
}

}  // namespace fusion_gst

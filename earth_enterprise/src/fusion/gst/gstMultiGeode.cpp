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


#include "fusion/gst/gstGeode.h"


// namespace fusion_gst {

// gstGeodeCollection
gstGeodeCollection::gstGeodeCollection(gstPrimType t)
    : gstGeodeImpl(t) {
}

gstGeodeCollection::~gstGeodeCollection() {
}

gstGeodeHandle gstGeodeCollection::Duplicate() const {
  gstGeodeHandle new_multi_geodeh = gstGeodeImpl::Create(PrimType());
  gstGeodeCollection *new_multi_geode =
    static_cast<gstGeodeCollection*>(&(*new_multi_geodeh));

  for (GeodeList::const_iterator it = geodes_.begin();
       it != geodes_.end(); ++it) {
    gstGeodeHandle new_geode = (*it)->Duplicate();
    new_multi_geode->AddGeode(new_geode);
  }

  return new_multi_geodeh;
}

void gstGeodeCollection::ChangePrimType(gstPrimType new_type,
                                        gstGeodeHandle *new_geodeh) {
  if (PrimType() != gstMultiPolygon &&
      PrimType() != gstMultiPolygon25D &&
      PrimType() != gstMultiPolygon3D) {
    notify(NFY_FATAL, "Invalid feature \"from-\" primary type in conversion.");
  }

  // TODO: remove after support of multi-polyline.
  // Special processing for gstPolyLine as single geode type.
  if (gstPolyLine == ToFlatPrimType(new_type) ||
      gstStreet == ToFlatPrimType(new_type)) {
    // Convert each part (polygon) of multi-geometry feature to part
    // of polyline.
    gstGeodeHandle new_gh = gstGeodeImpl::Create(new_type);
    gstGeode *new_g = static_cast<gstGeode*>(&*(new_gh));
    for (unsigned int p = 0; p < NumParts(); ++p) {
      const gstGeodeHandle &geodeh = GetGeode(p);
      for (unsigned int pp = 0; pp < geodeh->NumParts(); ++pp) {
        const gstGeode *geode = static_cast<const gstGeode*>(&(*geodeh));
        new_g->AddPart(geode->VertexCount(pp));
        for (unsigned int v = 0; v < geode->VertexCount(pp); ++v) {
          new_g->AddVertex(geode->GetVertex(pp, v));
        }
      }
    }
    // return new multi-geometry geode as result of converting.
    if (new_geodeh) {
      (*new_geodeh).swap(new_gh);
    }
  } else {
    if (gstPolygon == new_type) {
      prim_type_ = static_cast<std::int8_t>(gstMultiPolygon);
    } else if (gstPolygon25D == new_type) {
      prim_type_ = static_cast<std::int8_t>(gstMultiPolygon25D);
    } else if (gstPolygon3D == new_type) {
      prim_type_ = static_cast<std::int8_t>(gstMultiPolygon3D);
    } else {
      notify(NFY_FATAL,
             "Invalid feature \"to-\" primary type in conversion.");
    }
    // convert all geodes in collection.
    for (GeodeList::const_iterator it = geodes_.begin();
         it != geodes_.end(); ++it) {
      (*it)->ChangePrimType(new_type);
    }
  }
}

bool gstGeodeCollection::IsEmpty() const {
  for (GeodeList::const_iterator it = geodes_.begin();
       it != geodes_.end(); ++it) {
    if (!(*it)->IsEmpty()) {
      return false;
    }
  }

  return true;
}

bool gstGeodeCollection::IsDegenerate() const {
  for (GeodeList::const_iterator it = geodes_.begin();
       it != geodes_.end(); ++it) {
    if (!(*it)->IsDegenerate()) {
      return false;
    }
  }

  return true;
}

unsigned int gstGeodeCollection::NumParts() const {
  return geodes_.size();
}

unsigned int gstGeodeCollection::TotalVertexCount() const {
  unsigned int total = 0;
  for (GeodeList::const_iterator it = geodes_.begin();
       it != geodes_.end(); ++it) {
    total += (*it)->TotalVertexCount();
  }

  return total;
}

void gstGeodeCollection::Clear() {
  geodes_.clear();
}

gstBBox gstGeodeCollection::BoundingBoxOfPart(unsigned int part) const {
  assert(part < NumParts());
  // First parameter of BoundingBoxOfPart() is 0 (index of polygon's outer
  // cycle);
  return geodes_[part]->BoundingBoxOfPart(0);
}

bool gstGeodeCollection::Equals(const gstGeodeHandle &bh,
                                bool reverse_ok) const {
  if (PrimType() != bh->PrimType())
    return false;

  if (NumParts() != bh->NumParts())
    return false;

  int p = 0;
  for (GeodeList::const_iterator it = geodes_.begin();
       it != geodes_.end(); ++it, ++p) {
    const gstGeodeHandle &geode =
      (static_cast<const gstGeodeCollection*>(&(*bh)))->GetGeode(p);
    if (!(*it)->Equals(geode, reverse_ok)) {
      return false;
    }
  }

  return true;
}

// Computes center of mass and area for specified part.
int gstGeodeCollection::Centroid(unsigned int part,
                                 double *x, double *y, double *area) const {
  assert(part < NumParts());
  // First parameter of Centroid() is 0 (index of polygon's outer cycle);
  return geodes_[part]->Centroid(0, x, y, area);
}

gstVertex gstGeodeCollection::GetPointInPolygon(
    unsigned int part, const gstVertex &centroid) const {
  assert(part < NumParts());
  // First parameter of GetPointInPolygon() is 0 (index of polygon's outer
  // cycle);
  return geodes_[part]->GetPointInPolygon(0, centroid);
}


gstVertex gstGeodeCollection::Center() const {
  if (center_is_valid_)
    return center_;

  double area;
  double largest_area = 0.0;
  double x, y;
  int largest_part_index = 0;
  gstVertex centroid_of_largest_part;

  // Calculate largest part (polygon) in multi-geode.
  for (unsigned int p = 0; p < NumParts(); ++p) {
    // First parameter of Centroid() is 0 (index of polygon's outer cycle);
    if (geodes_[p]->Centroid(0, &x, &y, &area) == 0) {
      if (fabs(area) > largest_area) {
        largest_area = fabs(area);
        centroid_of_largest_part = gstVertex(x, y);
        largest_part_index = p;
      }
    }
  }

  // Must return a point that actually lies in the largest polygon
  // (largest_part_index).
  // First parameter of GetPointInPolygon() is 0 (index of polygon's outer
  // cycle);
  center_ = geodes_[largest_part_index]->GetPointInPolygon(
      0, centroid_of_largest_part);
  center_is_valid_ = true;

  return center_;
}


unsigned int gstGeodeCollection::Flatten(GeodeList* pieces) const {
  for (GeodeList::const_iterator it = geodes_.begin();
       it != geodes_.end(); ++it) {
    (*it)->Flatten(pieces);
  }
  return pieces->size();
}

void gstGeodeCollection::Draw(
    const gstDrawState& state,
    const gstFeaturePreviewConfig &preview_config) const {
  for (GeodeList::const_iterator it = geodes_.begin();
       it != geodes_.end(); ++it) {
    (*it)->Draw(state, preview_config);
  }
}

int gstGeodeCollection::RemoveCollinearVerts() {
  int num_removed_verts = 0;
  GeodeList tmp_geodes;
  for (GeodeList::const_iterator it = geodes_.begin();
       it != geodes_.end(); ++it) {
    num_removed_verts += (*it)->RemoveCollinearVerts();

    // Check part for degenerate after removing collinear vertices to remove
    // from collection.
    if (!(*it)->IsDegenerate()) {
      tmp_geodes.push_back((*it));
    }
  }

  geodes_.swap(tmp_geodes);

  return num_removed_verts;
}

void gstGeodeCollection::ApplyScaleFactor(double scale) {
  for (GeodeList::const_iterator it = geodes_.begin();
       it != geodes_.end(); ++it) {
    (*it)->ApplyScaleFactor(scale);
  }
}

bool gstGeodeCollection::Intersect(const gstBBox& b, uint* part) const {
  if (part)
    *part = 0;

  bool is_intersecting = false;
  unsigned int cur_part = 0;
  for (GeodeList::const_iterator it = geodes_.begin();
       it != geodes_.end(); ++it, ++cur_part) {
    if ((*it)->Intersect(b, NULL)) {
      if (part)
        *part = cur_part;

      is_intersecting = true;
      break;
    }
  }
  return is_intersecting;
}

 std::uint32_t gstGeodeCollection::RawSize() const {
  std::uint32_t size = sizeof(RecHeader);

  for (GeodeList::const_iterator it = geodes_.begin();
           it != geodes_.end(); ++it) {
    size += (*it)->RawSize();
  }
  return size;
}

char* gstGeodeCollection::ToRaw(char* buf) const {
  int buff_size = RawSize();

  // passing NULL means we allocate space
  if (buf == NULL)
    buf = reinterpret_cast<char*>(malloc(buff_size * sizeof(char)));

  RecHeader *hdr = reinterpret_cast<RecHeader*>(buf);
  hdr->type = PrimType();
  hdr->count = NumParts();
  hdr->size = buff_size;                 // size of this object
  hdr->fid = 0;                          // feature id

  char *tbuf = buf + sizeof(RecHeader);
  for (GeodeList::const_iterator it = geodes_.begin();
           it != geodes_.end(); ++it) {
    (*it)->ToRaw(tbuf);
    tbuf += (*it)->RawSize();
  }

  return buf;
}

void gstGeodeCollection::ComputeBounds() const {
  if (IsEmpty())
    return;

  for (GeodeList::const_iterator it = geodes_.begin();
           it != geodes_.end(); ++it) {
    bounding_box_.Grow((*it)->BoundingBox());
  }
}

// gstMultiPoly
gstMultiPoly::gstMultiPoly(gstPrimType t)
    : gstGeodeCollection(t) {
}

gstMultiPoly::~gstMultiPoly() {
}

// }  // namespace fusion_gst

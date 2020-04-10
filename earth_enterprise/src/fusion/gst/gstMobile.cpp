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


#include <limits>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <GL/gl.h>

#include <gstMobile.h>
#include <gstGridUtils.h>


bool MobileBlockImpl::init_gfx_ = false;
unsigned int MobileBlockImpl::ramp_texture_id_ = 0;
//uint MobileBlockImpl::road_texture_id_ = 0;

MobileBlockHandle MobileBlockImpl::Create(const khTileAddr& addr, int sg,
                                          double width) {
  return khRefGuardFromNew(new MobileBlockImpl(addr, sg, width));
}

MobileBlockImpl::MobileBlockImpl(const khTileAddr& addr, int snap_grid,
                                 double width)
    : addr_(addr), width_(width) {
  snap_grid_ = static_cast<double>(snap_grid);
}

void MobileBlockImpl::InitGfx() {
  // build up an alpha texture for our ramp
  // the ramp goes linearly from 0 -> max -> 0 over our range
  // so half way through the range should be max
  // the ramp size should be a power of 2 to match a valid texture size
  const int kRampSize = 64;
  const int kMaxVal = 255;
  const float kRampStep = static_cast<float>(kMaxVal) /
                          static_cast<float>((kRampSize >> 1) - 1);
  std::uint8_t tex_buff[kRampSize];
  for (int i = 0; i < (kRampSize >> 1); ++i) {
    tex_buff[i] = static_cast<int>((i * kRampStep) + 0.5);
    tex_buff[kRampSize - i - 1] = tex_buff[i];
  }

  // configure line texture
  glGenTextures(1, &ramp_texture_id_);
  glBindTexture(GL_TEXTURE_2D, ramp_texture_id_);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, kRampSize, 1, 0, GL_ALPHA,
               GL_UNSIGNED_BYTE, static_cast<void*>(tex_buff));
#if 0
  //
  // make a road texture that looks like the google maps road
  const int kRoadImageSize = 64;
  GLubyte road_image[kRoadImageSize][4];
  for (int i = 0; i < kRoadImageSize; ++i) {
    if (i < 2 || i >= 62) {
      road_image[i][0] = 0;
      road_image[i][1] = 0;
      road_image[i][2] = 0;
      road_image[i][3] = 0;
    } else if (i < 8 || i >= 56) {
      road_image[i][0] = 50;
      road_image[i][1] = 50;
      road_image[i][2] = 50;
      road_image[i][3] = 200;
    } else {
      road_image[i][0] = 200;
      road_image[i][1] = 200;
      road_image[i][2] = 0;
      road_image[i][3] = 255;
    }
  }

  glGenTextures(1, &road_texture_id_);
  glBindTexture(GL_TEXTURE_2D, road_texture_id_);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
               kRoadImageSize, 1, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, road_image);
#endif
  init_gfx_ = true;
}

void MobileBlockImpl::Draw(const gstDrawState& state,
                           const BlendFuncModes& blend_modes) {
  if (!init_gfx_)
    InitGfx();

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  double width = state.cullbox.Width();
  double height = state.cullbox.Height();
  double grid_step = Grid::Step(addr_.level);

  double scale_x = (width / grid_step) * (snap_grid_);
  double scale_y = (height / grid_step) * (snap_grid_);

  glOrtho(0, scale_x, 0, scale_y, -1, 1);

  float trans_x = (static_cast<float>(addr_.col) -
                   (state.cullbox.w / grid_step)) * (snap_grid_);
  float trans_y = (static_cast<float>(addr_.row) -
                   (state.cullbox.s / grid_step)) * (snap_grid_);

  glTranslatef(trans_x, trans_y, 0);

  //
  // draw decimation grid
  //
  if (state.mode & DrawGrids) {
    //glEnable(GL_LINE_STIPPLE);
    //glLineStipple(1, 0xAAAA);
    glColor3f(0.7, 0.0, 0.7);
    glLineWidth(1);
    glBegin(GL_LINES);
    for (int r = 0; r < snap_grid_; r += 4096) {
      glVertex2f(0, r);
      glVertex2f(snap_grid_ - 1, r);
      glVertex2f(r, 0);
      glVertex2f(r, snap_grid_ - 1);
    }
    glEnd();
    //glDisable(GL_LINE_STIPPLE);
  }

  //
  // draw features here
  //
  // first polys
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnable(GL_BLEND);
  glBlendFunc(blend_modes.src_factor, blend_modes.dest_factor);
  glLineWidth(2);
  glPointSize(3);
  unsigned int off = 0;
  for (std::vector< unsigned int> ::const_iterator it = counts_.begin();
       it != counts_.end(); ++it) {
    glVertexPointer(2, GL_SHORT, 0, &vertexes_[off]);
    glColor4f(0.0, 0.8, 0.8, 0.8);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, *it);
    off += *it;
  }

  glDisable(GL_BLEND);

  glDisableClientState(GL_VERTEX_ARRAY);
  glPopMatrix();
}

gstBBox MobileBlockImpl::GetBoundingBox() {
  double grid_step = Grid::Step(addr_.level);
  double w = static_cast<double>(addr_.col) * grid_step;
  double e = w + grid_step;
  double s = static_cast<double>(addr_.row) * grid_step;
  double n = s + grid_step;
  return gstBBox(w, e, s, n);
}

MobileVertex MobileBlockImpl::SnapVertexToLattice(double origin_x,
                                                  double origin_y,
                                                  double scale,
                                                  const gstVert2D& vert) {
  int x = static_cast<int>(((vert.x - origin_x) / scale) + 0.5);
  int y = static_cast<int>(((vert.y - origin_y) / scale) + 0.5);

  if (x > std::numeric_limits<MobileScalar>::max())
    x = std::numeric_limits<MobileScalar>::max();
  if (y > std::numeric_limits<MobileScalar>::max())
    y = std::numeric_limits<MobileScalar>::max();

  return MobileVertex(static_cast<MobileScalar>(x),
                      static_cast<MobileScalar>(y));
}

bool MobileBlockImpl::AddGeometry(const gstGeodeHandle geode) {
  if (geode->VertexCount(0) < 2)
    return false;

  double grid_step = Grid::Step(addr_.level);
  double origin_x = static_cast<double>(addr_.col) * grid_step;
  double origin_y = static_cast<double>(addr_.row) * grid_step;
  double scale = grid_step / (snap_grid_);

  notify(NFY_DEBUG, "l:%d r:%u c:%u vertex length:%d  -",
         addr_.level, addr_.row, addr_.col, geode->VertexCount(0));

  //
  // build center-lines
  //

  vertexes_.reserve(geode->VertexCount(0));
  for (unsigned int v = 0; v < geode->VertexCount(0); ++v) {
    const gstVertex& vert = geode->GetVertex(0, v);
    vertexes_.push_back(SnapVertexToLattice(origin_x, origin_y, scale,
                                            gstVert2D(vert.x, vert.y)));
  }
  counts_.push_back(geode->VertexCount(0));

#if 0
  //
  // build polygons
  //
  int vertex_count = 0;
  gstVert2D prev_v0;
  gstVert2D prev_v1;
  gstVert2D prev_v2;
  gstVert2D prev_v3;
  for (unsigned int n = 0; n < geode->VertexCount(0) - 1; ++n) {
    const gstVert2D a(geode->GetVertex(0, n).x, geode->GetVertex(0, n).y);
    const gstVert2D b(geode->GetVertex(0, n + 1).x, geode->GetVertex(0, n + 1).y);

    // find the simple vector of this segment
    gstVert2D v(b.x - a.x, b.y - a.y);
    v.Normalize();

    // rotate
    gstVert2D r(-v.y, v.x);

    // scale
    gstVert2D s(r.x * width_, r.y * width_);

    // find four courners of extruded line
    gstVert2D v0(a.x + s.x, a.y + s.y);
    gstVert2D v1(a.x - s.x, a.y - s.y);
    gstVert2D v2(b.x + s.x, b.y + s.y);
    gstVert2D v3(b.x - s.x, b.y - s.y);

    if (n == 0) {
      vertexes_.push_back(SnapVertexToLattice(origin_x, origin_y, scale, v0));
      vertexes_.push_back(SnapVertexToLattice(origin_x, origin_y, scale, v1));
      vertex_count += 2;
    } else {
      // skip any vertex that is on the same line as the previous two
      if (!prev_v2.AlmostEqual(v0, 0.0000001)) {
        // find four corners of extruded line
        gstVert2D v4 = gstVert2D::Intersect(prev_v0, prev_v2, v0, v2);
        gstVert2D v5 = gstVert2D::Intersect(prev_v1, prev_v3, v1, v3);

        vertexes_.push_back(SnapVertexToLattice(origin_x, origin_y, scale, v4));
        vertexes_.push_back(SnapVertexToLattice(origin_x, origin_y, scale, v5));
        vertex_count += 2;
      }
    }

    // special case end
    if (n == geode->VertexCount(0) - 2) {
      vertexes_.push_back(SnapVertexToLattice(origin_x, origin_y, scale, v2));
      vertexes_.push_back(SnapVertexToLattice(origin_x, origin_y, scale, v3));
      vertex_count += 2;
    }

    prev_v0 = v0;
    prev_v1 = v1;
    prev_v2 = v2;
    prev_v3 = v3;
  }
  counts_.push_back(vertex_count);
#endif

  return true;
}

void MobileBlockImpl::ReduceGeometry(std::list<MobileVertex>* snap_verts) {
  if (snap_verts->size() == 0)
    return;
  // remove duplicates
  std::list<MobileVertex>::iterator last = snap_verts->begin();
  std::list<MobileVertex>::iterator next = last;
  ++next;
  while (next != snap_verts->end()) {
    if (*last == *next)
      next = snap_verts->erase(next);
    else {
      last = next;
      ++next;
    }
  }

  // remove degenerates
  int before_size, after_size;
  do {
    before_size = snap_verts->size();
    if (before_size < 3)
      break;

    std::list<MobileVertex>::iterator a = snap_verts->begin();
    std::list<MobileVertex>::iterator b = a;
    ++b;
    std::list<MobileVertex>::iterator c = b;
    ++c;
    while (c != snap_verts->end()) {
      if (*a == *c) {
        b = snap_verts->erase(b);
        c = b;
        ++c;
      } else {
        ++a;
        ++b;
        ++c;
      }
    }

    after_size = snap_verts->size();
  } while (before_size != after_size);
}

 std::uint32_t MobileBlockImpl::ExportSize() const {
  std::uint32_t size = 0;

  // primitive type
  size += sizeof(std::uint16_t);

  // number of prims
  size += sizeof(std::uint16_t);

  // length of each prim
  size += counts_.size() * sizeof(std::uint16_t);

  // vertex data
  size += vertexes_.size() * sizeof(MobileVertex);

  return size;
}

 std::uint32_t MobileBlockImpl::ExportRaw(char* buff) const {
  char* orig_buff = buff;

  // primitive type
  std::uint16_t* prim_type = reinterpret_cast<std::uint16_t*>(buff);
  *prim_type = Roads;
  buff += sizeof(std::uint16_t);

  // number of prims
  std::uint16_t* num_parts = reinterpret_cast<std::uint16_t*>(buff);
  *num_parts = static_cast<std::uint16_t>(counts_.size());
  buff += sizeof(std::uint16_t);

  // length of each prim
  std::uint16_t* counts = reinterpret_cast<std::uint16_t*>(buff);
  for (unsigned int p = 0; p < counts_.size(); ++p) {
    counts[p] = counts_[p];
  }
  buff += sizeof(std::uint16_t) * counts_.size();

  // vertex data
  MobileVertex* verts = reinterpret_cast<MobileVertex*>(buff);
  for (unsigned int v = 0; v < vertexes_.size(); ++v)
    verts[v] = vertexes_[v];
  buff += sizeof(MobileVertex) * vertexes_.size();

  return buff - orig_buff;
}

MobileBlockHandle MobileBlockImpl::FromRaw(const khTileAddr& addr,
                                           const char* buff) {
  MobileBlockHandle new_block = khRefGuardFromNew(
      new MobileBlockImpl(addr, kBlockLatticeRes, 0.001));

  //const char* tmp_buff = buff;

  // primitive type
  const std::uint16_t* prim_type = reinterpret_cast<const std::uint16_t*>(buff);
  new_block->prim_type_ = *prim_type;
  buff += sizeof(std::uint16_t);

  // number of prims
  const std::uint16_t* num_parts = reinterpret_cast<const std::uint16_t*>(buff);
  new_block->counts_.reserve(*num_parts);
  buff += sizeof(std::uint16_t);

  // length of each prim
  int total_length = 0;
  const std::uint16_t* counts = reinterpret_cast<const std::uint16_t*>(buff);
  for (unsigned int p = 0; p < *num_parts; ++p) {
    new_block->counts_.push_back(counts[p]);
    total_length += counts[p];
  }
  buff += sizeof(std::uint16_t) * (*num_parts);

  // vertex data
  new_block->vertexes_.reserve(total_length);
  const MobileVertex* verts = reinterpret_cast<const MobileVertex*>(buff);
  for (int v = 0; v < total_length; ++v)
    new_block->vertexes_.push_back(verts[v]);
  //buff += sizeof(MobileVertex) * total_length;

  return new_block;
}

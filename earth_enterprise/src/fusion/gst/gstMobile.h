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

#ifndef KHSRC_FUSION_GST_GSTMOBILE_H__
#define KHSRC_FUSION_GST_GSTMOBILE_H__

#include <list>
#include <vector>

#include <khRefCounter.h>
#include <khGuard.h>
#include <gstGeode.h>

const int kBlockLatticeRes = 65536;
typedef std::uint16_t MobileScalar;
typedef Vert2<MobileScalar> MobileVertex;

// ----------------------------------------------------------------------------

class MobileBlockImpl;
typedef khRefGuard<MobileBlockImpl> MobileBlockHandle;
class MobileBlockImpl : public khRefCounter {
 public:
  static MobileBlockHandle Create(const khTileAddr& addr,
                                  int snap_grid, double width);
  ~MobileBlockImpl() {}

  struct BlendFuncModes {
    int src_factor;
    int dest_factor;
    int equation;
  };

  enum {
    Lines   = 1,
    Roads   = 2,
    Borders = 3
  };

  void Draw(const gstDrawState& state, const BlendFuncModes& modes);
  void InitGfx();

  MobileVertex SnapVertexToLattice(double origin_x, double origin_y,
                                   double scale, const gstVert2D& vert);

  bool AddGeometry(const gstGeodeHandle geode);
  void ReduceGeometry(std::list<MobileVertex>* snap_verts);
  void ConvertLinesToPolys();
  gstBBox GetBoundingBox();

  int Level() const { return addr_.level; }
  int Row() const { return addr_.row; }
  int Col() const { return addr_.col; }

  // converting to and from a raw memory buffer
  std::uint32_t ExportSize() const;
  std::uint32_t ExportRaw(char* buff) const;
  static MobileBlockHandle FromRaw(const khTileAddr& addr, const char* buf);

  // only initialize once for all blocks
  static bool init_gfx_;
  static unsigned int ramp_texture_id_;
  //static unsigned int road_texture_id_;

 private:
  friend class khRefGuard<MobileBlockImpl>;

  // private and unimplemented -- illegal to construct, copy or copy-construct
  MobileBlockImpl();
  MobileBlockImpl(const MobileBlockImpl& b);
  MobileBlockImpl& operator=(const MobileBlockImpl& b);

  MobileBlockImpl(const khTileAddr& addr, int snap_grid, double width);

  khTileAddr addr_;
  double snap_grid_;
  double width_;

  int prim_type_;
  std::vector<MobileVertex> vertexes_;
  std::vector< unsigned int>  counts_;
};

#endif // !KHSRC_FUSION_GST_GSTMOBILE_H__

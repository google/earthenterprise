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

#ifndef KHSRC_FUSION_GST_GSTTEXTURE_H__
#define KHSRC_FUSION_GST_GSTTEXTURE_H__

#include <khTileAddr.h>
#include <image.h>
#include <gstThreadCache.h>
#include <gstTypes.h>
#include <gstLimits.h>
#include <notify.h>
#include <gstBBox.h>
#include <gstImage.h>
#include <khMTTypes.h>
#include <gstEarthStream.h>
#include <gstGLUtils.h>

class gstTextureManager;
struct TileExistance;

// -----------------------------------------------------------------------------

class gstTextureImpl : public khMTRefCounter {
 public:
  gstTextureImpl(void) : enabled_(false) {}
  virtual ~gstTextureImpl() {}

  const gstGLTexSpec &glspec() const { return spec_; }
  bool enabled() const { return enabled_; }
  void setEnabled(bool e) { enabled_ = e; }
  const gstBBox &bbox() const { return bbox_; }
  int cellSize() const { return spec_.CellSize(); }
  int id() const { return id_; }

 protected:
  friend class gstTextureManager;

  virtual bool Load(std::uint64_t addr, unsigned char *obuff) = 0;
  virtual void Find(TileExistance *te, std::uint64_t addr) = 0;
  // This base virtual function sets default to false. When the derived
  // gstTextureImpl, is a mercator imagery, it will override this virtual method
  // to return false.
  virtual bool IsMercatorImagery() const { return false; }

  void id(int i) { id_ = i; }

  gstGLTexSpec spec_;
  bool enabled_;
  gstBBox bbox_;
  int id_;
};

// -----------------------------------------------------------------------------

typedef khRefGuard<gstTextureImpl> gstTextureGuard;

gstTextureGuard NewPYRTexture(const std::string& kippath,
                              const std::string& kmppath);

class MapLayerConfig;
class QString;
gstTextureGuard NewMapTexture(const MapLayerConfig& cfg,
                              gstTextureGuard prev,
                              QString &error,
                              const bool is_mercator_preview);

gstTextureGuard NewHTTPTexture(const std::string& server);

gstTextureGuard NewGEIndexTexture(const std::string& path);

gstTextureGuard NewGEDBTexture(const std::string& path);

void xyToBlist(unsigned int xbits, unsigned int ybits, int blevel, unsigned char blist[32]);

// -----------------------------------------------------------------------------

struct TileExistance {
  std::uint64_t bestAvailable;
};


class TexTile {
 public:
  TexTile(int l, unsigned int r, unsigned int c)
      : s0(0),
        s1(1),
        t0(0),
        t1(1),
        lev(l),
        row(r),
        col(c),
        step_(1),
        bit_(0),
        s_(0),
        t_(0) {
    src = alpha_ = 0;
  }
  TexTile(std::uint64_t a)
    : s0(0), s1(1), t0(0), t1(1),
      step_(1), bit_(0), s_(0), t_(0) {
    addr(a);
  }
  TexTile(double x, double y, double g, int l, int alpha = 0)
      : s0(0),
        s1(1),
        t0(0),
        t1(1),
        xx(x),
        yy(y),
        grid(g),
        lev(l),
        alpha_(alpha),
        step_(1),
        bit_(0),
        s_(0),
        t_(0) {
    row = uint(yy / grid);
    col = uint(xx / grid);
    src = 0;
  }

  int upLevel() {
    if (lev == 0)
      return -1;

    step_ *= 0.5;
    s_ |= ((col & 0x1) << bit_);
    t_ |= ((row & 0x1) << bit_);
    bit_++;

    col >>= 1;
    row >>= 1;
    --lev;

    s0 = s_ * step_;
    s1 = s0 + step_;
    t0 = t_ * step_;
    t1 = t0 + step_;

    return lev;
  }

  std::uint64_t addr() const { return TILEADDR(lev, row, col, alpha_, src); }
  void addr(std::uint64_t a) {
    lev = LEVFROMADDR(a);
    row = ROWFROMADDR(a);
    col = COLFROMADDR(a);
    alpha_ = SUBFROMADDR(a);
    src = SRCFROMADDR(a);
  }

  bool isValid() const {
    if (xx < 0.0 || xx >= 1.0 || yy < 0.0 || yy >= 1.00) return false;
    if (lev < 0 || lev > MaxFusionLevel)
      return false;
    return true;
  }

  double s0, s1, t0, t1;
  double xx, yy, grid;
  unsigned int lev;
  unsigned int row, col;
  unsigned int src;

  unsigned int alpha() const { return alpha_; }

 private:
  unsigned int alpha_;
  double step_;
  unsigned int bit_;
  unsigned int s_;
  unsigned int t_;
};

#endif  // !KHSRC_FUSION_GST_GSTTEXTURE_H__

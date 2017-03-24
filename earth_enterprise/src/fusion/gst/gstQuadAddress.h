/*
 * Copyright 2017 Google Inc.
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

#ifndef KHSRC_FUSION_GST_GSTQUADADDRESS_H__
#define KHSRC_FUSION_GST_GSTQUADADDRESS_H__

#include <gst/gstBBox.h>

class gstQuadAddress {
 public:
  gstQuadAddress(uint32 l = 0, uint32 r = 0, uint32 c = 0)
      : level_(l), row_(r), col_(c) {
    calcBBox();
    validate();
  }

  gstQuadAddress(const gstQuadAddress& b) {
    level_ = b.level();
    row_ = b.row();
    col_ = b.col();
    calcBBox();
    validate();
  }

  gstQuadAddress &operator=(const gstQuadAddress& b) {
    level_ = b.level();
    row_ = b.row();
    col_ = b.col();
    calcBBox();
    validate();
    return *this;
  }

  int operator==(const gstQuadAddress& b) const {
    return (level_ == b.level()) && (row_ == b.row()) && (col_ == b.col());
  }

  int operator!=(const gstQuadAddress &b) const {
    return (level_ != b.level()) || (row_ != b.row()) || (col_ != b.col());
  }

  //
  // overload the logical AND (&) operator which is used by khHashTable
  // to generate a hash key
  //
  // these prime numbers were picked with some simple analysis of a
  // test database to see that it was distributed evenly
  //
  int operator&(int d) const {
    return ((level_ * 151) + (row_ * 733) + (col_ * 1013)) & d;
  }

  uint32 level() const { return level_; }
  uint32 row() const { return row_; }
  uint32 col() const { return col_; }

  void blist(uchar* blist) const {
    uint64 right, top;
    static const int order[][2] = { {0, 3}, {1, 2} };

    for (uint32 j = 0; j < level_; ++j) {
      right = col_ & (0x1U << (level_ - j - 1));
      top   = row_ & (0x1U << (level_ - j - 1));
      blist[j] = order[right != 0][top != 0];
    }
  }

  std::string BlistAsString() const {
    uchar branch_list[32];
    char child_names[] = "0123";
    blist(branch_list);
    std::string blist_string;
    for (uint lev = 0; lev < level_; ++lev)
      blist_string += child_names[branch_list[lev]];
    return blist_string;
  }

  gstQuadAddress snapUp() const {
    int up = level_ % 4;
    return gstQuadAddress(level_ - up, row_ >> up, col_ >> up);
  }

  gstQuadAddress up(int n) const {
    return gstQuadAddress(level_ - n, row_ >> n, col_ >> n);
  }

  //
  // children are addressed as follows
  //
  enum {
    LowerLeft   = 0x00,
    LowerRight  = 0x01,
    UpperRight  = 0x02,
    UpperLeft   = 0x03
  };


  gstQuadAddress child(int addr) const {
    static const int rmap[] = {0, 0, 1, 1};
    static const int cmap[] = {0, 1, 1, 0};
    return gstQuadAddress(level_ + 1,
                          (row_ << 1) + rmap[addr],
                          (col_ << 1) + cmap[addr]);
  }

  const gstBBox &bbox() const { return bbox_; }

  bool valid() const { return valid_; }

 private:
  void validate() {
    uint32 max = 0x01U << level_;
    if (row_ >= max || col_ >= max) {
      valid_ = false;
    } else {
      valid_ = true;
    }
  }

  void calcBBox() {
    // const double grid = 256.0 / pow(2, level_ + 8);
    const double grid = 1.0 / static_cast<double>(0x01 << level_);
    bbox_ = gstBBox(col_ * grid, (col_ + 1) * grid,
                    row_ * grid, (row_ + 1) * grid);
  }

  uint32 level_;
  uint32 row_;
  uint32 col_;
  gstBBox bbox_;
  bool valid_;
};

#endif  // !KHSRC_FUSION_GST_GSTQUADADDRESS_H__

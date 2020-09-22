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


#include "fusion/gst/gstGeoIndex.h"

#include <GL/gl.h>
#include <algorithm>

#include "fusion/gst/gstConstants.h"
#include "fusion/gst/gstMisc.h"
#include "fusion/gst/gstGeode.h"
#include "fusion/gst/gstSourceManager.h"
#include "common/khInsetCoverage.h"
#include "common/khFileUtils.h"
#include "common/khException.h"

gstGeoIndexHandle gstGeoIndexImpl::Load(const std::string &select_file,
                                        const gstSharedSource &source,
                                        const khTilespace &tilespace,
                                        const double oversize_factor,
                                        const unsigned int target_level) {
  return khRefGuardFromNew(new gstGeoIndexImpl(select_file, source, tilespace,
                                               oversize_factor, target_level));
}

gstGeoIndexImpl::gstGeoIndexImpl()
    : tilespace_(ClientVectorTilespace),
      oversize_factor_(0.0),
      grid_(),
      box_list_(),
      box_index_list_(),
      bounding_box_(),
      coverage_(),
      presence_mask_(),
      coverage_mask_(),
      target_level_(0) {
  Init();
}

gstGeoIndexImpl::gstGeoIndexImpl(const khLevelCoverage &cov,
                                 const khTilespace &tilespace,
                                 double oversize_factor)
    : tilespace_(tilespace),
      oversize_factor_(oversize_factor),
      grid_(),
      box_list_(),
      box_index_list_(),
      bounding_box_(),
      coverage_(cov),
      presence_mask_(),
      coverage_mask_(),
      target_level_(0) {
  Init();
}

gstGeoIndexImpl::gstGeoIndexImpl(const std::string &select_file,
                                 const gstSharedSource &source,
                                 const khTilespace &tilespace,
                                 const double oversize_factor,
                                 const unsigned int target_level)
    : tilespace_(tilespace),
      oversize_factor_(oversize_factor),
      grid_(),
      box_list_(),
      box_index_list_(),
      bounding_box_(),
      coverage_(),
      presence_mask_(),
      coverage_mask_(),
      target_level_(target_level) {
  Init();
  ThrowingReadFile(select_file, source->Id());
}

void gstGeoIndexImpl::Init() {
  box_list_ = FeatureList::Create();
}

// Prepare index for a new insert cycle.
void gstGeoIndexImpl::Reset() {
  bounding_box_.Invalidate();
  for (FeatureGridIterator it = grid_.begin();
       it != grid_.end(); ++it) {
    it->clear();
  }
  box_list_ = FeatureList::Create();
  box_index_list_.clear();
  coverage_ = khLevelCoverage();
  presence_mask_.clear();  // Delete old presence mask.
  coverage_mask_.clear();  // Delete old coverage mask.
}

// Each insert adds a single feature id and it's corresponding bounding box.
void gstGeoIndexImpl::Insert(int feature_id, const gstBBox& bounding_box) {
  assert(box_list_);
  box_list_->push_back(FeatureHandle(feature_id, bounding_box));
  box_index_list_.push_back(box_list_->size() - 1);
  assert(box_list_->size() == box_index_list_.size());
  bounding_box_.Grow(bounding_box);
}

// Each insert adds a single feature index and it's corresponding bounding box.
void gstGeoIndexImpl::InsertIndex(unsigned int idx) {
  assert(box_list_);
  box_index_list_.push_back(idx);
  bounding_box_.Grow((*box_list_)[idx].bounding_box);
}


// Once all features are inserted, compute the dimensions of our grid index
// and insert the features into each cell that their bounding box intersects.
void gstGeoIndexImpl::Finalize(void) {
  if (!bounding_box_.Valid())
    return;

  if (coverage_.empty()) {
    // pick a new size for the grid based on how many objects
    // figure that the shapes are perfectly distributed geographically
    // then we would put 100 features in each bucket
    std::uint32_t size = static_cast<std::uint32_t>(sqrt(box_index_list_.size()) * 0.1);
    std::uint64_t targetTotal = size * size;
    std::uint64_t minTotal = 100;
    std::uint64_t maxTotal = 1000000;

    // now find the largest level that has fewer tiles than our max
    khExtents<double> normExtents(NSEWOrder,
                                  bounding_box_.n, bounding_box_.s,
                                  bounding_box_.e, bounding_box_.w);
    notify(NFY_DEBUG, "----------");
    // Our (super)tile size is 2048 pxl (FusionMapTilespace.tileSizeLog2) and we
    // need oversize of one map tile 256 pxl
    // (FusionMapTilespace.pixelsAtLevel0Log2) in all directions. Oversize of
    // factor .25 effectively adds 1 more tile in each direction. We need that
    // to be 256 pxl at target_level_. So we go deep to 3 (11-8) more levels (or
    // less) here. Going to MaxClientLevel, rather was causing the extra tile
    // not add any extra tile at target_level_. Also we don't have a reason
    // to go to such finer grid as MaxClientLevel (24) when the target_level_
    // is lower.
    const unsigned int level_where_oversize_is_a_tile = target_level_ +
        tilespace_.tileSizeLog2 - tilespace_.pixelsAtLevel0Log2;
    for (int level = std::min(MaxClientLevel, level_where_oversize_is_a_tile);
         level >= 0; --level) {
      khLevelCoverage tmpCov =
        khLevelCoverage::FromNormExtentsWithCrop(tilespace_,
                                                 normExtents,
                                                 level, level);
      if (tmpCov.NumTiles() > maxTotal) {
        continue;
      } else if (tmpCov.NumTiles() < minTotal) {
        if (coverage_.extents.empty()) {
          coverage_ = tmpCov;
          notify(NFY_DEBUG, "Chose level %u: %Zu (only one) (target %Zu)",
                 coverage_.level,
                 coverage_.NumTiles(),
                 targetTotal);
         } else {
          notify(NFY_DEBUG,
                 "Chose level %u: %Zu over %Zu (too small) (target %Zu)",
                 coverage_.level,
                 coverage_.NumTiles(),
                 tmpCov.NumTiles(),
                 targetTotal);
        }
        break;
      } else if (tmpCov.NumTiles() > targetTotal) {
        notify(NFY_DEBUG, "saving level %u", tmpCov.level);
        coverage_ = tmpCov;
      } else {
        std::uint64_t myDiff = targetTotal - tmpCov.NumTiles();
        std::uint64_t prevDiff = coverage_.NumTiles() - targetTotal;
        if (myDiff < prevDiff) {
          notify(NFY_DEBUG, "Chose level %u: %Zu over %Zu (target %Zu)",
                 tmpCov.level,
                 tmpCov.NumTiles(),
                 coverage_.NumTiles(),
                 targetTotal);
          coverage_ = tmpCov;
        } else {
          notify(NFY_DEBUG, "Chose level %u: %Zu over %Zu (target %Zu)",
                 coverage_.level,
                 coverage_.NumTiles(),
                 tmpCov.NumTiles(),
                 targetTotal);
        }
        break;
      }
    }

    // Create presence mask for original build set for levels from
    // coverage_.level to kMaxLevelForBuildPresenceMask. It is created once -
    // only for original build sets when Finalize() is called from ReadFile().
    // Presence mask of original build set is updated in QuadExporter and in
    // gstLayer::ExportQuad() and used for optimization quadtree partitioning
    // to skip empty quads at early stage.

    // TODO: based on coverage of source data set we
    // can calculate MaxLevelForBuildPresenceMask. Need to verify will it be
    // useful. It should be more effective for city, buidings data set (but
    // actually they have simple polygons).

    // +1 - one beyond the valid level.
    khInsetCoverage covt(tilespace_, coverage_, coverage_.level,
                         (coverage_.level < kMaxLevelForBuildPresenceMask) ?
                         (kMaxLevelForBuildPresenceMask + 1) :
                         (coverage_.level + 1));

    presence_mask_ = TransferOwnership(
        new khPresenceMask(covt, true));  // true - fill all with present.

    // Set to "not present" all elements of beginLevel. The PresenceMask of
    // beginLevel will be initialized below during grid filling.
    presence_mask_->SetPresence(covt.beginLevel(), false);

    // Create coverage mask for original build set for levels from
    // coverage_.level to kMaxLevelForBuildPresenceMask. It is created once -
    // only for original build sets when Finalize() is called from ReadFile().
    coverage_mask_ = TransferOwnership(
        new khCoverageMask(covt, false));  // false - fill all with not covered.
  } else {
    // Create presence mask. It is used when we call Finalize() from
    // SplitCell().
    presence_mask_ = TransferOwnership(
        new khPresenceMask(khInsetCoverage(coverage_)));
  }

  // resize our grid to have the right number of buckets
  grid_.resize(coverage_.NumTiles());


  // iterate through the bbox index list, inserting every feature into the
  // cells that they intersect with
  for (BoxIndexList::const_iterator it = box_index_list_.begin();
       it != box_index_list_.end(); ++it) {
    const FeatureHandle& feature_handle = (*box_list_)[*it];

    // Note: we can check the bounding boxes of parts for
    // multi-polygon features instead of bounding box of feature. But in this
    // case we need to read feature from kvp-file. We check bounding boxes of
    // parts in clipper and it is effective.

    // get the coverage of this feature at this level
    const gstBBox& box = feature_handle.bounding_box;
    khExtents<double> normExtents(NSEWOrder, box.n, box.s, box.e, box.w);
    khLevelCoverage thisCov =
      khLevelCoverage::FromNormExtentsWithOversizeFactor
      (tilespace_, normExtents,
       coverage_.level, coverage_.level, oversize_factor_);

    // intersect this features coverage with the total coverage for the index
    khExtents<std::uint32_t> tiles =
      khExtents<std::uint32_t>::Intersection(coverage_.extents, thisCov.extents);

    // put this id into every grid cell that it touches
    {
      std::uint32_t row     = tiles.beginRow();
      std::uint32_t gridRow = row - coverage_.extents.beginRow();
      for (; row < tiles.endRow(); ++row, ++gridRow) {
        std::uint32_t col     = tiles.beginCol();
        std::uint32_t gridCol = col - coverage_.extents.beginCol();
        for (; col < tiles.endCol(); ++col, ++gridCol) {
          unsigned int pos = (gridRow * coverage_.extents.numCols()) + gridCol;

          // add it to the appropriate bucket
          grid_[pos].push_back(*it);

          // update the presence mask (cascading up)
          presence_mask_->SetPresenceCascade(
              khTileAddr(coverage_.level, row, col));
        }
      }
    }
  }
}


void gstGeoIndexImpl::Intersect(const gstBBox& bbox,
                                std::vector<int>* match_list,
                                std::vector<gstBBox>* index_boxes) {
  if (!bounding_box_.Valid())
    return;

  // this is our real cull box
  gstBBox ubox = gstBBox::Intersection(bbox, bounding_box_);
  if (!ubox.Valid())
    return;

  // convert to tile coverage at this level
  khExtents<double> normExtents(NSEWOrder, ubox.n, ubox.s, ubox.e, ubox.w);
  khLevelCoverage thisCov =
    khLevelCoverage::FromNormExtentsWithOversizeFactor(
        tilespace_, normExtents,
        coverage_.level, coverage_.level, oversize_factor_);

  // intersect this coverage with the total coverage for the index
  // since we intersected the geo extents this intersection is probably
  // redundant. But we'd hate to have float rouding cause us to
  // index off the end of an array somewhere. :-)
  khExtents<std::uint32_t> tiles =
    khExtents<std::uint32_t>::Intersection(coverage_.extents, thisCov.extents);

  // covert level-wide extents to be index wide extents
  tiles.makeRelativeTo(coverage_.extents.origin());

  // put all feature ids in a set so we don't get duplicates
  std::set<int> set;
  for (unsigned int row = tiles.beginRow(); row < tiles.endRow(); ++row) {
    for (unsigned int col = tiles.beginCol(); col < tiles.endCol(); ++col) {
      unsigned int pos = (row * coverage_.extents.numCols()) + col;
      for (FeatureBucketIterator it = grid_[pos].begin();
           it != grid_[pos].end(); ++it) {
        const FeatureHandle& feature_handle = (*box_list_)[*it];
        if (ubox.Intersect(feature_handle.bounding_box))
          set.insert(feature_handle.feature_id);
      }

      // caller wants the index grid boxes for debug display purposes
      if (index_boxes != NULL) {
        khExtents<double> norm_extents =
          khTileAddr(coverage_.level,
                     row + coverage_.extents.beginRow(),
                     col + coverage_.extents.beginCol())
          .normExtents(tilespace_);
        index_boxes->push_back(gstBBox(norm_extents.west(),
                                       norm_extents.east(),
                                       norm_extents.south(),
                                       norm_extents.north()));
      }
    }
  }

  // copy cell contents to supplied list
  std::copy(set.begin(), set.end(), std::back_inserter(*match_list));
}


void gstGeoIndexImpl::SelectAll(std::vector<int>* list) {
  list->reserve(list->size() + box_index_list_.size());
  const FeatureList& fh_list = *box_list_;
  for (BoxIndexList::const_iterator it = box_index_list_.begin();
       it != box_index_list_.end(); ++it) {
    list->push_back(fh_list[*it].feature_id);
  }
}


namespace {
// Assumption: tilespace is in pixel coordinate where as box is in normalized
// coordinate.
void ExpandBbox(gstBBox* box_in_norm, const double oversize_factor,
                const unsigned int level, const khTilespace& tilespace) {
  // Stretch in width, delta_width = width * oversizeFactor
  // Stretch in width in each dir = delta_width / 2.0
  const double expand_pixel = tilespace.tileSize * (oversize_factor / 2.0);
  const double num_pixel_world =
      static_cast<double>(tilespace.pixelsAtLevel0 << level);
  const double expand_norm = expand_pixel / num_pixel_world;
  box_in_norm->ExpandBy(expand_norm);
}
}  // end namespace


bool gstGeoIndexImpl::ReadFile(const std::string& path, int source_id) {
  try {
    ThrowingReadFile(path, source_id);
    return true;
  } catch(const std::exception &e) {
    notify(NFY_WARN, "%s", e.what());
  } catch(...) {
    notify(NFY_WARN, "Unknown error reading %s", path.c_str());
  }
  return false;
}


void gstGeoIndexImpl::ThrowingReadFile(const std::string& path,
                                       int source_id) {
  assert(box_list_);

  FILE* select_fp = ::fopen(path.c_str(), "r");
  if (select_fp == NULL) {
    throw khErrnoException(kh::tr("Unable to open query results file %1")
                           .arg(path.c_str()));
  }
  khFILECloser closer(select_fp);

  // read old style file and upgrade it if necessary
  double xmin, xmax, ymin, ymax;
  if (fscanf(select_fp, "EXTENTS: %lf, %lf, %lf, %lf\n",
             &xmin, &xmax, &ymin, &ymax) == 4) {
    bounding_box_.init(xmin, xmax, ymin, ymax);
  }

  // The bounding box of a set of features, is used to decide whether there is
  // any feature on a tile. Since a feature may not be on a tile, but its
  // plotting (like label, icon etc.) may be on a tile, we expand each dimension
  // of bounding box by oversize_factor_ to get a bigger bounding box to decide
  // whether a set of feature has any intersection with a tile.
  ExpandBbox(&bounding_box_, oversize_factor_, target_level_, tilespace_);

  // TODO: use counter for number of features in box_list_.
  int val;
  while (!feof(select_fp)) {
    fscanf(select_fp, "%d\n", &val);
    gstBBox box = theSourceManager->GetFeatureBoxOrThrow(
        UniqueFeatureId(source_id, 0, val));
    if (!box.Valid()) {
      throw khException(
          kh::tr("Invalid bounding box for selected feature %1")
          .arg(val));
    }

    box_list_->push_back(FeatureHandle(val, box));
    box_index_list_.push_back(box_list_->size() - 1);
    assert(box_list_->size() == box_index_list_.size());
  }

  Finalize();
}


bool gstGeoIndexImpl::WriteFile(const std::string& path) {
  // silently remove file if it already exists
  if (khExists(path)) {
    if (unlink(path.c_str()) == -1) {
      notify(NFY_WARN, "Unable to remove select results file %s", path.c_str());
      return false;
    }
  }

  FILE* selectfp = fopen(path.c_str(), "w");
  if (selectfp == NULL) {
    notify(NFY_WARN, "Unable to create select results file %s", path.c_str());
    return false;
  }


  // this is a simple text file with every feature id on it's own line

  fprintf(selectfp, "EXTENTS: %.20lf, %.20lf, %.20lf, %.20lf\n",
          bounding_box_.w, bounding_box_.e, bounding_box_.s, bounding_box_.n);

  for (BoxIndexList::const_iterator it = box_index_list_.begin();
       it != box_index_list_.end(); ++it) {
    fprintf(selectfp, "%d\n", (*box_list_)[*it].feature_id);
  }

  fclose(selectfp);
  return true;
}

namespace {

const unsigned int SplitStepSize = 3;

}

gstGeoIndexHandle gstGeoIndexImpl::SplitCell(std::uint32_t row, std::uint32_t col,
                                             const khLevelCoverage &targetCov) {
  assert(coverage_.level < targetCov.level);
  assert(coverage_.extents.ContainsRowCol(row, col));

  // choose level to split to. Try to go SplitStepSize levels. But stop
  // earlier if we hit the target level
  unsigned int splitLevel = 0;
  unsigned int levelDiff = targetCov.level - coverage_.level;
  if (levelDiff <= SplitStepSize) {
    // We're close to the target level. Split on the target level
    splitLevel = targetCov.level;
  } else {
    splitLevel = coverage_.level + SplitStepSize;
  }

  // figure out this tiles coverage at the split level
  khLevelCoverage mySplitCov =
    khTileAddr(coverage_.level, row, col).MagnifiedToLevel(splitLevel);

  // figure out the target coverage at the split level
  khLevelCoverage targetSplitCov = targetCov.MinifiedToLevel(splitLevel);

  // intersect the two
  khExtents<std::uint32_t> splitExtents =
      khExtents<std::uint32_t>::Intersection(mySplitCov.extents,
                                      targetSplitCov.extents);

  // make a new index supplying the coverage we want
  gstGeoIndexHandle newIndex =
    khRefGuardFromNew(new gstGeoIndexImpl(khLevelCoverage(splitLevel,
                                                          splitExtents),
                                          tilespace_, oversize_factor_));
  // initialize box list handle.
  newIndex->box_list_ = box_list_;

  // walk my grid cell and add all the FeatureHandles into the new index
  std::uint32_t gridRow = row - coverage_.extents.beginRow();
  std::uint32_t gridCol = col - coverage_.extents.beginCol();
  unsigned int pos = (gridRow * coverage_.extents.numCols()) + gridCol;
  for (FeatureBucketIterator it = grid_[pos].begin();
       it != grid_[pos].end(); ++it) {
    newIndex->InsertIndex(*it);
  }

  // finalize the new index (assigns features into new grid & populates
  // presence_mask_)
  newIndex->Finalize();

  return newIndex;
}


const gstGeoIndexImpl::FeatureBucket*
gstGeoIndexImpl::GetBucket(std::uint32_t row, std::uint32_t col) const {
  std::uint32_t gridRow = row - coverage_.extents.beginRow();
  std::uint32_t gridCol = col - coverage_.extents.beginCol();
  unsigned int pos = (gridRow * coverage_.extents.numCols()) + gridCol;
  return &grid_[pos];
}

void gstGeoIndexImpl::PopulateBucket(const khExtents<std::uint32_t> &extents,
                                     FeatureBucket *bucket) const {
  std::set<int> set;

  std::uint32_t row     = extents.beginRow();
  std::uint32_t gridRow = row - coverage_.extents.beginRow();
  for (; row < extents.endRow(); ++row, ++gridRow) {
    std::uint32_t col     = extents.beginCol();
    std::uint32_t gridCol = col - coverage_.extents.beginCol();
    for (; col < extents.endCol(); ++col, ++gridCol) {
      unsigned int pos = (gridRow * coverage_.extents.numCols()) + gridCol;
      for (FeatureBucketConstIterator it = grid_[pos].begin();
           it != grid_[pos].end(); ++it) {
        set.insert(*it);
      }
    }
  }
  std::copy(set.begin(), set.end(), back_inserter(*bucket));
}

void gstGeoIndexImpl::GetFeatureIdsFromBucket(std::uint32_t row, std::uint32_t col,
                                              std::vector<int> &ids) const {
  // find the desired bucket
  std::uint32_t gridRow = row - coverage_.extents.beginRow();
  std::uint32_t gridCol = col - coverage_.extents.beginCol();
  unsigned int pos = (gridRow * coverage_.extents.numCols()) + gridCol;
  const gstGeoIndexImpl::FeatureBucket &bucket = grid_[pos];

  // copy the ids out of the bucket and into the vector
  ids.reserve(bucket.size());
  for (FeatureBucketConstIterator it = bucket.begin();
       it != bucket.end(); ++it) {
    ids.push_back((*box_list_)[*it].feature_id);
  }
}

void gstGeoIndexImpl::GetFeatureIdsFromBuckets(const khExtents<std::uint32_t> &extents,
                                               std::vector<int> &ids) const {
  std::set<int> set;

  std::uint32_t row     = extents.beginRow();
  std::uint32_t gridRow = row - coverage_.extents.beginRow();
  std::uint32_t count = 0;
  for (; row < extents.endRow(); ++row, ++gridRow) {
    std::uint32_t col     = extents.beginCol();
    std::uint32_t gridCol = col - coverage_.extents.beginCol();
    for (; col < extents.endCol(); ++col, ++gridCol) {
      unsigned int pos = (gridRow * coverage_.extents.numCols()) + gridCol;
      count += grid_[pos].size();
      for (FeatureBucketConstIterator it = grid_[pos].begin();
           it != grid_[pos].end(); ++it) {
        set.insert((*box_list_)[*it].feature_id);
      }
    }
  }
  ids.reserve(count);
  std::copy(set.begin(), set.end(), back_inserter(ids));
}

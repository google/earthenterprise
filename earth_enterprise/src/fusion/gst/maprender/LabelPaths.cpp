// Copyright 2017 Google Inc.
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


#include "fusion/gst/maprender/LabelPaths.h"

#include <cmath>
#include <limits>

#include <SkPathMeasure.h>
#include <SkBitmap.h>
#include <SkImageDecoder.h>

#include "fusion/gst/maprender/TextRenderer.h"

namespace {

// CheckBoundingBox - return true if label bounding box does not overlap any
// previous label bounding boxes in feature_labels.
// Even if the label is not completely inside a tile it is OK to render it.
bool CheckBoundingBox(const SkRect &label_bounds,
                       const std::vector<SkRect> &bounds) {
  for (std::vector<SkRect>::const_iterator feature_bounds = bounds.begin();
       feature_bounds != bounds.end();
       ++feature_bounds) {
    if (SkRect::Intersects(label_bounds, *feature_bounds))
      return false;
  }

  return true;
}

// CheckBoundingBox - return true if label bounding box does not overlap any
// previous label bounding boxes in feature_labels.
// Even if the label is not completely inside a tile it is OK to render it.
bool CheckBoundingBox(const SkRect &label_bounds,
                       const maprender::QStringToSkRectHashTable &bounds) {
  for (maprender::QStringToSkRectHashTable::const_iterator it =
           bounds.begin(); it != bounds.end(); ++it) {
    if (SkRect::Intersects(label_bounds, it->second)) {
      return false;
    }
  }

  return true;
}

// CheckBoundingBox - return true if label bounding box does not overlap
// previous label bounding boxes with identical text in feature_labels.
// Even if the label is not completely inside a tile it is OK to render it.
bool CheckBoundingBoxWithIdenticalLabel(
    const QString &text,
    const SkRect &label_bounds,
    const maprender::QStringToSkRectHashTable &bounds) {
  std::pair<maprender::QStringToSkRectHashTable::const_iterator,
            maprender::QStringToSkRectHashTable::const_iterator> range =
      bounds.equal_range(text);

  if (range.first != bounds.end()) {
    // Check if there is an intersection with bounding boxes of labels having
    // the same text.
    for (maprender::QStringToSkRectHashTable::const_iterator it =
             range.first; it != bounds.end(); ++it) {
      if (SkRect::Intersects(label_bounds, it->second)) {
        return false;
      }
    }
  }

  return true;
}

}  // namespace.


namespace maprender {

// Default values for parameter set

// TODO: make parameters settable.
const float LabelPaths::kDefault_smooth_angle_limit = 20;  // 15.0;
const float LabelPaths::kDefault_label_spacing_goal = 2.0;
const float LabelPaths::kDefault_shield_spacing_goal = 5.0;


// Amount to adjust text path length to avoid truncation (> 1)

const float LabelPaths::kTextLengthAdjustment = 1.025;

// LabelPoints and ReversedLabelPoints - classes to access points and
// distances along path in forward or reverse order.  Note that these
// classes do not own the referenced vectors.

class LabelPoints {
 public:
  LabelPoints(const std::vector<SkPoint> &points,
              const std::vector<SkScalar> &point_distance)
      : points_(points),
        point_distance_(point_distance) {
  }
  virtual ~LabelPoints() {}

  inline size_t size() const {
    assert(points_.size() == point_distance_.size());
    return points_.size();
  }
  virtual const SkPoint &point(size_t i) const {
    return points_.at(i);
  }
  virtual SkScalar distance(size_t i) const {
    return point_distance_.at(i);
  }

 protected:
  const std::vector<SkPoint> &points_;
  const std::vector<SkScalar> &point_distance_;
};

class ReversedLabelPoints : public LabelPoints {
 public:
  ReversedLabelPoints(const std::vector<SkPoint> &points,
                      const std::vector<SkScalar> &point_distance)
      : LabelPoints(points, point_distance) {
  }
  explicit ReversedLabelPoints(const LabelPoints label_path)
      : LabelPoints(label_path) {
  }
  virtual ~ReversedLabelPoints() {}

  virtual const SkPoint &point(size_t i) const {
    return points_.at(size() - i - 1);
  }
  virtual SkScalar distance(size_t i) const {
    return point_distance_.back() - point_distance_.at(size() - i - 1);
  }
};

// AddPathLabels - generate labels for path

void LabelPaths::AddPathLabels(const MapFeatureConfig &feature_config,
                               const QString &label_text,
                               const RenderPath &path,
                               std::vector<FeatureLabel> &feature_labels,
                               std::vector<SkRect> &bounds) {
  // Get size of text
  TextRenderer text_renderer(feature_config.label.textStyle, false, false);
  LabelTextInfo text_info(label_text, path);
  text_info.length = text_renderer.MeasureText(label_text,
                                               &text_info.above,
                                               &text_info.below);
  text_info.length *= kTextLengthAdjustment;  // add a little slack

  // Offset text baseline to center text on path
  text_info.vert_offset = -0.333 * text_info.above;
  text_info.below += text_info.vert_offset;
  text_info.above += text_info.vert_offset;

  // Get the points on the path
  size_t point_count = path->getPoints(NULL, 0);
  if (point_count < 2)
    return;                             // not a path
  points_.resize(point_count);
  point_distance_.resize(point_count);
#ifndef NDEBUG
  size_t retrieved_count = path->getPoints(&points_[0], point_count);
  assert(retrieved_count == point_count);
#else
  path->getPoints(&points_[0], point_count);
#endif

  SubdividePath(text_info, feature_labels, bounds);
}

// AddPathShields - generate shields for path

void LabelPaths:: AddPathShields(const MapFeatureConfig &feature_config,
                                 const QString &shield_text,
                                 const RenderPath &path,
                                 std::vector<FeatureShield> &feature_shields,
                                 std::vector<SkRect> &bounds) {
  // Get size of text
  TextRenderer text_renderer(feature_config.shield.textStyle, true, false);
  LabelTextInfo text_info(shield_text, path);
  text_info.length = text_renderer.MeasureText(shield_text,
                                               &text_info.above,
                                               &text_info.below);
  if (text_info.length == 0.0)
    return;                             // no text

  // Check if it is a bitmapped shield style:
  bool bitmap_shields = feature_config.shield.style_ ==
      MapShieldConfig::IconStyle;

  // Try to load the bitmap:
  SkBitmap icon;
  if (bitmap_shields) {
    IconReference ref(feature_config.shield.icon_type_,
                      feature_config.shield.icon_href_.c_str());
    bitmap_shields = SkImageDecoder::DecodeFile(ref.SourcePath().c_str(),
                                                &icon);
  }

  if (!bitmap_shields) {
    // We keep the "little slack" for backward compatibility:
    text_info.length *= kTextLengthAdjustment;  // add a little slack
  }

  // Build bounding rectangle relative to center of shield (vertical
  // center computation ignores descenders represented by "below")
  SkScalar outline_thickness = feature_config.shield.textStyle.outlineThickness;

  // We need to distinguish between the shield bounds (the shield includes
  // the icon and the text) and the icon bounds (only the icon, this can be
  // different from the shield bounds in case of fixed size images that allow
  // for text outside the icon area):
  SkRect relative_shield_bounds;
  SkRect relative_icon_bounds;

  const double text_height =
    fabs(text_info.above - text_info.below - 2*outline_thickness);
  const double text_width =
    fabs(text_info.length + 2*outline_thickness);

  const bool fixed_aspect_ratio = bitmap_shields &&
    (feature_config.shield.scaling_ == MapShieldConfig::IconFixedSizeStyle ||
     feature_config.shield.scaling_ == MapShieldConfig::IconFixedAspectStyle);

  const bool fixed_size_image = bitmap_shields &&
     feature_config.shield.scaling_ == MapShieldConfig::IconFixedSizeStyle;

  if (fixed_size_image) {
      // For fixed size images:
      // Adjust shield height so that the text fits inside the shield, and then
      // choose shield width so that the aspect ratio is maintained.
      // Note that the text may be outside the shield horizontally.

      const double scalex = text_width/(icon.width()
                     - feature_config.shield.left_margin_
                     - feature_config.shield.right_margin_);
      const double scaley = text_height/(icon.height()/3.0
                           - feature_config.shield.top_margin_
                           - feature_config.shield.bottom_margin_);
      relative_icon_bounds.fTop = 0.5 * text_info.above - outline_thickness
        - feature_config.shield.top_margin_ * scaley;
      relative_icon_bounds.fBottom = -0.5 * text_info.above + text_info.below
        + outline_thickness + feature_config.shield.bottom_margin_ * scaley;
      relative_icon_bounds.fLeft = -0.5 * icon.width() * scaley
        - feature_config.shield.left_margin_ * scaley/scalex;
      relative_icon_bounds.fRight = 0.5 * icon.width() * scaley
        + feature_config.shield.right_margin_ * scaley/scalex;

      relative_shield_bounds.fTop = relative_icon_bounds.fTop;
      relative_shield_bounds.fBottom = relative_icon_bounds.fBottom;
      relative_shield_bounds.fLeft = std::min(
         -0.5 * text_info.length - outline_thickness,
         static_cast<double>(relative_icon_bounds.fLeft));
      relative_shield_bounds.fRight = std::max(
         0.5 * text_info.length + outline_thickness,
         static_cast<double>(relative_icon_bounds.fRight));

  } else if (fixed_aspect_ratio) {
      // Fixed aspect: Do whatever it takes to make the text fit inside
      // the shield, w/o changing the aspect ratio of the image.

      const double scalex = text_width/(icon.width() -
                                        feature_config.shield.left_margin_ -
                                        feature_config.shield.right_margin_);
      const double scaley = text_height/(icon.height()/3 -
                                         feature_config.shield.top_margin_ -
                                         feature_config.shield.bottom_margin_);
      if (scalex > scaley) {
        relative_icon_bounds.fRight = 0.5 * text_info.length
          + outline_thickness + feature_config.shield.right_margin_ * scalex;
        relative_icon_bounds.fLeft = -0.5 * text_info.length
          - outline_thickness - feature_config.shield.left_margin_ * scalex;
        relative_icon_bounds.fTop =
          (0.5 * text_info.above - outline_thickness
           - feature_config.shield.top_margin_ * scaley) * scalex/scaley;
        relative_icon_bounds.fBottom = (-0.5 * text_info.above
          + text_info.below + outline_thickness
          + feature_config.shield.bottom_margin_ * scaley) * scalex/scaley;

      } else {
        relative_icon_bounds.fRight = (0.5 * text_info.length
          + outline_thickness
          + feature_config.shield.right_margin_ * scalex) * scaley/scalex;
        relative_icon_bounds.fLeft = (-0.5 * text_info.length
          - outline_thickness
          - feature_config.shield.left_margin_ * scalex) * scaley/scalex;
        relative_icon_bounds.fTop = 0.5 * text_info.above - outline_thickness
          - feature_config.shield.top_margin_ * scaley;
        relative_icon_bounds.fBottom = -0.5 * text_info.above
          + text_info.below + outline_thickness
          + feature_config.shield.bottom_margin_ * scaley;
      }
      relative_shield_bounds = relative_icon_bounds;

  } else {
    // variable aspect: freely place the shield as needed

    double scalex = 0.0, scaley = 0.0;
    if (bitmap_shields) {
      scalex = text_width/(icon.width() -
                           feature_config.shield.left_margin_ -
                           feature_config.shield.right_margin_);
      scaley = text_height/(icon.height() / 3 -
                            feature_config.shield.top_margin_ -
                            feature_config.shield.bottom_margin_);
    }

    relative_icon_bounds.fLeft = -0.5 * text_info.length - outline_thickness
      - feature_config.shield.left_margin_ * scalex;
    relative_icon_bounds.fRight = 0.5 * text_info.length + outline_thickness
      + feature_config.shield.right_margin_ * scalex;
    relative_icon_bounds.fTop = 0.5 * text_info.above - outline_thickness
      - feature_config.shield.top_margin_ * scaley;
    relative_icon_bounds.fBottom = -0.5 * text_info.above + text_info.below
      + outline_thickness + feature_config.shield.bottom_margin_ * scaley;
    relative_shield_bounds = relative_icon_bounds;
  }

  PlaceShields(text_info,
               relative_shield_bounds,
               relative_icon_bounds,
               feature_shields,
               bounds);
}

// SubdividePath - find relatively smooth segments of path, and label
// each segment.  Smoothness is defined as no angles > smooth_angle_limit.

void LabelPaths::SubdividePath(const LabelTextInfo &text_info,
                               std::vector<FeatureLabel> &feature_labels,
                               std::vector<SkRect> &bounds) {
  if (points_.size() < 2 || text_info.length == .0)
    return;                             // no path to label or no label.


  SkScalar label_spacing = area_bounds_.fRight / kDefault_label_spacing_goal;

  std::vector<std::pair<size_t, size_t> > smooth_subpaths;
  SkScalar subdivision_length;
  size_t subdivision_start = 0;
  SkVector last_segment_vector = points_.at(1) - points_.at(0);
  point_distance_.at(0) = 0.0;
  point_distance_.at(1) = last_segment_vector.length();
  last_segment_vector.normalize();

  // Collect smooth sub-paths
  for (size_t i = 2; i < points_.size(); ++i) {
    // Get vector and distance to next point
    SkVector segment_vector = points_.at(i) - points_.at(i - 1);
    SkScalar segment_length = segment_vector.length();
    point_distance_.at(i) = point_distance_.at(i - 1) + segment_length;
    segment_vector.normalize();

    // If angle to next segment is sharper than smoothness
    // threshold, label path so far and start a new subdivision
    SkScalar point_cosine =
      SkVector::DotProduct(last_segment_vector, segment_vector);

    subdivision_length = point_distance_.at(i - 1)
        - point_distance_.at(subdivision_start);

    if (segment_length > 0.0  &&
        point_cosine < smooth_angle_cosine_) {
      if (subdivision_length >= text_info.length) {
        // insert smooth sub-path into collection.
        smooth_subpaths.push_back(
            std::pair<size_t, size_t>(subdivision_start, i - 1));
      }

      // Start a new subdivision.
      subdivision_start = i - 1;
    }

    last_segment_vector = segment_vector;
  }

  // process last segment of path.
  SkVector segment_vector = points_.at(points_.size() - 1) -
      points_.at(subdivision_start);
  SkScalar segment_length = segment_vector.length();
  if (segment_length >= text_info.length)
    smooth_subpaths.push_back(
        std::pair<size_t, size_t>(subdivision_start, points_.size() - 1));

  // Placing labels.
  size_t last_label_subpath_idx = -1;
  if (smooth_subpaths.size()) {
    bool is_labeled = false;

    // last sub-path is labeled only if no one label were placed on path.
    // just to exlude two close located labels.
    for (size_t p = 0; p < smooth_subpaths.size() - 1; ++p) {
      if (last_label_subpath_idx == static_cast<size_t>(-1) ||
          (point_distance_.at(smooth_subpaths[p].first) -
           point_distance_.at(smooth_subpaths[last_label_subpath_idx].first) >
           label_spacing)) {
        if (is_labeled |=
            LabelSubdivision(text_info,
                             feature_labels,
                             bounds,
                             smooth_subpaths[p].first,
                             smooth_subpaths[p].second)) {
          last_label_subpath_idx = p;
        }
      }
    }

    // place at last sub-path if path is not labeled yet.
    if (!is_labeled)
      LabelSubdivision(text_info,
                       feature_labels,
                       bounds,
                       smooth_subpaths[smooth_subpaths.size() - 1].first,
                       smooth_subpaths[smooth_subpaths.size() - 1].second);
  }
}


// LabelSubdivision - apply a label to a subdivision of a path
bool LabelPaths::LabelSubdivision(const LabelTextInfo &text_info,
                                  std::vector<FeatureLabel> &feature_labels,
                                  std::vector<SkRect> &bounds,
                                  size_t subdivision_start,
                                  size_t subdivision_stop) {
  SkScalar subdivision_length = point_distance_.at(subdivision_stop)
                                - point_distance_.at(subdivision_start);

  // Don't label if label is longer than path
  if (subdivision_length < text_info.length)
    return false;

  // FeatureLabel will be added to feature_labels if label passes
  // bounding box tests

  FeatureLabel feature_label;
  feature_label.boundText = text_info.text;
  feature_label.horiz_offset = 0.5 * (subdivision_length - text_info.length);
  feature_label.vert_offset = text_info.vert_offset;

  // Select label direction along path to be as close as possible to
  // the correct upward orientation
  SkRect label_bounds;
  if (points_.at(subdivision_stop).fX >= points_.at(subdivision_start).fX) {
    // Draw label in original path direction
    BuildLabelSubPath(feature_label,
                      label_bounds,
                      text_info,
                      subdivision_start,
                      subdivision_stop,
                      LabelPoints(points_, point_distance_));
  } else {
    // Draw label in reverse of original path direction
    BuildLabelSubPath(feature_label,
                      label_bounds,
                      text_info,
                      points_.size() - subdivision_stop - 1,
                      points_.size() - subdivision_start - 1,
                      ReversedLabelPoints(points_, point_distance_));
  }

  // For shields it is important to have the enire shield in a tile, as
  // there is no good algorithm to partially draw shields in two neighboring
  // tiles. The absent shield will anyway be visible in either a different
  // location (completely inside the tile) or in next super tile
  // Please refer http://b/1972765
  // TODO: Update the algorithm so that the same drawable in two
  // neighboring tiles draw exactly at the same position, and remove the
  // restriction area_bounds_.contains(label_bounds), as this one may be
  // dropping drawables at the tile boundary.
  if (feature_label.path
      && area_bounds_.contains(label_bounds)
      && CheckBoundingBox(label_bounds, bounds)) {
    feature_labels.push_back(feature_label);
    bounds.push_back(label_bounds);
  } else {
    return false;   // label intersect tile boundary - not added.
  }

  return true;
}

// BuildLabelSubPath - build a sub-path for the label.  The direction
// of the sub-path may be reversed from the path depending on
// text_info.dir.  Also compute an axis-aligned bounding box for the
// label (eventually this should be replace with a tighter bounding
// box method).

void LabelPaths::BuildLabelSubPath(FeatureLabel &feature_label,
                                   SkRect &label_bounds,
                                   const LabelTextInfo &text_info,
                                   size_t subdivision_start,
                                   size_t subdivision_stop,
                                   const LabelPoints &label_points) {
  SkScalar offset = label_points.distance(subdivision_start) +
      feature_label.horiz_offset;  // current distance into path
  feature_label.horiz_offset = 0;
  SkScalar length = text_info.length;   // remaining text length

  // Find segment containing starting offset of label
  size_t label_segment = subdivision_start;
  while ( label_segment < subdivision_stop
          &&  offset >= label_points.distance(label_segment+1)) {
    ++label_segment;
  }

  // Compute arbitrarily aligned rectangle around each segment, and
  // add to aggregate bounding box.  Add baseline to label path.

  label_bounds.setEmpty();
  SkPoint label_path_end;
  label_path_end.set(0, 0);
  while (length > 0.0  &&  label_segment < subdivision_stop) {
    // Compute orientation unit vectors for the text in this segment
    SkVector segment_direction = label_points.point(label_segment+1)
                                 - label_points.point(label_segment);
    if (segment_direction.normalize()) {
      SkVector above_vector;
      above_vector.set(-segment_direction.fY * text_info.above,
                        segment_direction.fX * text_info.above);
      SkVector below_vector;
      below_vector.set(-segment_direction.fY * text_info.below,
                       segment_direction.fX * text_info.below);

      // Compute approximate text corners for left edge of text.  Note
      // that text may start part way into segment;

      SkScalar offset_in_segment =
        offset - label_points.distance(label_segment);
      SkPoint anchor;
      if (offset_in_segment > 0.0) {
        SkVector offset_vector;
        segment_direction.scale(offset_in_segment, &offset_vector);
        anchor = label_points.point(label_segment) + offset_vector;
      } else {
        anchor = label_points.point(label_segment);
      }
      SkPoint text_top_left = anchor + above_vector;
      SkPoint text_bottom_left = anchor + below_vector;

      if (feature_label.path) {
        feature_label.path->lineTo(anchor);
        ExtendRect(label_bounds, text_top_left);
      } else {
        feature_label.path = TransferOwnership(new SkPath);
        feature_label.path->moveTo(anchor);
        label_bounds.set(text_top_left.fX,
                         text_top_left.fY,
                         text_top_left.fX,
                         text_top_left.fY);
      }
      ExtendRect(label_bounds, text_bottom_left);

      // Compute text vector along segment, and use it to compute
      // right edge of text in this segment.

      SkScalar full_segment_length =
        label_points.distance(label_segment+1)
        - label_points.distance(label_segment);
      SkScalar segment_text_length =
        std::min(length, full_segment_length - offset_in_segment);
      SkVector segment_text_vector;
      segment_direction.scale(segment_text_length, &segment_text_vector);

      SkPoint text_top_right = text_top_left + segment_text_vector;
      SkPoint text_bottom_right = text_bottom_left + segment_text_vector;

      ExtendRect(label_bounds, text_top_right);
      ExtendRect(label_bounds, text_bottom_right);

      length -= segment_text_length;
      offset += segment_text_length;
      label_path_end = anchor + segment_text_vector;
    }
    ++label_segment;
  }
  if (feature_label.path)
    feature_label.path->lineTo(label_path_end);
}

// PlaceShields - place shields along a path without overlapping
// previously placed shield or label bounding boxes.

void LabelPaths::PlaceShields(const LabelTextInfo &text_info,
                              const SkRect &relative_shield_bounds,
                              const SkRect &relative_icon_bounds,
                              std::vector<FeatureShield> &feature_shields,
                              std::vector<SkRect> &bounds) {
  // Calculate desired spacing of shields along path
  if (parameter_set_.shield_spacing_goal < 1.0)
    return;                             // no shields
  SkScalar shield_spacing =
      area_bounds_.fRight / parameter_set_.shield_spacing_goal;
  assert(shield_spacing > 0.0);

  // Calculate offset of first shield
  SkPathMeasure path_measure(*text_info.path, false);
  SkScalar path_length = path_measure.getLength();

  SkScalar shield_offset = (shield_spacing <= path_length)
      ? 0.5 * shield_spacing
      : 0.5 * path_length;

  // To avoid copying points, push new item onto vector before filling
  {                                     // limit scope of new_shields
    FeatureShield new_shields;
    feature_shields.push_back(new_shields);
  }
  feature_shields.back().boundText = text_info.text;

  // Place shields along path
  while (shield_offset < path_length) {
    bool shield_drawn = false;
    SkPoint location;
    SkVector tangent;
    if (path_measure.getPosTan(shield_offset, &location, &tangent)) {
      SkRect shield_bounds = relative_shield_bounds;
      SkRect icon_bounds = relative_icon_bounds;
      shield_bounds.offset(location.fX, location.fY);
      icon_bounds.offset(location.fX, location.fY);
      location.fY -= 0.5 * text_info.above;
      // For shields it is important to have the enire shield in a tile, as
      // there is no good algorithm to partially draw shields in two neighboring
      // tiles. The absent shield will anyway be visible in either a different
      // location (completely inside the tile) or in next super tile
      if (area_bounds_.contains(shield_bounds) &&
          CheckBoundingBox(shield_bounds, bounds)) {
        feature_shields.back().points.push_back(location);
        feature_shields.back().bounds.push_back(shield_bounds);
        feature_shields.back().icon_bounds.push_back(icon_bounds);
        bounds.push_back(shield_bounds);
        shield_drawn = true;
      }
    }

    if (shield_drawn) {
      shield_offset += shield_spacing;
    } else {
      // Couldn't draw shield at desired location.  Try again at less
      // than full spacing.
      shield_offset += 0.25 * shield_spacing;
    }
  }

  if (feature_shields.back().points.empty()) {
    feature_shields.pop_back();
  }
}

// Extend rectangle to include point

void LabelPaths::ExtendRect(SkRect &rect, const SkPoint &point) {
  if (point.fX < rect.fLeft)
    rect.fLeft = point.fX;
  if (point.fX > rect.fRight)
    rect.fRight = point.fX;
  if (point.fY < rect.fTop)
    rect.fTop = point.fY;
  if (point.fY > rect.fBottom)
    rect.fBottom = point.fY;
}

void LabelPaths::GetBoundingBox(const MapSiteConfig& site_config,
                                const QString& label, SkScalar *const length,
                                SkScalar *const above, SkScalar *const below) {
  assert(length);
  assert(above);
  assert(below);
  maprender::TextRenderer text_renderer(
      site_config.label.textStyle, true, false);
  *length = text_renderer.MeasureText(label, above, below);
}

void LabelPaths::GetBoundingBox(const MapSiteConfig& site_config,
                                const maprender::SiteLabel& site_label,
                                SkRect *const label_bounds) {
  assert(label_bounds);
  // Initialize to empty polygon.
  *label_bounds = SkRect();

  SkScalar text_length, above, below;
  LabelPaths::GetBoundingBox(
      site_config, site_label.boundText, &text_length, &above, &below);

  if (text_length == 0.0) {
    return;  // no text.
  }
  text_length *= LabelPaths::kTextLengthAdjustment;  // add a little slack

  // Find bounding box.

  label_bounds->fRight = 0.5 * text_length;
  label_bounds->fLeft = -label_bounds->fRight;
  label_bounds->fTop = above;
  label_bounds->fBottom = below;
  const SkPoint &site_point = site_label.point;
  label_bounds->offset(site_point.fX, site_point.fY);
}

// CheckSiteVisibility - set "visible" tag in SiteLabel.

void LabelPaths::CheckSiteVisibility(const MapSiteConfig& site_config,
                                     SiteLabel *const site_label,
                                     QStringToSkRectHashTable *const bounds,
                                     bool check_identical) {
  assert(site_label);
  assert(bounds);

  // Calculate bounding box.

  SkRect label_bounds;
  GetBoundingBox(site_config, *site_label, &label_bounds);
  if (label_bounds.isEmpty()) {
    site_label->visible = false;         // no text
    return;
  }

  // If bounding box OK, mark label visible.

  if (check_identical ?
      CheckBoundingBoxWithIdenticalLabel(
          site_label->boundText, label_bounds, *bounds) :
      CheckBoundingBox(label_bounds, *bounds)) {
    (*bounds)[site_label->boundText] = label_bounds;
    site_label->visible = true;
  } else {
    site_label->visible = false;
  }
}

}  // namespace maprender

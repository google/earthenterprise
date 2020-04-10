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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_MAPRENDER_LABELPATHS_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_MAPRENDER_LABELPATHS_H_

#include <math.h>
#include <vector>
#include <string>
#include <tr1/functional>
#include <tr1/unordered_map>

#include <SkPath.h>
#include <common/base/macros.h>
#include "common/khRefCounter.h"
#include "fusion/gst/maprender/RenderTypes.h"

class QString;

namespace std {
namespace tr1 {

template<>
struct hash<QString> : public unary_function<QString, std::size_t> {
  std::size_t operator()(const QString &_val) const {
    QByteArray byte_array = _val.utf8();
    return hash<std::string>()(byte_array.data());
  }
};

}  // namespace tr1.
}  // namespace std.


namespace maprender {

// QString to SkRect hash table.
typedef std::tr1::unordered_map<QString, SkRect> QStringToSkRectHashTable;

class LabelPoints;

// LabelPaths - generates labels along a path or paths.
class LabelPaths {
 public:
  // ParameterSet - structure to contain set of tuning
  // parameters for labelling a path or paths.
  struct ParameterSet {
   public:
    ParameterSet()
        : smooth_angle_limit(kDefault_smooth_angle_limit),
          shield_spacing_goal(kDefault_shield_spacing_goal) {
    }
    ParameterSet(float smooth_angle_limit_,
                 float shield_spacing_goal_)
        : smooth_angle_limit(smooth_angle_limit_),
          shield_spacing_goal(shield_spacing_goal_) {
    }

    // smooth_angle_limit - maximum angle considered smooth (degrees)
    float smooth_angle_limit;

    // shield_spacing_goal - target number of shields for a path the
    // length of the tile image size (will be pro-rated by actual path
    // length)
    float shield_spacing_goal;
  };

  // Calculates bounding box of site label in terms of length, top, bottom.
  static void GetBoundingBox(const MapSiteConfig& site_config,
                             const QString& label,
                             SkScalar *const length,
                             SkScalar *const above,
                             SkScalar *const below);

  // Calculates bounding box of site label.
  static void GetBoundingBox(const MapSiteConfig& site_config,
                             const maprender::SiteLabel& site_label,
                             SkRect *label_bounds);

  // Checks site visibility by intersecting bounding box of site label with
  // bounding boxes of all already placed labels.
  // check_identical: whether to only check against labels with identical text.
  static void CheckSiteVisibility(const MapSiteConfig &site_config,
                                  SiteLabel *const site_label,
                                  QStringToSkRectHashTable *const bounds,
                                  bool check_identical);

  // Defaults for ParameterSet elements
  static const float kTextLengthAdjustment;
  static const float kDefault_smooth_angle_limit;
  static const float kDefault_label_spacing_goal;
  static const float kDefault_shield_spacing_goal;

  explicit LabelPaths(unsigned int tile_size) {
    area_bounds_.set(0, 0, tile_size, tile_size);
    UpdateCosine();
  }

  LabelPaths(unsigned int tile_size, const ParameterSet &params) {
    area_bounds_.set(0, 0, tile_size, tile_size);
    SetParameters(params);
  }

  void SetParameters(const ParameterSet &params) {
    parameter_set_ = params;
    UpdateCosine();
  }

  void AddPathLabels(const MapFeatureConfig &feature_config,
                     const QString &label_text,
                     const RenderPath &path,
                     std::vector<FeatureLabel> &feature_labels,
                     std::vector<SkRect> &bounds);
  void AddPathShields(const MapFeatureConfig &feature_config,
                      const QString &shield_text,
                      const RenderPath &path,
                      std::vector<FeatureShield> &feature_shields,
                      std::vector<SkRect> &bounds);

 private:
  struct LabelTextInfo {
    LabelTextInfo(const QString &text_, const RenderPath &path_)
        : text(text_),
          path(path_) {
    }
    const QString &text;
    const RenderPath &path;
    SkScalar length;
    SkScalar above;                     // adjusted by vert_offset
    SkScalar below;                     // adjusted by vert_offset
    SkScalar vert_offset;
  };

  void SubdividePath(const LabelTextInfo &text_info,
                     std::vector<FeatureLabel> &feature_labels,
                     std::vector<SkRect> &bounds);
  bool LabelSubdivision(const LabelTextInfo &text_info,
                        std::vector<FeatureLabel> &feature_labels,
                        std::vector<SkRect> &bounds,
                        size_t subdivision_start,
                        size_t subdivision_stop);
  void BuildLabelSubPath(FeatureLabel &flabel,
                         SkRect &label_bounds,
                         const LabelTextInfo &text_info,
                         size_t subdivision_start,
                         size_t subdivision_stop,
                         const LabelPoints &label_points);
  void PlaceShields(const LabelTextInfo &text_info,
                    const SkRect &shield_bounds,
                    const SkRect &icon_bounds,
                    std::vector<FeatureShield> &feature_shields,
                    std::vector<SkRect> &bounds);

  void ExtendRect(SkRect &rect, const SkPoint &point);


  void UpdateCosine() {
    smooth_angle_cosine_ =
      cos(parameter_set_.smooth_angle_limit / 180.0 * M_PI);
  }

  ParameterSet parameter_set_;

  // Temporary arrays for points and distance on path.  We define it
  // here so it won't need to be reallocated for every label.
  std::vector<SkPoint> points_;
  std::vector<SkScalar> point_distance_;  // distance from start of path.

  SkScalar smooth_angle_cosine_;

  SkRect area_bounds_;                  // drawing area bounds.
};

}  // namespace maprender

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_MAPRENDER_LABELPATHS_H_

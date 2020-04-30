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

#ifndef KHSRC_FUSION_GST_GSTFEATURE_H__
#define KHSRC_FUSION_GST_GSTFEATURE_H__

#include <gstRegistry.h>
#include <gstRecord.h>
#include <autoingest/.idl/storage/LayerConfig.h>
#include <khRefCounter.h>
#include <gstGeode.h>

#include <vector>

class gstFilter;
class JSDisplayBundle;

// ****************************************************************************
// ***  gstFeaturePreviewConfig
// ***
// ***  Holds the members with the styling information needed for previewing
// ***  in the fusion GUI.
// ****************************************************************************
class gstFeaturePreviewConfig {
 public:
  double line_width_;
  std::vector< unsigned int>  line_color_;
  std::vector< unsigned int>  poly_color_;
  VectorDefs::PolygonDrawMode polygon_draw_mode_;
  VectorDefs::FeatureDisplayType feature_display_type_;

  // used when building a gstFeature when you only have a FuseFeatureConfig
  // the values don't matter, since they will never be used
  gstFeaturePreviewConfig(void) :
      line_width_(1.0),
      line_color_(std::vector< unsigned int> (4, 255)),
      poly_color_(std::vector< unsigned int> (4, 255)),
      polygon_draw_mode_(VectorDefs::FillAndOutline),
      feature_display_type_(VectorDefs::PointZ)
  { }

  explicit gstFeaturePreviewConfig(const FeatureConfig &cfg) :
      line_width_(cfg.style.lineWidth),
      line_color_(cfg.style.lineColor),
      poly_color_(cfg.style.polyColor),
      // Note: polygons can be drawn as lines or polygons.
      // For lines, we want outline only.
      polygon_draw_mode_((cfg.featureType == VectorDefs::LineZ) ?
          VectorDefs::OutlineOnly : cfg.polygonDrawMode),
      feature_display_type_(cfg.featureType)
  { }


  void glLineColor(float *c) const {
    c[0] = static_cast<float>(line_color_[0]) / 255.0;
    c[1] = static_cast<float>(line_color_[1]) / 255.0;
    c[2] = static_cast<float>(line_color_[2]) / 255.0;
    c[3] = static_cast<float>(line_color_[3]) / 255.0;
  }
  void glPolyColor(float *c) const {
    c[0] = static_cast<float>(poly_color_[0]) / 255.0;
    c[1] = static_cast<float>(poly_color_[1]) / 255.0;
    c[2] = static_cast<float>(poly_color_[2]) / 255.0;
    c[3] = static_cast<float>(poly_color_[3]) / 255.0;
  }

};

class gstFeatureConfigs {
 public:
  gstFeatureConfigs(const FuseFeatureConfig& cfg,
             const gstFeaturePreviewConfig &preview_cfg);
  virtual ~gstFeatureConfigs();

  // some convenience wrappers around config
  QString label() const { return config.style.label.label; }
  bool reformatLabel() const { return config.style.label.reformat; }
  QString PrettyFeatureType() const;

  // the real workers
  bool AttributeExpansionNeeded() const;
  gstRecordHandle Expand(gstRecordHandle rec,
                         JSDisplayBundle &jsbundle,
                         unsigned int filterId) const;
  gstRecordHandle DummyExpand() const;

  // This is now const. No more setting here. No more config extraction either!
  const FuseFeatureConfig config;
  const gstFeaturePreviewConfig preview_config;

 private:
  mutable gstHeaderHandle feature_header_;
  mutable gstRecordFormatter* label_fmt_;
  mutable gstRecordFormatter* custom_height_fmt_;
};

class gstFeatureSet {
 public:
  gstFeatureSet() : feature_configs(0) {}

  GeodeList glist;
  // used by streets for name, used by all for custom height value
  RecordList rlist;
  const gstFeatureConfigs* feature_configs;
};

#endif  // !KHSRC_FUSION_GST_GSTFEATURE_H__

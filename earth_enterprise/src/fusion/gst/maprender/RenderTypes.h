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


#ifndef FUSION_GST_MAPRENDER_RENDERTYPES_H__
#define FUSION_GST_MAPRENDER_RENDERTYPES_H__

#include <SkPath.h>
#include <SkPoint.h>
#include <SkRect.h>
#include <vector>
#include <autoingest/.idl/storage/MapSubLayerConfig.h>

namespace maprender {

// ****************************************************************************
// ***  NOTE:
// ***  All SkPath objects are held out ref-counted pointers, they could be
// ***  big and will definitely be expensive to copy. This is the same thing
// ***  we do with gst's geodes
// ****************************************************************************
typedef khSharedHandle<SkPath> RenderPath;

class FeatureLabel {
 public:
  QString boundText;
  RenderPath path;
  SkScalar horiz_offset;
  SkScalar vert_offset;
};

class FeatureShield {
 public:
  QString boundText;
  std::vector<SkPoint> points;
  std::vector<SkRect> bounds;
  std::vector<SkRect> icon_bounds;
};

class Feature {
 public:
  const MapFeatureConfig &config;

  // no correlation between these three vectors
  // labels and shields are almost certainly longer than paths
  std::vector<RenderPath>    paths;
  std::vector<FeatureLabel>  labels;
  std::vector<FeatureShield> shields;

  inline Feature(const MapFeatureConfig &config_) : config(config_) { }
 private:
  DISALLOW_COPY_AND_ASSIGN(Feature);
};

class SiteLabel {
 public:
  QString boundText;
  QString boundTextOutline;
  SkPoint point;
  bool visible;

  SiteLabel(void) : visible(false) { }
  SiteLabel(const QString &text, const QString & outline_text, const SkPoint &p) :
      boundText(text), boundTextOutline(outline_text), point(p), visible(false) { }
  SiteLabel(const QString &text, const QString & outline_text, const SkPoint &p, bool visible_) :
      boundText(text), boundTextOutline(outline_text), point(p), visible(visible_) { }
};

class Site {
 public:
  const MapSiteConfig   &config;
  std::vector<SiteLabel> labels;

  inline Site(const MapSiteConfig &config_) : config(config_) { }
 private:
  DISALLOW_COPY_AND_ASSIGN(Site);
};

class DisplayRule {
 public:
  Feature  feature;
  Site     site;

  inline DisplayRule(const MapDisplayRuleConfig &config) :
      feature(config.feature),
      site(config.site)
  { }
 private:
  DISALLOW_COPY_AND_ASSIGN(DisplayRule);
};

class SubLayer {
 public:
  khDeletingVector<DisplayRule> displayRules;

  inline SubLayer(void) { }
 private:
  DISALLOW_COPY_AND_ASSIGN(SubLayer);
};


} // namespace maprender

#endif // FUSION_GST_MAPRENDER_RENDERTYPES_H__

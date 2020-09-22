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


#include "fusion/gst/maprender/Combiner.h"
#include "fusion/gst/maprender/LabelPaths.h"
#include "common/projection.h"

namespace maprender {

void GetTranslation(VectorDefs::EightSides dir,
                    const MapSiteConfig& site_config,
                    const SiteLabel& site_label,
                    const SkRect& marker_box,
                    SkPoint &point) {
  SkScalar length, above, below;
  LabelPaths::GetBoundingBox(site_config, site_label.boundTextOutline, &length,
                             &above, &below);
  // The box is not same in both above and below, so height_adjust
  const SkScalar height_adjust = -(above + below) / 2.0;
  // The estimates don't include space between letter and its box, so add 2 pxl
  const SkScalar half_length = length / 2.0 + 2.0;
  const SkScalar left_margin = -marker_box.fLeft + 1.0;  // 1.0 for spacing
  const SkScalar right_margin = marker_box.fRight + 1.0;
  const SkScalar top_margin = -marker_box.fTop + 1.0;
  const SkScalar bottom_margin = marker_box.fBottom + 1.0;
  // Note that in drawing coordinate y starts from top rather than bottom
  switch (dir) {
    case VectorDefs::TopRight:
      point.set(right_margin + half_length, -top_margin - below);
      break;
    case VectorDefs::Top:
      point.set(marker_box.centerX(), -top_margin - below);
      break;
    case VectorDefs::TopLeft:
      point.set(-left_margin - half_length, -top_margin - below);
      break;
    case VectorDefs::Left:
      point.set(-left_margin - half_length,
                marker_box.centerY() + height_adjust);
      break;
    case VectorDefs::BottomLeft:
      point.set(-left_margin - half_length, bottom_margin - above);
      break;
    case VectorDefs::Bottom:
      point.set(marker_box.centerX(), bottom_margin - above);
      break;
    case VectorDefs::BottomRight:
      point.set(right_margin + half_length, bottom_margin - above);
      break;
    case VectorDefs::Right:
      point.set(right_margin + half_length,
                marker_box.centerY() + height_adjust);
      break;
    default:
      point.set(0, 0);
      break;
  }
}


// ****************************************************************************
// ***  Combiner::TranslationContext
// ****************************************************************************
Combiner::TranslationContext::TranslationContext(const khTilespace &tilespace,
                                                 const QuadtreePath &path)
    : tilespace_(tilespace) {
  std::uint32_t row, col;
  path.GetLevelRowCol(&level_, &row, &col);
  // TODO: - MERCATOR - this has to change
  if (tilespace.projection_type == khTilespace::MERCATOR_PROJECTION) {
    origin_x_ = col * tilespace.tileSize;
    origin_y_ = row * tilespace.tileSize;
  } else {
    double normTileSize = tilespace.NormTileSize(level_);
    origin_x_ = col * normTileSize;
    // +1 because we are inverting the Y axis
    origin_y_ = (row + 1) * normTileSize;
    scale_x_ = normTileSize / tilespace.tileSize;
    scale_y_ = normTileSize / tilespace.tileSize;
  }
}

void Combiner::TranslationContext::TranslatePoint(
    SkPoint *point,
    const gstVertex &vertex) const {
  if (tilespace_.projection_type == khTilespace::MERCATOR_PROJECTION) {
    Projection::Point pt = tilespace_.projection->FromNormLatLngToPixel(
        Projection::LatLng(vertex.y, vertex.x), level_);
    point->fX = static_cast<double>(pt.X()) - origin_x_;
    // must invert y axis because skia wants upper-left
    point->fY = static_cast<float>(tilespace_.tileSize) -
        (static_cast<double>(pt.Y()) - origin_y_);
  } else {
    point->fX = static_cast<float>((vertex.x - origin_x_) / scale_x_);
    // origin_y - vertex.y because we are inverting the Y axis
    point->fY = static_cast<float>((origin_y_ - vertex.y) / scale_y_);
  }
}


// ****************************************************************************
// ***  Combiner
// ****************************************************************************
RenderPath Combiner::PathFromGeode(const TranslationContext &trans,
                                   const gstGeodeHandle &geodeh) {
  RenderPath path(TransferOwnership(new SkPath()));

  if (geodeh->PrimType() == gstMultiPolygon ||
      geodeh->PrimType() == gstMultiPolygon25D ||
      geodeh->PrimType() == gstMultiPolygon3D) {
    // TODO: Can we get multi-polygon features here?
    notify(NFY_WARN,
           "%s: multi-polygon features are not supported.", __func__);
  } else {
    const gstGeode *geode = static_cast<const gstGeode*>(&(*geodeh));
    assert(geode->VertexCount(0) > 0);

    for (unsigned int p = 0; p < geode->NumParts(); ++p) {
      SkPoint point;
      if (geode->VertexCount(p)) {
        // moveTo for first point of contour.
        trans.TranslatePoint(&point, geode->GetVertex(p, 0));
        path->moveTo(point);

        // lineTo for other points of contour.
        for (unsigned int v = 1; v < geode->VertexCount(p); ++v) {
          trans.TranslatePoint(&point, geode->GetVertex(p, v));
          path->lineTo(point);
        }
      }
    }
  }

  return path;
}

SkPoint
Combiner::PointFromVertex(const TranslationContext &trans,
                          const gstVertex &vertex) {
  SkPoint point;
  trans.TranslatePoint(&point, vertex);
  return point;
}


void
Combiner::ConvertFeatureGeometry(Feature& feature,
                                 const TranslationContext &trans,
                                 const InFeatureTile &in) {
  // convert the GeodeList to RenderPaths
  feature.paths.reserve(in.glist.size());
  for (unsigned int i = 0; i < in.glist.size(); ++i) {
    feature.paths.push_back(PathFromGeode(trans, in.glist[i]));
  }

  // Don't populate feature.labels or feature.shields that happens later
}

void
Combiner::ConvertSiteGeometry(Site& site,
                              const TranslationContext &trans,
                              const InSiteTile &in) {
  // for now convert all the sites, later we may decide to throw some away
  site.labels.reserve(in.vlist.size());
  for (unsigned int i = 0; i < in.vlist.size(); ++i) {
    site.labels.push_back(SiteLabel(
        in.rlist[i]->FieldByName("label")->ValueAsUnicode(),
        in.rlist[i]->FieldByName("outline_label")->ValueAsUnicode(),
                                    PointFromVertex(trans, in.vlist[i])));
  }
}


khTransferGuard<DisplayRule>
Combiner::ConvertDisplayRuleGeometry(const TranslationContext &trans,
                                     const InDisplayRuleTile &in) {
  khTransferGuard<DisplayRule> disprule =
    TransferOwnership(new DisplayRule(in.config));
  ConvertFeatureGeometry(disprule->feature, trans, in.feature);
  ConvertSiteGeometry(disprule->site, trans, in.site);
  return disprule;
}


khTransferGuard<SubLayer>
Combiner::ConvertSubLayerGeometry(const TranslationContext &trans,
                                  const InLayerTile &in) {
  khTransferGuard<SubLayer> sublayer = TransferOwnership(new SubLayer());
  sublayer->displayRules.reserve(in.displayRules.size());

  for (unsigned int d = 0; d < in.displayRules.size(); ++d) {
    sublayer->displayRules.push_back(
        ConvertDisplayRuleGeometry(trans, *in.displayRules[d]));
  }
  return sublayer;
}


bool
Combiner::Process(OutTile *out, const std::vector<const InTile*> &in) {
  // copy geometry from in to out (converting to local pixel coordinates)
  TranslationContext trans(tilespace, out->path);

  out->subLayers.reserve(in.size());
  for (unsigned int s = 0; s < in.size(); ++s) {
    out->subLayers.push_back(ConvertSubLayerGeometry(trans, *in[s]));
  }

  PlaceLabels(out, in);

  return true;
}


// ****************************************************************************
// ***  PlaceLabels
// ***
// ***  out has already been populated with Features and Sites
// ***
// ***  The Feature geometries are fully populated with local pixel
// ***  coordinates. No FeatureLabels or FeatureShields have been
// ***  instantiated.
// ***
// ***  Sites are all translated to local pixel coordinates. Maybe we need to
// ***  Throw some of them away if there are two many?
// ****************************************************************************
// Note: Since GEE-5.1.0, site labels (polygon center labels) and
// path labels (polygon outline labels, path labels and shields) are placed
// independently. We put the bounding boxes of the site labels and the bounding
// boxes of the path labels into separate containers. So, when placing the site
// labels, we only check for overlapping with already placed site labels, and
// when placing the path labels, we only check for overlapping with already
// placed path labels.
void
Combiner::PlaceLabels(OutTile *out, const std::vector<const InTile*> &in) {
  // Containers for bounds of placed labels to check for overlapping against
  // next placing one.
  // Container for bounds of site labels.
  QStringToSkRectHashTable site_label_bounds;
  // Container for bounds of path labels.
  std::vector<SkRect> path_label_bounds;
  unsigned int level = out->path.Level();

  // traverse each sublayer
  for (unsigned int s = 0; s < out->subLayers.size(); ++s) {
    SubLayer *subLayer = out->subLayers[s];
    const InTile   *inTile = in[s];

    // traverse each display rule in the sub layer
    for (unsigned int d = 0; d < subLayer->displayRules.size(); ++d) {
      const InDisplayRuleTile *inDisplayRule = inTile->displayRules[d];
      const InFeatureTile     &inFeature     = inDisplayRule->feature;
      const MapFeatureConfig &featureConfig = inDisplayRule->config.feature;
      const MapSiteConfig    &siteConfig    = inDisplayRule->config.site;
      Feature *feature = &subLayer->displayRules[d]->feature;
      Site *site = &subLayer->displayRules[d]->site;

      if (featureConfig.displayType == VectorDefs::IconZ) {
        // Since for icons we don't want to do complex visibility calculation,
        // always render it.
        for (unsigned int p = 0; p < site->labels.size(); ++p) {
          site->labels[p].visible = true;
        }
      } else if (featureConfig.displayType == VectorDefs::PolygonZ &&
                 siteConfig.label.displayAll) {
        // In "Display all"-mode for polygon center label, place all labels
        // except overlapping, identical ones.
        bool CHECK_IDENTICAL = true;
        for (unsigned int p = 0; p < site->labels.size(); ++p) {
          LabelPaths::CheckSiteVisibility(
              siteConfig, &(site->labels[p]), &site_label_bounds,
              CHECK_IDENTICAL);
        }
      } else {
        bool CHECK_IDENTICAL = false;
        for (unsigned int p = 0; p < site->labels.size(); ++p) {
          LabelPaths::CheckSiteVisibility(siteConfig,
                                          &(site->labels[p]),
                                          &site_label_bounds,
                                          CHECK_IDENTICAL);
        }
      }

      // traverse each path (piece of geometry) in the display rule
      for (unsigned int p = 0; p < feature->paths.size(); ++p) {
        gstRecordHandle boundRec = inFeature.rlist[p];
        if (boundRec) {
          // check if we want labels
          if (featureConfig.label.EnabledForLevel(level)) {
            gstValue *label = boundRec->FieldByName("label");
            assert(label);

            path_labeler_.AddPathLabels(featureConfig,
                                        label->ValueAsUnicode(),
                                        feature->paths[p],
                                        feature->labels,
                                        path_label_bounds);
          }

          // check if we want shields
          if (featureConfig.shield.EnabledForLevel(level)) {
            gstValue *shield = boundRec->FieldByName("shield");
            assert(shield);

            path_labeler_.AddPathShields(featureConfig,
                                         shield->ValueAsUnicode(),
                                         feature->paths[p],
                                         feature->shields,
                                         path_label_bounds);
          }
        }
      }
    }
  }
}



}  // namespace maprender

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


#include "fusion/gst/maprender/Renderer.h"

#include "fusion/gst/maprender/PointRenderer.h"
#include "fusion/gst/maprender/TextRenderer.h"
#include "SGLHelps.h"
#include <SkGraphics.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkBlurMaskFilter.h>
#include <SkImageDecoder.h>

namespace maprender {


// ****************************************************************************
// ***  RendererOutputTile
// ****************************************************************************
RendererOutputTile::RendererOutputTile(std::uint32_t rasterSize)
    : pixelBuf(TransferOwnership(new char[rasterSize*rasterSize*4])),
      bitmap(TransferOwnership(new SkBitmap())),
      canvas(), owns_buffer_(true) {
  bitmap->setConfig(SkBitmap::kARGB_8888_Config, rasterSize, rasterSize);
  bitmap->setPixels(&pixelBuf[0]);

  // delay creation of Canvas until after we have configured the Bitmap
  // too bad bitmap doesn't have a contructor that takes the config args
  // and the pixel buffer
  canvas = TransferOwnership(new SkCanvas(*bitmap));
}


// ****************************************************************************
// ***  RendererOutputTile
// ****************************************************************************
RendererOutputTile::RendererOutputTile(std::uint32_t rasterSize, char* buffer)
    : pixelBuf(TransferOwnership(buffer)),
      bitmap(TransferOwnership(new SkBitmap())),
      canvas(), owns_buffer_(false) {
  bitmap->setConfig(SkBitmap::kARGB_8888_Config, rasterSize, rasterSize);
  bitmap->setPixels(&pixelBuf[0]);

  // delay creation of Canvas until after we have configured the Bitmap
  // too bad bitmap doesn't have a contructor that takes the config args
  // and the pixel buffer
  canvas = TransferOwnership(new SkCanvas(*bitmap));
}


RendererOutputTile::~RendererOutputTile(void) {
  if (!owns_buffer_) {
    pixelBuf.take();  // so that we don't delete again
  }
}


// ****************************************************************************
// ***  Renderer
// ****************************************************************************
bool Renderer::Process(OutTile *out, const InTile &in) {
  bool is_empty = true;
  if (out->owns_buffer_) {
    // clear all channels
    if (debug) {
      out->bitmap->eraseARGB(128, 128, 128, 128);
    } else {
      // We find the SKIA eraseARGB is slow because of function calls in order
      // of canvas height. We rather use memset for fast erasing the canvas.
      // replacing out->bitmap->eraseARGB(0, 0, 0, 0);
      char * buff = &(*out->pixelBuf);
      memset(buff, 0, out->bitmap->width() * out->bitmap->height() * 4);
    }
  }

  // Draw all feature paths first

  for (unsigned int sub = 0; sub < in.subLayers.size(); ++sub) {
    const SubLayer* subLayer = in.subLayers[sub];
    for (unsigned int d = 0; d < subLayer->displayRules.size(); ++d) {
      const DisplayRule* displayRule = subLayer->displayRules[d];

      // Feature
      {
        const Feature& feature = displayRule->feature;
        for (unsigned int g = 0; g < feature.paths.size(); ++g) {
          switch (feature.config.displayType) {
            case VectorDefs::PointZ:
              // nothing to do - in fact we shouldn't ever get here
            case VectorDefs::IconZ:
              break;
            case VectorDefs::LineZ:
              if (RenderFeaturePath(*out->canvas, feature.config,
                                    feature.paths[g])) {
                is_empty = false;
              }
              break;
            case VectorDefs::PolygonZ:
              if (RenderFeaturePolygon(*out->canvas, feature.config,
                                       feature.paths[g])) {
                is_empty = false;
              }
              break;
          }
        }
      }
    }
  }

  // Render the labels after all the paths have been rendered.  This
  // prevents the paths from drawing on top of the labels.

  for (unsigned int sub = 0; sub < in.subLayers.size(); ++sub) {
    const SubLayer* subLayer = in.subLayers[sub];
    for (unsigned int d = 0; d < subLayer->displayRules.size(); ++d) {
      const DisplayRule* displayRule = subLayer->displayRules[d];

      // Feature
      {
        const Feature& feature = displayRule->feature;
        is_empty = is_empty && (feature.labels.size() == 0);
        for (unsigned int l = 0; l < feature.labels.size(); ++l) {
          RenderFeatureLabel(*out->canvas, feature.config.label,
                             feature.labels[l]);
        }
        is_empty = is_empty && (feature.shields.size() == 0);
        for (unsigned int s = 0; s < feature.shields.size(); ++s) {
          RenderFeatureShield(*out->canvas, feature.config.shield,
                              feature.shields[s]);
        }
      }

      // Site
      {
        const MapFeatureConfig& featureConfig = displayRule->feature.config;
        const Site& site = displayRule->site;
        // If point type is chosen we should draw label only if it is enabled.
        // Note that we have chosen to set site.label.enabled to true so that
        // we can get centroid points to draw point itself. So presence of
        // site.labels[] doesn't mean that we need to draw label.
        if (featureConfig.displayType == VectorDefs::IconZ) {
          const MapSiteConfig& siteConfig = site.config;
          PointRenderer pointRenderer(featureConfig);
          is_empty = is_empty && (site.labels.size() == 0);
          for (unsigned int l = 0; l < site.labels.size(); ++l) {
            SkPoint point = site.labels[l].point;
            if (featureConfig.isPointLabelEnabled) {  // center label enabled
              pointRenderer.SetCenterLabel(site.labels[l].boundText);
            }
            pointRenderer.Render(*out->canvas, point);
            if (siteConfig.label.hasOutlineLabel) {
              SkRect box;
              pointRenderer.GetBoundingBox(box);
              SkPoint labelPosition;
              GetTranslation(featureConfig.labelPositionRelativeToPoint,
                             siteConfig, site.labels[l],
                             box, labelPosition);
              labelPosition += point;
              RenderSiteLabel(*out->canvas, siteConfig.label.textStyle,
                              site.labels[l].boundTextOutline, labelPosition);
            }
          }
        } else {
          for (unsigned int l = 0; l < site.labels.size(); ++l) {
            if (site.labels[l].visible) {
              is_empty = false;
              RenderSiteLabel(*out->canvas, site.config.label.textStyle,
                              site.labels[l].boundText, site.labels[l].point);
            }
          }
        }
      }
    }
  }
  return !is_empty;
}

namespace {

void SetupPainterForLine(SkPaint &paint, const MapFeatureConfig &config) {
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeJoin(SkPaint::kRound_Join);
  paint.setColor(SkColorFromQColor(config.stroke_color));
  paint.setStrokeWidth(SkFloatToScalar(config.stroke_width));
  paint.setAntiAlias(true);
}

void SetupPainterForFill(SkPaint &paint, const MapFeatureConfig &config) {
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(SkColorFromQColor(config.fill_color));
  paint.setAntiAlias(true);
}

}  // anonymous namespace


bool Renderer::RenderFeaturePath(SkCanvas &canvas,
                                 const MapFeatureConfig &config,
                                 const RenderPath &path) {
  if (0 == config.stroke_width)
    return false;  // do not render zero-width paths.

  SkPaint paint;
  SetupPainterForLine(paint, config);

  canvas.drawPath(*path, paint);
  return true;
}

bool Renderer::RenderFeaturePolygon(SkCanvas &canvas,
                                    const MapFeatureConfig &config,
                                    const RenderPath &path) {
  bool is_rendered = false;

  // draw filled area first if requested
  if (config.polygonDrawMode == VectorDefs::FillAndOutline ||
      config.polygonDrawMode == VectorDefs::FillOnly) {
    SkPaint paint;
    SetupPainterForFill(paint, config);

    canvas.drawPath(*path, paint);
    is_rendered = true;
  }

#if 0
  // XXX NEED TO HANDLE EDGE FLAGS
  pakdata->numEdgeFlags = geode->VertexCount(0);

  const std::vector<bool>& flags = geode->EdgeFlags();
  pakdata->edgeFlags =
      static_cast<bool*>(mem_pool_.Allocate(pakdata->numEdgeFlags *
                                            sizeof(bool)));
  for (unsigned int i = 0; i < pakdata->numEdgeFlags; ++i) {
    pakdata->edgeFlags[i] = flags[i];
  }
#endif

  if (0 == config.stroke_width)
    return is_rendered;  // do not render zero-width outline.

  // draw outline if requested
  if (config.polygonDrawMode == VectorDefs::FillAndOutline ||
      config.polygonDrawMode == VectorDefs::OutlineOnly) {
    SkPaint paint;
    SetupPainterForLine(paint, config);
    canvas.drawPath(*path, paint);
    is_rendered = true;
  }

  return is_rendered;
}

void Renderer::RenderFeatureLabel(SkCanvas &canvas,
                                  const MapLabelConfig &config,
                                  const FeatureLabel& label) {
  TextRenderer textRenderer(config.textStyle, false, false);

  textRenderer.DrawTextOnPath(canvas,
                              label.boundText,
                              *label.path,
                              label.horiz_offset,
                              label.vert_offset);
}

void Renderer::RenderBox(SkCanvas& canvas,
                         const SkRect& box,
                         bool isRectBox,
                         const SkColor fillColor,
                         const SkColor outlineColor) {
  SkPaint fill_paint;
  fill_paint.setAntiAlias(true);
  fill_paint.setColor(fillColor);
  fill_paint.setStyle(SkPaint::kFill_Style);

  SkPaint outline_paint;
  outline_paint.setAntiAlias(true);
  outline_paint.setColor(outlineColor);
  outline_paint.setStyle(SkPaint::kStroke_Style);

  if (isRectBox) {
    canvas.drawRect(box, fill_paint);
    canvas.drawRect(box, outline_paint);
  } else {
    canvas.drawOval(box, fill_paint);
    canvas.drawOval(box, outline_paint);
  }
}


void Renderer::RenderFeatureShield(SkCanvas &canvas,
                                   const MapShieldConfig &config,
                                   const FeatureShield& shield) {
  TextRenderer textRenderer(config.textStyle, true, false);

  bool bitmap_shields = config.style_ == MapShieldConfig::IconStyle;

  SkBitmap icon;
  if (bitmap_shields) {
    // Try to read the icon file:
    IconReference ref(config.icon_type_, config.icon_href_.c_str());
    bitmap_shields =
        SkImageDecoder::DecodeFile(ref.SourcePath().c_str(), &icon);
  }

  for (unsigned int i = 0; i < shield.points.size(); ++i) {
    if (bitmap_shields) {
      RenderIcon(canvas, icon, shield.icon_bounds[i]);
    } else {
      const bool is_rect = config.style_ == MapShieldConfig::BoxStyle;
      const SkColor fill_color = SkColorFromQColor(config.fill_color_);
      const SkColor outline_color = SkColorFromQColor(config.box_color_);
      RenderBox(canvas, shield.icon_bounds[i], is_rect, fill_color,
                outline_color);
    }

    textRenderer.DrawText(canvas, shield.boundText,
                          shield.points[i].fX, shield.points[i].fY);
  }
}


void Renderer::RenderSiteLabel(SkCanvas &canvas,
                               const MapTextStyleConfig& textStyle,
                               const QString& boundText,
                               const SkPoint& point) {
  TextRenderer textRenderer(textStyle, true, false);

  textRenderer.DrawText(canvas, boundText,
                        point.fX, point.fY);
}




}  // namespace maprender

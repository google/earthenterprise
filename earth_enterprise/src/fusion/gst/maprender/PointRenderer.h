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

#ifndef  FUSION_GST_MAPRENDER_POINT_RENDERER_H__
#define  FUSION_GST_MAPRENDER_POINT_RENDERER_H__

#include "TextRenderer.h"
#include <autoingest/.idl/storage/MapSubLayerConfig.h>
#include <SkCanvas.h>

class QColor;

namespace maprender {

  // Draw the icon at the position specified by (bounding) box.
void RenderIcon(SkCanvas& canvas, const SkBitmap& icon, const SkRect& box);


class ShapeRenderer {
 public:
  SkRect rect_;
  SkPaint paint_;

  ShapeRenderer() {
    paint_.setAntiAlias(true);
  }

  void RenderTriangle(SkCanvas &canvas,
                      const SkScalar x, const SkScalar y) const;

  void RenderRectangle(SkCanvas &canvas,
                       const SkScalar x, const SkScalar y) const;

  void RenderOval(SkCanvas &canvas, const SkScalar x, const SkScalar y) const;

  void RenderCircle(SkCanvas &canvas,
                    const SkScalar x, const SkScalar y) const;
};


class OutlineShapeRenderer : public ShapeRenderer {
 public:
  void SetPaint(const QColor color, const SkScalar outlineWidth);
};


class FillShapeRenderer : public ShapeRenderer {
 public:
  void SetPaint(const QColor color);
};


class PointRendererInterface {
 public:
  virtual void ResizeToWrapTextBox(
      const SkRect& box, const MapShieldConfig::ShieldIconScaling scaling) = 0;
  virtual void Render(SkCanvas &canvas,
                      const SkScalar x, const SkScalar y) const = 0;
  virtual void GetBoundingBox(SkRect& box) const = 0;
  virtual ~PointRendererInterface() {};
};


class FilledAndOutlinedRenderer : public PointRendererInterface {
 protected:
  const SkScalar halfWidth_;
  const SkScalar height_;
  VectorDefs::PolygonDrawMode fillMode_;
  SkScalar halfOutlineWidth_;
  OutlineShapeRenderer outline_;
  FillShapeRenderer fill_;
  SkRect boundingBoxWhenOutline_;

  FilledAndOutlinedRenderer(SkScalar width, SkScalar height,
                            VectorDefs::PolygonDrawMode fillMode,
                            SkScalar outlineWidth,
                            QColor outlineColor, QColor fillColor);

  virtual ~FilledAndOutlinedRenderer();

  virtual void GetBoundingBox(SkRect& box) const;
};


class TriangleRenderer : public FilledAndOutlinedRenderer {
 public:
  static const SkScalar KsquareRootThree;
  static const SkScalar KsquareRootThreeDividedTwo;

  TriangleRenderer(const MapFeatureConfig& config);

  virtual ~TriangleRenderer();

  // Cannot assume the box to be symmetrical around 0,0
  virtual void ResizeToWrapTextBox(
      const SkRect& box, const MapShieldConfig::ShieldIconScaling scaling);

  virtual void Render(SkCanvas &canvas,
                      const SkScalar x, const SkScalar y) const;
 private:
  void InitFillOnly();
};


class RectangleRenderer : public FilledAndOutlinedRenderer {
 public:
  void InitFillOnly();

  RectangleRenderer(const MapFeatureConfig& config);

  virtual ~RectangleRenderer();

  // Cannot assume the box to be symmetrical around 0,0
  virtual void ResizeToWrapTextBox(
      const SkRect& textBox, const MapShieldConfig::ShieldIconScaling scaling);

  static void ResizeBox(const SkRect& textBox,
                        MapShieldConfig::ShieldIconScaling scaling,
                        SkScalar halfWidth, SkScalar height, SkRect& box);

  void AdjustBoundingRects(const SkRect& box);

  virtual void Render(SkCanvas &canvas,
                      const SkScalar x, const SkScalar y) const;
};


class OvalRenderer : public RectangleRenderer {
 public:
  static const SkScalar KsquareRootTwoMinusOneDividedTwo;

  OvalRenderer(const MapFeatureConfig& config) : RectangleRenderer(config) {}

  virtual ~OvalRenderer();

  // Cannot assume the box to be symmetrical around 0,0
  virtual void ResizeToWrapTextBox(
      const SkRect& textBox, const MapShieldConfig::ShieldIconScaling scaling);

  virtual void Render(SkCanvas &canvas,
                      const SkScalar x, const SkScalar y) const;
};


class IconRenderer : public PointRendererInterface {
 public:
  IconRenderer(const MapFeatureConfig& config);

  virtual ~IconRenderer();

  // Cannot assume the box to be symmetrical around 0,0
  virtual void ResizeToWrapTextBox(
      const SkRect& textBox, const MapShieldConfig::ShieldIconScaling scaling);

  virtual void Render(SkCanvas &canvas,
                      const SkScalar x, const SkScalar y) const;

  virtual void GetBoundingBox(SkRect& box) const;

 private:
  void FindIconImgBoundingBox();

  SkBitmap icon_;
  SkRect iconBox_;
  SkRect adjustedIconBox_;
};


class ShieldTextRenderer : public TextRenderer {
 public:
  // When resize_marker_to_wrap_label is true margins are used to add margin
  // between marker and text box. When it is false margins are used to shift the
  // text box center. Left margin => shift to right.
  ShieldTextRenderer(bool resize_marker_to_wrap_label,
                     const MapShieldConfig& config, const QString& text);

  const SkRect& GetBoundingBox() const { return box_; }

  void Render(SkCanvas& canvas, const SkPoint& point) const;

 private:
  float CalculateBoundingBox(const MapShieldConfig& config);

  const bool is_resize_marker_to_wrap_center_label_;
  const QString text_;
  float adjustTextY_;
  SkRect box_;
};


// Renders a point shield oval/rectangle/icon
class PointRenderer {
 public:
  PointRenderer(const MapFeatureConfig& config) :
      mapFeatureConfig_(config),
      pointRenderer_(CreatePointRenderer(config)),
      textRenderer_(NULL) {}

  void SetCenterLabel(const QString& centerLabel);

  static PointRendererInterface* CreatePointRenderer(
      const MapFeatureConfig& config);

  void Render(SkCanvas& canvas, const SkPoint& point) const;

  void GetBoundingBox(SkRect& box) const {
    pointRenderer_->GetBoundingBox(box);
  }

  ~PointRenderer();

private:
  const MapFeatureConfig& mapFeatureConfig_;
  PointRendererInterface* const pointRenderer_;
  const ShieldTextRenderer* textRenderer_;
};

}  // end namespace maprender

#endif  // FUSION_GST_MAPRENDER_POINT_RENDERER_H__

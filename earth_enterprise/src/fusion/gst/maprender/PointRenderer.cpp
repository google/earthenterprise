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


#include "PointRenderer.h"

#include "LabelPaths.h"
#include "SGLHelps.h"

#include <SkImageDecoder.h>

namespace maprender {

// Previously SkCanvas had an overloaded scale method doing ScaleWithPivot.
// Since it is no longer available adding that here. It scales so that
// the pivot point (pivot_x, pivot_y) remains unchanged.
void ScaleWithPivot(SkCanvas& canvas, SkScalar scale_x, SkScalar scale_y,
                    SkScalar pivot_x, SkScalar pivot_y) {
  canvas.translate(pivot_x, pivot_y);
  canvas.scale(scale_x, scale_y);
  canvas.translate(-pivot_x, -pivot_y);
}

void RenderIcon(SkCanvas& canvas, const SkBitmap& icon,
                const SkRect& box) {
  canvas.save();

  SkPaint paint;
  paint.setAntiAlias(true);

  // determine scaling factors to fit icon inside the icon bounds:
  double scalex= box.width() / icon.width();
  double scaley= box.height() / (icon.height()/3);

  // Clip the lower third of the image (fusion icons are stored as three
  // stacked images in one):
  SkRect clip;
  clip.set(box.fLeft, box.fTop, box.fRight, box.fBottom);
  canvas.clipRect(clip);

  // Stretch the bitmap so that it fills the entire bounding box:
  ScaleWithPivot(canvas, scalex, scaley, box.fLeft, box.fTop);

  // Move up the image to draw the lower third only.
  canvas.translate(0, -icon.height() * 2.0 /3.0);

  canvas.drawBitmap(icon, box.fLeft, box.fTop, &paint);
  canvas.restore();
}


void ShapeRenderer::RenderTriangle(SkCanvas &canvas,
                                   const SkScalar x, const SkScalar y) const {
  SkRect box = rect_;
  box.offset(x, y);
  SkPath path;
  path.moveTo(x, box.fTop);
  path.lineTo(box.fLeft, box.fBottom);
  path.lineTo(box.fRight, box.fBottom);
  path.lineTo(x, box.fTop);
  path.close();
  canvas.drawPath(path, paint_);
}

void ShapeRenderer::RenderRectangle(SkCanvas &canvas,
                                   const SkScalar x, const SkScalar y) const {
  SkRect box = rect_;
  box.offset(x, y);
  canvas.drawRect(box, paint_);
}

void ShapeRenderer::RenderOval(SkCanvas &canvas,
                               const SkScalar x, const SkScalar y) const {
  SkRect box = rect_;
  box.offset(x, y);
  canvas.drawOval(box, paint_);
}

void ShapeRenderer::RenderCircle(SkCanvas &canvas,
                                 const SkScalar x, const SkScalar y) const {
  canvas.drawCircle(x, y, rect_.fRight, paint_);
}

void OutlineShapeRenderer::SetPaint(const QColor color,
                                    const SkScalar outlineWidth) {
  paint_.setStyle(SkPaint::kStroke_Style);
  paint_.setColor(SkColorFromQColor(color));
  paint_.setStrokeWidth(outlineWidth);
}

void FillShapeRenderer::SetPaint(const QColor color) {
  paint_.setColor(SkColorFromQColor(color));
  paint_.setStyle(SkPaint::kFill_Style);
}

FilledAndOutlinedRenderer::FilledAndOutlinedRenderer(
                          SkScalar width, SkScalar height,
                          VectorDefs::PolygonDrawMode fillMode,
                          SkScalar outlineWidth,
                          QColor outlineColor, QColor fillColor) :
    halfWidth_(width / 2.0), height_(height), fillMode_(fillMode) {
  if (fillMode != VectorDefs::FillOnly) {
    halfOutlineWidth_ = outlineWidth / 2.0;
    outline_.SetPaint(outlineColor, outlineWidth);
  }
  if (fillMode_ != VectorDefs::OutlineOnly) {
    fill_.SetPaint(fillColor);
  }
}

FilledAndOutlinedRenderer::~FilledAndOutlinedRenderer() {}

void FilledAndOutlinedRenderer::GetBoundingBox(SkRect& box) const {
  box = fillMode_ == VectorDefs::FillOnly ? fill_.rect_
                                          : boundingBoxWhenOutline_;
}

void TriangleRenderer::InitFillOnly() {
  fill_.rect_.fRight = halfWidth_;
  fill_.rect_.fLeft = -fill_.rect_.fRight;
  fill_.rect_.fBottom = height_ / 3.0;
  fill_.rect_.fTop = fill_.rect_.fBottom - height_;
}

TriangleRenderer::TriangleRenderer(const MapFeatureConfig& config) :
    FilledAndOutlinedRenderer(config.pointWidth,
      (config.pointMarker != VectorDefs::EquilateralTriangle
       ? config.pointHeight : config.pointWidth * KsquareRootThreeDividedTwo),
                              config.polygonDrawMode, config.stroke_width,
                              config.stroke_color, config.fill_color) {
  if (!config.IsCenterLabelDependentIconBoundingBox()) {
    if (fillMode_ ==  VectorDefs::FillOnly) {
      InitFillOnly();
    } else {
      const float tan_theta = height_ / halfWidth_;
      const float cot_theta = halfWidth_ / height_;
      const float cosec_theta = sqrt(1 + cot_theta * cot_theta);
      const SkScalar delta_half_width_for_fill = config.stroke_width *
                                                 (cosec_theta + cot_theta);
      if (delta_half_width_for_fill >= halfWidth_) {
        fillMode_ = VectorDefs::FillOnly;
        fill_.SetPaint(config.stroke_color);
        InitFillOnly();
      } else {
        float delta_half_width_for_outline = delta_half_width_for_fill / 2.0;
        outline_.rect_.fRight = halfWidth_ - delta_half_width_for_outline;
        outline_.rect_.fLeft = -outline_.rect_.fRight;
        const SkScalar outline_height = outline_.rect_.fRight * tan_theta;
        outline_.rect_.fBottom = height_ / 3.0 - halfOutlineWidth_;
        outline_.rect_.fTop = outline_.rect_.fBottom - outline_height;
        if (outline_.rect_.fBottom <= outline_.rect_.fTop) {
          fillMode_ = VectorDefs::FillOnly;
          fill_.SetPaint(config.stroke_color);
          InitFillOnly();
        }
        boundingBoxWhenOutline_.fRight = halfWidth_;
        boundingBoxWhenOutline_.fLeft = -halfWidth_;
        boundingBoxWhenOutline_.fBottom = outline_.rect_.fBottom +
                                          halfOutlineWidth_;
        boundingBoxWhenOutline_.fTop = boundingBoxWhenOutline_.fBottom -
                                       height_;

        if (fillMode_ == VectorDefs::FillAndOutline) {
          fill_.rect_.fRight = halfWidth_ - delta_half_width_for_fill;
          fill_.rect_.fLeft = -fill_.rect_.fRight;
          const SkScalar fill_height = fill_.rect_.fRight * tan_theta;
          fill_.rect_.fBottom = outline_.rect_.fBottom - halfOutlineWidth_;
          fill_.rect_.fTop = fill_.rect_.fBottom - fill_height;
        }
      }
    }
  }
}

TriangleRenderer::~TriangleRenderer() {}

// Cannot assume the box to be symmetrical around 0,0
void TriangleRenderer::ResizeToWrapTextBox(
    const SkRect& box, const MapShieldConfig::ShieldIconScaling scaling) {
  const float tan_theta = scaling == MapShieldConfig::IconFixedAspectStyle ?
    height_ / halfWidth_ : KsquareRootThree;
  const float height = box.height();
  float delta_half_width = height / tan_theta;
  if (fillMode_ != VectorDefs::OutlineOnly) {
    fill_.rect_.fRight = box.fRight + delta_half_width;
    fill_.rect_.fLeft = box.fLeft - delta_half_width;
    const float fill_height = fill_.rect_.fRight * tan_theta;
    fill_.rect_.fBottom = box.fBottom;
    fill_.rect_.fTop = fill_.rect_.fBottom - fill_height;
    if (fillMode_ == VectorDefs::FillOnly) {
      return;
    }
  }
  const float cot_theta = 1.0 / tan_theta;
  const float cosec_theta = sqrt(1 + cot_theta * cot_theta);
  float delta_half_width_for_outline = halfOutlineWidth_ *
                                 (cosec_theta + cot_theta);
  outline_.rect_.fRight = box.fRight + delta_half_width +
                          delta_half_width_for_outline;
  outline_.rect_.fLeft = box.fLeft - delta_half_width -
                         delta_half_width_for_outline;
  const float outline_height = outline_.rect_.width() / 2.0 * tan_theta ;
  outline_.rect_.fBottom = box.fBottom + halfOutlineWidth_;
  outline_.rect_.fTop = outline_.rect_.fBottom - outline_height;
  boundingBoxWhenOutline_.fRight = outline_.rect_.fRight +
                                   delta_half_width_for_outline;
  boundingBoxWhenOutline_.fLeft = outline_.rect_.fLeft -
                                   delta_half_width_for_outline;
  boundingBoxWhenOutline_.fBottom = outline_.rect_.fBottom +
                                    halfOutlineWidth_;
  boundingBoxWhenOutline_.fTop = boundingBoxWhenOutline_.fBottom -
      boundingBoxWhenOutline_.width() / 2.0 * tan_theta;
}

void TriangleRenderer::Render(SkCanvas &canvas,
                              const SkScalar x, const SkScalar y) const {
  if (fillMode_ != VectorDefs::OutlineOnly) {
    fill_.RenderTriangle(canvas, x, y);
    if (fillMode_ == VectorDefs::FillOnly) {
      return;
    }
  }
  outline_.RenderTriangle(canvas, x, y);
}

SkScalar const TriangleRenderer::KsquareRootThree = SkFloatToScalar(sqrt(3.0));
SkScalar const TriangleRenderer::KsquareRootThreeDividedTwo =
  TriangleRenderer::KsquareRootThree / 2.0;


void RectangleRenderer::InitFillOnly() {
  fill_.rect_.fRight = halfWidth_;
  fill_.rect_.fLeft = -fill_.rect_.fRight;
  fill_.rect_.fBottom = height_ / 2.0;
  fill_.rect_.fTop = fill_.rect_.fBottom - height_;
}

RectangleRenderer::RectangleRenderer(const MapFeatureConfig& config) :
    FilledAndOutlinedRenderer(config.pointWidth,
                              ((config.pointMarker == VectorDefs::Square ||
                                config.pointMarker == VectorDefs::Circle) ?
                               config.pointWidth : config.pointHeight),
                              config.polygonDrawMode, config.stroke_width,
                              config.stroke_color, config.fill_color) {
  if (!config.IsCenterLabelDependentIconBoundingBox()) {
    if (fillMode_ ==  VectorDefs::FillOnly) {
      InitFillOnly();
    } else {
      if (config.stroke_width >= halfWidth_) {
        fillMode_ = VectorDefs::FillOnly;
        fill_.SetPaint(config.stroke_color);
        InitFillOnly();
      } else {
        outline_.rect_.fRight = halfWidth_ - config.stroke_width / 2.0;
        outline_.rect_.fLeft = -outline_.rect_.fRight;
        outline_.rect_.fBottom = height_ / 2.0 - halfOutlineWidth_;
        outline_.rect_.fTop = -outline_.rect_.fBottom;
        if (outline_.rect_.fBottom <= outline_.rect_.fTop) {
          fillMode_ = VectorDefs::FillOnly;
          fill_.SetPaint(config.stroke_color);
          InitFillOnly();
        }

        boundingBoxWhenOutline_ = outline_.rect_;
        boundingBoxWhenOutline_.inset(-halfOutlineWidth_, -halfOutlineWidth_);

        if (fillMode_ == VectorDefs::FillAndOutline) {
          fill_.rect_.fRight = halfWidth_ - config.stroke_width;
          fill_.rect_.fLeft = -fill_.rect_.fRight;
          fill_.rect_.fBottom = outline_.rect_.fBottom - halfOutlineWidth_;
          fill_.rect_.fTop = -fill_.rect_.fBottom;
        }
      }
    }
  }
}

RectangleRenderer::~RectangleRenderer() {}

// Cannot assume the box to be symmetrical around 0,0
void RectangleRenderer::ResizeToWrapTextBox(
    const SkRect& textBox, const MapShieldConfig::ShieldIconScaling scaling) {
  SkRect box;
  ResizeBox(textBox, scaling, halfWidth_, height_, box);
  AdjustBoundingRects(box);
}

void RectangleRenderer::ResizeBox(
    const SkRect& textBox, MapShieldConfig::ShieldIconScaling scaling,
    SkScalar halfWidth, SkScalar height, SkRect& box) {
  if (scaling == MapShieldConfig::IconFixedAspectStyle) {
    box = textBox;
    const float width_height_ratio_diff = textBox.width() / (halfWidth *2.0) -
                                          textBox.height() / height;
    if (width_height_ratio_diff > 0) {
      const float extra = width_height_ratio_diff * height / 2.0;
      box.fBottom += extra;
      box.fTop -= extra;
    } else {
      const float extra = -width_height_ratio_diff * halfWidth;
      box.fRight += extra;
      box.fLeft -= extra;
    }
  } else {
    box = textBox;
  }
}

void RectangleRenderer::AdjustBoundingRects(const SkRect& box) {
  if (fillMode_ != VectorDefs::OutlineOnly) {
    fill_.rect_ = box;
    if (fillMode_ == VectorDefs::FillOnly) {
      return;
    }
  }
  outline_.rect_.fRight = box.fRight + halfOutlineWidth_;
  outline_.rect_.fLeft = box.fLeft - halfOutlineWidth_;
  outline_.rect_.fBottom = box.fBottom + halfOutlineWidth_;
  outline_.rect_.fTop = box.fTop - halfOutlineWidth_;
  boundingBoxWhenOutline_ = outline_.rect_;
  boundingBoxWhenOutline_.inset(-halfOutlineWidth_, -halfOutlineWidth_);
}

void RectangleRenderer::Render(SkCanvas &canvas,
                               const SkScalar x, const SkScalar y) const {
  if (fillMode_ != VectorDefs::OutlineOnly) {
    fill_.RenderRectangle(canvas, x, y);
    if (fillMode_ == VectorDefs::FillOnly) {
      return;
    }
  }
  outline_.RenderRectangle(canvas, x, y);
}

OvalRenderer::~OvalRenderer() {}

// Cannot assume the box to be symmetrical around 0,0
void OvalRenderer::ResizeToWrapTextBox(
    const SkRect& textBox, const MapShieldConfig::ShieldIconScaling scaling) {
  SkScalar widthExtra, heightExtra;
  const SkScalar width = textBox.width();
  const SkScalar height = textBox.height();
  if (scaling == MapShieldConfig::IconFixedAspectStyle) {
    const float ratio = height_ / (halfWidth_* 2);
    const SkScalar oval_width = sqrt(width * width +
                                     height * height / (ratio * ratio));
    const SkScalar oval_height = oval_width * ratio;
    widthExtra = (oval_width - width) / 2.0;
    heightExtra = (oval_height - height) / 2.0;
  } else {
    // Equation of ellipse centered at 0, 0 is x^2 / a^2 + y^2 / b^2 = 1, and
    // it goes through (+-a,0) and (+-b,0). I decided to equally share the
    // projection beyond the original rectangle to both x and y axis and that
    // is why the choice of KsquareRootTwo
    widthExtra = width * KsquareRootTwoMinusOneDividedTwo;
    heightExtra = height * KsquareRootTwoMinusOneDividedTwo;
  }
  SkRect box = textBox;
  box.fRight += widthExtra;
  box.fLeft -= widthExtra;
  box.fBottom += heightExtra;
  box.fTop -= heightExtra;
  AdjustBoundingRects(box);
}

void OvalRenderer::Render(SkCanvas &canvas,
                          const SkScalar x, const SkScalar y) const {
  if (fillMode_ != VectorDefs::OutlineOnly) {
    fill_.RenderOval(canvas, x, y);
    if (fillMode_ == VectorDefs::FillOnly) {
      return;
    }
  }
  outline_.RenderOval(canvas, x, y);
}

const SkScalar OvalRenderer::KsquareRootTwoMinusOneDividedTwo =
  (SkFloatToScalar(sqrt(2.0)) - 1.0) / 2.0;


IconRenderer::IconRenderer(const MapFeatureConfig& config) {
  IconReference ref(config.shield.icon_type_, config.shield.icon_href_.c_str());
  bool found = SkImageDecoder::DecodeFile(ref.SourcePath().c_str(), &icon_);
  if (!found) {
    throw khException(kh::tr("Cannot read and/or decode icon file '%1'").arg(
                             ref.SourcePath().c_str()));
  }
  FindIconImgBoundingBox();
  if (!config.IsCenterLabelDependentIconBoundingBox()) {
    adjustedIconBox_ = iconBox_;
  }
}

IconRenderer::~IconRenderer() {}

  // Cannot assume the box to be symmetrical around 0,0
void IconRenderer::ResizeToWrapTextBox(
    const SkRect& textBox, const MapShieldConfig::ShieldIconScaling scaling) {
  RectangleRenderer::ResizeBox(textBox, scaling, iconBox_.fRight,
                               iconBox_.height(), adjustedIconBox_);
}

void IconRenderer::Render(SkCanvas &canvas,
                          const SkScalar x, const SkScalar y) const {
  SkRect box = adjustedIconBox_;
  box.offset(x, y);
  RenderIcon(canvas, icon_, box);
}

void IconRenderer::GetBoundingBox(SkRect& box) const {
  box = adjustedIconBox_;
}

void IconRenderer::FindIconImgBoundingBox() {
  iconBox_.fRight = icon_.width() / 2.0;
  // fusion icons are stored as three stacked images in one
  iconBox_.fBottom = (icon_.height() / 3.0) / 2.0;

  iconBox_.fLeft = -iconBox_.fRight;
  iconBox_.fTop = -iconBox_.fBottom;
}

ShieldTextRenderer::ShieldTextRenderer(bool resize_marker_to_wrap_label,
                                       const MapShieldConfig& config,
                                       const QString& text):
    TextRenderer(config.textStyle, true /*center*/,
                 false /*allow_default_typeface*/),
    is_resize_marker_to_wrap_center_label_(resize_marker_to_wrap_label),
    text_(text) {
  adjustTextY_ = CalculateBoundingBox(config);
}

void ShieldTextRenderer::Render(SkCanvas& canvas, const SkPoint& point) const {
  if (is_resize_marker_to_wrap_center_label_) {
    // When resize marker to wrap center label, just draw at the center.
    DrawText(canvas, text_, point.fX, point.fY + adjustTextY_);
  } else {
    // Else draw at the center of the bounding box.
    // Note: With all margins set to 0 the box is centered at 0.
    DrawText(canvas, text_, point.fX - (box_.fLeft + box_.fRight),
             point.fY + adjustTextY_ - (box_.fBottom + box_.fTop));
  }
}

float ShieldTextRenderer::CalculateBoundingBox(const MapShieldConfig& config) {
  SkScalar length = MeasureText(text_,
                                &box_.fTop,
                                &box_.fBottom) *
                    LabelPaths::kTextLengthAdjustment;
  // The text box_ bounding box_ is a bit up/down, adjust the drawing point
  const float adjust_text_y = ((-box_.fTop) - box_.fBottom) / 2.0;
  box_.fBottom += adjust_text_y;
  box_.fRight = length / 2.0;
  box_.fLeft = -box_.fRight;
  box_.fTop = -box_.fBottom;

  // Add margin
  box_.fTop -= config.top_margin_;
  box_.fLeft -= config.left_margin_;
  box_.fBottom += config.bottom_margin_;
  box_.fRight += config.right_margin_;
  return adjust_text_y;
}

PointRenderer::~PointRenderer() {
  if (pointRenderer_ != NULL) {
    delete pointRenderer_;
  }
  if (textRenderer_ != NULL) {
    delete textRenderer_;
  }
}

void PointRenderer::SetCenterLabel(const QString& centerLabel) {
  if (textRenderer_ != NULL) {
    delete textRenderer_;
    textRenderer_ = NULL;
  }
  textRenderer_ = new ShieldTextRenderer(
      mapFeatureConfig_.IsCenterLabelDependentIconBoundingBox(),
      mapFeatureConfig_.shield, centerLabel);
  if (mapFeatureConfig_.IsCenterLabelDependentIconBoundingBox()) {
    const MapShieldConfig::ShieldIconScaling scaling =
        (mapFeatureConfig_.pointMarker == VectorDefs::Circle ||
         mapFeatureConfig_.pointMarker == VectorDefs::Square ||
         mapFeatureConfig_.pointMarker == VectorDefs::EquilateralTriangle)
        ? MapShieldConfig::IconFixedAspectStyle
        : mapFeatureConfig_.shield.scaling_;

    pointRenderer_->ResizeToWrapTextBox(textRenderer_->GetBoundingBox(),
                                        scaling);
  }
}

PointRendererInterface* PointRenderer::CreatePointRenderer(
    const MapFeatureConfig& config) {
  PointRendererInterface* point_renderer;
  switch (config.pointMarker) {
    case VectorDefs::Circle:
    case VectorDefs::Oval:
      point_renderer = new OvalRenderer(config);
      break;
    case VectorDefs::Square:
    case VectorDefs::Rectangle:
      point_renderer = new RectangleRenderer(config);
      break;
    case VectorDefs::EquilateralTriangle:
    case VectorDefs::Triangle:
      point_renderer = new TriangleRenderer(config);
      break;
    case VectorDefs::Icon:
      point_renderer = new IconRenderer(config);
      break;
    default:
      point_renderer = NULL;
      break;
  }
  return point_renderer;
}

void PointRenderer::Render(SkCanvas& canvas, const SkPoint& point)
    const {
  pointRenderer_->Render(canvas, point.fX, point.fY);
  if (textRenderer_ != NULL) {
    textRenderer_->Render(canvas, point);
  }
}

}  // end namespace maprender

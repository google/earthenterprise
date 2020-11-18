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


#include "TextRenderer.h"
#include <Qt/q3cstring.h>
#include <qimage.h>
#include <SkFontHost.h>
#include "fusion/gst/maprender/SGLHelps.h"
#include <khSimpleException.h>
#include <autoingest/.idl/storage/MapSubLayerConfig.h>
#include <SkGraphics.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkBlurMaskFilter.h>


using QCString = Q3CString;

namespace maprender {


TextRenderer::TextRenderer(const MapTextStyleConfig &config,
                           bool center,
                           bool allow_default_typeface) :
    drawOutline(config.drawOutline)
{
  // translate "weight" to type style
  SkTypeface::Style typestyle = SkTypeface::kNormal;
  if (config.weight == MapTextStyleConfig::Bold)
    typestyle = SkTypeface::kBold;
  else if (config.weight == MapTextStyleConfig::Italic)
    typestyle = SkTypeface::kItalic;
  else if (config.weight == MapTextStyleConfig::BoldItalic)
    typestyle = SkTypeface::kBoldItalic;

  SkTypeface *typeface = FontInfo::GetTypeFace(config.font.toUtf8().constData(), typestyle);
  if (typeface == NULL) {
    throw khSimpleException("Typeface not found for map tile rendering: ")
        << config.font.utf8()
        << ((config.weight & SkTypeface::kBold) ? "-Bold" : "")
        << ((config.weight & SkTypeface::kItalic) ? "-Italic" : "");
  }

 // normal painting
  normalPaint.setTextAlign(center ? SkPaint::kCenter_Align
                           : SkPaint::kLeft_Align);
  normalPaint.setTextSize(config.size);
  normalPaint.setAntiAlias(true);
  normalPaint.setStyle(SkPaint::kFill_Style);
  normalPaint.setColor(SkColorFromQColor(config.color));
  normalPaint.setTypeface(typeface);

  // outline painting
  //  outlinePaint.setPorterDuffXfermode(SkPorterDuff::kSrc_Mode);
  outlinePaint.setTextAlign(center ? SkPaint::kCenter_Align
                            : SkPaint::kLeft_Align);
  outlinePaint.setTextSize(config.size);
  outlinePaint.setAntiAlias(true);
  outlinePaint.setStyle(SkPaint::kStrokeAndFill_Style);
  outlinePaint.setColor(SkColorFromQColor(config.outlineColor));
  outlinePaint.setStrokeWidth(config.outlineThickness);
  outlinePaint.setStrokeJoin(SkPaint::kRound_Join);
  outlinePaint.setTypeface(typeface);
}

void
TextRenderer::DrawText(SkCanvas &canvas, const QString &text,
                       SkScalar x, SkScalar y) const
{
  QCString utf8 = text.utf8();
  if (drawOutline) {
    canvas.drawText(utf8, utf8.length(), x, y, outlinePaint);
  }
  canvas.drawText(utf8, utf8.length(), x, y, normalPaint);
}

void
TextRenderer::DrawTextOnPath(SkCanvas &canvas,
                             const QString &text,
                             const SkPath &path,
                             SkScalar horiz_offset,
                             SkScalar vert_offset) const
{
  QCString utf8 = text.utf8();
  if (drawOutline) {
    canvas.drawTextOnPathHV(utf8.data(),
                          utf8.length(),
                          path,
                          horiz_offset,
                          vert_offset,
                          outlinePaint);
  }
  canvas.drawTextOnPathHV(utf8.data(),
                        utf8.length(),
                        path,
                        horiz_offset,
                        vert_offset,
                        normalPaint);
}

SkScalar
TextRenderer::MeasureText(const QString &text,
                          SkScalar* above, SkScalar* below) const
{
  QCString utf8 = text.utf8();
  SkRect bounds;
  const SkScalar retVal = drawOutline
      ? outlinePaint.measureText(utf8.data(), utf8.length(), &bounds)
      : normalPaint.measureText(utf8.data(), utf8.length(), &bounds);
  *above = bounds.fTop;
  *below = bounds.fBottom;
  return retVal;
}

QPixmap
TextStyleToPixmap(const MapTextStyleConfig &config,
                  const QColor &bgColor, unsigned int overrideSize)
{
  // generate string to render
  QString weightLabel;
  switch (config.weight) {
    case MapTextStyleConfig::Regular:
      weightLabel = "";
      break;
    case MapTextStyleConfig::Bold:
      weightLabel = "/b";
      break;
    case MapTextStyleConfig::Italic:
      weightLabel = "/i";
      break;
    case MapTextStyleConfig::BoldItalic:
      weightLabel = "/bi";
      break;
  }
  QString text = QString("%1/%2%3")
                 .arg(config.font)
                 .arg(config.size)
                 .arg(weightLabel);

  MapTextStyleConfig configCopy = config;
  if (overrideSize) {
    configCopy.size = overrideSize;
  }
  maprender::TextRenderer textRenderer(configCopy, false, true);

  // figure out how big of a pixmap we need and where we want to draw the text
  SkScalar textWidth = 0;
  SkScalar textAscent = 0;
  SkScalar textDescent = 0;
  textWidth = textRenderer.MeasureText(text, &textAscent, &textDescent);
  textAscent = -textAscent;
  SkScalar margin = 1;
  SkScalar pixelWidth  = textWidth + margin * 2;
  SkScalar pixelHeight = textAscent + textDescent + margin * 2;
  SkScalar posX = margin;
  SkScalar posY = margin + textAscent;

  // create the SkCanvas
  QImage qimage(int(pixelWidth), int(pixelHeight), 32);
  // qimage.setAlphaBuffer(true);
  qimage.fill(bgColor.rgb());

  SkBitmap bitmap;
  bitmap.setConfig(SkBitmap::kARGB_8888_Config,
                   uint(pixelWidth), uint(pixelHeight));
  bitmap.setPixels(qimage.bits());
  SkCanvas canvas(bitmap);

  // render the preview into a canvas
  textRenderer.DrawText(canvas, text, posX, posY);

  // convert to a QPixmap
  return QPixmap(qimage);
}


} // namespace maprender

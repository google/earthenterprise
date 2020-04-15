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

//

#ifndef FUSION_GST_MAPRENDER_TEXTRENDERER_H__
#define FUSION_GST_MAPRENDER_TEXTRENDERER_H__

#include <qpixmap.h>
#include <SkPaint.h>

class MapTextStyleConfig;
class QString;
class SkCanvas;

namespace maprender {

class TextRenderer {
 public:
  TextRenderer(const MapTextStyleConfig &config,
               bool center,
               bool allow_default_typeface);

  void DrawText(SkCanvas &canvas, const QString &text,
                SkScalar x, SkScalar y) const;
  void DrawTextOnPath(SkCanvas &canvas,
                      const QString &text,
                      const SkPath &path,
                      SkScalar horiz_offset,
                      SkScalar vert_offset) const;
  SkScalar MeasureText(const QString &text,
                       SkScalar* above = 0, SkScalar* below = 0) const;
  inline void SetDrawOutline(bool draw_outline) { drawOutline = draw_outline; }
  inline bool GetDrawOutline() { return drawOutline; }
  inline const SkPaint &GetNormalPaint() const { return normalPaint; }
  inline const SkPaint &GetOutlinePaint() const { return outlinePaint; }

 private:
  SkPaint normalPaint;
  SkPaint outlinePaint;
  bool drawOutline;
};

QPixmap TextStyleToPixmap(const MapTextStyleConfig &config,
                          const QColor &bgColor, unsigned int overrideSize = 0);



} // namespace maprender

#endif // FUSION_GST_MAPRENDER_TEXTRENDERER_H__

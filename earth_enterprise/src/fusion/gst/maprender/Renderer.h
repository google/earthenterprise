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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_MAPRENDER_RENDERER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_MAPRENDER_RENDERER_H_

#include <SkColor.h>

#include "fusion/gst/vectorprep/WorkflowProcessor.h"
#include "fusion/gst/maprender/RenderTypes.h"
#include "fusion/gst/maprender/Combiner.h"

class SkBitmap;
class SkCanvas;
class SkPaint;
class QColor;

namespace maprender {

typedef CombinerOutputTile RendererInputTile;

class RendererOutputTile {
 public:
  const QuadtreePath& GetPath() const { return path; }
  QuadtreePath path;
  khDeleteGuard<char, ArrayDeleter> pixelBuf;

  // hold out a pointer so we can shield the rest of the code from needing
  // more SGL headers
  khDeleteGuard<SkBitmap> bitmap;
  khDeleteGuard<SkCanvas> canvas;
  const bool owns_buffer_;

  explicit RendererOutputTile(std::uint32_t rasterSize);
  RendererOutputTile(std::uint32_t rasterSize, char* buffer);
  ~RendererOutputTile(void);

 private:
  DISALLOW_COPY_AND_ASSIGN(RendererOutputTile);
};


inline void ResetTile(RendererOutputTile* t) {
  // nothing to do
}



class Renderer
    : public workflow::Processor<RendererOutputTile, RendererInputTile> {
 public:
  typedef RendererOutputTile OutTile;
  typedef RendererInputTile  InTile;
  typedef workflow::Processor<OutTile, InTile> ProcessorBase;

  inline explicit Renderer(bool debug_) : ProcessorBase(), debug(debug_) { }

  virtual bool Process(OutTile *out, const InTile &in);

  static void RenderBox(SkCanvas& canvas, const SkRect& box, bool isRectBox,
                        const SkColor fillColor, const SkColor outlineColor);
 private:
  bool RenderFeaturePath(SkCanvas &canvas, const MapFeatureConfig &config,
                         const RenderPath& geom);
  bool RenderFeaturePolygon(SkCanvas &canvas, const MapFeatureConfig &config,
                            const RenderPath& geom);
  void RenderFeatureLabel(SkCanvas &canvas, const MapLabelConfig &config,
                          const FeatureLabel& label);
  void RenderFeatureShield(SkCanvas &canvas, const MapShieldConfig &config,
                           const FeatureShield& shield);
  void RenderSiteLabel(SkCanvas &canvas, const MapTextStyleConfig& config,
                       const QString& boundText, const SkPoint& point);

  bool debug;
};


}  // namespace maprender


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_MAPRENDER_RENDERER_H_

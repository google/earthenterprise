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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTBOXCUTTER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTBOXCUTTER_H_

#include <deque>

#include "fusion/gst/gstTypes.h"
#include "fusion/gst/gstGeode.h"
#include "fusion/gst/gstPolygonClipper2.h"

namespace fusion_gst {

// Class for clipping different types of geometry (gstGeode) against rectangle
// window. It uses BBox::ClipLine() to clip "line"-types (gstPolyLine,
// gstStreet) of geometry, and PolygonClipper to clip "polygon"-types of
// geometry (gstPolygon, gstPolygon25D, gstPolygon3D).
// using:
//   BoxCutter box_cutter;           // Create BoxCutter object.
//   box_cutter.SetClipRect(bbox);   // Set clipping window.
//   box_cutter.Run(geode, pieces);  // Run clipping.
class BoxCutter {
 public:
  struct Segment {
    Segment(const gstVertex& _v0, const gstVertex& _v1)
        : v0(_v0), v1(_v1) {}

    gstVertex v0;
    gstVertex v1;
  };

  explicit BoxCutter(bool cut_holes);
  BoxCutter(const gstBBox &_box, bool cut_holes);

  ~BoxCutter();

  // Sets clipping rectangle.
  //
  // @param wx1, wx2, wy1, wy2 coordinates of clipping rectangle.
  void SetClipRect(const double _wx1, const double _wx2,
                   const double _wy1, const double _wy2);

  // Sets clipping rectangle cpecified by bounding box.
  //
  // @param _box bounding box specified clipping window.
  void SetClipRect(const gstBBox &_box);

  // Runs clipping. It accepts input geometry element and list for output
  // geometry elements. Output geometry are parts of input geometry that lie
  // inside clipping window.
  //
  // @param geode input geode;
  // @param pieces list of output geodes.
  // @param completely_covered returns whether quad is completly covered
  // by polygon.
  //
  // @return number of output geodes.
  unsigned int Run(const gstGeodeHandle &geode,
           GeodeList &pieces, bool *completely_covered);

  inline unsigned int Run(const gstGeodeHandle &geodeh, GeodeList &pieces) {
    bool completely_covered = false;
    return Run(geodeh, pieces, &completely_covered);
  }

 private:
  // Clips line-types (gstPolyline, gstStreet) of geometry. Iterate across all
  // segments and cut pieces that lie inside clipping recnagle.
  void ClipLines(const gstGeode *geode, GeodeList *pieces) const;

  // Combines clipped segments into polylines.
  void CombineOrderedSegments(const std::deque<Segment>& segments,
                              const gstPrimType geode_prim_type,
                              GeodeList *pieces) const;

  // Converts clipped segments into geodes.
  void ConvertSegmentsToGeodes(const std::deque<Segment>& segments,
                               const gstPrimType geode_prim_type,
                               GeodeList *pieces) const;

  // Runs clipping for Polygon3D.
  // Function implements corresponding pre- and post-processing and calls
  // polygon clipping in 2D.
  void ClipPolygon3D(const gstGeode *geode,
                     GeodeList *pieces,  bool *completely_covered);

  // Clipping rectangle.
  gstBBox box_;

  // Auxiliary object to implement polygon clipping.
  PolygonClipper polygon_clipper_;
};

}  // namespace fusion_gst

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTBOXCUTTER_H_

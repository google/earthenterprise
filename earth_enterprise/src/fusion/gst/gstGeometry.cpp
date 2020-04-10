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


#include "fusion/gst/gstGeometry.h"

#include <GL/gl.h>
#include <GL/glu.h>

#include "common/khGuard.h"
#include "fusion/gst/gstFeature.h"

// TessPoly is a helper class that wraps the OpenGL GLU toolkit for tesselating
// and rendering polygons.
// Usage:
// TessPoly tesselator;
// std::vector<gstGeometryImpl::Point> points;
// /* fill in points */
// tesselator.AddContour(&points[0], points.size());
// tesselator.EndPolygon();
class TessPoly {
 private:
  GLUtesselator *tobj;  // the tessellation object
  std::vector<GLdouble*> vertex_storage_;

 public:
  TessPoly() {
    // Create a new tessellation object
    tobj = gluNewTess();
    // GLU_TESS_WINDING_ODD is sufficient for our purposes.

    // Set callback functions
    gluTessCallback(tobj, GLU_TESS_VERTEX, (GLvoid (*)( )) &glVertex3dv);
    gluTessCallback(tobj, GLU_TESS_BEGIN, (GLvoid (*)( )) &glBegin);
    gluTessCallback(tobj, GLU_TESS_END, (GLvoid (*)( )) &glEnd);
    // We don't support self-intersecting polygons so GLU_TESS_COMBINE is not
    // needed here.

    // Start the polygon.
    BeginPolygon();
  }

  ~TessPoly() {
    gluDeleteTess(tobj);
    for (std::vector<GLdouble*>::iterator it = vertex_storage_.begin();
         it != vertex_storage_.end(); ++it) {
      delete *it;
    }
  }

  // Render a polygon defined by an array of points of the given length.
  void AddContour(gstGeometryImpl::Point* points, int length) {
    GLdouble* verts = new GLdouble[length * 3];
    vertex_storage_.push_back(verts);

    for (int v = 0; v < length; ++v) {
      verts[(v * 3) + 0] = points[v].x;
      verts[(v * 3) + 1] = points[v].y;
      verts[(v * 3) + 2] = 0.0;
    }

    gluTessBeginContour(tobj);
    RenderContour(&verts[0], length);
    gluTessEndContour(tobj);
  }

  void SetWindingRule(GLenum winding_rule) {
    gluTessProperty(tobj, GLU_TESS_WINDING_RULE, winding_rule);
  }

  void RenderContour(GLdouble* vertexes, int length) {
    for (int v = 0; v < length; ++v) {
      gluTessVertex(tobj, &vertexes[v * 3],
                    reinterpret_cast<GLvoid*>(&vertexes[v * 3]));
    }
  }

  void BeginPolygon() {
    gluTessBeginPolygon(tobj, NULL);
  }

  void EndPolygon() {
    gluTessEndPolygon(tobj);
  }
};

gstGeometryImpl::gstGeometryImpl(const gstGeodeHandle &geodeh) {
  Init(geodeh);
}

void gstGeometryImpl::Init(const gstGeodeHandle &geodeh) {
  type_ = geodeh->PrimType();
  points_.reserve(geodeh->TotalVertexCount());

  // this is the exact center of our box
  double dx = geodeh->BoundingBox().CenterX();
  double dy = geodeh->BoundingBox().CenterY();

  // float center of box
  origin_.x = static_cast<float>(dx);
  origin_.y = static_cast<float>(dy);

  // error of double-center to float-center
  // this will be added back to all vertex data
  // once the float-center is subtracted
  float f_error_x = static_cast<float>(dx - static_cast<double>(origin_.x));
  float f_error_y = static_cast<float>(dy - static_cast<double>(origin_.y));

  switch (type_) {
    case gstUnknown:
      break;

    case gstPoint:
    case gstPoint25D:
    case gstPolyLine:
    case gstPolyLine25D:
    case gstStreet:
    case gstStreet25D:
    case gstPolygon:
    case gstPolygon25D:
    case gstPolygon3D:
      {
        const gstGeode *geode = static_cast<const gstGeode*>(&(*geodeh));
        ConvertGeode(geode, dx, dy, f_error_x, f_error_y);
      }
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      {
        const gstGeodeCollection *multi_geode =
            static_cast<const gstGeodeCollection*>(&(*geodeh));
        for (unsigned int p = 0; p < multi_geode->NumParts(); ++p) {
          const gstGeode *geode =
              static_cast<const gstGeode*>(&(*multi_geode->GetGeode(p)));
          ConvertGeode(geode, dx, dy, f_error_x, f_error_y);
        }
      }
      break;

    default:
      notify(NFY_FATAL, "%s: invalid geometry type %d.", __func__, type_);
      break;
  }
}

void gstGeometryImpl::ConvertGeode(const gstGeode *geode,
                                   const double dx, const double dy,
                                   const float f_error_x,
                                   const float f_error_y) {
  // convert global geode vertexes to local point vertexes
  for (unsigned int p = 0; p < geode->NumParts(); ++p) {
    lengths_.push_back(geode->VertexCount(p));
    for (unsigned int v = 0; v < geode->VertexCount(p); ++v) {
      const gstVertex& vert = geode->GetVertex(p, v);
      points_.push_back(Point(static_cast<float>(vert.x - dx) + f_error_x,
                              static_cast<float>(vert.y - dy) + f_error_y));
    }
  }
}

void gstGeometryImpl::Draw(
    const gstBBox& frust,
    const gstFeaturePreviewConfig &preview_config) {
  glPushMatrix();

  glTranslatef(origin_.x - frust.w, origin_.y - frust.s, 0);

  // We're drawing 3 types of geometries.
  // Points, Polylines and Polygons
  bool is_polyline_type = (ToFlatPrimType(type_) == gstPolyLine ||
                           ToFlatPrimType(type_) == gstStreet);
  bool is_polygon_type = (type_ == gstPolygon ||
                          type_ == gstPolygon25D ||
                          type_ == gstPolygon3D ||
                          type_ == gstMultiPolygon ||
                          type_ == gstMultiPolygon25D ||
                          type_ == gstMultiPolygon3D);

  if (type_ == gstPoint || preview_config.feature_display_type_
      == VectorDefs::PointZ) {
    // Nothing to do here. Preview is label and icon only.
  } else if (is_polyline_type || is_polygon_type) {
    // Render the polygon/polyline as a line, filled or not.
    // The code to render the outline for polyline and polygon is the same.

    // May need to fill the polygon.
    // First render the polygon fill if specified.
    if (is_polygon_type && (preview_config.polygon_draw_mode_
        == VectorDefs::FillAndOutline || preview_config.polygon_draw_mode_
        == VectorDefs::FillOnly)) {
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_BLEND);

      float clr[4];
      preview_config.glPolyColor(clr);
      glColor4fv(clr);

      TessPoly tess_poly;
      unsigned int addr = 0;
      for (std::vector< unsigned int> ::const_iterator it = lengths_.begin(); it
          != lengths_.end(); ++it) {
        tess_poly.AddContour(&points_[addr], *it);
        addr += *it;
      }
      tess_poly.EndPolygon();
      glDisable(GL_BLEND);
    }

    // Render the polyline outline or polygon outline
    // (as long as it's not "FillOnly").
    if (is_polyline_type || preview_config.polygon_draw_mode_
        != VectorDefs::FillOnly) {
      float clr[4];
      preview_config.glLineColor(clr);
      glColor4fv(clr);
      glLineWidth(preview_config.line_width_);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_BLEND);

      // iterate through each subpart
      // Note: GL_VERTEX_ARRAY causes problems with 64-bit SLES10.
      // Use glBegin/glEnd until problem is understood.
      unsigned int addr = 0;
      for (std::vector< unsigned int> ::const_iterator it = lengths_.begin(); it
          != lengths_.end(); ++it) {
        glBegin(GL_LINE_STRIP);
        for (unsigned int j = 0; j < *it; ++j) {
          glVertex2f(points_[addr + j].x, points_[addr + j].y);
        }
        glEnd();
        addr += *it;
      }

      glDisable(GL_BLEND);
    }
  }

  glPopMatrix();
}

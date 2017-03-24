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


#include "fusion/gst/gstGeomUtils.h"

#include <assert.h>
#include <cmath>

#include "fusion/gst/gstConstants.h"
#include "fusion/gst/gstMathUtils.h"
#include "common/notify.h"

namespace fusion_gst {

bool ComputePlaneEquation(const Vert3<double> &a,
                          const Vert3<double> &b,
                          const Vert3<double> &c,
                          Vert3<double> *normal,
                          double *distance) {
  Vert3<double> u = b - a;
  Vert3<double> v = c - a;

  *normal = u.CrossVector(v);

  double mod_normal = normal->length();

  if (Equals(mod_normal, .0)) {
    return false;  // source points are collinear, plane equation can't be
    // computed.
  }

  normal->x /= mod_normal;
  normal->y /= mod_normal;
  normal->z /= mod_normal;

  *distance = normal->x*a.x + normal->y*a.y + normal->z*a.z;

  return true;  // plane equation is computed.
}

bool Collinear(const gstVertex &a, const gstVertex &b, const gstVertex &c,
               const double epsilon) {
  double dx = c.x - a.x;
  double dy = c.y - a.y;
  double ax = b.x - a.x;
  double ay = b.y - a.y;
  double d2 = (dx * dx) + (dy * dy);
  double a2 = (ax * ax) + (ay * ay);

  double nda = (dx * ay) - (dy * ax);
  if ((nda * nda) <= (epsilon * d2 * a2))
    return true;

  return false;

#if 0
  // this method computes the area of the triangle
  // described by the three points in 2D.
  // im still debating whether i should do a 3D test

  const double epsilon = 1e-15;
  double area = a.x(b.y - c.y) + b.x(c.y - a.y) + c.x(a.y - b.y);
  if (area <= epsilon)
    return true;

  return false;
#endif
}

bool Collinear3D(const gstVertex &a, const gstVertex &b, const gstVertex &c,
                 const double epsilon) {
  // Calculate vectors and their cross-product vector.
  gstVertex u = b - a;
  gstVertex v = c - a;

  gstVertex vcross = u.CrossVector(v);

  // Calculate lengths of source vectors and their cross-product vector.
  double d_u = (u.x * u.x) + (u.y * u.y) + (u.z * u.z);
  double d_v = (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
  double d_vcross = (vcross.x * vcross.x) + (vcross.y * vcross.y) +
      (vcross.z * vcross.z);

  if (d_vcross <= (epsilon * d_u * d_v)) {
    return true;
  }

  return false;
}

int Orientation(const gstVertex &a, const gstVertex &b, const gstVertex &c) {
  gstVert2D u(b.x - a.x, b.y - a.y);
  gstVert2D v(c.x - a.x, c.y - a.y);
  double cross = u.Cross(v);
  if (Equals(cross, .0, kDblEpsilon)) {
    return 0;
  } else if (cross > .0) {
    return 1;
  } else {
    assert(cross < .0);
    return -1;
  }
}

bool AreSegmentsIntersecting(const gstVertex &a, const gstVertex &b,
                             const gstVertex &c, const gstVertex &d) {
  // check location of second segment endpoints relative to first segment.
  int orient_c = Orientation(a, b, c);
  int orient_d = Orientation(a, b, d);

  if (orient_c * orient_d == 1)
    return false;  // c and d are on the one side relative to line (a, b).

  // check location of first segment endpoints relative to second segment.
  int orient_a = Orientation(c, d, a);
  int orient_b = Orientation(c, d, b);

  return (orient_a * orient_b == -1 ||
          (orient_a * orient_b == 0 && orient_c * orient_d == -1));
}

gstVertex LinesIntersection2D(const gstVertex &a, const gstVertex &b,
                              const gstVertex &c, const gstVertex &d) {
  gstVert2D u(b.x - a.x, b.y - a.y);
  gstVert2D v(d.x - c.x, d.y - c.y);

  gstVertex intersection_pt;

  if (.0 == u.x) {  // first line is vertical.
    assert(v.x != .0);
    intersection_pt.x = a.x;
    intersection_pt.y = GetLineYFromX(c, d, intersection_pt.x);
  } else if (.0 == u.y) {  // first line is horizontal.
    assert(v.y != .0);
    if (0 == v.x) {
      intersection_pt = gstVertex(c.x, a.y, .0);
    } else {
      intersection_pt.y = a.y;
      intersection_pt.x = GetLineXFromY(c, d, intersection_pt.y);
    }
  } else if (.0 == v.x) {  // second_line is vertical.
    assert(u.x != .0);
    intersection_pt.x = c.x;
    intersection_pt.y = GetLineYFromX(a, b, intersection_pt.x);
  } else if (.0 == v.y) {   // second line is horizontal.
    assert(u.y != .0);
    intersection_pt.y = c.y;
    intersection_pt.x = GetLineXFromY(a, b, intersection_pt.y);
  } else {
    // generic case.
    gstVert2D pt2D = gstVert2D::Intersect(
        gstVert2D(a.x, a.y), gstVert2D(b.x, b.y),
        gstVert2D(c.x, c.y), gstVert2D(d.x, d.y));
    intersection_pt.x = pt2D.x;
    intersection_pt.y = pt2D.y;
  }
  // calculate Z-coordinate of intersection point.
  GetLineZFromXY(a, b, &intersection_pt);
  return intersection_pt;
}


bool SegmentClipper::Run(const gstVertex &v1, const gstVertex &v2,
                         gstVertexVector &intersections) const {
  double deltax, deltay, xin, xout, yin, yout;
  double tinx, tiny, toutx, touty, tin1, tin2, tout1;

  double x1 = v1.x;
  double y1 = v1.y;
  double x2 = v2.x;
  double y2 = v2.y;

  x1 = ((x1 == wx1_) || (x1 == wx2_)) ? x1 + kEpsilon : x1;
  x2 = ((x2 == wx1_) || (x2 == wx2_)) ? x2 + kEpsilon : x2;
  y1 = ((y1 == wy1_) || (y1 == wy2_)) ? y1 + kEpsilon : y1;
  y2 = ((y2 == wy1_) || (y2 == wy2_)) ? y2 + kEpsilon : y2;

  deltax = x2 - x1;
  if (.0 == deltax) {   // bump off of the vertical.
    deltax = (x1 > wx1_) ? -kAlmostZero : kAlmostZero;
  }
  deltay = y2 - y1;
  if (.0 == deltay) {   // bump off of the horizontal.
    deltay = (y1 > wy1_) ? -kAlmostZero : kAlmostZero;
  }

  if (deltax > 0) {   //  Points to right.
    xin = wx1_;
    xout = wx2_;
  } else {            //  Points to left.
    xin = wx2_;
    xout = wx1_;
  }

  if (deltay > 0) {   //  Points up.
    yin = wy1_;
    yout = wy2_;
  } else {            //  Points down.
    yin = wy2_;
    yout = wy1_;
  }

  tinx = (xin - x1) / deltax;
  tiny = (yin - y1) / deltay;

  if (tinx < tiny) {  // Hits x first.
    tin1 = tinx;
    tin2 = tiny;
  } else {            // Hits y first.
    tin1 = tiny;
    tin2 = tinx;
  }

  // (1 > tin1) && (1 > tin2) excludes touch points (touch by second
  // end point).
  // (1 >= tin1) && (1 >= tin2) to report touch points.
  if (1 >= tin1) {
    if (1 >= tin2) {
      toutx = (xout - x1)/deltax;
      touty = (yout - y1)/deltay;

      if (toutx < touty) {  //  Hits x first.
        tout1 = toutx;
      } else {             //   Hits y first.
        tout1 = touty;
      }

      // (0 < tin2 || 0 < tout1) excludes touch points (touch by first end
      // point). (0 <= tin2 || 0 <= tout1) to report touch points.
      if (0 <= tin2 || 0 <= tout1) {
        // (tin2 < tout1) excludes cases where segment intersects clipping
        // window corner. (tin2 <= tout1) to report corner intersection points.
        if (tin2 <= tout1) {
          // Compute entry point.
          if (0 < tin2) {
            if (tinx > tiny) {
              intersections.push_back(gstVertex(xin, (y1 + tinx*deltay), .0));
            } else {
              intersections.push_back(gstVertex((x1 + tiny*deltax), yin, .0));
            }
          } else {
            intersections.push_back(gstVertex(x1, y1, .0));
          }

          // Compute exit point.
          if (1 > tout1) {
            if (toutx < touty) {
              intersections.push_back(gstVertex(xout, (y1 + toutx*deltay), .0));
            } else {
              intersections.push_back(gstVertex((x1 + touty*deltax), yout, .0));
            }
          } else {
            intersections.push_back(gstVertex(x2, y2, .0));
          }
        }
      }
    }
  }

  assert((intersections.size() <= 2));

  if (intersections.empty()) {
    return false;  // no intersection with clipping window.
  }

  return true;
}

}  // namespace fusion_gst

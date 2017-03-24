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


#include "fusion/gst/gstPolygonUtils.h"
#include <assert.h>
#include <values.h>
#include <algorithm>
#include <limits>
#include <vector>
#include "fusion/gst/gstBBox.h"

void gstPolygonUtils::GetClosestPointOnPolygonBoundary(
     const std::vector<gstVertex>& vertex_list,
     const gstVertex& point, gstVertex* closest_point, double* distance) {
  assert(!vertex_list.empty());  // Should never get an empty vertex_list!
  // We assume the polygon is closed (i.e., first and last vertex are the same.
  assert(vertex_list[0] == vertex_list[vertex_list.size()-1]);

  gstVert2D point_2d(point);
  gstVert2D closest_point_2d;
  double min_squared_distance = std::numeric_limits<double>::max();

  // Start by walking the vertices to identify the closest vertex to our
  // point (skip the last vertex as it's the same as the first).
  for (int i = 0; i < static_cast<int>(vertex_list.size()) - 1; ++i) {
    const gstVertex& vertex = vertex_list[i];
    double dx = vertex.x - point.x;
    double squared_distance = dx * dx;
    if (squared_distance >= min_squared_distance)
      continue;  // No improvement...don't bother checking y coordinates.
    double dy = vertex.y - point.y;
    squared_distance += dy * dy;
    if (squared_distance < min_squared_distance) {
      min_squared_distance = squared_distance;
      closest_point_2d = gstVert2D(vertex);
    }
  }

  // Create a bounding box around the point which encloses our search radius.
  *distance = sqrt(min_squared_distance);  // our current search radius.
  gstBBox search_bbox;
  search_bbox.Grow(point);
  search_bbox.ExpandBy(*distance);

  // Now we need to walk the edges...a little more work, but we have a
  // bound on the search so we can ignore most edges quickly.
  gstVert2D vertex_a(vertex_list[0]);
  gstVert2D vertex_b;
  for (int i = 1; i < static_cast<int>(vertex_list.size()); ++i,
       vertex_a = vertex_b) {
    vertex_b = gstVert2D(vertex_list[i]);
    if (vertex_b == vertex_a)
      continue;  // No edge.

    // Test that the bounding box of the edge overlaps our search bbox.
    gstBBox edge_box(vertex_a.x, vertex_b.x, vertex_a.y, vertex_b.y);
    if (!search_bbox.Intersect(edge_box))
      continue;  // The edge doesn't intersect our search radius.

    // Need to compute the perpendicular projection of our point onto the edge.
    // d_p_a is the vector from point to vertex_a
    gstVert2D d_p_a(point_2d.Subtract(vertex_a));
    // d_b_a is the vector from vertex_b to vertex_a
    gstVert2D d_b_a(vertex_b.Subtract(vertex_a));
    // The dot product = projection = projection_length * length(d_b_a)
    double projection = d_p_a.Dot(d_b_a);
    if (projection <= 0)
      continue;  // The closest point is on the other side of vertex_a. Ignore.
    double d_b_a_length_squared = d_b_a.SquaredLength();
    if (projection >= d_b_a_length_squared)
      continue;  // The closest point is on the other side of vertex_b. Ignore.

    // Otherwise, the closest point is in between vertex_a and vertex_b.
    // d_closest_to_p = vector from closest point on edge to point p
    // Get the closest point and squared distance while avoiding the
    // square root.
    // d_closest_to_p = (d_p_a - (projection/|d_b_a|^2) d_b_a)
    // d_closest_to_p |d_b_a|^2 = |d_b_a|^2 d_p_a - projection d_b_a
    // We can compute the right side and then divide by d_b_a_length_squared
    // to get the vector between the closest point and p.
    gstVert2D right_side =
      d_p_a.Scale(d_b_a_length_squared).Subtract(d_b_a.Scale(projection));
    gstVert2D d_closest_to_p(right_side.Scale(1.0/d_b_a_length_squared));
    double squared_distance = d_closest_to_p.SquaredLength();
    if (squared_distance < min_squared_distance) {
      min_squared_distance = squared_distance;
      closest_point_2d = point_2d.Subtract(d_closest_to_p);
    }
  }

  // Pack up the results.
  *closest_point = gstVertex(closest_point_2d.x, closest_point_2d.y, 0);
  *distance = sqrt(min_squared_distance);
}

gstVertex gstPolygonUtils::FindPointInPolygonRelativeToPoint(
  const std::vector<gstVertex>& vertex_list,
  const gstVertex& origin) {
  if (vertex_list.empty())
    return origin;  // Return origin by default.

  // Using the origin, compute the largest piece of contiguous overlap
  // of the polygon interior with both the x- and y-axes which pass
  // through the origin. This will give us 2 good candidates for
  // the interior point.
  double horizontal_overlap_center = 0;
  double horizontal_overlap_length = 0;
  HorizontalOverlap(vertex_list, origin.y,
                    &horizontal_overlap_center,
                    &horizontal_overlap_length);

  double vertical_overlap_center = 0;
  double vertical_overlap_length = 0;
  VerticalOverlap(vertex_list, origin.x,
                    &vertical_overlap_center,
                    &vertical_overlap_length);

  // Now we have two candidates:
  // Choose the one furthest from any edge.
  gstVertex center_horizontal(horizontal_overlap_center, origin.y);
  gstVertex center_vertical(origin.x, vertical_overlap_center);
  if (horizontal_overlap_length == 0 && vertical_overlap_length == 0)
    return origin;  // No valid overlap was found, it indicates the
                    // origin was wrong.
  if (horizontal_overlap_length == 0)
    return center_vertical;  // No horizontal overlap.
                             // Return the vertical candidate.
  if (vertical_overlap_length == 0)
    return center_horizontal;  // No vertical overlap...return the horizontal
                               // candidate.

  double distance_to_center_h;
  double distance_to_center_v;
  gstVertex closest_point;  // Ignored.
  GetClosestPointOnPolygonBoundary(vertex_list, center_horizontal,
                                   &closest_point, &distance_to_center_h);
  GetClosestPointOnPolygonBoundary(vertex_list, center_vertical,
                                   &closest_point, &distance_to_center_v);

  // Take the center from the largest overlap run.
  // Just make sure we found a valid overlap (i.e., overlap_length > 0,
  // to make sure we're inside the polygon).
  if (distance_to_center_h == distance_to_center_v) {
    // In case of tie, give it to the candidate with the larger overlap length.
    if (horizontal_overlap_length >= vertical_overlap_length)
      return center_horizontal;
    else
      return center_vertical;
  } else if (distance_to_center_h > distance_to_center_v) {
    return center_horizontal;
  } else {
    return center_vertical;
  }
}

// Tests for convexity by looking for a change in the cross product as we
// walk around the polygon.
// This is made slightly more complex because we have to assume
// 1) we don't know the winding (whether Clockwise or counterclockwise)
// 2) there are possibly duplicate points
bool gstPolygonUtils::IsConvex(const std::vector<gstVertex>& vertex_list) {
  int vertex_count = vertex_list.size();
  if (vertex_count <= 4)
    return true;  // This is a triangle (remember vertex 0 is repeated so
                  // we have 4 vertices), by definition convex.

  // We assume the polygon is closed (i.e., first and last vertex are the same.
  assert(vertex_list[0] == vertex_list[vertex_list.size()-1]);

  // Walk the edges of the polygon and return false if there is a
  // concave corner detected (i.e., the sign of the cross product of
  // edge vectors switches between positive and negative).
  // At each step we need the next and previous vertex...so for vertex 0, we
  // will need vertex 1, vertex 0 and vertex (N-2)
  // (since vertex N-1 == vertex 0).

  // NOTE: we need to beware of repeated points which will kill our test if
  // we don't skip over them during the walk.
  // The last vertex in the list is always the same as the first...we want
  // the last unique vertex...so we step back by one from the end.
  int last_vertex_index = vertex_count - 2;
  gstVert2D vertex_a(vertex_list[last_vertex_index]);
  gstVert2D vertex_b(vertex_list[0]);
  gstVert2D d_vertex_b_to_a(vertex_b.Subtract(vertex_a));
  // We need to have a non-zero vector for d_vertex_b_to_a (i.e., b != a).
  // walk backwards from last_vertex_index until we find it.
  while (d_vertex_b_to_a.IsZero()) {
    last_vertex_index--;
    if (last_vertex_index <= 1)
      return true;  // We have a degenerate polygon, a line or point
                    // ...convex it is.
    vertex_a.x = vertex_list[last_vertex_index].x;
    vertex_a.y = vertex_list[last_vertex_index].y;
    d_vertex_b_to_a = vertex_b.Subtract(vertex_a);
  }
  // Keep the last edge direction (d_vertex_b_to_a) around for the last
  // test.
  gstVert2D d_vertex_last_edge_direction = d_vertex_b_to_a;
  // Keep track of the cross products at each vertex...if we have
  // a positive AND negative cross product, it means we have a concavity.
  bool cross_product_negative = false;
  bool cross_product_positive = false;
  // We need to Analyze (vertex_count - 1) corners (modulo the repeated points).
  // The loop assumes vertex_a and vertex_b are initialized.
  for (int i = 1; i <= last_vertex_index + 1; ++i) {
    // Get the next vertex in the list to form our triplet.
    gstVert2D vertex_c(vertex_list[i]);
    gstVert2D d_vertex_c_to_b(vertex_c.Subtract(vertex_b));

    // Special case: we've reached the end and are testing the angle at the
    // last vertex (not counting the very last vertex which is always the same
    // as the first).  In this case, use the d_vertex_last_edge_direction since
    // we did some work to make sure it was non-degenerate.
    if (i > last_vertex_index)
      d_vertex_c_to_b = d_vertex_last_edge_direction;
    if (d_vertex_c_to_b.IsZero())
      continue;  // Skip over repeated points.

    // Take the cross product at the corner B.
    // The sign of the cross product will correlate to the angle of the edges.
    // Looking for combination of positive and negative cross products within
    // our polygon to indicate concavity (i.e., exterior angle < 180 degrees).
    double cross_product = d_vertex_c_to_b.Cross(d_vertex_b_to_a);
    cross_product_negative |= cross_product < 0.0;
    cross_product_positive |= cross_product > 0.0;
    if (cross_product_negative && cross_product_positive)
      return false;  // We have a change in sense of the cross products,
                     // indicating a concavity.
    // rotate the vertices by one.
    vertex_a = vertex_b;
    vertex_b = vertex_c;
    d_vertex_b_to_a = d_vertex_c_to_b;
  }
  return true;
}


void gstPolygonUtils::HorizontalOverlap(
  const std::vector<gstVertex>& vertex_list,
  double y_intercept,
  double* coordinate,
  double* length_of_largest_piece) {
  *length_of_largest_piece = 0;
  *coordinate = 0;
  if (vertex_list.size() <= 3)
    return;  // Degenerate polygon.

  // Walk the polygon and build up a list of the edge crossings.
  std::vector<PolygonCrossing> crossings;
  ComputeHorizontalCrossings(vertex_list, y_intercept, &crossings);

  // Now process the crossings to get the results.
  LargestOverlapStats(crossings, coordinate, length_of_largest_piece);
}

// Rather than duplicate the code for ComputeHorizontalCrossings we're going
// to make a copy of the vertices and transpose the coordinates.
// There's a performance penalty for this, but we are ignoring for now.
void gstPolygonUtils::VerticalOverlap(
  const std::vector<gstVertex>& vertex_list,
  double x_intercept,
  double* coordinate,
  double* length_of_largest_piece) {
  *length_of_largest_piece = 0;
  *coordinate = 0;
  if (vertex_list.size() <= 3)
    return;  // Degenerate polygon.

  // transpose the coordinates
  std::vector<gstVertex> vertex_list_transposed(vertex_list.size());
  for (int i = 0; i < static_cast<int>(vertex_list.size()); ++i) {
    const gstVertex& vertex = vertex_list[i];
    vertex_list_transposed[i] = gstVertex(vertex.y, vertex.x);
  }

  // Call the horizontal version with our transposed polygon.
  HorizontalOverlap(vertex_list_transposed, x_intercept, coordinate,
                    length_of_largest_piece);
}

void gstPolygonUtils::ComputeHorizontalCrossings(
  const std::vector<gstVertex>& vertex_list,
  double y_intercept,
  std::vector<gstPolygonUtils::PolygonCrossing>* crossings) {
  // We assume the polygon is closed (i.e., first and last vertex are the same.
  assert(vertex_list[0] == vertex_list[vertex_list.size()-1]);

  gstVertex vertex_a(vertex_list[0]);
  gstVertex vertex_b(vertex_list[0]);

  for (int i = 1; i < static_cast<int>(vertex_list.size()); ++i,
       vertex_a = vertex_b) {
    // Get the next vertex in the list to form our triplet.
    vertex_b = vertex_list[i];
    if (vertex_b == vertex_a)
      continue;  // Skip repeated points.

    double y_a = vertex_a.y;
    double y_b = vertex_b.y;

    // We're going to check all the edge crossing cases.
    // First check for no overlap.
    if ((y_a > y_intercept && y_b > y_intercept) ||
        (y_a < y_intercept && y_b < y_intercept)) {
      continue;  // No overlap of this edge.
    }

    // We have some overlap, several cases to consider.
    double x_a = vertex_a.x;
    double x_b = vertex_b.x;

    // Check for simple crossing or vertex on the intercept.
    if (y_a < y_intercept && y_b > y_intercept ||
        y_a > y_intercept && y_b < y_intercept) {
      // Case 1: simple crossing of the edge with the line.
      // simply intersect this line segment with y = y_intercept
      double dy = y_b - y_a;
      double dy_intercept = y_intercept - y_a;
      double dx = x_b - x_a;
      double x_coordinate = x_a + dx * dy_intercept / dy;
      crossings->push_back(gstPolygonUtils::PolygonCrossing(
           x_coordinate,
           gstPolygonUtils::kCrossing));
    } else if (y_b == y_intercept) {  // vertex_b is on the y_intercept
      // To identify the crossing properly we need to know vertex_c.
      // Find the next vertex (skipping duplicates)
      // remember vertex_list[0] is same as the last.
      int next_id = (i < static_cast<int>(vertex_list.size()) - 1) ? i + 1 : 1;
      gstVertex vertex_c(vertex_list[next_id]);
      while (vertex_c == vertex_b) {
        next_id++;
        if (next_id >= static_cast<int>(vertex_list.size()) - 1)
          next_id = 1;  // Skip to 1 if necessary.
        vertex_c = vertex_list[next_id];
      }
      double y_c = vertex_c.y;

      // 5 more cases for vertex_b here:
      //  - normal crossing at vertex_b,
      //  - a corner that doesn't cross
      //  - both edges on either side of b are collinear with the intercept
      //  - one edge on the intercept and the other edge is above
      //  - one edge on the intercept and the other edge is below
      if ((y_a < y_intercept && y_c > y_intercept) ||
          (y_a > y_intercept && y_c < y_intercept)) {
        // Case 2: normal crossing:
        // y_a & y_c on opposite sides of y_intercept
        crossings->push_back(gstPolygonUtils::PolygonCrossing(x_b,
                gstPolygonUtils::kCrossing));
      } else if ((y_a < y_intercept && y_c < y_intercept) ||
      (y_a > y_intercept && y_c > y_intercept)) {
        // Case 3: corner with no crossing:
        // y_a & y_c on same sides of y_intercept
        crossings->push_back(gstPolygonUtils::PolygonCrossing(x_b,
                gstPolygonUtils::kTouchingButNotCrossing));
      } else if (y_a == y_intercept && y_c == y_intercept) {
        // Case 4: both edges colinear (ignore this vertex)
      } else {
        // edge_a_to_b OR edge_b_to_c is collinear with y_intercept
        double y_other = (y_a == y_intercept) ? y_c : y_a;
        if (y_other < y_intercept) {
          // Case 5: one edge colinear, other from below
          crossings->push_back(gstPolygonUtils::PolygonCrossing(x_b,
                    gstPolygonUtils::kCollinearFromBelow));
        } else {  // (y_other > y_intercept)
          // Case 6: one edge colinear, other from above
          crossings->push_back(gstPolygonUtils::PolygonCrossing(x_b,
                    gstPolygonUtils::kCollinearFromAbove));
        }
      }
    } else {  // y_a == y_intercept
      // Ignore...we're only worried about categorizing vertex_b.
    }
  }

  // Sort the intersections by coordinate.
  std::sort(crossings->begin(), crossings->end(), PolygonCrossing::LessThan);
}

// Crossings indicate where a polygon intersects a line passing through it.
// Generally if no vertices of the polygon lie on the line, we'll have
// 2 crossing points for every overlapping chunk of the polygon...
// the first point indicates we've entered the interior of the polygon and
// the second indicates we've exited...likewise for subsequent pairs.
// In general we have to note places where:
//   1) the polygon intersects but doesn't cross (a vertex on the line)
//   2) the polygon intersects and crosses (an edge on the line)
// This method walks along the sorted crossings and keeps track of when
// we enter and when we exit the polygon and returns the largest overlapping
// piece (center and length).
void gstPolygonUtils::LargestOverlapStats(
  std::vector<PolygonCrossing>& crossings,
  double* center_of_longest_piece,
  double* length_of_longest_piece) {
  *length_of_longest_piece = 0;
  *center_of_longest_piece = 0;
  if (crossings.size() < 1)
    return;  // Not a valid slice.
  if (crossings.size() == 1) {
    // Degenerate case but worth returning something useful.
    *center_of_longest_piece = crossings[0].coordinate;
    return;
  }

  // Walk through the crossings, keeping track of when we enter and exit.
  // On each exit from the polygon, compare the most recent overlap piece
  // with the largest encountered so far which we'll return at the end.
  std::vector<PolygonCrossing>::const_iterator iter = crossings.begin();
  double x_a = 0;  // Placeholder for start of an overlapping run.
  bool is_inside_slice = false;  // Start from outside...our first crossing
                                 // may actually be a kTouchingButNotCrossing
                                 // or kCollinearFrom*
  for (; iter != crossings.end(); ++iter) {
    double x_b = iter->coordinate;
    gstPolygonUtils::CrossingType current_crossing = iter->crossing_type;
    if (is_inside_slice) {
      double length = x_b - x_a;  // The length of the current piece of overlap.

      if (length > *length_of_longest_piece) {
        // Update the results if we found a longer run of overlap.
        *length_of_longest_piece = length;
        // Compute the midpoint between begin and end of overlap.
        *center_of_longest_piece = (x_a + x_b) / 2.0;
      }
      if (current_crossing == kTouchingButNotCrossing) {
        x_a = x_b;  // We're still inside the polygon, but start a new overlap
                    // run since we don't want to return a point on a vertex
                    // of the polygon.
      } else {  // other crossings mean we're not in the polygon or
                // on a collinear edge of the polygon.
        is_inside_slice = false;
        if (current_crossing != kCrossing) {
          // Collinear* : need to skip to the next crossing which will indicate
          // if this Collinear edge is "Crossing" the intercept
          // or not crossing.
          ++iter;  // Collinear crossings come in pairs.
          // If they're not actually crossing the intercept, then
          // it's like a "kTouchingButNotCrossing" with extent...so start a new
          // run.
          if (iter->crossing_type == current_crossing) {
            // We're starting a new overlap run at the end of the segment.
            x_a = iter->coordinate;
            is_inside_slice = true;
          }
        }
      }
    } else {  // Current crossing is not in an overlapping run.
      if (current_crossing == kCrossing) {
        // If we hit normal crossing, we are back inside the slice.
        is_inside_slice = true;
        x_a = x_b;
      } else if (current_crossing == kTouchingButNotCrossing) {
        // kTouchingButNotCrossing => still outside, do nothing.
      } else {  // kCollinearFromAbove or kCollinearFromBelow
        // Collinear* : need to skip to the next crossing which will indicate
        // if this Collinear edge is "Crossing" the intercept
        // or not crossing.
        ++iter;  // Collinear crossings come in pairs.
        // If they're actually crossing the intercept, then
        // it's like a "Crossing" with extent...so start a new run.
        // i.e., if current is kCollinearFromAbove and next is
        // kCollinearFromBelow this indicates a crossing into the polygon.
        if (iter->crossing_type != current_crossing) {
          // We're starting a new overlap run at the end of the segment.
          x_a = iter->coordinate;
          is_inside_slice = true;
        }
      }
    }
  }
}

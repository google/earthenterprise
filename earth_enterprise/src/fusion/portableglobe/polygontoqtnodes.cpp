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


// Methods for converting a polygon from a kml file into a
// set of quadtree nodes that the polygon encompasses.

#include "fusion/portableglobe/polygontoqtnodes.h"

#include <math.h>
#include <notify.h>
#include <stdlib.h>
#include <algorithm>
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <string>
#include <vector>
#include <sstream>
#include "fusion/portableglobe/quadtree/qtutils.h"

namespace fusion_portableglobe {

/**
 * Polygon constructor.
 * Reads in the first polygon from the given kml file.
 */
Polygon::Polygon(std::istream* fin) {
  std::string token;

  bool found_linear_ring = false;
  bool found_coordinates = false;

  // TODO: Use more robust xml parsing.
  while (!fin->eof()) {
    token = "";
    *fin >> token;
    if (!token.empty()) {
      if (found_linear_ring) {
        if (found_coordinates) {
          if ("</coordinates>" == token) {
            break;
          }

          int first_comma = token.find(',');
          int second_comma = token.find(',', first_comma + 1);
          double x;
          double y;
          // NOLINT(runtime/printf)
          sscanf(token.substr(0, first_comma).c_str(), "%lf", &x);
          // NOLINT(runtime/printf)
          sscanf(token.substr(first_comma + 1,
                              second_comma - first_comma - 1).c_str(),
                              "%lf", &y);
          vertices.push_back(Vertex(x, y));
        } else {
          found_coordinates = "<coordinates>" == token;
        }
      } else {
        found_linear_ring = "<LinearRing>" == token;
      }
    }
  }

  notify(NFY_NOTICE, "Added polygon of %d vertices.",
         static_cast<int>(vertices.size()));
}


/**
 * Polygons constructor.
 * Reads in the polygons from the given kml file.
 */
Polygons::Polygons(const std::string& kml_file) {
  std::ifstream fin(kml_file.c_str());
  AddPolygonsFromStream(fin);
  fin.close();
}


/**
 * Polygons constructor.
 * Reads in the polygons from the given kml string.
 */
Polygons::Polygons(const kml_as_string&, const std::string& kml_string) {
  std::istringstream iss(kml_string);
  AddPolygonsFromStream(iss);
}


/**
 * Polygons constructor helper.
 * Reads in the polygons from an input stream object.
 */
void Polygons::AddPolygonsFromStream(std::istream& in_stream) {
  std::string token;

  // TODO: Use more robust xml parsing.
  while (!in_stream.eof()) {
    token = "";
    in_stream >> token;
    if (!token.empty()) {
      if ("<Polygon>" == token) {
        polygons.push_back(Polygon(&in_stream));
      }
    }
  }

  notify(NFY_NOTICE, "Found %Zu polygons.", polygons.size());
}


/**
 * Grid constructor.
 * Sets the origin for the grid.
 */
Grid::Grid(const double west_origin, const double south_origin)
    : west_origin_(west_origin), south_origin_(south_origin),
    polygons(NULL), polygon(NULL),
    polygon_west_boundary_(west_origin + 360.0),
    polygon_east_boundary_(west_origin),
    polygon_south_boundary_(south_origin + 180.0),
    polygon_north_boundary_(south_origin) {
  // Pre-calculate number of quadtree nodes in each dimension at each level.
  // Dimension size = 2 ^ level
  // Valid levels are 0 through 24
  level_dim_sizes_.resize(MAX_LEVEL + 1);
  for (std::uint32_t level = 0; level <= MAX_LEVEL; ++level) {
    level_dim_sizes_[level] = static_cast<double>(1L << level);
  }
}

/**
 * Sets polygons that defines the region of interest for the grid. Determines
 * the bounding box for the set of polygons.
 */
void Grid::AddPolygons(Polygons* polys) {
  polygons = polys;
  for (size_t i = 0; i < polygons->NumberOfPolygons(); ++i) {
    AddPolygon(polygons->GetPolygon(i));
  }
}

/**
 * Sets polygon that defines the region of interest for the grid. Determines
 * the bounding box for the polygon.
 */
void Grid::AddPolygon(Polygon* poly) {
  polygon = poly;
  std::vector<Vertex>* vertices = polygon->Vertices();
  Vertex* point;

  // Look for the WENS extremes.
  for (size_t j = 0; j < vertices->size(); ++j) {
    point = &(*vertices)[j];
    ConvertToLocal(point);

    if (point->x_ < polygon_west_boundary_) {
      polygon_west_boundary_ = point->x_;
    }
    if (point->x_ > polygon_east_boundary_) {
      polygon_east_boundary_ = point->x_;
    }
    if (point->y_ < polygon_south_boundary_) {
      polygon_south_boundary_ = point->y_;
    }
    if (point->y_ > polygon_north_boundary_) {
      polygon_north_boundary_ = point->y_;
    }
  }
}

/**
 * Returns the area of the grid's polygon. Used to determine which grid
 * (based on its origin) has the smallest bounding box for the polygon.
 */
double Grid::BoundingBoxArea() {
  return (polygon_east_boundary_ - polygon_west_boundary_)
      * (polygon_north_boundary_ - polygon_south_boundary_);
}

/**
 * Calculates all of the nodes in the quadtree at the max level that minimally
 * encompass the polygons.
 */
void Grid::CalculateQuadtreeNodes(int max_level,
                                  bool is_mercator,
                                  std::ostream& out) {
  if (polygons == NULL)
    return;

  for (size_t i = 0; i < polygons->NumberOfPolygons(); ++i) {
    notify(NFY_DEBUG, "Calculate qtnodes for polygon %Zu.", i);
    polygon = polygons->GetPolygon(i);
    CalculateQuadtreeNodesForPolygon(max_level, is_mercator, out);
  }
  polygon = NULL;
}

/**
 * Calculates all of the nodes in the quadtree at the max level that minimally
 * encompass the current polygon.
 *
 * It first determines all of the column crossings at a grid resolution
 * based on the max level (2^level pixels in each dimension). It then adjusts
 * these column crossings to include any vertical spans from the edges.
 * Finally, it recurses from top to bottom (depth first search) to output
 * the reduced quadtree nodes for all quadtree nodes that overlap
 * the polygon. At the max level, the presence of a node in
 * the polygon is decided by overlap of the node square with any of the
 * polygon's column crossings. At intermediate levels, whether to output a
 * quadtree node or to reduce it to parent, depends on whether all 4
 * children are in the polygon or not. If all 4 childrem are in the
 * polygon, the task of outputting the node is left to
 * the parent's parent node, otherwise the child nodes that are in
 * the polygon are output at this level.
 *
 * There is an optimization based on bounding box of polygon and bounding box
 * of quadtree node. If no intersection is there, we avoid recursing down
 * that subtree.
 *
 * TODO: We may want to search the quadtree for the max level,
 * TODO: since this is very wasteful if the max level is set too
 * TODO: high.
 */
void Grid::CalculateQuadtreeNodesForPolygon(int max_level,
                                            bool is_mercator,
                                            std::ostream& out) {
  if (polygon == NULL)
    return;

  max_level_ = max_level;
  is_mercator_ = is_mercator;
  col_spans_.clear();
  col_crossings_.clear();
  dim_size_ = level_dim_sizes_[max_level];
  // x direction only for Mercator
  qtnodes_per_degree_ = dim_size_ / 360.0;

  // Find smallest qtnode that contains the polygon, to use as a starting
  // point.
  std::string base_path;
  int level;
  double south;
  double north;
  double west;
  CalculateSmallestEncompassingQuadtreeNode(
      &base_path, &west, &south, &north, &level, max_level);

  // Calculate row offset for qt node resolution at max level.
  col_offset_ = floor((polygon_west_boundary_ - west_origin_)
      * qtnodes_per_degree_);
  // Set quadtree west boundary to nearest qtnode boundary at max level.
  west_boundary_ = static_cast<double>(col_offset_) / qtnodes_per_degree_
      + west_origin_;

  // Calculate east offset for qt node resolution at max level.
  // Add 1 to move to the east side of the last qt node.
  std::int32_t east_offset = ceil(
      (polygon_east_boundary_ - west_origin_) * qtnodes_per_degree_);
  // Set quadtree east boundary to nearest qtnode boundary at max level.
  east_boundary_ = static_cast<double>(east_offset) / qtnodes_per_degree_
      + west_origin_;

  // Calculate north offset for qt node resolution at max level.
  // Add 1 to move to the north side of the last qt node.
  std::int32_t north_offset = ceil(
      (polygon_north_boundary_ - south_origin_ + 90.0) * qtnodes_per_degree_);
  // Set quadtree north boundary to nearest qtnode boundary at max level.
  north_boundary_ = static_cast<double>(north_offset) / qtnodes_per_degree_
      + south_origin_ - 90.0;

  // Calculate south offset for qt node resolution at max level.
  std::int32_t south_offset = floor(
      (polygon_south_boundary_ - south_origin_ + 90.0) * qtnodes_per_degree_);
  // Set quadtree south boundary to nearest qtnode boundary at max level.
  south_boundary_ = static_cast<double>(south_offset) / qtnodes_per_degree_
      + south_origin_ - 90.0;

  // Calculate the number of columns in the grid.
  int number_of_cols = 1 + ceil(
      (east_boundary_ - west_origin_)
      / 360.0 * static_cast<double>(dim_size_)) - col_offset_;
  col_crossings_.resize(number_of_cols);
  col_spans_.resize(number_of_cols);

  CalculateColCrossings();
  // Sort column crossings by row.
  for (size_t i = 0; i < col_crossings_.size(); ++i) {
    std::sort(col_crossings_[i].begin(), col_crossings_[i].end());
  }

  // Adjust column crossings for vertical spans.
  AddSpansToColCrossings();
  // Done with the column spans.
  col_spans_.clear();

  // Output the reduced quadtree node addresses to the given stream.
  if (OutputQuadtreeNodesOverlappingPolygon(
      base_path, west, south, north, level, max_level, out)) {
    out << base_path << std::endl;
  }
}

/**
 * Outputs quadtree nodes that overlap polygon at the resolution of the
 * polygon_level to the given file. If hi-res quadtree nodes fill a lower
 * resolution quadtree node, then only the lo-res quadtree node is
 * output to help make output as small as possible.
 *
 * Recurses through the quadtree. Maximum depth of the call stack is equal
 * to the max_level for the grid.
 *
 * @return whether all 4 sub-nodes are overlap the polygon.
 */
bool Grid::OutputQuadtreeNodesOverlappingPolygon(const std::string& base_path,
                                                 const double west,
                                                 const double south,
                                                 const double north,
                                                 const int level,
                                                 const int polygon_level,
                                                 std::ostream& out) {
  double level_dim_size = level_dim_sizes_[level];
  double degrees_per_qtnode = 360.0 / level_dim_size;

  // Degrees per quadtree node next level down.
  double degrees_per_child_qtnode = degrees_per_qtnode / 2.0;

  double north_south_midpoint = BisectLatitudes(south, north, is_mercator_);

  // If we are at the level of the polygon, check whether quadtree node
  // is in the polygon.
  if (level == polygon_level) {
    return IsLocationInPolygon(
        west + degrees_per_child_qtnode, north_south_midpoint);
  }

  // Check if there is any possible overlap at all.
  // Otherwise, we will be walking the whole tree for nothing.
  double east = west + degrees_per_qtnode;
  if ((east_boundary_ < west) || (west_boundary_ > east) ||
      (north_boundary_ < south) || (south_boundary_ > north)) {
      return false;
  }

  // Check which of the sub-nodes is in the polygon.
  bool node0 = OutputQuadtreeNodesOverlappingPolygon(
      base_path + '0',
      west,
      south,
      north_south_midpoint,
      level + 1,
      polygon_level,
      out);
  bool node1 = OutputQuadtreeNodesOverlappingPolygon(
      base_path + '1',
      west + degrees_per_child_qtnode,
      south,
      north_south_midpoint,
      level + 1,
      polygon_level,
      out);
  bool node2 = OutputQuadtreeNodesOverlappingPolygon(
      base_path + '2',
      west + degrees_per_child_qtnode,
      north_south_midpoint,
      north,
      level + 1,
      polygon_level,
      out);
  bool node3 = OutputQuadtreeNodesOverlappingPolygon(
      base_path + '3',
      west,
      north_south_midpoint,
      north,
      level + 1,
      polygon_level,
      out);

  // If all are in the polygon, return true to the level above so that it
  // can check if a lower resolution quadtree nodes are full.
  if (node0 && node1 && node2 && node3) {
    return true;
  }

  // If we had to wrap the globe to prevent breaks, then
  // west/east swap the first quadtree address value.
  std::string path;
  if (west_origin_ == 0.0) {
    switch (base_path[0]) {
      case '0':
        path = '1' + base_path.substr(1);
        break;

      case '1':
        path = '0' + base_path.substr(1);
        break;

      case '2':
        path = '3' + base_path.substr(1);
        break;

      case '3':
        path = '2' + base_path.substr(1);
        break;
    }
  } else {
    path = base_path;
  }

  // If one or more is not in the polygon, then output the addresses of the
  // ones that are in the polygon because we are as low as we can go.
  // TODO: Consider outputting std::int64_t qtpaths instead.
  if (node0) {
    out << path + '0' << std::endl;
  }

  if (node1) {
    out << path + '1' << std::endl;
  }

  if (node2) {
    out << path + '2' << std::endl;
  }

  if (node3) {
    out << path + '3' << std::endl;
  }

  // Indicate that the above node is not completely full.
  return false;
}

/**
 * Converts the latitude and longitude to a quadtree column and row and
 * checks if the corresponding quadtree node is in the polygon based on
 * the stored adjusted column crossings.
 */
bool Grid::IsLocationInPolygon(double x, double y) {
  std::int32_t row;
  std::int32_t col;
  ConvertToLevelCoordinates(x, y, &col, &row);

  // Double check, but shouldn't be possible.
  if ((col < 0) || (col >= static_cast<std::int32_t>(col_crossings_.size()))) {
    return false;
  }

  for (size_t i = 0; i < col_crossings_[col].size(); i += 2) {
    if ((col_crossings_[col][i] <= row) &&
        (row <= col_crossings_[col][i + 1])) {
      return true;
    }
  }

  return false;
}

/**
 * Finds the smallest quadtree node that encompasses the entire polygon.
 * Descends the quadtree until it hits a node whose square does not
 * encompass the polygon's bounding box.
 *
 * Returns the south/west corner of the quadtree node, its level, and
 * its quadtree address.
 */
void Grid::CalculateSmallestEncompassingQuadtreeNode(std::string* base_path,
                                                     double* west,
                                                     double* south,
                                                     double* north,
                                                     int* level,
                                                     int max_level) {
  if (is_mercator_) {
    *south = -MAX_MERCATOR_LATITUDE;
    *north = MAX_MERCATOR_LATITUDE;
  } else {
    *south = -180.0;
    *north = 180.0;
  }

  *west = west_origin_;
  char next_qt_node;
  for (*level = 0; *level < max_level; *level += 1) {
    double level_dim_size = level_dim_sizes_[*level];
    double qt_node_size = 180.0 / level_dim_size;
    double north_south_midpoint =
        BisectLatitudes(*south, *north, is_mercator_);

    // Get which sub-node the SW vertex is in.
    if (polygon_south_boundary_ <= north_south_midpoint) {
      if (polygon_west_boundary_ <= *west + qt_node_size) {
        next_qt_node = '0';
      } else {
        next_qt_node = '1';
      }
    } else {
      if (polygon_west_boundary_ <= *west + qt_node_size) {
        next_qt_node = '3';
      } else {
        next_qt_node = '2';
      }
    }

    // Check if NE vertex is in the same sub-node. If
    // not, then break at the node we are at.
    if (polygon_north_boundary_ <= north_south_midpoint) {
      if (polygon_east_boundary_ <= *west + qt_node_size) {
        if (next_qt_node != '0') {
          break;
        }
      } else {
        if (next_qt_node != '1') {
          break;
        }
        *west += qt_node_size;
      }

      *north = north_south_midpoint;
    } else {
      if (polygon_east_boundary_ <= *west + qt_node_size) {
        if (next_qt_node != '3') {
          break;
        }
      } else {
        if (next_qt_node != '2') {
          break;
        }
        *west += qt_node_size;
      }

      *south = north_south_midpoint;
    }

    // If still contained, descend to the next level of the tree.
    (*base_path) += next_qt_node;
  }
}

/**
 * Converts point to lie within defining box for this grid.
 * E.g. -90 to 90 South/North and -180 to 180 West/East.
 */
void Grid::ConvertToLocal(Vertex* point) {
  while (point->y_ < south_origin_) {
    point->y_ += 180.0;
  }

  while (point->y_ >= south_origin_ + 180.0) {
    point->y_ -= 180.0;
  }

  while (point->x_ < west_origin_) {
    point->x_ += 360.0;
  }

  while (point->x_ >= west_origin_ + 360.0) {
    point->x_ -= 360.0;
  }
}

/**
 * Converts vertices to column and row values for the max level.
 * Assumes that we want to round to closest quadtree node at the max level.
 * This is mostly what dictates the max level, as if you pick to low of a
 * max level, rounding error might lead to important areas being left out.
 */
void Grid::ConvertToLevelCoordinates(double x,
                                     double y,
                                     std::int32_t* col,
                                     std::int32_t* row) {
  double y_norm;
  if (is_mercator_) {
    // Get y in radians and convert to degrees.
    y_norm = MercatorLatToY(y) / PI * 180.0;
  } else {
    y_norm = y;
  }
  *row = static_cast<std::int32_t>(floor(
      (y_norm - south_origin_ + 90.0)
      * qtnodes_per_degree_));
  *col = static_cast<std::int32_t>(floor(
      (x - west_boundary_) * qtnodes_per_degree_));
}

/**
 * Calculates all column crossings for the polygon.
 */
void Grid::CalculateColCrossings() {
  // Initialize the vertices for the polygon perimeter.
  std::vector<Vertex>* vertices = polygon->Vertices();
  std::int32_t col;
  std::int32_t row;
  ConvertToLevelCoordinates(
      (*vertices)[0].x_, (*vertices)[0].y_, &col, &row);
  StartPolygon(col, row);

  // Process each vertex in the polygon perimeter.
  last_polygon_point_ = false;
  size_t i;
  for (i = 1; i < vertices->size() - 1; ++i) {
    ConvertToLevelCoordinates(
        (*vertices)[i].x_, (*vertices)[i].y_, &col, &row);
    NextPolygonEdge(col, row);
  }

  // Process last vertex in polygon with flag set.
  last_polygon_point_ = true;
  ConvertToLevelCoordinates(
      (*vertices)[i].x_, (*vertices)[i].y_, &col, &row);
  NextPolygonEdge(col, row);

  // Clean up buffered values.
  EndPolygon();
}

/**
 * Adjusts column crossings so that vertical spans at those column crossings are
 * included.
 */
void Grid::AddSpansToColCrossings() {
  for (size_t i = 0; i < col_crossings_.size(); ++i) {
    if (col_crossings_[i].size() & 1) {
      notify(NFY_FATAL, "Col %d. Number of column crossings should not be odd. "
                        "Polygon may not be legal.", static_cast<int>(i));
    }

    for (size_t j = 0; j < col_crossings_[i].size(); ++j) {
      std::int32_t crossing = col_crossings_[i][j];
      for (size_t k = 0; k < col_spans_[i].size(); ++k) {
        // Look for a crossing touching a span.
        if ((crossing >= col_spans_[i][k].start - 1) &&
            (crossing <= col_spans_[i][k].end + 1)) {
          // If we have a paired crossing to the left, then use end of span.
          if (j & 1) {
            if (col_crossings_[i][j] < col_spans_[i][k].end) {
              col_crossings_[i][j] = col_spans_[i][k].end;
            }
            if (col_crossings_[i][j-1] > col_spans_[i][k].start) {
              col_crossings_[i][j-1] = col_spans_[i][k].start;
            }

          // Otherwise, if we have a paired crossing to the right, then use
          // start of span.
          } else {
            if (col_crossings_[i][j] > col_spans_[i][k].start) {
              col_crossings_[i][j] = col_spans_[i][k].start;
            }
            if (col_crossings_[i][j+1] < col_spans_[i][k].end) {
              col_crossings_[i][j+1] = col_spans_[i][k].end;
            }
          }

          crossing = col_crossings_[i][j];
        }
      }
    }
  }
}

/**
 * Initializes variables for traversing the perimeter of the polygon.
 */
void Grid::StartPolygon(const std::int32_t start_col, const std::int32_t start_row) {
  first_dx_ = 0;
  last_dx_ = 0;

  current_row_ = start_row;
  current_col_ = start_col;
  last_col_ = start_col;
  last_row_ = start_row;

  col_change_ = true;
}

/**
 * Processes next segment of perimeter of the polygon. Uses basic line
 * drawing approach where quadtree nodes at the max level resolution
 * are treated as pixels. In general, whenever the column changes, a
 * column crossing is saved. Horizontal spans are also saved for later
 * use to adjust the column crossings.
 *
 * WARNING: Line drawing is done between qt node vertex "pixels", not
 * absolute coordinates, so intermediate values on the edge might
 * differ slightly from what you would get using absolute coordinates.
 * However, values should never differ by more than one "pixel."
 */
void Grid::NextPolygonEdge(const std::int32_t dest_col, const std::int32_t dest_row) {
  double dx = static_cast<double>(dest_col - current_col_);
  double dy = static_cast<double>(dest_row - current_row_);
  std::int32_t step = 1;

  double x = static_cast<double>(current_col_);
  double y = static_cast<double>(current_row_);

  // Handle vertical lines.
  if (dx == 0) {
    AddColCrossing(current_col_, current_row_);
    AddSpan(dest_col, current_row_, dest_row);
    current_row_ = dest_row;

  // Handle horizontal lines.
  } else if (dy == 0) {
    if (dest_col < current_col_) {
      step = -1;
    }

    while (true) {
      AddColCrossing(current_col_, current_row_);
      if (current_col_ == dest_col) {
        break;
      }

      current_col_ += step;
      col_change_ = true;
    }

  // Handle shallow lines.
  } else if (abs(dx) > abs(dy)) {
    double m = dy / dx;
    if (dest_col < current_col_) {
      m = -m;
      step = -1;
    }

    while (true) {
      AddColCrossing(current_col_, current_row_);
      if (current_col_ == dest_col) {
        break;
      }

      current_col_ += step;
      y += m;

      current_row_ = round(y);
    }

  // Handle steep lines.
  } else {  // abs(dx) <= abs(dy))
    col_change_ = true;
    double m = dx / dy;
    if (dest_row < current_row_) {
      m = -m;
      step = -1;
    }

    std::int32_t last_row0 = current_row_;
    std::int32_t last_col0 = current_col_;
    while (true) {
      AddColCrossing(current_col_, current_row_);
      if (current_row_ == dest_row) {
        break;
      }

      current_row_ += step;
      x += m;
      current_col_ = round(x);
      if (current_col_ != last_col0) {
        // Do not add span of a single row.
        if (last_row0 != current_row_ - step) {
          AddSpan(last_col0, current_row_ - step, last_row0);
        }

        last_row0 = current_row_;
        last_col0 = current_col_;
        col_change_ = true;
      }
    }

    // Always add last span.
    AddSpan(last_col0, current_row_, last_row0);
  }
}

/**
 * Handles the last point, which is dependent on the dx of the
 * first segment of the polygon.
 */
void Grid::EndPolygon() {
  AddColCrossing(current_col_, current_row_);
  if (first_dx_ == 0) {
    col_crossings_[current_col_].push_back(last_row_);
    col_crossings_[current_col_].push_back(current_row_);
  } else if (last_dx_ != first_dx_) {
    col_crossings_[current_col_].push_back(current_row_);
  }
}

/**
 * Show column crossings for debugging.
 */
void Grid::ShowColCrossings() {
  std::cout << "Column crossings:" << std::endl;
  for (size_t i = 0; i < col_crossings_.size(); ++i) {
    std::cout << " column " << i << ": ";
    for (size_t j = 0; j < col_crossings_[i].size(); ++j) {
      std::cout << " " << col_crossings_[i][j];
    }
    std::cout << std::endl;
  }
}

/**
 * Show column spans for debugging.
 */
void Grid::ShowColSpans() {
  std::cout << "Column spans:" << std::endl;
  for (size_t i = 0; i < col_spans_.size(); ++i) {
    std::cout << " column " << i << ": ";
    for (size_t j = 0; j < col_spans_[i].size(); ++j) {
      std::cout << " (" << col_spans_[i][j].start << ","
          << col_spans_[i][j].end << ")" << std::endl;
    }
    std::cout << std::endl;
  }
}

/**
 * Adds a column crossing for the last column and row if the column changed.
 * Keeps track of what the last column and row were.
 */
void Grid::AddColCrossing(const std::int32_t col, const std::int32_t row) {
  // If we have changed columns, mark a crossing in the last column.
  if (last_col_ != col) {
    // If moving in same direction, add row once.
    if ((last_dx_ == col - last_col_) || (last_dx_ == 0)) {
      col_crossings_[last_col_].push_back(last_row_);

      if (last_dx_ == 0) {
        first_dx_ = col - last_col_;
        first_col_ = last_col_;
      }

    // Min or max so enter the row twice to create span.
    } else {
      col_crossings_[last_col_].push_back(last_row_);
      col_crossings_[last_col_].push_back(last_row_);
    }

    // Check if crossing is on the end of a span.
    int idx = col_spans_[last_col_].size() - 1;
    if (idx >= 0) {
      if (col_spans_[last_col_][idx].start - 1 == last_row_) {
        col_spans_[last_col_][idx].start = last_row_;
      } else if (col_spans_[last_col_][idx].end + 1 == last_row_) {
        col_spans_[last_col_][idx].end = last_row_;
      }
    }

    last_dx_ = col - last_col_;
  }

  last_col_ = col;
  last_row_ = row;
}

/**
 * Adds a vertical span. If a span extends the previous span (i.e. because
 * a vertex was reached within a longer span), combines the two spans together.
 */
void Grid::AddSpan(const std::int32_t col, const std::int32_t row0, const std::int32_t row1) {
  Span span;

  if (row0 < row1) {
    span.start = row0;
    span.end = row1;
  } else {
    span.start = row1;
    span.end = row0;
  }

  // If span continues or reverses, extend span rather than creating a new
  // one.
  int idx = col_spans_[col].size() - 1;
  if (!col_change_ && (idx >= 0)) {
    if ((span.start < col_spans_[col][idx].start) &&
        (span.end >= col_spans_[col][idx].start - 1)) {
      col_spans_[col][idx].start = span.start;
    }

    if ((span.start <= col_spans_[col][idx].end + 1) &&
        (span.end > col_spans_[col][idx].end)) {
      col_spans_[col][idx].end = span.end;
    }

  // If last polygon point, see if we can merge with the first one.
  // Make sure that it actually is a span from the first point.
  } else if (last_polygon_point_ && (col_spans_[col].size() > 0) &&
             (col == first_col_) && (col_spans_[col][0].start - 1 <= span.end)
              && (col_spans_[col][0].end + 1 >= span.start)) {
    idx = 0;
    if ((span.start < col_spans_[col][idx].start) &&
        (span.end >= col_spans_[col][idx].start - 1)) {
      col_spans_[col][idx].start = span.start;
    }

    if ((span.start <= col_spans_[col][idx].end + 1) &&
        (span.end > col_spans_[col][idx].end)) {
      col_spans_[col][idx].end = span.end;
    }

  // If new span, add it to the column.
  } else {
    col_spans_[col].push_back(span);
  }

  col_change_ = false;
}

}  // namespace fusion_portableglobe


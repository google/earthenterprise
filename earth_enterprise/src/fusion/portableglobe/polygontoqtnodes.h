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


// Classes for converting a polygon from a kml file into a
// set of quadtree nodes that the polygon encompasses.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_POLYGONTOQTNODES_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_POLYGONTOQTNODES_H_

#include <string>
#include <vector>
#include "common/khTypes.h"
#include <cstdint>

namespace fusion_portableglobe {

/**
 * Latitude and longitude point stored in the -90 to 90 South/North and
 * -180 to 180 West/East grid form. X is used for longitude and y is
 * used for latitude.
 */
class Vertex {
 public:
  Vertex(double x, double y) : x_(x), y_(y) {}
  double x_;
  double y_;
};

/**
 * List of vertices on globe that define a polygon. Polygon is interpreted
 * to be the polygon of the smallest bounding box.
 * All vertices are stored in the -90 to 90 South/North and
 * -180 to 180 West/East grid form.
 *
 * It is ok for two edges to have the same vertices so you can implement
 * a hole by wrapping the polygon around it. We may also want to
 * implement a more robust set of actions at the quadtree node file
 * level that would allow the user to add or subtract quadtree
 * nodes from the set of nodes being selected.
 */
class Polygon {
 public:
  /**
   * Constructor for testing.
   */
  Polygon() {}

  explicit Polygon(std::istream* fin);

  std::vector<Vertex> *Vertices() { return &vertices; }

  size_t NumberOfVertices() { return vertices.size(); }

 private:
  std::vector<Vertex> vertices;
};


/**
 * This is a dummy struct for passing to the Polygons constructor
 * to indicate that the accompanying string is actual KML and not
 * the name of a KML file.
 */
struct kml_as_string {
};


/**
 * Set of Polygons read in from a kml file.
 */
class Polygons {
 public:
  /**
   * Constructor for testing.
   */
  explicit Polygons(const Polygon& polygon) { polygons.push_back(polygon); }

  explicit Polygons(const std::string& kml_file);

  Polygons(const kml_as_string&, const std::string& kml_string);

  Polygon* GetPolygon(size_t index) { return &polygons[index]; }

  size_t NumberOfPolygons() { return polygons.size(); }

  /**
   * Add polygon for testing.
   */
  void AddPolygon(const Polygon& polygon) { polygons.push_back(polygon); }

 private:
  std::vector<Polygon> polygons;

  void AddPolygonsFromStream(std::istream& in_stream);
};

/**
 * Defines a bounding box within a flat globe. The defining box for
 * the globe (not the bounding box) is set during construction.
 * E.g. defaults is -90 to 90 South/North and -180 to 180 West/East,
 * but others are possible to prevent polygon wrap around.
 *
 * Contains methods for determining which quadtree nodes are
 * in the polygon defined within the grid.
 */
class Grid {
 public:
  Grid(const double west_origin, const double south_origin);

  /**
   * Adds the polygons defining the region of interest for this grid.
   */
  void AddPolygons(Polygons* polys);

  /**
   * Adds a polygon defining all or part of the region of interest for this
   * grid.
   */
  void AddPolygon(Polygon* polygon);

  /**
   * Returns the area of the bounding box for the polygon. This is used to
   * determine if this is the best grid (per its origin), for determining
   * encompassed quadtree nodes.
   */
  double BoundingBoxArea();


  /**
   * Calculates the quadtree nodes for each polygon associated with the Grid.
   */
  void CalculateQuadtreeNodes(int max_level,
                              bool is_mercator,
                              std::ostream& out);

  /**
   * Calculates the quadtree nodes at the resolution of the max_level that
   * overlap the grid's current polygon. Outputs the reduced quadtree node
   * addresses to the given stream; i.e. it uses the lowest resolution quadtree
   * node address it can that is correct for all quadtree nodes at the
   * max_level. For example, it might output "3102112" instead of outputting
   * "31021120", "31021121", "31021122", and "31021123", which is equivalent.
   */
  void CalculateQuadtreeNodesForPolygon(int max_level,
                                        bool is_mercator,
                                        std::ostream& out);

  /**
   * Calculates the small quadtree node that encompasses the entire polygon.
   */
  void CalculateSmallestEncompassingQuadtreeNode(std::string* base_path,
                                                 double* west,
                                                 double* south,
                                                 double* north,
                                                 int* level,
                                                 int max_level);

  /**
   * Show row crossings for debugging.
   */
  void ShowColCrossings();

  /**
   * Show row spans for debugging.
   */
  void ShowColSpans();

 private:
  // Origin for grid.
  // This allows the grid to be best positioned to encompass the polygon.
  const double west_origin_;
  const double south_origin_;

  // Polygons defining a regions of interest for the globe.
  Polygons* polygons;

  // Current polygon being processed.
  Polygon* polygon;

  // Whether we are using a mercator projection
  // Otherwise, we assume plate carree.
  bool is_mercator_;

  // Bounding box for the polygons.
  double polygon_west_boundary_;
  double polygon_east_boundary_;
  double polygon_south_boundary_;
  double polygon_north_boundary_;

  // Bounding box for the polygon on quadtree borders at max level.
  double west_boundary_;
  double east_boundary_;
  double south_boundary_;
  double north_boundary_;

  // Values associated with the max_level, which is the resolution at
  // which quadtree nodes are to be defined at.

  // Max level itself.
  std::int32_t max_level_;
  // Number of quadtree nodes in horizontal and vertical directions.
  double dim_size_;
  // Number of quadtree nodes per degree of latitude or longitude.
  double qtnodes_per_degree_;
  // Offset to polygon bounding box starting column.
  std::int32_t col_offset_;

  // Values associated with determining the nodes that the polygon
  // overlaps.

  // Previous row where an overlapping quadtree node was detected.
  std::int32_t last_row_;
  // Previous column where an overlapping quadtree node was detected.
  std::int32_t last_col_;

  // Current row where an overlapping quadtree node is being processed.
  std::int32_t current_row_;
  // Current column where an overlapping quadtree node is being processed.
  std::int32_t current_col_;

  // Flag indicating that algorithm has moved to a new column.
  bool col_change_;
  // Flag indicating that the last point in the polygon is being processed.
  bool last_polygon_point_;

  // At lowest level use x and y, where x = col and y = row

  // Indicates direction of last column change.
  int last_dx_;
  // Indicates direction of first column change.
  int first_dx_;
  // Column of first vertex of the polygon.
  int first_col_;

  // Struct used to store vertical spans of quadtree nodes.
  struct Span {
    std::int32_t start;
    std::int32_t end;
  };

  // Vector indexed by column giving a vector of rows where
  // the polygon perimeter crosses that column.
  std::vector<std::vector<std::int32_t> > col_crossings_;

  // Because crossings are specified by a single row value, we
  // must also keep track of vertical spans of quadtree nodes within a
  // row for the perimeter of a polygon. There is more complexity
  // because of the spans. The code could be simplified by keeping
  // the entire grid in memory and filling perimeter pixels as you
  // go and then using crossings to fill in the area. However, this
  // will not scale as well at high-resolution and/or over large
  // areas.
  std::vector<std::vector<Span> > col_spans_;

  // Lookup table for quadtree node dimensions associated with each level.
  // Keep reusing, so calculate these at the beginning.
  std::vector<double> level_dim_sizes_;

  /**
   * Converts point from -90 to 90 South/North and -180 to 180 West/East
   * coordinates to coordinates using the grid's origin.
   */
  void ConvertToLocal(Vertex* point);

  /**
   * Converts point to row and column for quadtree node grid at this
   * grid's max_level.
   */
  void ConvertToLevelCoordinates(double x,
                                 double y,
                                 std::int32_t* col,
                                 std::int32_t* row);

  /**
   * Calculates all of the column crossings for the polygon perimeter.
   */
  void CalculateColCrossings();

  /**
   * Calculates vertical column spans for the polygon perimeter.
   */
  void AddSpansToColCrossings();

  /**
   * Outputs to the given output stream the reduced quadtree node addresses
   * for all quadtree nodes that overlap with the grid's polygon.
   * Parameters give the southwest corner of the current quadtree node,
   * the level it is at, and its quadtree path. The max_level parameter
   * gives the level of max resolution for the quadtree nodes.
   *
   * @returns whether all 4 sub-nodes overlap the polygon.
   */
  bool OutputQuadtreeNodesOverlappingPolygon(const std::string& base_path,
                                             const double west,
                                             const double south,
                                             const double north,
                                             const int level,
                                             const int max_level,
                                             std::ostream& out);

  /**
   * Returns whether the given longitude (x) and latitude (y) are within the
   * polygon.
   */
  bool IsLocationInPolygon(double x, double y);

  /**
   * Initializes parameters for traversing the perimeter of the polygon to
   * determine column crossings.
   */
  void StartPolygon(const std::int32_t col, const std::int32_t row);

  /**
   * Processes next perimeter edge while traversing the perimeter of the
   * polygon to determine column crossings. It is assumed that StartPolygon
   * will be called before this called 2 or more times. EndPolygon should
   * be called after the final edge.
   */
  void NextPolygonEdge(const std::int32_t dest_col, const std::int32_t dest_row);

  /**
   * Does final processing for traversing the perimeter of the polygon to
   * determine column crossings.
   */
  void EndPolygon();

  /**
   * Adds a column crossing for the given column at the given row.
   */
  void AddColCrossing(const std::int32_t col, const std::int32_t row);

  /**
   * Adds a vertical span for the given column between the two
   * given rows inclusive.
   */
  void AddSpan(const std::int32_t col, const std::int32_t row0, const std::int32_t row1);
};

}  // namespace fusion_portableglobe

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_POLYGONTOQTNODES_H_


/*
 * Copyright 2017 Google Inc.
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
// File:        gstPolygonClipper.h
// Description: Clips polygons using Liang-Barsky polygon clipper

#ifndef KHSRC_FUSION_GST_POLYGON_CLIPPER_H__
#define KHSRC_FUSION_GST_POLYGON_CLIPPER_H__

#include <gstVertex.h>
#include <common/base/macros.h>
#include <deque>
#include <vector>

enum pcQuadEdgeType {
  QUAD_NONE = 0,
  QUAD_LEFT = 1,
  QUAD_RIGHT = 2,
  QUAD_BOTTOM = 3,
  QUAD_TOP = 4
};

class pcVertex {
 public:
  double x, y, z;
  bool internal_vertex;

  pcVertex(double t_x, double t_y, double t_z, bool t_internal_vertex) :
      x(t_x),
      y(t_y),
      z(t_z),
      internal_vertex(t_internal_vertex) {}

  bool isInternal() const { return internal_vertex; }

  bool operator==(const pcVertex& v) {
    return ((x == v.x) && (y == v.y));
  }

  bool operator!=(const pcVertex& v) {
    return ((x != v.x) || (y != v.y));
  }
};

class pcEdge {
 public:
  int start, end;
  bool internal_edge;
  pcQuadEdgeType quad_edge_type;

  pcEdge(int t_start, int t_end, bool t_internal_edge,
         pcQuadEdgeType t_quad_edge_type) :
      start(t_start),
      end(t_end),
      internal_edge(t_internal_edge),
      quad_edge_type(t_quad_edge_type) {}
};

typedef std::vector<pcEdge> EdgeList;
typedef std::vector<pcEdge>::iterator EdgeListIterator;

class pcPolygon {
 public:
  std::vector<pcVertex> vertex_list;
  EdgeList edge_list;
  std::vector<bool> internal_edge;

  pcPolygon() {}

  pcPolygon(int num_verts, double* x, double* y) {
    vertex_list.reserve(num_verts);
    for (int i = 0; i < num_verts; ++i) {
      vertex_list.push_back(pcVertex(x[i], y[i], 0, false));
    }
  }

  void AddVertex(const pcVertex& v) {
    vertex_list.push_back(v);
  }
  void AddVertex(const gstVertex& vert) {
    vertex_list.push_back(pcVertex(vert.x, vert.y, 0, false));
  }
  void Reserve(int sz) {
    vertex_list.reserve(sz);
  }

  void ClipPolygon(pcPolygon* op,
                   double wx1, double wx2,
                   double wy1, double wy2);

 private:
  void RemoveDegenerateEdges(double wx1, double wx2, double wy1, double wy2);
  void ReArrangeEdges(int* edge_bucket, pcQuadEdgeType type);
};

#endif  // !KHSRC_FUSION_GST_POLYGON_CLIPPER_H__

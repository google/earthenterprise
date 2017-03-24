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

//
// File:        gstPolygonClipper.cpp
// Description: Clips polygons using Liang-Barsky polygon clipper


#include "gstPolygonClipper.h"
#include <stdlib.h>
#include <gstJobStats.h>
#include <list>

#define ALMOST_ZERO             (1.0e-30)
#define EPSILON                 (1.0e-10)

struct Elem {
  int index;
  double value;
};

int compareElem(const void* a, const void* b) {
  double x = reinterpret_cast<const Elem*>(a)->value;
  double y = reinterpret_cast<const Elem*>(b)->value;

  if (x < y)
    return -1;
  else if (x > y)
    return 1;
  else
    return 0;
}

pcQuadEdgeType GetEdgeType(const pcVertex& v1,
                           const pcVertex& v2,
                           double wx1, double wx2,
                           double wy1, double wy2) {
  pcQuadEdgeType type;
  if ((v1.x == v2.x) && (v1.x == wx1))
    type = QUAD_LEFT;
  else if ((v1.x == v2.x) && (v1.x == wx2))
    type = QUAD_RIGHT;
  else if ((v1.y == v2.y) && (v1.y == wy1))
    type = QUAD_BOTTOM;
  else if ((v1.y == v2.y) && (v1.y == wy2))
    type = QUAD_TOP;
  else                                    // edge is not internal
    type = QUAD_NONE;

  return type;
}

void pcPolygon::ReArrangeEdges(int* edge_bucket, pcQuadEdgeType type) {
  int num_verts = edge_bucket[type] * 2;
  int vert_index = 0;
  Elem* vert_list = new Elem[num_verts];
  EdgeListIterator iE = edge_list.end();
  do {
    --iE;
    if (iE->quad_edge_type == type) {
      vert_list[vert_index].index = iE->start;
      vert_list[vert_index++].value = (type == QUAD_LEFT || type == QUAD_RIGHT)
          ? vertex_list[iE->start].y : vertex_list[iE->start].x;
      vert_list[vert_index].index = iE->end;
      vert_list[vert_index++].value = (type == QUAD_LEFT || type == QUAD_RIGHT)
          ? vertex_list[iE->end].y : vertex_list[iE->end].x;
      edge_list.erase(iE);
    }
  } while (iE != edge_list.begin());

  qsort(vert_list, num_verts, sizeof(Elem), compareElem);
  for (int i = 0; i < num_verts; i += 2) {
    // check for single vertex edges
    if (vert_list[i].value != vert_list[i+1].value)
      edge_list.push_back(pcEdge(vert_list[i].index, vert_list[i+1].index,
                                 true, type));
  }

  delete[] vert_list;
}

#ifdef JOBSTATS_ENABLED
enum {
  JOBSTATS_CLIP,
  JOBSTATS_CLIP_P0, JOBSTATS_CLIP_P1, JOBSTATS_CLIP_P2,
  JOBSTATS_CLEANUP, JOBSTATS_DEGENERATE,
  JOBSTATS_DEG_P0, JOBSTATS_DEG_P1, JOBSTATS_DEG_P2,
  JOBSTATS_DEG_P3, JOBSTATS_DEG_P4
};

static gstJobStats::JobName JobNames[] = {
  {JOBSTATS_CLIP,          "Clip        "},
  {JOBSTATS_CLIP_P0,       "+ P0        "},
  {JOBSTATS_CLIP_P1,       "+ P1        "},
  {JOBSTATS_CLIP_P2,       "+ P2        "},
  {JOBSTATS_CLEANUP,       "Cleanup     "},
  {JOBSTATS_DEGENERATE,    "Degenerate  "},
  {JOBSTATS_DEG_P0,        "+ Deg P0    "},
  {JOBSTATS_DEG_P1,        "+ Deg P1    "},
  {JOBSTATS_DEG_P2,        "+ Deg P2    "},
  {JOBSTATS_DEG_P3,        "+ Deg P3    "},
  {JOBSTATS_DEG_P4,        "+ Deg P4    "}
};
gstJobStats* clip_stats = new gstJobStats("Clip Polygon", JobNames, 11);
#endif

void pcPolygon::RemoveDegenerateEdges(double wx1, double wx2,
                                      double wy1, double wy2) {
  JOBSTATS_SCOPED(clip_stats, JOBSTATS_DEGENERATE);
  // remove consecutive duplicate vertices
  std::vector<pcVertex>::iterator iV = vertex_list.end();
  --iV;
  while (iV != vertex_list.begin()) {
    if (*iV == *(iV-1)) {
      vertex_list.erase(iV);
    }
    --iV;
  }

  JOBSTATS_BEGIN(clip_stats, JOBSTATS_DEG_P0);
  bool isClipped = false;
  int edge_bucket[5] = {0};
  for (int i = 0; i < static_cast<int>(vertex_list.size()) - 1; ++i) {
    if (vertex_list[i].isInternal() && vertex_list[i + 1].isInternal()) {
      isClipped = true;
      // both vertices are generated because of clipping
      // now lets check if the edge is actually internal
      pcQuadEdgeType type = GetEdgeType(vertex_list[i], vertex_list[i + 1],
                                        wx1, wx2, wy1, wy2);
      if (type == QUAD_NONE) {
        internal_edge.push_back(false);
        edge_list.push_back(pcEdge(i, i + 1, false, QUAD_NONE));
      } else {
        edge_bucket[type]++;
        edge_list.push_back(pcEdge(i, i + 1, true, type));
        internal_edge.push_back(true);
      }
    } else {
      // external edge
      internal_edge.push_back(false);
      edge_list.push_back(pcEdge(i, i + 1, false, QUAD_NONE));
    }
  }
  internal_edge.push_back(false);
  JOBSTATS_END(clip_stats, JOBSTATS_DEG_P0);

  // if the polygon is not clipped then replicate the
  // last vertex and return. this is required by boxcut.
  if (!isClipped) {
    vertex_list.push_back(vertex_list[vertex_list.size() - 1]);
    internal_edge.push_back(false);
    return;
  }

  // if there is <=1 edges clipped no need to re-arrange them
  // just replicate and return
  if ((edge_bucket[QUAD_LEFT] <= 1) &&
      (edge_bucket[QUAD_RIGHT] <= 1) &&
      (edge_bucket[QUAD_BOTTOM] <= 1) &&
      (edge_bucket[QUAD_TOP] <= 1)) {
    vertex_list.push_back(vertex_list[vertex_list.size() - 1]);
    internal_edge.push_back(false);
    return;
  }


  // clear the internal edge flags
  // coz we are going to rearrange everything
  internal_edge.clear();

  // Re-arrange edges that intersect with the quad boundaries
  for (int i = 1; i <= 4; ++i) {
    if (edge_bucket[i] > 1)
      ReArrangeEdges(edge_bucket, pcQuadEdgeType(i));
  }

  JOBSTATS_BEGIN(clip_stats, JOBSTATS_DEG_P1);

  // make a copy of the edge list as a std::list to facilitate
  // the random erase operation that is used to join the edges
  std::list<pcEdge> edges;
  for (EdgeListIterator it = edge_list.begin(); it != edge_list.end(); ++it) {
    edges.push_back(*it);
  }

  // Create output vertices
  // look for closed components
  std::vector<pcVertex> final_list;

  while (edges.size() != 0) {
    // take the first edge and add it to our final list
    std::list<pcEdge>::iterator edge = edges.begin();
    pcVertex first_vertex = vertex_list[edge->start];
    pcVertex last_vertex = vertex_list[edge->end];
    final_list.push_back(first_vertex);
    final_list.push_back(last_vertex);
    internal_edge.push_back(edge->internal_edge);
    edges.erase(edge);

    // now look for any edge that touches the end
    while (last_vertex != first_vertex) {
      JOBSTATS_SCOPED(clip_stats, JOBSTATS_DEG_P2);
      for (edge = edges.begin(); edge != edges.end(); /* do nothing */) {
        if (vertex_list[edge->start] == last_vertex) {
          JOBSTATS_SCOPED(clip_stats, JOBSTATS_DEG_P3);
          last_vertex = vertex_list[edge->end];
          final_list.push_back(last_vertex);
          internal_edge.push_back(edge->internal_edge);
          edge = edges.erase(edge);
        } else if (vertex_list[edge->end] == last_vertex) {
          JOBSTATS_SCOPED(clip_stats, JOBSTATS_DEG_P4);
          last_vertex = vertex_list[edge->start];
          final_list.push_back(last_vertex);
          internal_edge.push_back(edge->internal_edge);
          edge = edges.erase(edge);
        } else {
          ++edge;
        }
      }
    }
    final_list.push_back(last_vertex);
    internal_edge.push_back(false);
    internal_edge.push_back(false);
  }

  vertex_list = final_list;
  JOBSTATS_END(clip_stats, JOBSTATS_DEG_P1);
}

void pcPolygon::ClipPolygon(pcPolygon* op,
                            double wx1, double wx2,
                            double wy1, double wy2) {
  double deltax, deltay, xin, xout, yin, yout;
  double tinx, tiny, toutx, touty, tin1, tin2, tout1;
  double x1, y1, x2, y2;

  JOBSTATS_SCOPED(clip_stats, JOBSTATS_CLIP);

  if (vertex_list[0] != vertex_list[vertex_list.size() - 1]) {
    vertex_list.push_back(vertex_list[0]);
  }

  for (int i = 0; i < static_cast<int>(vertex_list.size()) - 1; ++i) {
    x1 = vertex_list[i].x;
    y1 = vertex_list[i].y;
    x2 = vertex_list[i+1].x;
    y2 = vertex_list[i+1].y;

    x1 = ((x1 == wx1) || (x1 == wx2)) ? x1 + EPSILON : x1;
    x2 = ((x2 == wx1) || (x2 == wx2)) ? x2 + EPSILON : x2;
    y1 = ((y1 == wy1) || (y1 == wy2)) ? y1 + EPSILON : y1;
    y2 = ((y2 == wy1) || (y2 == wy2)) ? y2 + EPSILON : y2;

    deltax = x2-x1;
    if (deltax == 0) { /* bump off of the vertical */
      deltax = (x1 > wx1) ? -ALMOST_ZERO : ALMOST_ZERO ;
    }
    deltay = y2-y1;
    if (deltay == 0) { /* bump off of the horizontal */
      deltay = (y1 > wy1) ? -ALMOST_ZERO : ALMOST_ZERO ;
    }

    if (deltax > 0) {               /*  points to right */
      xin = wx1;
      xout = wx2;
    } else {
      xin = wx2;
      xout = wx1;
    }

    if (deltay > 0) {               /*  points up */
      yin = wy1;
      yout = wy2;
    } else {
      yin = wy2;
      yout = wy1;
    }

    tinx = (xin - x1) / deltax;
    tiny = (yin - y1) / deltay;

    if (tinx < tiny) {              /* hits x first */
      tin1 = tinx;
      tin2 = tiny;
    } else {                        /* hits y first */
      tin1 = tiny;
      tin2 = tinx;
    }

    if (1 >= tin1) {
      JOBSTATS_SCOPED(clip_stats, JOBSTATS_CLIP_P0);
      if (0 < tin1) {
        op->AddVertex(pcVertex(xin, yin, 0, true));
      }
      if (1 >= tin2) {
        JOBSTATS_SCOPED(clip_stats, JOBSTATS_CLIP_P1);
        toutx = (xout - x1)/deltax;
        touty = (yout - y1)/deltay;

        tout1 = (toutx < touty) ? toutx : touty ;

        if (0 < tin2 || 0 < tout1) {
          JOBSTATS_SCOPED(clip_stats, JOBSTATS_CLIP_P2);
          if (tin2 <= tout1) {
            if (0 < tin2) {
              if (tinx > tiny) {
                op->AddVertex(pcVertex(xin, y1+tinx*deltay, 0, true));
              } else {
                op->AddVertex(pcVertex(x1 + tiny*deltax, yin, 0, true));
              }
            }
            if (1 > tout1) {
              if (toutx < touty) {
                op->AddVertex(pcVertex(xout, y1+toutx*deltay, 0, true));
              } else {
                op->AddVertex(pcVertex(x1 + touty*deltax, yout, 0, true));
              }
            } else {
              op->AddVertex(pcVertex(x2, y2, 0, false));
            }
          } else {
            if (tinx > tiny) {
              op->AddVertex(pcVertex(xin, yout, 0, true));
            } else {
              op->AddVertex(pcVertex(xout, yin, 0, true));
            }
          }
        }
      }
    }
  }
  JOBSTATS_BEGIN(clip_stats, JOBSTATS_CLEANUP);
  if (op->vertex_list.size() > 0) {
    op->AddVertex(op->vertex_list[0]);
    op->RemoveDegenerateEdges(wx1, wx2, wy1, wy2);
  }
  JOBSTATS_END(clip_stats, JOBSTATS_CLEANUP);
}

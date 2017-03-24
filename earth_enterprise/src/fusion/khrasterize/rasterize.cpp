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

#include "rasterize.h"
#include <stdio.h>


using namespace std;

// Global declarations

bool g_verbose = true;

typedef vector<VPoints> VVPoints;

struct InternalGrid {
  double          xmin;
  double          ymin;
  double          resolution;
  int             rows;
  int             cols;
  VVPoints        points;
};




void ComputeXYBounds(VPoints& points,
                     double resolution,
                     InternalGrid& internalGrid)
{
  double xmin, xmax, ymin, ymax;

  xmin = ymin = INFINITY;
  xmax = ymax = -INFINITY;
  int numPoints = points.size();
  int i;
  for(i=0; i<numPoints; i++) {
    if (points[i].x < xmin) {
      xmin = points[i].x;
    }
    if (points[i].y < ymin) {
      ymin = points[i].y;
    }
    if (points[i].x > xmax) {
      xmax = points[i].x;
    }
    if (points[i].y > ymax) {
      ymax = points[i].y;
    }
  }
  int rows = (int)ceil(double(ymax - ymin)/resolution + 1);
  int cols = (int)ceil(double(xmax - xmin)/resolution + 1);
  internalGrid.xmin = xmin;
  internalGrid.ymin = ymin;
  internalGrid.resolution = resolution;
  internalGrid.rows = rows;
  internalGrid.cols = cols;

  return;
}




InternalGrid CreateXYMesh( VPoints      points,
                           double       resolution)
{
  InternalGrid internalGrid;
  ComputeXYBounds(points, resolution, internalGrid);
  if (g_verbose) {
    fprintf(stdout, "Rows: %d\n", internalGrid.rows);
    fprintf(stdout, "Cols: %d\n", internalGrid.cols);
  }
  int i, j;
  for (i=0; i<internalGrid.rows; i++) {
    VPoints row;
    for (j=0; j<internalGrid.cols; j++) {
      Point aPoint = {internalGrid.xmin + j*resolution, internalGrid.ymin + i*resolution, 0};
      row.push_back(aPoint);
    }
    internalGrid.points.push_back(row);
  }
  return internalGrid;
}



inline double Compute2DDistance(Point a, Point b)
{
  return sqrt((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y));
}



int FindNearestPoint(VPoints& points,
                     Point aPoint)
{
  int index=0;
  double minDistance = INFINITY;
  int numPoints = points.size();

  int i;
  for (i=0; i<numPoints; i++) {
    double distance = Compute2DDistance(points[i], aPoint);
    if (distance < minDistance) {
      minDistance = distance;
      index = i;
    }
  }

  return index;
}



double FindNeighboringGridPoint(InternalGrid& internalGrid,
                                VVDouble& minDistanceGrid,
                                int row,
                                int col,
                                int radius )
{
  int i, j;
  double minDist=INFINITY;
  double distance;
  double minZ = KH_MISSING_ZVALUE;
  for (i=row-(radius-1); i<row+radius; i++) {
    if (i<0 || i>=internalGrid.rows) {
      continue;
    }
    for (j=col-(radius-1); j<col+radius; j++) {
      if (j<0 || j>=internalGrid.cols) {
        continue;
      }
      if (minDistanceGrid[i][j] < INFINITY) {
        distance = i*i + j*j;
        if (distance < minDist) {
          minZ = internalGrid.points[i][j].z;
        }
      }
    }
  }

  return minZ;
}



void RasterizeNearest(VPoints&      points,
                      InternalGrid& internalGrid,
                      int           fillRadius=3)
{
  VVDouble minDistanceGrid;
  // initalize the minimum distance grid
  int i, j;
  VDouble row;
  row.resize(internalGrid.cols*sizeof(double));
  for (j=0; j<internalGrid.cols; j++) {
    row[j] = INFINITY;
  }
  for (i=0; i<internalGrid.rows; i++) {
    minDistanceGrid.push_back(row);
  }

  int numPoints = points.size();
  double xd, yd;
  int cx, cy, fx, fy;
  for (i=0; i<numPoints; i++) {
    // find the index of the 4 grid points
    // surrounding the point
    xd = (points[i].x - internalGrid.xmin) / internalGrid.resolution;
    cx = (int)ceil(xd);
    fx = (int)floor(xd);

    yd = (points[i].y - internalGrid.ymin) / internalGrid.resolution;
    cy = (int)ceil(yd);
    fy = (int)floor(yd);

    // this point will affect all 4 points surrounding it
    int xarr[] = {fx, cx, fx, cx};
    int yarr[] = {fy, fy, cy, cy};
    int k;
    int xi, yi;
    for(k=0; k<4; k++) {
      xi = xarr[k];
      yi = yarr[k];
      double distance = Compute2DDistance(points[i], internalGrid.points[yi][xi]);
      if (distance < minDistanceGrid[yi][xi]) {
        minDistanceGrid[yi][xi] = distance;
        internalGrid.points[yi][xi].z = points[i].z;
      }
    }
  }

  int ip=0;
  const int progStep = 10;
  double dp=0;
  if (g_verbose) {
    fprintf(stdout, "0");
  }
  /*
    for (i=0; i<internalGrid.rows; i++) {
    for (j=0; j<internalGrid.cols; j++) {
    if (minDistanceGrid[i][j] < INFINITY) {
    continue;
    }
    int index = FindNearestPoint(points, internalGrid.points[i][j]);
    internalGrid.points[i][j].z = points[index].z;
    }
    if (g_verbose) {
    dp += 100.0/double(internalGrid.rows-1);
    if ((int(dp) % progStep == 0) && int(dp) != ip) {
    ip += progStep;
    fprintf(stdout, "..%d", ip);
    }
    }
    }
  */
  for (i=0; i<internalGrid.rows; i++) {
    for (j=0; j<internalGrid.cols; j++) {
      if (minDistanceGrid[i][j] < INFINITY) {
        continue;
      }
      internalGrid.points[i][j].z = FindNeighboringGridPoint(internalGrid, minDistanceGrid, i, j, fillRadius);
    }
    if (g_verbose) {
      dp += 100.0/double(internalGrid.rows-1);
      if ((int(dp) % progStep == 0) && int(dp) != ip) {
        ip += progStep;
        fprintf(stdout, "..%d", ip);
      }
    }
  }
  fprintf(stdout, "\n");
  return;
}



void RasterizeLinear(VPoints&      points,
                     InternalGrid& internalGrid)
{
  fprintf(stderr, "Notice: Only Nearest Neighbor Interpolation implemented so far\n");
  RasterizeNearest(points, internalGrid);

  return;
}



Grid Rasterize( VPoints                         points,
                double                          resolution,
                int                             fillRadius,
                int                             method,
                bool                            verbose)
{
  Grid grid;
  int numPoints = points.size();
  if (numPoints <= 3) {
    fprintf(stderr, "Error: Insufficient points\n");
    return grid;
  }
  if (resolution <= 0) {
    fprintf(stderr, "Error: Bad resolution\n");
    return grid;
  }

  g_verbose = verbose;
  // Create the XY mesh
  InternalGrid internalGrid = CreateXYMesh(points, resolution);

  // Now compute z values based on one of the
  // following methods
  switch (method) {
    case INTP_NEAREST:
      RasterizeNearest(points, internalGrid, fillRadius);
      break;
    case INTP_LINEAR:
      RasterizeLinear(points, internalGrid);
      break;
  }

  grid.rows = internalGrid.rows;
  grid.cols = internalGrid.cols;
  grid.xmin = internalGrid.xmin;
  grid.ymin = internalGrid.ymin;
  grid.resolution = internalGrid.resolution;
  int i,j;
  for (i=0; i<internalGrid.rows; i++) {
    VDouble row;
    for (j=0; j<internalGrid.cols; j++) {
      row.push_back(internalGrid.points[i][j].z);
    }
    grid.z.push_back(row);
  }

  return grid;
}

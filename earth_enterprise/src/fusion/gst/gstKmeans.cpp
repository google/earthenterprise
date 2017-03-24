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


/************************************************************************
 File:           kmeans.cpp
 Description:    This the implementation of the popular k-means algorithm.
                 It accepts a number of points (x,y pairs) and returns
                 an array defining the groups.
*************************************************************************/

#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cmath>

#include "gstKmeans.h"

// Global state variables
static bool g_strict    = false;                // set for being strict at getting K centroids
static bool g_verbose   = true;                 // set for displaying progress

#define CONVERGENCE_THRESHOLD   1e-10
#define MAX_ITERATIONS  20

// Compute the minimum of a number of points
Point Min(Point *points,
          int numPoints)
{
  int i;
  Point min = {INFINITY, INFINITY};
  for (i=0; i<numPoints; i++) {
    if (points[i].x < min.x) {
      min.x = points[i].x;
    }
    if (points[i].y < min.y) {
      min.y = points[i].y;
    }
  }
  return min;
}


// Compute the maximum of a number of points
Point Max(Point *points,
          int numPoints)
{
  int i;
  Point max = {-INFINITY, -INFINITY};
  for (i=0; i<numPoints; i++) {
    if (points[i].x > max.x) {
      max.x = points[i].x;
    }
    if (points[i].y > max.y) {
      max.y = points[i].y;
    }
  }
  return max;
}


// Returns a random point from a set of points
Point SelectRandomPoint(Point* points,
                        int numPoints)
{
  Point aPoint;
  int     randNum = int(double(rand())*(numPoints-1)/RAND_MAX);
  aPoint = points[randNum];
  return aPoint;
}


// Initializes the centers to random points from the entire set
Point* ChooseInitCenters(Point* points,
                         int numPoints,
                         int numCenters)
{
  Point *centers = new Point[numCenters];
  srand((unsigned)time(0));
  int i;
  for (i=0; i<numCenters; i++) {
    centers[i] = SelectRandomPoint(points, numPoints);
  }
  return centers;
}



// Computes euclidean distance between 2 points
double ComputeDistance(Point a,
                       Point b)
{
  double distance = sqrt((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y));
  return distance;
}



// This is the main iteration loop. It finds, for every point, the closest
// center and assigns the point to that center
Point* Iterate(Point *points,
               int numPoints,
               Point *centers,
               int numCenters,
               int *closestCenters)
{
  if (points == 0 || centers == 0) {
    return 0;
  }
  int i, j;
  for (i=0; i<numPoints; i++) {
    double minDistance = INFINITY;
    for (j=0; j<numCenters; j++) {
      if (centers[j].x == -INFINITY) {
        continue;
      }
      double distance = ComputeDistance(points[i], centers[j]);
      if (distance < minDistance) {
        minDistance = distance;
        closestCenters[i] = j;
      }
    }
  }

  Point *newCenters = new Point[numCenters];
  for (j=0; j<numCenters; j++) {
    newCenters[j].x = -INFINITY;
    newCenters[j].y = -INFINITY;
    Point thisCenter = {0, 0};
    int thisCount = 0;
    for (i=0; i<numPoints; i++) {
      if (closestCenters[i] == j) {
        thisCenter.x += points[i].x;
        thisCenter.y += points[i].y;
        thisCount++;
      }
    }
    if (thisCount > 0) {
      newCenters[j].x = thisCenter.x/double(thisCount);
      newCenters[j].y = thisCenter.y/double(thisCount);
    }
    else if (g_strict == true) {
      // reallocate the unassigned centers to some random points
      newCenters[j] = SelectRandomPoint(points, numPoints);
    }

  }
  return newCenters;
}



// Computes how much did each center move and reports the
// max movement.
double ComputeMaxMovement(Point *oldCenters,
                          Point *newCenters,
                          int numCenters)
{
  int i;
  double maxDistance = -INFINITY;
  for (i=0; i<numCenters; i++) {
    if (oldCenters[i].x == -INFINITY || newCenters[i].x == -INFINITY) {
      continue;
    }
    double distance = ComputeDistance(oldCenters[i], newCenters[i]);
    if (maxDistance < distance) {
      maxDistance = distance;
    }
  }
  return maxDistance;
}



// This the main Kmeans function. It accepts a number of
// points and a value for K and returns back an array
// equal to the size of the points array. Each entry in this
// array indicates which of the K groups this point belongs.
int* Kmeans(Point* points,
            int numPoints,
            int K)
{
  if (points == 0) {
    return 0;
  }
  Point *centers = ChooseInitCenters(points, numPoints, K);
  int *closestCenters = new int[numPoints];
  Point *newCenters = Iterate(points, numPoints, centers, K, closestCenters);
  double maxDistance = ComputeMaxMovement(centers, newCenters, K);
  int iteration = 1;
  while ( (maxDistance > CONVERGENCE_THRESHOLD) &&
          (iteration < MAX_ITERATIONS) ) {
    delete[] centers;
    centers = newCenters;
    newCenters = Iterate(points, numPoints, centers, K, closestCenters);
    maxDistance = ComputeMaxMovement(centers, newCenters, K);
    iteration++;
    if (g_verbose) {
      fprintf(stdout, "Iteration: %d, MaxDistance: %4.10f\n", iteration, maxDistance);
    }
  }
  return closestCenters;
}



// This function picks a point from a group such that the
// point is closest to the centroid. This function is used
// by the Kmeans wrapper function.
int FindCentroid(Point *points,
                 int numPoints,
                 int *classes,
                 int thisClass)
{
  int i;
  Point center = {0, 0};
  int count = 0;
  for (i=0; i<numPoints; i++) {
    if (classes[i] == thisClass) {
      center.x += points[i].x;
      center.y += points[i].y;
      count++;
    }
  }
  if (0 == count) {
    return -1;
  }
  center.x = center.x/double(count);
  center.y = center.y/double(count);
  double minDistance=INFINITY, distance;
  int minIndex = -1;
  for (i=0; i<numPoints; i++) {
    if (classes[i] == thisClass) {
      distance = ComputeDistance(points[i], center);
      if (distance < minDistance) {
        minDistance = distance;
        minIndex = i;
      }
    }
  }
  if (minIndex >= 0) {
    return minIndex;
  }
  return minIndex;
}



// This is a wrapper function that accepts x, y in vectors along
// with a compression ratio. It computes appropriate value for K
// from the compressionRatio and runs the Kmeans function. It
// returns a vector of integers indicating which of the <=K points
// have been picked for representation.
std::vector<int> Kmeans(const std::vector<double>& x,
                   const std::vector<double>& y,
                   double compressionRatio,
                   bool strict,
                   bool verbose)
{
  // Set the global state variables
  g_verbose       = verbose;
  g_strict        = strict;

  // Create the output vector
  std::vector<int> levels;

  int numPoints = x.size();
  if (numPoints == 0) {
    fprintf(stderr, "Error: 0 point features\n");
    return levels;
  }
  if (compressionRatio < 0 || compressionRatio > 1) {
    fprintf(stderr, "Error: Invalid compression ratio requested\n");
    return levels;
  }


  // Create the points array
  Point *points = new Point[numPoints];
  int i;
  for (i=0; i<numPoints; i++) {
    points[i].x = x[i];
    points[i].y = y[i];
  }

  // Run the actual K-means algorithm
  int K = int(compressionRatio * numPoints);
  int *classes = Kmeans(points, numPoints, K);
  int *pLevels = new int[numPoints];
  for (i=0; i<numPoints; i++) {
    pLevels[i]=0;
  }
  for (i=0; i<K; i++) {
    int index = FindCentroid(points, numPoints, classes, i);
    if (index < 0) {
      continue;
    }
    pLevels[index] = 1;
  }

  for (i=0; i<numPoints; i++) {
    levels.push_back(pLevels[i]);
  }

  delete[] points;
  delete[] pLevels;

  return levels;
}



// This is a overloaded wrapper function that accepts x, y in vectors
// along with a integer value K. It runs the Kmeans function and
// returns a vector of integers indicating which of the <=K points
// have been picked for representation. If strict is set to true
// then the algorithm tries to approach as close to the desired
// value of K as possible. However, exact value of K representatives
// is still not guaranteed.
std::vector<int> Kmeans(const std::vector<double>& x,
                   const std::vector<double>& y,
                   const int             K,
                   bool                  strict,
                   bool                  verbose)
{

  // Set the global state variables
  g_verbose       = verbose;
  g_strict        = strict;

  // Create the output vector
  std::vector<int> levels;

  int numPoints = x.size();
  if (numPoints == 0) {
    fprintf(stderr, "Error: 0 point features\n");
    return levels;
  }
  if ( K<=0 || K>numPoints ) {
    fprintf(stderr, "Error: Invalid K\n");
    return levels;
  }
  // Create the points array
  Point *points = new Point[numPoints];
  int i;
  for (i=0; i<numPoints; i++) {
    points[i].x = x[i];
    points[i].y = y[i];
  }

  // Run the actual K-means algorithm
  int *classes = Kmeans(points, numPoints, K);
  int *pLevels = new int[numPoints];
  for (i=0; i<numPoints; i++) {
    pLevels[i]=0;
  }
  for (i=0; i<K; i++) {
    int index = FindCentroid(points, numPoints, classes, i);
    if (index < 0) {
      continue;
    }
    pLevels[index] = 1;
  }

  for (i=0; i<numPoints; i++) {
    levels.push_back(pLevels[i]);
  }

  delete[] points;
  delete[] pLevels;

  return levels;

}

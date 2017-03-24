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

/***********************************************************************
        File:                   kmeans.h
        Description:    Header for kmeans implementation
***********************************************************************/

#ifndef KMEANS_H
#define KMEANS_H


#include <vector>


#ifndef INFINITY
#define INFINITY                                1e10
#endif


struct Point {
  double x, y;
};

// This the main Kmeans function. It accepts a number of
// points and a value for K and returns back an array
// equal to the size of the points array. Each entry in this
// array indicates which of the K groups this point belongs.
int* Kmeans(Point* points, int numPoints, int K);

// This is a wrapper function that accepts x, y in vectors along
// with a compression ratio. It computes appropriate value for K
// from the compressionRatio and runs the Kmeans function. It
// returns a vector of integers indicating which of the <=K points
// have been picked for representation.
std::vector<int> Kmeans(const std::vector<double>& x,
                        const std::vector<double>& y,
                        double compressionRatio,
                        bool strict = false,
                        bool verbose = false);

// This is a overloaded wrapper function that accepts x, y in vectors
// along with a integer value K. It runs the Kmeans function and
// returns a vector of integers indicating which of the <=K points
// have been picked for representation. If strict is set to true
// then the algorithm tries to approach as close to the desired
// value of K as possible. However, exact value of K representatives
// is still not guaranteed.
std::vector<int> Kmeans(const std::vector<double>& x,
                        const std::vector<double>& y,
                        const int K,
                        bool strict = false,
                        bool verbose = false);

// Computes euclidean distance between 2 points
double ComputeDistance(Point a, Point b);

#endif

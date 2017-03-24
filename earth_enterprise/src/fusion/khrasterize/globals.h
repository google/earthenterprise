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

#ifndef GLOBALS_H
#define GLOBALS_H

#include <vector>

#ifdef INFINITY
#undef INFINITY
#endif
#define INFINITY 1e10

struct Point;

typedef std::vector<Point> VPoints;
typedef std::vector<double> VDouble;
typedef std::vector<VDouble> VVDouble;
typedef std::vector<VPoints> VVPoints;

struct Point {
  double x, y, z;
};

struct Grid {
  double xmin;
  double ymin;
  double resolution;
  int rows;
  int cols;
  VVDouble z;
};

#endif

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

#ifndef RASTERIZE_H
#define RASTERIZE_H

#include <cmath>
#include "globals.h"

#define KH_MISSING_ZVALUE -32768

#define INTP_NEAREST 0
#define INTP_LINEAR 1

Grid Rasterize(VPoints points, double resolution, int fillRadius = 3,
               int method = 0, bool verbose = true);

#endif

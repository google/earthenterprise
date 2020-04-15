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


/******************************************************************************
File:        ffio.h
******************************************************************************/
#ifndef __ffio_h
#define __ffio_h

#include <cstdint>
#include <string>
#include <vector>
#include <assert.h>
#include <khConstants.h>
#include <khExtents.h>
#include <khTileAddr.h>

namespace ffio {
// Don't change these enum values! They are stored in ff indexes.
enum Type {
  Raster = 0,
  Tmesh  = 1,
  Vector = 2
};

inline std::string TypeName(Type t) {
  switch (t) {
    case Raster:
      return kRasterType;
    case Tmesh:
      return kTmeshType;
    case Vector:
      return kVectorType;
  }
  return std::string(); // unreached but silences warnings
}
}



#endif /* __ffio_h */

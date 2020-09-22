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


#ifndef __TmeshGenerator_h
#define __TmeshGenerator_h

#include <cstdint>
#include <khTileAddr.h>
#include "vipm/array.h"
#include "vipm/vector.h"
#include "vipm/mesh.h"

class TmeshGenerator
{
  const khTilespaceFlat &sampleTilespace;

  // temporaries used by generate. Hold them in the class so they don't
  // need to be constantly reallocated.
  etArray<etVec3f>          points;
  etArray<etFace3i>         faces;
  etMesh<etVec3f, etFace3i> mesh;
  std::vector<etVec3f>      elev;

 public:
  TmeshGenerator(const khTilespaceFlat &sampleTilespace_) :
      sampleTilespace(sampleTilespace_)
  { }

  void Generate(float *srcSamples,
                const khSize<std::uint32_t> &srcSamplesSize,
                const khOffset<std::uint32_t> &wantOffset,
                const khTileAddr &tmeshAddr,
                etArray<unsigned char> &compressed,
                const size_t reserve_size,
                bool  decimate,
                double decimation_threshold
               );
};


#endif /* __TmeshGenerator_h */

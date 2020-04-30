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


#ifndef FUSION_GEINDEXGEN_TODO_H__
#define FUSION_GEINDEXGEN_TODO_H__

#include <geindex/Writer.h>
#include <geindexgen/.idl/BlendStack.h>
#include <geindexgen/.idl/VectorStack.h>
#include "PacketReader.h"


namespace geindexgen {


// ****************************************************************************
// ***  BlendTodo
// ****************************************************************************
class BlendTodo {
 public:
  typedef BlendStack Stack;

  khDeletingVector<BlendPacketReader> readers_;

  // Create a new raster indexindex
  BlendTodo(geindex::BlendWriter &writer, geFilePool &file_pool,
            const BlendStack &stack);

  inline std::uint64_t TotalTileVisits(void) const { return total_tile_visits_; }

 private:
  std::uint64_t total_tile_visits_;
};


// ****************************************************************************
// ***  VectorTodo
// ****************************************************************************
class VectorTodo {
 public:
  typedef VectorStack Stack;

  khDeletingVector<VectorPacketReader> readers_;

  // Create a new raster indexindex
  VectorTodo(geindex::VectorWriter &writer, geFilePool &file_pool,
             const VectorStack &stack);

  inline std::uint64_t TotalTileVisits(void) const { return total_tile_visits_; }

 private:
  std::uint64_t total_tile_visits_;
};


} // namespace geindexgen


#endif // FUSION_GEINDEXGEN_TODO_H__

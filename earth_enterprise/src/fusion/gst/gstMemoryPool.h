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

#ifndef KHSRC_FUSION_GST_GSTMEMORYPOOL_H__
#define KHSRC_FUSION_GST_GSTMEMORYPOOL_H__

#include <cstddef>
#include <vector>

static const int kMaxPools = 20;  // allows 128MB allocations!
static const int kExpBase = 7;

class gstMemoryPool {
 public:
  gstMemoryPool();
  ~gstMemoryPool();

  void* Allocate(size_t size);
  void ResetAll();

 private:
  int DeterminePool(size_t size);
  typedef std::vector<void*> BlockAllocator;
  BlockAllocator allocated_blocks_[kMaxPools];
  BlockAllocator free_blocks_[kMaxPools];
};

#endif  // !KHSRC_FUSION_GST_GSTMEMORYPOOL_H__

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


// A simple memory management system
//
// intended to be used for a fairly small number of
// objects that get allocated/deallocated frequently
//
// this is designed for allocating a bunch
// of objects which will get released all at the same time
// with resetAll()

#include <gstMemoryPool.h>
#include <stdlib.h>
#include <notify.h>
#include <gstTypes.h>

gstMemoryPool::gstMemoryPool() {
}

gstMemoryPool::~gstMemoryPool() {
  for (int exp = 0; exp < kMaxPools; ++exp) {
    if (allocated_blocks_[exp].size()) {
      notify(NFY_DEBUG, "pool %d has %llu allocated blocks",
             exp,
             static_cast<long long unsigned>(allocated_blocks_[exp].size()));
    }
    for (BlockAllocator::iterator it = allocated_blocks_[exp].begin();
         it < allocated_blocks_[exp].end(); ++it) {
      free(*it);
    }
  }
}

void gstMemoryPool::ResetAll() {
  // scan all allocated blocks marking the in-use blocks to available
  // and stopping when the first available block is found
  for (int exp = 0; exp < kMaxPools; ++exp) {
    free_blocks_[exp] = allocated_blocks_[exp];
  }
}

void* gstMemoryPool::Allocate(size_t size) {
  int exp = DeterminePool(size);

  int sz = free_blocks_[exp].size();
  if (sz != 0) {
    void* mem = free_blocks_[exp][sz - 1];
    free_blocks_[exp].resize(sz - 1);
    return mem;
  }

  // no more blocks available, allocate a new one
  size_t allocate_size = 0x01 << (exp + kExpBase);
  void* new_block = malloc(allocate_size);
  allocated_blocks_[exp].push_back(new_block);
  return new_block;
}

int gstMemoryPool::DeterminePool(size_t size) {
  // determine which pool to pull from
  int exp = 0;
  while (size > size_t(0x01 << (exp + kExpBase)))
    ++exp;
  assert(exp < kMaxPools);
  return exp;
}

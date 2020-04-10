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

#ifndef COMMON_GEBUFFER_H__
#define COMMON_GEBUFFER_H__

#if 0

#include <new>
#include <cstdlib>

// ****************************************************************************
// ***  geBuffer
// ***
// ***  Simple wrapper around a raw buffer
// ***  - Manages lifetime of buffer
// ***  - Never shrinks, useful for processing many requests of variable
// ***       size. Will only reallocate until max size is reached.
// ***  - Resize is destructive, don't try to preserve buffer contents. This
// ***       saves the copy cost associated with a content preserving resize.
// ***
// ***  ----- sample usage -----
// ***  geBuffer<char> buf(2048);   // initially size it to 2K
// ***  for (unsigned int i = 0; i < numtiles; ++i) {
// ***    std::uint32_t tileSize = GetTileSize(i);
// ***    buf.destructive_resize(tileSize);
// ***    file.Pread(buf, tileSize, GetTileOffset(i));
// ***    Tile tile(buf, tileSize);
// ***    ... use tile ...;
// ***  }
// ****************************************************************************
class geBuffer {
 public:
  inline geBuffer(std::size_t prealloc = 0) : ptr(0), count(0), allocated(0) {
    destructive_resize(prealloc);
  }
  inline ~geBuffer(void) {
    release();
  }

  inline operator void*(void) const { return ptr; }
  inline std::size_t size(void) const { return count; }

  inline void release(void) {
    if (ptr) {
      std::free(ptr);
      ptr = 0;
    }
    count = allocated = 0;
  }

  inline void destructive_resize(std::size_t newcount) {
    destructive_reserve(newcount);
    count = newcount;
  }

 protected:
  void *ptr;
  std::size_t  count;
  std::size_t  allocated;

  // private and unimplemented - illegal to copy
  geBuffer(const geBuffer &);
  geBuffer& operator=(const geBuffer &);

  inline void destructive_reserve(std::size_t newallocated) {
    if (newallocated > allocated) {
      assert(newallocated > 0);
      void *newptr = std::malloc(newallocated);
      if (!newptr) {
        throw std::bad_alloc();
      }
      if (ptr) {
        std::free(ptr);
      }
      allocated = newallocated;
      ptr = newptr;
    }
  }
};


// ****************************************************************************
// ***  geResizableBuffer
// ***
// ***  Like geBuffer, but resize preserves original contents
// ****************************************************************************
class geResizableBuffer : public geBuffer {
 public:
  inline geResizableBuffer(std::size_t prealloc = 0) : geBuffer(prealloc) { }
  inline ~geBuffer(void) { }

  inline void resize(std::size_t newcount) {
    reserve(newcount);
    count = newcount;
  }

  inline void reserve(std::size_t newallocated) {
    if (newallocated > allocated) {
      assert(newallocated > 0);
      void *newptr = std::realloc(ptr, newallocated);
      if (!newptr) {
        throw std::bad_alloc();
      }
      allocated = newallocated;
      ptr = newptr;
    }
  }
 private:
  // private and unimplemented - illegal to copy
  geResizableBuffer(const geResizableBuffer &);
  geResizableBuffer& operator=(const geResizableBuffer &);
};


#endif

#endif // COMMON_GEBUFFER_H__

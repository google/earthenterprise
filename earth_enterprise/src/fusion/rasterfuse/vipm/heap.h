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

#ifndef _HEAP_H
#define _HEAP_H

//////////////////////////////////////////////////////////////////////////////
// Each object to insert in the heap will have to be of this type.
//
class etHeapable {
  int index;
 public:
  etHeapable() { resetHepeable(); }
  // XXX - since the current implementation needs getKey to be
  // virtual, go ahead and make a virtual destructor as well to
  // avoid compiler warnings.
  virtual ~etHeapable() { }

  virtual double getKey() = 0; // Must be defined in the heapable object

  inline void resetHepeable()           { index = -1;         }
  inline bool isInHeap()                { return index != -1; }
  inline int  getHeapPos() const        { return index;       }
  inline void setHeapPos(const int pos) { index = pos;        }
};


//////////////////////////////////////////////////////////////////////////////
class etHeap {
  etHeapable **table;
  unsigned int size;
  unsigned int length;

  inline unsigned int parent(unsigned int i) { return (i-1)>>1; }
  inline unsigned int left  (unsigned int i) { return (i<<1)+1; }
  inline unsigned int right (unsigned int i) { return (i+1)<<1; }
  inline void place(etHeapable *x, unsigned int i) {
    table[i] = x;
    x->setHeapPos(i);
  }

  void up  (unsigned int i);
  void down(unsigned int i);

  etHeapable *extract();

 public:
  etHeap() { table = 0; size = 0; length = 0; }
  etHeap(const int sz) { table = 0; size = 0; length = 0; init(sz); }

  ~etHeap();

  void init(const int sz);
  void reset();

  void insert(etHeapable *t);
  void update(etHeapable *t);

  inline etHeapable *getTop()   { return (length ? table[0] : 0); }
  inline etHeapable *getFirst() { return extract(); }

  etHeapable *remove(etHeapable *t);
};


#endif

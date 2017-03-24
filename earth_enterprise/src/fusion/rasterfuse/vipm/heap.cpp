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

#include <assert.h>
#include "heap.h"

//////////////////////////////////////////////////////////////////////////////
etHeap::~etHeap()
{
  delete [] table;
}
//////////////////////////////////////////////////////////////////////////////
void
etHeap::init(const int sz)
{
  assert(sz != 0);

  delete [] table;
  table = new etHeapable*[sz];
  size = sz;
  length = 0;

  reset();
}

//////////////////////////////////////////////////////////////////////////////
void
etHeap::reset()
{
  for(unsigned int i=0;i<size;i++) table[i] = 0;
  length = 0;
}

//////////////////////////////////////////////////////////////////////////////
void etHeap::up(unsigned int i)
{
  etHeapable *moving = table[i];
  unsigned int index = i;
  unsigned int p = parent(i);

  while( index > 0 && moving->getKey() > table[p]->getKey() )
  {
    place(table[p], index);
    index = p;
    p = parent(p);
  }

  if( index != i ) place(moving, index);
}

//////////////////////////////////////////////////////////////////////////////
void etHeap::down(unsigned int i)
{
  etHeapable *moving = table[i];
  unsigned int index = i;
  unsigned int l = left(i);
  unsigned int r = right(i);
  unsigned int largest;

  while( l < length )
  {
    if( r < length && table[l]->getKey() < table[r]->getKey() )
      largest = r;
    else
      largest = l;

    if( moving->getKey() < table[largest]->getKey() )
    {
      place(table[largest], index);
      index = largest;
      l = left(index);
      r = right(index);
    }
    else break;
  }

  if( index != i ) place(moving, index);
}

//////////////////////////////////////////////////////////////////////////////
void
etHeap::insert(etHeapable *t)
{
  assert(!t->isInHeap() );
  table[length++] = t;
  t->setHeapPos(length-1);
  up(length-1);
}

//////////////////////////////////////////////////////////////////////////////
void
etHeap::update(etHeapable *t)
{
  assert(t->isInHeap() );

  double       v = t->getKey();
  unsigned int i = t->getHeapPos();

  if( i > 0 && v > table[parent(i)]->getKey() )
    up(i);
  else
    down(i);
}

//////////////////////////////////////////////////////////////////////////////
etHeapable *etHeap::extract()
{
  if( !length ) return 0;

  etHeapable *dead = table[0];
  table[0] = table[--length];
  table[0]->setHeapPos(0);

  down(0);

  dead->resetHepeable();
  assert(!dead->isInHeap() );

  return dead;
}

//////////////////////////////////////////////////////////////////////////////
etHeapable *etHeap::remove(etHeapable *t)
{
  if( !t->isInHeap() ) return 0;

  int i = t->getHeapPos();
  table[i] = table[--length];
  table[i]->setHeapPos(i);

  down(i);

  t->resetHepeable();
  assert(!t->isInHeap() );

  return t;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

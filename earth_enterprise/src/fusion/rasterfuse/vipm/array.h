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

#ifndef _ET_ARRAY_H
#define _ET_ARRAY_H


#include <assert.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////////
template <class T>
class etArray {
  T   *array;
  int size;
  int chunk;

  // private and unimplemented. etArray is NOT safe across
  // copy operations. It loses knowledge of the array ownership.
  etArray(const etArray &);
  etArray& operator=(const etArray &);
 public:
  int length;

  // Constructor
  etArray(int chk = 256);
  ~etArray();

  // Accessors
  inline T& operator[](int index) { return array[index]; }
  inline const T &operator[](int index) const { return array[index]; }

  // Add an element to the list.
  int add(const T *elm);

  void reset() { length = 0; }

  T* peek() const { return array; }
  int bufSize(void) const { return size; }

  void merge(const etArray<T> &list);

  void remove(const int idx);
  void findAndRemove(const T &item);

  inline void setChunck(int v) { chunk = v; }

  bool expand( int len);
};


//////////////////////////////////////////////////////////////////////////////
template <class T>
etArray<T>::etArray(int chk)
{
  chunk  = chk;
  size   = 0;
  array  = 0;
  length = 0;
}


//////////////////////////////////////////////////////////////////////////////
template <class T>
etArray<T>::~etArray()
{
  delete [] array;
}


//////////////////////////////////////////////////////////////////////////////
template <class T>
bool etArray<T>::expand(int len)
{
  if(len < size) return true;

  // Need to resize
  T * ptr = new T[size+chunk];
  if(!ptr) return false;

  if(array) memcpy(ptr, array, sizeof(T) * len);
  delete [] array;
  array = ptr;
  size += chunk;

  return true;
}


//////////////////////////////////////////////////////////////////////////////
template <class T>
int etArray<T>::add(const T *elm)
{
  if(!expand(length+1)) return -1;
  memcpy(&(array[length]), elm, sizeof(T));
  return length++;
}

//////////////////////////////////////////////////////////////////////////////
template <class T>
void etArray<T>::merge(const etArray<T> &list)
{
  int i;
  for(i =0; i<list.length;i++) add(&(list[i]));
}

//////////////////////////////////////////////////////////////////////////////
template <class T>
void etArray<T>::remove(const int idx)
{
  assert(idx >= 0 && idx < length);

  array[idx] = array[length-1];
  length--;
}

//////////////////////////////////////////////////////////////////////////////
template <class T>
void etArray<T>::findAndRemove(const T &item)
{
  // Note: I assume that only one istance of the item is in the list.
  int i;
  for(i=0;i<length;i++)
  {
    if (array[i] == item) {
      if(i < length-1)
      {
        array[i] = array[length-1];
        length--;
      }
      else length--;
      break;
    }
  }
}

#endif

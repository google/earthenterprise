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




#ifndef _khHashTable_h_
#define _khHashTable_h_

#include <khMisc.h>
#include <khArray.h>

//
// General purpose hash table.
//
// Table size should be a power of two,
//   if not, round up so that it is.
// 

template <class Type, class TypeID=int>
class khHashTable 
{
 private:
  class HashItem 
  {
   public:
    HashItem(const TypeID &id, Type data) : _id(id), _data(data), _next( 0 ) {  }

    const TypeID &id() const { return _id; }
    Type data() const { return _data; }
    HashItem *next() const { return _next; }
    void next( HashItem *n ) { _next = n; }

   private:
    TypeID _id;
    Type _data;
    HashItem *_next;
  };

 public:
  khHashTable( int sz )
  {
    int nearestPower;
    nearestPower = log2up(sz);
    _size = 1 << nearestPower; 
    _hashMask = _size - 1;
    _hashPrime = primeNumbers[nearestPower];
    _table = new HashItem*[_size];
    memset(_table, 0, _size * sizeof(HashItem*));
    _numElements = 0;
  }

  ~khHashTable()
  {
    HashItem *cur;
    for (int ii = 0; ii < _size; ii++) {
      cur = _table[ii];
      while (cur) {
        HashItem *tmp = cur;
        cur = cur->next();
        delete tmp;
      }
    }
    delete [] _table;
  }

  int length() { return _numElements; }

  unsigned int hash(const TypeID &id) const
  {
    return (id % _hashPrime) & _hashMask;
  }

  void insertBlind(const TypeID &id, Type obj)
  {
    HashItem *item = new HashItem(id, obj);
    item->next( _table[hash(id)] );
    _table[hash(id)] = item;
    _numElements++;
  }

  Type find( const TypeID &id ) const
  {
    const HashItem *cur = _table[ hash( id ) ];
    while ( cur ) {
      if ( cur->id() == id )
        return cur->data();
      else
        cur = cur->next();
    }
    return Type();
  }

  Type remove(const TypeID &id)
  {
    HashItem *cur  = _table[hash(id)];
    HashItem *prev = NULL;
    while (cur) {
      if (cur->id() == id) {
        if (prev)
          prev->next( cur->next() );
        else
          _table[hash(id)] = cur->next();
        Type gone = cur->data();
        delete cur;
        _numElements--;
        return gone;
      } else {
        prev = cur;
        cur = cur->next();
      }
    }
    return Type();
  }


  // delete contents of hash table as well hash infrastructure
  void deleteAll()
  {
    HashItem *cur;
    for ( int ii = 0; ii < _size; ii++ ) {
      cur = _table[ ii ];
      while ( cur ) {
        HashItem *tmp = cur;
        cur = cur->next();
        delete tmp->data();
        delete tmp;
      }
      _table[ ii ] = NULL;
    }
    _numElements = 0;
  }


  // allocate an array and place all hash table elements into it
  // this is a convenience function for examining every element
  // of the hash table
  //
  // ** NOTE: it is the calling functions responsibility to 
  //          free this memory 
  //
  khArray<Type> *array() 
  {
    khArray<Type> *array = new khArray<Type>( _numElements );
    HashItem *cur;
    for ( int ii = 0; ii < _size; ii++ ) {
      cur = _table[ ii ];
      while ( cur ) {
        array->append( cur->data() );
        cur = cur->next();
      }
    }
    return array;
  }

#if 0
  void print()
  {
    int sums[10000];
    memset(&sums[0], 0, 10000 * sizeof(int));

    int max = 0;
    int tot = 0;
    HashItem *cur;
    for (int ii = 0; ii < _size; ii++) {
      cur = _table[ii];
      int bc = 0;
      while (cur) {
        bc++;
        tot++;
        if (bc > max)
          max = bc;
        cur = cur->next();
      }
      sums[bc]++;
      //fprintf(stderr, "  [%d] %d\n", ii, bc);
    }
    fprintf(stderr, "largest bin has %d items\n", max);
    fprintf(stderr, "total number of elements %d are distributed in %d bins\n", tot, _size);

    for (int ii = 0; ii <= max; ii++) {
      if (sums[ii] > 0) {
	fprintf(stderr, "bins with %d elements: %d\n", ii, sums[ii]);
      }
    }
  }
#endif


 private:
  HashItem **_table;
  int _size;
  unsigned int _hashMask;
  int _hashPrime;
  int _numElements;
  static const int primeNumbers[];
};

template <class Type, class TypeID>
const int khHashTable<Type, TypeID>::primeNumbers[] = {3, 5, 7, 13, 23, 41, 71, 137, 271, 523, 1033, 2063, 4111, 8219, 16417, 32797, 65543, 131111, 262151, 524341, 1048589, 2097169, 4194319, 8388617, 16777259, 33554467, 67108879, 134217757, 268435459, 536870923, 1073741827};



#endif // !_khHashTable_h_

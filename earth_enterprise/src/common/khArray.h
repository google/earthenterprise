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



#ifndef _khArray_h_
#define _khArray_h_

#include <stdlib.h>
#include <assert.h>

// general purpose array
// _size is the number of <class Type> objects space has been allocated for
// _length is the current number of <class Type> objects in the array

// ****************************************************************************
// ***  khArray
// ***
// ***  This class is safe only for classes with very simple or no
// ***  constructors & destructors. It uses realloc to grow instead of
// ***  copying the held objects.
// ***
// ***  New code should use std::vector instead
// ****************************************************************************
template <class Type>
class khArray 
{
 public:
  // constructors
  khArray() { _init( 2, 2 ); }

  khArray( unsigned int s, unsigned int g = 10 ) { _init( s, g ); }
  khArray( const khArray& that ) { _init( that ); }

  ~khArray() { free( _array ); }

 private:

  // Private Initialization (only called by constructors)
  khArray& _init( unsigned int s, unsigned int g = 10 )
  { 
    _length = 0; 
    _size = s; 
    _growSize = g;

    _array = ( Type* )malloc( sizeof( Type ) * _size );

    assert( _array != NULL );
    return *this; 
  }

  khArray& _init(const khArray& a)
  { 
    _init(a.size(), a.growSize());
    _length = a.length(); 
    assert(_array != NULL);
    memcpy(_array, a.array(), _length * sizeof(Type)); 
    return *this; 
  }

 public:

  // Public Initialization 
  // This allows objects to re-init their arrays 
  khArray& init( unsigned int s, unsigned int g = 10 ) 
  { 
    _length = 0; 
    _size = s; 
    _growSize = g;

    _array = ( Type* )realloc( _array, sizeof( Type ) * _size );

    assert( _array != NULL );
    return *this; 
  }

  khArray& init( const khArray& a )
  { 
    init( a.size(), a.growSize() );
    _length = a.length(); 
    assert( _array != NULL );
    memcpy( _array, a.array(), _length * sizeof( Type ) ); 
    return *this; 
  }

  khArray& init( unsigned int s, Type *a )
  {
    _length = _size = s;
    free( _array );
    _array = a;
    return *this;
  }

  void clear() { _length = 0; }

  // Copying
  khArray& operator=( const khArray& a ) { init( a ); return *this; }

  Type &operator[]( unsigned int idx ) const { return _array[ idx ]; }

  Type get( unsigned int idx ) const { return _array[ idx ]; }

  void append(Type a)
  {
    _length++;
    if (_length > _size) {
      _size += _growSize;
      assert(_array != NULL);
      _array = (Type*)realloc(_array, _size * sizeof(Type));
      assert(_array != NULL);
    }

    _array[_length - 1] = a;
  }

  void insert( const khArray &a, unsigned int pos )
  {
    if ( a.length() == 0 )
      return;

    unsigned int move = _length - pos;

    _length += a.length();
    if ( _length > _size ) {
      _size = _length;
      _array = ( Type* )realloc( _array, _size * sizeof( Type ) );
      assert( _array != NULL );
    }
    for ( unsigned int end = _length - 1; move; end--, move-- )
      _array[ end ] = _array[ end - a.length() ];

    move = 0;
    for ( unsigned int begin = pos; begin < pos + a.length(); begin++, move++ )
      _array[ begin ] = a.array()[ move ];
  }
    
  void append( const khArray &a ) { insert( a, _length ); }

  // same as above, but incoming array is reversed
  void insertReverse( khArray &a, unsigned int pos )
  {
    unsigned int move = _length - pos;

    _length += a.length();
    if ( _length > _size ) {
      _size = _length;
      _array = ( Type* )realloc( _array, _size * sizeof( Type ) );
      assert( _array != NULL );
    }
    for ( unsigned int end = _length - 1; move; end--, move-- )
      _array[ end ] = _array[ end - a.length() ];

    move = a.length() - 1;
    for ( unsigned int begin = pos; begin < pos + a.length(); begin++, move-- )
      _array[ begin ] = a.array()[ move ];
  }

  void insert( Type a, unsigned int p )
  {
    _length++;
    if ( _length > _size ) {
      _size += _growSize;
      _array = ( Type* )realloc( _array, _size * sizeof( Type ) );
      assert( _array != NULL );
    }

    for ( unsigned int end = _length - 1; end > p; end-- )
      _array[ end ] = _array[ end - 1 ];

    _array[ p ] = a;
  }

  void swap( Type a, Type b )
  {
    unsigned int posa;
    unsigned int posb;

    if ( findIndex( a, posa ) && findIndex( b, posb ) ) {
      Type tmp = _array[ posa ];
      _array[ posa ] = _array[ posb ];
      _array[ posb ] = tmp;
    }
  }

  Type removeIndex( unsigned int pos )
  {
    if ( pos < _length ) {
      Type ret = _array[ pos ];
      for ( ; pos < _length - 1; pos++ )
        _array[ pos ] = _array[ pos + 1 ];
      _length--;
      return ret;
    } else
      return Type();
  }

  Type remove( Type a ) 
  { 
    unsigned int id;
    if ( findIndex( a, id ) )
      return removeIndex( id );
    return Type();
  }

  Type removeLast()
  {
    if ( _length == 0 )
      return Type();
    _length--;
    return _array[ _length ];
  }


  bool findIndex( Type a, unsigned int &id ) const
  {
    for ( unsigned int id = 0; id < _length; ++id )
      if ( _array[ id ] == a )
        return true;
    return false;
  }

  // supplied list must be in same order as this
  void removeList(khArray& a)
  {
    unsigned int skip = 0;
    Type *loc = _array;
    while ( ( loc + skip ) < ( _array + _length ) ) {
      if ( skip >= a.length() || *( loc + skip ) != a[ skip ] ) {
        *loc = *( loc + skip );
        loc++;
      } else
        while ( skip < a.length() && *( loc + skip ) == a[ skip ] )
          skip++;
    }

    _length -= a.length();
  }

  void removeToEnd( unsigned int pos )
  {
    if (pos < _length)
      _length = pos + 1;
  }

  Type head() const { return _length > 0 ? _array[ 0 ] : ( Type )NULL; }
  Type tail() const { return _length > 0 ? _array[ _length - 1 ] : ( Type )NULL; }
  Type* array() const { return _array; }

  unsigned int length() const { return _length; }
  unsigned int size() const { return _size; }

  void growSize( unsigned int g ) { _growSize = g; }
  unsigned int growSize() const { return _growSize; }

 private:
  Type *_array;
  unsigned int _length;
  unsigned int _size;

  // Amount to grow array when more space is needed
  // Copy operator should not copy this. Only the contructor
  // sets it, or explicetly by growSize()
  unsigned int _growSize;
};

#endif //!_khArray_h_

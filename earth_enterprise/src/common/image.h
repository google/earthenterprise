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


#ifndef _image_h_
#define _image_h_

#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>


template < class Type > class Tile3D 
{
 public:
  Tile3D( Type X, Type Y, Type C ) : x(X), y(Y), c(C) { }

  Type size() const { return x * y * c; }

  Type x, y, c;
};

typedef Tile3D< std::uint32_t > ImgTile;

enum ImgOrder {
  Interleaved,
  Separate
};

enum ImgOrientation {
  UpperLeft,
  UpperRight,
  LowerLeft,
  LowerRight
};

// Image object to assist with converting between different
// tile sizes, orientations, and orders

template < class Type > class ImageObj 
{

 public:
  ImageObj( const ImgTile &sz, ImgOrder ord, ImgOrientation ori ) 
      : _tile( sz ), _order( ord ), _orientation( ori ) 
  { 
    if ( _order == Interleaved ) {
      assert(_tile.c == 1);
    }
    // allocate buffer pointers
    _buffers = new Type*[_tile.c];
    for (unsigned int i = 0; i < _tile.c; ++i) {
      _buffers[i] = new Type[_tile.x * _tile.y];
    }
  }

  ~ImageObj()
  {
    for (unsigned int i = 0; i < _tile.c; ++i) {
      delete [] _buffers[i];
    }
    delete [] _buffers;
  }

  const ImgTile &tile() const { return _tile; }

  ImgOrder order() const { return _order; }

  ImgOrientation orientation() const { return _orientation; }

  inline char *getData( int chan ) { return (char*)_buffers[ chan ]; }

  void getTile( std::uint32_t x, std::uint32_t y, std::uint32_t w, std::uint32_t h, Type *outbuf, 
                ImgOrder ord = Interleaved, ImgOrientation ori = UpperLeft )
  {
    assert(w && h);
        
#if 0
    // This Interleaved/Interleaved is broken (but unused).
    // It subscripts _buffers as if it were T[] instead of T*[]
    if ( _order == Interleaved && ord == Interleaved ) {

      Type *row, *col;

      if ( _orientation == LowerLeft && ori == UpperLeft ) {

        for ( std::uint32_t sy = y + h - 1; sy >= y; sy-- ) {
          row = _buffers[ sy * _tile.x * _tile.c ];
          for ( std::uint32_t sx = x; sx < x + w; sx++ ) {
            col = &row[ sx * _tile.c ];
            for ( unsigned int c = 0; c < _tile.c; c++ ) {
              *outbuf = col[ c ];
              outbuf++;
            }
          }
          // don't wrap our unsigned int
          if (sy == 0) break;
        }

        // this is WRT the incoming image orientation.  
        // probably should be WRT the destination?
        // we're not going to use this right now anyway...
      } else if ( _orientation == UpperLeft && ori == UpperLeft ) {

        for ( std::uint32_t sy = y; sy < y + h; sy++ ) {
          row = _buffers[ sy * _tile.x * _tile.c ];
          for ( std::uint32_t sx = x; sx < x + w; sx++ ) {
            col = &row[ sx * _tile.c ];
            for ( unsigned int c = 0; c < _tile.c; c++ ) {
              *outbuf = col[ c ];
              outbuf++;
            }
          }
        }
      }

    } else
#endif
      if ( _order == Separate && ord == Interleaved ) {

        int rowoff;
        int coloff;

        if ( _orientation == LowerLeft && ori == UpperLeft ) {

          for ( std::uint32_t sy = y + h - 1; sy >= y; sy-- ) {
            rowoff = sy * _tile.x;
            for ( std::uint32_t sx = x; sx < x + w; sx++ ) {
              coloff = rowoff + sx;
              for ( unsigned int c = 0; c < _tile.c; c++ ) {
                *outbuf = _buffers[ c ][ coloff ];
                outbuf++;
              }
            }
            // don't wrap our unsigned int
            if (sy == 0) break;
          }

        } else if ( _orientation == LowerLeft && ori == LowerLeft ) {

          for ( std::uint32_t sy = y ; sy < y + h; sy++ ) {
            rowoff = sy * _tile.x;
            for ( std::uint32_t sx = x; sx < x + w; sx++ ) {
              coloff = rowoff + sx;
              for ( unsigned int c = 0; c < _tile.c; c++ ) {
                *outbuf = _buffers[ c ][ coloff ];
                outbuf++;
              }
            }
          }

        }

      } else {
        fprintf( stderr, "Oops.  This combination not supported yet.  Sorry!\n" );
        exit( -1 );
      }
  }


 private:
  Type **_buffers;

  ImgTile _tile;
  ImgOrder _order;
  ImgOrientation _orientation;
};

#endif // !_image_h_

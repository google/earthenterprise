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

#ifndef KHSRC_FUSION_GST_GSTQUEUE_H__
#define KHSRC_FUSION_GST_GSTQUEUE_H__

#include <khArray.h>
#include <gstList.h>

//
// brief simple FIFO Queue, implemented as an array
//
// Implemented as an array for speed of use,
// but this requires the queue to be cleared frequently

template <class Type>
class gstQueue {
 public:
  /*!
   * \brief constructor sets queue size
   * \param siz Maximum size of queue
   */
  gstQueue( unsigned int sz )
  {
    _elements = new khArray<Type>( sz );
    _curItem = 0;
  }

  /*!
   * Note - doesn't perform delete on elements
   */
  ~gstQueue() { delete _elements; }

  /*!
   * \brief Put an item at the back of the queue
   */
  void put( Type item ) { _elements->append( item ); }

  /*!
   * \brief Get the item from the front of the queue
   */
  Type get()
  {
    if ( _curItem < _elements->length() )
      return ( *_elements )[ _curItem++ ];
    else
      return Type();
  }

  /*!
   * \brief Reset queue to empty
   *
   * Note - doesn't perform delete on elements
   */
  void clear()
  {
    _elements->clear();
    _curItem = 0;
  }

  /*!
   * \brief Return size (number of elements currently stored in Queue)
   */
  int size() { return _elements->length() - _curItem; }

 private:
  khArray<Type> *_elements;
  unsigned int _curItem;
};

/*!
 * \brief FIFO Queue - implemented as a wrapping array
 *
 * Implemented as an array (for speed) but performs wrapping so array doesn't
 * grow with every new item.
 */

template <class Type>
class gstArrayQueue {
 public:
  /*!
   * \brief constructor sets queue size
   * \param siz Maximum size of queue
   */
  gstArrayQueue(int sz)
  {
    _elements = new Type[sz];
    _size = sz;
    _firstItem = 0;
    _lastItem = 0;
    _numItems = 0;
  }

  /*!
   * Note - doesn't perform delete on elements
   */
  ~gstArrayQueue() { delete _elements; }

  /*!
   * \brief Put an item at the back of the queue
   */
  void put(Type item) {
    if (_numItems == _size)
      notify(NFY_FATAL, "gstArrayQueue full");
    _numItems++;
    _elements[_lastItem] = item;
    _lastItem++;
    if (_lastItem == _size)
      _lastItem = 0;
  }

  /*!
   * \brief Get the item from the front of the queue
   */
  Type get()
  {
    if (_numItems == 0) {
      notify( NFY_DEBUG, "Queue get -- numItems 0" );
      return (Type) NULL;
    }
    Type ret = _elements[_firstItem];
    _firstItem++;
    if (_firstItem == _size)
      _firstItem = 0;
    _numItems--;
    /*             printf("Queue get %i -- _firstItem %i, _lastItem %i, " */
    /*                    "_numItems now %i\n", */
    /*                    ret, _firstItem, _lastItem, _numItems); */
    return ret;
  }

  /*!
   * \brief Reset queue to empty
   *
   * Note - doesn't perform delete on elements
   */
  void clear()
  {
    _numItems = 0;
    _firstItem = 0;
    _lastItem = 0;
  }

  /*!
   * \brief Return size (number of elements currently stored in Queue)
   */
  int size() { return _numItems; }

 private:
  Type *_elements;
  int _size;
  int _firstItem;
  int _lastItem;
  int _numItems;
};

/*!
 * \brief FIFO Queue - implemented as a linked list
 *
 * Implemented as an linked list which saves on
 * memory usage, and never requires clearing, but might
 * be a little bit slower for put/get since it has to
 * new and delete each time.
 */

template <class Type>
class gstListQueue {
 public:
  /*!
   * \brief Put an item at the back of the queue
   */
  void put(Type item) { _elements.append(item); }

  /*!
   * \brief Get the item from the front of the queue
   */
  Type get() { return _elements.removeIndex(0); }

  /*!
   * \brief Return size (number of elements currently stored in Queue)
   */
  int size() { return _elements.length(); }

 private:
  gstList<Type> _elements;

};

#endif  // !KHSRC_FUSION_GST_GSTQUEUE_H__

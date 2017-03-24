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

#ifndef KHSRC_FUSION_GST_GSTLIST_H__
#define KHSRC_FUSION_GST_GSTLIST_H__

// ----------------------------------------------------------------------------
//
// OBSOLETE!
//
// All code that uses this custom linked-list object should be converted to
// use the STL list.
//
// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

template <class DataType>
class gstList {

 private:

  class LinkItem {

   public:
    LinkItem(DataType data) : _data(data)
    { _next = _prev = NULL; }

    LinkItem *next() { return _next; }
    DataType getData() { return _data; }

   private:
    LinkItem *_next;
    LinkItem *_prev;
    DataType _data;

    friend class gstList;
  };


 public:
  gstList() { reset(); }
  gstList(const gstList *that)
  {
    reset();
    for (int ii = 0; ii < that->length(); ii++)
      append(that->get(ii));
  }

  ~gstList() { while (removeIndex(0)) { ; } }

  void reset() {
    _head = _tail = _curr = _succ = NULL;
    _length = _arrayState = 0;
    _array = NULL;
  }

  void resetIter()
  { _curr = _succ = _head; }

  DataType head() const
  { return _length ? (DataType)_head->getData() : NULL; }

  DataType tail() const
  { return _length ? (DataType)_tail->getData() : NULL; }

  DataType *array()
  {
    if (_arrayState)
      return _array;
    if (_array)
      delete _array;
    _array = new DataType[_length];
    LinkItem *item = _head;
    for (int idx = 0; idx < _length; idx++) {
      _array[idx] = item->getData();
      item = item->_next;
    }
    _arrayState = 1;
    return _array;
  }

  void append(DataType data)
  {
    LinkItem *item = new LinkItem(data);
    if (_head == NULL) {
      _head = _tail = item;
      resetIter();
    } else {
      _tail->_next = item;
      item->_prev = _tail;
      _tail = item;
    }
    _length++;
    _arrayState = 0;
  }

  DataType next() {
    _curr = _succ;
    _succ = _succ ? _succ->next() : NULL;
    return _curr ? _curr->getData() : NULL;
  }

  DataType get(int idx) const
  { LinkItem *item = _getIndex(idx); return item ? item->getData() : NULL; }

  DataType removeIndex(int idx) {
    LinkItem *found = _getIndex(idx);

    if (found) {
      DataType foundData = found->getData();
      remove_p(found);
      return foundData;
    } else
      return NULL;
  }

  DataType remove(DataType item)
  {
    LinkItem *found = _get(item);

    if (found) {
      remove_p(found);
      DataType foundData = found->getData();
      delete found;
      return foundData;
    } else
      return NULL;
  }

  void insertIndex(DataType data, int idx)
  {
    assert(idx < _length);

    LinkItem *found = _getIndex(idx);
    assert(found != NULL);

    LinkItem *item = new LinkItem(data);

    item->_prev = found->_prev;
    item->_next = found;
    if (found->_prev)
      found->_prev->_next = item;
    found->_prev = item;

    if (idx == 0)
      _head = item;

    _length++;
  }

  DataType removeCurrent()
  { return remove_p(_curr); }

  int length() const
  { return _length; }

 private:

  // user is responsible for freeing LinkItem::_data
  void remove_p(LinkItem *bye)
  {
    if (!bye) return;

    if (bye->_prev)
      bye->_prev->_next = bye->_next;
    if (bye->_next)
      bye->_next->_prev = bye->_prev;

    if (bye == _head)
      _head = bye->_next;

    if (bye == _tail)
      _tail = bye->_prev;

    _length--;
    _arrayState = 0;
    delete bye;
  }

  LinkItem *_getIndex(int n) const {
    LinkItem *found;
    if (_length == 0 || n < 0 || n >= _length)
      found = NULL;
    else if (n == 0)
      found = _head;
    else if (n == _length - 1)
      found = _tail;
    else {
      found = _head;
      int pos = 0;
      while (n != pos) {
        found = found->_next;
        pos++;
      }
    }
    return found;
  }

  LinkItem *_get(DataType item)
  {
    if (item == NULL)
      return NULL;

    LinkItem *found = _head;
    while (found) {
      if (found->getData() == item)
        break;
      found = found->_next;
    }

    return found;
  }


  int _length;
  DataType *_array;
  int _arrayState;

  LinkItem *_head;
  LinkItem *_tail;
  LinkItem *_succ;
  LinkItem *_curr;
};

#endif  // !KHSRC_FUSION_GST_GSTLIST_H__

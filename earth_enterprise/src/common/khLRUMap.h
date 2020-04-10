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

#ifndef COMMON_KHLRUMAP_H__
#define COMMON_KHLRUMAP_H__


#include <map>
#include <assert.h>

/******************************************************************************
 ***  Simple LRU Map
 ***
 ***  This is intended to be a building block for higher level LRU caches
 ***
 ***  Key can be anything that supports less<T>
 ***  You can Add, Remove, Find in the Map
 ***  You can also iterate the map from oldest to newest
 ***
 ***  Only one thread may access a khLRUMap object at a time. You must
 ***  protect access with a mutex.
 ******************************************************************************/
template <class Key, class Value>
class khLRUMapItem {
  // private and unimplemented - no copying allowed, would mess up links
  khLRUMapItem(const khLRUMapItem&);
  khLRUMapItem& operator=(const khLRUMapItem&);
 public:
  khLRUMapItem *next;
  khLRUMapItem *prev;
  Key   key;
  Value val;
  khLRUMapItem(const Key &key_, const Value &val_)
      : next(0), prev(0), key(key_), val(val_) { }
};


// #define CHECK_INVARIANTS

template <class Key, class Value>
class khLRUMap {
 public:
  typedef khLRUMapItem<Key, Value> Item;
  typedef std::map<Key, Item*> MapType;

  class iterator {
    friend class khLRUMap;

    Item* item;
    iterator(Item *i) : item(i) { }
   public:
    iterator(void) : item(0) { }

    inline bool operator==(const iterator &o) { return item == o.item; }
    inline bool operator!=(const iterator &o) { return !operator==(o); }
    inline Value* operator->(void)  { return &item->val; }
    inline Value& operator* (void)  { return item->val; }
    inline iterator& operator++(void) { item = item->next; return *this; }
  };

  typedef typename MapType::size_type size_type;
  inline size_type size(void) const { return map.size(); }

  khLRUMap(void);
  ~khLRUMap(void);

  iterator begin(void) { return iterator(head);}
  iterator end(void)   { return iterator();}

  void Clear(void);
  void Add(const Key &key, const Value &val);
  void Remove(const Key &key);
  bool HasKey(const Key &key);
  bool Find(const Key &key, Value &returnValue);

  template <class Functor>
  void Apply(const Functor &func);

 private:
  MapType map;
  Item *head;
  Item *tail;
  unsigned int numItems;

#ifdef CHECK_INVARIANTS
  bool InList(Item *item);
  void CheckListInvariant(void);
#else
  inline void CheckListInvariant(void) { }
#endif
  void Link(Item *item);
  void Unlink(Item *item);
  Item* FindItem(const Key& key);
};

// ****************************************************************************
// ***  Member implementation
// ****************************************************************************
#ifdef CHECK_INVARIANTS
template <class Key, class Value>
bool khLRUMap<Key, Value>::InList(Item *item) {
  Item *tmp = head;
  while (tmp) {
    if (tmp == item)
      return true;
    tmp = tmp->next;
  }
  return false;
}

template <class Key, class Value>
void khLRUMap<Key, Value>::CheckListInvariant(void) {
  assert(numItems == map.size());
  if (!numItems) {
    assert(!head);
    assert(!tail);
    return;
  }

  assert(head);
  assert(tail);

  // check forward links
  Item *tmp = head;
  while (tmp) {
    if (!tmp->next)
      assert(tmp == tail);
    tmp = tmp->next;
  }

  // check reverse links
  tmp = tail;
  while (tmp) {
    if (!tmp->prev)
      assert(tmp == head);
    tmp = tmp->prev;
  }

  // check map contents
  for (typename MapType::const_iterator i = map.begin();
       i != map.end(); ++i) {
    assert(InList(i->second));
  }
}
#endif // CHECK_INVARIANTS

template <class Key, class Value>
void khLRUMap<Key, Value>::Link(Item *item) {
  assert(item->next == 0);
  assert(item->prev == 0);
  item->next = head;
  item->prev = 0;
  head = item;
  if (item->next)
    item->next->prev = item;
  else
    tail = item;
  ++numItems;
}

template <class Key, class Value>
void khLRUMap<Key, Value>::Unlink(Item *item) {
  if (item->prev)
    item->prev->next = item->next;
  else
    head = item->next;
  if (item->next)
    item->next->prev = item->prev;
  else
    tail = item->prev;
  item->next = 0;
  item->prev = 0;
  --numItems;
}

template <class Key, class Value>
khLRUMapItem<Key, Value>* khLRUMap<Key, Value>::FindItem(const Key& key) {
  typename MapType::const_iterator found = map.find(key);
  if (found != map.end()) {
    return found->second;
  } else {
    return 0;
  }
}

template <class Key, class Value>
void khLRUMap<Key, Value>::Clear(void) {
  CheckListInvariant();
  // Delete all the items. 
  for (typename MapType::iterator i = map.begin(); i != map.end(); ++i) {
    assert(i->second);
    delete i->second;
    i->second = 0;
  }
  map.clear();
  head = tail = 0;
}


template <class Key, class Value>
void khLRUMap<Key, Value>::Add(const Key &key, const Value &val) {
  CheckListInvariant();

  // delete any previous item
  Item *item = FindItem(key);
  if (item) {
    Unlink(item);
    delete item;
  }

  // make a new item, link it in and add to map
  item = new Item(key, val);
  Link(item);
  map[key] = item;
        
  CheckListInvariant();
}
    
template <class Key, class Value>
void khLRUMap<Key, Value>::Remove(const Key &key) {
  CheckListInvariant();

  Item *item = FindItem(key);
  if (item) {
    Unlink(item);
    map.erase(key);
    delete item;
  }

  CheckListInvariant();
}
    
template <class Key, class Value>
bool khLRUMap<Key, Value>::HasKey(const Key &key) {
  return (FindItem(key) != 0);
}

template <class Key, class Value>
bool khLRUMap<Key, Value>::Find(const Key &key, Value &returnValue) {
  Item *item = FindItem(key);
  if (item) {
    // move it to the head of the list
    Unlink(item);
    Link(item);

    CheckListInvariant();

    returnValue = item->val;
    return true;
  } else {
    return false;
  }
}

template <class Key, class Value>
khLRUMap<Key, Value>::khLRUMap(void) : head(0), tail(0), numItems(0) {
  CheckListInvariant();
}

template <class Key, class Value>
khLRUMap<Key, Value>::~khLRUMap(void) {
  Clear();
}

template <class Key, class Value>
template <class Functor>
void khLRUMap<Key, Value>::Apply(const Functor &func) {
  Item *tmp = head;
  while (tmp) {
    func(*tmp->val);
    tmp = tmp->next;
  }
}

#endif // COMMON_KHLRUMAP_H__

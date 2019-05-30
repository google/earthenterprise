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


#ifndef __khCache_h
#define __khCache_h

#include <map>
#include <vector>
#include <memory>

#include "khTypes.h"
#include "fusion/autoingest/MiscConfig.h"
#include "khRefCounter.h"
#include "SharedString.h"

// #define SUPPORT_VERBOSE
#ifdef SUPPORT_VERBOSE
#include <notify.h>
#endif

/******************************************************************************
 ***  Simple cache of refcounted objects
 ***
 ***  Key can be anything that supports less<T> and has value semantics
 ***  Value must act like a refcounting handle(like khRefGuard & khSharedHandle)
 ***     - must have value semantics
 ***     - must supply refcount() member function
 ***
 ***  You can Add, Remove & Find in the cache.
 ***  The targetMax is the max size it tries to maintain. But it can be bigger
 ***      if all the values have a refcount > 1
 ***  The refcounting handle semantics makes the pinning & unpinning automatic
 ***  A value with a refcount > 1 (because other handles to the same object
 ***      exist somewhere) will never be removed from the cache.
 ***  Oldest items (Added or Found the longest ago) will be removed first.
 ***
 ***  Note: This cache object has no notion of how to create the values.
 ***
 ***  Only one thread may access a khCache object at a time. You must protect
 ***  access with a mutex. However, if the Value type is MT safe (like
 ***  khMTRefCounter), refguards to the Values can be safely used in other
 ***  threads w/o needing to acquire the mutex that guards access to the
 ***  cache.
 ******************************************************************************/
template <class Key, class Value>
class khCacheItem {
  // private and unimplemented - no copying allowed, would mess up links
  khCacheItem(const khCacheItem&);
  khCacheItem& operator=(const khCacheItem&);
 public:
  khCacheItem *next;
  khCacheItem *prev;
  Key   key;
  Value val;
  uint64 size;
  khCacheItem(const Key &key_, const Value &val_)
      : next(0), prev(0), key(key_), val(val_), size(0) { }
};

// #define CHECK_INVARIANTS

template <class Key, class Value>
class khCache {
  typedef khCacheItem<Key, Value> Item;
  typedef std::map<Key, Item*> MapType;
  MapType map;
  Item *head;
  Item *tail;
  const uint targetMax;
#ifdef SUPPORT_VERBOSE
  bool verbose;
#endif
  uint numItems;
  uint64 cacheObjectSizes;
  bool limitCacheMemory;
  uint64 maxCacheMemory;
  const uint64 khCacheItemSize;

  bool InList(Item *item) {
    Item *tmp = head;
    while (tmp) {
      if (tmp == item)
        return true;
      tmp = tmp->next;
    }
    return false;
  }
  void CheckListInvariant(void) {
#ifdef CHECK_INVARIANTS
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
#endif
  }
  void Link(Item *item) {
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
  void Unlink(Item *item) {
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
  Item* FindItem(const Key& key) {
    typename MapType::const_iterator found = map.find(key);
    if (found != map.end()) {
      return found->second;
    } else {
      return 0;
    }
  }

 public:
  typedef typename MapType::size_type size_type;
  size_type size(void) const { return map.size(); }
  uint64 capacity(void) const { return targetMax; }
  uint64 getMemoryUse(void) const { return cacheObjectSizes; }
  uint64 itemSizeMinusKeyAndValue(void) const { return sizeof(khCacheItem<Key, Value>) - sizeof(Key) - sizeof(Value); }

  khCache(uint64 targetMax_
#ifdef SUPPORT_VERBOSE
          , bool verbose_ = false
#endif
          ) : head(0), tail(0), targetMax(targetMax_),
#ifdef SUPPORT_VERBOSE
              verbose(verbose_),
#endif
              numItems(0), cacheObjectSizes(0),
              limitCacheMemory(0),
              khCacheItemSize(itemSizeMinusKeyAndValue())

  {
    CheckListInvariant();
  }
  ~khCache(void) {
    clear();
  }
  template<class T>
  uint64 getKeyOrValueSize(const T obj) {
    //notify(NFY_WARN, "Default: %s\t%lu", typeid(obj).name(), sizeof(obj));
    return sizeof(obj);
  }
  uint64 getKeyOrValueSize(const std::string str) {
    //notify(NFY_WARN, "String: %s\t%lu\t%lu", str.c_str(), sizeof(str), (str.capacity() * sizeof(char)));
    return (sizeof(str) + (str.capacity() * sizeof(char)));
  }
  uint64 getKeyOrValueSize(const SharedString shstr) {
    return shstr.GetSharedStringSize();
  }
  template<class T>
  uint64 getKeyOrValueSize(const khRefGuard<T> obj) {
    //notify(NFY_WARN, "khRefGuard: %lu", sizeof(obj));
    return (sizeof(obj) + obj.getRefGuardSize());
  }
  void updateObjectSize(const Key &key) {
    if ( FindItem(key) != 0 ) {
      //notify(NFY_WARN, "%s", key.c_str());
      Item *item = FindItem(key);
      //notify(NFY_WARN, "%lu\t%lu\t%lu", khCacheItemSize, getKeyOrValueSize(item->key), getKeyOrValueSize(item->val));
      uint64 size = khCacheItemSize + getKeyOrValueSize(item->key) + getKeyOrValueSize(item->val);
      notify(NFY_WARN, "%s: %lu", key.c_str(), size);
      if (size > item->size) {
        if (item->size == 0) {
          notify(NFY_WARN, "NEW OBJECT");
        }
        else {
          notify(NFY_WARN, "SIZE INCREASED");
        }
        cacheObjectSizes += (size - item->size);
      }
      else if (size < item->size) {
        notify(NFY_WARN, "SIZE DECREASED");
        cacheObjectSizes -= (item->size - size);
      }
      else {
        notify(NFY_WARN, "SIZE NOT CHANGED");
      }

      item->size = size;
    }
  }
  void setCacheMemoryLimit(bool check, uint limit) {
    limitCacheMemory = check;
    maxCacheMemory = limit;
  }
  void clear(void) {
    CheckListInvariant();
    // Delete all the items.
    for (typename MapType::iterator i = map.begin(); i != map.end(); ++i) {
      assert(i->second);
      delete i->second;
      i->second = 0;
    }
    map.clear();
    head = tail = 0;
    cacheObjectSizes = 0;
  }
  void Add(const Key &key, const Value &val, bool prune = true) {
    CheckListInvariant();

    // delete any previous item
    Item *item = FindItem(key);
    if (item) {
      Unlink(item);
#ifdef SUPPORT_VERBOSE
      if (verbose) notify(NFY_ALWAYS, "Deleting previous %s from cache", key.c_str());
#endif
      delete item;
    }

    // make a new item, link it in and add to map
    item = new Item(key, val);
    Link(item);
    map[key] = item;
#ifdef SUPPORT_VERBOSE
    if (verbose) notify(NFY_ALWAYS, "Adding %s to cache", key.c_str());
#endif

    CheckListInvariant();

    // prune old items to maintain max size
    if (prune) {
      Prune();
    }
  }

  void Remove(const Key &key, bool prune = true) {
    CheckListInvariant();

    Item *item = FindItem(key);
    if (item) {
      Unlink(item);
      map.erase(key);
#ifdef SUPPORT_VERBOSE
      if (verbose) notify(NFY_ALWAYS, "Removing %s from cache", key.c_str());
#endif
      delete item;
    }

    CheckListInvariant();

    // prune old items to maintain max size
    if (prune) {
      Prune();
    }
  }

  bool Find(const Key &key, Value &val) {
    Item *item = FindItem(key);
    if (item) {
      // move it to the head of the list
      Unlink(item);
      Link(item);

      CheckListInvariant();

      val = item->val;
      return true;
    } else {
      return false;
    }
  }

  void Prune(void) {
    CheckListInvariant();
    Item *item = tail;
    while (item && ( ((cacheObjectSizes > maxCacheMemory) && limitCacheMemory) || ((map.size() > targetMax) && !limitCacheMemory) )) {
      // Note: this refcount() > 1 check is safe even with
      // khMTRefCounter based guards. See explanaition with the
      // definition of khMTRefCounter in khMTTypes.h.
      while (item && (item->val.refcount() > 1)) {
        item = item->prev;
      }
      if (item) {
        Item *tokill = item;
        item = item->prev;
        Unlink(tokill);
        map.erase(tokill->key);
#ifdef SUPPORT_VERBOSE
        if (verbose) notify(NFY_ALWAYS, "Pruning %s from cache", tokill->key.c_str());
#endif
        delete tokill;
      }
    }
    CheckListInvariant();
  }

};



#endif /* __khCache_h */

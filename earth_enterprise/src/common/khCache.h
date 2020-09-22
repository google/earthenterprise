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


#ifndef __khCache_h
#define __khCache_h

#include <cctype>
#include <chrono>
#include <cstdint>
#include <map>
#include <vector>

#include "CacheSizeCalculations.h"
#include "notify.h"

/******************************************************************************
 ***  Simple cache of refcounted objects
 ***
 ***  Key can be anything that supports less<T> and has value semantics
 ***  Value must act like a refcounting handle(like khRefGuard & khSharedHandle)
 ***     - must have value semantics
 ***     - must supply use_count() member function
 ***
 ***  You can Add, Remove & Find in the cache.
 ***  The targetMax is the max size it tries to maintain. But it can be bigger
 ***      if all the values have a use_count > 1
 ***  The refcounting handle semantics makes the pinning & unpinning automatic
 ***  A value with a use_count > 1 (because other handles to the same object
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
  std::uint64_t size;
  khCacheItem(const Key &key_, const Value &val_)
      : next(0), prev(0), key(key_), val(val_), size(0) { }
};

// #define CHECK_INVARIANTS

template <class Key, class Value>
class khCache {
  typedef khCacheItem<Key, Value> Item;
  typedef std::map<Key, Item*> MapType;
  using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
  MapType map;
  Item *head;
  Item *tail;
  static const khNotifyLevel PURGE_TIME_NOTIFY_LEVEL = NFY_DEBUG;
  const unsigned int targetMax;
  const std::string name;
#ifdef SUPPORT_VERBOSE
  bool verbose;
#endif
  std::uint64_t cacheMemoryUse;
  bool limitCacheMemory;
  std::uint64_t maxCacheMemory;
  const std::uint64_t khCacheItemSize = sizeof(khCacheItem<Key, Value>);
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
  }
  Item* FindItem(const Key& key) {
    typename MapType::const_iterator found = map.find(key);
    if (found != map.end()) {
      return found->second;
    } else {
      return 0;
    }
  }
  bool TooManyObjects() {
    return (map.size() > targetMax) && !limitCacheMemory;
  }
  bool TooMuchMemory() {
    return (cacheMemoryUse > maxCacheMemory) && limitCacheMemory;
  }

 public:
  typedef typename MapType::size_type size_type;
  size_type size(void) const { return map.size(); }
  size_type capacity(void) const { return targetMax; }
  std::uint64_t getMemoryUse(void) const { return cacheMemoryUse; }

  khCache(unsigned int targetMax_, std::string name_
#ifdef SUPPORT_VERBOSE
          , bool verbose_ = false
#endif
          ) : head(0), tail(0), targetMax(targetMax_),
              name(name_.replace(0, 1, 1, std::toupper(name_[0]))), // Make the first letter upper case for pretty notify statements
#ifdef SUPPORT_VERBOSE
              verbose(verbose_),
#endif
              cacheMemoryUse(0),
              limitCacheMemory(false)

  {
    CheckListInvariant();
  }
  ~khCache(void) {
    clear();
  }
  // returns the size of a cache item if it exists
  std::uint64_t getCacheItemSize(const Key &key) {
    Item *item = FindItem(key);
    if (item) {
      return item->size;
    }
    return 0;
  }
  // calculates the current size of a cache item
  std::uint64_t calculateCacheItemSize(Item *item) {
    return khCacheItemSize + GetHeapUsage(item->key) + GetHeapUsage(item->val);
  }
  // sets a given cache item's size to its current size and updates the total cache memory in use
  void updateCacheItemSize(const Key &key) {
    Item *item = FindItem(key);
    if ( item ) {
      std::uint64_t size = calculateCacheItemSize(item);
      cacheMemoryUse = (cacheMemoryUse - item->size) + size;
      item->size = size;
    }
  }
  void setCacheMemoryLimit(bool enabled, std::uint64_t maxMemory) {
    limitCacheMemory = enabled;
    maxCacheMemory = maxMemory;
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
    cacheMemoryUse = 0;
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
      cacheMemoryUse -= item->size;
      delete item;
    }

    // make a new item, link it in and add to map
    item = new Item(key, val);
    Link(item);
    map[key] = item;
    item->size = calculateCacheItemSize(item);
    cacheMemoryUse += item->size;
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
      cacheMemoryUse -= item->size;
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
    TimePoint startTime = std::chrono::high_resolution_clock::now();
    CheckListInvariant();
    Item *item = tail;
    while (item && ( TooMuchMemory() || TooManyObjects() )) {
      // Note: this use_count() > 1 check is safe even with
      // khMTRefCounter based guards. See explanaition with the
      // definition of khMTRefCounter in khMTTypes.h.
      while (item && (item->val.use_count() > 1)) {
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
        cacheMemoryUse -= tokill->size;
        delete tokill;
      }
    }
    CheckListInvariant();
    TimePoint finishTime = std::chrono::high_resolution_clock::now();
    // Log when the cache size is exceeded
    if (getNotifyLevel() >= PURGE_TIME_NOTIFY_LEVEL &&
        (TooMuchMemory() || TooManyObjects())) {
      LogPruneTime(startTime, finishTime);
    }
  }
 private:
  void LogPruneTime(TimePoint startTime, TimePoint finishTime) {
    std::chrono::duration<double> elapsedTime = finishTime - startTime;
    std::uint64_t configuredSize, actualSize;
    std::string units;
    if (limitCacheMemory) {
      configuredSize = maxCacheMemory;
      actualSize = cacheMemoryUse;
      units = "bytes";
    }
    else {
      configuredSize = targetMax;
      actualSize = map.size();
      units = "items";
    }
    notify(PURGE_TIME_NOTIFY_LEVEL, "%s cache size exceeded. Configured size: %lu %s, Actual size: %lu %s, Prune time: %f seconds",
           name.c_str(), configuredSize, units.c_str(), actualSize, units.c_str(), elapsedTime.count());
  }

};



#endif /* __khCache_h */

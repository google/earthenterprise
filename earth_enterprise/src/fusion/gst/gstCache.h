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

#ifndef KHSRC_FUSION_GST_GSTCACHE_H__
#define KHSRC_FUSION_GST_GSTCACHE_H__

#include <khHashTable.h>
#include <notify.h>

// brief General purpose LRU cache.
//
// Constructor sets max size, but initially the
// cache is empty.  It will grow until size reaches
// max, then the least recently used item (tail)
// will be recycled for the next new item.

extern int CacheItemCount;

template < class Type, class TypeID = int >
class gstCache {
 protected:
  class CacheItem {
   public:
    CacheItem(Type d)
        : pinned_(0),
          data_(d),
          next_(0),
          prev_(0) {
      ++CacheItemCount;
    }

    ~CacheItem() {
      --CacheItemCount;
      // cannot delete data_ here as it is a template type
      // and could very well be a built-in type such as std::uint64_t
      // delete data_;
    }

    Type data() const { return data_; }
    void data(Type d) { data_ = d; }
    const TypeID &id() const { return id_; }
    CacheItem* next() const { return next_; }
    CacheItem* prev() const { return prev_; }
    void pin() {
      ++pinned_;
    }
    void unpin() {
      --pinned_;
      if (pinned_ < 0) {
        notify(NFY_WARN, "cache item %llu unpinned more than pinned",
               static_cast<long long unsigned>(id_));
        pinned_ = 0;
      }
    }
    int pinnedCount() const { return pinned_; }

   protected:
    friend class gstCache;

    int pinned_;
    Type data_;
    TypeID id_;
    CacheItem* next_;
    CacheItem* prev_;
  };

 public:
  typedef bool (*fetchFunc)(void*, Type&, const TypeID&);

  gstCache(int maxsize)
      : max_size_(maxsize),
        size_(0),
        first_(0),
        last_(0),
        fetch_callback_(NULL) {
    hash_table_ = new khHashTable<CacheItem*, TypeID>(max_size_);
  }

  ~gstCache() {
    int count = 0;
    CacheItem* checking = last_;

    while (checking) {
      if (checking->pinnedCount())
        notify(NFY_DEBUG, "gstCache deleting cache item "
               "that is still pinned (count=%i)", checking->pinnedCount());
      checking = checking->prev_;
      ++count;
    }

    if (count != size_) {
      notify(NFY_WARN, "** gstCache LRU count (%i) doesn't match size (%i)",
             count, size_);
    }

    hash_table_->deleteAll();
    delete hash_table_;
  }

  bool full() const {
    return size_ >= max_size_;
  }

  khArray<Type>* array() {
    khArray<Type>* cArray = new khArray< Type >(hash_table_->length());
    khArray<CacheItem*>* hArray = hash_table_->array();
    for (int ii = 0; ii < hash_table_->length(); ++ii)
      cArray->append(hArray->get(ii)->data());
    delete hArray;
    return cArray;
  }

  void flush() {
    hash_table_->deleteAll();
    first_ = last_ = NULL;
    size_ = 0;
  }

  void addFetchCallback(fetchFunc func, void* obj) {
    fetch_callback_ = func;
    fetch_obj_ = obj;
  }

  Type fetch(const TypeID &id) {
    CacheItem* found = hash_table_->find(id);

    if (found != 0) {
      makeHot(found);
      return found->data();
    }

    // user function to load data into cache
    if (fetch_callback_) {
      // check to see if cache is full
      CacheItem* item = NULL;
      Type replace = Type();
      if (full() && (item = getColdest())) {
        replace = item->data();
        if ((*fetch_callback_)(fetch_obj_, replace, id)) {
          insert(item, replace, id);
        } else {
          assert(replace == item->data());
          insertColdest(item, replace, item->id());
          replace = 0;
        }
      } else {
        item = addItem();
        if ((*fetch_callback_)(fetch_obj_, replace, id)) {
          insert(item, replace, id);
        } else {
          discardItem(item);
          replace = 0;
        }
      }

      return replace;
    }

    return 0;
  }

  Type fetchAndPin(const TypeID &id) {
    CacheItem* found = hash_table_->find(id);

    if (found != 0) {
      makeHot(found);
      found->pin();
      return found->data();
    }

    // user function to load data into cache
    if (fetch_callback_) {
      // check to see if cache is full
      CacheItem* item = NULL;
      Type replace = Type();
      if (full() && (item = getColdest())) {
        replace = item->data();
        if ((*fetch_callback_)(fetch_obj_, replace, id)) {
          item->pin();
          insert(item, replace, id);
        } else {
          assert(replace == item->data());
          insertColdest(item, replace, item->id());
          replace = 0;
        }
      } else {
        item = addItem();
        if ((*fetch_callback_)(fetch_obj_, replace, id)) {
          item->pin();
          insert(item, replace, id);
        } else {
          discardItem(item);
          replace = 0;
        }
      }

      return replace;
    }

    return 0;
  }

  // insert an object directly instead of using a fetch callback
  // return value is data from the displaced cache entry (if there is one)
  Type insert(Type data, const TypeID& id) {
    // if data is NULL then don't actually insert into cache
    // (this matches behavior of using a fetch callback)
    if (data == NULL)
      return data;

    CacheItem* item = NULL;
    Type replace = Type();
    if (full() && (item = getColdest())) {
      replace = item->data();
      insert(item, data, id);
    } else {
      item = addItem();
      insert(item, data, id);
    }

    return replace;
  }

  // Pin a cache item (make unavailable for reuse)
  // Can be called multiple times to act as a reference counter.
  bool pin(TypeID id) {
    CacheItem* found = hash_table_->find(id);
    if (!found)
      return false;
    found->pin();
    return true;
  }

  // Unpin a cache item (make available for reuse)
  // Cache item is not marked as available for reuse until unpin is called
  // the same number of times as pin was called for this item.
  bool unpin(TypeID id) {
    CacheItem* found = hash_table_->find(id);
    if (!found)
      return false;
    found->unpin();
    return true;
  }

 protected:
  void discardItem(CacheItem* item) {
    delete item;
    --size_;
  }

  CacheItem* addItem() {
    CacheItem* item = new CacheItem(Type());
    ++size_;
    return item;
  }

  // insert an item into the cache
  void insert(CacheItem* item, Type data, const TypeID &id) {
    item->id_ = id;
    item->data_ = data;
    makeHot(item);
    hash_table_->insertBlind(id, item);
  }

  void insertColdest(CacheItem* item, Type data, const TypeID& id) {
    item->id_ = id;
    item->data_ = data;

    hash_table_->insertBlind(id, item);

    // fix pointers by hand
    item->prev_ = last_;
    item->next_ = NULL;
    if (last_)
      last_->next_ = item;
    last_ = item;
    if (!first_)
      first_ = item;
  }

  // Mark item as most recently used
  // item may already be in list
  void makeHot(CacheItem* item) {
    if (first_ == item) {
      return;  // item is already hot, do nothing
    }

    // if item was last, update last
    if (last_ == item)
      last_ = item->prev_;

    if (first_)
      first_->prev_ = item;

    if (item->next_)
      item->next_->prev_ = item->prev_;
    if (item->prev_)
      item->prev_->next_ = item->next_;
    item->next_ = first_;
    item->prev_ = NULL;

    first_ = item;

    if (!last_)
      last_ = item;
  }


  // get the least recently used cache entry
  CacheItem* getColdest() {
    CacheItem* checking = last_;
    while (checking) {
      if (checking->pinnedCount() == 0) {
        // item not pinned - so remove from LRU list
        if (last_ == checking)
          last_ = checking->prev_;
        if (checking->prev_)
          checking->prev_->next_ = checking->next_;
        if (checking->next_)
          checking->next_->prev_ = checking->prev_;
        checking->prev_ = NULL;
        checking->next_ = NULL;
        // remove from hash table
        hash_table_->remove(checking->id());

        return checking;
      } else {
        checking = checking->prev_;
      }
    }

    notify(NFY_WARN, "gstCache can't find unpinned item");
    return NULL;
  }

  int max_size_;
  int size_;

  CacheItem* first_;
  CacheItem* last_;

  khHashTable<CacheItem*, TypeID>* hash_table_;

  fetchFunc fetch_callback_;
  void* fetch_obj_;
};

#endif  // !KHSRC_FUSION_GST_GSTCACHE_H__

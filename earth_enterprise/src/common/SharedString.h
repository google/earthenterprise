/*
 * Copyright 2019 The Open GEE Contributors
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

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_SHAREDSTRING_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_SHAREDSTRING_H_

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <stdint.h>
#include <assert.h>
#include <common/notify.h>
#include <pthread.h>

// The goal of this class is to keep in memory only one copy of the name of
// a cached asset or asset version. Previously, each asset stored the full names
// of all of its children, parents, listeners, and inputs, resulting in tens of
// thousands of duplicates of each string in very large projects.
//
// With this class, only an integer identifier is stored in each instance (thus
// drastically reducing the amount of memory needed in large projects), and
// that identifier is used to retrieve the string value when it is requested.
// Since the string-to-identifier ratio is 1:1, equality comparisons are also 
// much faster as we only need to compare the identifiers.
class SharedString {
protected:
    class stringStorageRWLock
    {
      private:
        pthread_rwlock_t rwlock;
      public:
        stringStorageRWLock() { pthread_rwlock_init(&rwlock,NULL); }
        virtual ~stringStorageRWLock() {}
        void lockRead(void) { pthread_rwlock_rdlock(&rwlock); }
        void lockWrite(void) { pthread_rwlock_wrlock(&rwlock); }
        void unlock(void) { pthread_rwlock_unlock(&rwlock); }
    };

    class stringStorageReadGuard
    {
      private:
        stringStorageReadGuard() = delete;
        stringStorageRWLock& theLock;
      public:
        stringStorageReadGuard(stringStorageRWLock& lock) : theLock(lock) {
          theLock.lockRead();
        }
        virtual ~stringStorageReadGuard(void) { theLock.unlock(); }
    };


    class stringStorageWriteGuard
    {
      private:
        stringStorageWriteGuard() = delete;
        stringStorageRWLock& theLock;
      public:
        stringStorageWriteGuard(stringStorageRWLock& lock) : theLock(lock) {
          theLock.lockWrite();
        }
        virtual ~stringStorageWriteGuard(void) { theLock.unlock(); }
    };

    class StringStorage {
      private:
        std::unordered_map<std::uint32_t, std::string> refFromKeyTable;
        std::unordered_map<std::string, std::uint32_t> keyFromRefTable;
        static std::uint32_t nextID;
        mutable stringStorageRWLock mtx;
      public:
        const std::string & RefFromKey(const std::uint32_t &key) {
          stringStorageReadGuard lock(mtx);
          auto refIter = refFromKeyTable.find(key);
          assert(refIter != refFromKeyTable.end());
          return refIter->second;
        }
        std::uint32_t KeyFromRef(const std::string &ref) {
          stringStorageWriteGuard lock(mtx);
          auto keyIter = keyFromRefTable.find(ref);
          if (keyIter != keyFromRefTable.end()) {
            return keyIter->second;
          }
          else {
            // We've never seen this ref before, so make a new key
            std::uint32_t key = nextID++;
            refFromKeyTable.insert(std::pair<std::uint32_t, std::string>(key, ref));
            keyFromRefTable.insert(std::pair<std::string, std::uint32_t>(ref, key));
            return key;
          }
        }
        StringStorage(){
          stringStorageWriteGuard lock(mtx);
          refFromKeyTable.insert(std::pair<std::uint32_t, std::string>(0, ""));
          keyFromRefTable.insert(std::pair<std::string, std::uint32_t>("", 0));
        }
        size_t size() const {
          stringStorageReadGuard lock(mtx);
          return refFromKeyTable.size();
        }
    };

    static StringStorage strStore;
    std::uint32_t key;

    friend std::ostream & operator<<(std::ostream &out, const SharedString & ref);
  public:
    static size_t StoreSize() {
      return strStore.size();
    }

    SharedString(): key(0) {}

    SharedString(const SharedString& str): key(str.key) {
    }

    SharedString(const std::string& str) {
        key = strStore.KeyFromRef(str);
    }

    SharedString(const char* s) {
        key = strStore.KeyFromRef(s);
    }

    bool empty() const {
        return 0 == key;
    }

    SharedString & operator=(const std::string &str) {
        key = strStore.KeyFromRef(str);
        return *this;
    }

    operator std::string() const {
        return strStore.RefFromKey(key);
    }

    const std::string & toString() const {
        return strStore.RefFromKey(key);
    }

    std::uint32_t getKey() const {
       return key;
    }

    bool operator<(const SharedString &other) const {
      return (strStore.RefFromKey(key) < strStore.RefFromKey(other.key));
    }

    bool operator==(const SharedString &other) const {
      return key == other.key;
    }

    bool operator!=(const SharedString &other) const {
      return !(*this == other);
    }
};

inline std::ostream & operator<<(std::ostream &out, const SharedString & str) {
  out << SharedString::strStore.RefFromKey(str.key);
  return out;
}

inline std::string ToString(const SharedString & str) {
  return str.toString();
}

namespace std {
  template <>
  class hash<SharedString> {
    public :
      size_t operator()(const SharedString & s) const {
        return s.getKey();
      }
  };
};

#endif

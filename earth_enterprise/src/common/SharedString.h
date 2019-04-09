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

class SharedString/*: public std::string*/ {
protected:
   class RefStorage {
      private:
        std::unordered_map</*AssetDefs::AssetKey*/uint32_t, std::string> refFromKeyTable;
        std::unordered_map<std::string, /*AssetDefs::AssetKey*/uint32_t> keyFromRefTable;
        static uint32_t nextID;
      public:
        std::string RefFromKey(const /*AssetDefs::AssetKey*/uint32_t &key) {
          auto refIter = refFromKeyTable.find(key);
          assert(refIter != refFromKeyTable.end());
          return refIter->second;
        }
        /*AssetDefs::AssetKey*/uint32_t KeyFromRef(const std::string &ref) {
          auto keyIter = keyFromRefTable.find(ref);
          if (keyIter != keyFromRefTable.end()) {
            return keyIter->second;
          }
          else {
            // We've never seen this ref before, so make a new key
            /*AssetDefs::AssetKey*/uint32_t key = nextID++; // For the initial pass, key == ref
            refFromKeyTable.insert(std::pair<uint32_t, std::string>(key, ref));
            keyFromRefTable.insert(std::pair<std::string, uint32_t>(ref, key));
            return key;
          }
        }
    };

    static RefStorage refStore;
    uint32_t key;

    friend std::ostream & operator<<(std::ostream &out, const SharedString & ref);
  public:
    SharedString(): key(0) {}

    SharedString(const SharedString& str): key(str.key) {
    }

    SharedString(const std::string& str) {
        key = refStore.KeyFromRef(str);
    } 

    SharedString(const char* s) {
        key = refStore.KeyFromRef(s);
    }

    bool empty() const {
        return 0 == key;
    }
    
    operator std::string() const {
        return refStore.RefFromKey(key);
    }

    std::string toString() const {
        return refStore.RefFromKey(key);
    }

    const char* c_str() const noexcept {
      return refStore.RefFromKey(key).c_str();
    }
    
    bool operator<(const SharedString &other) const {
      return (refStore.RefFromKey(key) < refStore.RefFromKey(other.key));
    }

    bool operator==(const SharedString &other) const {
      return key == other.key;
    }

    bool operator!=(const SharedString &other) const {
      return !(*this == other);
    }
};


inline std::ostream & operator<<(std::ostream &out, const SharedString & str) {
  out << SharedString::refStore.RefFromKey(str.key);
  return out;
}

#endif

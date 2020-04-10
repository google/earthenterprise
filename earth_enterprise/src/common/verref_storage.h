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

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_VERREF_STORAGE_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_VERREF_STORAGE_H_

#include <stdlib.h>
#include <string>
#include <iterator>
#include <iostream>

#include "common/khTypes.h"
#include <cstdint>
#include "CacheSizeCalculations.h"


// Random access iterator for version reference generator.
template<class _Tp, class _Ptr, class _Ref>
struct _VerRefGenIterator :
    public std::iterator<std::random_access_iterator_tag,
                         _Tp, std::int32_t, _Ptr, _Ref> {
  typedef _VerRefGenIterator<_Tp, _Tp*, _Tp&> iterator;
  typedef _VerRefGenIterator<_Tp, const _Tp*, const _Tp&> const_iterator;

  typedef _Tp value_type;
  typedef std::int32_t difference_type;
  typedef _Ptr pointer;
  typedef _Ref reference;
  typedef std::random_access_iterator_tag iterator_category;
  typedef  _VerRefGenIterator _Self;

  _VerRefGenIterator()
      : cur_() {
  }

  explicit _VerRefGenIterator(const std::string& __x)
      : cur_(__x) {
  }

  explicit _VerRefGenIterator(const _Tp &__x)
      : cur_(__x) {
  }

  _VerRefGenIterator(const iterator &__x)
      : cur_(__x.cur_) {
  }

  ~_VerRefGenIterator() {}

  bool operator==(const _Self& __x) const {
    return (cur_ == __x.cur_);
  }

  bool operator!=(const _Self& __x) const {
    return !(*this == __x);
  }

  bool operator<(const _Self& __x) const {
    // Note: we use operator>() for comparing internal data since version
    // references in asset config is listed from latest to first one.
    return (cur_ > __x.cur_);
  }
  bool operator>(const _Self& __x) const { return __x < *this; }
  bool operator<=(const _Self& __x) const { return !(__x < *this); }
  bool operator>=(const _Self& __x) const { return !(*this < __x); }

  _Self& operator+=(const difference_type& __n) {
    cur_ -= __n;
    return (*this);
  }

  _Self& operator-=(const difference_type& __n) {
    cur_ += __n;
    return (*this);
  }

  _Self& operator++() {
    --cur_;
    return (*this);
  }

  _Self& operator--() { ++cur_; return (*this); }

  _Self operator++(std::int32_t) {
    _Self temp(*this);
    ++*this;
    return temp;
  }

  _Self operator--(std::int32_t) {
    _Self temp(*this);
    --*this;
    return temp;
  }

  _Self operator+(const difference_type& __n) const {
    _Self temp(*this);
    temp += __n;
    return temp;
  }

  _Self operator-(const difference_type& __n) const {
    _Self temp(*this);
    temp -= __n;
    return temp;
  }

  difference_type operator-(const _Self& other) const {
    return (cur_ - other.cur_);
  }

  _Self operator[](const difference_type& __n) const {
    return (*this).operator+(__n);
  }

  const std::string& operator*() const { return cur_.GetRef(); }
  const std::string* operator->() const { return &cur_.GetRef(); }

  value_type cur_;

  std::uint64_t GetHeapUsage() const {
    return ::GetHeapUsage(cur_);
  }
};

// class _VerRefDef - version reference definition.
struct _VerRefDef {
  typedef _VerRefDef _Self;
  _VerRefDef();

  explicit _VerRefDef(const std::string& ref);

  _VerRefDef(const std::string& _asset_name, std::uint32_t _ver_num);

  bool Valid() const {
    return (ver_num >= 0 && (!asset_name.empty()));
  }

  const std::string& GetRef() const;

  operator const std::string() const {
    return GetRef();
  }

  bool operator==(const _Self& __x) const {
    return (ver_num == __x.ver_num && asset_name == __x.asset_name);
  }
  bool operator!=(const _Self& __x) const {
    return (!(*this == __x));
  }

  bool operator==(const std::string& __x) const {
    return (*this == _Self(__x));
  }
  bool operator!=(const std::string& __x) const {
    return (!(*this == _Self(__x)));
  }

  bool operator<(const _Self& __x) const {
    assert(asset_name == __x.asset_name);
    return (ver_num < __x.ver_num);
  }
  bool operator>(const _Self& __x) const  { return __x < *this; }
  bool operator<=(const _Self& __x) const { return !(__x < *this); }
  bool operator>=(const _Self& __x) const { return !(*this < __x); }

  const std::string& operator*() const { return GetRef(); }

  _Self& operator+=(const std::int32_t __x) {
    ver_num += __x;
    InvalidateRef();
    return (*this);
  }
  _Self& operator-=(const std::int32_t __x) {
    ver_num -= __x;
    InvalidateRef();
    return (*this);
  }

  _Self& operator++() {
    ++ver_num;
    InvalidateRef();
    return (*this);
  }

  _Self& operator--() {
    --ver_num;
    InvalidateRef();
    return (*this);
  }

  _Self operator++(std::int32_t) {
    _Self temp(*this);
    ++*this;
    return temp;
  }

  _Self operator--(std::int32_t) {
    _Self temp(*this);
    --*this;
    return temp;
  }

  _Self operator+(const std::int32_t __x) const {
    _Self temp = *this;
    temp += __x;
    return temp;
  }

  _Self operator-(const std::int32_t __x) const {
    _Self temp = *this;
    temp -= __x;
    return temp;
  }

  std::int32_t operator-(const _Self& __x) const { return ver_num - __x.ver_num; }

  std::string asset_name;
  std::int32_t ver_num;

  std::uint64_t GetHeapUsage() const {
    return ::GetHeapUsage(asset_name)
            + ::GetHeapUsage(ref);
  }

 private:
  void InvalidateRef() const { ref.clear(); }

  // Caches a version reference which is defined when first requested.
  mutable std::string ref;
};

// Version reference generator - class with API of sequence container of
// std::string object.
class VerRefGen {
 public:
  typedef _VerRefGenIterator<_VerRefDef, _VerRefDef*, _VerRefDef&> iterator;
  typedef _VerRefGenIterator<
    _VerRefDef, const _VerRefDef*, const _VerRefDef&> const_iterator;

  typedef std::string& reference;
  typedef const std::string& const_reference;
  typedef size_t size_type;
  typedef std::int32_t difference_type;

  VerRefGen();

  explicit VerRefGen(const std::string& val);

  std::string ToString() const { return front(); }

  // The container's API.
  size_type size() const { return start_ - finish_; }

  bool empty() const { return start_ == finish_; }

  std::string front() const {
    assert(size() != 0);
    return *start_;
  }

  std::string back() const {
    assert(size() != 0);
    const_iterator _tmp = finish_;
    --_tmp;
    return *_tmp;
  }

  iterator begin() { return start_; }

  const_iterator begin() const { return start_; }

  iterator end() { return finish_; }

  const_iterator end() const { return finish_; }

  std::string operator[](size_type __n) const {
    return *start_[difference_type(__n)];
  }

  bool operator==(const VerRefGen& __x) const {
    return (start_ == __x.start_ && finish_ == __x.finish_);
  }
  bool operator!=(const VerRefGen& __x) const { return (!(*this == __x)); }

  iterator insert(iterator pos, const std::string& val) {
    // Note: Only allow to insert at the beginning of sequence.
    // It is minimal functionality we need for list of references.
    assert(pos == start_);
    if (begin() == end()) {
      // Inserting first element - initialize container.
      *this = VerRefGen(val);
    } else {
      // Update start iterator.
      iterator _old_start = start_;
      start_ = iterator(val);
      assert(_old_start > start_);
    }

    return  start_;
  }

 private:
  iterator start_;
  iterator finish_;

 public:
  std::uint64_t GetHeapUsage() const {
    return ::GetHeapUsage(start_)
            + ::GetHeapUsage(finish_);
  }
};


inline std::string ToString(const VerRefGen &val) {
  return val.ToString();
}

inline void FromString(const std::string &str, VerRefGen &val) {
  val = VerRefGen(str);
}

inline std::uint64_t GetHeapUsage(const _VerRefDef &verRefDef) {
  return verRefDef.GetHeapUsage();
}

template <class _Tp>
inline std::uint64_t GetHeapUsage(const _VerRefGenIterator<_Tp, _Tp*, _Tp&> &verRefGenIter) {
  return verRefGenIter.GetHeapUsage();
}

template <class _Tp>
inline std::uint64_t GetHeapUsage(const _VerRefGenIterator<_Tp, const _Tp*, const _Tp&> &constVerRefGenIter) {
  return constVerRefGenIter.GetHeapUsage();
}

inline std::uint64_t GetHeapUsage(const VerRefGen &verRefGen) {
  return verRefGen.GetHeapUsage();
}

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_VERREF_STORAGE_H_

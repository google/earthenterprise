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

#ifndef __khstl_h
#define __khstl_h

#include <string>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <ctype.h>


// ****************************************************************************
// ***  Is the type a reference
// ****************************************************************************
template <class T>
class ConstReferenceType {
 public:
  typedef const T& Result;
};
template <class T>
class ConstReferenceType<T&> {
 public:
  typedef const T Result;
};
template <class T>
class ConstReferenceType<const T&> {
 public:
  typedef T Result;
};



template <class T>
class ReferenceeType {
 public:
  enum { result = false };
  typedef T Result;
};
template <class T>
class ReferenceeType<T&> {
 public:
  enum { result = true };
  typedef T Result;
};

template <class T>
class IsReference {
 public:
  enum { result = ReferenceeType<T>::result };
};


// ****************************************************************************
// ***  compile time type selection
// ****************************************************************************
template <bool expression, class TrueType, class FalseType>
class SelectType {
 public:
  typedef TrueType Result;
};
template <class TrueType, class FalseType>
class SelectType<false, TrueType, FalseType> {
 public:
  typedef FalseType Result;
};



// ****************************************************************************
// ***  What type to use to pass a param
// ***
// ***  TODO: We should expand this to pass atomic types as raw
// ***  TODO: We really should be using Boost instead
// ****************************************************************************
template <class T>
class ParamType {
 public:
  typedef typename ConstReferenceType<T>::Result Result;
};



// ****************************************************************************
// ***  bind_this - binds an object pointer with a pointer to a member function
// ****************************************************************************
template <class Type, class Arg, class Result>
class unary_member_function_closure :
    public std::unary_function<Arg, Result> {
 protected:
  Type *objPtr;
  Result (Type::*memFun)(Arg);
 public:
  explicit unary_member_function_closure(Type *obj,
                                         Result (Type::*fun)(Arg)) :
      objPtr(obj), memFun(fun) { }

  Result operator()(typename ParamType<Arg>::Result x) const {
    return (objPtr->*memFun)(x);
  }
};
template <class Type, class Arg1, class Arg2, class Result>
class binary_member_function_closure :
    public std::binary_function<Arg1, Arg2, Result> {
 protected:
  Type *objPtr;
  Result (Type::*memFun)(Arg1, Arg2);
 public:
  explicit binary_member_function_closure(Type *obj,
                                          Result (Type::*fun)(Arg1, Arg2)) :
      objPtr(obj), memFun(fun) { }

  Result operator()(typename ParamType<Arg1>::Result x,
                    typename ParamType<Arg2>::Result y) const {
    return (objPtr->*memFun)(Arg1(x),Arg2(y));
  }
};


template <class Type, class Arg, class Result>
inline unary_member_function_closure<Type, Arg, Result>
unary_obj_closure(Type *obj, Result (Type::*fun)(Arg))
{
  return unary_member_function_closure<Type, Arg, Result>(obj, fun);
}
template <class Type, class Arg1, class Arg2, class Result>
inline binary_member_function_closure<Type, Arg1, Arg2, Result>
binary_obj_closure(Type *obj, Result (Type::*fun)(Arg1, Arg2))
{
  return binary_member_function_closure<Type, Arg1, Arg2, Result>(obj, fun);
}



// ****************************************************************************
// ***  mem_var() & mem_var_ref() for extracting members
// ****************************************************************************
template <class _Ret, class _Tp>
class mem_var_t : public std::unary_function<const _Tp*, const _Ret&> {
  _Ret _Tp::*pv;
 public:
  explicit mem_var_t(_Ret _Tp::*pv_) : pv(pv_) {}
  const _Ret& operator()(const _Tp* p) const { return p->*pv; }
};

template <class _Ret, class _Tp>
class mem_var_ref_t : public std::unary_function<_Tp,const _Ret&> {
  _Ret _Tp::*pv;
 public:
  explicit mem_var_ref_t(_Ret _Tp::*pv_) : pv(pv_) {}
  const _Ret& operator()(const _Tp& r) const { return r.*pv; }
};

template <class _Ret, class _Tp>
inline mem_var_t<_Ret,_Tp> mem_var(_Ret _Tp::*pv) {
  return mem_var_t<_Ret,_Tp>(pv);
}

template <class _Ret, class _Tp>
inline mem_var_ref_t<_Ret,_Tp> mem_var_ref(_Ret _Tp::*pv) {
  return mem_var_ref_t<_Ret,_Tp>(pv);
}


template <template <class t> class Op, class _Ret, class _Tp>
class mem_var_pred_t : public std::unary_function<const _Tp*,bool> {
  _Ret _Tp::*pv;
  _Ret arg;
 public:
  explicit mem_var_pred_t(_Ret _Tp::*pv_, const _Ret &a) :
      pv(pv_), arg(a) {}
  bool operator()(const _Tp* p) const { return Op<_Ret>()(p->*pv, arg); }
};

template <template <class t> class Op, class _Ret, class _Tp>
class mem_var_pred_ref_t : public std::unary_function<_Tp,bool> {
  _Ret _Tp::*pv;
  _Ret arg;
 public:
  explicit mem_var_pred_ref_t(_Ret _Tp::*pv_, const _Ret &a) :
      pv(pv_), arg(a) {}
  bool operator()(const _Tp& r) const { return Op<_Ret>()(r.*pv, arg); }
};

template <template <class t> class Op, class _Ret, class _Tp>
inline mem_var_pred_t<Op, _Ret,_Tp> mem_var_pred(_Ret _Tp::*pv, const _Ret &a) {
  return mem_var_pred_t<Op, _Ret,_Tp>(pv,a);
}

template <template <class t> class Op, class _Ret, class _Tp>
inline mem_var_pred_ref_t<Op, _Ret,_Tp> mem_var_pred_ref(_Ret _Tp::*pv, const _Ret &a) {
  return mem_var_pred_ref_t<Op, _Ret,_Tp>(pv,a);
}



template <class Map>
typename Map::mapped_type
GetMapValue(const Map &map, const typename Map::key_type &key)
{
  typedef typename Map::mapped_type mapped_type;
  typename Map::const_iterator found = map.find(key);
  if (found != map.end()) {
    return found->second;
  } else {
    return mapped_type();
  }
}

// *** not particularly efficient, so only use with lightweight
// *** objects in the vector.
template <class T>
std::vector<T>
makevec1(const T &e)
{
  return std::vector<T>(1, e);
}
template <class T>
std::vector<T>
makevec(const T &e)
{
  return std::vector<T>(1, e);
}

// *** not particularly efficient, so only use with lightweight
// *** objects in the vector.
template <class T>
std::vector<T>
makevec2(const T &e1, const T &e2)
{
  std::vector<T> vec;
  vec.reserve(2);
  vec.push_back(e1);
  vec.push_back(e2);
  return vec;
}
template <class T>
std::vector<T>
makevec(const T &e1, const T &e2)
{
  std::vector<T> vec;
  vec.reserve(2);
  vec.push_back(e1);
  vec.push_back(e2);
  return vec;
}
template <class T>
std::vector<T>
makevec3(const T &e1, const T &e2, const T &e3)
{
  std::vector<T> vec;
  vec.reserve(3);
  vec.push_back(e1);
  vec.push_back(e2);
  vec.push_back(e3);
  return vec;
}
template <class T>
std::vector<T>
makevec(const T &e1, const T &e2, const T &e3)
{
  std::vector<T> vec;
  vec.reserve(3);
  vec.push_back(e1);
  vec.push_back(e2);
  vec.push_back(e3);
  return vec;
}
template <class T>
std::vector<T>
makevec4(const T &e1, const T &e2, const T &e3, const T &e4)
{
  std::vector<T> vec;
  vec.reserve(4);
  vec.push_back(e1);
  vec.push_back(e2);
  vec.push_back(e3);
  vec.push_back(e4);
  return vec;
}
template <class T>
std::vector<T>
makevec(const T &e1, const T &e2, const T &e3, const T &e4)
{
  std::vector<T> vec;
  vec.reserve(4);
  vec.push_back(e1);
  vec.push_back(e2);
  vec.push_back(e3);
  vec.push_back(e4);
  return vec;
}

// *** not particularly efficient, so only use with lightweight
// *** objects in the vector.
template <class T>
std::set<T>
makeset(const T &e)
{
  std::set<T> tmp;
  tmp.insert(e);
  return tmp;
}
template <class T>
std::set<T>
makeset(const T &e1, const T &e2)
{
  std::set<T> tmp;
  tmp.insert(e1);
  tmp.insert(e2);
  return tmp;
}
template <class T>
std::set<T>
makeset(const T &e1, const T &e2, const T &e3)
{
  std::set<T> tmp;
  tmp.insert(e1);
  tmp.insert(e2);
  tmp.insert(e3);
  return tmp;
}
template <class T>
std::set<T>
makeset(const T &e1, const T &e2, const T &e3, const T &e4)
{
  std::set<T> tmp;
  tmp.insert(e1);
  tmp.insert(e2);
  tmp.insert(e3);
  tmp.insert(e4);
  return tmp;
}
template <class T>
std::set<T>
makeset(const T &e1, const T &e2, const T &e3, const T &e4, const T &e5)
{
  std::set<T> tmp;
  tmp.insert(e1);
  tmp.insert(e2);
  tmp.insert(e3);
  tmp.insert(e4);
  tmp.insert(e5);
  return tmp;
}
template <class T>
std::set<T>
makeset(const T &e1, const T &e2, const T &e3, const T &e4, const T &e5, const T &e6)
{
  std::set<T> tmp;
  tmp.insert(e1);
  tmp.insert(e2);
  tmp.insert(e3);
  tmp.insert(e4);
  tmp.insert(e5);
  tmp.insert(e6);
  return tmp;
}
template <class T>
std::set<T>
makeset(const T &e1, const T &e2, const T &e3, const T &e4, const T &e5, const T &e6, const T &e7)
{
  std::set<T> tmp;
  tmp.insert(e1);
  tmp.insert(e2);
  tmp.insert(e3);
  tmp.insert(e4);
  tmp.insert(e5);
  tmp.insert(e6);
  tmp.insert(e7);
  return tmp;
}

template <class Key, class Value>
std::map<Key, Value>
makemap(const Key &k1, const Value &v1)
{
  std::map<Key, Value> tmp;
  tmp[k1] = v1;
  return tmp;
}

template <class Key, class Value>
std::map<Key, Value>
makemap(const Key &k1, const Value &v1,
        const Key &k2, const Value &v2)
{
  std::map<Key, Value> tmp;
  tmp[k1] = v1;
  tmp[k2] = v2;
  return tmp;
}

template <class Key, class Value>
std::map<Key, Value>
makemap(const Key &k1, const Value &v1,
        const Key &k2, const Value &v2,
        const Key &k3, const Value &v3)
{
  std::map<Key, Value> tmp;
  tmp[k1] = v1;
  tmp[k2] = v2;
  tmp[k3] = v3;
  return tmp;
}

template <class Key, class Value>
std::map<Key, Value>
makemap(const Key &k1, const Value &v1,
        const Key &k2, const Value &v2,
        const Key &k3, const Value &v3,
        const Key &k4, const Value &v4)
{
  std::map<Key, Value> tmp;
  tmp[k1] = v1;
  tmp[k2] = v2;
  tmp[k3] = v3;
  tmp[k4] = v4;
  return tmp;
}





#if (__GNUC__ == 2) && (__GNUC_MINOR__ == 96)

inline int
stdstrcompare(const std::string &a,
              std::string::size_type pos, std::string::size_type len,
              const std::string &b) {
  return a.compare(b, pos, len);
}


#else

template <class T>
T& identity(T& t)
{
  return t;
}

inline int
stdstrcompare(const std::string &a,
              std::string::size_type pos, std::string::size_type len,
              const std::string &b) {
  return a.compare(pos, len, b);
}


#endif


inline
bool
StartsWith(const std::string &str, const std::string &find)
{
  return ((str.length() >= find.length()) &&
          (stdstrcompare(str, 0, find.length(), find) == 0));
}

inline
bool
EndsWith(const std::string &str, const std::string &find)
{
  std::string::size_type strlen = str.length();
  std::string::size_type findlen = find.length();
  return ((strlen >= findlen) &&
          (stdstrcompare(str, strlen - findlen, findlen, find) == 0));
}

inline
std::string
TrimLeadingWhite(const std::string &str)
{
  std::string::size_type begin = 0;
  std::string::size_type end   = str.size();

  while ((begin < end) && isspace(str[begin]))
    ++begin;

  if (begin == 0) {
    return str;
  } else if (begin == end) {
    return std::string();
  } else {
    return str.substr(begin, end - begin);
  }
}

inline
std::string
TrimTrailingWhite(const std::string &str)
{
  std::string::size_type begin = 0;
  std::string::size_type end   = str.size();

  while ((end > begin) && isspace(str[end-1]))
    --end;

  if (end == str.size()) {
    return str;
  } else if (begin == end) {
    return std::string();
  } else {
    return str.substr(begin, end - begin);
  }
}

inline
std::string
TrimWhite(const std::string &str)
{
  std::string::size_type begin = 0;
  std::string::size_type end   = str.size();

  while ((begin < end) && isspace(str[begin]))
    ++begin;
  while ((end > begin) && isspace(str[end-1]))
    --end;

  if ((begin == 0) && (end == str.size())) {
    return str;
  } else if (begin == end) {
    return std::string();
  } else {
    return str.substr(begin, end - begin);
  }
}

inline
std::string
TrimQuotes(const std::string &str)
{
  if ((str.size() >= 2) &&
      (((str[0] == '"') && (str[str.size()-1] == '"')) ||
       ((str[0] == '\'') && (str[str.size()-1] == '\'')))) {
    if (str.size() == 2)
      return std::string();
    else
      return str.substr(1, str.size()-2);
  }
  return str;
}

inline
std::string
AppendUnlessPresent(const std::string &str, std::string::value_type term)
{
  if (str.empty()) {
    return std::string(1, term);
  } else if (str[str.size()-1] != term) {
    return str + term;
  } else {
    return str;
  }
}


inline std::string::size_type find_and_replace(
    std::string &str,
    const std::string &target,
    const std::string &replacement,
    std::string::size_type start = 0)
{
  if (start >= str.length()) {
    return std::string::npos;
  }
  std::string::size_type found = str.find(target, start);
  if (found != std::string::npos) {
    str.replace(found, target.size(), replacement);
  }
  return found;
}


template <class T, class OutIter>
void split(const std::string &str, const T &t, OutIter oi,
           std::string::size_type start = 0)
{
  size_t seplen = std::string(t).length();
  if (start >= str.length()) return;
  std::string::size_type found;
  do {
    found = str.find(t, start);
    oi++ = str.substr(start, found - start);
    start = found + seplen;
  } while (found != std::string::npos);
}

// This is for vector/deque/set... type string containers. Usage example:
// const std::vector<std::string> my_vec(2, "x");
// std::string result =
//     join(my_vec.begin(), my_vec.end(), std::string("_");
// assert(result == "x_x")
template <class Iterator>
std::string
join(Iterator begin, Iterator end, const std::string &sep)
{
  std::string result;
  if (begin == end) {
    return result;
  }
  result = *begin;
  for (; ++begin != end;) {
    result += sep;
    result += *begin;
  }
  return result;
}

#endif /* __khstl_h */

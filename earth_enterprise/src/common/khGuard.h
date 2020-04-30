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


#ifndef _khGuard_h_
#define _khGuard_h_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/mman.h>
#include <vector>
#include <assert.h>
#include <khTypes.h>
#include <cstdint>
#include "notify.h"
#include <cerrno>


// ****************************************************************************
// ***  Delete helpers
// ****************************************************************************
template <class T>
class SingleDeleter {
 public:
  static void Delete(T* ptr) { delete ptr; }
};

template <class T>
class ArrayDeleter {
 public:
  static void Delete(T* ptr) { delete[] ptr; }
};


template <class T, template <class U> class deleter = SingleDeleter >
class khDeleteGuard;

// ****************************************************************************
// ***  Guard to make transer of ownership explicit
// ***
// ***  You can't create one of these directly, use TransferOwnership()
// ****************************************************************************
template <class T>
class khTransferGuard {
 private:
  mutable T* ptr;

 public:
  explicit inline khTransferGuard(T *p = 0) : ptr(p) { }
  explicit inline khTransferGuard(khDeleteGuard<T>& deleter);
  inline khTransferGuard &operator=(const khTransferGuard &o) {
    if (this != &o) {
      operator=(o.take());
    }
    return *this;
  }
  inline khTransferGuard(const khTransferGuard &o) : ptr(o.take()) { }

  // allow copying between compatible transfer guards
  template <class U>
  inline khTransferGuard(const khTransferGuard<U> &o) : ptr(o.take()) { }

  inline ~khTransferGuard(void) { delete ptr; }

  inline T* operator->(void) const { return ptr; }
  inline T& operator* (void) const { return *ptr; }
  inline operator bool(void) const { return ptr != 0;}
  inline bool operator==(const khTransferGuard &o) const {
    return ptr == o.ptr;
  }
  inline bool operator!=(const khTransferGuard &o) const {
    return !operator==(o);
  }
  inline khTransferGuard &operator=( T *p ) {
    if (ptr != p) delete ptr;
    ptr = p;
    return *this;
  }

  inline T* take(void) const {
    T *tmp = ptr;
    ptr = 0;
    return tmp;
  }
};

template <class T>
inline khTransferGuard<T>
TransferOwnership(T *ptr) { return khTransferGuard<T>(ptr); }

template <class T>
inline khTransferGuard<T>
TransferOwnership(khDeleteGuard<T>& deleter) {
  return khTransferGuard<T>(deleter);
}


// ****************************************************************************
// ***  khDeleteGuard
// ****************************************************************************
template <class T, template <class U> class deleter>
class khDeleteGuard 
{
 public:
  inline khDeleteGuard( const khTransferGuard<T> &p)
      : ptr(p.take()) { }
  inline khDeleteGuard( void ) : ptr( 0 ) { }
  inline ~khDeleteGuard( ) { deleter<T>::Delete(ptr); }

#if 0
  // DO NOT ENABLE THESE
  // With these disabled users are required to use TransferOwnership()
  // which is exception safe and semantics reminding
  inline khDeleteGuard( T *p = 0 ) : ptr( p ) { }
  inline khDeleteGuard &operator=( T *p ) {
    if (ptr != p) deleter<T>::Delete(ptr);
    ptr = p;
    return *this;
  }
#endif

  inline khDeleteGuard &operator=( const khTransferGuard<T> &p ) {
    // our ownership semantics make it impossible to the incoming transfer
    // guard to hold the same pointer as I do. So there is no need to check.
    clear();
    ptr = p.take();
    return *this;
  }

  inline void clear(void) {
    if (ptr) deleter<T>::Delete(ptr);
    ptr = 0;
  }

  inline T *take() { 
    T *tmp = ptr;
    ptr = 0;
    return tmp;
  }

  inline T* operator->(void) const { return ptr; }
  inline T& operator* (void) const { return *ptr; }

  inline operator T*(void) const { return ptr; }
  inline operator bool(void) const { return ptr != 0; }
  inline bool operator!(void) const { return ptr == 0; }
  inline bool operator==(const khDeleteGuard &o) const {
    return ptr == o.ptr;
  }
  inline bool operator!=(const khDeleteGuard &o) const {
    return !operator==(o);
  }

 private:
  T *ptr;

  // private and unimplemented - it's illegal to copy or assign
  khDeleteGuard &operator=( const khDeleteGuard & );
  khDeleteGuard( const khDeleteGuard &);

  // intentionally unimplemented - never called but provides ambiguity
  // if somebody does the following
  //    khDeleteGuard<Foo> foo(new Foo());
  //    ...;
  //    delete foo;   <-- will be ambiguous, causing compiler to disallow
  // Unfortunately it also means you can't to this.
  //    khDeleteGuard<Foo> foo(new Foo());
  //    void *v = foo;
  // But that's rare anyway, and you can always do this.
  //    void *v = (Foo*)foo;
  operator void*(void) const;
};


template <class T>
inline
khTransferGuard<T>::khTransferGuard(khDeleteGuard<T>& deleter) :
    ptr(deleter.take()) { }


template< class T > 
class khDeletingVector : private std::vector<T*>
{
 public:
  typedef std::vector<T*> Base;
  typedef typename Base::size_type       size_type;
  typedef typename Base::const_reference const_reference;
  typedef typename Base::const_iterator  const_iterator;

  khDeletingVector(typename std::vector<T*>::size_type presize) :
      Base(presize) { }
  khDeletingVector(void) { }
  ~khDeletingVector(void) {
    for (typename std::vector<T*>::const_iterator iter = this->begin();
         iter != this->end(); ++iter) {
      delete *iter;
    }
  }

  // Non const methods - forward only if you know it preserves the ownership
  // requirements of this class.
  // Specifically dont expose the non-const operator[] and at() since they
  // could be misued to break the ownership semantics
  void reserve(size_type r)  { Base::reserve(r); }
  void push_back(T *t)       { Base::push_back(t); }
  void push_back(const khTransferGuard<T> &t) {
    // resize first - ensures we can grow before releasing claim on t
    resize(size()+1);
    Base::operator[](size()-1) = t.take();
  }
  void pop_back(void)        { Base::pop_back(); }
  void resize(size_type newSize) {
    for (size_type  i = newSize; i < this->size(); ++i) {
      delete this->operator[](i);
    }
    Base::resize(newSize);
  }
  void clear(void) { resize(0); }
  void erase(size_type pos) {
    delete Base::operator[](pos);
    Base::erase(Base::begin() + pos);
  }
  void Assign(size_type pos, T* t) {
    T *curr = Base::operator[](pos);
    if (curr != t) {
      delete curr;
    }
    Base::operator[](pos) = t;
  }
  T* Take(size_type pos) {
    T *curr = Base::operator[](pos);
    Base::operator[](pos) = 0;
    return curr;
  }
  void Transfer(khDeletingVector &other) {
    other.reserve(other.size() + size());
    for (unsigned int i = 0; i < size(); ++i) {
      other.push_back(Take(i));
    }
    resize(0);
  }

  // Forward various const methods - feel free to add to this list as necessary
  size_type       size(void)      const { return Base::size(); }
  bool            empty(void)     const { return Base::empty(); }
  const_iterator  begin(void)     const { return Base::begin(); }
  const_iterator  end(void)       const { return Base::end(); }
  const_reference back(void)      const { return Base::back(); }
  const_reference at(size_type n) const { return Base::at(n); }
  const_reference operator[](size_type n) const { return Base::operator[](n); }

 private:
  // private and unimplemented - it's illegal to copy or assign
  khDeletingVector &operator=(const khDeletingVector &);
  khDeletingVector(const khDeletingVector &);
};

// ****************************************************************************
// ***  Guard to free a pointer
// ****************************************************************************
class khFreeGuard 
{
 public:
  khFreeGuard( void *p = 0 ) : _ptr( p ) { }
  ~khFreeGuard( )
  {
    if (_ptr) free (_ptr);
  }

  khFreeGuard &operator=( void *p )
  {
    if (_ptr && _ptr != p) free (_ptr);
    _ptr = p;
    return *this;
  }

  void* ptr(void) { return _ptr;}
  void *take() 
  { 
    void *tmp = _ptr;
    _ptr = 0;
    return tmp;
  }

 private:
  void *_ptr;
  khFreeGuard &operator=( const khFreeGuard & );
  khFreeGuard( const khFreeGuard &);
};

class khReadFileCloser
{
  // private and unimplemented
  khReadFileCloser(const khReadFileCloser &);
  khReadFileCloser& operator=(const khReadFileCloser &);
 public:
  khReadFileCloser( int fd = -1 ) : fileDesc( fd ) { }
  ~khReadFileCloser() { 
    close();
  }
    
  khReadFileCloser& operator=(int fd) {
    if (fileDesc != fd) {
      close();
      fileDesc = fd;
    }
    return *this;
  }

  bool valid(void) const { return fileDesc != -1; }
  void release() { fileDesc = -1 ; }
  void close(void) {
    if (fileDesc != -1) {
      (void) ::close(fileDesc);
      fileDesc = -1;
    }
  }

  int fd(void) const { return fileDesc; }
 private:
  int fileDesc;
};

class khWriteFileCloser
{
  // private and unimplemented
  khWriteFileCloser(const khWriteFileCloser &);
  khWriteFileCloser& operator=(const khWriteFileCloser &);
 public:
  khWriteFileCloser( int fd = -1 ) : fileDesc( fd ) { }
  ~khWriteFileCloser() { 
    close();
  }
    
  khWriteFileCloser& operator=(int fd) {
    if (fileDesc != fd) {
      close();
      fileDesc = fd;
    }
    return *this;
  }

  bool valid(void) const { return fileDesc != -1; }
  void release() { fileDesc = -1 ; }
  void close(void) {
    if (fileDesc != -1) {
      if (::fsync(fileDesc) == -1) {
        // some file types don't support fsync, don't whine about
        // them
        if ((errno != EINVAL) && (errno != EROFS)) {
          notify(NFY_WARN, "fsync error: %s",
                 khstrerror(errno).c_str());
        }
      }
      (void) ::close(fileDesc);
      fileDesc = -1;
    }
  }

  int fd(void) const { return fileDesc; }
 private:
  int fileDesc;
};

class khFILECloser
{
  FILE *file;
 public:
  khFILECloser(FILE *file_) : file(file_) { }
  ~khFILECloser(void) { 
    if (file)
      ::fclose(file);
  }
  bool close() {
    if (file) {
      bool ret = (::fclose(file) == 0);
      file = 0;
      return ret;
    }
    return true;
  }
  void release() { file = 0 ; }
};

class khDIRCloser
{
  DIR *dir;
 public:
  khDIRCloser(DIR *dir_) : dir(dir_) { }
  ~khDIRCloser(void) { 
    if (dir)
      ::closedir(dir);
  }
  void release() { dir = 0 ; }
};


// ****************************************************************************
// ***  ArgumentStorage
// ***
// ***  Used by the CallGuards below to know how to store their bound arguments
// ***
// ***  Be careful with pointers and non-const references, since we can't
// ***  guarantee their lifecycles
// ****************************************************************************
template <class T>
class ArgumentStorage {
 public:
  typedef T Type;
};

// for const references, store the raw type, because the arg may have only
// been a temporary and could go away leaving us a bad reference
template <class T>
class ArgumentStorage<const T &> {
 public:
  typedef T Type;
};

// for references, generate a compile time error, the handling is ambiguous
// should I store the ref to an object with guaranteed lifetime or should
// I store a copy.
// Change the receiving frunction to take a pointer, a const ref, or the full
// type
template <class T>
class ArgumentStorage<T &> {
};


// ****************************************************************************
// ***  khCallGuard
// ***
// ***  Simple classes to ensure that a routine gets called with the given
// ***  argument.
// ****************************************************************************
template <class ArgType, class RetType = void>
class khCallGuard
{
  RetType (*fn)(ArgType);
  typename ArgumentStorage<ArgType>::Type arg;
 public:
  khCallGuard(RetType (*fun)(ArgType), ArgType a) :
      fn(fun), arg(a) { }
  ~khCallGuard(void) { (void)fn(arg); }
};


template <class ObjType, class RetType = void>
class khMemFunCallGuard
{
  typedef RetType (ObjType::*FuncType)(void);
  typedef RetType (ObjType::*ConstFuncType)(void) const;


  ObjType *obj;
  FuncType fn;
 public:
  khMemFunCallGuard(ObjType *o, FuncType fun):
      obj(o), fn(fun) { }
  khMemFunCallGuard(ObjType *o, ConstFuncType fun):
      obj(o), fn(FuncType(fun)) { }
  ~khMemFunCallGuard(void) { (void)(obj->*fn)(); }
};

template <class ObjType, class ArgType, class RetType = void>
class khMemFun1CallGuard
{
  ObjType *obj;
  RetType (ObjType::*fn)(ArgType);
  typename ArgumentStorage<ArgType>::Type arg;
 public:
  khMemFun1CallGuard(ObjType *o, RetType (ObjType::*fun)(ArgType),ArgType a):
      obj(o), fn(fun), arg(a) { }
  ~khMemFun1CallGuard(void) { (void)(obj->*fn)(arg); }
};


// ****************************************************************************
// ***  khMunmapGuard
// ****************************************************************************
class khMunmapGuard
{
  void *filebuf;
  size_t maplen;

  // private and unimplemted -- illegal to copy
  khMunmapGuard(const khMunmapGuard&);
  khMunmapGuard& operator=(const khMunmapGuard&);
 public:
  void *buffer(void) { return filebuf; }

  khMunmapGuard(void)
      : filebuf(MAP_FAILED), maplen(0) { }
  khMunmapGuard(void *buf, size_t len)
      : filebuf(buf), maplen(len) { }

  void DelayedSet(void *buf, size_t len) {
    assert(filebuf == MAP_FAILED);
    filebuf = buf;
    maplen = len;
  }

  ~khMunmapGuard(void) {
    if (filebuf != MAP_FAILED) {
      (void)msync(filebuf, maplen, MS_SYNC);
      (void)munmap(filebuf, maplen);
    }
  }
};

#endif // !_khGuard_h_

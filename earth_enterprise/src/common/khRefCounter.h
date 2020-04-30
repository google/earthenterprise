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

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_KHREFCOUNTER_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_KHREFCOUNTER_H_

#include <assert.h>
//#include "common/khTypes.h"
#include <cstdint>
#include "common/khGuard.h"
#include "common/khThreadingPolicy.h"
#include "common/khCppStd.h"


// ****************************************************************************
// ***  khRefGuard<T>
// ***
// ***
// ***  Refcounting handle class
// ***  There are _strict_ construction rules to eliminate the possibility of
// ***  this class being used improperly.
// ***
// ***  Example usage:
// ***
// ***     v----- valid usage -----v
// ***  khRefGuard<Foo> foo = khRefGuardFromNew(new Foo(...));
// ***
// ***     v----- invalid usage -----v
// ***  Foo *foo = new Foo(...);
// ***  khRefGuard<Foo> foo2(foo2);  // will generate compile time error
// ***
// ***
// ***  If you are in a member function of foo and need to a handle to
// ***  yourself you call khrefGuardFromThis:
// ***  void Foo::Register(void) {
// ***       khRefGuard<Foo> handle = khRefGuardFromThis();
// ***       Manager::Register(handle);
// ***  }
// ***
// ****************************************************************************

// part of implementation details - defined below
template <class T> class khUnrefNewGuard_;
template <class T> class khRerefThisGuard_;

template<class T>
class khRefGuard {
 private:
  T *ptr;

  // Don't allow construction or assignment directly from pointer
  // This allows the caller to forget to unref the pointer
  // e.g. khRefGuard<Foo> guard(new Foo()) <-- Bad! Bad! Bad!
  // Do something like this instead:
  // khRefGuard<Foo> foo = khRefGuardFromNew(new Foo(...));
  khRefGuard(T *p);         // private and unimplemented


 public:
  typedef T RefType;

  inline T* operator->(void) const { return ptr; }
  inline T& operator* (void) const { return *ptr; }
  inline operator bool(void) const { return ptr != 0;}
  inline bool operator==(const khRefGuard &o) const {
    return ptr == o.ptr;
  }
  inline bool operator!=(const khRefGuard &o) const {
    return !operator==(o);
  }

  // expose refcount function from my shared object
  inline std::uint32_t refcount(void) const  { return ptr ? ptr->refcount() : 0; }
  inline std::uint32_t use_count(void) const { return refcount(); }

  inline void release(void) {
    if (ptr) {
      ptr->unref();
      ptr = 0;
    }
  }
  inline khRefGuard(void) : ptr(0) { }
  inline ~khRefGuard(void) { release(); }

  // Allow construction/assignment from my UnrefNewGuard
  // This is used internally from khRefGuardFromNew
  inline explicit khRefGuard(const khUnrefNewGuard_<T> &o);
  inline khRefGuard& operator=(const khUnrefNewGuard_<T> &o);

  // Allow construction/assignment from my RerefThisGuard
  // This is used internally from khRefGuardFromThis
  inline explicit khRefGuard(const khRerefThisGuard_<T> &o);
  inline khRefGuard& operator=(const khRerefThisGuard_<T> &o);

  // assignment & copy constructor
  khRefGuard(const khRefGuard &o) : ptr(o.ptr) { if (ptr) ptr->ref(); }
  khRefGuard& operator=(const khRefGuard &o) {
    if (o.ptr) o.ptr->ref();
    if (ptr) ptr->unref();
    ptr = o.ptr;
    return *this;
  }

#ifdef GEE_HAS_MOVE
  // move assignment & copy constructor
  khRefGuard(khRefGuard &&o) noexcept : ptr(o.ptr) { o.ptr = nullptr; }
  khRefGuard& operator=(khRefGuard &&o) noexcept {
    if (this != &o) {
      T* ptr_ = ptr;
      ptr = o.ptr;
      o.ptr = nullptr;
      if (ptr_) ptr_->unref();
    }
    return *this;
  }
#endif // GEE_HAS_MOVE

  // 2 assignments is cheaper than 2 mutex locks. So in case you don't care
  // about b, a.swap(b) is faster than a = b
  void swap(khRefGuard &o) {
    T* tmp = o.ptr;
    o.ptr = ptr;
    ptr = tmp;
  }

  // assignment and constructor from compatible khRefGuards
  // If the pointer's are compatible the khRefGuards will be too
  // It the pointer's are not compatible, you'll get a cryptic compile-time
  // error :-)
  template <class U>
  explicit khRefGuard(const khRefGuard<U> &o)
      : ptr(&*o) {
    if (ptr) ptr->ref();
  }

  template <class U>
  khRefGuard& operator=(const khRefGuard<U> &o) {
    U* optr = &*o;
    if (optr) optr->ref();
    if (ptr) ptr->unref();
    ptr = optr;
    return *this;
  }

  // assign from dynamic cast compatible khRefGuards
  template <class U>
  khRefGuard& dyncastassign(const khRefGuard<U> &o) {
    if (ptr) {
      ptr->unref();
    }
    ptr = dynamic_cast<T*>(&*o);
    if (ptr) {
      ptr->ref();
    }
    return *this;
  }
};

template <class T> inline khRefGuard<T> khRefGuardFromNew(T *newobj);
template <class T> inline khRefGuard<T> khRefGuardFromThis_(T *thisobj);
#define khRefGuardFromThis() khRefGuardFromThis_(this)
#define AnotherRefGuardFromRaw(x) khRefGuardFromThis_(x)



// ****************************************************************************
// ***  khRefCounter
// ***
// ***  Base class to add ref counting lifetime control to another class.
// ***  Classes derived from khRefCounter should only be accessed via
// ***  khRefGuard and his cousins.
// ***
// ***  Please dont use the khRefCounterImpl class directly. Use the
// ***  khRefCounter typedef instead. A typedef for the MT-safe variant
// ***  (khMTRefCounter) can be found in khMTTypes.h
// ****************************************************************************
template <class ThreadPolicy>
class khRefCounterImpl : public ThreadPolicy::MutexHolder {
 private:
  mutable std::uint32_t refcount_;

 protected:
  // protected and implemented to do nothing
  // classes derived from khRefCounterImpl can decide if they want to support
  // copy/move operations on their state but this base class state should
  // do nothing during these operations as this state is tied to memory management
  // and not really object state.
  // NOTE: should move to using shared_ptr<> and make_shared<> when we have C++11
  // as this gives nearly all the same bennifets without these oddities.
  khRefCounterImpl(const khRefCounterImpl &) : refcount_(1) {
    // intentionally left empty.  The right thing to do is nothing.
  }
  khRefCounterImpl & operator=(const khRefCounterImpl&) {
    // intentionally left empty.  The right thing to do is nothing.
    return *this;
  }

#if GEE_HAS_MOVE
  khRefCounterImpl(khRefCounterImpl &&) noexcept : refcount_(1) {
    // intentionally left empty.  The right thing to do is nothing.
  }
  khRefCounterImpl & operator=(khRefCounterImpl&&) noexcept {
    // intentionally left empty.  The right thing to do is nothing.
    return *this;
  }
#endif // GEE_HAS_MOVE

  inline khRefCounterImpl(void) : refcount_(1) { }

  // virtual to force my derivations to be virtual this is needed since this
  // base deletes itself
  virtual ~khRefCounterImpl(void) { }

 private:
  template <class T> friend class khRefGuard;
  template <class T> friend class khUnrefNewGuard_;
  typedef typename ThreadPolicy::LockGuard LockGuard;

  inline void ref(void) const {
    LockGuard guard(this);
    ++refcount_;
  }
  inline void unref(void) const {
    bool deleteme = false;
    {
      LockGuard guard(this);
      assert(refcount_);
      deleteme = (--refcount_ == 0);
    }
    // the delete has to be done after the mutex is unlocked, or it will
    // try to unlock it after it has been destroyed. This is safe to do
    // w/o the lock, since the only way deleteme can be set is if there
    // are no more references. And if there are no more references,
    // nobody else can be trying to ref/unref. Unless of course another
    // thread is accesing this object through the same guard at the same
    // time, but that's bad news anyway.
    if (deleteme)
      delete this;
  }

 public:
  std::uint32_t refcount(void) const {
    LockGuard guard(this);
    return refcount_;
  }

  std::uint32_t use_count(void) const {
      return refcount();
  }
};
typedef khRefCounterImpl<SingleThreadingPolicy> khRefCounter;



// Used to wrap a new ref counted object while putting into a ref guard
// This can only be made from the khRefGuardFromNew function
template <class T>
class khUnrefNewGuard_ {
 private:
  friend class khRefGuard<T>;
  friend khRefGuard<T> khRefGuardFromNew<T>(T *newobj);
  // should only be accessed from khRefGuardFromNew
  inline explicit khUnrefNewGuard_(T *p) : ptr( p ) { }
  // private and unimplemented - shouldn't be used
  khUnrefNewGuard_& operator=(const khUnrefNewGuard_ &);

  mutable T *ptr;

 public:
  // copy semantics transfer ownership to new one
  // required to exists (and be public) by new C++ rules regarding
  // the initialization of const & from temporary objects
  khUnrefNewGuard_(const khUnrefNewGuard_ &o) {
    ptr = o.ptr;
    o.ptr = 0;
  }
  inline ~khUnrefNewGuard_(void) { if (ptr) ptr->unref(); }
};

// Used to wrap 'this' when making another khRefGuard for an already
// existing object.
// This can only be made from the khRefGuardFromThis_ function
template <class T>
class khRerefThisGuard_ {
 private:
  friend class khRefGuard<T>;
  friend khRefGuard<T> khRefGuardFromThis_<T>(T *newobj);
  // should only be accessed from khRefGuardFromThis_
  inline explicit khRerefThisGuard_(T *p) : ptr(p) { }

  T *ptr;

 public:
  // default copy and assignement operators are fine. this is wrapped
  // only to changed the signature of the khRefGuard constructor.
  // default destructor is fine too.
};

template <class T>
inline khRefGuard<T>::khRefGuard(const khUnrefNewGuard_<T> &o)
    : ptr(o.ptr) {
  if (ptr) ptr->ref();
}

template <class T>
inline khRefGuard<T>&
khRefGuard<T>::operator=(const khUnrefNewGuard_<T> &o) {
  if (o.ptr) o.ptr->ref();
  if (ptr) ptr->unref();
  ptr = o.ptr;
  return *this;
}

template <class T>
inline khRefGuard<T>::khRefGuard(const khRerefThisGuard_<T> &o)
    : ptr(o.ptr) {
  if (ptr) ptr->ref();
}

template<class T>
inline khRefGuard<T>&
khRefGuard<T>::operator=(const khRerefThisGuard_<T> &o) {
  if (o.ptr) o.ptr->ref();
  if (ptr) ptr->unref();
  ptr = o.ptr;
  return *this;
}


template <class T>
inline khRefGuard<T>
khRefGuardFromNew(T *newobj) {
  return khRefGuard<T>(khUnrefNewGuard_<T>(newobj));
}

template <class T>
inline khRefGuard<T>
khRefGuardFromThis_(T *thisobj) {
  return khRefGuard<T>(khRerefThisGuard_<T>(thisobj));
}

// ****************************************************************************
// ***  like khRefGuard but for objects w/o native ref counting ability
// ***
// *** usage:
// *** khSharedHandle<T> h1(TransferOwnership(new T()));
// *** ...
// *** khSharedhandle<T> h2 = h1;
// *** ...
// *** khSharedhandle<T> h3 = h2;
// ***
// *** h1, h2, & h3 all point to the same copy of T. The last one to go out
// *** of scope will take the copy of T with it.
// ***
// *** Note: You cannot create a khSharedHandle directly from a T*, you must
// *** pass it through a call to TransferOwnership. This requirment reminds
// *** develpers that ownership of the pointer is being passed to the
// *** khSharedHandle
// ****************************************************************************
template <class T, class RefClass = khRefCounter>
class khSharedHandle {
  typedef khTransferGuard<T> TransferGuard;

  struct Impl : public RefClass {
    khDeleteGuard<T, SingleDeleter > ptr;
    inline Impl(const TransferGuard &guard) : ptr(guard) { }
    inline ~Impl(void) { }
  };
  khRefGuard<Impl> impl;

 public:
  // compiler-generated copy constructor and assignment operator are fine

  khSharedHandle(void) { }

  inline explicit khSharedHandle(const TransferGuard &guard) :
      impl(guard ? khRefGuardFromNew(new Impl(guard)) :khRefGuard<Impl>()) {}
  inline khSharedHandle& operator=(const TransferGuard &guard) {
    impl = guard ? khRefGuardFromNew(new Impl(guard)) :khRefGuard<Impl>();
    return *this;
  }

  inline T* operator->(void) const { return impl->ptr; }
  inline T& operator* (void) const { return *(impl->ptr); }
  inline operator bool(void) const { return impl;}
  inline bool operator==(const khSharedHandle &o) const {
    return ((!impl && !o.impl) || (impl->ptr == o.impl->ptr));
  }
  inline bool operator!=(const khSharedHandle &o) const {
    return !operator==(o);
  }

  // expose refcount function from my shared object
  inline std::uint32_t refcount(void) const  { return impl ? impl->refcount() : 0; }
  inline std::uint32_t use_count(void) const { return refcount(); }
};



#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_KHREFCOUNTER_H_

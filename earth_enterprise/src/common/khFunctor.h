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

#ifndef __khFunctor_h
#define __khFunctor_h

#include "khGuard.h"
#include <functional>

// ****************************************************************************
// ***  STL <functional> extensions
// ****************************************************************************
namespace std {
// ***** ternary functions *****
template <class _Arg1, class _Arg2, class _Arg3, class _Result>
struct ternary_function {
  typedef _Arg1 first_argument_type;
  typedef _Arg2 second_argument_type;
  typedef _Arg3 third_argument_type;
  typedef _Result result_type;
};

template <class _Ret, class _Tp, class _Arg1, class _Arg2>
class mem_fun2_t : public ternary_function<_Tp*,_Arg1,_Arg2,_Ret> {
 public:
  explicit mem_fun2_t(_Ret (_Tp::*__pf)(_Arg1,_Arg2)) : _M_f(__pf) {}
  _Ret operator()(_Tp* __p, _Arg1 __x, _Arg2 __y) const {
    return (__p->*_M_f)(__x, __y);
  }
 private:
  _Ret (_Tp::*_M_f)(_Arg1, _Arg2);
};

template <class _Ret, class _Tp, class _Arg1, class _Arg2>
class const_mem_fun2_t : public ternary_function<_Tp*,_Arg1,_Arg2,_Ret> {
 public:
  explicit const_mem_fun2_t(_Ret (_Tp::*__pf)(_Arg1,_Arg2) const) :
      _M_f(__pf) {}
  _Ret operator()(const _Tp* __p, _Arg1 __x, _Arg2 __y) const {
    return (__p->*_M_f)(__x, __y);
  }
 private:
  _Ret (_Tp::*_M_f)(_Arg1, _Arg2) const;
};

template <class _Ret, class _Tp, class _Arg1, class _Arg2>
inline mem_fun2_t<_Ret,_Tp,_Arg1,_Arg2>
mem_fun(_Ret (_Tp::*__f)(_Arg1,_Arg2)) {
  return mem_fun2_t<_Ret,_Tp,_Arg1,_Arg2>(__f);
}

template <class _Ret, class _Tp, class _Arg1, class _Arg2>
inline const_mem_fun2_t<_Ret,_Tp,_Arg1,_Arg2>
mem_fun(_Ret (_Tp::*__f)(_Arg1,_Arg2) const) {
  return const_mem_fun2_t<_Ret,_Tp,_Arg1,_Arg2>(__f);
}


// ***** quaternary functions *****
template <class _Arg1, class _Arg2, class _Arg3, class _Arg4, class _Result>
struct quaternary_function {
  typedef _Arg1 first_argument_type;
  typedef _Arg2 second_argument_type;
  typedef _Arg3 third_argument_type;
  typedef _Arg4 fourth_argument_type;
  typedef _Result result_type;
};

template <class _Ret, class _Tp, class _Arg1, class _Arg2, class _Arg3>
class mem_fun3_t : public quaternary_function<_Tp*,_Arg1,_Arg2,_Arg3,_Ret> {
 public:
  explicit mem_fun3_t(_Ret (_Tp::*__pf)(_Arg1,_Arg2,_Arg3)) : _M_f(__pf) {}
  _Ret operator()(_Tp* __p, _Arg1 __x, _Arg2 __y, _Arg3 __z) const {
    return (__p->*_M_f)(__x, __y, __z);
  }
 private:
  _Ret (_Tp::*_M_f)(_Arg1, _Arg2, _Arg3);
};

template <class _Ret, class _Tp, class _Arg1, class _Arg2, class _Arg3>
class const_mem_fun3_t :
      public quaternary_function<_Tp*,_Arg1,_Arg2,_Arg3,_Ret> {
 public:
  explicit const_mem_fun3_t(_Ret (_Tp::*__pf)(_Arg1,_Arg2,_Arg3) const) :
      _M_f(__pf) {}
  _Ret operator()(const _Tp* __p, _Arg1 __x, _Arg2 __y, _Arg3 __z) const {
    return (__p->*_M_f)(__x, __y, __z);
  }
 private:
  _Ret (_Tp::*_M_f)(_Arg1, _Arg2, _Arg3) const;
};

template <class _Ret, class _Tp, class _Arg1, class _Arg2, class _Arg3>
inline mem_fun3_t<_Ret,_Tp,_Arg1,_Arg2,_Arg3>
mem_fun(_Ret (_Tp::*__f)(_Arg1,_Arg2,_Arg3)) {
  return mem_fun3_t<_Ret,_Tp,_Arg1,_Arg2,_Arg3>(__f);
}

template <class _Ret, class _Tp, class _Arg1, class _Arg2, class _Arg3>
inline const_mem_fun3_t<_Ret,_Tp,_Arg1,_Arg2,_Arg3>
mem_fun(_Ret (_Tp::*__f)(_Arg1,_Arg2,_Arg3) const) {
  return const_mem_fun3_t<_Ret,_Tp,_Arg1,_Arg2,_Arg3>(__f);
}

}

// ****************************************************************************
// ***  khFunctor
// ***
// ***  Simplistic closure object for delaying a call
// ***  This object does NOT use khRefCounter for it's impl objects because
// ***  one of the main purposes of the object is to pass commands to another
// ***  thread and ref counting across threads can be quite difficult to get
// ***  right.
// ****************************************************************************
template <class RetType = void>
class khFunctor {
 public:
  typedef RetType result_type;

 private:
  class Impl /* NOT khRefCounter since functors are often used
                between multiple threads */ {
   public:
    virtual RetType operator()(void) const = 0;
    virtual Impl* clone(void) const = 0;
    virtual ~Impl(void) { }
  };

  template <class Fun>
  class BindNoneImpl : public Impl {
    Fun fun;
   public:
    BindNoneImpl(const Fun &fun_) : fun(fun_) { }
    virtual RetType operator()(void) const {
      return (RetType)fun();
    }
    virtual Impl* clone(void) const {
      return new BindNoneImpl(*this);
    }
  };

  template <class Fun>
  class BindOneImpl : public Impl {
    typedef typename ArgumentStorage<typename Fun::argument_type>::Type StorageType;
    Fun fun;
    StorageType arg;
   public:
    BindOneImpl(const Fun &fun_, typename Fun::argument_type arg_) :
        fun(fun_), arg(arg_) { }
    virtual RetType operator()(void) const {
      return (RetType)fun(arg);
    }
    virtual Impl* clone(void) const {
      return new BindOneImpl(*this);
    }
  };

  template <class Fun>
  class BindTwoImpl : public Impl {
    typedef typename ArgumentStorage<typename Fun::first_argument_type>::Type  StorageType1;
    typedef typename ArgumentStorage<typename Fun::second_argument_type>::Type StorageType2;
    Fun fun;
    StorageType1 arg1;
    StorageType2 arg2;
   public:
    BindTwoImpl(const Fun &fun_,
                typename Fun::first_argument_type arg1_,
                typename Fun::second_argument_type arg2_) :
        fun(fun_), arg1(arg1_), arg2(arg2_) { }
    virtual RetType operator()(void) const {
      return (RetType)fun(arg1, arg2);
    }
    virtual Impl* clone(void) const {
      return new BindTwoImpl(*this);
    }
  };

  template <class Fun>
  class BindThreeImpl : public Impl {
    typedef typename ArgumentStorage<typename Fun::first_argument_type>::Type  StorageType1;
    typedef typename ArgumentStorage<typename Fun::second_argument_type>::Type StorageType2;
    typedef typename ArgumentStorage<typename Fun::third_argument_type>::Type StorageType3;
    Fun fun;
    StorageType1 arg1;
    StorageType2 arg2;
    StorageType3 arg3;
   public:
    BindThreeImpl(const Fun &fun_,
                  typename Fun::first_argument_type arg1_,
                  typename Fun::second_argument_type arg2_,
                  typename Fun::third_argument_type arg3_) :
        fun(fun_), arg1(arg1_), arg2(arg2_), arg3(arg3_) { }
    virtual RetType operator()(void) const {
      return (RetType)fun(arg1, arg2, arg3);
    }
    virtual Impl* clone(void) const {
      return new BindThreeImpl(*this);
    }
  };

  template <class Fun>
  class BindFourImpl : public Impl {
    typedef typename ArgumentStorage<typename Fun::first_argument_type>::Type  StorageType1;
    typedef typename ArgumentStorage<typename Fun::second_argument_type>::Type StorageType2;
    typedef typename ArgumentStorage<typename Fun::third_argument_type>::Type StorageType3;
    typedef typename ArgumentStorage<typename Fun::fourth_argument_type>::Type StorageType4;
    Fun fun;
    StorageType1 arg1;
    StorageType2 arg2;
    StorageType3 arg3;
    StorageType4 arg4;
   public:
    BindFourImpl(const Fun &fun_,
                 typename Fun::first_argument_type arg1_,
                 typename Fun::second_argument_type arg2_,
                 typename Fun::third_argument_type arg3_,
                 typename Fun::fourth_argument_type arg4_) :
        fun(fun_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_) { }
    virtual RetType operator()(void) const {
      return (RetType)fun(arg1, arg2, arg3, arg4);
    }
    virtual Impl* clone(void) const {
      return new BindFourImpl(*this);
    }
  };

  khDeleteGuard<Impl> impl;
 public:
  RetType operator()(void) const {
    if (impl)
      return (*impl)();
    else
      return RetType();
  }


  bool IsNoOp(void) const { return !(bool)impl; }

  // empty constructor - a No-Op, can also be tested for with IsNoUp()
  khFunctor(void) : impl() { }

  // copy and assignment operators to get clone semantics
  // for our impl
  khFunctor(const khFunctor &o) : impl(
      TransferOwnership(o.impl ? o.impl->clone() : 0)) { }
  khFunctor& operator=(const khFunctor &o) {
    impl = TransferOwnership(o.impl ? o.impl->clone() : 0);
    return *this;
  }

  template <class Fun>
  khFunctor(const Fun &fun_) :
      impl(TransferOwnership(new BindNoneImpl<Fun>(fun_))) { }

  template <class Fun>
  khFunctor(const Fun &fun_,
            typename Fun::argument_type arg1_) :
      impl(TransferOwnership(new BindOneImpl<Fun>(fun_, arg1_))) { }

  template <class Fun>
  khFunctor(const Fun &fun_,
            typename Fun::first_argument_type arg1_,
            typename Fun::second_argument_type arg2_) :
      impl(TransferOwnership(new BindTwoImpl<Fun>(fun_, arg1_, arg2_))) { }

  template <class Fun>
  khFunctor(const Fun &fun_,
            typename Fun::first_argument_type arg1_,
            typename Fun::second_argument_type arg2_,
            typename Fun::third_argument_type arg3_) :
      impl(TransferOwnership(
               new BindThreeImpl<Fun>(fun_, arg1_, arg2_, arg3_))) { }

  template <class Fun>
  khFunctor(const Fun &fun_,
            typename Fun::first_argument_type arg1_,
            typename Fun::second_argument_type arg2_,
            typename Fun::third_argument_type arg3_,
            typename Fun::fourth_argument_type arg4_) :
      impl(TransferOwnership(
               new BindFourImpl<Fun>(fun_, arg1_, arg2_, arg3_, arg4_))) { }
};


// The argument passed to the operator () will be used as first argument
template <class ArgType, class RetType = void>
class khFunctor1 {
 public:
  typedef ArgType argument_type;
  typedef RetType result_type;

 private:
  class Impl /* NOT khRefCounter since functors are often used
                between multiple threads */ {
   public:
    virtual RetType operator()(ArgType a) const = 0;
    virtual Impl* clone(void) const = 0;
    virtual ~Impl(void) { }
  };

  template <class Fun>
  class BindNoneImpl : public Impl {
    Fun fun;
   public:
    BindNoneImpl(const Fun &fun_) : fun(fun_) { }
    virtual RetType operator()(ArgType a) const {
      return (RetType)fun(a);
    }
    virtual Impl* clone(void) const {
      return new BindNoneImpl(*this);
    }
  };

  template <class Fun>
  class BindOneImpl : public Impl {
    typedef typename ArgumentStorage<typename Fun::second_argument_type>::Type StorageType;
    Fun fun;
    StorageType arg;
   public:
    BindOneImpl(const Fun &fun_, typename Fun::second_argument_type arg_) :
        fun(fun_), arg(arg_) { }
    virtual RetType operator()(ArgType a) const {
      return (RetType)fun(a, arg);
    }
    virtual Impl* clone(void) const {
      return new BindOneImpl(*this);
    }
  };

  template <class Fun>
  class BindTwoImpl : public Impl {
    typedef typename ArgumentStorage<typename Fun::second_argument_type>::Type StorageType1;
    typedef typename ArgumentStorage<typename Fun::third_argument_type>::Type StorageType2;
    Fun fun;
    StorageType1 arg1;
    StorageType2 arg2;
   public:
    BindTwoImpl(const Fun &fun_,
                typename Fun::second_argument_type arg1_,
                typename Fun::third_argument_type arg2_) :
        fun(fun_), arg1(arg1_), arg2(arg2_) { }
    virtual RetType operator()(ArgType a) const {
      return (RetType)fun(a, arg1, arg2);
    }
    virtual Impl* clone(void) const {
      return new BindTwoImpl(*this);
    }
  };

  khDeleteGuard<Impl> impl;
 public:
  RetType operator()(ArgType a) const {
    if (impl)
      return (*impl)(a);
    else
      return RetType();
  }

  bool IsNoOp(void) const { return !(bool)impl; }

  // empty constructor - a No-Op, can also be tested for with IsNoUp()
  khFunctor1(void) : impl() { }

  // copy and assignment operators to get clone semantics
  // for our impl
  khFunctor1(const khFunctor1 &o) : impl(
      TransferOwnership(o.impl ? o.impl->clone() : 0)) { }
  khFunctor1& operator=(const khFunctor1 &o) {
    impl = TransferOwnership(o.impl ? o.impl->clone() : 0);
    return *this;
  }

  template <class Fun>
  khFunctor1(const Fun &fun_) :
      impl(TransferOwnership(new BindNoneImpl<Fun>(fun_))) { }

  template <class Fun>
  khFunctor1(const Fun &fun_,
             typename Fun::second_argument_type arg1_) :
      impl(TransferOwnership(new BindOneImpl<Fun>(fun_, arg1_))) { }

  template <class Fun>
  khFunctor1(const Fun &fun_,
             typename Fun::second_argument_type arg1_,
             typename Fun::third_argument_type arg2_) :
      impl(TransferOwnership(new BindTwoImpl<Fun>(fun_, arg1_, arg2_))) { }
};


// The argument passed to the operator () will be used as last argument
template <class ArgType, class RetType = void>
class khFunctor1Last {
 public:
  typedef ArgType argument_type;
  typedef RetType result_type;

 private:
  class Impl /* NOT khRefCounter since functors are often used
                between multiple threads */ {
   public:
    virtual RetType operator()(ArgType a) const = 0;
    virtual Impl* clone(void) const = 0;
    virtual ~Impl(void) { }
  };

  template <class Fun>
  class BindNoneImpl : public Impl {
    Fun fun;
   public:
    BindNoneImpl(const Fun &fun_) : fun(fun_) { }
    virtual RetType operator()(ArgType a) const {
      return (RetType)fun(a);
    }
    virtual Impl* clone(void) const {
      return new BindNoneImpl(*this);
    }
  };

  template <class Fun>
  class BindOneImpl : public Impl {
    typedef typename ArgumentStorage<typename Fun::first_argument_type>::Type StorageType;
    Fun fun;
    StorageType arg;
   public:
    BindOneImpl(const Fun &fun_, typename Fun::first_argument_type arg_) :
        fun(fun_), arg(arg_) { }
    virtual RetType operator()(ArgType a) const {
      return (RetType)fun(arg, a);
    }
    virtual Impl* clone(void) const {
      return new BindOneImpl(*this);
    }
  };

  template <class Fun>
  class BindTwoImpl : public Impl {
    typedef typename ArgumentStorage<typename Fun::first_argument_type>::Type StorageType1;
    typedef typename ArgumentStorage<typename Fun::second_argument_type>::Type StorageType2;
    Fun fun;
    StorageType1 arg1;
    StorageType2 arg2;
   public:
    BindTwoImpl(const Fun &fun_,
                typename Fun::first_argument_type arg1_,
                typename Fun::second_argument_type arg2_) :
        fun(fun_), arg1(arg1_), arg2(arg2_) { }
    virtual RetType operator()(ArgType a) const {
      return (RetType)fun(arg1, arg2, a);
    }
    virtual Impl* clone(void) const {
      return new BindTwoImpl(*this);
    }
  };

  khDeleteGuard<Impl> impl;
 public:
  RetType operator()(ArgType a) const {
    if (impl)
      return (*impl)(a);
    else
      return RetType();
  }

  bool IsNoOp(void) const { return !(bool)impl; }

  // empty constructor - a No-Op, can also be tested for with IsNoUp()
  khFunctor1Last(void) : impl() { }

  // copy and assignment operators to get clone semantics
  // for our impl
  khFunctor1Last(const khFunctor1Last &o) : impl(
      TransferOwnership(o.impl ? o.impl->clone() : 0)) { }
  khFunctor1Last& operator=(const khFunctor1Last &o) {
    impl = TransferOwnership(o.impl ? o.impl->clone() : 0);
    return *this;
  }

  template <class Fun>
  khFunctor1Last(const Fun &fun_) :
      impl(TransferOwnership(new BindNoneImpl<Fun>(fun_))) { }

  template <class Fun>
  khFunctor1Last(const Fun &fun_,
             typename Fun::first_argument_type arg1_) :
      impl(TransferOwnership(new BindOneImpl<Fun>(fun_, arg1_))) { }

  template <class Fun>
  khFunctor1Last(const Fun &fun_,
             typename Fun::first_argument_type arg1_,
             typename Fun::second_argument_type arg2_) :
      impl(TransferOwnership(new BindTwoImpl<Fun>(fun_, arg1_, arg2_))) { }
};

#endif /* __khFunctor_h */

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

//
// Low level wrapper around SpiderMonkey libjs
// These objects are not intended to be used outside the libjs directory

#ifndef COMMON_KHJS_KHJSIMPL_H__
#define COMMON_KHJS_KHJSIMPL_H__

// Thin wrapper around the spidermonkey public api header
// It defines all of the necessary CPP macros and then includes the original
// jsapi.h
#include <cstdint>
#include <gejsapi.h>
#include <qstring.h>
#include <qstringlist.h>
#include <khMTTypes.h>

// ****************************************************************************
// ***  predeclarations to avoid recursive #include's
// ****************************************************************************
class KHJSContextImpl;
typedef khRefGuard<KHJSContextImpl> KHJSContext;


// ****************************************************************************
// ***  from KHJSContext.cpp
// ****************************************************************************
QString MakeJSContextErrorMessage(struct JSContext *cx, const QString &header);


// ****************************************************************************
// ***  KHJSRuntime
// ****************************************************************************
class KHJSRuntime {
 public:
  JSRuntime *runtime;

  KHJSRuntime(void);
  ~KHJSRuntime(void);

  static KHJSRuntime* Singleton(void);
 private:
  // private and unimplemented
  KHJSRuntime(const KHJSRuntime &);
  KHJSRuntime& operator=(const KHJSRuntime &);
};


// ****************************************************************************
// ***  JSContextUser
// ***
// ***  Guard class that wraps all exposed JSAPI calls to make sure they are
// ***  never called outside a BeginRequest/EndRequest pair.
// ****************************************************************************
class JSContextUser {
  friend class JSLocalRootScopeGuard;
 public:
  JSContextUser(const KHJSContext &cx_);
  ~JSContextUser(void);

  void AddNamedRoot(void *rp, const std::string &name);
  void RemoveRoot(void *rp) throw();
  void ClearErrors(void);
  void* GetPrivate(JSObject *obj);
  void SetPrivate(JSObject *obj, void *priv);
  bool IsIdentifier(const QString &str);
  void ExecuteScript(JSScript *script, jsval *rval);
  bool    ValueToBoolean(jsval v);
  QString ValueToString(jsval val);
  std::int32_t   ValueToInt32(jsval v);
  std::uint32_t  ValueToUint32(jsval v);
  float64 ValueToFloat64(jsval v);
  inline QString MakeErrorMessage(const QString &header) {
    return MakeJSContextErrorMessage(rawcx, header);
  }
  void throwError(const QString &header);
  void FindGlobalFunctionNames(QStringList &fnames);
  void DefineReadOnlyPropertyWithTinyId(JSObject *obj, const QString &name,
                                        std::int8_t tinyid, JSPropertyOp getter);
  void AddNamedGlobalObject(const QString &name, JSObject *obj);
  void DeleteNamedGlobalObject(const QString &name);
  void MaybeGC(void);

 private:
  // holds context busy so it can't go away while we're using it
  KHJSContext cx;

  // extract these so their quick to get to
  JSContext * const rawcx;
  JSObject  * const rawglobal;
};


// ****************************************************************************
// ***  AddRootGuard
// ****************************************************************************
class JSAddRootGuard {
 public:
  inline JSAddRootGuard(JSContextUser &jsuser_,
                        void *rp_, const std::string &name) :
      jsuser(jsuser_), rp(rp_)
  {
    jsuser.AddNamedRoot(rp, name);
  }
  inline ~JSAddRootGuard(void) {
    jsuser.RemoveRoot(rp);
  }
 private:
  JSContextUser &jsuser;
  void *rp;
};

// ****************************************************************************
// ***  JSLocalRootScopeGuard
// ***
// ***  Guard class that wraps all JSAPI calls that allocate new GC-able
// ***  things. This makes sure they are never called outside a
// ***  EnterLocalRootScope/LeaveLocalRootScope pair. This in turn makes
// ***  sure that the newborn things are not GC'd before we're finised
// ***  hooking them up
// ****************************************************************************
class JSLocalRootScopeGuard {
 public:
  JSLocalRootScopeGuard(JSContextUser &jsuser_);
  ~JSLocalRootScopeGuard(void);
  JSObject* CompileScript(const QString &scriptText,
                          const QString &errorContext);

  // If parent is null, use context global as parent
  JSObject* NewObject(JSClass *clasp, JSObject *parent = 0);

 private:
  JSContextUser &jsuser;
};


#endif // COMMON_KHJS_KHJSIMPL_H__

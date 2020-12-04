// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//

#include "KHJSImpl.h"
#include "KHJSContext.h"
#include <khException.h>


// ****************************************************************************
// ***  KHJSRuntime
// ****************************************************************************
KHJSRuntime::KHJSRuntime(void) :
    runtime(JS_NewRuntime(32 * 1024 * 1024))
{
  if (!runtime) {
    throw khException("Unable to create Javascript runtime");
  }
}
KHJSRuntime::~KHJSRuntime(void) {
  (void)JS_DestroyRuntime(runtime);
}
KHJSRuntime* KHJSRuntime::Singleton(void) {
  static KHJSRuntime* singleton = 0;
  if (!singleton) {
    singleton = new KHJSRuntime();
  }
  return singleton;
}

// ****************************************************************************
// ***  JSContextUser
// ****************************************************************************
JSContextUser::JSContextUser(const KHJSContext &cx_) :
    cx(cx_),
    rawcx(cx->cx_),
    rawglobal(JS_GetGlobalObject(rawcx))
{
  JS_BeginRequest(rawcx);
}
JSContextUser::~JSContextUser(void) {
  JS_EndRequest(rawcx);
}

void JSContextUser::AddNamedRoot(void *rp, const std::string &name) {
  if (!JS_AddNamedRoot(rawcx, rp, name.c_str())) {
    throwError(kh::tr("Unable to add root for %1 object").arg(name.c_str()));
  }
}

void JSContextUser::RemoveRoot(void *rp) throw() {
  (void)JS_RemoveRoot(rawcx, rp);
}

void JSContextUser::ClearErrors(void) {
  (void)JS_ClearPendingException(cx->cx_);
  cx->pendingErrorMsg = QString();
}

void* JSContextUser::GetPrivate(JSObject *obj) {
  return JS_GetPrivate(rawcx, obj);
}

void JSContextUser::SetPrivate(JSObject *obj, void* priv) {
  (void) JS_SetPrivate(rawcx, obj, priv);
}

bool JSContextUser::IsIdentifier(const QString &str) {
  return JS_IsIdentifier(rawcx, str.ucs2(), str.length());
}

void JSContextUser::ExecuteScript(JSScript *script, jsval *rval) {
  ClearErrors();
  if (!JS_ExecuteScript(rawcx, rawglobal, script, rval)) {
    throwError(kh::tr("Error running script"));
  }
}

bool JSContextUser::ValueToBoolean(jsval v) {
  JSBool b = JS_FALSE;
  (void)JS_ValueToBoolean(rawcx, v, &b);
  return b;
}

std::int32_t JSContextUser::ValueToInt32(jsval v) {
  std::int32_t i = 0;
  (void)JS_ValueToECMAInt32(rawcx, v, &i);
  return i;
}

 std::uint32_t JSContextUser::ValueToUint32(jsval v) {
  std::uint32_t u = 0;
  (void)JS_ValueToECMAUint32(rawcx, v, &u);
  return u;
}

float64 JSContextUser::ValueToFloat64(jsval v) {
  float64 f = 0.0;
  (void)JS_ValueToNumber(rawcx, v, &f);
  return f;
}

QString JSContextUser::ValueToString(jsval val) {
  JSString *str = JS_ValueToString(rawcx, val);
  JSAddRootGuard guard(*this, &str, "Temporary String");
  if (str) {
    return QString((QChar*)JS_GetStringChars(str), JS_GetStringLength(str));
  } else {
    return QString();
  }
}

void JSContextUser::throwError(const QString &header) {
  throw khException(MakeErrorMessage(header));
}

void JSContextUser::FindGlobalFunctionNames(QStringList &fnames) {
  JSObject *glob = rawglobal;
  JSObject *iter = JS_NewPropertyIterator(rawcx, glob);
  if (!iter) {
    throwError(kh::tr("Cannot make new property iterator"));
  }

  // Make sure my iterator doesn't get GC'd while I'm still using it
  JSAddRootGuard guard(*this, &iter, "Property Iterator");

  // traverse each property
  jsid propid;
  while (JS_NextProperty(rawcx, iter, &propid) && (propid != JSVAL_VOID)) {
    jsval propval;

    // use property jsid to get the jsval for the property
    if (JS_GetJSIDProperty(rawcx, glob, propid, &propval)) {
      if (JSVAL_IS_OBJECT(propval)) {
        JSObject *obj;
        // We have to check ourselves if the value is a function before
        // calling JS_ValueToFunction. Otherwise it might emit warnings
        // when the jsval isn't really a function
        if (JS_ValueToObject(rawcx, propval, &obj) &&
            JS_ObjectIsFunction(rawcx, obj)) {
          JSFunction *func = JS_ValueToFunction(rawcx, propval);

          // I'm not sure if this can ever fail. But there are so many
          // different paths through the SpiderMonkey code that I just feel
          // like being safe
          if (func) {
            JSString *str = JS_GetFunctionId(func);
            if (str) {
              JSAddRootGuard(*this, &str, "Temporary String");
              fnames.push_back(QString((QChar*)JS_GetStringChars(str),
                                       JS_GetStringLength(str)));
            }
          }
        }
      }
    }
  }
}

void JSContextUser::DefineReadOnlyPropertyWithTinyId
(JSObject *obj, const QString &name, std::int8_t tinyid, JSPropertyOp getter)
{
  if (!JS_DefineUCPropertyWithTinyId(rawcx, obj,
                                     name.ucs2(), name.length(),
                                     tinyid,
                                     0 /* default jsval */,
                                     getter,
                                     0 /* setter */,
                                     JSPROP_READONLY)) {
    throwError(kh::tr("Unable to add properry %1").arg(name));
  }
}

void JSContextUser::AddNamedGlobalObject(const QString &name, JSObject *obj)
{
  if (!JS_DefineUCProperty(rawcx, rawglobal,
                           name.ucs2(), name.length(),
                           OBJECT_TO_JSVAL(obj),
                           0 /* getter */, 0 /* setter */,
                           JSPROP_READONLY)) {
    throwError(kh::tr("Unable to global object %1").arg(name));
  }
}

void JSContextUser::DeleteNamedGlobalObject(const QString &name)
{
  jsval rval;
  (void) JS_DeleteUCProperty2(rawcx, rawglobal,
                              name.ucs2(), name.length(),
                              &rval);
}

void JSContextUser::MaybeGC(void)
{
  (void) JS_MaybeGC(rawcx);
}

// ****************************************************************************
// ***  JSLocalRootScopeGuard
// ****************************************************************************
JSLocalRootScopeGuard::JSLocalRootScopeGuard(JSContextUser &jsuser_) :
    jsuser(jsuser_) {
  if (!JS_EnterLocalRootScope(jsuser.rawcx)) {
    jsuser.throwError(kh::tr("Unable to enter local root scope"));
  }
}
JSLocalRootScopeGuard::~JSLocalRootScopeGuard(void) {
  JS_LeaveLocalRootScope(jsuser.rawcx);
}


JSObject* JSLocalRootScopeGuard::CompileScript(const QString &scriptText,
                                               const QString &errorContext) {
  jsuser.ClearErrors();
  JSScript *script =
    JS_CompileUCScript(jsuser.rawcx, jsuser.rawglobal,
                       scriptText.ucs2(), scriptText.length(),
                       errorContext.latin1(), 0 /* linenum */);
  if (!script) {
    jsuser.throwError(kh::tr("Error compiling script"));
  }

  JSObject *obj = JS_NewScriptObject(jsuser.rawcx, script);
  if (!obj) {
    // extract the error msg first in case JS_DestroyScript changes it
    QString msg =
      jsuser.MakeErrorMessage(kh::tr("Unable to wrap script in object"));

    // nobody else owns the script yet, so we need to clean it up
    (void)JS_DestroyScript(jsuser.rawcx, script);

    throw khException(msg);
  }

  return obj;
}

JSObject* JSLocalRootScopeGuard::NewObject(JSClass *clasp, JSObject *parent) {
  if (parent == 0) {
    parent = jsuser.rawglobal;
  }
  JSObject *obj = JS_NewObject(jsuser.rawcx, clasp, 0 /* prototype */, parent);
  if (!obj) {
    jsuser.throwError(kh::tr("Unable to make new %1 object")
                      .arg(clasp->name));
  }
  return obj;
}

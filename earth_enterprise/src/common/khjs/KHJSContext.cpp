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

#include "KHJSContext.h"
#include "KHJSImpl.h"
#include <khException.h>


// ****************************************************************************
// ***  Global utility functions
// ***
// ***  There should be very few of these. Most functions should be in
// ***  JSContextUser, but there are a handful of routines that need to be
// ***  called before a JSContextUser can be constructed.
// ****************************************************************************
QString MakeJSContextErrorMessage(struct JSContext *cx,
                                  const QString &header) {
  QString ret = header + ": ";

  if (JS_IsExceptionPending(cx)) {
    ret += "Exception pending";
  } else {
    KHJSContextImpl *khcx = (KHJSContextImpl*)JS_GetContextPrivate(cx);

    // make sure we're still attached. We may have gone away.
    if (khcx && !khcx->pendingErrorMsg.isEmpty()) {
      ret += QString("\n") + khcx->pendingErrorMsg;
      khcx->pendingErrorMsg = QString();
    }
  }

  return ret;
}


// ****************************************************************************
// ***  KJSScript
// ****************************************************************************
KHJSScriptImpl::KHJSScriptImpl(const KHJSContext &cx_,
                               const QString &scriptText,
                               const QString &errorContext) :
    cx(cx_), scriptObj(0)
{
  // Give this thread permission to execute in the context
  JSContextUser jsuser(cx);

  // Make sure all newborns can't be GC'd until we're done here.
  JSLocalRootScopeGuard rootGuard(jsuser);

  // compile the script
  scriptObj = rootGuard.CompileScript(scriptText, errorContext);

  // protect scriptObj from GC
  jsuser.AddNamedRoot(&scriptObj, "KHJSScript");
}

KHJSScriptImpl::~KHJSScriptImpl(void)
{
  // Give this thread permission to execute in the context
  JSContextUser jsuser(cx);

  // remove scriptObj from GC root list. The script should get finalized on
  // the next GC run.
  jsuser.RemoveRoot(&scriptObj);
}




// ****************************************************************************
// ***  KHJSContext
// ****************************************************************************
KHJSContextImpl::KHJSContextImpl(void) :
    cx_(JS_NewContext(KHJSRuntime::Singleton()->runtime,
                      4096 /* stackChunkSize */)),
    branchCallbackCount(0)
{
  if (!cx_) {
    throw khException(kh::tr("Unable to create javascript context"));
  }

  // no other thread could possibly have a handle to this context yet
  // so we don't need to enter a JSContextUser. That's good since
  // we're not finished constructing yet, and the JSContextUser wants
  // a refguard for this object.
  JS_SetContextPrivate(cx_, this);
  (void) JS_SetErrorReporter(cx_, &cbErrorHandler);
  (void) JS_SetBranchCallback(cx_, &cbBranchCallback);

  // make the global object
  JSObject *glob = JS_NewObject(cx_, 0, 0, 0);
  if (!glob) {
    QString msg = MakeJSContextErrorMessage
                  (cx_, kh::tr("Unable to create global javascript object"));

    // clean up before we leave
    (void) JS_DestroyContext(cx_);

    throw khException(msg);
  }

  // populate the global object. This will also make it the root object
  // for the context
  if (!JS_InitStandardClasses(cx_, glob)) {
    QString msg = MakeJSContextErrorMessage
                  (cx_, "Unable to initialize global javascript object");

    // clean up before we leave
    (void) JS_DestroyContext(cx_);

    throw khException(msg);
  }

  //  (void) JS_SetOptions(cx_, JSOPTION_STRICT|JSOPTION_WERROR);
}
KHJSContextImpl::~KHJSContextImpl(void)
{
  // clear the back pointer before we start to tear it down
  JS_SetContextPrivate(cx_, 0);

  (void)JS_DestroyContext(cx_);
}

void
KHJSContextImpl::HandleError(const char *msg, JSErrorReport *report)
{
  // Give this thread permission to execute in the context
  JSContextUser jsuser(khRefGuardFromThis());

  QString fullMsg = QString("%1\n%2 line %3")
                    .arg(msg)
                    .arg(report->filename?report->filename:"")
                    .arg(report->lineno+1);

  pendingErrorMsg = fullMsg;
  notify(NFY_DEBUG, "Javascript HandleError: %s", fullMsg.latin1());
}

void
KHJSContextImpl::cbErrorHandler(JSContext *cx, const char *message,
                                JSErrorReport *report)
{
  // We don't have a JSContextUser, but nobody will ever change the context
  // private data, so there is no race.
  KHJSContextImpl *khcx = (KHJSContextImpl*)JS_GetContextPrivate(cx);

  // make sure we're still attached. We may have gone away.
  if (khcx) {
    khcx->HandleError(message, report);
  }
}

void
KHJSContextImpl::HandleBranchCallback(void)
{
  if (branchCallbackCount & 0x000001ff) {
    // Give this thread permission to execute in the context
    JSContextUser jsuser(khRefGuardFromThis());

    jsuser.MaybeGC();
  }
}

int
KHJSContextImpl::cbBranchCallback(struct JSContext *cx, JSScript *script)
{
  // We don't have a JSContextUser, but nobody will ever change the context
  // private data, so there is no race.
  KHJSContextImpl *khcx = (KHJSContextImpl*)JS_GetContextPrivate(cx);

  // make sure we're still attached. We may have gone away.
  if (khcx) {
    khcx->HandleBranchCallback();
  }

  return JS_TRUE;
}

KHJSContext
KHJSContextImpl::Create(void)
{
  return khRefGuardFromNew(new KHJSContextImpl());
}

bool
KHJSContextImpl::IsIdentifier(const QString &str)
{
  // Give this thread permission to execute in the context
  JSContextUser jsuser(khRefGuardFromThis());

  return jsuser.IsIdentifier(str);
}

void
KHJSContextImpl::FindGlobalFunctionNames(QStringList &fnames)
{
  // Give this thread permission to execute in the context
  JSContextUser jsuser(khRefGuardFromThis());
  return jsuser.FindGlobalFunctionNames(fnames);
}

KHJSScript
KHJSContextImpl::CompileScript(const QString &scriptText,
                               const QString &errorContext)
{
  // JSContextUser will be acquired by KHJSScriptImpl
  return khRefGuardFromNew(new KHJSScriptImpl(khRefGuardFromThis(),
                                              scriptText, errorContext));
}

void
KHJSContextImpl::ExecuteScript(const KHJSScript &script)
{
  // Give this thread permission to execute in the context
  JSContextUser jsuser(khRefGuardFromThis());
  jsval unused;
  jsuser.ExecuteScript((JSScript*)jsuser.GetPrivate(script->scriptObj),
                       &unused);
}

void
KHJSContextImpl::ExecuteScript(const KHJSScript &script, bool &ret)
{
  // Give this thread permission to execute in the context
  JSContextUser jsuser(khRefGuardFromThis());
  jsval rval;
  jsuser.ExecuteScript((JSScript*)jsuser.GetPrivate(script->scriptObj), &rval);
  ret = jsuser.ValueToBoolean(rval);
}

void
KHJSContextImpl::ExecuteScript(const KHJSScript &script, QString &ret)
{
  // Give this thread permission to execute in the context
  JSContextUser jsuser(khRefGuardFromThis());
  jsval rval;
  jsuser.ExecuteScript((JSScript*)jsuser.GetPrivate(script->scriptObj), &rval);
  ret = jsuser.ValueToString(rval);
}


void
KHJSContextImpl::ExecuteScript(const KHJSScript &script, std::int32_t &ret)
{
  // Give this thread permission to execute in the context
  JSContextUser jsuser(khRefGuardFromThis());
  jsval rval;
  jsuser.ExecuteScript((JSScript*)jsuser.GetPrivate(script->scriptObj), &rval);
  ret = jsuser.ValueToInt32(rval);
}

void
KHJSContextImpl::ExecuteScript(const KHJSScript &script, std::uint32_t &ret)
{
  // Give this thread permission to execute in the context
  JSContextUser jsuser(khRefGuardFromThis());
  jsval rval;
  jsuser.ExecuteScript((JSScript*)jsuser.GetPrivate(script->scriptObj), &rval);
  ret = jsuser.ValueToUint32(rval);
}

void
KHJSContextImpl::ExecuteScript(const KHJSScript &script, float64 &ret)
{
  // Give this thread permission to execute in the context
  JSContextUser jsuser(khRefGuardFromThis());
  jsval rval;
  jsuser.ExecuteScript((JSScript*)jsuser.GetPrivate(script->scriptObj), &rval);
  ret = jsuser.ValueToFloat64(rval);
}


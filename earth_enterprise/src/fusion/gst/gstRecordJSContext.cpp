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

#include "gstRecordJSContext.h"
#include <khException.h>
#include <khjs/KHJSImpl.h>

class gstRecordJSWrapperImpl : public khRefCounter {
 public:
  static gstRecordJSWrapper Create(KHJSContext &cx_,
                                   const gstHeaderHandle &header,
                                   const QString &name_);

  inline void SetRecord(const gstRecordHandle &rec) { recordHandle = rec; }
  inline const gstHeaderHandle &header(void) const { return headerHandle; }
  inline const gstRecordHandle &record(void) const { return recordHandle; }

 private:
  gstRecordJSWrapperImpl(KHJSContext &cx_, const gstHeaderHandle &header,
                         const QString &name_);
  ~gstRecordJSWrapperImpl(void);


  const QString name;
  gstHeaderHandle headerHandle;
  gstRecordHandle recordHandle;

  KHJSContext cx;
  struct JSObject *wrapperObj;
};


// ****************************************************************************
// ***  static helper functions
// ****************************************************************************
JSBool GetValue(JSContext *cx, JSObject *obj, int index, jsval *ret) {
  gstRecordJSWrapperImpl* wrapper =
    static_cast<gstRecordJSWrapperImpl*>(JS_GetPrivate(cx, obj));
  if (!wrapper) {
    JS_ReportError(cx, "object doesn't have gstRecordWrapper\n");
    return JS_FALSE;
  }

  const gstHeaderHandle &header = wrapper->header();
  const gstRecordHandle &rec    = wrapper->record();
  if (!rec) {
    JS_ReportError(cx, "gstRecordWrapper doesn't have an active record");
    return JS_FALSE;
  }


  if ((index >= 0) && (index < (int)header->numColumns())) {
    switch (header->ftype(index)) {
      case gstTagBoolean:
        *ret = BOOLEAN_TO_JSVAL(rec->Field(index)->ValueAsBoolean());
        return JS_TRUE;
      case gstTagInt:
      case gstTagUInt:
      case gstTagInt64:
      case gstTagUInt64:
      case gstTagFloat:
      case gstTagDouble:
        return JS_NewNumberValue(cx, rec->Field(index)->ValueAsDouble(), ret);
      case gstTagString:
      case gstTagUnicode:
        {
          QString str = rec->Field(index)->ValueAsUnicode();
          if (str.isEmpty()) {
            *ret = JS_GetEmptyStringValue(cx);
          } else {
            JSString *newstr = JS_NewUCStringCopyN(cx, str.ucs2(), str.length());
            if (!newstr) {
              // out of memory error already reported
              return JS_FALSE;
            }
            *ret = STRING_TO_JSVAL(newstr);
          }
          return JS_TRUE;
        }
      default:
        JS_ReportError(cx, "Unhandled field type: %d",
                       header->ftype(index));
    }
  } else {
    JS_ReportError(cx, "field index out of range: %d", index);
  }
  return JS_FALSE;
}


JSBool GetGstValueField(JSContext *cx, JSObject *obj, jsval id, jsval *ret) {
  if (JSVAL_IS_INT(id)) {
    int f = JSVAL_TO_INT(id);
    return GetValue(cx, obj, f, ret);
  } else {
    JS_ReportError(cx, "field index is not a number");
  }
  return JS_FALSE;
}

JSBool GetGstValueFieldByIndex(JSContext *cx, JSObject *obj,
                               jsval id, jsval *ret) {
  int f = JSVAL_TO_INT(id);
  return GetValue(cx, obj, f, ret);
}

JSBool GetGstValueFieldByIndexPlus128(JSContext *cx, JSObject *obj,
                                      jsval id, jsval *ret) {
  int f = JSVAL_TO_INT(id) + 128;
  return GetValue(cx, obj, f, ret);
}

JSBool GetGstValueFieldByIndexPlus256(JSContext *cx, JSObject *obj,
                                      jsval id, jsval *ret) {
  int f = JSVAL_TO_INT(id) + 256;
  return GetValue(cx, obj, f, ret);
}
JSBool GetGstValueFieldByIndexPlus384(JSContext *cx, JSObject *obj,
                                      jsval id, jsval *ret) {
  int f = JSVAL_TO_INT(id) + 384;
  return GetValue(cx, obj, f, ret);
}

JSPropertyOp TinyIdGetterFuncs[] = {
  GetGstValueFieldByIndex,
  GetGstValueFieldByIndexPlus128,
  GetGstValueFieldByIndexPlus256,
  GetGstValueFieldByIndexPlus384,
};
const unsigned int gstRecordJSContextImpl::MaxNumProperties = 512;

JSClass gstRecord_wrapper_class = {
    "gstRecord_wrapper_class", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,JS_PropertyStub,GetGstValueField,JS_PropertyStub,
    JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub
};


// ****************************************************************************
// ***  gstRecordJSWrapperImpl
// ****************************************************************************


gstRecordJSWrapperImpl::gstRecordJSWrapperImpl(KHJSContext &cx_,
                                       const gstHeaderHandle &header,
                                       const QString &name_) :
    name(name_), headerHandle(header), recordHandle(),
    cx(cx_), wrapperObj(0)
{

  // Give this thread permission to execute in the context
  JSContextUser jsuser(cx);

  // Make sure all newborns can't be GC'd until we're done here.
  JSLocalRootScopeGuard rootGuard(jsuser);

  // Create the JS object and point it to me
  wrapperObj = rootGuard.NewObject(&gstRecord_wrapper_class);
  jsuser.SetPrivate(wrapperObj, this);

  // Walk the recordHeader, add each viable field name as a property to the
  // newly created object.
  for (unsigned int f = 0; f < std::min(headerHandle->numColumns(),
                                gstRecordJSContextImpl::MaxNumProperties);
       ++f) {
    QString name = headerHandle->Name(f);
    if (cx->IsIdentifier(name)) {
      jsuser.DefineReadOnlyPropertyWithTinyId(wrapperObj, name,
                                              f % 128 /* tiny index */,
                                              TinyIdGetterFuncs[f / 128]);
    }
  }

  // add the object as a named property to the global object
  jsuser.AddNamedGlobalObject(name, wrapperObj);

  // protect scriptObj from GC
  jsuser.AddNamedRoot(&wrapperObj, "gstRecordJSWrapper");
}

gstRecordJSWrapperImpl::~gstRecordJSWrapperImpl(void)
{
  // Give this thread permission to execute in the context
  JSContextUser jsuser(cx);

  // delete the named object from the global object
  jsuser.DeleteNamedGlobalObject(name);

  // clear myself out of private slot since I'm no longer here
  jsuser.SetPrivate(wrapperObj, 0);

  jsuser.RemoveRoot(&wrapperObj);
}


gstRecordJSWrapper
gstRecordJSWrapperImpl::Create(KHJSContext &cx_,
                               const gstHeaderHandle &header,
                               const QString &name_)
{
  return khRefGuardFromNew(new gstRecordJSWrapperImpl(cx_, header, name_));
}



// ****************************************************************************
// ***  gstRecordJSContext
// ****************************************************************************
gstRecordJSContext
gstRecordJSContextImpl::Create(const gstHeaderHandle &header,
                               const QStringList &contextScripts,
                               const QString &primaryScript)
{
  return khRefGuardFromNew(new gstRecordJSContextImpl(header,
                                                      contextScripts,
                                                      primaryScript));
}

bool
gstRecordJSContextImpl::CompileCheck(const gstHeaderHandle &header,
                                     const QStringList &contextScripts,
                                     const QString &primaryScript,
                                     QString &errorReturn) {
  try {
    (void) gstRecordJSContextImpl::Create(header, contextScripts,
                                          primaryScript);

    return true;
  } catch (const khException &e) {
    errorReturn = e.qwhat();
  } catch (const std::exception &e) {
    errorReturn = e.what();
  } catch (...) {
    errorReturn = "Unknown error";
  }
  return false;
}

gstRecordJSContextImpl::gstRecordJSContextImpl(
    const gstHeaderHandle &header,
    const QStringList &contextScripts,
    const QString &primaryScript_) :
    cx(KHJSContextImpl::Create()),
    rec(gstRecordJSWrapperImpl::Create(cx, header, "fields")),
    primaryScript()
{
  // TODO: turn on strict mode ?

  // compile the contextScripts into this context since our script
  // may depend on things they provide
  if (!contextScripts.empty()) {
    for (QStringList::const_iterator cs = contextScripts.begin();
         cs != contextScripts.end(); ++cs) {
      if (!(*cs).isEmpty()) {
        cx->ExecuteScript(cx->CompileScript(*cs, "Context script"));
      }
    }
  }

  if (!primaryScript_.isEmpty()) {
    primaryScript = cx->CompileScript(primaryScript_, "Primary Script");
  }
}

gstRecordJSContextImpl::~gstRecordJSContextImpl(void)
{
}

void gstRecordJSContextImpl::SetRecord(const gstRecordHandle &recHandle) {
  rec->SetRecord(recHandle);
}

KHJSScript
gstRecordJSContextImpl::CompileScript(const QString &scriptText,
                                      const QString &errorContext) {
  return cx->CompileScript(scriptText, errorContext);
}

void gstRecordJSContextImpl::ExecuteScript(const KHJSScript &script,
                                           gstValue &ret) {
  switch (ret.Type()) {
    case gstTagInt:
      {
        std::int32_t tmp;
        cx->ExecuteScript(script, tmp);
        ret.set(tmp);
      }
      break;
    case gstTagUInt:
      {
        std::uint32_t tmp;
        cx->ExecuteScript(script, tmp);
        ret.set(tmp);
      }
      break;
    case gstTagInt64:
    case gstTagUInt64:
    case gstTagFloat:
    case gstTagDouble:
      {
        float64 tmp;
        cx->ExecuteScript(script, tmp);
        ret.set(tmp);
      }
      break;
    case gstTagUnicode:
      {
        QString tmp;
        cx->ExecuteScript(script, tmp);
        ret.set(tmp);
      }
      break;
    case gstTagBoolean:
      {
        bool tmp;
        cx->ExecuteScript(script, tmp);
        ret.set(tmp);
      }
      break;
    default:
      throw khException(kh::tr("Unsupported return type %1 for ExecuteScript")
                        .arg(ret.Type()));
  }
}

bool
gstRecordJSContextImpl::TryExecuteScript(const KHJSScript &script,
                                         bool &ret, QString &error)
{
  try {
    ExecuteScript(script, ret);
    return true;
  } catch (const khException &e) {
    error = e.qwhat();
  } catch (const std::exception &e) {
    error = e.what();
  } catch (...) {
    error = "Unknown error";
  }
  return false;
}

bool
gstRecordJSContextImpl::TryExecuteScript(const KHJSScript &script,
                                         QString &ret, QString &error)
{
  try {
    ExecuteScript(script, ret);
    return true;
  } catch (const khException &e) {
    error = e.qwhat();
  } catch (const std::exception &e) {
    error = e.what();
  } catch (...) {
    error = "Unknown error";
  }
  return false;
}

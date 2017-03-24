// Add ons for SpiderMonkey JSAPI


#ifndef __gejsapi_h
#define __gejsapi_h

#include "jsapi.h"

JS_BEGIN_EXTERN_C

extern JS_PUBLIC_API(JSBool)
JS_IsIdentifier(JSContext *cx, const jschar *name, size_t namelen);

extern JS_PUBLIC_API(JSBool)
JS_GetJSIDProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

JS_END_EXTERN_C

#endif /* __gejsapi_h */

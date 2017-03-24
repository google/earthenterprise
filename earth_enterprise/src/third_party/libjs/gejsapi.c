
#include "gejsapi.h"
#include "jsatom.h"
#include "jsfun.h"
#include "jsobj.h"

// This Macro is no longer part of jsatom as of libjs 1.7
#define ATOM_KEYWORD(atom)        ((struct keyword *)(atom)->entry.value)

JSBool
JS_IsIdentifier(JSContext *cx, const jschar *name, size_t namelen)
{
  JSAtom* atom = js_AtomizeChars(cx, name, namelen, 0);
  if (!atom) return JS_FALSE;
  return (!ATOM_KEYWORD(atom) && js_IsIdentifier(ATOM_TO_STRING(atom)));
}

JSBool
JS_GetJSIDProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  return OBJ_GET_PROPERTY(cx, obj, id, vp);
}

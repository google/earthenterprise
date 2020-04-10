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

#ifndef COMMON_KHJS_KHJSCONTEXT_H__
#define COMMON_KHJS_KHJSCONTEXT_H__

#include <cstdint>
#include <qstring.h>
#include <qstringlist.h>
#include <khMTTypes.h>

// ****************************************************************************
// ***  Keyhole interface to Javascript library
// ***
// ***  The specific JS library used should be completely invisible to the
// ***  code that uses this API.
// ***
// ***  It throws exceptions!
// ****************************************************************************

// predeclare so we don't have to include the underlying API's header
struct JSObject;
struct JSContext;
struct JSErrorReport;
struct JSScript;


// predeclare our own to avoid circular dependencies
class KHJSScriptImpl;
typedef khRefGuard<KHJSScriptImpl> KHJSScript;
class KHJSContextImpl;
typedef khRefGuard<KHJSContextImpl> KHJSContext;

// ****************************************************************************
// ***  KHJSContext
// ***
// ***  Toplevel public interface to Keyhole Javscript API
// ****************************************************************************
class KHJSContextImpl : public khMTRefCounter {
  friend class JSContextUser;
 public:
  static KHJSContext Create(void);
  ~KHJSContextImpl(void);

  bool IsIdentifier(const QString &str);
  void FindGlobalFunctionNames(QStringList &fnames);

  KHJSScript CompileScript(const QString &scriptText,
                           const QString &errorContext);

  void ExecuteScript(const KHJSScript &script);
  void ExecuteScript(const KHJSScript &script, bool &ret);
  void ExecuteScript(const KHJSScript &script, QString &ret);
  void ExecuteScript(const KHJSScript &script, std::int32_t &ret);
  void ExecuteScript(const KHJSScript &script, std::uint32_t &ret);
  void ExecuteScript(const KHJSScript &script, float64 &ret);

  QString pendingErrorMsg;

 private:
  KHJSContextImpl(void);
  void HandleError(const char *msg, struct JSErrorReport *report);
  static void cbErrorHandler(struct JSContext *cx, const char *message,
                             struct JSErrorReport *report);
  void HandleBranchCallback(void);
  static int cbBranchCallback(struct JSContext *cx, JSScript *script);

  // accessable only via KHJSContextUser
  struct JSContext *cx_;
  std::uint32_t branchCallbackCount;
};


// ****************************************************************************
// ***  KHJSScript
// ***
// ***    Simple wrapper around low-level compiled script. Handles all the
// ***  lifetime and GC issues.
// ***    KHJSScript is passed opaquely to other parts of the API.
// ****************************************************************************
class KHJSScriptImpl : public khMTRefCounter {
  friend class KHJSContextImpl;
 public:
  virtual ~KHJSScriptImpl(void);

 private:
  KHJSScriptImpl(const KHJSContext &cx_, const QString &scriptText,
                 const QString &errorContext);

  // holds context around until after script goes away
  KHJSContext cx;

  // holds script and is registered as a root to protect against GC
  struct JSObject *scriptObj;
};






#endif // COMMON_KHJS_KHJSCONTEXT_H__

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

#ifndef FUSION_GST_GSTRECORDJSCONTEXT_H__
#define FUSION_GST_GSTRECORDJSCONTEXT_H__


#include <gstRecord.h>
#include <khRefCounter.h>
#include <khjs/KHJSContext.h>

class gstRecordJSWrapperImpl;
typedef khRefGuard<gstRecordJSWrapperImpl> gstRecordJSWrapper;


class gstRecordJSContextImpl;
typedef khRefGuard<gstRecordJSContextImpl> gstRecordJSContext;

class gstRecordJSContextImpl : public khRefCounter {
 public:
  static const unsigned int MaxNumProperties;

  static gstRecordJSContext Create(const gstHeaderHandle &header,
                                   const QStringList &contextScripts,
                                   const QString &primaryScript = QString());

  // wraps try/catch, error decoding, etc.
  static bool CompileCheck(const gstHeaderHandle &header,
                           const QStringList &contextScripts,
                           const QString &primaryScript,
                           QString &errorReturn);

  void SetRecord(const gstRecordHandle &recHandle);
  inline bool IsIdentifier(const QString &name) {
    return cx->IsIdentifier(name);
  }
  inline void FindGlobalFunctionNames(QStringList &fnames) {
    return cx->FindGlobalFunctionNames(fnames);
  }
  KHJSScript CompileScript(const QString &scriptText,
                           const QString &errorContext);
  inline void ExecuteScript(const KHJSScript &script, bool &ret) {
    cx->ExecuteScript(script, ret);
  }
  inline void ExecuteScript(const KHJSScript &script, QString &ret) {
    cx->ExecuteScript(script, ret);
  }
  void ExecuteScript(const KHJSScript &script, gstValue &ret);

  // convenience funcs
  inline void ExecutePrimaryScript(bool &ret) {
    ExecuteScript(primaryScript, ret);
  }
  inline void ExecutePrimaryScript(QString &ret) {
    ExecuteScript(primaryScript, ret);
  }
  bool TryExecuteScript(const KHJSScript &script, bool &ret, QString &error);
  bool TryExecuteScript(const KHJSScript &script, QString &ret, QString &error);

 private:
  gstRecordJSContextImpl(const gstHeaderHandle &header,
                          const QStringList &contextScripts,
                         const QString &primaryScript_);
  ~gstRecordJSContextImpl(void);

  KHJSContext cx;
  gstRecordJSWrapper rec;
  KHJSScript primaryScript;
};



#endif // FUSION_GST_GSTRECORDJSCONTEXT_H__

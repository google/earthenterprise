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


#ifndef FUSION_GST_VECTORPREP_BINDERS_H__
#define FUSION_GST_VECTORPREP_BINDERS_H__

#include <autoingest/FieldBinderDef.h>
#include <gstRecordJSContext.h>

namespace vectorprep {

// ****************************************************************************
// ***  Binders
// ****************************************************************************
class Binder {
 protected:
  FieldBinderDef def;
 public:
  Binder(const FieldBinderDef &def_) : def(def_) { }
  virtual ~Binder(void) { }

  virtual void Bind(gstRecordHandle &out, gstRecordHandle &in, unsigned int pos) = 0;
};

class RecordFormatterBinder : public Binder {
  gstRecordFormatter formatter;
 public:
  RecordFormatterBinder(const FieldBinderDef &def_, gstHeaderHandle &hdr) :
      Binder(def_),
      formatter(def.fieldGen.value, hdr)
  {
    assert(def.fieldGen.mode == FieldGenerator::RecordFormatter);
  }
  virtual ~RecordFormatterBinder(void) { }

  virtual void Bind(gstRecordHandle &out, gstRecordHandle &in, unsigned int pos) {
    out->Field(pos)->set(formatter.out(in));
  }
};

class JSBinder : public Binder {
  gstRecordJSContext &cx;
  KHJSScript script;
 public:
  JSBinder(const FieldBinderDef &def_, gstRecordJSContext &cx_) :
      Binder(def_),
      cx(cx_)
  {
    assert(def.fieldGen.mode == FieldGenerator::JSExpression);
    if (!def.fieldGen.empty()) {
      script = cx->CompileScript(def.fieldGen.value, def.errorContext);
    }
  }
  virtual ~JSBinder(void) { }

  virtual void Bind(gstRecordHandle &out, gstRecordHandle &in, unsigned int pos) {
    if (!def.fieldGen.empty()) {
      cx->SetRecord(in);
      cx->ExecuteScript(script, *out->Field(pos));
    }
  }
};



} // namespace vectorprep

#endif // FUSION_GST_VECTORPREP_BINDERS_H__

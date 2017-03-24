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

#ifndef FUSION_AUTOINGEST_FIELDBINDERDEF_H__
#define FUSION_AUTOINGEST_FIELDBINDERDEF_H__

#include <autoingest/.idl/storage/FieldGenerator.h>
#include <gst/gstTypes.h>

class FieldBinderDef {
 public:
  FieldGenerator fieldGen;
  QString        fieldName;
  gstTagFlags    fieldType;
  QString        errorContext;

  FieldBinderDef(const FieldGenerator &fgen, const QString &fname,
                 gstTagFlags ftype) :
      fieldGen(fgen),
      fieldName(fname),
      fieldType(ftype),
      errorContext(fname)
  { }
};



#endif // FUSION_AUTOINGEST_FIELDBINDERDEF_H__

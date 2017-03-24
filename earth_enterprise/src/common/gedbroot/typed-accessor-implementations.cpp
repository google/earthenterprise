// Copyright 2017 Google Inc.
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


#include <string>

#include "common/gedbroot/typed-accessor-declarations.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"

namespace PBuf = google::protobuf;

////////////////////////////////////////
// Singular getters

template<typename T, bool>
T GetT(const PBuf::Reflection* refl,
       const PBuf::Message* msg,
       const PBuf::FieldDescriptor* fdesc);
// undefined

template<>
PBuf::int32 GetT<PBuf::int32, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc) {
  return refl->GetInt32(*msg, fdesc);
}

template<>
PBuf::int64 GetT<PBuf::int64, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc) {
  return refl->GetInt64(*msg, fdesc);
}

template<>
PBuf::uint32 GetT<PBuf::uint32, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc) {
  return refl->GetUInt32(*msg, fdesc);
}

template<>
PBuf::uint64 GetT<PBuf::uint64, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc) {
  return refl->GetUInt64(*msg, fdesc);
}

template<>
double GetT<double, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc) {
  return refl->GetDouble(*msg, fdesc);
}

template<>
float GetT<float, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc) {
  return refl->GetFloat(*msg, fdesc);
}

template<>
bool GetT<bool, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc) {
  return refl->GetBool(*msg, fdesc);
}

template<>
const PBuf::EnumValueDescriptor* GetT<const PBuf::EnumValueDescriptor*, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc) {
  return refl->GetEnum(*msg, fdesc);
}

template<>
std::string GetT<std::string, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc) {
  return refl->GetString(*msg, fdesc);
}

template<>
const PBuf::Message& GetT<const PBuf::Message&, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc) {
  return refl->GetMessage(*msg, fdesc);
}


template<typename T, bool>
void SetT(const PBuf::Reflection* refl,
          PBuf::Message* msg,
          const PBuf::FieldDescriptor* fdesc,
          T t);

////////////////////////////////////////
// Singular setters
template<>
void SetT<PBuf::int32, false>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, PBuf::int32 t) {
  refl->SetInt32(msg, fdesc, t);
}

template<>
void SetT<PBuf::int64, false>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, PBuf::int64 t) {
  refl->SetInt64(msg, fdesc, t);
}

template<>
void SetT<PBuf::uint32, false>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, PBuf::uint32 t) {
  refl->SetUInt32(msg, fdesc, t);
}

template<>
void SetT<PBuf::uint64, false>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, PBuf::uint64 t) {
  refl->SetUInt64(msg, fdesc, t);
}

template<>
void SetT<double, false>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, double t) {
  refl->SetDouble(msg, fdesc, t);
}

template<>
void SetT<float, false>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, float t) {
  refl->SetFloat(msg, fdesc, t);
}

template<>
void SetT<bool, false>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, bool t) {
  refl->SetBool(msg, fdesc, t);
}

// not template
void SetSingularEnum(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int enum_value) {
  const PBuf::EnumValueDescriptor* original_vdesc = refl->GetEnum(*msg, fdesc);
  const PBuf::EnumDescriptor* edesc = original_vdesc->type();
  const PBuf::EnumValueDescriptor* new_vdesc = edesc->
      FindValueByNumber(enum_value);
  // leave error checking to protobuf
  refl->SetEnum(msg, fdesc, new_vdesc);
}

template<>
void SetT<std::string, false>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc,
    std::string t) {
  refl->SetString(msg, fdesc, t);
}

////////////////////////////////////////
// repeated getters
template<typename T, bool>
T GetT(const PBuf::Reflection* refl,
       const PBuf::Message* msg,
       const PBuf::FieldDescriptor* fdesc,
       int index);
// undefined

template<>
PBuf::int32 GetT<PBuf::int32, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc,
    int index) {
  return refl->GetRepeatedInt32(*msg, fdesc, index);
}

template<>
PBuf::int64 GetT<PBuf::int64, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc,
    int index) {
  return refl->GetRepeatedInt64(*msg, fdesc, index);
}

template<>
PBuf::uint32 GetT<PBuf::uint32, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index) {
  return refl->GetRepeatedUInt32(*msg, fdesc, index);
}

template<>
PBuf::uint64 GetT<PBuf::uint64, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index) {
  return refl->GetRepeatedUInt64(*msg, fdesc, index);
}

template<>
double GetT<double, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index) {
  return refl->GetRepeatedDouble(*msg, fdesc, index);
}

template<>
float GetT<float, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index) {
  return refl->GetRepeatedFloat(*msg, fdesc, index);
}

template<>
bool GetT<bool, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index) {
  return refl->GetRepeatedBool(*msg, fdesc, index);
}

template<>
std::string GetT<std::string, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index) {
  return refl->GetRepeatedString(*msg, fdesc, index);
}

template<>
const PBuf::Message& GetT<const PBuf::Message&, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index) {
  return refl->GetRepeatedMessage(*msg, fdesc, index);
}


// repeated setters
template<typename T, bool>
void SetT(const PBuf::Reflection* refl,
          PBuf::Message* msg,
          const PBuf::FieldDescriptor* fdesc,
          int index, T value);

template<>
void SetT<PBuf::int32, true>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc,
    int index, PBuf::int32 value) {
  refl->SetRepeatedInt32(msg, fdesc, index, value);
}

template<>
void SetT<PBuf::int64, true>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index,
    PBuf::int64 value) {
  refl->SetRepeatedInt64(msg, fdesc, index, value);
}

template<>
void SetT<PBuf::uint32, true>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index,
    PBuf::uint32 value) {
  refl->SetRepeatedUInt32(msg, fdesc, index, value);
}

template<>
void SetT<PBuf::uint64, true>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index,
    PBuf::uint64 value) {
  refl->SetRepeatedUInt64(msg, fdesc, index, value);
}

template<>
void SetT<double, true>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index,
    double value)  {
  refl->SetRepeatedDouble(msg, fdesc, index, value);
}

template<>
void SetT<float, true>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index,
    float value) {
  refl->SetRepeatedFloat(msg, fdesc, index, value);
}

template<>
void SetT<bool, true>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index,
    bool value) {
  refl->SetRepeatedBool(msg, fdesc, index, value);
}

void SetRepeatedEnum(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc,
    int index,
    int enum_value) {
  const PBuf::EnumValueDescriptor* original_vdesc = refl->GetEnum(*msg, fdesc);
  const PBuf::EnumDescriptor* edesc = original_vdesc->type();
  const PBuf::EnumValueDescriptor* new_vdesc =
      edesc->FindValueByNumber(enum_value);
  // leave error checking to protobuf
  refl->SetRepeatedEnum(msg, fdesc, index, new_vdesc);
}

template<>
void SetT<std::string, true>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index,
    std::string value) {
  refl->SetRepeatedString(msg, fdesc, index, value);
}

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


#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_TYPED_ACCESSOR_DECLARATIONS_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_TYPED_ACCESSOR_DECLARATIONS_H_

#include <string>

#include "google/protobuf/stubs/common.h"

namespace google {
namespace protobuf {
class Message;
class Reflection;
class FieldDescriptor;
class EnumDescriptor;
class EnumValueDescriptor;
}
}

namespace PBuf = google::protobuf;

////////////////////////////////////////
// Singular getters

template<typename T, bool>
T GetT(const PBuf::Reflection* refl,
       const PBuf::Message* msg,
       const PBuf::FieldDescriptor* fdesc);
// unimplemented

template<>
PBuf::int32 GetT<PBuf::int32, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc);

template<>
PBuf::int64 GetT<PBuf::int64, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc);

template<>
PBuf::uint32 GetT<PBuf::uint32, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc);

template<>
PBuf::uint64 GetT<PBuf::uint64, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc);

template<>
double GetT<double, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc);

template<>
float GetT<float, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc);

template<>
bool GetT<bool, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc);

template<>
const PBuf::EnumValueDescriptor*
GetT<const PBuf::EnumValueDescriptor*, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc);

template<>
std::string GetT<std::string, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc);

template<>
const PBuf::Message& GetT<const PBuf::Message&, false>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc);


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
    const PBuf::FieldDescriptor* fdesc, PBuf::int32 t);

template<>
void SetT<PBuf::int64, false>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, PBuf::int64 t);

template<>
void SetT<PBuf::uint32, false>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, PBuf::uint32 t);

template<>
void SetT<PBuf::uint64, false>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, PBuf::uint64 t);

template<>
void SetT<double, false>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, double t);

template<>
void SetT<float, false>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, float t);

template<>
void SetT<bool, false>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, bool t);

// not a template
void SetSingularEnum(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int enum_value);

template<>
void SetT<std::string, false>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc,
    std::string t);

/*
  It makes no sense to write a whole message at once. And, protobufs don't
  allow it.
*/

////////////////////////////////////////
// repeated getters
template<typename T, bool>
T GetT(const PBuf::Reflection* refl,
       const PBuf::Message* msg,
       const PBuf::FieldDescriptor* fdesc,
       int index);
// unimplemented

template<>
PBuf::int32 GetT<PBuf::int32, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc,
    int index);

template<>
PBuf::int64 GetT<PBuf::int64, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc,
    int index);

template<>
PBuf::uint32 GetT<PBuf::uint32, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index);

template<>
PBuf::uint64 GetT<PBuf::uint64, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index);

template<>
double GetT<double, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index);

template<>
float GetT<float, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index);

template<>
bool GetT<bool, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index);

template<>
const PBuf::EnumDescriptor*
GetT<const PBuf::EnumDescriptor*, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc,
    int index);

template<>
std::string GetT<std::string, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index);

template<>
const PBuf::Message& GetT<const PBuf::Message&, true>(
    const PBuf::Reflection* refl,
    const PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index);

////////////////////////////////////////
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
    int index, PBuf::int32 value);

template<>
void SetT<PBuf::int64, true>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index,
    PBuf::int64 value);

template<>
void SetT<PBuf::uint32, true>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index,
    PBuf::uint32 value);

template<>
void SetT<PBuf::uint64, true>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index,
    PBuf::uint64 value);

template<>
void SetT<double, true>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index,
    double value);

template<>
void SetT<float, true>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index,
    float value);

template<>
void SetT<bool, true>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index,
    bool value);

void SetRepeatedEnum(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc,
    int index,
    int enum_value);

template<>
void SetT<std::string, true>(
    const PBuf::Reflection* refl,
    PBuf::Message* msg,
    const PBuf::FieldDescriptor* fdesc, int index,
    std::string value);

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_TYPED_ACCESSOR_DECLARATIONS_H_

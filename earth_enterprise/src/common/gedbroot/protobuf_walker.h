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


#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_PROTOBUF_WALKER_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_PROTOBUF_WALKER_H_

/*
  This is machinery to edit protobuffers. EditProtobuf(), supplied
  with an object of callback methods, walks over a whole given
  protobuf, applying the type-appropriate callback to each field. The
  client is given a FieldAccessor object which provides all
  information about the field and makes editing the field easier by
  hiding the repeated / singular distinction. The client also never
  needs to write any loops to deal with repeated fields. The client
  can filter which fields to edit (besides via type) by using the
  FieldAccessor's Fieldpath() (looks like 'end_snippet.database_url.value')
  and any other conditions and protobuf info + accessors, obtainable
  from the FieldAccessor object.

  The client can read, write, or clear a field; if a /required/ field
  is cleared however, the message that owns it is cleared / removed as
  well, to preserve the integrity (IsInitialized()) of that message
  and of the whole protobuf. If all optional fields of a message are
  removed, that message is similarly removed. This is more for
  aesthetics rather than integrity; an empty mymessage { } or
  submessage is valid, but is at the least inelegant, and possibly
  confusing to final-consumer code.

  Though this method is a bit indirect and clumsy, it was easier to
  implement than something more direct. And, its pretty well suited to
  a large number of near-neighbor fields (they share traversal costs,
  though performance doesn't really matter), which was its original
  use case.
 */


#include <string>

#include "google/protobuf/descriptor.h"
#include "common/gedbroot/typed-accessor-declarations.h"

namespace google {
namespace protobuf {
class FieldDescriptor;
class Message;
}
}

namespace PBuf = google::protobuf;


template<int>
struct cpp_type {};

template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_DOUBLE> {
  typedef double Type;
};
template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_FLOAT> {
  typedef float Type;
};
template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_INT64> {
  typedef PBuf::int64 Type;
};
template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_UINT64> {
  typedef PBuf::uint64 Type;
};
template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_INT32> {
  typedef PBuf::int32 Type;
};
template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_FIXED64> {
  typedef PBuf::uint64 Type;
};
template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_FIXED32> {
  typedef PBuf::uint32 Type;
};
template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_BOOL> {
  typedef bool Type;
};
template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_STRING> {
  typedef std::string Type;
};
template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_GROUP> {
  typedef PBuf::Message* Type;
};
template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_MESSAGE> {
  typedef PBuf::Message* Type;
};
template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_BYTES> {
  typedef std::string Type;
};
template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_UINT32> {
  typedef PBuf::uint32 Type;
};
template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_ENUM> {
  typedef const PBuf::EnumValueDescriptor* Type;
};
template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_SFIXED32> {
  typedef PBuf::int32 Type;
};
template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_SFIXED64> {
  typedef PBuf::int64 Type;
};
template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_SINT32> {
  typedef PBuf::int32 Type;
};
template<>
struct cpp_type<PBuf::FieldDescriptor::TYPE_SINT64> {
  typedef PBuf::int64 Type;
};

// Non-smart package except for convenience functions, of field information
// necessary to read or modify fields.
struct FieldInfo {
  FieldInfo(const PBuf::Reflection* refl,
            PBuf::Message* msg,
            const PBuf::FieldDescriptor* fdesc,
            std::string fieldpath,
            int repeated_index = -1)
      : msg_(msg),
        refl_(refl),
        repeated_index_(repeated_index),
        fdesc_(fdesc),
        fieldpath_(fieldpath) {
    InitHasNonDefaultData(msg, refl, fdesc);
  }

  // Convenience functions; everything is derivable from the included
  // PBuf::FieldDescriptor and other fields.
  bool HasNonDefaultData() const { return has_non_default_data_; }
  bool HasDefault() const { return fdesc_->has_default_value(); }
  const std::string& fieldname() const { return fdesc_->name(); }
  const std::string& fieldpath() const { return fieldpath_; }
  const std::string& fully_qualified_fieldname() const {
    return fdesc_->full_name();
  }

  void set_repeated_index(int index) { repeated_index_ = index; }
  void unset_repeated_index() { repeated_index_ = -1; }
  int repeated_index() const { return repeated_index_; }
  bool is_repeated() const { return 0 <= repeated_index_; }

  PBuf::Message* owning_message() const { return msg_; }
  const PBuf::Reflection* refl() const { return refl_; }
  const PBuf::FieldDescriptor* fdesc() const { return fdesc_; }

 private:
  // Figure out whether this field has non-default ie actual, data.
  void InitHasNonDefaultData(
      PBuf::Message* msg,
      const PBuf::Reflection* refl,
      const PBuf::FieldDescriptor* fdesc);

  // Data
  PBuf::Message* msg_;
  const PBuf::Reflection* refl_;
  int repeated_index_;
  const PBuf::FieldDescriptor* fdesc_;
  std::string fieldpath_;
  bool has_non_default_data_;
};

// Object that gives the callback client a simple interface to read, write, or
// delete a field, regardless of whether it's repeated or not.
// This base provides type-independent functionality - FieldAccessor_T
// is typed for the field passed to the callback.
class FieldAccessor {
 public:
  enum ActionType {
    kNoAction = 0,
    kAddedData,
    kDeletedASeverable,
    kDeletedARequired
  };

  explicit FieldAccessor(const FieldInfo& field_info)
      : action_type_(kNoAction),
        field_info_(field_info)
  {}
  void SetEnum(int enum_value) const;
  ActionType ClearField();
  // Add() unimplemented.
  void Add() const;
  bool HasDefault() const { return field_info().fdesc()->has_default_value(); }
  // Whether the field can be deleted without affecting the validity of its
  // owning message.
  bool is_severable() const { return !field_info().fdesc()->is_required(); }

  const std::string& Fieldpath() const { return field_info().fieldpath(); }
  const FieldInfo& field_info() const { return field_info_; }
  const PBuf::FieldDescriptor* fdesc() const { return field_info().fdesc(); }
  const PBuf::Message* owning_message() const {
    return field_info().owning_message();
  }
  // TODO: check whether this is consumed, finally, anymore.
  ActionType action_type() const { return action_type_; }

 protected:
  ActionType action_type_;

 private:
  const FieldInfo& field_info_;
};

// Type-specific FieldAccessor.
template <typename T>
class FieldAccessor_T : public FieldAccessor {
 public:
  explicit FieldAccessor_T(const FieldInfo& field_info)
      : FieldAccessor(field_info)
  {}
  // We don't simply make member templates of these because that puts control
  // of the type into the clients' hands, which they could get wrong.
  T get() const;
  void set(T t);
};


template <typename T>
T FieldAccessor_T<T>::get() const {
  if (field_info().repeated_index() < 0) {
    return GetT<T, false>(field_info().refl(),
      field_info().owning_message(),
      field_info().fdesc());
  } else {
    return GetT<T, true>(field_info().refl(),
      field_info().owning_message(),
      field_info().fdesc(),
      field_info().repeated_index());
  }
}

template <typename T>
void FieldAccessor_T<T>::set(T t) {
  if (field_info().repeated_index() < 0) {
    SetT<T, false>(field_info().refl(), field_info().owning_message(),
                        field_info().fdesc(),
                        t);
  } else {
    SetT<T, true>(field_info().refl(), field_info().owning_message(),
                       field_info().fdesc(),
                       field_info().repeated_index(),
                       t);
  }
  // Record the user's action, it may replace a 'clear'.
  action_type_ = kAddedData;
}

// Base class of field callbacks; by default they do nothing.
// TODO: re-implement with smarter, more type-safe
// registration technique (as is, is very easy to accidentally create
// a new callback instead of overriding an existing one).
class ProtobufWalkerCallbacks {
 public:
  virtual ~ProtobufWalkerCallbacks() {}
  virtual void on_DOUBLE(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_DOUBLE>::Type>& ) {}
  virtual void on_FLOAT(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_FLOAT>::Type>&) {}
  virtual void on_INT64(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_INT64>::Type>&) {}
  virtual void on_UINT64(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_UINT64>::Type>&) {}
  virtual void on_INT32(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_INT32>::Type>&) {}
  virtual void on_FIXED64(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_FIXED64>::Type>&) {}
  virtual void on_FIXED32(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_FIXED32>::Type>&) {}
  virtual void on_BOOL(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_BOOL>::Type>&) {}
  virtual void on_STRING(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_STRING>::Type>&) {}
  virtual void on_GROUP(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_GROUP>::Type>&) {}
  virtual void on_MESSAGE(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_MESSAGE>::Type>&) {}
  virtual void on_BYTES(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_BYTES>::Type>&) {}
  virtual void on_UINT32(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_UINT32>::Type>&) {}
  virtual void on_ENUM(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_ENUM>::Type>&) {}
  virtual void on_SFIXED32(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_SFIXED32>::Type>&) {}
  virtual void on_SFIXED64(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_SFIXED64>::Type>&) {}
  virtual void on_SINT32(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_SINT32>::Type>&) {}
  virtual void on_SINT64(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_SINT64>::Type>&) {}
};

// Encapsulates algorithm (in <Edit>) of traversing a protobuf as a tree.
// Calls the appropriate user-supplied callback on each /primitive/
// (eg std::int32_t, string; not Message) field.
class Walker {
 public:
  explicit Walker(ProtobufWalkerCallbacks* callbacks)
      : callbacks_(callbacks)
  { }
  // Returns whether <msg> survives editing.
  bool Edit(const std::string& parent_name, PBuf::Message* msg);

 private:
  // Fns
  ProtobufWalkerCallbacks* callbacks() { return callbacks_; }

  // Applies user callbacks to primitive fields of <msg>, below this
  // particular field (<fdesc>).
  // Returns whether this field survives.
  // <fieldpath> looks like:  "end_snippet.ads_url_patterns"
  bool EditField(const std::string& fieldpath, PBuf::Message* msg,
                 const PBuf::FieldDescriptor* fdesc);

  // Breaks out the case where <msg_fdesc> is a proto.
  // Returns whether the field survives.
  // <fieldpath> looks like:  "end_snippet.ads_url_patterns"
  bool EditOneProtoField(const std::string& fieldpath,
                         PBuf::Message* msg,
                         const PBuf::Reflection* refl,
                         const PBuf::FieldDescriptor* msg_fdesc,
                         // -1 signals it's not repeated
                         int iProto = -1);
  // The user callbacks are the equivalent of 'EditPrimitiveField'.

  // User's callback object. Not constant because they may want to
  // change their state while they traverse.
  ProtobufWalkerCallbacks* callbacks_;
};

// Walks the protobuf, calling callbacks' .on_SOMETYPE(). Within it, a
// client can read, write, or clear a field. If they clear a required
// field however, the walker will clear the whole message it's part
// of, to preserve the validity ('IsInitialized()') of the message.
// Returns whether the message is still valid.
bool EditProtobuf(const std::string& msg_name, PBuf::Message* msg,
                  ProtobufWalkerCallbacks* cbs);

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_PROTOBUF_WALKER_H_

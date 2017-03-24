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


#include "common/gedbroot/protobuf_walker.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <string>
#include <vector>
#include <iostream>  // NOLINT(readability/streams)

using std::string;
using std::vector;
using std::cout;
using std::endl;
using google::protobuf::Message;
using google::protobuf::Descriptor;
using google::protobuf::Reflection;
using google::protobuf::FieldDescriptor;
using google::protobuf::EnumValueDescriptor;


static
bool IsProto(const FieldDescriptor* fdesc) {
  // CPPTYPE_MESSAGE captures both GROUP and MESSAGE
  return fdesc->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE;
}

static
bool IsDeletedAction(FieldAccessor::ActionType action) {
  return action == FieldAccessor::kDeletedASeverable ||
      action == FieldAccessor::kDeletedARequired;
}

static
bool PrimitiveHasDataNow(bool had_data_before,
                         FieldAccessor::ActionType action) {
  if (IsDeletedAction(action)) {
    return false;
  } else if (action == FieldAccessor::kAddedData) {
    return true;
  } else {
    // no action
    assert(action == FieldAccessor::kNoAction);
    return had_data_before;
  }
}

static
// Add subfield to path.
string AppendToFieldpath(const string& path, const FieldDescriptor* fdesc) {
  string fieldpath;
  if (!path.empty()) {
    fieldpath += path;
    fieldpath += '.';
  }
  fieldpath += fdesc->name();

  return fieldpath;
}

static
// Preserves order; expensive to use in bulk.
// Doesn't clear anything, just does the surgery.
void ClearRepeatedField(const Reflection* refl, Message* msg,
                        const FieldDescriptor* msg_fdesc,
                        int index_to_remove) {
  int ilast = refl->FieldSize(*msg, msg_fdesc) - 1;
  // Bubble this element to last (preserves order).
  for (int iclear = index_to_remove; iclear < ilast; ++iclear) {
    refl->SwapElements(msg, msg_fdesc, iclear, iclear + 1);
  }

  refl->RemoveLast(msg, msg_fdesc);
}

static
void ClearMaybeRepeatedField(
    Message* msg,
    const Reflection* refl,
    const FieldDescriptor* msg_fdesc,
    // -1 signals that it's singular
    int index_to_remove) {
  if (0 <= index_to_remove) {
    ClearRepeatedField(refl, msg, msg_fdesc, index_to_remove);
  } else {
    refl->ClearField(msg, msg_fdesc);
  }
}

// Sets a repeated or singular field has non-default data.
void FieldInfo::InitHasNonDefaultData(
    PBuf::Message* msg,
    const PBuf::Reflection* refl,
    const PBuf::FieldDescriptor* fdesc) {
  if (fdesc->is_repeated()) {
    has_non_default_data_ = 0 < refl->FieldSize(*msg, fdesc);
  } else {
    has_non_default_data_ = refl->HasField(*msg, fdesc);
  }
}


void FieldAccessor::SetEnum(int enum_value) const {
  if (field_info().repeated_index() < 0) {
    // Singular
    SetSingularEnum(field_info().refl(), field_info().owning_message(),
                            field_info().fdesc(),
                            enum_value);
  } else {
    // Repeated
    SetRepeatedEnum(field_info().refl(), field_info().owning_message(),
      field_info().fdesc(),
      field_info().repeated_index(),
      enum_value);
  }
}

FieldAccessor::ActionType FieldAccessor::ClearField() {
  if (field_info().fdesc()->is_repeated()) {
    // Repeated
    ClearRepeatedField(field_info().refl(), field_info().owning_message(),
                       field_info().fdesc(), field_info().repeated_index());
    action_type_ = FieldAccessor::kDeletedASeverable;
  } else {
    // Singular
    field_info().refl()->ClearField(field_info().owning_message(),
                                    field_info().fdesc());
    action_type_ = field_info().fdesc()->is_required() ?
        kDeletedARequired : kDeletedASeverable;
  }
  return action_type_;
}

// The Walker's interface to the user's callbacks. Converts from runtime
// protobuf field type to typed accessor.
FieldAccessor::ActionType UserActionOnField(
    const FieldInfo& finfo, ProtobufWalkerCallbacks* cbs) {
  FieldAccessor::ActionType action_type = FieldAccessor::kNoAction;
  switch (finfo.fdesc()->type()) {
    case FieldDescriptor::TYPE_INT32: {
      FieldAccessor_T<PBuf::int32> accessor(finfo);
      cbs->on_INT32(accessor);
      action_type = accessor.action_type();
      break;
    }

    case FieldDescriptor::TYPE_SINT32: {
      FieldAccessor_T<PBuf::int32> accessor(finfo);
      cbs->on_SINT32(accessor);
      action_type = accessor.action_type();
      break;
    }

    case FieldDescriptor::TYPE_SFIXED32: {
      FieldAccessor_T<PBuf::int32> accessor(finfo);
      cbs->on_SFIXED32(accessor);
      action_type = accessor.action_type();
      break;
    }

    case FieldDescriptor::TYPE_INT64: {
      FieldAccessor_T<PBuf::int64> accessor(finfo);
      cbs->on_INT64(accessor);
      action_type = accessor.action_type();
      break;
    }

    case FieldDescriptor::TYPE_SINT64: {
      FieldAccessor_T<PBuf::int64> accessor(finfo);
      cbs->on_SINT64(accessor);
      action_type = accessor.action_type();
      break;
    }

    case FieldDescriptor::TYPE_SFIXED64: {
      FieldAccessor_T<PBuf::int64> accessor(finfo);
      cbs->on_SFIXED64(accessor);
      action_type = accessor.action_type();
      break;
    }

    case FieldDescriptor::TYPE_UINT32: {
      FieldAccessor_T<PBuf::uint32> accessor(finfo);
      cbs->on_UINT32(accessor);
      action_type = accessor.action_type();
      break;
    }

    case FieldDescriptor::TYPE_FIXED32: {
      FieldAccessor_T<PBuf::uint32> accessor(finfo);
      cbs->on_FIXED32(accessor);
      action_type = accessor.action_type();
      break;
    }

    case FieldDescriptor::TYPE_UINT64: {
      FieldAccessor_T<PBuf::uint64> accessor(finfo);
      cbs->on_UINT64(accessor);
      action_type = accessor.action_type();
      break;
    }

    case FieldDescriptor::TYPE_FIXED64: {
      FieldAccessor_T<PBuf::uint64> accessor(finfo);
      cbs->on_FIXED64(accessor);
      action_type = accessor.action_type();
      break;
    }

    case FieldDescriptor::TYPE_DOUBLE: {
      FieldAccessor_T<double> accessor(finfo);
      cbs->on_DOUBLE(accessor);
      action_type = accessor.action_type();
      break;
    }

    case FieldDescriptor::TYPE_FLOAT: {
      FieldAccessor_T<float> accessor(finfo);
      cbs->on_FLOAT(accessor);
      action_type = accessor.action_type();
      break;
    }

    case FieldDescriptor::TYPE_BOOL: {
      FieldAccessor_T<bool> accessor(finfo);
      cbs->on_BOOL(accessor);
      action_type = accessor.action_type();
      break;
    }

    case FieldDescriptor::TYPE_ENUM: {
      FieldAccessor_T<const EnumValueDescriptor*> accessor(finfo);
      cbs->on_ENUM(accessor);
      action_type = accessor.action_type();
      break;
    }

    case FieldDescriptor::TYPE_STRING: {
      FieldAccessor_T<string> accessor(finfo);
      cbs->on_STRING(accessor);
      action_type = accessor.action_type();
      break;
    }

    case FieldDescriptor::TYPE_BYTES: {
      FieldAccessor_T<string> accessor(finfo);
      cbs->on_BYTES(accessor);
      action_type = accessor.action_type();
      break;
    }

    case FieldDescriptor::TYPE_MESSAGE: {
      FieldAccessor_T<Message*> accessor(finfo);
      cbs->on_MESSAGE(accessor);
      action_type = accessor.action_type();
      break;
    }

    case FieldDescriptor::TYPE_GROUP:
      // Fall through to unknown.
    default:
      assert(false);
  }

  return action_type;
}


// Just a broken-out chunk of <EditField>, for where the field is a
// proto / message.  Applies to singular or one of a pack of repeated
// protos.
bool Walker::EditOneProtoField(const string& this_fieldpath,
                               Message* msg,
                               const Reflection* refl_msg,
                               const FieldDescriptor* msg_fdesc,
                               int iProto /* = -1 */) {
  Message* fmsg;
  if (iProto < 0) {
    fmsg = refl_msg->MutableMessage(msg, msg_fdesc);
  } else {
    fmsg = refl_msg->MutableRepeatedMessage(msg, msg_fdesc, iProto);
  }
  bool fmsg_has_data_now = false;
  const Descriptor* desc_fmsg = fmsg->GetDescriptor();

  int ith_field = 0, num_fields = desc_fmsg->field_count();
  for (; ith_field < num_fields; ++ith_field) {
    const FieldDescriptor* fmsg_fdesc = desc_fmsg->field(ith_field);
    string ffmsg_fieldpath = AppendToFieldpath(this_fieldpath, fmsg_fdesc);
    bool ffmsg_has_legit_data_now = EditField(ffmsg_fieldpath, fmsg,
                                              fmsg_fdesc);
    if (ffmsg_has_legit_data_now) {
      fmsg_has_data_now = true;
    } else if (fmsg_fdesc->is_required()) {
      // clean /self/ up.
      ClearMaybeRepeatedField(msg, refl_msg, msg_fdesc, iProto);
      // no further decision or action needed at this level.
      return false;
    } else {
      // deleted optional or repeated
    }
  }

  if (!fmsg_has_data_now) {
    ClearMaybeRepeatedField(msg, refl_msg, msg_fdesc, iProto);
    return false;
  }

  return true;
}

// Called on every field, primitive or proto/message. For repeated
// fields it's called on the bunch as a whole, not individually. We
// break them out individually for the user, inside.
bool Walker::EditField(
    const string& this_fieldpath,
    Message* msg, const FieldDescriptor* msg_fdesc) {
  const Reflection* refl_msg = msg->GetReflection();
  // REPEATED
  if (msg_fdesc->is_repeated()) {
    // REPEATED PROTO
    if (IsProto(msg_fdesc)) {
      // Live FieldSize() since we might delete.
      for (int iProto = 0; iProto < refl_msg->FieldSize(*msg, msg_fdesc); ) {
        // didactic; always true for repeated fields.
        // -- although, that applies to whether the field is /present/
        // or not, not whether it has content. It might be /possible/
        // for a repeated field to be present, but empty.
        bool used_to_have_data = true;
        bool now_has_data = EditOneProtoField(
            this_fieldpath, msg, refl_msg, msg_fdesc, iProto);

        // ie !deleted
        if (!(!now_has_data && used_to_have_data))
          ++iProto;
      }
      // REPEATED PRIMITIVE
    } else {
      // Use live FieldSize() since we might delete.
      for (int iPrim = 0; iPrim < refl_msg->FieldSize(*msg, msg_fdesc); ) {
        FieldInfo field_info(refl_msg, msg, msg_fdesc, this_fieldpath, iPrim);

        FieldAccessor::ActionType action_type =
            UserActionOnField(field_info, callbacks());
        // We care only about whether we /removed/ a message, for our iteration.
        if (!IsDeletedAction(action_type)) {
          ++iPrim;
        }
      }
    }

    bool any_left = 0 < refl_msg->FieldSize(*msg, msg_fdesc);
    return any_left;
  } else {
    bool has_data_now;
    // SINGULAR
    FieldAccessor::ActionType action_type;
    if (IsProto(msg_fdesc)) {
      // SINGULAR PROTO
      has_data_now = EditOneProtoField(this_fieldpath, msg, refl_msg,
                                       msg_fdesc, -1);
    } else {
      // SINGULAR PRIMITIVE
      FieldInfo field_info(refl_msg, msg, msg_fdesc, this_fieldpath);
      bool had_data_before = refl_msg->HasField(*msg, msg_fdesc);
      action_type = UserActionOnField(field_info, callbacks());
      has_data_now = PrimitiveHasDataNow(had_data_before, action_type);
    }
    return has_data_now;
  }
}

// Returns whether the message survived the editing. If not,
// this Clear()s it, but destruction is left to the caller.
// (<msg> can be rendered invalid by deleting required fields.)
bool Walker::Edit(const string& parent_name, Message* msg) {
  const Reflection* refl_msg = msg->GetReflection();
  const Descriptor* desc_msg = msg->GetDescriptor();
  int ith_field = 0;
  int num_fields = desc_msg->field_count();
  for (; ith_field < num_fields; ++ith_field) {
    const FieldDescriptor* msg_fdesc = desc_msg->field(ith_field);
    string field_path = AppendToFieldpath(parent_name, msg_fdesc);
    FieldInfo field_info(refl_msg, msg, msg_fdesc, field_path);
    bool has_data_now = EditField(field_path, msg, msg_fdesc);

    if (!has_data_now) {
#if !defined(NDEBUG)
      bool field_present = msg_fdesc->is_repeated() ?
          // Looks dubious, but indeed, we edit the whole rptd bunch at once.
          refl_msg->FieldSize(*msg, msg_fdesc) :
          refl_msg->HasField(*msg, msg_fdesc);
      assert(!field_present);
#endif  // !NDEBUG
      if (msg_fdesc->is_required()) {
        break;
      }
    }
  }

  // Deleted a top-level required field; this message is toast.
  if (ith_field < num_fields) {
    msg->Clear();
    return false;
  }
  return true;
}

// Walks the protobuf, calling callbacks.on_SOMETYPE(). The user can
// do whatever they want with the field, read, write, or clear (add
// for repeateds is not implemented). If they clear a required field
// however, the walker will clear the whole message it's part of, to
// preserve the validity (IsInitialized()) of the message.
bool EditProtobuf(const string& top_name, Message* msg,
                  ProtobufWalkerCallbacks* callbacks) {
  Walker walker(callbacks);
  return walker.Edit(top_name, msg);
}

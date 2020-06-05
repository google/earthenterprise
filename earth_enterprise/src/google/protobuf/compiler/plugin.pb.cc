// Generated by the protocol buffer compiler.  DO NOT EDIT!

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "google/protobuf/compiler/plugin.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace google {
namespace protobuf {
namespace compiler {

namespace {

const ::google::protobuf::Descriptor* CodeGeneratorRequest_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  CodeGeneratorRequest_reflection_ = NULL;
const ::google::protobuf::Descriptor* CodeGeneratorResponse_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  CodeGeneratorResponse_reflection_ = NULL;
const ::google::protobuf::Descriptor* CodeGeneratorResponse_File_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  CodeGeneratorResponse_File_reflection_ = NULL;

}  // namespace


void protobuf_AssignDesc_google_2fprotobuf_2fcompiler_2fplugin_2eproto() {
  protobuf_AddDesc_google_2fprotobuf_2fcompiler_2fplugin_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "google/protobuf/compiler/plugin.proto");
  GOOGLE_CHECK(file != NULL);
  CodeGeneratorRequest_descriptor_ = file->message_type(0);
  static const int CodeGeneratorRequest_offsets_[3] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CodeGeneratorRequest, file_to_generate_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CodeGeneratorRequest, parameter_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CodeGeneratorRequest, proto_file_),
  };
  CodeGeneratorRequest_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      CodeGeneratorRequest_descriptor_,
      CodeGeneratorRequest::default_instance_,
      CodeGeneratorRequest_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CodeGeneratorRequest, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CodeGeneratorRequest, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(CodeGeneratorRequest));
  CodeGeneratorResponse_descriptor_ = file->message_type(1);
  static const int CodeGeneratorResponse_offsets_[2] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CodeGeneratorResponse, error_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CodeGeneratorResponse, file_),
  };
  CodeGeneratorResponse_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      CodeGeneratorResponse_descriptor_,
      CodeGeneratorResponse::default_instance_,
      CodeGeneratorResponse_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CodeGeneratorResponse, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CodeGeneratorResponse, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(CodeGeneratorResponse));
  CodeGeneratorResponse_File_descriptor_ = CodeGeneratorResponse_descriptor_->nested_type(0);
  static const int CodeGeneratorResponse_File_offsets_[3] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CodeGeneratorResponse_File, name_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CodeGeneratorResponse_File, insertion_point_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CodeGeneratorResponse_File, content_),
  };
  CodeGeneratorResponse_File_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      CodeGeneratorResponse_File_descriptor_,
      CodeGeneratorResponse_File::default_instance_,
      CodeGeneratorResponse_File_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CodeGeneratorResponse_File, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CodeGeneratorResponse_File, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(CodeGeneratorResponse_File));
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_google_2fprotobuf_2fcompiler_2fplugin_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    CodeGeneratorRequest_descriptor_, &CodeGeneratorRequest::default_instance());
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    CodeGeneratorResponse_descriptor_, &CodeGeneratorResponse::default_instance());
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    CodeGeneratorResponse_File_descriptor_, &CodeGeneratorResponse_File::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_google_2fprotobuf_2fcompiler_2fplugin_2eproto() {
  delete CodeGeneratorRequest::default_instance_;
  delete CodeGeneratorRequest_reflection_;
  delete CodeGeneratorResponse::default_instance_;
  delete CodeGeneratorResponse_reflection_;
  delete CodeGeneratorResponse_File::default_instance_;
  delete CodeGeneratorResponse_File_reflection_;
}

void protobuf_AddDesc_google_2fprotobuf_2fcompiler_2fplugin_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::protobuf_AddDesc_google_2fprotobuf_2fdescriptor_2eproto();
  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n%google/protobuf/compiler/plugin.proto\022"
    "\030google.protobuf.compiler\032 google/protob"
    "uf/descriptor.proto\"}\n\024CodeGeneratorRequ"
    "est\022\030\n\020file_to_generate\030\001 \003(\t\022\021\n\tparamet"
    "er\030\002 \001(\t\0228\n\nproto_file\030\017 \003(\0132$.google.pr"
    "otobuf.FileDescriptorProto\"\252\001\n\025CodeGener"
    "atorResponse\022\r\n\005error\030\001 \001(\t\022B\n\004file\030\017 \003("
    "\01324.google.protobuf.compiler.CodeGenerat"
    "orResponse.File\032>\n\004File\022\014\n\004name\030\001 \001(\t\022\027\n"
    "\017insertion_point\030\002 \001(\t\022\017\n\007content\030\017 \001(\t", 399);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "google/protobuf/compiler/plugin.proto", &protobuf_RegisterTypes);
  CodeGeneratorRequest::default_instance_ = new CodeGeneratorRequest();
  CodeGeneratorResponse::default_instance_ = new CodeGeneratorResponse();
  CodeGeneratorResponse_File::default_instance_ = new CodeGeneratorResponse_File();
  CodeGeneratorRequest::default_instance_->InitAsDefaultInstance();
  CodeGeneratorResponse::default_instance_->InitAsDefaultInstance();
  CodeGeneratorResponse_File::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_google_2fprotobuf_2fcompiler_2fplugin_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_google_2fprotobuf_2fcompiler_2fplugin_2eproto {
  StaticDescriptorInitializer_google_2fprotobuf_2fcompiler_2fplugin_2eproto() {
    protobuf_AddDesc_google_2fprotobuf_2fcompiler_2fplugin_2eproto();
  }
} static_descriptor_initializer_google_2fprotobuf_2fcompiler_2fplugin_2eproto_;


// ===================================================================

#ifndef _MSC_VER
const int CodeGeneratorRequest::kFileToGenerateFieldNumber;
const int CodeGeneratorRequest::kParameterFieldNumber;
const int CodeGeneratorRequest::kProtoFileFieldNumber;
#endif  // !_MSC_VER

CodeGeneratorRequest::CodeGeneratorRequest()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void CodeGeneratorRequest::InitAsDefaultInstance() {
}

CodeGeneratorRequest::CodeGeneratorRequest(const CodeGeneratorRequest& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void CodeGeneratorRequest::SharedCtor() {
  _cached_size_ = 0;
  parameter_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

CodeGeneratorRequest::~CodeGeneratorRequest() {
  SharedDtor();
}

void CodeGeneratorRequest::SharedDtor() {
  if (parameter_ != &::google::protobuf::internal::kEmptyString) {
    delete parameter_;
  }
  if (this != default_instance_) {
  }
}

void CodeGeneratorRequest::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* CodeGeneratorRequest::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return CodeGeneratorRequest_descriptor_;
}

const CodeGeneratorRequest& CodeGeneratorRequest::default_instance() {
  if (default_instance_ == NULL) 
  {
    protobuf_AddDesc_google_2fprotobuf_2fcompiler_2fplugin_2eproto();  
  }
  return *default_instance_;
}

CodeGeneratorRequest* CodeGeneratorRequest::default_instance_ = NULL;

CodeGeneratorRequest* CodeGeneratorRequest::New() const {
  return new CodeGeneratorRequest;
}

void CodeGeneratorRequest::Clear() {
  if (_has_bits_[1 / 32] & (0xffu << (1 % 32))) {
    if (has_parameter()) {
      if (parameter_ != &::google::protobuf::internal::kEmptyString) {
        parameter_->clear();
      }
    }
  }
  file_to_generate_.Clear();
  proto_file_.Clear();
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool CodeGeneratorRequest::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // repeated string file_to_generate = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_file_to_generate:
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->add_file_to_generate()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8String(
            this->file_to_generate(0).data(), this->file_to_generate(0).length(),
            ::google::protobuf::internal::WireFormat::PARSE);
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(10)) goto parse_file_to_generate;
        if (input->ExpectTag(18)) goto parse_parameter;
        break;
      }
      
      // optional string parameter = 2;
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_parameter:
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_parameter()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8String(
            this->parameter().data(), this->parameter().length(),
            ::google::protobuf::internal::WireFormat::PARSE);
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(122)) goto parse_proto_file;
        break;
      }
      
      // repeated .google.protobuf.FileDescriptorProto proto_file = 15;
      case 15: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_proto_file:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
                input, add_proto_file()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(122)) goto parse_proto_file;
        if (input->ExpectAtEnd()) return true;
        break;
      }
      
      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void CodeGeneratorRequest::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // repeated string file_to_generate = 1;
  for (int i = 0; i < this->file_to_generate_size(); i++) {
  ::google::protobuf::internal::WireFormat::VerifyUTF8String(
    this->file_to_generate(i).data(), this->file_to_generate(i).length(),
    ::google::protobuf::internal::WireFormat::SERIALIZE);
    ::google::protobuf::internal::WireFormatLite::WriteString(
      1, this->file_to_generate(i), output);
  }
  
  // optional string parameter = 2;
  if (has_parameter()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->parameter().data(), this->parameter().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    ::google::protobuf::internal::WireFormatLite::WriteString(
      2, this->parameter(), output);
  }
  
  // repeated .google.protobuf.FileDescriptorProto proto_file = 15;
  for (int i = 0; i < this->proto_file_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      15, this->proto_file(i), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* CodeGeneratorRequest::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // repeated string file_to_generate = 1;
  for (int i = 0; i < this->file_to_generate_size(); i++) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->file_to_generate(i).data(), this->file_to_generate(i).length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    target = ::google::protobuf::internal::WireFormatLite::
      WriteStringToArray(1, this->file_to_generate(i), target);
  }
  
  // optional string parameter = 2;
  if (has_parameter()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->parameter().data(), this->parameter().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        2, this->parameter(), target);
  }
  
  // repeated .google.protobuf.FileDescriptorProto proto_file = 15;
  for (int i = 0; i < this->proto_file_size(); i++) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        15, this->proto_file(i), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int CodeGeneratorRequest::ByteSize() const {
  int total_size = 0;
  
  if (_has_bits_[1 / 32] & (0xffu << (1 % 32))) {
    // optional string parameter = 2;
    if (has_parameter()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->parameter());
    }
    
  }
  // repeated string file_to_generate = 1;
  total_size += 1 * this->file_to_generate_size();
  for (int i = 0; i < this->file_to_generate_size(); i++) {
    total_size += ::google::protobuf::internal::WireFormatLite::StringSize(
      this->file_to_generate(i));
  }
  
  // repeated .google.protobuf.FileDescriptorProto proto_file = 15;
  total_size += 1 * this->proto_file_size();
  for (int i = 0; i < this->proto_file_size(); i++) {
    total_size +=
      ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
        this->proto_file(i));
  }
  
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void CodeGeneratorRequest::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const CodeGeneratorRequest* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const CodeGeneratorRequest*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void CodeGeneratorRequest::MergeFrom(const CodeGeneratorRequest& from) {
  GOOGLE_CHECK_NE(&from, this);
  file_to_generate_.MergeFrom(from.file_to_generate_);
  proto_file_.MergeFrom(from.proto_file_);
  if (from._has_bits_[1 / 32] & (0xffu << (1 % 32))) {
    if (from.has_parameter()) {
      set_parameter(from.parameter());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void CodeGeneratorRequest::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void CodeGeneratorRequest::CopyFrom(const CodeGeneratorRequest& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool CodeGeneratorRequest::IsInitialized() const {
  
  for (int i = 0; i < proto_file_size(); i++) {
    if (!this->proto_file(i).IsInitialized()) return false;
  }
  return true;
}

void CodeGeneratorRequest::Swap(CodeGeneratorRequest* other) {
  if (other != this) {
    file_to_generate_.Swap(&other->file_to_generate_);
    std::swap(parameter_, other->parameter_);
    proto_file_.Swap(&other->proto_file_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata CodeGeneratorRequest::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = CodeGeneratorRequest_descriptor_;
  metadata.reflection = CodeGeneratorRequest_reflection_;
  return metadata;
}


// ===================================================================

#ifndef _MSC_VER
const int CodeGeneratorResponse_File::kNameFieldNumber;
const int CodeGeneratorResponse_File::kInsertionPointFieldNumber;
const int CodeGeneratorResponse_File::kContentFieldNumber;
#endif  // !_MSC_VER

CodeGeneratorResponse_File::CodeGeneratorResponse_File()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void CodeGeneratorResponse_File::InitAsDefaultInstance() {
}

CodeGeneratorResponse_File::CodeGeneratorResponse_File(const CodeGeneratorResponse_File& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void CodeGeneratorResponse_File::SharedCtor() {
  _cached_size_ = 0;
  name_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  insertion_point_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  content_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

CodeGeneratorResponse_File::~CodeGeneratorResponse_File() {
  SharedDtor();
}

void CodeGeneratorResponse_File::SharedDtor() {
  if (name_ != &::google::protobuf::internal::kEmptyString) {
    delete name_;
  }
  if (insertion_point_ != &::google::protobuf::internal::kEmptyString) {
    delete insertion_point_;
  }
  if (content_ != &::google::protobuf::internal::kEmptyString) {
    delete content_;
  }
  if (this != default_instance_) {
  }
}

void CodeGeneratorResponse_File::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* CodeGeneratorResponse_File::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return CodeGeneratorResponse_File_descriptor_;
}

const CodeGeneratorResponse_File& CodeGeneratorResponse_File::default_instance() {
  if (default_instance_ == NULL) 
  {
    protobuf_AddDesc_google_2fprotobuf_2fcompiler_2fplugin_2eproto();  
  }
  return *default_instance_;
 
}

CodeGeneratorResponse_File* CodeGeneratorResponse_File::default_instance_ = NULL;

CodeGeneratorResponse_File* CodeGeneratorResponse_File::New() const {
  return new CodeGeneratorResponse_File;
}

void CodeGeneratorResponse_File::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (has_name()) {
      if (name_ != &::google::protobuf::internal::kEmptyString) {
        name_->clear();
      }
    }
    if (has_insertion_point()) {
      if (insertion_point_ != &::google::protobuf::internal::kEmptyString) {
        insertion_point_->clear();
      }
    }
    if (has_content()) {
      if (content_ != &::google::protobuf::internal::kEmptyString) {
        content_->clear();
      }
    }
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool CodeGeneratorResponse_File::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional string name = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_name()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8String(
            this->name().data(), this->name().length(),
            ::google::protobuf::internal::WireFormat::PARSE);
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(18)) goto parse_insertion_point;
        break;
      }
      
      // optional string insertion_point = 2;
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_insertion_point:
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_insertion_point()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8String(
            this->insertion_point().data(), this->insertion_point().length(),
            ::google::protobuf::internal::WireFormat::PARSE);
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(122)) goto parse_content;
        break;
      }
      
      // optional string content = 15;
      case 15: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_content:
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_content()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8String(
            this->content().data(), this->content().length(),
            ::google::protobuf::internal::WireFormat::PARSE);
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectAtEnd()) return true;
        break;
      }
      
      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void CodeGeneratorResponse_File::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // optional string name = 1;
  if (has_name()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->name().data(), this->name().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    ::google::protobuf::internal::WireFormatLite::WriteString(
      1, this->name(), output);
  }
  
  // optional string insertion_point = 2;
  if (has_insertion_point()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->insertion_point().data(), this->insertion_point().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    ::google::protobuf::internal::WireFormatLite::WriteString(
      2, this->insertion_point(), output);
  }
  
  // optional string content = 15;
  if (has_content()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->content().data(), this->content().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    ::google::protobuf::internal::WireFormatLite::WriteString(
      15, this->content(), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* CodeGeneratorResponse_File::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // optional string name = 1;
  if (has_name()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->name().data(), this->name().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        1, this->name(), target);
  }
  
  // optional string insertion_point = 2;
  if (has_insertion_point()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->insertion_point().data(), this->insertion_point().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        2, this->insertion_point(), target);
  }
  
  // optional string content = 15;
  if (has_content()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->content().data(), this->content().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        15, this->content(), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int CodeGeneratorResponse_File::ByteSize() const {
  int total_size = 0;
  
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional string name = 1;
    if (has_name()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->name());
    }
    
    // optional string insertion_point = 2;
    if (has_insertion_point()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->insertion_point());
    }
    
    // optional string content = 15;
    if (has_content()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->content());
    }
    
  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void CodeGeneratorResponse_File::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const CodeGeneratorResponse_File* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const CodeGeneratorResponse_File*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void CodeGeneratorResponse_File::MergeFrom(const CodeGeneratorResponse_File& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_name()) {
      set_name(from.name());
    }
    if (from.has_insertion_point()) {
      set_insertion_point(from.insertion_point());
    }
    if (from.has_content()) {
      set_content(from.content());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void CodeGeneratorResponse_File::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void CodeGeneratorResponse_File::CopyFrom(const CodeGeneratorResponse_File& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool CodeGeneratorResponse_File::IsInitialized() const {
  
  return true;
}

void CodeGeneratorResponse_File::Swap(CodeGeneratorResponse_File* other) {
  if (other != this) {
    std::swap(name_, other->name_);
    std::swap(insertion_point_, other->insertion_point_);
    std::swap(content_, other->content_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata CodeGeneratorResponse_File::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = CodeGeneratorResponse_File_descriptor_;
  metadata.reflection = CodeGeneratorResponse_File_reflection_;
  return metadata;
}


// -------------------------------------------------------------------

#ifndef _MSC_VER
const int CodeGeneratorResponse::kErrorFieldNumber;
const int CodeGeneratorResponse::kFileFieldNumber;
#endif  // !_MSC_VER

CodeGeneratorResponse::CodeGeneratorResponse()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void CodeGeneratorResponse::InitAsDefaultInstance() {
}

CodeGeneratorResponse::CodeGeneratorResponse(const CodeGeneratorResponse& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void CodeGeneratorResponse::SharedCtor() {
  _cached_size_ = 0;
  error_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

CodeGeneratorResponse::~CodeGeneratorResponse() {
  SharedDtor();
}

void CodeGeneratorResponse::SharedDtor() {
  if (error_ != &::google::protobuf::internal::kEmptyString) {
    delete error_;
  }
  if (this != default_instance_) {
  }
}

void CodeGeneratorResponse::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* CodeGeneratorResponse::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return CodeGeneratorResponse_descriptor_;
}

const CodeGeneratorResponse& CodeGeneratorResponse::default_instance() {
  if (default_instance_ == NULL) 
  {
    protobuf_AddDesc_google_2fprotobuf_2fcompiler_2fplugin_2eproto();  
  }
  return *default_instance_;
}

CodeGeneratorResponse* CodeGeneratorResponse::default_instance_ = NULL;

CodeGeneratorResponse* CodeGeneratorResponse::New() const {
  return new CodeGeneratorResponse;
}

void CodeGeneratorResponse::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (has_error()) {
      if (error_ != &::google::protobuf::internal::kEmptyString) {
        error_->clear();
      }
    }
  }
  file_.Clear();
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool CodeGeneratorResponse::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional string error = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_error()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8String(
            this->error().data(), this->error().length(),
            ::google::protobuf::internal::WireFormat::PARSE);
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(122)) goto parse_file;
        break;
      }
      
      // repeated .google.protobuf.compiler.CodeGeneratorResponse.File file = 15;
      case 15: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_file:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
                input, add_file()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(122)) goto parse_file;
        if (input->ExpectAtEnd()) return true;
        break;
      }
      
      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void CodeGeneratorResponse::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // optional string error = 1;
  if (has_error()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->error().data(), this->error().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    ::google::protobuf::internal::WireFormatLite::WriteString(
      1, this->error(), output);
  }
  
  // repeated .google.protobuf.compiler.CodeGeneratorResponse.File file = 15;
  for (int i = 0; i < this->file_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      15, this->file(i), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* CodeGeneratorResponse::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // optional string error = 1;
  if (has_error()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->error().data(), this->error().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        1, this->error(), target);
  }
  
  // repeated .google.protobuf.compiler.CodeGeneratorResponse.File file = 15;
  for (int i = 0; i < this->file_size(); i++) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        15, this->file(i), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int CodeGeneratorResponse::ByteSize() const {
  int total_size = 0;
  
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional string error = 1;
    if (has_error()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->error());
    }
    
  }
  // repeated .google.protobuf.compiler.CodeGeneratorResponse.File file = 15;
  total_size += 1 * this->file_size();
  for (int i = 0; i < this->file_size(); i++) {
    total_size +=
      ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
        this->file(i));
  }
  
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void CodeGeneratorResponse::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const CodeGeneratorResponse* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const CodeGeneratorResponse*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void CodeGeneratorResponse::MergeFrom(const CodeGeneratorResponse& from) {
  GOOGLE_CHECK_NE(&from, this);
  file_.MergeFrom(from.file_);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_error()) {
      set_error(from.error());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void CodeGeneratorResponse::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void CodeGeneratorResponse::CopyFrom(const CodeGeneratorResponse& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool CodeGeneratorResponse::IsInitialized() const {
  
  return true;
}

void CodeGeneratorResponse::Swap(CodeGeneratorResponse* other) {
  if (other != this) {
    std::swap(error_, other->error_);
    file_.Swap(&other->file_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata CodeGeneratorResponse::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = CodeGeneratorResponse_descriptor_;
  metadata.reflection = CodeGeneratorResponse_reflection_;
  return metadata;
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace compiler
}  // namespace protobuf
}  // namespace google

// @@protoc_insertion_point(global_scope)

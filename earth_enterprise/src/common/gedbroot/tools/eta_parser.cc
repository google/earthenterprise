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
//

#include "common/gedbroot/tools/eta_parser.h"

#include <google/protobuf/stubs/strutil.h>  // For UnescapeCEscapeString.
#include <google/protobuf/stubs/stl_util-inl.h>

#include <utility>
#include <algorithm>
#include <limits>
#include <cwctype>
#include <cerrno>

#include "common/gedbroot/strings/numbers.h"

using std::string;

namespace {

// Parses until next delimiter or end. if delim is ' ', stop on any whitespace.
const char* ParseString(const char* ptr, const char* end,
                        char delim, string* output) {
  const char* last = ptr;
  if (delim == ' ') {
    while (last < end && !iswspace(*last)) {
      ++last;
    }
  } else {
    while (last < end && *last != delim && *last != '\n') {
      ++last;
    }
  }

  if (output) {
    output->assign(ptr, last - ptr);
  }
  if (last < end)
    ++last;
  return last;
}

}  // anonymous namespace

namespace keyhole {

EtaStruct::EtaStruct()
    : parent_(NULL) {}

EtaStruct::~EtaStruct() {
  google::protobuf::STLDeleteElements(&children_);
  google::protobuf::STLDeleteValues(&type_map_);
}

EtaStruct* EtaStruct::Clone() const {
  EtaStruct* clone = new EtaStruct();
  clone->set_name(name());
  clone->set_type(type());
  clone->set_value(value());
  for (int i = 0; i < num_children(); ++i)
    clone->AddChild(child(i)->Clone());

  return clone;
}

const string& EtaStruct::name() const { return name_; }

const string& EtaStruct::type() const { return type_; }

const string& EtaStruct::value() const { return value_; }

bool EtaStruct::has_value() const { return !value_.empty(); }

bool EtaStruct::GetBoolValue() const {
  if (!has_value()) {
    return false;
  }
  // Compare to acceptable true boolean values.
  return value_ == "true" || value_ == "t" || value_ == "1";
}

std::int32_t EtaStruct::GetInt32Value() const {
  std::int32_t value = 0;
  if (has_value()) {
    value = ParseLeadingInt32Value(value_.c_str(), value);
  }
  return value;
}

std::uint32_t EtaStruct::GetUInt32Value() const {
  std::uint32_t value = 0;
  if (has_value()) {
    value = ParseLeadingUInt32Value(value_.c_str(), value);
  }
  return value;
}

double EtaStruct::GetDoubleValue() const {
  double value = 0.0;
  if (has_value()) {
    value = ParseLeadingDoubleValue(value_.c_str(), value);
  }
  return value;
}

float EtaStruct::GetFloatValue() const {
  return static_cast<float>(GetDoubleValue());
}

void EtaStruct::set_name(const string& name) {
  name_ = name;
}

void EtaStruct::set_type(const string& type) {
  if (parent_) {
    parent_->RemoveChildFromMap(this);
  }
  type_ = type;
  if (parent_) {
    parent_->InsertChildInMap(this);
  }
}

void EtaStruct::set_value(const string& value) {
  value_ = value;
}

int EtaStruct::num_children() const {
  return static_cast<int>(children_.size());
}

EtaStruct* EtaStruct::child(int which) {
  CHECK(which >= 0 && static_cast<size_t>(which) < children_.size());
  return children_[which];
}

const EtaStruct* EtaStruct::child(int which) const {
  CHECK(which >= 0 && static_cast<size_t>(which) < children_.size());
  return children_[which];
}

void EtaStruct::AddChild(EtaStruct* child) {
  child->parent_ = this;
  children_.push_back(child);
  InsertChildInMap(child);
}

void EtaStruct::GetChildrenByType(const string& type,
                                  EtaStructVector* output) const {
  CHECK(output != NULL);
  if (type.empty()) {
    return;
  }
  output->clear();
  TypedStructMap::const_iterator it = type_map_.find(type);
  if (it != type_map_.end()) {
    const EtaStructVector* vec = it->second;
    CHECK(vec != NULL);
    output->assign(vec->begin(), vec->end());
  }
}

const EtaStruct* EtaStruct::GetChildByName(const string& name) const {
  return FindChildInList(name, children_);
}

EtaStruct* EtaStruct::GetChildByName(const string& name) {
  const EtaStruct* child = FindChildInList(name, children_);
  return const_cast<EtaStruct*>(child);
}

const EtaStruct* EtaStruct::GetChildByName(const string& name,
                                           const string& type) const {
  EtaStructVector children;
  GetChildrenByType(type, &children);
  return FindChildInList(name, children);
}

// Template type mananagement functions
void EtaStruct::AddTemplate(
    const string& template_name, const EtaStruct* eta_template) {
  if (!template_name.empty() && eta_template)
    templates_[template_name] = eta_template;
}

const EtaStruct* EtaStruct::GetTemplate(const string& template_name) const {
  TemplateMap::const_iterator i = templates_.find(template_name);
  if (i != templates_.end()) {
    return i->second;
  } else {
    return NULL;
  }
}

void EtaStruct::InsertChildInMap(EtaStruct* child) {
  CHECK(child != NULL);
  const string& type_str = child->type();
  if (type_str.empty())
    return;
  TypedStructMap::iterator it = type_map_.find(type_str);
  EtaStructVector* vec = NULL;
  if (it == type_map_.end()) {
    vec = new EtaStructVector();
    type_map_.insert(std::make_pair(type_str, vec));
  } else {
    vec = it->second;
  }
  CHECK(vec != NULL);
  vec->push_back(child);
}

void EtaStruct::RemoveChildFromMap(EtaStruct* child) {
  CHECK(child != NULL);
  if (child->type().empty()) {
    return;
  }
  TypedStructMap::const_iterator it = type_map_.find(child->type());
  if (it != type_map_.end()) {
    // Remove child in vector.
    EtaStructVector* vec = it->second;
    CHECK(vec != NULL);
    vec->erase(std::remove(vec->begin(), vec->end(), child), vec->end());
  }
}

// Helper functor to find structs by name.
struct SameNameFunctor {
  explicit SameNameFunctor(const string& eta_name): name(&eta_name) {}

  bool operator()(const EtaStruct* eta_struct) {
    return eta_struct->name() == *name;
  }
 private:
  const string* name;
};

const EtaStruct* EtaStruct::FindChildInList(const string& name,
                                            const EtaStructVector& children) {
  EtaStructVector::const_iterator it =
      std::find_if(children.begin(), children.end(), SameNameFunctor(name));
  if (it != children.end()) {
    return *it;
  }
  return NULL;
}

//------------------------------------------------------------------------------
// EtaDocument class

EtaDocument::EtaDocument()
: root_(TransferOwnership(new EtaStruct())) {
}

EtaDocument::~EtaDocument() {
}

void EtaDocument::ParseFromString(const string& input) {
  const char* ptr = input.data();
  const char* end = ptr + input.size();

  ParsePartial(ptr, end, *&root_);
}

const EtaStruct* EtaDocument::root() const {
  return *&root_;
}

const char* EtaDocument::ParsePartial(const char* ptr, const char* end,
                                      EtaStruct* parent) {
  string current_type;
  string current_name;
  string current_value;
  EtaStruct* current_struct = NULL;
  int num_parsed = 0;

  while (ptr < end) {
    char c = *ptr;
    const char* next_ptr = ptr + 1;
    switch (c) {
      case '<':
        // '<' marks the beginning of a type. Parse type until next closing
        // angle bracket.
        ptr = ParseString(next_ptr, end, '>', &current_type);
        break;
      case '[':
        // '[' marks the beginning of a struct. Parse name until next closing
        // square bracket and add new struct to parent.
        ptr = ParseString(next_ptr, end, ']', &current_name);

        // Clone from template if available.
        if (current_struct == NULL) {
          if (const EtaStruct* eta_template =
              root_->GetTemplate(current_type)) {
            current_struct = eta_template->Clone();
            current_struct->set_name(current_name);
            parent->AddChild(current_struct);
          }
        }

        // Re-use existing struct if one exists with the same name.
        if (current_struct == NULL) {
          current_struct = parent->GetChildByName(current_name);
        }

        if (current_struct == NULL) {
          current_struct = new EtaStruct();
          current_struct->set_name(current_name);
          parent->AddChild(current_struct);
        }

        current_struct->set_type(current_type);

        // Prevent current type and name from "leaking" to next value.
        current_name.clear();
        current_type.clear();
        break;
      case '#':
        // Beginning of a comment, skip until end of line
        ptr = ParseString(next_ptr, end, '\n', NULL);
        break;
      case '{':
        // Recurse into next structure.
        if (current_struct != NULL) {
          ptr = ParsePartial(next_ptr, end, current_struct);

          MaybeAddTemplate(current_struct);

          // Done editing current_struct structure.
          current_struct = NULL;
        }
        break;
      case '}':
        // Done with current recursion level, break out of whole loop.
        return next_ptr;
        break;
      default:
        // If we hit a whitespace, eat all of them until we have a
        // non-whitespace character.
        if (iswspace(*ptr)) {
          // Consume whitespace
          while (ptr < end && iswspace(*ptr)) {
            ++ptr;
          }
          break;
        }

        // Consume until next delimiter. If current character is a double
        // quote, use double quote as delimiter, otherwise use space.
        char delim = ' ';
        if (ptr[0] == '"') {
          delim = '"';
          ++ptr;
        }
        ptr = ParseString(ptr, end, delim, &current_value);

        if (current_struct == NULL) {
          // In case this is a template struct instance, we should override
          // the default values set by the template struct.
          if (root_->GetTemplate(parent->type()) &&
              num_parsed < parent->num_children()) {
            current_struct = parent->child(num_parsed);
          } else {
            current_struct = new EtaStruct();
            parent->AddChild(current_struct);
          }
        }

        current_struct->set_value(current_value);
        ++num_parsed;

        // After getting value for current_struct, it cannot be modified any
        // more. Any other content is for next current_struct for this parent,
        // so release current current_struct pointer (owned by parent at this
        // point).
        current_struct = NULL;

        break;
    }
    CHECK(ptr >= next_ptr);
  }
  return NULL;
}

void EtaDocument::MaybeAddTemplate(const EtaStruct* eta_struct) {
  static const char kTemplateType[] = "etTemplate";
  const string& name = eta_struct->name();
  if (eta_struct->type() != kTemplateType || eta_struct->name().empty()) {
    return;
  }
  // Strip "<" and ">" from name.
  if (name[0] == '<' && name[name.size() - 1] == '>') {
    string template_name;
    template_name.assign(name.begin() + 1, name.end() - 1);
    root_->AddTemplate(template_name, eta_struct);
  }
}

}  // namespace keyhole

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


#ifndef GOOGLE3_KEYHOLE_TOOLS_DBROOT_ETA_PARSER_H_
#define GOOGLE3_KEYHOLE_TOOLS_DBROOT_ETA_PARSER_H_

#include <common/base/macros.h>

#include <tr1/unordered_map>
#include <memory>
#include <string>
#include <vector>

//#include "common/khTypes.h"
#include <cstdint>
#include "common/notify.h"
#include "common/khGuard.h"

namespace keyhole {

#ifndef CHECK
#define CHECK(condition)  { \
    if (!(condition)) \
      notify(NFY_FATAL, "Caught Exception at %s:%d", __FILE__, __LINE__); \
}
#endif

class EtaStruct {
 public:
  EtaStruct();
  virtual ~EtaStruct();

  // Deep copy. Caller becomes owner of new object.
  EtaStruct* Clone() const;

  const std::string& name() const;
  const std::string& type() const;
  const std::string& value() const;

  bool has_value() const;

  // Returns value converted to bool, false by default.
  bool GetBoolValue() const;

  // Converts value to std::int32_t, returns 0 if conversion cannot be done.
  std::int32_t GetInt32Value() const;

  // Converts value to std::int32_t, returns 0 if conversion cannot be done.
  std::uint32_t GetUInt32Value() const;

  // Converts value to float, returns 0.0f if conversion cannot be done.
  float GetFloatValue() const;

  // Converts value to double, returns 0.0 if conversion cannot be done.
  double GetDoubleValue() const;

  void set_name(const std::string& name);
  void set_type(const std::string& type);
  void set_value(const std::string& value);

  int num_children() const;
  EtaStruct* child(int which);
  const EtaStruct* child(int which) const;

  // Takes ownership of child.
  void AddChild(EtaStruct* child);

  // Finds all children with given type. Order in vector is the same
  // as long as the type of each child is set before parenting it.
  void GetChildrenByType(const std::string& type,
                         std::vector<EtaStruct*>* output) const;

  // Finds first child with given name (slow).
  const EtaStruct* GetChildByName(const std::string& name) const;
  EtaStruct* GetChildByName(const std::string& name);

  // Finds first child with given name and type.
  const EtaStruct* GetChildByName(const std::string& name,
                                  const std::string& type) const;

  void AddTemplate(const std::string& template_name,
                   const EtaStruct* eta_template);
  const EtaStruct* GetTemplate(const std::string& template_name) const;

 private:
  typedef std::vector<EtaStruct*> EtaStructVector;
  typedef std::tr1::unordered_map<std::string, EtaStructVector*> TypedStructMap;
  typedef std::tr1::unordered_map<std::string, const EtaStruct*> TemplateMap;
  TemplateMap templates_;

  void InsertChildInMap(EtaStruct* child);
  void RemoveChildFromMap(EtaStruct* child);

  static const EtaStruct* FindChildInList(const std::string& name,
                                          const EtaStructVector& children);

  std::string name_;
  std::string type_;
  std::string value_;
  EtaStructVector children_;
  EtaStruct* parent_;
  // Maps a type string to vector of children (in the same order as
  // in children_).
  TypedStructMap type_map_;

  DISALLOW_COPY_AND_ASSIGN(EtaStruct);
};

class EtaDocument {
 public:
  EtaDocument();
  virtual ~EtaDocument();

  void ParseFromString(const std::string& input);

  const EtaStruct* root() const;

 private:
  const char* ParsePartial(const char* ptr, const char* end, EtaStruct* parent);
  void MaybeAddTemplate(const EtaStruct* eta_struct);

  khDeleteGuard<EtaStruct> root_;

  DISALLOW_COPY_AND_ASSIGN(EtaDocument);
};

}  // namespace keyhole

#endif  // GOOGLE3_KEYHOLE_TOOLS_DBROOT_ETA_PARSER_H_

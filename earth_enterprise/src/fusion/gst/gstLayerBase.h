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

#ifndef KHSRC_FUSION_GST_GSTLAYERBASE_H__
#define KHSRC_FUSION_GST_GSTLAYERBASE_H__

#include <gstTypes.h>
#include <gstMemory.h>

class gstLayerBase {
 public:
  gstLayerBase(const std::string& name);
  gstLayerBase(const gstLayerBase& l);
  virtual ~gstLayerBase();

  unsigned int sortID() const { return sort_id_; }
  void sortID(unsigned int i);

  const std::string& Name() const { return name_; }
  void SetName(const std::string& name) { name_ = name; }

 private:
  void operator=(const gstLayerBase&);  // private and unimplemented
  std::string name_;
  unsigned int sort_id_;
};

#endif  // !KHSRC_FUSION_GST_GSTLAYERBASE_H__

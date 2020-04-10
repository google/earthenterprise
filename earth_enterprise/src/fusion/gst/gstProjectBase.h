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

#ifndef KHSRC_FUSION_GST_GSTPROJECTBASE_H__
#define KHSRC_FUSION_GST_GSTPROJECTBASE_H__

#include <vector>
#include <gstMemory.h>

class gstLayerBase;

class gstProjectBase {
 public:
  gstProjectBase(const std::string& name);
  virtual ~gstProjectBase();

  // derived clases must re-implement these
  virtual bool load(const char* path) = 0;

  void removeLayer(gstLayerBase* layer);
  void swapLayers(gstLayerBase* a, gstLayerBase* b);

  unsigned int numLayers() const { return layers_.size(); }
  void addLayer(gstLayerBase* layer);
  void insertLayer(gstLayerBase* layer, unsigned int pos);
  gstLayerBase* getLayer(int id) { return layers_[id]; }

  void setModified(bool m = true);
  bool isModified() const;

  const std::string& Name() const { return name_; }
  void SetName(const std::string& n) { name_ = n; }
  bool Untitled() const { return name_.empty(); }

 protected:
  std::vector<gstLayerBase*> layers_;

 private:
  std::string name_;
  bool modified_;
};

#endif  // !KHSRC_FUSION_GST_GSTPROJECTBASE_H__

// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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


#include <vector>
#include <algorithm>

#include <gstProjectBase.h>
#include <gstLayerBase.h>

gstProjectBase::gstProjectBase(const std::string& name)
    : name_(name),
      modified_(false) {
}

gstProjectBase::~gstProjectBase() {
}

void gstProjectBase::setModified(bool m) {
  modified_ = m;
}

bool gstProjectBase::isModified() const {
  return modified_;
}


void gstProjectBase::addLayer(gstLayerBase* layer) {
  layer->sortID(layers_.size());
  layers_.push_back(layer);
  setModified();
}

void gstProjectBase::insertLayer(gstLayerBase* layer, unsigned int pos) {
  std::vector<gstLayerBase*>::iterator renumber =
    layers_.insert(layers_.begin() + pos, layer);
  for (; renumber != layers_.end(); ++renumber, ++pos)
    (*renumber)->sortID(pos);
  setModified();
}

void gstProjectBase::removeLayer(gstLayerBase* layer) {
  setModified();

  std::vector<gstLayerBase*>::iterator found =
    find(layers_.begin(), layers_.end(), layer);

  if (found != layers_.end()) {
    std::vector<gstLayerBase*>::iterator tail = layers_.erase(found);
    for (int id = layer->sortID(); tail != layers_.end(); ++tail, ++id)
      (*tail)->sortID(id);
    delete layer;
  }
}

void gstProjectBase::swapLayers(gstLayerBase* a, gstLayerBase* b) {
  layers_[a->sortID()] = b;
  layers_[b->sortID()] = a;

  unsigned int tmp = a->sortID();
  a->sortID(b->sortID());
  b->sortID(tmp);

  setModified();
}

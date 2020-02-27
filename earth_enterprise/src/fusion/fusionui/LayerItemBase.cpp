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


#include "LayerItemBase.h"

LayerItemBase::LayerItemBase(Q3ListView* parent)
    : Q3ListViewItem(parent) {
}

LayerItemBase::LayerItemBase(Q3ListViewItem* parent)
    : Q3ListViewItem(parent) {
}

bool LayerItemBase::CanMoveUp() const {
  return Previous() != NULL;
}

bool LayerItemBase::CanMoveDown() const {
  return nextSibling() != 0;
}

void LayerItemBase::SwapPosition(LayerItemBase* item) {
  if (item == Next()) {
    moveItem(item);
  } else if (item == Previous()) {
    item->moveItem(this);
  }
}

LayerItemBase* LayerItemBase::Next() const {
  return static_cast<LayerItemBase*>(nextSibling());
}

LayerItemBase* LayerItemBase::Previous() const {
  LayerItemBase* item;
  if (parent() == 0) {
    item = static_cast<LayerItemBase*>(listView()->firstChild());
  } else {
    item = static_cast<LayerItemBase*>(parent()->firstChild());
  }
  if (item == this)
    return NULL;
  while (item->Next() != this)
    item = item->Next();
  return item;
}

int LayerItemBase::SiblingCount() const {
  if (parent() == 0) {
    return listView()->childCount();
  } else {
    return parent()->childCount();
  }
}

void LayerItemBase::MoveUp() {
  if (!CanMoveUp())
    return;
  SwapPosition(Previous());
}

void LayerItemBase::MoveDown() {
  if (!CanMoveDown())
    return;
  SwapPosition(Next());
}

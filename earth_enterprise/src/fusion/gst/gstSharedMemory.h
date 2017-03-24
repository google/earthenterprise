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

#ifndef _gstSharedMemory_h_
#define _gstSharedMemory_h_

#include <khArray.h>
#include <gstCallback.h>
#include <gstTypes.h>


class gstSharedObject {
 public:
  gstSharedObject(const char *n)
  {
    _name = new char[strlen(n) + 1];
    strcpy(_name, n);
  }

  const char *name() { return _name; }

  void *get() { return _val; }
  void set(void *v) {
    if (_val == v)
      return;
    _val = v;
    for (int ii = 0; ii < _callbacks.length(); ii++)
      _callbacks[ii]->invoke(_val);
  }

  void notification(gstCallbackFunc func, void *obj)
  {
    _callbacks.append(new gstCallback(func, obj));
  }

 private:
  char *_name;
  void *_val;

  khArray<gstCallback*> _callbacks;
};

typedef khArray<gstSharedObject*> SharedObjectList;


class gstSharedMemory {

 public:
  gstSharedMemory()
  {
    _objects.init(10);
  }

  gstSharedObject *allocate(const char *name)
  {
    gstSharedObject *obj = new gstSharedObject(name);
    _objects.append(obj);
    return obj;
  }

  gstSharedObject *retrieve(const char *name)
  {
    int handle = getHandle(name);
    if (handle == -1)
      return NULL;
    return _objects[handle];
  }

 private:

  int getHandle(const char *name)
  {
    int obj = 0;
    while (obj < _objects.length()) {
      if (!strcmp(_objects[obj]->name(), name))
        return obj;
      obj++;
    }
    return -1;
  }

  SharedObjectList _objects;

};

extern gstSharedMemory *theSharedMemory;

#endif //!_gstSharedMemory_h_

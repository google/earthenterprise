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

#ifndef KHSRC_FUSION_GST_GSTMEMORY_H__
#define KHSRC_FUSION_GST_GSTMEMORY_H__

#include <signal.h>
#include <pthread.h>

#include <gstMisc.h>
#include <notify.h>

// instantiated in gstInit.c++
extern pthread_mutex_t MemoryMutex;

class gstMemory {
 public:
  gstMemory(const char* n = NULL)
      : refcount_(1),
        destroyed_(0) {
    name_ = strdupSafe(n);
  }

  virtual ~gstMemory() {
    if (destroyed_) {
      notify(NFY_WARN,
             "Trying to delete gstMemory object that has already been deleted!");
      raise(SIGSEGV);
      return;
    }
    destroyed_ = 1;
    delete [] name_;
  }

  virtual const char* name() const { return name_; }

  virtual void setName(const char* n) {
    if (name_)
      delete [] name_;
    name_ = strdupSafe(n);
  }

  void ref() {
    pthread_mutex_lock(&MemoryMutex);
    refcount_++;
    pthread_mutex_unlock(&MemoryMutex);
  }

  void unref() {
    pthread_mutex_lock(&MemoryMutex);
    refcount_--;
    int newval = refcount_;
    pthread_mutex_unlock(&MemoryMutex);
    if (newval == 0) {
      delete this;
    } else if (newval < 0) {
      notify(NFY_WARN,
             "Trying to delete gstMemory object with a ref count less than 0!");
      raise(SIGSEGV);
    }
  }

  int getRef() const { return refcount_; }

 protected:
  char* name_;

 private:
  int refcount_;
  int destroyed_;
};

#endif  // !KHSRC_FUSION_GST_GSTMEMORY_H__

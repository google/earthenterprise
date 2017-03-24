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

#ifndef FUSION_WORKFLOW_PROCESSOR_H__
#define FUSION_WORKFLOW_PROCESSOR_H__

#include <vector>
#include <exception>
#include <common/base/macros.h>

namespace workflow {


// Reset the variable in preparation for Workflow processing The default is
// to assign an empty object. If your object doesn't support assignment (or
// if you have a more efficient way to reset), then define your own
// ResetTile function for your class.
template <class T>
inline void ResetTile(T *t) {
  *t = T();
}

template <class T>
class ResetTileOnException {
 public:
  ResetTileOnException(T *t_) : t(t_) { }
  ~ResetTileOnException(void) {
    if (std::uncaught_exception()) {
      ResetTile(t);
    }
  }

 private:
  T* t;
};


template <class Out, class In>
class Processor {
 public:
  typedef Out OutType;
  typedef In  InType;

  Processor(void) { }
  virtual ~Processor(void) { }

  virtual bool Process(Out *out, const In &in) = 0;
 private:
  DISALLOW_COPY_AND_ASSIGN(Processor);
};

template <class Out, class In>
class AggregateProcessor {
 public:
  typedef Out OutType;
  typedef In  InType;

  AggregateProcessor(void) { }
  virtual ~AggregateProcessor(void) { }

  virtual bool Process(Out *out, const std::vector<const In*> &in) = 0;
 private:
  DISALLOW_COPY_AND_ASSIGN(AggregateProcessor);
};


} // namespace workflow



#endif // FUSION_WORKFLOW_PROCESSOR_H__

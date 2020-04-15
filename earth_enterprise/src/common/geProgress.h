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


#ifndef COMMON_GEPROGRESS_H_
#define COMMON_GEPROGRESS_H_

#include "khTypes.h"
#include <cstdint>

class geProgress
{
 public:
  virtual ~geProgress() { }
  virtual bool incrementTotal(std::int64_t val) = 0;
  virtual bool incrementDone(std::int64_t val) = 0;
};

#endif /* COMMON_GEPROGRESS_H_ */


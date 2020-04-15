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


#include <assert.h>
#include "SoftErrorPolicy.h"


SoftErrorPolicy::SoftErrorPolicy(unsigned int max_soft_before_fatal) :
    max_soft_before_fatal_(max_soft_before_fatal)
{
  assert(max_soft_before_fatal_ < 100);
  error_messages_.reserve(max_soft_before_fatal_);
}


void SoftErrorPolicy::HandleSoftError(const QString &error) {
  if (max_soft_before_fatal_ == 0) {
    throw khException(error);
  }
  error_messages_.push_back((const char *)error.utf8());
  if (NumSoftErrors() >= max_soft_before_fatal_) {
    throw TooManyException(error_messages_);
  }
}

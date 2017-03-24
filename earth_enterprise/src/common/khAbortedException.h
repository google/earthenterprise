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

//

#ifndef COMMON_KHABORTEDEXCEPTION_H__
#define COMMON_KHABORTEDEXCEPTION_H__

#include "khException.h"


// Used when throwing an exception triggered by somebody else aborting.
// The place that triggered the abort will already have emitted a message.
// This specialization allows placed that distinguish between a
// khAbortedExcetpion and a normal khException to not emit another message
// for each aborted exception.
class khAbortedException : public khException
{
 public:
  static const QString Message;

  khAbortedException(void) : khException(Message) { }
};


#endif // COMMON_KHABORTEDEXCEPTION_H__

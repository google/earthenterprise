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

#ifndef __AssetThrowPolicy_h
#define __AssetThrowPolicy_h

#include <khException.h>
#include <notify.h>

class AssetThrowPolicy {
 public:
  static bool allow_throw;
  static void WarnOrThrow(const QString &msg) {
    if (allow_throw) {
      throw khException(msg);
    } else {
      notify(NFY_WARN, "%s", msg.latin1());
    }
  }
  static void FatalOrThrow(const QString &msg) {
    if (allow_throw) {
      throw khException(msg);
    } else {
      notify(NFY_FATAL, "%s", msg.latin1());
    }
  }
};



#endif /* __AssetThrowPolicy_h */

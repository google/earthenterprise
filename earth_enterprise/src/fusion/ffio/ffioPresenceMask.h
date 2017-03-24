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


/******************************************************************************
File:        ffioPresenceMask.h
******************************************************************************/
#ifndef __ffioPresenceMask_h
#define __ffioPresenceMask_h

#include <string>
#include <khPresenceMask.h>

namespace ffio {

inline std::string PresenceFilename(const std::string &ffdir) {
  return ffdir + "/pack.presence";
}

class WaitBase {
 public:
  WaitBase(const std::string &filename);
};

// wrapper that waits if file is too new
class PresenceMask : private WaitBase, public khPresenceMask {
 public:
  PresenceMask(const std::string &filename);
};

} // namespace ffio


#endif /* __ffioPresenceMask_h */

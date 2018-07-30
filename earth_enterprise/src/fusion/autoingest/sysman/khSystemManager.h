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

#ifndef __khSystemManager_h
#define __khSystemManager_h

#include <khMTTypes.h>


class khSystemManager  {
  khMutex mutex;
  bool wantexit;

  void SignalLoop(void);
 public:
  bool WantExit(void) {
    khLockGuard guard(mutex);
    return wantexit;
  }
  void SetWantExit(void);

  khSystemManager(void);
  virtual ~khSystemManager(void);
  void Run(void);
};

extern khSystemManager theSystemManager;

#endif /* __khSystemManager_h */

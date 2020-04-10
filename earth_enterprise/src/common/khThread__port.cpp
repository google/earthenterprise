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

// Ports filebundle.cpp for different OS.

#include <common/port_flags.h>
#include <errno.h>
#include "khThread.h"

void khMutexBase::Lock(void) {}
void khMutexBase::Unlock(void) {}
bool khMutexBase::TimedTryLock(unsigned int) { return true; }
khMutex::khMutex(void) {};
khMutex::~khMutex() {};

khCondVar::khCondVar(void) {}
khCondVar::~khCondVar() {}
void khCondVar::signal_one(void) {}
void khCondVar::broadcast_all(void) {}
void khCondVar::wait(khMutexBase &mutex) {}

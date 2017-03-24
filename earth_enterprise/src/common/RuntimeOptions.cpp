// Copyright 2017 Google Inc.
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

//

#include "common/RuntimeOptions.h"
#include <stdlib.h>
#include <notify.h>

bool RuntimeOptions::EnableAll_        = false;
bool RuntimeOptions::ExpertMode_       = false;
bool RuntimeOptions::ExperimentalMode_ = false;
bool RuntimeOptions::GoogleInternal_   = false;
bool RuntimeOptions::EnablePyGeePublisher_ = false;

void RuntimeOptions::Init(void) {
  static bool initOnce = false;
  if (initOnce) return;
  initOnce = true;

  if (getenv("KH_ENABLE_ALL"))
    EnableAll_ = true;

  if (getenv("KH_EXPERT"))
    ExpertMode_ = true;

  if (getenv("KH_EXPERIMENTAL"))
    ExperimentalMode_ = true;

  if (getenv("KH_GOOGLE"))
    GoogleInternal_ = true;

  if (getenv("KH_ENABLE_PY_GEE_PUBLISHER"))
    EnablePyGeePublisher_ = true;
}

QString RuntimeOptions::DescString(void) {
  Init();
  QString desc;
  if (ExpertMode_) {
    if (!desc.isEmpty()) {
      desc += " | ";
    }
    desc += "Expert";
  }
  if (EnableAll_) {
    if (!desc.isEmpty()) {
      desc += " | ";
    }
    desc += "EnableAll";
  }
  if (ExperimentalMode_) {
    if (!desc.isEmpty()) {
      desc += " | ";
    }
    desc += "Experimental";
  }
  if (GoogleInternal_) {
    if (!desc.isEmpty()) {
      desc += " | ";
    }
    desc += "GoogleInternal";
  }
  if (EnablePyGeePublisher_) {
    if (!desc.isEmpty()) {
      desc += " | ";
    }
    desc += "EnablePyGeePublisher";
  }
  if (getNotifyLevel() != NFY_DEFAULT_LEVEL) {
    if (!desc.isEmpty()) {
      desc += " | ";
    }
    desc += "Notify=" + QString::number(getNotifyLevel());
  }
  return desc;
}

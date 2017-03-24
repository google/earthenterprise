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


#include <gstProgress.h>
#include <gstMisc.h>

gstProgress::gstProgress() {
  Reset();
}

gstProgress::~gstProgress() {
}

void gstProgress::Reset(void) {
  khLockGuard lock(mutex);
  val_ = 0;
  state_ = Busy;
  errors_.clear();
  warning_.clear();
}

int gstProgress::Val() const {
  khLockGuard lock(mutex);
  return val_;
}
std::string gstProgress::Warning() const {
  khLockGuard lock(mutex);
  return warning_;
}
std::vector<std::string> gstProgress::Errors(void) const {
  khLockGuard lock(mutex);
  return errors_;
}

gstProgress::State gstProgress::GetState(void) const {
  khLockGuard lock(mutex);
  return state_;
}



// ****************************************************************************
// ***  Set routines
// ****************************************************************************
void gstProgress::SetVal(int v) {
  khLockGuard lock(mutex);
  val_ = v;
}

void gstProgress::SetWarning(const std::string &warning) {
  khLockGuard lock(mutex);
  warning_ = warning;
}

void gstProgress::SetInterrupted(void) {
  khLockGuard lock(mutex);
  state_ = Interrupted;
}
void gstProgress::SetSucceeded(void) {
  khLockGuard lock(mutex);
  state_ = Succeeded;
}

void gstProgress::SetFatalError(const std::string &error) {
  khLockGuard lock(mutex);
  state_ = Failed;
  errors_.push_back(error);
}

void gstProgress::SetFatalErrors(const std::vector<std::string> &errors) {
  khLockGuard lock(mutex);
  state_ = Failed;
  std::copy(errors.begin(), errors.end(), back_inserter(errors_));
}

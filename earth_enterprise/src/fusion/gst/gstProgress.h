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

#ifndef KHSRC_FUSION_GST_GSTPROGRESS_H__
#define KHSRC_FUSION_GST_GSTPROGRESS_H__

#include <qstring.h>
#include <khThread.h>

class gstProgress {
 public:
  enum State { Busy, Interrupted, Succeeded, Failed };

  gstProgress();
  ~gstProgress();

  //  void Init(int v = 0, bool b = true, bool i = false);
  void Reset(void);

  // these update other variable but not the state
  void SetVal(int v);
  void SetWarning(const std::string &warning);

  // these change the state
  void SetInterrupted(void);
  void SetSucceeded(void);
  void SetFatalError(const std::string &error);
  void SetFatalErrors(const std::vector<std::string> &errors);

  State GetState(void) const;
  int Val(void) const;
  // intentionally return these by value, rather than reference
  std::string Warning(void) const;
  std::vector<std::string> Errors(void) const;

 private:
  mutable khMutex mutex;

  // use std::string instead of QString because QString has come problems w/
  // MT access to its shared version of "empty".
  std::vector<std::string> errors_;
  std::string warning_;

  int val_;
  State state_;
};


#endif  // !KHSRC_FUSION_GST_GSTPROGRESS_H__

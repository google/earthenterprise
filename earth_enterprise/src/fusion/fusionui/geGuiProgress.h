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


#ifndef FUSIONUI_GEGUIPROGRESS_H_
#define FUSIONUI_GEGUIPROGRESS_H_

#include <geProgress.h>
#include <khThread.h>

class geGuiProgress : public geProgress
{
 public:
  geGuiProgress(int64 total)
  : total_(total), done_(0), canceled_(false) { }

  bool incrementTotal(int64 val);
  bool incrementDone(int64 val);
  void setCanceled();
  int64 total();
  int64 done();

 private:
  int64 total_;
  int64 done_;
  khMutex mutex_;
  bool canceled_;
};

#endif /* FUSIONUI_GEGUIPROGRESS_H_ */

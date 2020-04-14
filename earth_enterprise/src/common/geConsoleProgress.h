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


#ifndef COMMON_GECONSOLEPROGRESS_H_
#define COMMON_GECONSOLEPROGRESS_H_

#include "geProgress.h"
#include "khProgressMeter.h"

class geConsoleProgress : public geProgress
{
 public:
  geConsoleProgress(std::int64_t total, const QString& desc = "tiles")
  : progress_meter_(total, desc) { }

  bool incrementTotal(std::int64_t val);
  bool incrementDone(std::int64_t val);

 private:
  khProgressMeter progress_meter_;
};

#endif /* COMMON_GECONSOLEPROGRESS_H_ */


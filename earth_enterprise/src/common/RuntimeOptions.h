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

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_RUNTIMEOPTIONS_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_RUNTIMEOPTIONS_H_

#include <qstring.h>

class RuntimeOptions {
  static bool EnableAll_;
  static bool ExpertMode_;
  static bool ExperimentalMode_;
  static bool GoogleInternal_;
  // TODO: temporary variable should be deleted
  // after complete switching to new Python Publisher.
  static bool EnablePyGeePublisher_;

  static void Init(void);

 public:
  static inline bool EnableAll(void) {
    Init();
    return EnableAll_;
  }
  static bool ExpertMode(void) {
    Init();
    return ExpertMode_;
  }
  static bool ExperimentalMode(void) {
    Init();
    return ExperimentalMode_;
  }
  static bool GoogleInternal(void) {
    Init();
    return GoogleInternal_;
  }
  static bool EnablePyGeePublisher(void) {
    Init();
    return EnablePyGeePublisher_;
  }

  static QString DescString(void);
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_RUNTIMEOPTIONS_H_

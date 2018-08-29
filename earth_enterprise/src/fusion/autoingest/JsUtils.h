/*
 * Copyright 2017 Google Inc. Copyright 2018 the Open GEE Contributors.
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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_JSUTILS_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_JSUTILS_H_

#include <string>


// JsUtils is a static utility class for JS
class JsUtils {

 public:
  JsUtils() = delete;
  JsUtils(const JsUtils&) = delete;

  // Utility for converting a string with a Java Script object definition
  // to a string with a JSON format.  Note: this function does not validate
  // the JS object definition string and assumes the input is valid JS.  If
  // the input is not valid JS undefined results will happen.
  static std::string JsToJson(const std::string& in);

};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_AUTOINGEST_JSUTILS_H_

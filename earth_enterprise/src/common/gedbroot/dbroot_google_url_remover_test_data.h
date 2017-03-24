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


#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_DBROOT_GOOGLE_URL_REMOVER_TEST_DATA_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_DBROOT_GOOGLE_URL_REMOVER_TEST_DATA_H_

enum TextBasedTestCases {
  kActualExample,
  kDisableAuthenticationRequirement,
  kLeavesOptionalSiblings,
};


extern const char* TEST_DBROOT_BASE;
extern const char* CANON_CLEANED_ALLFIELDS;
extern const char* HAND_DBROOTS[];
extern const char* CANON_CLEANED_HAND_DBROOTS[];
extern const char* GOOGLEFREE_NON_DEFAULT_FIELD_PATHS[];
extern const char* GOOGLISH_NON_DEFAULT_FIELD_PATHS[];
extern const char* GOOGLE_DEFAULTED_FIELD_PATHS[];

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_DBROOT_GOOGLE_URL_REMOVER_TEST_DATA_H_

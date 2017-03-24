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


#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_DBROOT_GOOGLE_URL_REMOVER_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_DBROOT_GOOGLE_URL_REMOVER_H_

class geProtoDbroot;

// Expose general method in case a client wants to play with it.
// dbroot: the proto-based dbroot to be cleared.
// fieldpaths: an array of fieldpaths (see
//   dbroot_v5_google_url_remover.cpp for examples).
// fieldpaths_size: number of elements of <fieldpaths>.
// mask_for_defaults: text to put where there's an exposed default.
//   By default it's: "" (empty string).
bool RemoveUnwantedDomains(
    geProtoDbroot* dbroot,
    const char* fieldpaths[], int fieldpaths_size,
    const char* mask_for_defaults);

// Like RemoveUnwantedDomains above, except that all of the parameters
// are built-in. Probably what should almost always be called.
bool RemoveUnwantedGoogleUrls(geProtoDbroot* dbroot);

extern const char* GOOGLEFREE_FIELDS[];

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_GEDBROOT_DBROOT_GOOGLE_URL_REMOVER_H_

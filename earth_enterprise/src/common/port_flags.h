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

// This file is a place holder for all macros for porting to various platforms

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_PORT_FLAGS_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_PORT_FLAGS_H_

// Define SINGLE_THREAD since we don't have a requirement yet for supporting
// multi-threading in windows. In future this may change
#define SINGLE_THREAD 1

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_PORT_FLAGS_H_

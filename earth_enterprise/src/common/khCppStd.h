/*
 * Copyright 2018 Google Inc.
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


#ifndef _khcpp11_h_
#define _khcpp11_h_

// This will help with transitioning from C++98 to C++11.  Once fully transitioned over
// we should stop using this file and delete this file

#if __cplusplus <= 199711L    // C++98
#  define nullptr NULL
#else  // C++11 or greater
#  define GEE_HAS_MOVE 1
#  define GEE_HAS_STATIC_ASSERT 1
#endif // __cplusplus

#endif // _khcpp11_h_
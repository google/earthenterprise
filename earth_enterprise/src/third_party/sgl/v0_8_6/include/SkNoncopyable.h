/* include/graphics/SkNoncopyable.h
**
** Copyright 2006, Google Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#ifndef SkNoncopyable_DEFINED
#define SkNoncopyable_DEFINED

#include "SkTypes.h"

/** \class SkNoncopyable

    SkNoncopyable is the base class for objects that may do not want to
    be copied. It hides its copy-constructor and its assignment-operator.
*/
class SkNoncopyable {
public:
    SkNoncopyable() {}

private:
    SkNoncopyable(const SkNoncopyable&);
    SkNoncopyable& operator=(const SkNoncopyable&);
};

#endif


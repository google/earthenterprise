# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

This directory simply defines a SConscript to build the gtest
library for use in unit testing.


To use gtest :

1) the xxx_unittest.cpp should:

#include "third_party/gtest/include/gtest/gtest.h"

and follow the normal GUnit test procedure.

2) the SConscript for a unit test should call the following:

env.test('xxx_unittest', 'xxx_unittest.cpp', LIBS=[getest])


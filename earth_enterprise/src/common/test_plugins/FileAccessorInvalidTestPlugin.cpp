// Copyright 2020 The Open GEE Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*
* The purpose of this file is to produce, as part of the build, a shared library
* that can be loaded by `dlopen` but that does not contain the function that
* the FileAccessorPluginLoader requires. 
* The resulting shared libary is used only by the unit tests.
*/
extern "C" {
  int add_numbers () {return 1 + 2;}
}

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
* The purpose of this file is to produce, as part of the build, an executable
* file that the plugin loader unit tests can attempt, and fail, to load.
*/
int main(void) {
    // Just do something basic and safe
    int x = 1;
    int y = x;
    x = y;
    return 0;
}
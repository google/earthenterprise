// Copyright 2017 Google Inc.
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


#include "khstrconv.h"

#include <stdio.h>

#include "khGuard.h"

bool FromStringStrict(const std::string str, float *f) {
  // Parse and require at least one value to be present.
  if (sscanf(str.c_str(), "%f", f) != 1)
    return false;
  // Make sure no extra characters are present.
  khDeleteGuard<char> buffer(TransferOwnership(new char[str.length() + 1]));
  strncpy(buffer, str.c_str(), (str.length() + 1));
  char *current = buffer;
  strtod(current, &current);
  // Return success iff we parsed to the end of the input.
  return *current == '\0';
}

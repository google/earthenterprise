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

#include "uuid.h"
#include "rfc_uuid.h"
#include <stdio.h>

// Create a hexadecimal string representation of a uuid of the form
// 5 groups separated by hyphens, for a total of 36 characters.
// For example:
//   550e8400-e29b-41d4-a716-446655440000
std::string create_uuid_string()
{
  uuid_t uuid;
  uuid_create(&uuid);
  char buffer[40];
  sprintf(buffer, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", static_cast<unsigned int>(uuid.time_low), uuid.time_mid, uuid.time_hi_and_version,
          uuid.clock_seq_hi_and_reserved, uuid.clock_seq_low,
          uuid.node[0], uuid.node[1], uuid.node[2], uuid.node[3], uuid.node[4], uuid.node[5]);
  std::string result(buffer);
  return result;
}

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


#include "khFusionURI.h"

#include <khstl.h>
#include "khVolumeManager.h"

khFusionURI::khFusionURI(const std::string &uri)
{
  // looking for: khfile://<volume>/<path>

  // volume & path must have at least one char each
  if (uri.size() < 12) return;

  if (!StartsWith(uri, "khfile://")) return;

  // find separator between volume and path
  std::string::size_type sep = uri.find('/', 9);

  // no separator found
  if (sep == std::string::npos) return;

  // volume is empty
  if (sep == 9) return;

  // path is empty
  if (sep+1 == uri.size()) return;

  volume = uri.substr(9, sep - 9);
  path   = uri.substr(sep+1, std::string::npos);
}


std::string
khFusionURI::NetworkPath(void) const
{
  return theVolumeManager.NetworkPathOf(*this);
}

std::string
khFusionURI::LocalPath(void) const
{
  return theVolumeManager.LocalPathOf(*this);
}

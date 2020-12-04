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

//

#include "geLockfileGuard.h"
#include <khFileUtils.h>
#include <khException.h>

geLockfileGuard::geLockfileGuard(const std::string filename,
                                 const std::string &contents) :
    filename_(filename)
{
  if (!khWriteStringToFile(filename_, contents)) {
    throw khException(kh::tr("Unable to create lockfile %1")
                      .arg(filename_.c_str()));
  }
}

geLockfileGuard::~geLockfileGuard(void) {
  (void)khUnlink(filename_);
}

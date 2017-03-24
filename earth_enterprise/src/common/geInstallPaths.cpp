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

#include "geInstallPaths.h"
#include <khFileUtils.h>

const std::string kGEOptPath   = "/opt/google";
const std::string kGEEtcPath   = "/etc/opt/google";
const std::string kGEBinPath   = khComposePath(kGEOptPath, "bin");

const std::string kGEVarPath   = "/var/opt/google";
const std::string kGERunPath   = khComposePath(kGEVarPath, "run");
const std::string kGELogPath   = khComposePath(kGEVarPath, "log");

const std::string kGESharePath     = khComposePath(kGEOptPath, "share");
const std::string kGESiteIconsPath = khComposePath(kGESharePath, "site-icons");
const std::string kDbrootTemplatePath
  = khComposePath(kGESharePath, "dbroot/template");
